#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Math.hpp>
#include <array>
#include <cfloat>
#include <cstdlib>  //for rand
#include <type_traits>
// #include <utility>
#include <cmath>

#ifdef WP_USE_RANDOMC_LIB
#    include <random/randomc.h>
#endif

#if WP_USE_BOOST
#    include <boost/algorithm/clamp.hpp>
#    include <boost/math/constants/constants.hpp>
#    include <boost/math/special_functions/relative_difference.hpp>
#endif

namespace workphone
{
    template <>
    s32 Math<s8>::m_randomSeed = 0;

    template <>
    s32 Math<u8>::m_randomSeed = 0;

    template <>
    s32 Math<s16>::m_randomSeed = 0;

    template <>
    s32 Math<u16>::m_randomSeed = 0;

    template <>
    s32 MathI::m_randomSeed = 0;

    template <>
    s32 Math<u32>::m_randomSeed = 0;

    template <>
    s32 Math<s64>::m_randomSeed = 0;

    template <>
    s32 MathF::m_randomSeed = 0;

    template <>
    s32 MathD::m_randomSeed = 0;

    // template <>
#if defined( __ANDROID__ )
    template <>
    s32 Math<size_t>::m_randomSeed = 0;
#endif

    template <>
    s32 Math<u64>::m_randomSeed = 0;

    template <>
    s32 Math<long double>::m_randomSeed = 0;

    // template<>
    // const s32 Math<s32>::EPSILON = (s32)FLT_EPSILON;
    // template<>
    // const s32 Math<s32>::ZERO_TOLERANCE = 0;
    // template<>
    // const s32 Math<s32>::MAX_F32 = 0;
    // template<>
    // const s32 Math<s32>::PI = (s32)((float)(4.0*atan(1.0)));
    // template<>
    // const s32 Math<s32>::TWO_PI = (s32)2.0f*Math<s32>::PI;
    // template<>
    // const s32 Math<s32>::HALF_PI = (s32)0.5f*Math<s32>::PI;
    // template<>
    // const s32 Math<s32>::INV_PI = (s32)(1.0f/Math<s32>::PI);
    // template<>
    // const s32 Math<s32>::INV_TWO_PI = (s32)(1.0f/Math<s32>::TWO_PI);
    // template<>
    // const s32 Math<s32>::DEG_TO_RAD = (s32)(Math<s32>::PI/180.0f);
    // template<>
    // const s32 Math<s32>::RAD_TO_DEG = (s32)(180.0f/Math<s32>::PI);
    // template<>
    // const s64 Math<s64>::EPSILON = FLT_EPSILON;
    // template<>
    // const s64 Math<s64>::ZERO_TOLERANCE = 1e-06f;
    //
    // template<>
    // const s64 Math<s64>::MAX_F32 = 0;
    // template<>
    // const s64 Math<s64>::PI = (float)(4.0*atan(1.0));
    // template<>
    // const s64 Math<s64>::TWO_PI = 2.0f*Math<s64>::PI;
    // template<>
    // const s64 Math<s64>::HALF_PI = 0.5f*Math<s64>::PI;
    // template<>
    // const s64 Math<s64>::INV_PI = 1.0f/Math<s64>::PI;
    // template<>
    // const s64 Math<s64>::INV_TWO_PI = 1.0f/Math<s64>::TWO_PI;
    // template<>
    // const s64 Math<s64>::DEG_TO_RAD = Math<s64>::PI/180.0f;
    // template<>
    // const s64 Math<s64>::RAD_TO_DEG = 180.0f/Math<s64>::PI;
    // template<>
    // const f32 Math<f32>::epsilon() = FLT_EPSILON;
    // template<>
    // const f32 Math<f32>::ZERO_TOLERANCE = 1e-06f;
    // template<>
    // const f32 Math<f32>::MAX_F32 = FLT_MAX;
    //
    // template<>
    // const f32 Math<f32>::PI = (f32)((float)(4.0*atan(1.0)));
    // template<>
    // const f32 Math<f32>::TWO_PI = (f32)2.0f*Math<f32>::PI;

