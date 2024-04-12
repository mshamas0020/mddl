// interpreter.cpp

#include "ief.hpp"
#include "interpreter.hpp"

#include <fstream>
#include <functional>


namespace MDDL {

static MIDI::input_configuration make_input_config( Interpreter* mddl )
{
    MIDI::input_configuration config = {};
    config.on_message = std::bind(
        &Interpreter::on_message_callback, mddl, std::placeholders::_1
    );
    return config;
}

Interpreter::Interpreter( const MIDI::observer& obs )
    : midi_in   { make_input_config( this ), MIDI::midi_in_configuration_for( obs ) }
    , midi_out  { MIDI::output_configuration{}, midi_out_configuration_for( obs ) }
{}

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

    for (const auto& track : r.tracks) {
        for (const MIDI::track_event& event : track) {
            receive_message( event.m, event.tick );
        }
    }

    program.resolve_links();
}

void Interpreter::receive_message( const MIDI::message& msg, int64_t tick )
{
    syntax.process_msg( msg, tick );

    if ( !syntax.active_sltx() ) {
        std::cout << "\r";
        printer.print( syntax.cst, program.tail );
    }

    if ( syntax.pending_ast() ) {
        const bool success = program.add_ast( syntax.get_ast() );
        syntax.clear();

        if ( success )
            std::cout << "\n";
    }

    if ( program.slrx_pending() && !syntax.active_sltx() )
        syntax.set_sltx( program.slrx_pop() );
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
    if ( humanization == 0 )
    while ( true ) {
        msg_queue_mtx.lock();

        msg_queue_mtx.unlock();

        ief_wait( LISTEN_SLEEP_MS );
    }


    // humanization > 0

    while ( true ) {
        msg_queue_mtx.lock();

        if ( humanization == 0 ) {
            while ( msg_queue.size() > 0 ) {
                const MIDI::message& msg = msg_queue.front();
                receive_message( msg, msg.timestamp );
                msg_queue.pop();
            }
        } else {
            if ( msg_delay_remaining > 0 ) {
                msg_delay_remaining -= LISTEN_SLEEP_MS;
                if ( msg_delay_remaining <= 0 || syntax.active_sltx() ) {
                    receive_message( msg_delayed, msg_delayed.timestamp );
                    msg_delay_remaining = 0;
                }
            }

            while ( msg_queue.size() > 0 ) {
                const MIDI::message& msg = msg_queue.front();

                if ( syntax.active_sltx() ) {
                    receive_message( msg, msg.timestamp );
                } else switch( msg.get_message_type() ) {
                    case MIDI::message_type::NOTE_ON:
                        receive_message( msg, msg.timestamp );
                        if ( msg_delay_remaining > 0 ) { 
                            // insert note on before delayed note off
                            receive_message( msg_delayed, msg_delayed.timestamp );
                            msg_delay_remaining = 0;
                        }
                        break;
                    case MIDI::message_type::NOTE_OFF:
                        if ( msg_delay_remaining > 0 )
                            // received delayed note off and replace with new delayed note off
                            receive_message( msg_delayed, msg_delayed.timestamp );
                        msg_delayed = msg;
                        msg_delay_remaining = humanization;
                        break;
                    default:
                        receive_message( msg, msg.timestamp );
                        break;
                }

                msg_queue.pop();
            }
        }

        msg_queue_mtx.unlock();

        ief_wait( LISTEN_SLEEP_MS );
    }
}

void Interpreter::run()
{
    DataRef output = runtime.execute( program.global );
    std::cout << "\nOutput:\n";
    output.print();
    output.ref->print();
}

void Interpreter::print() const
{
    program.print();
}

} // namespace MDDL

