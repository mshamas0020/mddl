// interpreter.hpp

#ifndef __MDDL_INTERPRETER_HPP__
#define __MDDL_INTERPRETER_HPP__

#include "environment.hpp"
#include "printer.hpp"
#include "runtime.hpp"

#include "midi_io.hpp"
#include "libremidi/reader.hpp"

#include <filesystem>
#include <iostream>
#include <mutex>
#include <queue>


namespace MDDL {

class Interpreter
{
public:
    static constexpr int    LISTEN_SLEEP_MS = 10; // ms

    Interpreter( const MIDI::observer& obs );

    void open_port_in( const MIDI::input_port& port );
    void open_port_out( const MIDI::output_port& port );

    void read_smf( const fs::path& path );
    void receive_message( const MIDI::message& msg, int64_t tick );
    void on_message_callback( const MIDI::message& msg );

    void listen();
    void run();

    void print() const;

private:
    Runtime             runtime;
    StaticEnvironment   program;
    SyntaxParser        syntax;
    Printer             printer;

    MIDI::midi_in       midi_in;
    MIDI::midi_out      midi_out;

    std::mutex          msg_queue_mtx;
    std::queue<MIDI::message>
                        msg_queue;
    MIDI::message       msg_delayed;
    int                 msg_delay_remaining = 0;

    int                 humanization = 100; // ms

};

} // namespace MDDL

#endif // __MDDL_INTERPRETER_HPP__