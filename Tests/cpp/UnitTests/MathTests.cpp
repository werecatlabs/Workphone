#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/Debug.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <set>
#include <vector>

using namespace workphone;

BOOST_AUTO_TEST_CASE( math_class )
{
    try
    {
        auto fValue = static_cast<real_Num>( 0.0 );
        auto fACos0 = std::acos( fValue );
        auto fACos1 = Math<real_Num>::ACos( fValue );
        BOOST_CHECK( Math<real_Num>::equals( fACos0, fACos1 ) == true );

        fValue = static_cast<real_Num>( 1.0 );
        fACos0 = std::acos( fValue );
        fACos1 = Math<real_Num>::ACos( fValue );
        BOOST_CHECK( Math<real_Num>::equals( fACos0, fACos1 ) == true );

        auto fVec0 = Vector3<real_Num>( 0, 0, 1 );
        auto fVec1 = Vector3<real_Num>( 0, 1, 0 );
        auto atanValue = Math<real_Num>::ATan2( fVec0.Z(), fVec1.Y() );
        auto atanDegrees = Math<real_Num>::RadToDeg( atanValue );
        BOOST_CHECK( Math<real_Num>::equals( atanDegrees, 45.0 ) == true );

        fVec0 = Vector3<real_Num>( 0, 0, 1 );
        fVec1 = Vector3<real_Num>( 0, 0, 1 );
        atanValue = Math<real_Num>::ATan2( fVec0.Z(), fVec1.Y() );
        atanDegrees = Math<real_Num>::RadToDeg( atanValue );
        BOOST_CHECK( Math<real_Num>::equals( atanDegrees, 90.0 ) == true );

        for( size_t i = 0; i < 1000; i++ )
        {
            auto fVecValue =
                Vector3<real_Num>( Math<real_Num>::RangedRandom( static_cast<real_Num>( -1.0 ),
                                                                 static_cast<real_Num>( 1.0 ) ),
                                   Math<real_Num>::RangedRandom( static_cast<real_Num>( -1.0 ),
                                                                 static_cast<real_Num>( 1.0 ) ),
                                   Math<real_Num>::RangedRandom( static_cast<real_Num>( -1.0 ),
                                                                 static_cast<real_Num>( 1.0 ) ) );

            auto fY = Vector3<real_Num>::up().dotProduct( fVecValue );
            auto fX = Vector3<real_Num>::forward().dotProduct( fVecValue );

            auto atan2Value = Math<real_Num>::ATan2( fY, fX );
            auto atan2Degrees = Math<real_Num>::RadToDeg( atan2Value );

            auto stdAtan2Value = std::atan2( fY, fX );
            auto stdAtan2Degrees = Math<real_Num>::RadToDeg( stdAtan2Value );

            auto atan1Value = Math<real_Num>::Atan( fY / fX );
            auto atan1Degrees = Math<real_Num>::RadToDeg( atan1Value );

            // check the unit circle quadrants
            if( fX == 0.0 )
            {
                if( fY > 0.0 )
                {
                    // todo
                }
                else
                {
                    // todo
                }
            }
            else
            {
                if( fX > 0.0 )
                {
                    // do nothing
                }
                else
                {
                    if( fY > 0.0 )
                    {
                        atan1Value = Math<real_Num>::pi() + Math<real_Num>::Atan( fY / fX );
                        atan1Degrees = Math<real_Num>::RadToDeg( atan1Value );
                    }
                    else
                    {
                        atan1Value = -Math<real_Num>::pi() + Math<real_Num>::Atan( fY / fX );
                        atan1Degrees = Math<real_Num>::RadToDeg( atan1Value );
                    }
                }
            }

            // WP_ASSERT(Math<real_Num>::equals(atan2Value, atan1Value) == true);

            // BOOST_CHECK(Math<real_Num>::equals(stdAtan2Value, atan2Value) == true);
            // BOOST_CHECK(Math<real_Num>::equals(atan2Value, atan1Value) == true);
            // BOOST_CHECK(Math<real_Num>::equals(atan2Degrees, atan1Degrees) == true);
        }

        real_Num degrees = 651651.0f;
        degrees = Math<real_Num>::wrapDegrees( degrees, static_cast<real_Num>( 180.0 ) );
        BOOST_CHECK( degrees > static_cast<real_Num>( -180.0 ) &&
                     degrees < static_cast<real_Num>( 180.0 ) );

        degrees = -854651651.0f;
        degrees = Math<real_Num>::wrapDegrees( degrees, static_cast<real_Num>( 180.0 ) );
        BOOST_CHECK( degrees > static_cast<real_Num>( -180.0 ) &&
                     degrees < static_cast<real_Num>( 180.0 ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( math_abs )
{
    BOOST_CHECK( Math<float>::equals( Math<float>::Abs( 3.5f ), 3.5f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::Abs( -3.5f ), 3.5f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::Abs( 0.0f ), 0.0f ) );

    BOOST_CHECK( Math<double>::equals( Math<double>::Abs( -1.0 ), 1.0 ) );
}

BOOST_AUTO_TEST_CASE( math_sqrt )
{
    BOOST_CHECK( Math<float>::equals( Math<float>::Sqrt( 9.0f ), 3.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::Sqrt( 4.0f ), 2.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::Sqrt( 0.0f ), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::Sqrt( 2.0f ), std::sqrt( 2.0f ) ) );
}

BOOST_AUTO_TEST_CASE( math_sqr )
{
    BOOST_CHECK( Math<float>::equals( Math<float>::Sqr( 3.0f ), 9.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::Sqr( -4.0f ), 16.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::Sqr( 0.0f ), 0.0f ) );
}

BOOST_AUTO_TEST_CASE( math_clamp )
{
    // Within range: value unchanged
    BOOST_CHECK( Math<float>::equals( Math<float>::clamp( 5.0f, 0.0f, 10.0f ), 5.0f ) );
    // Below minimum: clamps to min
    BOOST_CHECK( Math<float>::equals( Math<float>::clamp( -1.0f, 0.0f, 10.0f ), 0.0f ) );
    // Above maximum: clamps to max
    BOOST_CHECK( Math<float>::equals( Math<float>::clamp( 11.0f, 0.0f, 10.0f ), 10.0f ) );
    // Exactly at bounds
    BOOST_CHECK( Math<float>::equals( Math<float>::clamp( 0.0f, 0.0f, 10.0f ), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::clamp( 10.0f, 0.0f, 10.0f ), 10.0f ) );
}

BOOST_AUTO_TEST_CASE( math_clamp01 )
{
    BOOST_CHECK( Math<float>::equals( Math<float>::clamp01( 0.5f ), 0.5f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::clamp01( -0.5f ), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::clamp01( 1.5f ), 1.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::clamp01( 0.0f ), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::clamp01( 1.0f ), 1.0f ) );
}

BOOST_AUTO_TEST_CASE( math_lerp )
{
    // Midpoint
    auto mid = Math<float>::lerp( 0.0f, 10.0f, 0.5f );
    BOOST_CHECK( Math<float>::equals( mid, 5.0f ) );
    // At t=0 returns 'a'
    auto atZero = Math<float>::lerp( 0.0f, 10.0f, 0.0f );
    BOOST_CHECK( Math<float>::equals( atZero, 0.0f ) );
    // At t=1 returns 'b'
    auto atOne = Math<float>::lerp( 0.0f, 10.0f, 1.0f );
    BOOST_CHECK( Math<float>::equals( atOne, 10.0f ) );
    // Negative range
    auto neg = Math<float>::lerp( -10.0f, 0.0f, 0.5f );
    BOOST_CHECK( Math<float>::equals( neg, -5.0f ) );
}

BOOST_AUTO_TEST_CASE( math_inverse_lerp )
{
    // Midpoint -> 0.5
    auto t = Math<float>::inverseLerp( 0.0f, 10.0f, 5.0f );
    BOOST_CHECK( Math<float>::equals( t, 0.5f ) );
    // At 'a' -> 0
    BOOST_CHECK( Math<float>::equals( Math<float>::inverseLerp( 0.0f, 10.0f, 0.0f ), 0.0f ) );
    // At 'b' -> 1
    BOOST_CHECK( Math<float>::equals( Math<float>::inverseLerp( 0.0f, 10.0f, 10.0f ), 1.0f ) );
}

BOOST_AUTO_TEST_CASE( math_sin_cos )
{
    // sin(0) == 0, cos(0) == 1
    BOOST_CHECK( Math<float>::equals( Math<float>::Sin( 0.0f ), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::Cos( 0.0f ), 1.0f ) );
    // sin(pi/2) == 1, cos(pi/2) == 0
    BOOST_CHECK( Math<float>::equals( Math<float>::Sin( MathF::half_pi() ), 1.0f ) );
    BOOST_CHECK_SMALL( Math<float>::Cos( MathF::half_pi() ), 1.0e-4f );
    // sin^2 + cos^2 == 1 (Pythagorean identity) for an arbitrary angle
    float angle = MathF::DegToRad( 37.0f );
    float s = Math<float>::Sin( angle );
    float c = Math<float>::Cos( angle );
    BOOST_CHECK( Math<float>::equals( s * s + c * c, 1.0f ) );
}

BOOST_AUTO_TEST_CASE( math_sign )
{
    BOOST_CHECK( Math<float>::Sign( 5.0f ) == 1 );
    BOOST_CHECK( Math<float>::Sign( -5.0f ) == -1 );
    BOOST_CHECK( Math<float>::Sign( 0.0f ) == 0 );
}

BOOST_AUTO_TEST_CASE( math_floor )
{
    BOOST_CHECK( Math<float>::equals( Math<float>::Floor( 3.7f ), 3.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::Floor( -3.2f ), -4.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::Floor( 5.0f ), 5.0f ) );
}

BOOST_AUTO_TEST_CASE( math_isnan_isfinite )
{
    BOOST_CHECK( Math<float>::isNaN( std::numeric_limits<float>::quiet_NaN() ) );
    BOOST_CHECK( !Math<float>::isNaN( 1.0f ) );
    BOOST_CHECK( Math<float>::isFinite( 1.0f ) );
    BOOST_CHECK( !Math<float>::isFinite( std::numeric_limits<float>::infinity() ) );
    BOOST_CHECK( !Math<float>::isFinite( -std::numeric_limits<float>::infinity() ) );
}

BOOST_AUTO_TEST_CASE( math_normalize_value )
{
    // 5 in [0, 10] -> 0.5
    BOOST_CHECK( Math<float>::equals( Math<float>::normalize( 5.0f, 0.0f, 10.0f ), 0.5f ) );
    // 0 in [0, 10] -> 0
    BOOST_CHECK( Math<float>::equals( Math<float>::normalize( 0.0f, 0.0f, 10.0f ), 0.0f ) );
    // 10 in [0, 10] -> 1
    BOOST_CHECK( Math<float>::equals( Math<float>::normalize( 10.0f, 0.0f, 10.0f ), 1.0f ) );
}

BOOST_AUTO_TEST_CASE( math_min_max )
{
    BOOST_CHECK( Math<float>::equals( Math<float>::min( 3.0f, 7.0f ), 3.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::max( 3.0f, 7.0f ), 7.0f ) );
    // Equal values
    BOOST_CHECK( Math<float>::equals( Math<float>::min( 5.0f, 5.0f ), 5.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::max( 5.0f, 5.0f ), 5.0f ) );
}

BOOST_AUTO_TEST_CASE( math_degrad_roundtrip )
{
    float deg = 90.0f;
    float rad = MathF::DegToRad( deg );
    float backToDeg = MathF::RadToDeg( rad );
    BOOST_CHECK( Math<float>::equals( backToDeg, deg ) );

    float deg2 = 270.0f;
    BOOST_CHECK( Math<float>::equals( MathF::RadToDeg( MathF::DegToRad( deg2 ) ), deg2 ) );
}

BOOST_AUTO_TEST_CASE( math_wrap_radians )
{
    // A value already within [-pi, pi] stays unchanged
    float inRange = 1.0f;
    float wrapped = Math<float>::wrapRadians( inRange );
    BOOST_CHECK( wrapped >= -MathF::pi() && wrapped <= MathF::pi() );

    // A value outside the range gets wrapped
    float outRange = MathF::pi() * 3.0f;
    float wrappedOut = Math<float>::wrapRadians( outRange );
    BOOST_CHECK( wrappedOut >= -MathF::pi() && wrappedOut <= MathF::pi() );
}

BOOST_AUTO_TEST_CASE( math_reciprocal )
{
    BOOST_CHECK( Math<float>::equals( Math<float>::reciprocal( 2.0f ), 0.5f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::reciprocal( 4.0f ), 0.25f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::reciprocal( 1.0f ), 1.0f ) );
}

BOOST_AUTO_TEST_CASE( math_acos_asin_edge_cases )
{
    // ACos(1) == 0, ACos(-1) == pi
    BOOST_CHECK( Math<float>::equals( Math<float>::ACos( 1.0f ), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::ACos( -1.0f ), MathF::pi() ) );
    // ASin(0) == 0, ASin(1) == pi/2
    BOOST_CHECK( Math<float>::equals( Math<float>::ASin( 0.0f ), 0.0f ) );
    BOOST_CHECK( Math<float>::equals( Math<float>::ASin( 1.0f ), MathF::half_pi() ) );
    // Round-trip: ACos(Cos(x)) == x for x in [0, pi]
    float angle = MathF::DegToRad( 60.0f );
    BOOST_CHECK( Math<float>::equals( Math<float>::ACos( Math<float>::Cos( angle ) ), angle ) );
}

namespace
{
    struct DebugLineRecord
    {
        hash_type id = 0;
        Vector3<real_Num> start;
        Vector3<real_Num> end;
        u32 colour = 0;
    };

    class RecordingDebug final : public render::Debug
    {
    public:
        SmartPtr<render::IDebugLine> drawLine( hash_type id, const Vector3<real_Num> &start,
                                               const Vector3<real_Num> &end, u32 colour ) override
        {
            lines.push_back( { id, start, end, colour } );
            return nullptr;
        }

        std::vector<DebugLineRecord> lines;
    };
}  // namespace

BOOST_AUTO_TEST_CASE( debug_draw_compound_primitives )
{
    RecordingDebug debug;
    constexpr auto primitiveId = static_cast<hash_type>( 42 );
    constexpr auto colour = 0x12345678u;

    const AABB3<real_Num> aabb( Vector3<real_Num>( -1, -2, -3 ), Vector3<real_Num>( 1, 2, 3 ) );
    debug.drawAABB( primitiveId, aabb, colour );

    BOOST_REQUIRE_EQUAL( debug.lines.size(), 12u );
    std::set<hash_type> ids;
    for( const auto &line : debug.lines )
    {
        ids.insert( line.id );
        BOOST_CHECK_EQUAL( line.colour, colour );

        const auto delta = line.end - line.start;
        const auto changingAxes = static_cast<u32>( !Math<real_Num>::equals( delta.X(), 0 ) ) +
                                  static_cast<u32>( !Math<real_Num>::equals( delta.Y(), 0 ) ) +
                                  static_cast<u32>( !Math<real_Num>::equals( delta.Z(), 0 ) );
        BOOST_CHECK_EQUAL( changingAxes, 1u );
    }
    BOOST_CHECK_EQUAL( ids.size(), 12u );

    debug.drawOBB( primitiveId, OBB3<real_Num>( aabb ), colour );
    BOOST_REQUIRE_EQUAL( debug.lines.size(), 24u );
    for( const auto &line : debug.lines )
    {
        ids.insert( line.id );
    }
    BOOST_CHECK_EQUAL( ids.size(), 24u );

    Transform3<real_Num> mirroredTransform;
    mirroredTransform.setScale( Vector3<real_Num>( -1, 2, -3 ) );
    const OBB3<real_Num> mirroredBox( aabb, mirroredTransform );
    BOOST_CHECK( mirroredBox.isValid() );
    BOOST_CHECK( mirroredBox.getHalfExtents() == Vector3<real_Num>( 1, 4, 9 ) );

    debug.drawArrow( primitiveId, Vector3<real_Num>::zero(), Vector3<real_Num>( 0, 0, 10 ), colour );
    BOOST_REQUIRE_EQUAL( debug.lines.size(), 27u );
    BOOST_CHECK( debug.lines[24].start == Vector3<real_Num>::zero() );
    BOOST_CHECK( debug.lines[24].end == Vector3<real_Num>( 0, 0, 10 ) );

    debug.drawArrow( primitiveId, Vector3<real_Num>::zero(), Vector3<real_Num>::zero(), colour );
    BOOST_CHECK_EQUAL( debug.lines.size(), 27u );
}
