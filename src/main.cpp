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

    MIDI::observer obs;
    const auto ports_in = MIDI_input_ports( obs );
    const auto ports_out = MIDI_output_ports( obs );

    if ( args_ports ) {
        print_ports( ports_in, ports_out );
        return 0;
    }

    Interpreter mddl( obs );

    std::clock_t start_clock = std::clock();

    // process input files

    for ( const std::string& filename : args::get( args_filenames ) ) {
        const fs::path smf = find_file( filename );
        mddl.read_smf( smf );
    }
    
    mddl.print();

    std::clock_t run_clock = std::clock();

    mddl.run();

    std::cout << "Runtime: " << (float )(std::clock() - run_clock) / CLOCKS_PER_SEC * 1000.f << " ms\n";
    std::cout << "Total: " << (float )(std::clock() - start_clock) / CLOCKS_PER_SEC * 1000.f << " ms\n";

    if ( true ) {//if ( args_port_in ) {
        const int port_idx = 0;//args::get( args_port_in );
        if ( port_idx < 0 || port_idx >= (int )ports_in.size() ) {
            std::cout << "Error: Invalid input port. Use enumeration below:\n";
            print_ports( ports_in, ports_out );
            return 0;
        }

        mddl.open_port_in( ports_in[args::get( args_port_in )] );
    } else {
        return 0;
    }

    while ( true ) {
        mddl.listen();
    }

    // enter REPL

    return 0;
}