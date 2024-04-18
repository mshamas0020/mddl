// utils.hpp

#ifndef __MDDL_UTILS_HPP__
#define __MDDL_UTILS_HPP__

#include "common.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <type_traits>



namespace MDDL {

static constexpr int OCTAVE = 12;

using Symbol = std::string;
using Time = std::chrono::high_resolution_clock;
using Clock = Time::time_point;

template <typename T>
using SymbolMap = std::map<Symbol, T>;

inline const char* note_to_str( uint8_t note )
{
    const char* arr[OCTAVE] = {
        "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b"
    };

    return arr[note % OCTAVE];
}

inline std::string symbol_to_str( const Symbol& s )
{
    int note = (int )(uint8_t )s[0];

    std::string str = note_to_str( note );
    for ( int i = 1; i < (int )s.size(); i ++ ) {
        const int dist = std::abs( (int )s[i] );
        note += (int )s[i];
        if ( (int )s[i] < 0 )
            str += "_";
        if ( dist >= OCTAVE )
            str += std::to_string( dist / OCTAVE * 8 );
        if ( note < 0 )
            note = (note % OCTAVE) + OCTAVE;
        str += note_to_str( note );
    }

    return str;
};

inline OpId note_to_op_id( uint8_t note, uint8_t root )
{
    static constexpr OpId id_map[OCTAVE] = {
        OP_DO,  // root
        OP_RE,  // minor 2nd
        OP_RE,  // major 2nd
        OP_MI,  // minor 3rd
        OP_MI,  // major 3rd
        OP_FA,  // perfect 4th
        OP_SO,  // diminished 5th
        OP_SO,  // perfect 5th
        OP_LA,  // minor 6th
        OP_LA,  // major 6th
        OP_TI,  // minor 7th
        OP_TI   // major 7th
    };

    const uint8_t interval = (note - root) % OCTAVE;
    return id_map[interval];
}

enum class DataType : uint8_t
{
    UNKNOWN, NONE, UNDEFINED, VOID, SEQ, VSEQ, SEQ_LIT, ATTR, VATTR, VALUE, INDEXER, ERROR
};

enum class AttrType : uint8_t
{
    ALL, PITCH, VELOCITY, DURATION, WAIT
};

// <from, to>
// cast to wider type only
static const std::map<DataType, DataType> implicit_cast_book = {
    { DataType::SEQ_LIT, DataType::SEQ },
    { DataType::SEQ, DataType::VSEQ },
    { DataType::ATTR, DataType::VATTR }
};

inline bool has_implicit_cast( DataType type )
{
    return implicit_cast_book.contains( type );
}

inline DataType implicit_cast( DataType type )
{
    return implicit_cast_book.at( type );
}

// from, to
inline bool may_implicit_cast( DataType a, DataType b )
{
    if ( a == b ) return true;
    while ( has_implicit_cast( a ) ) {
        a = implicit_cast( a );
        if ( a == b ) return true;
    }
    return false;
}

inline const char* dt_to_string( DataType type )
{
    switch ( type ) {
        case DataType::UNKNOWN: return "UNKNOWN";
        case DataType::NONE: return "NONE";
        case DataType::UNDEFINED: return "UNDEFINED";
        case DataType::VOID: return "VOID";
        case DataType::SEQ: return "SEQ";
        case DataType::VSEQ: return "VSEQ";
        case DataType::SEQ_LIT: return "SEQ_LIT";
        case DataType::ATTR: return "ATTR";
        case DataType::VATTR: return "VATTR";
        case DataType::VALUE: return "VALUE";
        case DataType::INDEXER: return "INDEXER";
        default: break;
    }

    return "ERROR";
}

inline const char* attr_to_string( AttrType attr )
{
    switch ( attr ) {
        case AttrType::ALL: return "ALL";
        case AttrType::PITCH: return "PITCH";
        case AttrType::VELOCITY: return "VELOCITY";
        case AttrType::DURATION: return "DURATION";
        case AttrType::WAIT: return "WAIT";
        default: break;
    }

    return "";
}

inline DataType to_copy_type( DataType type )
{
    switch ( type ) {
        case DataType::SEQ_LIT:
        case DataType::SEQ: return DataType::VSEQ;
        case DataType::ATTR: return DataType::VATTR;
        default: break;
    }

    return type;
}

template <typename M>
struct member_type {
    template <typename C, typename T>
    static T get_type(T C::*v);

    typedef decltype( get_type( static_cast<M>( nullptr ) ) ) type;
};

template <typename M>
using member_t = member_type<M>::type;

}

#endif // __MDDL_UTILS_HPP__