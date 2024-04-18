// scheduler.cpp

#include "scheduler.hpp"

namespace MDDL {

Scheduler::Scheduler( MIDI::midi_out& midi_out )
    : midi_out  { midi_out }
{}

void Scheduler::launch()
{
    active = true;
    last_clock = Time::now();
    thread = std::thread( &Scheduler::thread_run, this );
}

void Scheduler::join()
{
    active = false;
    thread.join();
}

void Scheduler::thread_run()
{
    while ( true ) {
        const Clock clock = Time::now();
        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>( clock - last_clock );
        int64_t ns = (int64_t )time.count();
        last_clock = clock;

        int64_t n_outgoing = 0;

        {
            std::lock_guard<std::mutex> guard( outgoing_mtx );

            while ( ns > 0 && outgoing.size() > 0 ) {
                Event& e = outgoing.front();

                if ( ns > e.wait ) {
                    ns -= e.wait;
                    send_message( e );
                    outgoing.pop_front();
                } else {
                    e.wait -= ns;
                    break;
                }
            }
            n_outgoing = outgoing.size();
        }

        if ( !active && n_outgoing == 0 )
            break;

        ief_sleep( SLEEP );
    }
}

void Scheduler::add_sequence( Sequence& seq, int64_t start, int64_t length )
{

    std::lock_guard<std::mutex> guard( outgoing_mtx );

    if ( seq.compressed && seq.comp.vel == 0 )
        return;

    const std::vector<Note>& data = seq.get_data();

    auto search = outgoing.begin(); // insert search start
    
    auto data_start = data.cbegin() + start;
    auto data_end = data_start + length;

    for ( auto itr = data_start; itr < data_end; itr ++ ) {
        if ( itr->vel == 0 )
            continue;

        Event e;
        e.pitch = itr->pitch;
        e.vel = itr->vel;
        e.wait = (int64_t )(itr->wait * ticks_to_ns);
        search = insert_event( e, search );

        e.wait = (int64_t )(itr->dur * ticks_to_ns);
        e.vel = 0;
        insert_event( e, std::next( search ) );
        search ++;
    }
}

std::list<Scheduler::Event>::iterator
Scheduler::insert_event( Event& e, std::list<Event>::iterator start )
{
    auto itr = start;
    while ( itr != outgoing.end() ) {
        Event& other = *itr;
        if ( e.wait < other.wait ) {
            outgoing.insert( itr, e );
            other.wait -= e.wait;
            return std::prev( itr );
        } else {
            e.wait -= other.wait;
        }

        itr ++;
    }

    outgoing.push_back( e );
    return std::prev( outgoing.end() );
}

void Scheduler::note_on( uint8_t pitch, uint8_t vel )
{
    midi_out.send_message( MIDI::channel_events::note_on( channel, pitch, vel ) );
}

void Scheduler::note_off( uint8_t pitch )
{
    midi_out.send_message( MIDI::channel_events::note_off( channel, pitch, 0 ) );
}

void Scheduler::send_message( const Event& e )
{
    if ( e.vel > 0 ) {
        note_on( e.pitch, e.vel );
    } else {
        note_off( e.pitch );
    }
}

} // namespace MDDL