    // template<>
    // const f32 Math<f32>::HALF_PI = 0.5f*Math<f32>::PI;
    // template<>
    // const f32 Math<f32>::INV_PI = 1.0f/Math<f32>::PI;
    // template<>
    // const f32 Math<f32>::INV_TWO_PI = 1.0f/Math<f32>::TWO_PI;
    // template<>
    // const f32 Math<f32>::DEG_TO_RAD = Math<f32>::PI/180.0f;
    // template<>
    // const f32 Math<f32>::RAD_TO_DEG = 180.0f/Math<f32>::PI;
    //
    // template<>
    // const f64 Math<f64>::EPSILON = FLT_EPSILON;
    //
    // template<>
    // const f64 Math<f64>::ZERO_TOLERANCE = 1e-06f;
    // template<>
    // const f64 Math<f64>::MAX_F32 = FLT_MAX;
    // template<>
    // const f64 Math<f64>::PI = (float)(4.0*atan(1.0));
    // template<>
    // const f64 Math<f64>::TWO_PI = 2.0f*Math<f64>::PI;
    // template<>
    // const f64 Math<f64>::HALF_PI = 0.5f*Math<f64>::PI;
    // template<>
    // const f64 Math<f64>::INV_PI = 1.0f/Math<f64>::PI;
    // template<>
    // const f64 Math<f64>::INV_TWO_PI = 1.0f/Math<f64>::TWO_PI;
    // template<>
    // const f64 Math<f64>::DEG_TO_RAD = Math<f64>::PI/180.0f;
    // template<>
    // const f64 Math<f64>::RAD_TO_DEG = 180.0f/Math<f64>::PI;

    // Returns a random number between min and max
    template <class T>
    auto ranged_random( T min, T max, [[maybe_unused]] int seed = 0 ) -> T
    {
#ifndef WP_USE_RANDOMC_LIB
        return min + ( max - min ) * rand() / 0x7fff;
#else
        TRandomMersenne random( seed );

        // f32 randNum = (f32)rand();
        f32 randNum = (f32)random.IRandom( 0, RAND_MAX );
        f32 randRatio = randNum / (f32)RAND_MAX;
        f32 result = (f32)min + ( (f32)max - (f32)min ) * randRatio;
        return (T)result;
#endif
    }

    template <>
    auto Math<s32>::equals( const s32 af, const s32 bf, const s32 maxDiff ) -> bool
    {
#if WP_USE_BOOST
        return boost::math::epsilon_difference( af, bf ) < maxDiff;
#else
        return af == bf;
#endif
    }

    template <>
    auto Math<s64>::equals( const s64 af, const s64 bf, const s64 maxDiff ) -> bool
    {
#if WP_USE_BOOST
        return Abs( af - bf ) < maxDiff;
#else
        return af == bf;
#endif
    }

    template <>
    auto Math<u64>::equals( const u64 af, const u64 bf, const u64 maxDiff ) -> bool
    {
        const auto diff = af > bf ? af - bf : bf - af;
        return diff < maxDiff;
    }

    template <>
    auto Math<long double>::equals( const long double af, const long double bf,
                                    const long double maxDiff ) -> bool
    {
#if WP_USE_BOOST
        return boost::math::epsilon_difference( af, bf ) < maxDiff;
#else
        return af == bf;
#endif
    }

    template <>
    auto Math<f32>::equals( const f32 af, const f32 bf, const f32 maxDiff ) -> bool
    {
        // from TAMING THE FLOATING POINT BEAST
#if 0
		// fast routine, not portable due to shifting signed int
		// works on Microsoft compilers.
		int ai = *reinterpret_cast<int*>(&af);
		int bi = *reinterpret_cast<int*>(&bf);
		int test = (ai^bi)>>31;
		//assert((0 == test) || (0xFFFFFFFF == test));
		int diff = (((0x80000000 - ai)&(test)) | (ai & (~test))) - bi;
		int v1 = maxDiff + diff;
		int v2 = maxDiff - diff;
		return (v1|v2) >= 0;
#else
        // solid, fast routine across all platforms
        // with constant time behavior
        const int ai = *reinterpret_cast<const int *>( &af );
        const int bi = *reinterpret_cast<const int *>( &bf );
        int test = ( static_cast<unsigned>( ai ^ bi ) >> 31 ) - 1;
        // assert((0 == test) || (0xFFFFFFFF == test));
        int diff = ( ( ( 0x80000000 - ai ) & ( ~test ) ) | ( ai & test ) ) - bi;
        int v1 = static_cast<s32>( maxDiff ) + diff;
        int v2 = static_cast<s32>( maxDiff ) - diff;
        return ( v1 | v2 ) >= 0;
#endif
    }

