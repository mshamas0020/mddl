// ief.hpp
// Interpreter Extended Functionality

#include <chrono>
#include <thread>

#ifndef __MDDL_IEF_HPP__
#define __MDDL_IEF_HPP__

namespace MDDL {

inline void ief_sleep( int64_t ms )
{
    std::this_thread::sleep_for( std::chrono::milliseconds( ms ) );
}

} // namespace MDDL

#endif // __MDDL_IEF_HPP__