// scheduler.hpp

#ifndef __MDDL_SCHEDULER_HPP__
#define __MDDL_SCHEDULER_HPP__

#include "ief.hpp"
#include "midi_io.hpp"
#include "sequence.hpp"
#include "utils.hpp"

#include <thread>



namespace MDDL {



class Scheduler
{
public:
    typedef Sequence::Elem Note;

    struct Event
    {
        uint8_t pitch;
        uint8_t vel;
        int64_t wait;   // nanoseconds
    };

    static constexpr int SLEEP = 0; // ms

    Scheduler( MIDI::midi_out& midi_out );

    void set_channel( uint8_t c ) { channel = c; }
    void set_tempo( int bpm ) { tempo = bpm; update_conversions(); }
    void set_ppq( int ticks ) { ppq = ticks; update_conversions();}
    void update_conversions()
    {
        ticks_to_ns = 60.0 / tempo / ppq * 1'000'000'000;
    }

    void launch();
    void join();
    void add_sequence( Sequence& seq, int64_t start, int64_t length );
    std::list<Event>::iterator
    insert_event( Event& e, std::list<Event>::iterator start );

    void note_on( uint8_t pitch, uint8_t vel );
    void note_off( uint8_t pitch );

    void thread_run();

    void send_message( const Event& e );


    MIDI::midi_out&     midi_out;
    std::list<Event>    outgoing;
    std::mutex          outgoing_mtx;
    std::thread         thread;
    Clock               last_clock;
    uint8_t             channel         = 0;
    int                 tempo           = 0;
    int                 ppq             = 0;
    double              ticks_to_ns     = 0;
    bool                active          = false;
};

} // namespace MDDL

#endif // __MDDL_SCHEDULER_HPP__