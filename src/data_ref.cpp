// data_ref.cpp

#include "data_ref.hpp"
#include "errors.hpp"



namespace MDDL {

DataRef::DataRef( DataType type )
    : type  { type }
{}

DataRef::DataRef( int64_t value )
    : type  { DataType::VALUE }
    , value { value }
{}

DataRef::DataRef( DataType type, Sequence* seq, AttrType attr )
    : type  { type }
    , attr  { attr }
    , ref   { seq }
{
    seq->ref_count ++;
}

void DataRef::attach( Sequence* seq, int64_t ref_start, int64_t ref_size )
{
    sys_assert( seq != nullptr );

    ref = seq;
    seq->ref_count ++;

    start = ref_start;
    size = ref_size;
}

void DataRef::release()
{
    if ( ref == nullptr )
        return;

    ref->ref_count --;
    if ( ref->ref_count == 0 )
        delete ref;
        
    ref = nullptr;
}

void DataRef::take( DataRef&& rhs )
{
    attach( rhs.ref, rhs.start, rhs.size );
} 

void DataRef::implicit_cast( DataType t )
{
    sys_assert( may_implicit_cast( type, t ) );
    type = t;
}

[[nodiscard]] DataRef DataRef::copy() const
{
    sys_assert( ref != nullptr );
    return DataRef( type, new Sequence( get(), start, length() ), attr );
}

[[nodiscard]] DataRef DataRef::duplicate() const
{
    sys_assert( ref != nullptr );
    ref->ref_count ++;
    return *this;
}

[[nodiscard]] DataRef DataRef::move()
{
    sys_assert( ref != nullptr );
    DataRef x = *this;
    ref = nullptr;
    return x;
}

// moves if exclusively owned,  
[[nodiscard]] DataRef DataRef::elide_copy()
{
    sys_assert( ref != nullptr );
    type = to_copy_type( type );

    if ( ref->ref_count == 1 ) {
        // ref is going to be destructed, move instead
        if ( is_subseq() )
            ref->crop( start, size );
        return move();
    }

    // sequence is held by other refs, must copy
    DataRef v = copy();
    release();
    return v;
}

[[nodiscard]] DataRef DataRef::cast_to_vseq()
{
    DataRef v;
    switch ( type ) {
        case DataType::VALUE:
            return DataRef( DataType::VSEQ, new Sequence( value ) );
        case DataType::UNDEFINED:
        case DataType::VOID:
        case DataType::INDEXER:
            return DataRef( DataType::VSEQ, new Sequence );
        case DataType::ATTR:
        case DataType::VATTR:
            v = elide_copy();
            v.ref->mask( attr );
            v.type = DataType::VSEQ;
            return v;
        case DataType::SEQ:
        case DataType::VSEQ:
            v = elide_copy();
            v.type = DataType::VSEQ;
            return v;
        default: enum_error(); break;
    }

    return DataType::ERROR;
}

[[nodiscard]] DataRef DataRef::cast_to_seq()
{
    DataRef v;
    switch ( type ) {
        case DataType::VALUE:
            return DataRef( DataType::VSEQ, new Sequence( value ) );
        case DataType::UNDEFINED:
        case DataType::INDEXER:
            return DataRef( DataType::VSEQ, new Sequence );
        case DataType::ATTR:
        case DataType::VATTR:
            v = elide_copy();
            v.ref->mask( attr );
            v.type = DataType::SEQ;
            return v;
        case DataType::SEQ:
            return move();
        case DataType::SEQ_LIT:
            v = move();
            v.type = DataType::SEQ;
            return v;
        case DataType::VSEQ:
            v = elide_copy();
            v.type = DataType::SEQ;
            return v;
        default: enum_error(); break;
    }

    return DataType::ERROR;
}

void DataRef::print()
{
    std::cout << "[Type: " << dt_to_string( type );

    std::cout << ", Attr: " << attr_to_string( attr );

    if ( stack_pos != -1 )
        std::cout << ", Stack: " << stack_pos;

    std::cout << ", Ref: " << ref;
    

    if ( ref != nullptr ) {
        std::cout << ", Ref Count: " << ref->ref_count;
        std::cout << ", Len: " << length();
    }
    
    if ( is_subseq() )
        std::cout << " (" << start << ", " << start + size << ")";

    std::cout << ", Val: " << value;
    std::cout << "]\n";
}

} // namepspace MDDL