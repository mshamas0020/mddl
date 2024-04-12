// midi_io.cpp

#include "midi_io.hpp"

#include <iostream>

namespace MDDL {

namespace MIDI = libremidi;

std::vector<MIDI::input_port> MIDI_input_ports( const MIDI::observer& obs )
{
    std::vector<MIDI::input_port> ports = obs.get_input_ports();

    std::sort( ports.begin(), ports.end(), []( const MIDI::input_port& a, const MIDI::input_port& b ) {
        return a.port_name < b.port_name;
    } );

    return ports;
}

std::vector<MIDI::output_port> MIDI_output_ports( const MIDI::observer& obs )
{
    std::vector<MIDI::output_port> ports = obs.get_output_ports();
    
    std::sort( ports.begin(), ports.end(), []( const MIDI::output_port& a, const MIDI::output_port& b ) {
        return a.port_name < b.port_name;
    } );
    return ports;
}

void print_ports(
    const std::vector<MIDI::input_port>& ports_in,
    const std::vector<MIDI::output_port>& ports_out
) {
    if ( ports_in.size() == 0 ) {
        std::cout << "No input ports available.\n";
    } else {
        std::cout << "Input ports:\n";

        for ( int i = 0; i < (int )ports_in.size(); i++ ) {
            const std::string& name = ports_in[i].port_name;

            if ( !name.empty() )
                std::cout << i << " - " << name << "\n";
        }
    }

    std::cout << "\n";

    if ( ports_out.size() == 0 ) {
        std::cout << "No output ports available.\n";
    } else {
        std::cout << "Output ports:\n";

        for ( int i = 0; i < (int )ports_out.size(); i++ ) {
            const std::string& name = ports_out[i].port_name;
            
            if ( !name.empty() )
                std::cout << i << " - " << name << "\n";
        }
    }
}

fs::path find_file( const fs::path& name )
{
    if ( fs::exists( name ) )
        return name;

    const fs::path lib_name = lib_path / name;

    if ( fs::exists( lib_name ) )
        return lib_name;

    std::cout << "Could not find " << name << ".\n";
    
    exit( 0 );
}

} // namespace MDDL