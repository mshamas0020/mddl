// main.cpp

#include "args/args.hxx"
#include "midi_io.hpp"
#include "syntax.hpp"
#include "environment.hpp"
#include "interpreter.hpp"

//#include "cpp-terminal/terminal.h"

#include "libremidi/reader.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>



static const char* MDDL_VERSION = "v0.1.0";

namespace fs = std::filesystem;

using namespace MDDL;

int main( int argc, char** argv )
{
    args::ArgumentParser parser( "MIDI Dynamic Development Language.", "" );
    args::PositionalList<std::string> args_filenames( parser, "files",
        "MIDI files to be used as input." );
    args::ValueFlag<int> args_port_in( parser, "port",
        "Input MIDI port enumeration.", { 'i', "input" } );
    args::ValueFlag<int> args_port_out( parser, "port",
        "Output MIDI port enumeration.", { 'o', "output" } );
    args::ValueFlag<int> args_filename_out( parser, "filename",
        "Write output to standard MIDI file.", { 'w', "write" } );
    //args::Flag args_verbose( parser, "verbose",
    //    "Print additional output.", { 'v', "verbose" } );
    args::Flag args_version( parser, "version",
        "Print MDDL version.", { "version" } );
    args::Flag args_quiet( parser, "quiet",
        "Mute all output.", { 'q', "quiet" } );
    args::Flag args_ports( parser, "ports",
        "List all available MIDI ports.", { "ports" } );
    args::Flag args_time( parser, "time",
        "Time input files.", { "time" } );
    args::Flag args_translate( parser, "translate",
        "Print text syntax translation of input files without executing.",
        { "translate" } );
    args::HelpFlag arg_help( parser, "help",
        "Show this help page.", { 'h', "help" } );

    try {
        parser.ParseCLI( argc, argv );
    } catch ( ... ) {
        std::cout << parser;
        return 0;
    }
    
    if ( arg_help ) {
        std::cout << parser;
        return 0;
    }

    if ( args_version ) {
        std::cout << "MDDL " << MDDL_VERSION << "\n";
        return 0;
    }

    MIDI::observer obs;
    const auto ports_in = MIDI_input_ports( obs );
    const auto ports_out = MIDI_output_ports( obs );

    if ( args_ports ) {
        print_ports( ports_in, ports_out );
        return 0;
    }

    Interpreter mddl( obs );

    if ( args_port_in ) {
        const int port_idx = args::get( args_port_in );
        if ( port_idx < 0 || port_idx >= (int )ports_in.size() ) {
            std::cout << "Error: Invalid input port. Use enumeration below:\n";
            print_ports( ports_in, ports_out );
            return 0;
        }

        mddl.open_port_in( ports_in[port_idx] );
    }

    if ( args_port_out ) {
        const int port_idx = args::get( args_port_out );
        if ( port_idx < 0 || port_idx >= (int )ports_out.size() ) {
            std::cout << "Error: Invalid output port. Use enumeration below:\n";
            print_ports( ports_in, ports_out );
            return 0;
        }

        mddl.open_port_out( ports_out[port_idx] );
    }

    [[maybe_unused]] const auto start_clock = std::chrono::steady_clock::now();

    // process input files

    for ( const std::string& filename : args::get( args_filenames ) ) {
        const fs::path smf = find_file( filename );
        mddl.read_smf( smf );
    }

    if ( args_translate ) {
        mddl.print();
        return 0;
    }

    [[maybe_unused]] const auto run_clock = std::chrono::steady_clock::now();

    mddl.run_head();

    if ( args_time ) {
        mddl.join();
        const std::chrono::duration<float>  run_time = std::chrono::steady_clock::now() - run_clock;
        const std::chrono::duration<float>  total_time = std::chrono::steady_clock::now() - start_clock;
        std::cout << "Run Time: " << run_time.count() << "s\n";
        std::cout << "Total Time: " << run_time.count() << "s\n";
        return 0;
    }

    if ( !args_port_in )
        return 0;

    // enter REPL

    std::cout << "Welcome to MDDL " << MDDL_VERSION << "\n";
    mddl.listen();
    mddl.join();

    return 0;
}