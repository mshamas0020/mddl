// midi_io.hpp

#ifndef __MDDL_MIDI_IO_HPP__
#define __MDDL_MIDI_IO_HPP__

#include "libremidi/libremidi.hpp"

#include <filesystem>


namespace MDDL {
    
namespace fs = std::filesystem;
namespace MIDI = libremidi;

static const fs::path lib_path { "lib" };



std::vector<MIDI::input_port> MIDI_input_ports( const MIDI::observer& obs );

std::vector<MIDI::output_port> MIDI_output_ports( const MIDI::observer& obs );

void print_ports(
    const std::vector<MIDI::input_port>& ports_in,
    const std::vector<MIDI::output_port>& ports_out
);

fs::path find_file( const fs::path& name );

} // namespace MDDL

#endif // __MDDL_MIDI_IO_HPP__