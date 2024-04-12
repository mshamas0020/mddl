// operations.cpp

#include "errors.hpp"
#include "ief.hpp"
#include "operations.hpp"
#include "runtime.hpp"

#include <cassert>
#include <iostream>


namespace MDDL {

static const char* SUBSEQ_BOUNDS_ERR    = "Cannot write outside bounds of subsequence.";
static const char* SUBSEQ_RESIZE_ERR    = "Cannot resize subsequence.";
static const char* SUBSEQ_CONCAT_ERR    = "Cannot concatenate to subsequence.";
static const char* INDEX_BOUNDS_ERR     = "Index is outside sequence bounds.";

static const char* NEW         = "NEW";
static const char* COMPLETE    = "COMPLETE";
static const char* ASSIGN      = "ASSIGN";
static const char* SET         = "SET";
static const char* RESIZE      = "RESIZE";
static const char* VALUE       = "VALUE";
static const char* CONCAT      = "CONCAT";
static const char* EXTEND      = "EXTEND";
static const char* INDEX       = "INDEX";
static const char* LENGTH      = "LENGTH";
static const char* COMPARE     = "COMPARE";
static const char* PITCH       = "PITCH";
static const char* ADD         = "ADD";
static const char* VELOCITY    = "VELOCITY";
static const char* SUBTRACT    = "SUBTRACT";
static const char* DURATION    = "DURATION";
static const char* MULTIPLY    = "MULTIPLY";
static const char* WAIT        = "WAIT";
static const char* DIVIDE      = "DIVIDE";


inline DataRef& get_stack_ref( Runtime* rt, DataRef ref )
{
    return rt->stack[ref.stack_pos];
}

static int64_t compare( int64_t a, int64_t b )
{
    return (int16_t )(a < b);
}

// Function implementations

#define MDDL_OP_FN( group, lhs_t, rhs_t )    \
    impl_ ## group ## _ ## lhs_t ## _ ## rhs_t

#define MDDL_OP_IMPL( group, name, lhs_t, rhs_t, return_t ) \
    DataRef MDDL_OP_FN( group, lhs_t, rhs_t )( Runtime* rt [[maybe_unused]], DataRef& lhs [[maybe_unused]], DataRef& rhs [[maybe_unused]] )



// DO, or NEW/ASSIGN
MDDL_OP_IMPL( OP_DO, NEW, VSEQ, NONE, VSEQ )
{
    return lhs.elide_copy();
}

MDDL_OP_IMPL( OP_DO, NEW, VALUE, NONE, VSEQ )
{
    return DataRef( DataType::VSEQ, new Sequence( lhs.value ) );
}

MDDL_OP_IMPL( OP_DO, COMPLETE, SEQ_LIT, NONE, VSEQ )
{
    while ( !lhs.get().complete )
        ief_wait( 10 );

    return lhs.elide_copy();
}

MDDL_OP_IMPL( OP_DO, ASSIGN, SEQ, SEQ, SEQ )
{
    DataRef& var = get_stack_ref( rt, lhs.move() );
    lhs.release();
    var.release();
    var.take( rhs.move() );
    return var.duplicate();
}

MDDL_OP_IMPL( OP_DO, SET, SEQ, VSEQ, SEQ )
{
    if ( lhs.is_subseq() ) {
        DataRef v = lhs.move();

        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );

        v.get().assign( v.start, rhs.get(), rhs.start, rhs.length() );
        return v;
    }

    DataRef& var = get_stack_ref( rt, lhs );
    lhs.release();
    var.take( rhs.elide_copy() );
    return var.duplicate();
}

MDDL_OP_IMPL( OP_DO, SET, SEQ, VATTR, SEQ )
{
    DataRef v = lhs.move();

    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().assign( rhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_DO, RESIZE, SEQ, VALUE, SEQ )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_RESIZE_ERR );

    v.get().resize( rhs.value );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_DO, SET, VSEQ, VSEQ, VSEQ )
{
    lhs.release();
    return rhs.elide_copy();
}