    template <>
    auto Math<f64>::equals( const f64 af, const f64 bf, [[maybe_unused]] const f64 maxDiff ) -> bool
    {
#if WP_USE_BOOST
        return boost::math::epsilon_difference( af, bf ) < maxDiff;
#else
        return af == bf;
#endif
    }

    template <>
    auto Math<s32>::RangedRandom( s32 min, s32 max, s32 seed ) -> s32
    {
        if( seed )
        {
            return ranged_random<s32>( min, max, seed );
        }

        return ranged_random<s32>( min, max, m_randomSeed++ );
    }

    template <>
    auto Math<s64>::RangedRandom( s64 min, s64 max, s32 seed ) -> s64
    {
        if( seed )
        {
            return ranged_random<s64>( min, max, seed );
        }

        return ranged_random<s64>( min, max, m_randomSeed++ );
    }

    template <>
    auto Math<u64>::RangedRandom( u64 min, u64 max, s32 seed ) -> u64
    {
        if( seed )
        {
            return ranged_random<u64>( min, max, seed );
        }

        return ranged_random<s64>( min, max, m_randomSeed++ );
    }

    template <>
    auto Math<long double>::RangedRandom( long double min, long double max, s32 seed ) -> long double
    {
        if( seed )
        {
            return ranged_random<long double>( min, max, seed );
        }

        return ranged_random<long double>( min, max, m_randomSeed++ );
    }

    template <>
    auto Math<f32>::RangedRandom( f32 min, f32 max, s32 seed ) -> f32
    {
        if( seed )
        {
            return ranged_random<f32>( min, max, seed );
        }

        return ranged_random<f32>( min, max, m_randomSeed++ );
    }

    template <>
    auto Math<f64>::RangedRandom( f64 min, f64 max, s32 seed ) -> f64
    {
        if( seed )
        {
            return ranged_random<f64>( min, max, seed );
        }

        return ranged_random<f64>( min, max, m_randomSeed++ );
    }

    template <>
    auto Math<s32>::isFinite( s32 value ) -> bool
    {
        return value > std::numeric_limits<s32>::min() && value < std::numeric_limits<s32>::max();
    }

    template <>
    auto Math<s64>::isFinite( s64 value ) -> bool
    {
        return value > std::numeric_limits<s64>::min() && value < std::numeric_limits<s64>::max();
    }

    template <>
    auto Math<u64>::isFinite( u64 value ) -> bool
    {
        return value < std::numeric_limits<u64>::max();
    }

    template <>
    auto Math<long double>::isFinite( long double value ) -> bool
    {
        return value > std::numeric_limits<long double>::min() &&
               value < std::numeric_limits<long double>::max();
    }

    template <>
    auto Math<f32>::isFinite( f32 value ) -> bool
    {
        return std::isfinite( value );
    }

    template <>
    auto Math<f64>::isFinite( f64 value ) -> bool
    {
        return std::isfinite( value );
    }

    template <>
    auto Math<s32>::isNaN( s32 value ) -> bool
    {
        return value == value;
    }

    template <>
    auto Math<f32>::isNaN( f32 value ) -> bool
    {
        return std::isnan( value );
    }

    template <>
    auto Math<s64>::isNaN( s64 value ) -> bool
    {
        return value == value;
    }

    template <>
    auto Math<u64>::isNaN( [[maybe_unused]] u64 value ) -> bool
    {
        return false;
    }

    template <>
    auto Math<f64>::isNaN( f64 value ) -> bool
    {
        return std::isnan( value );
    }

    template <>
    auto Math<long double>::isNaN( long double value ) -> bool
    {
        return std::isnan( static_cast<f64>( value ) );
    }

    template <class T>
    void Math<T>::Limit( T &v, T value )
    {
        if( v > value )
        {
            v = value;
        }
    }

    template <class T>
    auto Math<T>::trunc( T value ) -> T
    {
        return static_cast<T>( std::trunc( static_cast<float>( value ) ) );
    }

    template <class T>
    auto Math<T>::epsilon() -> T
    {
        return std::numeric_limits<T>::epsilon();
    }

    template <class T>
    auto Math<T>::ASin( T fValue ) -> T
    {
        if( T( -1.0 ) < fValue )
        {
            if( fValue < T( 1.0 ) )
            {
                return T( asin( static_cast<f32>( fValue ) ) );
            }
            return Math<T>::half_pi();
        }

        return Math<T>::half_pi();
    }

