// common.hpp

#ifndef __MDDL_COMMON_HPP__
#define __MDDL_COMMON_HPP__

namespace MDDL {

enum OpId : int
{
    OP_UNKNOWN,
    OP_DO, OP_RE, OP_MI, OP_FA, OP_SO, OP_LA, OP_TI,
    IEF_DEFAULT
};

} // namespace MDDL

#endif // __MDDL_COMMON_HPP