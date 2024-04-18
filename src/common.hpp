// common.hpp

#ifndef __MDDL_COMMON_HPP__
#define __MDDL_COMMON_HPP__

namespace MDDL {

enum OpId : int
{
    OP_UNKNOWN      = 0x00,

    OP_DO           = 0x10,
    OP_RE           = 0x11,
    OP_MI           = 0x12,
    OP_FA           = 0x13,
    OP_SO           = 0x14,
    OP_LA           = 0x15,
    OP_TI           = 0x16,

    IEF_DEFAULT     = 0x20,
    IEF_PLAY        = 0x21,
    IEF_NOTE_ON     = 0x22,
    IEF_NOTE_OFF    = 0x23,
    IEF_SLEEP       = 0x24,
    IEF_PRINT       = 0x25,
    IEF_PRINTD      = 0x26,
    IEF_RECORDING   = 0x27,
    IEF_RANDOM      = 0x28,
};

} // namespace MDDL

#endif // __MDDL_COMMON_HPP