    template <class T>
    auto Math<T>::wrapRadians( T r ) -> T
    {
        return static_cast<T>( std::remainder( r, T( 2.0 ) * Math<T>::pi() ) );
    }

    template <class T>
    auto Math<T>::wrapDegrees( T degrees, T range ) -> T
    {
        return static_cast<T>( std::remainder( degrees, T( 2.0 ) * range ) );
    }

    template <class T>
    auto Math<T>::wrapGradians( T r ) -> T
    {
        float i;
        r = static_cast<T>( modff( static_cast<float>( r ), &i ) );

        if( r > T( 0.5 ) )
        {
            r -= T( 1.0 );
        }

        if( r <= T( -0.5 ) )
        {
            r += T( 1.0 );
        }

        return r;
    }

    template <class T>
    auto Math<T>::wrap( T value, T lowerBound, T upperBound ) -> T
    {
        if( value < lowerBound || value > upperBound )
        {
            return static_cast<T>( std::fmod( value, upperBound ) );
        }

        return value;
    }

    template <>
    auto Math<s32>::ACos( s32 fValue ) -> s32
    {
        if( static_cast<s32>( -1.0 ) < fValue )
        {
            if( fValue < static_cast<s32>( 1.0 ) )
            {
                return static_cast<s32>( acos( static_cast<f32>( fValue ) ) );
            }
            return static_cast<s32>( 0.0 );
        }
        return pi();
    }

    template <class T>
    auto Math<T>::ACos( T fValue ) -> T
    {
        if( T( -1.0 ) < fValue )
        {
            if( fValue < T( 1.0 ) )
            {
                return static_cast<T>( ::acos( fValue ) );
            }
            return T( 0.0 );
        }
        return pi();
    }

    template <class T>
    auto Math<T>::Ln( T value ) -> T
    {
        return static_cast<T>( std::log( value ) );
    }

    template <class T>
    auto Math<T>::Round( T value ) -> s32
    {
        return static_cast<s32>( ::floor( value + 0.5f ) );
    }

    template <class T>
    auto Math<T>::Sin( T value ) -> T
    {
        return static_cast<T>( sin( static_cast<f32>( value ) ) );
    }

    template <class T>
    auto Math<T>::Cos( T value ) -> T
    {
        return static_cast<T>( cos( static_cast<f32>( value ) ) );
    }

    template <class T>
    auto Math<T>::Tan( T value ) -> T
    {
        return static_cast<T>( tan( static_cast<f32>( value ) ) );
    }

    template <>
    auto Math<s32>::Atan( s32 x ) -> s32
    {
        return static_cast<s32>( std::atan( static_cast<f32>( x ) ) );
    }

    template <class T>
    auto Math<T>::Atan( T x ) -> T
    {
        return static_cast<T>( std::atan( x ) );
    }

    template <>
    auto Math<s32>::ATan2( const s32 &y, const s32 &x ) -> s32
    {
        return static_cast<s32>( std::atan2( static_cast<f32>( y ), static_cast<f32>( x ) ) );
    }

    template <class T>
    auto Math<T>::ATan2( const T &y, const T &x ) -> T
    {
        return static_cast<T>( std::atan2( y, x ) );
    }

    template <class T>
    auto Math<T>::Sqr( T value ) -> T
    {
        return value * value;
    }

    template <class T>
    auto Math<T>::Sqrt( T value ) -> T
    {
        return static_cast<T>( std::sqrt( value ) );
    }

    template <>
    auto Math<s32>::Sqrt( s32 value ) -> s32
    {
        return static_cast<s32>( std::sqrt( static_cast<f32>( value ) ) );
    }

    template <>
    auto Math<s64>::Sqrt( s64 value ) -> s64
    {
        return static_cast<s64>( std::sqrt( static_cast<f32>( value ) ) );
    }

    template <class T>
    auto Math<T>::SqrtInv( T value ) -> T
    {
        return T( 1.0 ) / Math<T>::Sqrt( value );
    }

    template <>
    auto Math<s32>::SqrtInv( s32 value ) -> s32
    {
        return static_cast<s32>( static_cast<float>( 1.0 ) / sqrt( static_cast<float>( value ) ) );
    }

