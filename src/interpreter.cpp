// interpreter.cpp

#include "errors.hpp"
#include "ief.hpp"
#include "interpreter.hpp"

#include <fstream>
#include <functional>

#include <ctime>


namespace MDDL {

static MIDI::input_configuration make_input_config( Interpreter* mddl )
{
    MIDI::input_configuration config = {};
    //config.timestamps = MIDI::timestamp_mode::SystemMonotonic;
    config.on_message = std::bind(
        &Interpreter::on_message_callback, mddl, std::placeholders::_1
    );
    return config;
}

Interpreter::Interpreter( const MIDI::observer& obs )
    : midi_in   { make_input_config( this ), MIDI::midi_in_configuration_for( obs ) }
    , midi_out  { MIDI::output_configuration{}, midi_out_configuration_for( obs ) }
    , runtime( &scheduler )
    , scheduler( midi_out )
{
    set_channel( ps.channel );
    set_tempo( ps.tempo );
    set_ppq( ps.ppq );
    scheduler.launch();

    all_notes_off();
}

Interpreter::~Interpreter()
{
    // TODO kill execution thread
    scheduler.join();
}

void Interpreter::join()
{
    if ( exec_thread.joinable() )
        exec_thread.join();
}

void Interpreter::all_notes_off()
{
    midi_out.send_message( MIDI::channel_events::control_change( ps.channel, 123, 0 ) );

    for ( int n = 0; n < N_MIDI_NOTES; n ++ )
        midi_out.send_message( MIDI::channel_events::note_off( ps.channel, n, 0 ) );
}

void Interpreter::set_channel( uint8_t c )
{
    ps.channel = c;
    scheduler.set_channel( c );
}

void Interpreter::set_tempo( int bpm )
{
    ps.tempo = bpm;
    syntax.set_tempo( bpm );
    scheduler.set_tempo( bpm );
}

void Interpreter::set_ppq( int ticks )
{
    ps.ppq = ticks;
    syntax.set_ppq( ticks );
    scheduler.set_ppq( ticks );
}

void Interpreter::on_message_callback( const MIDI::message& msg )
{
    const std::lock_guard<std::mutex> lock( msg_queue_mtx );
    msg_queue.push( msg );
}

void Interpreter::read_smf( const fs::path& path )
{
    std::ifstream file( path, std::ios::binary );

    if ( !file.is_open() ) {
        std::cout << "Could not open file " << path << ".\n";
        return;
    }

    std::vector<uint8_t> bytes;
    bytes.assign(
        std::istreambuf_iterator<char>( file ),
        std::istreambuf_iterator<char>()
    );

    // Initialize our reader object
    MIDI::reader r( true );

    // Parse
    MIDI::reader::parse_result result = r.parse( bytes );

    if ( result == MIDI::reader::invalid )
        return;

    if ( syntax.active_sltx() ) {
        syntax.force_sltx();

        for (const auto& track : r.tracks)
        for (const MIDI::track_event& event : track)
            syntax.process_msg( event.m, event.tick );

        syntax.close_sltx();
        return;
    }

    for (const auto& track : r.tracks)
    for (const MIDI::track_event& event : track) {
        syntax.process_msg( event.m, event.tick );

        if ( syntax.pending_ast() ) {
            // printer.print( syntax.cst );
            // std::cout << "\n";
            program.add_ast( syntax.get_ast() );
            syntax.clear();
        }

        if ( program.slrx_pending() && !syntax.active_sltx() )
            syntax.set_sltx( program.slrx_pop() );
    }

    program.resolve_links();
}

void Interpreter::receive_message( const MIDI::message& msg )
{
    syntax.process_msg( msg, msg.timestamp );

    if ( !syntax.active_sltx() )
        repl_display_line();

    if ( syntax.pending_ast() ) {
        if ( exec_thread.joinable() ) {
            std::cout << "... \r";
            exec_thread.join();
            std::cout << "  > \r";
        }

        bool success = program.add_ast( syntax.get_ast() );
        syntax.clear();

        if ( success )
            run_tail();
    }

    if ( program.slrx_pending() && !syntax.active_sltx() )
        syntax.set_sltx( program.slrx_pop() );
}

void Interpreter::repl_display_line()
{
    std::cout << "  > ";
    printer.print( syntax.cst );
    std::cout << "\r";
}

void Interpreter::open_port_in( const MIDI::input_port& port_in )
{
    midi_in.open_port( port_in );

    if ( !midi_in.is_port_connected() ) {
        std::cout << "Could not connect to input port \""
            << port_in.port_name << "\".\n";
        exit( 0 );
    }
}

void Interpreter::open_port_out( const MIDI::output_port& port_out )
{
    midi_out.open_port( port_out );

    if ( !midi_out.is_port_connected() ) {
        std::cout << "Could not connect to output port \""
            << port_out.port_name << "\".\n";
        exit( 0 );
    }
}

void Interpreter::listen()
{
    std::cout << "  > ";
    std::cout << "\r";

    while ( true ) {
        const Clock clock = Time::now();
        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>( clock - last_clock );
        int64_t ticks = (int64_t )time.count();
        last_clock = clock;

        if ( msg_delay_remaining > 0 ) {
            // delay has finished, receive note off
            msg_delay_remaining -= ticks;
            if ( msg_delay_remaining <= 0 || syntax.active_sltx() ) {
                receive_message( msg_delayed );
                msg_delay_remaining = 0;
            }
        }

        msg_queue_mtx.lock();

        // process new messages
        while ( msg_queue.size() > 0 ) {
            const MIDI::message msg = msg_queue.front();
            const uint8_t vel = msg.bytes[2];
            msg_queue.pop();

            if ( syntax.active_sltx() ) {
                receive_message( msg );
                continue;
            }
            
            switch( msg.get_message_type() ) {
                case MIDI::message_type::NOTE_ON:
                    if ( vel == 0 ) {
                        receive_note_off( msg );
                    } else {
                        receive_note_on( msg );
                    }
                    break;
                case MIDI::message_type::NOTE_OFF:
                    receive_note_off( msg );
                    break;
                default:
                    receive_message( msg );
                    break;
            }
        }

        msg_queue_mtx.unlock();

        ief_sleep( LISTEN_SLEEP_MS );
    }
}

void Interpreter::receive_note_on( const MIDI::message& msg )
{
    const uint8_t note = msg.bytes[1];

    if ( msg_delay_remaining > 0 ) {
        // there is a currently held delayed message
        if ( note == prev_note_on ) {
            // do not reorder same notes
            receive_message( msg_delayed );
            receive_message( msg );
        } else {
            // insert note on before delayed note off
            receive_message( msg );
            receive_message( msg_delayed );
        }
        msg_delay_remaining = 0;
    } else {
        receive_message( msg );
    }
    
    prev_note_on = note;
}

void Interpreter::receive_note_off( const MIDI::message& msg )
{
    const uint8_t note = msg.bytes[1];

    if ( msg_delay_remaining > 0 )
        // received delayed note off
        receive_message( msg_delayed );

    if ( note == prev_note_on ) {
        // and replace with new delayed note off
        msg_delayed = msg;
        msg_delay_remaining = ps.humanization * 1'000'000; // ms to ns
        return;
    }
       
    receive_message( msg );
}

void Interpreter::thread_run( const ExprRoot* entry )
{
    if ( entry == nullptr )
        return;
    
    program.resolve_links();
    runtime.push_scope( program.global );

    std::cout << "\n";
    DataRef v = DataType::ERROR;
    try {
        v = runtime.execute( entry );
    } catch ( const std::exception& err ) {
        std::cout << err.what() << "\n";
    }
    repl_print( v );
    
    if ( !v.empty() ) {
        std::lock_guard<std::mutex> seq_guard( v.mtx() );
        scheduler.add_sequence( v.get(), v.start, v.length() );
    }

    v.release();
}

void Interpreter::launch_thread_run( const ExprRoot* entry )
{
    assert( !exec_thread.joinable() );

    exec_thread = std::thread( &Interpreter::thread_run, this, entry );
}

void Interpreter::run_head()
{
    launch_thread_run( program.global->head );
}

void Interpreter::run_tail()
{
    launch_thread_run( program.global->tail );
}

void Interpreter::repl_print( const DataRef& ref )
{
    std::cout << "[";
    switch ( ref.type ) {
        case DataType::SEQ:
        case DataType::VSEQ:
        case DataType::SEQ_LIT:
        case DataType::ATTR:
        case DataType::VATTR:
            std::cout << ref.length();
            break;
        case DataType::VALUE:
            std::cout << ref.value;
            break;
        case DataType::ERROR:
        case DataType::UNDEFINED:
            std::cout << "undefined";
            break;
        default: break;
    }
    std::cout << "]\n";
}

void Interpreter::stop()
{
    runtime.pop_scope( program.global );
}

void Interpreter::print() const
{
    program.print();
}

} // namespace MDDL

