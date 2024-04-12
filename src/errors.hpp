// errors.hpp

#ifndef __MDDL_ERRORS_HPP__
#define __MDDL_ERRORS_HPP__

#include <cassert>
#include <exception>
#include <stdexcept>

namespace MDDL {

class MDDL_RuntimeError : public std::runtime_error
{
public:
    explicit MDDL_RuntimeError( const std::string& msg )
        : std::runtime_error( "Runtime Error: " + msg )
    {}
};

class MDDL_SysError : public std::runtime_error
{
public:
    explicit MDDL_SysError( const std::string& msg )
        : std::runtime_error( "MDDL System Error: " + msg )
    {}
};

inline void rt_error( const char* msg = "" )
{
    // assert( false );
    throw MDDL_RuntimeError( msg );
}

inline void rt_assert( bool condition, const char* msg = "" )
{
    if ( !condition )
        rt_error( msg );
}

inline void sys_error( const char* msg = "" )
{
    // assert( false );
    //throw MDDL_SysError( msg );
}

inline void sys_assert( bool condition, const char* msg = "" )
{
    if ( !condition )
        sys_error( msg );
}

inline void enum_error()
{
    sys_error( "Unrecognized enum." );
}

} // namespace MDDL

#endif // __MDDL_ERRORS_HPP__