    template <>
    auto Math<s64>::SqrtInv( s64 value ) -> s64
    {
        const auto sqrtValue = std::sqrt( static_cast<f32>( value ) );
        const auto fValue = static_cast<f32>( 1.0 ) / sqrtValue;
        return static_cast<s64>( fValue );
    }

    template <class T>
    auto Math<T>::Abs( T value ) -> T
    {
        if constexpr( std::is_unsigned_v<T> )
        {
            return value;
        }
        else
        {
            return ( value < T( 0 ) ) ? -value : value;
        }
    }

    template <class T>
    auto Math<T>::normalize( const T &value, const T &min, const T &max ) -> T
    {
        return ( ( value - min ) / ( max - min ) );
    }

    template <class T>
    auto Math<T>::isZero( const T a, const T tolerance ) -> bool
    {
        return Math<T>::Abs( a ) < tolerance;
    }

    template <class T>
    auto Math<T>::DegToRad( const T &deg ) -> T
    {
        return static_cast<T>( deg * deg_to_rad() );
    }

    template <class T>
    auto Math<T>::RadToDeg( const T &rad ) -> T
    {
        return static_cast<T>( rad * rad_to_deg() );
    }

    template <class T>
    auto Math<T>::RadToFullDegrees( const T &rad ) -> T
    {
        T angleDegrees = static_cast<T>( rad * rad_to_deg() );
        if( angleDegrees < T( 0.0 ) )
        {
            angleDegrees = angleDegrees + T( 360.0 );
        }

        return angleDegrees;
    }

    template <class T>
    void Math<T>::SetRandomSeed( s32 seed )
    {
        m_randomSeed = seed;
    }

    template <class T>
    void Math<T>::swap( T &firstValue, T &secondValue )
    {
        auto tmp = firstValue;
        firstValue = secondValue;
        secondValue = tmp;
    }

    template <class T>
    void Math<T>::swapIfGreater( T &firstValue, T &secondValue )
    {
        if( firstValue > secondValue )
        {
            swap( firstValue, secondValue );
        }
    }

    template <class T>
    void Math<T>::swapIfLessThan( T &firstValue, T &secondValue )
    {
        if( firstValue < secondValue )
        {
            swap( firstValue, secondValue );
        }
    }

    template <class T>
    auto Math<T>::makeMultipleOf( T numToRound, T multiple ) -> T
    {
        if( multiple == T( 0.0 ) )
        {
            return numToRound;
        }

        auto remainder = Mod( Abs( numToRound ), multiple );
        if( remainder == T( 0.0 ) )
        {
            return numToRound;
        }

        if constexpr( !std::is_unsigned_v<T> )
        {
            if( numToRound < T( 0.0 ) )
            {
                return -( Abs( numToRound ) - remainder );
            }
        }

        return numToRound + multiple - remainder;
    }

    template <class T>
    auto Math<T>::nextPow2( T x ) -> T
    {
        T y;
        for( y = 1; y < x; y *= 2 )
        {
            ;
        }
        return y;
    }

    template <class T>
    auto Math<T>::Exp( T value ) -> T
    {
        return static_cast<T>( ::exp( value ) );
    }

    template <typename T>
    auto Math<T>::Sign( T value ) -> s32
    {
        return ( T( 0 ) < value ) - ( value < T( 0 ) );
    }

    template <class T>
    auto Math<T>::half_pi() -> T
    {
        return pi() / T( 2.0 );
    }

    template <class T>
    auto Math<T>::two_pi() -> T
    {
        return pi() * T( 2.0 );
    }

    template <class T>
    auto Math<T>::pi() -> T
    {
        return T( 3.14159265358979323846 );
    }

    template <class T>
    auto Math<T>::Floor( T value ) -> T
    {
        return static_cast<T>( std::floor( value ) );
    }

    template <class T>
    auto Math<T>::FloorToInt( T value ) -> s32
    {
        return static_cast<s32>( std::floor( value ) );
    }

    template <>
    auto Math<s32>::Mod( s32 x, s32 y ) -> s32
    {
        return static_cast<s32>( std::fmod( static_cast<f32>( x ), static_cast<f32>( y ) ) );
    }

    template <class T>
    auto Math<T>::Mod( T x, T y ) -> T
    {
        return static_cast<T>( std::fmod( x, y ) );
    }

    template <class T>
    auto Math<T>::Pow( T value, T powVal ) -> T
    {
        return static_cast<T>( ::pow( value, powVal ) );
    }

