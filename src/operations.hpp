// operations.hpp

#ifndef __MDDL_OPERATIONS_HPP__
#define __MDDL_OPERATIONS_HPP__

#include "common.hpp"
#include "data_ref.hpp"
#include "utils.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <string>



namespace MDDL {

static constexpr int N_OP_IDS = 7;

struct OpBookKey
{
    OpBookKey( OpId group, DataType lhs_t, DataType rhs_t )
        : group { group }
        , lhs_t { lhs_t }
        , rhs_t { rhs_t }
    {}

    bool operator<( const OpBookKey& rhs ) const
    {
        if ( group < rhs.group ) return true;
        if ( group > rhs.group ) return false;
        if ( lhs_t < rhs.lhs_t ) return true;
        if ( lhs_t > rhs.lhs_t ) return false;
        return (rhs_t < rhs.rhs_t);
    }

    const OpId  group   = OP_UNKNOWN;
    DataType    lhs_t   = DataType::UNKNOWN;
    DataType    rhs_t   = DataType::UNKNOWN;
};

class Runtime;
using OpFn = std::function<DataRef( Runtime*, DataRef&, DataRef& )>;

struct OpBookEntry
{
    OpBookEntry() = default;

    OpBookEntry( const char* name, OpFn fn, DataType return_t )
        : name      { name }
        , fn        { fn }
        , return_t  { return_t }
    {}

    const char*     name        = "UNKNOWN";
    const OpFn      fn          = nullptr;
    const DataType  return_t    = DataType::UNKNOWN;
};


using OpBook = std::map<OpBookKey, OpBookEntry>;

extern OpBook op_book;

};

#endif // __MDDL_OPERATIONS_HPP__