// sequence.hpp

#ifndef __MDDL_SEQUENCE_HPP__
#define __MDDL_SEQUENCE_HPP__

#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <mutex>
#include <type_traits>
#include <vector>


namespace MDDL {

struct Sequence
{
    struct Elem
    {
        uint8_t     pitch       = 0;
        uint8_t     vel         = 0;
        int32_t     dur         = 0;
        int32_t     wait        = 0;

        bool operator==( const Elem& rhs ) const;
        void operator+=( const Elem& rhs );
        void operator-=( const Elem& rhs );
        void operator*=( const Elem& rhs );
        void operator/=( const Elem& rhs );
    };

    typedef std::vector<Elem> Data;

    Sequence();
    Sequence( int64_t value );
    Sequence( const Elem& elem, int64_t size = 1 );
    Sequence( const Sequence& rhs, int64_t rhs_start, int64_t rhs_length );

    void note_on( uint8_t pitch, uint8_t vel, int64_t wait );
    void note_hold( int64_t idx, int64_t duration );
    void mark_complete() { complete = true; }

    std::vector<Elem> get_data() const;

    Elem& at( int64_t idx );
    const Elem& at( int64_t idx ) const { return at( idx ); };

    bool empty() const { return (size == 0); }

    Elem& front() { return at( 0 ); }
    const Elem& front() const { return at( 0 ); }
    Elem& back() { return at( size - 1 ); }
    const Elem& back() const { return at( size - 1 ); }

    void expand();
    std::vector<Elem> expanded() const;
    void resize( int64_t end );
    void expect( int64_t end );
    void crop( int64_t start, int64_t length );

    void mask( AttrType attr );
    template <auto M>
    void mask_attr();

    void assign( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );
    void assign( AttrType attr, AttrType rhs_attr, int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );
    template <auto M1, auto M2>
    void assign_attr( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );

    void assign_value( AttrType attr, int64_t start, int64_t length, int64_t value );
    template <auto M>
    void assign_value_attr( int64_t start, int64_t length, int64_t value );

    int64_t value();
    int64_t value( AttrType attr );
    template <auto M>
    int64_t value_attr();
    
    void concat( const Sequence& rhs, int64_t rhs_start, int64_t rhs_length );
    void concat( AttrType attr, AttrType rhs_attr, const Sequence& rhs, int64_t rhs_start, int64_t rhs_length );
    template <auto M1, auto M2>
    void concat_attr( const Sequence& rhs, int64_t rhs_start, int64_t rhs_length );
    
    void extend( int64_t length );

    void add( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );
    void add( AttrType attr, AttrType rhs_attr, int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );
    template <auto M1, auto M2>
    void add_attr( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );

    void add_value( AttrType attr, int64_t start, int64_t length, int64_t value );
    template <auto M>
    void add_value_attr( int64_t start, int64_t length, int64_t value );

    void subtract( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );
    void subtract( AttrType attr, AttrType rhs_attr, int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );
    template <auto M1, auto M2>
    void subtract_attr( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );

    void subtract_value( AttrType attr, int64_t start, int64_t length, int64_t value );
    template <auto M>
    void subtract_value_attr( int64_t start, int64_t length, int64_t value );

    void multiply( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );
    void multiply( AttrType attr, AttrType rhs_attr, int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );
    template <auto M1, auto M2>
    void multiply_attr( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );

    void multiply_value( AttrType attr, int64_t start, int64_t length, int64_t value );
    template <auto M>
    void multiply_value_attr( int64_t start, int64_t length, int64_t value );

    void divide( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );
    void divide( AttrType attr, AttrType rhs_attr, int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );
    template <auto M1, auto M2>
    void divide_attr( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length );

    void divide_value( AttrType attr, int64_t start, int64_t length, int64_t value );
    template <auto M>
    void divide_value_attr( int64_t start, int64_t length, int64_t value );

    void print();

    Data        data        = {};
    Elem        comp        = {};
    int64_t     size        = 0;
    int32_t     ref_count   = 0;
    bool        compressed  = true;
    bool        complete    = true;

    std::mutex  mtx;
};

} // namepspace MDDL

#endif // __MDDL_SEQUENCE_HPP__