    template <class T>
    auto Math<T>::roundToFloat( T value ) -> f32
    {
        return static_cast<f32>( floor( static_cast<double>( value + T( 0.5 ) ) ) );
    }

    template <class T>
    auto Math<T>::roundToInt( T value ) -> s32
    {
        return static_cast<int>( floor( static_cast<double>( value + T( 0.5 ) ) ) );
    }

    template <class T>
    auto Math<T>::round( T value ) -> T
    {
        return static_cast<T>( floor( static_cast<double>( value + T( 0.5 ) ) ) );
    }

    template <class T>
    auto Math<T>::round( T value, u32 prec ) -> T
    {
        auto pow_10 = Math<T>::Pow( T( 10.0 ), static_cast<T>( prec ) );
        return Math<T>::round( value * pow_10 ) / pow_10;
    }

    static std::array<int, 257> perm = {
        151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103,
        30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,
        197, 62,  94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174,
        20,  125, 136, 171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231,
        83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143,
        54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196,
        135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124,
        123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,
        182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101,
        155, 167, 43,  172, 9,   129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185,
        112, 104, 218, 246, 97,  228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,
        51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184, 84,  204, 176,
        115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,  222, 114, 67,  29,  24,  72,  243,
        141, 128, 195, 78,  66,  215, 61,  156, 180, 151
    };

    template <class T>
    auto Math<T>::PerlinNoise( T x, T y ) -> T
    {
        auto X = Math<T>::FloorToInt( x ) & 0xff;
        auto Y = Math<T>::FloorToInt( y ) & 0xff;
        x -= Math<T>::Floor( x );
        y -= Math<T>::Floor( y );
        auto u = Fade( x );
        auto v = Fade( y );
        auto A = ( perm[X] + Y ) & 0xff;
        auto B = ( perm[X + 1] + Y ) & 0xff;
        return lerp( v, lerp( u, Grad( perm[A], x, y ), Grad( perm[B], x - 1, y ) ),
                     lerp( u, Grad( perm[A + 1], x, y - 1 ), Grad( perm[B + 1], x - 1, y - 1 ) ) );
    }

    template <class T>
    auto Math<T>::Fade( T t ) -> T
    {
        return t * t * t * ( t * ( t * T( 6 ) - T( 15 ) ) + T( 10 ) );
    }

    template <class T>
    auto Math<T>::Lerp( T t, T a, T b ) -> T
    {
        return a + t * ( b - a );
    }

    template <class T>
    auto Math<T>::Grad( int hash, T x ) -> T
    {
        return ( hash & 1 ) == 0 ? x : T( 0 ) - x;
    }

    template <class T>
    auto Math<T>::Grad( int hash, T x, T y ) -> T
    {
        return ( ( hash & 1 ) == 0 ? x : T( 0 ) - x ) + ( ( hash & 2 ) == 0 ? y : T( 0 ) - y );
    }

    template <class T>
    auto Math<T>::Grad( int hash, T x, T y, T z ) -> T
    {
        auto h = hash & 15;
        auto u = h < 8 ? x : y;
        auto v = h < 4 ? y : ( h == 12 || h == 14 ? x : z );
        return ( ( h & 1 ) == 0 ? u : T( 0 ) - u ) + ( ( h & 2 ) == 0 ? v : T( 0 ) - v );
    }

    template <class T>
    auto Math<T>::deg_to_rad() -> T
    {
        return Math<T>::pi() / T( 180.0 );
    }

    template <class T>
    auto Math<T>::rad_to_deg() -> T
    {
        return T( 180.0 ) / Math<T>::pi();
    }

    template <class T>
    T Math<T>::smoothDamp( T current, T target, T &velocity, T time, T smoothTime )
    {
        // Based on Unity's Mathf.SmoothDamp implementation.
        const T eps = static_cast<T>( 1e-6 );
        if( smoothTime < eps )
        {
            // Immediate snap
            velocity = T( 0 );
            return target;
        }

        T omega = static_cast<T>( 2 ) / smoothTime;
        T x = omega * time;
        T exp = static_cast<T>( 1 ) / ( static_cast<T>( 1 ) + x + static_cast<T>( 0.48 ) * x * x +
                                        static_cast<T>( 0.235 ) * x * x * x );

        T change = current - target;
        T temp = ( velocity + omega * change ) * time;
        velocity = ( velocity - omega * temp ) * exp;
        T result = target + ( change + temp ) * exp;

        // Prevent overshooting
        if( ( target - current ) > T( 0 ) == result > target )
        {
            result = target;
            velocity = T( 0 );
        }

        return result;
    }