MDDL_OP_IMPL( OP_DO, SET, VSEQ, VATTR, VSEQ )
{
    DataRef v = lhs.elide_copy();

    v.get().expect( rhs.length() );
    v.get().assign( rhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_DO, RESIZE, VSEQ, VALUE, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.ref->resize( rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_DO, SET, ATTR, VSEQ, ATTR )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().assign( v.attr, v.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_DO, SET, ATTR, VATTR, ATTR )
{
    DataRef v = lhs.move();

    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.ref->expect( rhs.length() );
    }

    v.get().assign( v.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_DO, SET, ATTR, VALUE, ATTR )
{
    DataRef v = lhs.move();
    v.get().assign_value( v.attr, v.start, v.length(), rhs.value );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_DO, SET, VATTR, VSEQ, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().assign( v.attr, v.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_DO, SET, VATTR, VATTR, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().assign( v.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_DO, SET, VATTR, VALUE, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().assign_value( v.attr, v.start, v.length(), rhs.value );
    rhs.release();
    return v;
}



// RE, or VALUE/CONCAT/INDEX
MDDL_OP_IMPL( OP_RE, VALUE, VSEQ, NONE, VALUE )
{
    DataRef v( lhs.get().value() );
    lhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, VALUE, VATTR, NONE, VALUE )
{
    DataRef v( lhs.get().value( lhs.attr ) );
    lhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, VALUE, VALUE, NONE, VALUE )
{
    return lhs;
}

MDDL_OP_IMPL( OP_RE, CONCAT, SEQ, VSEQ, SEQ )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_CONCAT_ERR );
    v.get().concat( rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, CONCAT, SEQ, VATTR, SEQ )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_CONCAT_ERR );
    
    v.get().concat( rhs.attr, rhs.attr, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, EXTEND, SEQ, VALUE, SEQ )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_RESIZE_ERR );

    v.get().extend( rhs.value );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, CONCAT, VSEQ, VSEQ, VSEQ )
{
    DataRef v = lhs.elide_copy();

    v.get().concat( rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, CONCAT, VSEQ, VATTR, VSEQ )
{
    DataRef v = lhs.elide_copy();

    v.get().concat( rhs.attr, rhs.attr, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, EXTEND, VSEQ, VALUE, VSEQ )
{
    DataRef v = lhs.elide_copy();

    v.get().extend( rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_RE, CONCAT, ATTR, VSEQ, ATTR )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_CONCAT_ERR );
    
    v.get().concat( lhs.attr, lhs.attr, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, CONCAT, ATTR, VATTR, ATTR )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_CONCAT_ERR );
    
    v.get().concat( lhs.attr, rhs.attr, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, EXTEND, ATTR, VALUE, ATTR )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_RESIZE_ERR );

    v.get().extend( rhs.value );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, CONCAT, VATTR, VSEQ, VATTR )
{
    DataRef v = lhs.elide_copy();

    v.get().concat( lhs.attr, lhs.attr, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, CONCAT, VATTR, VATTR, VATTR )
{
    DataRef v = lhs.elide_copy();

    v.get().concat( lhs.attr, rhs.attr, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, EXTEND, VATTR, VALUE, VATTR )
{
    DataRef v = lhs.elide_copy();

    v.get().extend( rhs.value );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, INDEX, VALUE, SEQ, SEQ )
{
    const int64_t idx = rhs.start + lhs.value;
    rt_assert( idx >= 0 && idx < rhs.length(), INDEX_BOUNDS_ERR );

    DataRef v = rhs.move();
    v.start = idx;
    v.size = 1;
    return v;
}

MDDL_OP_IMPL( OP_RE, INDEX, VALUE, VSEQ, VSEQ )
{
    const int64_t idx = rhs.start + lhs.value;
    rt_assert( idx >= 0 && idx < rhs.length(), INDEX_BOUNDS_ERR );

    const Sequence::Elem& elem = rhs.get().at( idx );
    DataRef v( DataType::VSEQ, new Sequence( elem ) );
    rhs.release();
    return v;
}


MDDL_OP_IMPL( OP_RE, INDEX, VALUE, ATTR, ATTR )
{
    const int64_t idx = rhs.start + lhs.value;
    rt_assert( idx >= 0 && idx < rhs.length(), INDEX_BOUNDS_ERR );

    DataRef v = rhs.move();
    v.start = idx;
    v.size = 1;
    return v;
}

MDDL_OP_IMPL( OP_RE, INDEX, VALUE, VATTR, VATTR )
{
    const int64_t idx = rhs.start + lhs.value;
    rt_assert( idx >= 0 && idx < rhs.length(), INDEX_BOUNDS_ERR );

    const Sequence::Elem& elem = rhs.get().at( idx );
    DataRef v( DataType::VSEQ, new Sequence( elem ) );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_RE, INDEX, VALUE, VALUE, INDEXER )
{
    DataRef v = DataType::INDEXER;
    v.start = lhs.value;
    v.size = rhs.value - lhs.value;
    return v;
}

MDDL_OP_IMPL( OP_RE, INDEX, INDEXER, SEQ, SEQ )
{
    const int64_t start = rhs.start + lhs.start;
    const int64_t size = lhs.size;
    rt_assert( start >= 0 && start < rhs.length() && size <= rhs.length(), INDEX_BOUNDS_ERR );

    DataRef v = rhs.move();
    v.start = start;
    v.size = size;
    return v;
}

MDDL_OP_IMPL( OP_RE, INDEX, INDEXER, VSEQ, VSEQ )
{
    const int64_t start = rhs.start + lhs.start;
    const int64_t size = lhs.size;
    rt_assert( start >= 0 && start < rhs.length() && size <= rhs.length(), INDEX_BOUNDS_ERR );

    DataRef v = rhs.elide_copy();
    v.start = start;
    v.size = size;
    return v;
}

MDDL_OP_IMPL( OP_RE, INDEX, INDEXER, ATTR, ATTR )
{
    const int64_t start = rhs.start + lhs.start;
    const int64_t size = lhs.size;
    rt_assert( start >= 0 && start < rhs.length() && size <= rhs.length(), INDEX_BOUNDS_ERR );

    DataRef v = rhs.move();
    v.start = start;
    v.size = size;
    return v;
}

MDDL_OP_IMPL( OP_RE, INDEX, INDEXER, VATTR, VATTR )
{
    const int64_t start = rhs.start + lhs.start;
    const int64_t size = lhs.size;
    rt_assert( start >= 0 && start < rhs.length() && size <= rhs.length(), INDEX_BOUNDS_ERR );

    DataRef v = rhs.elide_copy();
    v.start = start;
    v.size = size;
    return v;
}



// MI, or LENGTH/COMPARE
MDDL_OP_IMPL( OP_MI, LENGTH, VSEQ, NONE, VALUE )
{
    const int64_t length = lhs.length();
    lhs.release();
    return length;
}

MDDL_OP_IMPL( OP_MI, LENGTH, VATTR, NONE, VALUE )
{
    const int64_t length = lhs.length();
    lhs.release();
    return length;
}

MDDL_OP_IMPL( OP_MI, LENGTH, VALUE, NONE, VALUE )
{
    return lhs.value;
}

MDDL_OP_IMPL( OP_MI, COMPARE, VSEQ, VSEQ, VALUE )
{
    const int64_t v = compare( lhs.length(), rhs.length() );
    lhs.release();
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_MI, COMPARE, VSEQ, VATTR, VALUE )
{
    const int64_t v = compare( lhs.length(), rhs.length() );
    lhs.release();
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_MI, COMPARE, VSEQ, VALUE, VALUE )
{
    const int64_t v = compare( lhs.length(), rhs.value );
    lhs.release();
    return v;
}

MDDL_OP_IMPL( OP_MI, COMPARE, VATTR, VSEQ, VALUE )
{
    const int64_t v = compare( lhs.length(), rhs.length() );
    lhs.release();
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_MI, COMPARE, VATTR, VATTR, VALUE )
{
    const int64_t v = compare( lhs.length(), rhs.length() );
    lhs.release();
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_MI, COMPARE, VATTR, VALUE, VALUE )
{
    const int64_t v = compare( lhs.length(), rhs.value );
    lhs.release();
    return v;
}

MDDL_OP_IMPL( OP_MI, COMPARE, VALUE, VSEQ, VALUE )
{
    const int64_t v = compare( lhs.value, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_MI, COMPARE, VALUE, VATTR, VALUE )
{
    const int64_t v = compare( lhs.value, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_MI, COMPARE, VALUE, VALUE, VALUE )
{
    return compare( lhs.value, rhs.value );
}

// FA, or PITCH/ADD
MDDL_OP_IMPL( OP_FA, PITCH, SEQ, NONE, ATTR )
{
    DataRef v = lhs.move();
    v.type = DataType::ATTR;
    v.attr = AttrType::PITCH;
    return v;
}

MDDL_OP_IMPL( OP_FA, PITCH, VSEQ, NONE, VATTR )
{
    DataRef v = lhs.move();
    v.type = DataType::VATTR;
    v.attr = AttrType::PITCH;
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, VALUE, NONE, VALUE )
{
    return lhs.value + 1;
}

MDDL_OP_IMPL( OP_FA, ADD, SEQ, VSEQ, SEQ )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().add( v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, SEQ, VATTR, SEQ )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().add( rhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, SEQ, VALUE, SEQ )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_RESIZE_ERR );

    v.get().resize( v.get().size + rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, VSEQ, VSEQ, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().add( v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, VSEQ, VATTR, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().add( rhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, VSEQ, VALUE, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().resize( v.get().size + rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, ATTR, VSEQ, ATTR )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().add( lhs.attr, lhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, ATTR, VATTR, ATTR )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().add( lhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, ATTR, VALUE, ATTR )
{
    DataRef v = lhs.move();
    v.get().add_value( lhs.attr, v.start, v.length(), rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, VATTR, VSEQ, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().add( lhs.attr, lhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, VATTR, VATTR, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().add( lhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, VATTR, VALUE, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().add_value( lhs.attr, v.start, v.get().size, rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_FA, ADD, VALUE, VALUE, VALUE )
{
    return lhs.value + rhs.value;
}



// SO, or VELOCITY/SUBTRACT
MDDL_OP_IMPL( OP_SO, VELOCITY, SEQ, NONE, ATTR )
{
    DataRef v = lhs.move();
    v.type = DataType::ATTR;
    v.attr = AttrType::VELOCITY;
    return v;
}

MDDL_OP_IMPL( OP_SO, VELOCITY, VSEQ, NONE, VATTR )
{
    DataRef v = lhs.move();
    v.type = DataType::VATTR;
    v.attr = AttrType::VELOCITY;
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, VALUE, NONE, VALUE )
{
    return lhs.value - 1;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, SEQ, VSEQ, SEQ )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().subtract( v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, SEQ, VATTR, SEQ )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().subtract( rhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, SEQ, VALUE, SEQ )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_RESIZE_ERR );

    v.get().resize( v.get().size - rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, VSEQ, VSEQ, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().subtract( v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, VSEQ, VATTR, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().subtract( rhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, VSEQ, VALUE, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().resize( v.get().size - rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, ATTR, VSEQ, ATTR )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().subtract( lhs.attr, lhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, ATTR, VATTR, ATTR )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().subtract( lhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, ATTR, VALUE, ATTR )
{
    DataRef v = lhs.move();
    v.get().subtract_value( lhs.attr, v.start, v.length(), rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, VATTR, VSEQ, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().subtract( lhs.attr, lhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, VATTR, VATTR, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().subtract( lhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, VATTR, VALUE, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().subtract_value( lhs.attr, v.start, v.get().size, rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_SO, SUBTRACT, VALUE, VALUE, VALUE )
{
    return lhs.value - rhs.value;
}



// LA, or DURATION/MULTIPLY
MDDL_OP_IMPL( OP_LA, DURATION, SEQ, NONE, ATTR )
{
    DataRef v = lhs.move();
    v.type = DataType::ATTR;
    v.attr = AttrType::DURATION;
    return v;
}

MDDL_OP_IMPL( OP_LA, DURATION, VSEQ, NONE, VATTR )
{
    DataRef v = lhs.move();
    v.type = DataType::VATTR;
    v.attr = AttrType::DURATION;
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, SEQ, VSEQ, SEQ )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().multiply( v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, SEQ, VATTR, SEQ )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().multiply( rhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, SEQ, VALUE, SEQ )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_RESIZE_ERR );

    v.get().resize( v.get().size * rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, VSEQ, VSEQ, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().multiply( v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, VSEQ, VATTR, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().multiply( rhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, VSEQ, VALUE, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().resize( v.get().size * rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, ATTR, VSEQ, ATTR )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().multiply( lhs.attr, lhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, ATTR, VATTR, ATTR )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().multiply( lhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, ATTR, VALUE, ATTR )
{
    DataRef v = lhs.move();
    v.get().multiply_value( lhs.attr, v.start, v.length(), rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, VATTR, VSEQ, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().multiply( lhs.attr, lhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, VATTR, VATTR, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().multiply( lhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, VATTR, VALUE, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().multiply_value( lhs.attr, v.start, v.get().size, rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_LA, MULTIPLY, VALUE, VALUE, VALUE )
{
    return lhs.value * rhs.value;
}



// TI, or WAIT/DIVIDE
MDDL_OP_IMPL( OP_TI, WAIT, SEQ, NONE, ATTR )
{
    DataRef v = lhs.move();
    v.type = DataType::ATTR;
    v.attr = AttrType::WAIT;
    return v;
}

MDDL_OP_IMPL( OP_TI, WAIT, VSEQ, NONE, VATTR )
{
    DataRef v = lhs.move();
    v.type = DataType::VATTR;
    v.attr = AttrType::WAIT;
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, SEQ, VSEQ, SEQ )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().divide( v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, SEQ, VATTR, SEQ )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().divide( rhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, SEQ, VALUE, SEQ )
{
    DataRef v = lhs.move();

    rt_assert( !v.is_subseq(), SUBSEQ_RESIZE_ERR );

    v.get().resize( v.get().size / rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, VSEQ, VSEQ, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().divide( v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, VSEQ, VATTR, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().divide( rhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, VSEQ, VALUE, VSEQ )
{
    DataRef v = lhs.elide_copy();
    v.get().resize( v.get().size / rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, ATTR, VSEQ, ATTR )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().divide( lhs.attr, lhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, ATTR, VATTR, ATTR )
{
    DataRef v = lhs.move();
    
    if ( v.is_subseq() ) {
        rt_assert( v.length() <= rhs.length(), SUBSEQ_BOUNDS_ERR );
    } else {
        v.get().expect( rhs.length() );
    }

    v.get().divide( lhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, ATTR, VALUE, ATTR )
{
    DataRef v = lhs.move();
    v.get().divide_value( lhs.attr, v.start, v.length(), rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, VATTR, VSEQ, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().divide( lhs.attr, lhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, VATTR, VATTR, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().expect( rhs.length() );
    v.get().divide( lhs.attr, rhs.attr, v.start, rhs.get(), rhs.start, rhs.length() );
    rhs.release();
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, VATTR, VALUE, VATTR )
{
    DataRef v = lhs.elide_copy();
    v.get().divide_value( lhs.attr, v.start, v.get().size, rhs.value );
    return v;
}

MDDL_OP_IMPL( OP_TI, DIVIDE, VALUE, VALUE, VALUE )
{
    return lhs.value / rhs.value;
}


#define MDDL_OP_REGISTER( group, name, lhs_t, rhs_t, return_t ) \
    { OpBookKey( group, DataType::lhs_t, DataType::rhs_t ), \
      OpBookEntry( name, MDDL_OP_FN( group, lhs_t, rhs_t ), DataType::return_t ) }

OpBook op_book = {
    // DO, or NEW/ASSIGN
    MDDL_OP_REGISTER( OP_DO, NEW, VSEQ, NONE, VSEQ ),
    MDDL_OP_REGISTER( OP_DO, NEW, VALUE, NONE, VSEQ ),
    MDDL_OP_REGISTER( OP_DO, COMPLETE, SEQ_LIT, NONE, VSEQ ),
    MDDL_OP_REGISTER( OP_DO, ASSIGN, SEQ, SEQ, SEQ ),
    MDDL_OP_REGISTER( OP_DO, SET, SEQ, VSEQ, SEQ ),
    MDDL_OP_REGISTER( OP_DO, SET, SEQ, VATTR, SEQ ),
    MDDL_OP_REGISTER( OP_DO, RESIZE, SEQ, VALUE, SEQ ),
    MDDL_OP_REGISTER( OP_DO, SET, VSEQ, VSEQ, VSEQ ),
    MDDL_OP_REGISTER( OP_DO, SET, VSEQ, VATTR, VSEQ ),
    MDDL_OP_REGISTER( OP_DO, RESIZE, VSEQ, VALUE, VSEQ ),
    MDDL_OP_REGISTER( OP_DO, SET, ATTR, VSEQ, ATTR ),
    MDDL_OP_REGISTER( OP_DO, SET, ATTR, VATTR, ATTR ),
    MDDL_OP_REGISTER( OP_DO, SET, ATTR, VALUE, ATTR ),
    MDDL_OP_REGISTER( OP_DO, SET, VATTR, VSEQ, VATTR ),
    MDDL_OP_REGISTER( OP_DO, SET, VATTR, VATTR, VATTR ),
    MDDL_OP_REGISTER( OP_DO, SET, VATTR, VALUE, VATTR ),

    // RE, or VALUE/CONCAT/INDEX
    MDDL_OP_REGISTER( OP_RE, VALUE, VSEQ, NONE, VALUE ),
    MDDL_OP_REGISTER( OP_RE, VALUE, VATTR, NONE, VALUE ),
    MDDL_OP_REGISTER( OP_RE, VALUE, VALUE, NONE, VALUE ),
    MDDL_OP_REGISTER( OP_RE, CONCAT, SEQ, VSEQ, SEQ ),
    MDDL_OP_REGISTER( OP_RE, CONCAT, SEQ, VATTR, SEQ ),
    MDDL_OP_REGISTER( OP_RE, EXTEND, SEQ, VALUE, SEQ ),
    MDDL_OP_REGISTER( OP_RE, CONCAT, VSEQ, VSEQ, VSEQ ),
    MDDL_OP_REGISTER( OP_RE, CONCAT, VSEQ, VATTR, VSEQ ),
    MDDL_OP_REGISTER( OP_RE, EXTEND, VSEQ, VALUE, VSEQ ),
    MDDL_OP_REGISTER( OP_RE, CONCAT, ATTR, VSEQ, ATTR ),
    MDDL_OP_REGISTER( OP_RE, CONCAT, ATTR, VATTR, ATTR ),
    MDDL_OP_REGISTER( OP_RE, EXTEND, ATTR, VALUE, ATTR ),
    MDDL_OP_REGISTER( OP_RE, CONCAT, VATTR, VSEQ, VATTR ),
    MDDL_OP_REGISTER( OP_RE, CONCAT, VATTR, VATTR, VATTR ),
    MDDL_OP_REGISTER( OP_RE, EXTEND, VATTR, VALUE, VATTR ),
    MDDL_OP_REGISTER( OP_RE, INDEX, VALUE, SEQ, SEQ ),
    MDDL_OP_REGISTER( OP_RE, INDEX, VALUE, VSEQ, VSEQ ),
    MDDL_OP_REGISTER( OP_RE, INDEX, VALUE, ATTR, ATTR ),
    MDDL_OP_REGISTER( OP_RE, INDEX, VALUE, VATTR, VATTR ),
    MDDL_OP_REGISTER( OP_RE, INDEX, VALUE, VALUE, INDEXER ),
    MDDL_OP_REGISTER( OP_RE, INDEX, INDEXER, SEQ, SEQ ),
    MDDL_OP_REGISTER( OP_RE, INDEX, INDEXER, VSEQ, VSEQ ),
    MDDL_OP_REGISTER( OP_RE, INDEX, INDEXER, ATTR, ATTR ),
    MDDL_OP_REGISTER( OP_RE, INDEX, INDEXER, VATTR, VATTR ),

    // MI, or LENGTH/COMPARE
    MDDL_OP_REGISTER( OP_MI, LENGTH, VSEQ, NONE, VALUE ),
    MDDL_OP_REGISTER( OP_MI, LENGTH, VATTR, NONE, VALUE ),
    MDDL_OP_REGISTER( OP_MI, LENGTH, VALUE, NONE, VALUE ),
    MDDL_OP_REGISTER( OP_MI, COMPARE, VSEQ, VSEQ, VALUE ),
    MDDL_OP_REGISTER( OP_MI, COMPARE, VSEQ, VATTR, VALUE ),
    MDDL_OP_REGISTER( OP_MI, COMPARE, VSEQ, VALUE, VALUE ),
    MDDL_OP_REGISTER( OP_MI, COMPARE, VATTR, VSEQ, VALUE ),
    MDDL_OP_REGISTER( OP_MI, COMPARE, VATTR, VATTR, VALUE ),
    MDDL_OP_REGISTER( OP_MI, COMPARE, VATTR, VALUE, VALUE ),
    MDDL_OP_REGISTER( OP_MI, COMPARE, VALUE, VSEQ, VALUE ),
    MDDL_OP_REGISTER( OP_MI, COMPARE, VALUE, VATTR, VALUE ),
    MDDL_OP_REGISTER( OP_MI, COMPARE, VALUE, VALUE, VALUE ),

    // FA, or PITCH/ADD
    MDDL_OP_REGISTER( OP_FA, PITCH, SEQ, NONE, ATTR ),
    MDDL_OP_REGISTER( OP_FA, PITCH, VSEQ, NONE, VATTR ),
    MDDL_OP_REGISTER( OP_FA, ADD, VALUE, NONE, VALUE ),
    MDDL_OP_REGISTER( OP_FA, ADD, SEQ, VSEQ, SEQ ),
    MDDL_OP_REGISTER( OP_FA, ADD, SEQ, VATTR, SEQ ),
    MDDL_OP_REGISTER( OP_FA, ADD, SEQ, VALUE, SEQ ),
    MDDL_OP_REGISTER( OP_FA, ADD, VSEQ, VSEQ, VSEQ ),
    MDDL_OP_REGISTER( OP_FA, ADD, VSEQ, VATTR, VSEQ ),
    MDDL_OP_REGISTER( OP_FA, ADD, VSEQ, VALUE, VSEQ ),
    MDDL_OP_REGISTER( OP_FA, ADD, ATTR, VSEQ, ATTR ),
    MDDL_OP_REGISTER( OP_FA, ADD, ATTR, VATTR, ATTR ),
    MDDL_OP_REGISTER( OP_FA, ADD, ATTR, VALUE, ATTR ),
    MDDL_OP_REGISTER( OP_FA, ADD, VATTR, VSEQ, VATTR ),
    MDDL_OP_REGISTER( OP_FA, ADD, VATTR, VATTR, VATTR ),
    MDDL_OP_REGISTER( OP_FA, ADD, VATTR, VALUE, VATTR ),
    MDDL_OP_REGISTER( OP_FA, ADD, VALUE, VALUE, VALUE ),
    
    // SO, or VELOCITY/SUBTRACT
    MDDL_OP_REGISTER( OP_SO, VELOCITY, SEQ, NONE, ATTR ),
    MDDL_OP_REGISTER( OP_SO, VELOCITY, VSEQ, NONE, VATTR ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, VALUE, NONE, VALUE ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, SEQ, VSEQ, SEQ ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, SEQ, VATTR, SEQ ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, SEQ, VALUE, SEQ ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, VSEQ, VSEQ, VSEQ ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, VSEQ, VATTR, VSEQ ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, VSEQ, VALUE, VSEQ ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, ATTR, VSEQ, ATTR ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, ATTR, VATTR, ATTR ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, ATTR, VALUE, ATTR ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, VATTR, VSEQ, VATTR ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, VATTR, VATTR, VATTR ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, VATTR, VALUE, VATTR ),
    MDDL_OP_REGISTER( OP_SO, SUBTRACT, VALUE, VALUE, VALUE ),
    
    // LA, or DURATION/MULTIPLY
    MDDL_OP_REGISTER( OP_LA, DURATION, SEQ, NONE, ATTR ),
    MDDL_OP_REGISTER( OP_LA, DURATION, VSEQ, NONE, VATTR ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, SEQ, VSEQ, SEQ ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, SEQ, VATTR, SEQ ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, SEQ, VALUE, SEQ ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, VSEQ, VSEQ, VSEQ ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, VSEQ, VATTR, VSEQ ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, VSEQ, VALUE, VSEQ ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, ATTR, VSEQ, ATTR ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, ATTR, VATTR, ATTR ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, ATTR, VALUE, ATTR ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, VATTR, VSEQ, VATTR ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, VATTR, VATTR, VATTR ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, VATTR, VALUE, VATTR ),
    MDDL_OP_REGISTER( OP_LA, MULTIPLY, VALUE, VALUE, VALUE ),

    // TI, or WAIT/DIVIDE
    MDDL_OP_REGISTER( OP_TI, WAIT, SEQ, NONE, ATTR ),
    MDDL_OP_REGISTER( OP_TI, WAIT, VSEQ, NONE, VATTR ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, SEQ, VSEQ, SEQ ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, SEQ, VATTR, SEQ ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, SEQ, VALUE, SEQ ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, VSEQ, VSEQ, VSEQ ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, VSEQ, VATTR, VSEQ ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, VSEQ, VALUE, VSEQ ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, ATTR, VSEQ, ATTR ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, ATTR, VATTR, ATTR ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, ATTR, VALUE, ATTR ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, VATTR, VSEQ, VATTR ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, VATTR, VATTR, VATTR ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, VATTR, VALUE, VATTR ),
    MDDL_OP_REGISTER( OP_TI, DIVIDE, VALUE, VALUE, VALUE ),
};

#undef MDDL_OP_FN
#undef MDDL_OP_FN_IMPL
#undef MDDL_OP_REGISTER

} // namespace MDDL