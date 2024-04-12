// data_ref.hpp

#ifndef __MDDL_DATA_REF_HPP__
#define __MDDL_DATA_REF_HPP__

#include "utils.hpp"
#include "sequence.hpp"

namespace MDDL {

struct DataRef
{
    DataRef() = default;
    DataRef( int64_t value );
    DataRef( DataType type );
    DataRef( DataType type, Sequence* seq, AttrType attr = AttrType::ALL );

    bool is_subseq() const { return size != 0; }
    int64_t length() const { return is_subseq() ? size : ref->size; }
    bool empty() const { return ref == nullptr; }
    bool is_ref_type() const { return type == DataType::SEQ || type == DataType::ATTR; }
    bool is_copy_type() const { return type == DataType::VSEQ || type == DataType::VATTR; }
    
    void attach( Sequence* seq, int64_t ref_start = 0, int64_t ref_size = 0 );
    void release();
    void take( DataRef&& ref );

    void implicit_cast( DataType t );

    [[nodiscard]] const Sequence& get() const { return *ref; }
    [[nodiscard]] Sequence& get() { return *ref; }
    [[nodiscard]] DataRef copy() const;
    [[nodiscard]] DataRef duplicate() const;
    [[nodiscard]] DataRef move();
    [[nodiscard]] DataRef elide_copy();
    [[nodiscard]] DataRef cast_to_vseq();
    [[nodiscard]] DataRef cast_to_seq();
    
    void print();

    DataType    type        = DataType::UNKNOWN;
    AttrType    attr        = AttrType::ALL;
    Sequence*   ref         = nullptr;
    int64_t     stack_pos   = -1;
    int64_t     start       = 0;
    int64_t     size        = 0;
    int64_t     value       = 0;
};


inline bool validate_type( const DataRef& ref, DataType type )
{
    return ref.type == type;
}

#define MDDL_ASSERT_TYPE( d, d_t ) \
    assert( validate_type( d, DataType::d_t ) )


} // namepspace MDDL

#endif // __MDDL_DATA_REF_HPP__