    template <class T>
    T Math<T>::smoothDampAngle( T current, T target, T &vel, T smoothTime )
    {
        // Ensure smoothTime is not too small to avoid division issues
        const T epsilon = static_cast<T>( 1e-6 );
        if( smoothTime < epsilon )
        {
            return target;
        }

        // Wrap the angle difference to the range [-180, 180]
        T delta = target - current;
        while( delta > static_cast<T>( 180 ) )
            delta -= static_cast<T>( 360 );
        while( delta < static_cast<T>( -180 ) )
            delta += static_cast<T>( 360 );

        // Use exponential smoothing
        T omega = static_cast<T>( 2.0 ) / smoothTime;
        T x = omega * smoothTime;
        T expFactor =
            static_cast<T>( 1.0 ) / ( static_cast<T>( 1.0 ) + x + static_cast<T>( 0.48 ) * x * x +
                                      static_cast<T>( 0.235 ) * x * x * x );

        // Compute the target velocity
        T temp = ( vel + omega * delta ) * smoothTime;
        vel = ( vel - omega * temp ) * expFactor;

        // Compute the final smoothed angle
        T result = current + ( delta + temp ) * expFactor;

        return result;
    }

    template <class T>
    T Math<T>::smoothDampAngle( T current, T target, T vel, T time, T smoothTime )
    {
        // Calculate the difference between the target and current angles
        T deltaAngle = target - current;

        // Ensure the delta angle is between -180 and 180 degrees
        if( deltaAngle > T( 180 ) )
        {
            deltaAngle -= T( 360 );
        }
        else if( deltaAngle < T( -180 ) )
        {
            deltaAngle += T( 360 );
        }

        // Calculate the maximum allowed change in velocity
        T maxDelta = T( 2.0 ) / smoothTime * time;

        // Clamp the deltaAngle within the allowed range
        T minDelta = T( 0 );
        if constexpr( !std::is_unsigned_v<T> )
        {
            minDelta = -maxDelta;
        }

        deltaAngle = Math<T>::clamp( deltaAngle, minDelta, maxDelta );

        // Update the velocity
        vel = ( vel + deltaAngle * T( 2.0 ) / smoothTime ) / ( T( 1.0 ) + time * T( 2.0 ) / smoothTime );

        // Calculate the new angle
        T newAngle = current + vel * time;

        return newAngle;
    }

    template <class T>
    T Math<T>::inverseLerp( const T a, const T b, const T t )
    {
        if( Abs( b - a ) < epsilon() )
        {
            return a;
        }

        return ( t - a ) / ( b - a );
    }

    template <class T>
    T Math<T>::bound( T value, T boundary1, T boundary2 )
    {
        if( boundary1 > boundary2 )
        {
            if( !( value >= boundary2 ) )
            {
                return boundary2;
            }
            if( !( value <= boundary1 ) )
            {
                return boundary1;
            }
        }
        else
        {
            if( !( value >= boundary1 ) )
            {
                return boundary1;
            }
            if( !( value <= boundary2 ) )
            {
                return boundary2;
            }
        }

        return value;
    }

    template <class T>
    T Math<T>::reciprocal( const T &f )
    {
        return T( 1.0 ) / f;
    }

    template <class T>
    T Math<T>::clamp01( T value )
    {
        return clamp( value, T( 0.0 ), T( 1.0 ) );
    }

    template <class T>
    T Math<T>::min( T a, T b )
    {
        return a < b ? a : b;
    }

    template <class T>
    T Math<T>::max( T a, T b )
    {
        return a < b ? b : a;
    }

    template <class T>
    const T Math<T>::clamp( T value, T low, T high )
    {
        return value < low ? low : ( value > high ? high : value );
    }

    // explicit instantiation
    template class Math<s8>;
    template class Math<u8>;
    template class Math<s16>;
    template class Math<u16>;
    template class Math<s32>;
    template class Math<u32>;
    template class Math<s64>;
    template class Math<f32>;
    template class Math<f64>;

#ifdef WP_PLATFORM_WIN32
    template class Math<long long>;
#endif

    template class Math<size_t>;
    template class Math<u64>;

}  // namespace workphone
