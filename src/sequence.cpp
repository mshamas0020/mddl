// sequence.cpp

#include "errors.hpp"
#include "sequence.hpp"



namespace MDDL {


#define MDDL_DISAMBIGUATE_ONE_ATTR_FN( attr_fn, attr, ... ) \
    switch ( attr ) { \
        case AttrType::PITCH: attr_fn<&Elem::pitch>( __VA_ARGS__ ); break; \
        case AttrType::VELOCITY: attr_fn<&Elem::vel>( __VA_ARGS__ ); break; \
        case AttrType::DURATION: attr_fn<&Elem::dur>( __VA_ARGS__ ); break; \
        case AttrType::WAIT: attr_fn<&Elem::wait>( __VA_ARGS__ ); break; \
        default: enum_error(); break; \
    }

#define MDDL_DISAMBIGUATE_TWO_ATTR_FN( attr_fn, lhs_attr, rhs_attr, ... ) \
    switch ( lhs_attr ) { \
        case AttrType::PITCH: \
            switch ( rhs_attr ) { \
                case AttrType::PITCH: \
                    attr_fn<&Elem::pitch, &Elem::pitch>( __VA_ARGS__ ); break; \
                case AttrType::VELOCITY: \
                    attr_fn<&Elem::pitch, &Elem::vel>( __VA_ARGS__ ); break; \
                case AttrType::DURATION: \
                    attr_fn<&Elem::pitch, &Elem::dur>( __VA_ARGS__ ); break; \
                case AttrType::WAIT: \
                    attr_fn<&Elem::pitch, &Elem::wait>( __VA_ARGS__ ); break; \
                default: /* error */ break; \
            } \
            break; \
        case AttrType::VELOCITY: \
            switch ( rhs_attr ) { \
                case AttrType::PITCH: \
                    attr_fn<&Elem::vel, &Elem::pitch>( __VA_ARGS__ ); break; \
                case AttrType::VELOCITY: \
                    attr_fn<&Elem::vel, &Elem::vel>( __VA_ARGS__ ); break; \
                case AttrType::DURATION: \
                    attr_fn<&Elem::vel, &Elem::dur>( __VA_ARGS__ ); break; \
                case AttrType::WAIT: \
                    attr_fn<&Elem::vel, &Elem::wait>( __VA_ARGS__ ); break; \
                default: /* error */ break; \
            } \
            break; \
        case AttrType::DURATION:\
            switch ( rhs_attr ) { \
                case AttrType::PITCH: \
                    attr_fn<&Elem::dur, &Elem::pitch>( __VA_ARGS__ ); break; \
                case AttrType::VELOCITY: \
                    attr_fn<&Elem::dur, &Elem::vel>( __VA_ARGS__ ); break; \
                case AttrType::DURATION: \
                    attr_fn<&Elem::dur, &Elem::dur>( __VA_ARGS__ ); break; \
                case AttrType::WAIT: \
                    attr_fn<&Elem::dur, &Elem::wait>( __VA_ARGS__ ); break; \
                default: /* error */ break; \
            } \
            break; \
        case AttrType::WAIT: \
            switch ( rhs_attr ) { \
                case AttrType::PITCH: \
                    attr_fn<&Elem::wait, &Elem::pitch>( __VA_ARGS__ ); break; \
                case AttrType::VELOCITY: \
                    attr_fn<&Elem::wait, &Elem::vel>( __VA_ARGS__ ); break; \
                case AttrType::DURATION: \
                    attr_fn<&Elem::wait, &Elem::dur>( __VA_ARGS__ ); break; \
                case AttrType::WAIT: \
                    attr_fn<&Elem::wait, &Elem::wait>( __VA_ARGS__ ); break; \
                default: /* error */ break; \
            } \
            break; \
        default: enum_error(); break; \
    }

using Elem = Sequence::Elem;

bool Elem::operator==( const Elem& rhs ) const
{
    return pitch == rhs.pitch
        && vel == rhs.vel
        && dur == rhs.dur
        && wait == rhs.wait;
}

void Elem::operator+=( const Elem& rhs )
{
    pitch += rhs.pitch;
    vel += rhs.vel;
    dur += rhs.dur;
    wait += rhs.wait;
}

void Elem::operator-=( const Elem& rhs )
{
    pitch -= rhs.pitch;
    vel -= rhs.vel;
    dur -= rhs.dur;
    wait -= rhs.wait;
}

void Elem::operator*=( const Elem& rhs )
{
    pitch *= rhs.pitch;
    vel *= rhs.vel;
    dur *= rhs.dur;
    wait *= rhs.wait;
}

void Elem::operator/=( const Elem& rhs )
{
    pitch /= rhs.pitch;
    vel /= rhs.vel;
    dur /= rhs.dur;
    wait /= rhs.wait;
}



Sequence::Sequence()
    : Sequence( 0 )
{};

Sequence::Sequence( int64_t value )
    : size          { value }
    , compressed    { true }
{}

Sequence::Sequence( const Elem& elem, int64_t size )
    : comp          { elem }
    , size          { size }
    , compressed    { true }
{}

Sequence::Sequence( const Sequence& rhs, int64_t rhs_start, int64_t rhs_length )
    : size          { rhs_length }
    , compressed    { rhs.compressed }
{
    if ( compressed ) {
        comp = rhs.comp;
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;
        const auto rd_end = rd_start + rhs_length;
        data = Data( rd_start, rd_end );
    }
}

void Sequence::note_on( uint8_t pitch, uint8_t vel, int64_t wait )
{
    Elem e;
    e.pitch = pitch;
    e.vel = vel;
    e.wait = wait;
    e.dur = 0;

    if ( compressed )
        expand();

    data.push_back( e );
    size ++;
}

void Sequence::note_hold( int64_t idx, int64_t duration )
{
    at( idx ).dur += duration;
}

Elem& Sequence::at( int64_t idx )
{
    sys_assert( idx >= 0 && idx < size, "Sequence bounds error." );
    return compressed ? comp : data[idx];
}

void Sequence::expand()
{
    data = Data( size, comp );
    compressed = false;
}

void Sequence::resize( int64_t end )
{
    if ( end < size ) {
        size = end;

        if ( compressed )
            return;

        data.resize( std::max( end, (int64_t )0 ) );
        return;
    }

    size = end;

    if ( compressed ) {
        if ( comp == Elem() )
            return;

        expand();
    }

    data.resize( end );
}

void Sequence::expect( int64_t end )
{
    if ( end < size )
        return;

    size = end;

    if ( compressed ) {
        if ( comp == Elem() )
            return;

        expand();
    }

    data.resize( end );
}

void Sequence::crop( int64_t start, int64_t length )
{
    size = length;

    if ( compressed )
        return;

    data.resize( start + length );
    data.erase( data.begin(), data.begin() + start );
}

void Sequence::mask( AttrType attr )
{
    MDDL_DISAMBIGUATE_ONE_ATTR_FN( mask_attr, attr );
}

template <auto M>
void Sequence::mask_attr()
{
    for ( Elem& e : data ) {
        Elem mask;
        mask.*M = e.*M;
        e = mask;
    }
}

void Sequence::assign( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    if ( compressed ) {
        if ( rhs.compressed && comp == rhs.comp && size == length )
            return;
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    if ( rhs.compressed ) {
        std::fill( wr_start, wr_end, rhs.comp );
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;
        const auto rd_end = rd_start + length;

        std::copy( rd_start, rd_end, wr_start );
    }
}

void Sequence::assign( AttrType attr, AttrType rhs_attr, int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    MDDL_DISAMBIGUATE_TWO_ATTR_FN( assign_attr, attr, rhs_attr, start, rhs, rhs_start, length );
}
    
template <auto M1, auto M2>
void Sequence::assign_attr( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    typedef member_t<decltype(M1)> M1_t;

    if ( compressed ) {
        if ( rhs.compressed && comp.*M1 == (M1_t )(rhs.comp.*M2) && size == length )
            return;
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    if ( rhs.compressed ) {
        auto m2 = (M1_t )(rhs.comp.*M2);
        for ( auto wr = wr_start; wr < wr_end; wr ++ )
            (*wr).*M1 = m2;
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;

        auto wr = wr_start;
        auto rd = rd_start;
        for ( ; wr < wr_end; wr ++, rd ++ )
            (*wr).*M1 = (M1_t )((*rd).*M2);
    }
}

void Sequence::assign_value( AttrType attr, int64_t start, int64_t length, int64_t value )
{
    MDDL_DISAMBIGUATE_ONE_ATTR_FN( assign_value_attr, attr, start, length, value );
}

template <auto M>
void Sequence::assign_value_attr( int64_t start, int64_t length, int64_t value )
{
    typedef member_t<decltype(M)> M_t;

    M_t m_value = (M_t )value;

    if ( compressed ) {
        if ( comp.*M == m_value )
            return;

        if ( size == length ) {
            comp.*M = m_value;
            return;
        }

        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    for ( auto wr = wr_start; wr < wr_end; wr ++ )
        (*wr).*M = m_value;
}

int64_t Sequence::value()
{
    return (int64_t )(compressed ? comp.pitch : data[0].pitch);
}

int64_t Sequence::value( AttrType attr )
{
    MDDL_DISAMBIGUATE_ONE_ATTR_FN( return value_attr, attr );
    return 0;
}

template <auto M>
int64_t Sequence::value_attr()
{
    rt_assert( compressed || data.size() > 0, "Cannot get value from empty sequence." );

    return (int64_t )(compressed ? comp.*M : data[0].*M);
}

void Sequence::concat( const Sequence& rhs, int64_t rhs_start, int64_t rhs_length )
{
    if ( compressed ) {
        if ( rhs.compressed && comp == rhs.comp ) {
            size += rhs_length;
            return;
        }

        expand();
    }

    size += rhs_length;
    data.reserve( size );

    if ( rhs.compressed ) {
        for ( int i = 0; i < (int )rhs_length; i ++ )
            data.push_back( rhs.comp );
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;
        const auto rd_end = rd_start + rhs_length;
        data.insert( data.end(), rd_start, rd_end );
    }
}

void Sequence::concat( AttrType attr, AttrType rhs_attr, const Sequence& rhs, int64_t rhs_start, int64_t rhs_length )
{
    MDDL_DISAMBIGUATE_TWO_ATTR_FN( concat_attr, attr, rhs_attr, rhs, rhs_start, rhs_length );
}

template <auto M1, auto M2>
void Sequence::concat_attr( const Sequence& rhs, int64_t rhs_start, int64_t rhs_length )
{
    typedef member_t<decltype(M1)> M1_t;

    if ( compressed ) {
        if ( rhs.compressed && comp.*M1 == (M1_t )(rhs.comp.*M2) ) {
            size += rhs_length;
            return;
        }

        expand();
    }

    size += rhs_length;
    data.reserve( size );

    if ( rhs.compressed ) {
        Elem m_comp = {};
        m_comp.*M1 = (M1_t )(rhs.comp.*M2);

        for ( int i = 0; i < (int )rhs_length; i ++ )
            data.push_back( m_comp );
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;
        const auto rd_end = rd_start + rhs_length;

        for ( auto rd = rd_start; rd < rd_end; rd ++ ) {
            Elem m_rd = {};
            m_rd.*M1 = (M1_t )((*rd).*M2);
            data.push_back( m_rd );
        }
    }
}

void Sequence::extend( int64_t length )
{
    resize( size + length );
}



void Sequence::add( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    if ( rhs.compressed && rhs.comp == Elem() )
        return;
    
    if ( compressed ) {
        if ( rhs.compressed && size == length ) {
            comp += rhs.comp;
            return;
        }
        
        expand();
    }


    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    if ( rhs.compressed ) {
        for ( auto wr = wr_start; wr < wr_end; wr ++ )
            *wr += rhs.comp;
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;

        auto wr = wr_start;
        auto rd = rd_start;
        for ( ; wr < wr_end; wr ++, rd ++ )
            *wr += *rd;
    }
}

void Sequence::add( AttrType attr, AttrType rhs_attr, int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    MDDL_DISAMBIGUATE_TWO_ATTR_FN( add_attr, attr, rhs_attr, start, rhs, rhs_start, length );
}
    
template <auto M1, auto M2>
void Sequence::add_attr( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    typedef member_t<decltype(M1)> M1_t;

    M1_t m_comp = (M1_t )(rhs.comp.*M2);
    if ( rhs.compressed && m_comp == 0 )
        return;

    if ( compressed ) {
        if ( rhs.compressed && size == length ) {
            comp.*M1 += m_comp;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    if ( rhs.compressed ) {
        for ( auto wr = wr_start; wr < wr_end; wr ++ )
            (*wr).*M1 += m_comp;
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;

        auto wr = wr_start;
        auto rd = rd_start;
        for ( ; wr < wr_end; wr ++, rd ++ )
            (*wr).*M1 += (M1_t )((*rd).*M2);
    }
}

void Sequence::add_value( AttrType attr, int64_t start, int64_t length, int64_t value )
{
    MDDL_DISAMBIGUATE_ONE_ATTR_FN( add_value_attr, attr, start, length, value );
}
    
template <auto M>
void Sequence::add_value_attr( int64_t start, int64_t length, int64_t value )
{
    typedef member_t<decltype(M)> M_t;

    M_t m_value = (M_t )value;
    if ( m_value == 0 )
        return;

    if ( compressed ) {
        if ( size == length ) {
            comp.*M += m_value;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    for ( auto wr = wr_start; wr < wr_end; wr ++ )
        (*wr).*M += m_value;
}



void Sequence::subtract( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    if ( rhs.compressed && rhs.comp == Elem() )
        return;
    
    if ( compressed ) {
        if ( rhs.compressed && size == length ) {
            comp -= rhs.comp;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    if ( rhs.compressed ) {
        for ( auto wr = wr_start; wr < wr_end; wr ++ )
            *wr -= rhs.comp;
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;

        auto wr = wr_start;
        auto rd = rd_start;
        for ( ; wr < wr_end; wr ++, rd ++ )
            *wr -= *rd;
    }
}

void Sequence::subtract( AttrType attr, AttrType rhs_attr, int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    MDDL_DISAMBIGUATE_TWO_ATTR_FN( subtract_attr, attr, rhs_attr, start, rhs, rhs_start, length );
}
    
template <auto M1, auto M2>
void Sequence::subtract_attr( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    typedef member_t<decltype(M1)> M1_t;

    M1_t m_comp = (M1_t )(rhs.comp.*M2);
    if ( rhs.compressed && m_comp == 0 )
        return;
    
    if ( compressed ) {
        if ( rhs.compressed && size == length ) {
            comp.*M1 -= m_comp;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    if ( rhs.compressed ) {
        for ( auto wr = wr_start; wr < wr_end; wr ++ )
            (*wr).*M1 -= m_comp;
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;

        auto wr = wr_start;
        auto rd = rd_start;
        for ( ; wr < wr_end; wr ++, rd ++ )
            (*wr).*M1 -= (M1_t )((*rd).*M2);
    }
}

void Sequence::subtract_value( AttrType attr, int64_t start, int64_t length, int64_t value )
{
    MDDL_DISAMBIGUATE_ONE_ATTR_FN( subtract_value_attr, attr, start, length, value );
}
    
template <auto M>
void Sequence::subtract_value_attr( int64_t start, int64_t length, int64_t value )
{
    typedef member_t<decltype(M)> M_t;

    M_t m_value = (M_t )value;
    if ( m_value == 0 )
        return;

    if ( compressed ) {
        if ( size == length ) {
            comp.*M -= m_value;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    for ( auto wr = wr_start; wr < wr_end; wr ++ )
        (*wr).*M -= m_value;
}



void Sequence::multiply( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    if ( compressed ) {
        if ( rhs.compressed && size == length ) {
            comp *= rhs.comp;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    if ( rhs.compressed ) {
        for ( auto wr = wr_start; wr < wr_end; wr ++ )
            *wr *= rhs.comp;
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;

        auto wr = wr_start;
        auto rd = rd_start;
        for ( ; wr < wr_end; wr ++, rd ++ )
            *wr *= *rd;
    }
}

void Sequence::multiply( AttrType attr, AttrType rhs_attr, int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    MDDL_DISAMBIGUATE_TWO_ATTR_FN( multiply_attr, attr, rhs_attr, start, rhs, rhs_start, length );
}
    
template <auto M1, auto M2>
void Sequence::multiply_attr( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    typedef member_t<decltype(M1)> M1_t;

    M1_t m_comp = (M1_t )(rhs.comp.*M2);
    if ( rhs.compressed && m_comp == 1 )
        return;
    
    if ( compressed ) {
        if ( rhs.compressed && size == length ) {
            comp.*M1 *= m_comp;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    if ( rhs.compressed ) {
        for ( auto wr = wr_start; wr < wr_end; wr ++ )
            (*wr).*M1 *= m_comp;
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;

        auto wr = wr_start;
        auto rd = rd_start;
        for ( ; wr < wr_end; wr ++, rd ++ )
            (*wr).*M1 *= (M1_t )((*rd).*M2);
    }
}

void Sequence::multiply_value( AttrType attr, int64_t start, int64_t length, int64_t value )
{
    MDDL_DISAMBIGUATE_ONE_ATTR_FN( multiply_value_attr, attr, start, length, value );
}
    
template <auto M>
void Sequence::multiply_value_attr( int64_t start, int64_t length, int64_t value )
{
    typedef member_t<decltype(M)> M_t;

    M_t m_value = (M_t )value;
    if ( m_value == 1 )
        return;

    if ( compressed ) {
        if ( size == length ) {
            comp.*M *= m_value;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    for ( auto wr = wr_start; wr < wr_end; wr ++ )
        (*wr).*M *= m_value;
}



void Sequence::divide( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    if ( compressed ) {
        if ( rhs.compressed && size == length ) {
            comp /= rhs.comp;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    if ( rhs.compressed ) {
        for ( auto wr = wr_start; wr < wr_end; wr ++ )
            *wr /= rhs.comp;
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;

        auto wr = wr_start;
        auto rd = rd_start;
        for ( ; wr < wr_end; wr ++, rd ++ )
            *wr /= *rd;
    }
}

void Sequence::divide( AttrType attr, AttrType rhs_attr, int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    MDDL_DISAMBIGUATE_TWO_ATTR_FN( divide_attr, attr, rhs_attr, start, rhs, rhs_start, length );
}
    
template <auto M1, auto M2>
void Sequence::divide_attr( int64_t start, const Sequence& rhs, int64_t rhs_start, int64_t length )
{
    typedef member_t<decltype(M1)> M1_t;

    M1_t m_comp = (M1_t )(rhs.comp.*M2);
    if ( rhs.compressed && m_comp == 1 )
        return;
    
    if ( compressed ) {
        if ( rhs.compressed && size == length ) {
            comp.*M1 /= m_comp;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    if ( rhs.compressed ) {
        for ( auto wr = wr_start; wr < wr_end; wr ++ )
            (*wr).*M1 /= m_comp;
    } else {
        const auto rd_start = rhs.data.cbegin() + rhs_start;

        auto wr = wr_start;
        auto rd = rd_start;
        for ( ; wr < wr_end; wr ++, rd ++ )
            (*wr).*M1 /= (M1_t )((*rd).*M2);
    }
}

void Sequence::divide_value( AttrType attr, int64_t start, int64_t length, int64_t value )
{
    MDDL_DISAMBIGUATE_ONE_ATTR_FN( divide_value_attr, attr, start, length, value );
}
    
template <auto M>
void Sequence::divide_value_attr( int64_t start, int64_t length, int64_t value )
{
    typedef member_t<decltype(M)> M_t;

    M_t m_value = (M_t )value;
    if ( m_value == 1 )
        return;

    if ( compressed ) {
        if ( size == length ) {
            comp.*M /= m_value;
            return;
        }
        
        expand();
    }

    const auto wr_start = data.begin() + start;
    const auto wr_end = wr_start + length;

    for ( auto wr = wr_start; wr < wr_end; wr ++ )
        (*wr).*M /= m_value;
}


void Sequence::print()
{
    std::cout << "Seq: " << std::hex << this << "\n";
    if ( compressed ) {
        std::cout << "[ " << (int )comp.pitch << ", " << (int )comp.vel
            << ", " << comp.dur << ", " << comp.wait << " ] x " << size << "\n";
        return;
    }

    for ( Sequence::Elem& e : data )
        std::cout << "[ " << (int )e.pitch << ", " << (int )e.vel
            << ", " << e.dur << ", " << e.wait << " ]\n";
}

} // namespace MDDL

#undef MDDL_DISAMBIGUATE_ATTR_FN