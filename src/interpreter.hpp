// interpreter.hpp

#ifndef __MDDL_INTERPRETER_HPP__
#define __MDDL_INTERPRETER_HPP__

#include "environment.hpp"
#include "printer.hpp"
#include "runtime.hpp"
#include "scheduler.hpp"
#include "utils.hpp"

#include "midi_io.hpp"
#include "libremidi/reader.hpp"

#include <filesystem>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>


namespace MDDL {

class Interpreter
{
public:
    struct {
        uint8_t channel         = 0;
        int     humanization    = 50; // ms
        int     tempo           = 120; // bpm
        int     ppq             = 960; // ticks per quarter note
    } ps;

    static constexpr int    LISTEN_SLEEP_MS = 0; // ms

    Interpreter( const MIDI::observer& obs );
    ~Interpreter();

    void set_channel( uint8_t c );
    void set_tempo( int bpm );
    void set_ppq( int ticks );

    void all_notes_off();

    void open_port_in( const MIDI::input_port& port );
    void open_port_out( const MIDI::output_port& port );

    void read_smf( const fs::path& path );
    void receive_message( const MIDI::message& msg );
    void receive_note_on( const MIDI::message& msg );
    void receive_note_off( const MIDI::message& msg );
    void on_message_callback( const MIDI::message& msg );

    void run_head();
    void run_tail();
    void thread_run( const ExprRoot* entry );
    void launch_thread_run( const ExprRoot* entry );

    void begin();
    void listen();
    void repl_display_line();
    void repl_run();
    void repl_print( const DataRef& ref );
    void stop();

    void print() const;

private:
    MIDI::midi_in       midi_in;
    MIDI::midi_out      midi_out;

    Runtime             runtime;
    StaticEnvironment   program;
    SyntaxParser        syntax;
    Scheduler           scheduler;
    Printer             printer;

    std::thread         exec_thread;

    Clock               last_clock;
    std::mutex          msg_queue_mtx;
    std::queue<MIDI::message>
                        msg_queue;
    MIDI::message       msg_delayed;
    int64_t             msg_delay_remaining = 0;
    uint8_t             prev_note_on        = 0;
};

} // namespace MDDL

#endif // __MDDL_INTERPRETER_HPP__