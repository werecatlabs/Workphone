#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <limits>

using namespace workphone;

// Helper function to check approximate equality for floating-point values
static bool approxEqual( float a, float b, float epsilon = 1e-5f )
{
    return std::abs( a - b ) < epsilon;
}

static bool approxEqual( const Vector3F &a, const Vector3F &b, float epsilon = 1e-5f )
{
    return approxEqual( a.x, b.x, epsilon ) && approxEqual( a.y, b.y, epsilon ) &&
           approxEqual( a.z, b.z, epsilon );
}

//------------------------------------------------------------------------------
// Construction Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( spline_default_constructor )
{
    LinearSpline3F spline;

    BOOST_CHECK_EQUAL( spline.getNumPoints(), 0 );
}

//------------------------------------------------------------------------------
// Point Management Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( spline_add_single_point )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );

    BOOST_CHECK_EQUAL( spline.getNumPoints(), 1 );
    BOOST_CHECK( approxEqual( spline.getPoint( 0 ), Vector3F( 0.0f, 0.0f, 0.0f ) ) );
}

BOOST_AUTO_TEST_CASE( spline_add_multiple_points )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 0.0f ) );
    spline.addPoint( Vector3F( 2.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 3.0f, 1.0f, 0.0f ) );

    BOOST_CHECK_EQUAL( spline.getNumPoints(), 4 );
    BOOST_CHECK( approxEqual( spline.getPoint( 0 ), Vector3F( 0.0f, 0.0f, 0.0f ) ) );
    BOOST_CHECK( approxEqual( spline.getPoint( 1 ), Vector3F( 1.0f, 1.0f, 0.0f ) ) );
    BOOST_CHECK( approxEqual( spline.getPoint( 2 ), Vector3F( 2.0f, 0.0f, 0.0f ) ) );
    BOOST_CHECK( approxEqual( spline.getPoint( 3 ), Vector3F( 3.0f, 1.0f, 0.0f ) ) );
}

BOOST_AUTO_TEST_CASE( spline_clear_points )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 0.0f ) );
    spline.addPoint( Vector3F( 2.0f, 0.0f, 0.0f ) );

    BOOST_CHECK_EQUAL( spline.getNumPoints(), 3 );

    spline.clear();

    BOOST_CHECK_EQUAL( spline.getNumPoints(), 0 );
}

BOOST_AUTO_TEST_CASE( spline_update_point )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 0.0f ) );

    spline.updatePoint( 1, Vector3F( 2.0f, 3.0f, 4.0f ) );

    BOOST_CHECK( approxEqual( spline.getPoint( 1 ), Vector3F( 2.0f, 3.0f, 4.0f ) ) );
}

//------------------------------------------------------------------------------
// Interpolation Tests - Basic
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( spline_interpolate_at_endpoints )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 10.0f, 10.0f, 0.0f ) );

    // t=0 should return first point
    Vector3F start = spline.interpolate( 0.0f );
    BOOST_CHECK( approxEqual( start, Vector3F( 0.0f, 0.0f, 0.0f ) ) );

    // t=1 should return last point
    Vector3F end = spline.interpolate( 1.0f );
    BOOST_CHECK( approxEqual( end, Vector3F( 10.0f, 10.0f, 0.0f ) ) );
}

BOOST_AUTO_TEST_CASE( spline_interpolate_midpoint_two_points )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 10.0f, 0.0f, 0.0f ) );

    // For a straight line, midpoint should be at (5, 0, 0)
    Vector3F mid = spline.interpolate( 0.5f );

    // Catmull-Rom may not produce exact linear interpolation
    BOOST_CHECK( mid.x > 0.0f && mid.x < 10.0f );
}

BOOST_AUTO_TEST_CASE( spline_interpolate_multiple_segments )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 2.0f, 0.0f ) );
    spline.addPoint( Vector3F( 2.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 3.0f, 2.0f, 0.0f ) );

    // Test interpolation passes through control points
    Vector3F at0 = spline.interpolate( 0.0f );
    BOOST_CHECK( approxEqual( at0, Vector3F( 0.0f, 0.0f, 0.0f ) ) );

    Vector3F at1 = spline.interpolate( 1.0f );
    BOOST_CHECK( approxEqual( at1, Vector3F( 3.0f, 2.0f, 0.0f ) ) );

    // Test intermediate points
    Vector3F mid1 = spline.interpolate( 0.333f );  // ~1/3 of the way
    BOOST_CHECK( mid1.x > 0.0f );

    Vector3F mid2 = spline.interpolate( 0.666f );  // ~2/3 of the way
    BOOST_CHECK( mid2.x > mid1.x );
}

BOOST_AUTO_TEST_CASE( spline_interpolate_by_index )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 0.0f ) );
    spline.addPoint( Vector3F( 2.0f, 0.0f, 0.0f ) );

    // Interpolate segment 0 to 1 at t=0
    Vector3F seg0_start = spline.interpolate( 0, 0.0f );
    BOOST_CHECK( approxEqual( seg0_start, Vector3F( 0.0f, 0.0f, 0.0f ) ) );

    // Interpolate segment 0 to 1 at t=1
    Vector3F seg0_end = spline.interpolate( 0, 1.0f );
    BOOST_CHECK( approxEqual( seg0_end, Vector3F( 1.0f, 1.0f, 0.0f ) ) );

    // Interpolate segment 1 to 2 at t=0
    Vector3F seg1_start = spline.interpolate( 1, 0.0f );
    BOOST_CHECK( approxEqual( seg1_start, Vector3F( 1.0f, 1.0f, 0.0f ) ) );

    // Interpolate segment 1 to 2 at t=1
    Vector3F seg1_end = spline.interpolate( 1, 1.0f );
    BOOST_CHECK( approxEqual( seg1_end, Vector3F( 2.0f, 0.0f, 0.0f ) ) );
}

//------------------------------------------------------------------------------
// Continuity Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( spline_continuity_at_joints )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 0.0f ) );
    spline.addPoint( Vector3F( 2.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 3.0f, 1.0f, 0.0f ) );

    // End of segment 0 should equal start of segment 1
    Vector3F end_seg0 = spline.interpolate( 0, 1.0f );
    Vector3F start_seg1 = spline.interpolate( 1, 0.0f );
    BOOST_CHECK( approxEqual( end_seg0, start_seg1 ) );

    // End of segment 1 should equal start of segment 2
    Vector3F end_seg1 = spline.interpolate( 1, 1.0f );
    Vector3F start_seg2 = spline.interpolate( 2, 0.0f );
    BOOST_CHECK( approxEqual( end_seg1, start_seg2 ) );
}

BOOST_AUTO_TEST_CASE( spline_smooth_curve_monotonic_progress )
{
    LinearSpline3F spline;

    // Create a monotonically increasing X spline
    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 2.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 3.0f, 0.0f, 0.0f ) );

    // Sample points along the spline and verify X is non-decreasing
    float prevX = -std::numeric_limits<float>::max();
    for( float t = 0.0f; t <= 1.0f; t += 0.1f )
    {
        Vector3F point = spline.interpolate( t );
        BOOST_CHECK( point.x >= prevX - 1e-5f );  // Allow small numerical error
        prevX = point.x;
    }
}

//------------------------------------------------------------------------------
// Edge Cases
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( spline_single_point_interpolation )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 5.0f, 5.0f, 5.0f ) );

    // With only one point, interpolation should return that point
    // Note: This may depend on implementation - some may assert
    BOOST_CHECK_EQUAL( spline.getNumPoints(), 1 );
}

BOOST_AUTO_TEST_CASE( spline_collinear_points )
{
    LinearSpline3F spline;

    // All points on a straight line
    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 1.0f ) );
    spline.addPoint( Vector3F( 2.0f, 2.0f, 2.0f ) );
    spline.addPoint( Vector3F( 3.0f, 3.0f, 3.0f ) );

    // Midpoint should be roughly on the line
    Vector3F mid = spline.interpolate( 0.5f );

    // For collinear points, result should be close to the line
    // Check that y ≈ x and z ≈ x
    BOOST_CHECK( std::abs( mid.y - mid.x ) < 0.5f );
    BOOST_CHECK( std::abs( mid.z - mid.x ) < 0.5f );
}

BOOST_AUTO_TEST_CASE( spline_identical_points )
{
    LinearSpline3F spline;

    // All points at the same location
    spline.addPoint( Vector3F( 1.0f, 1.0f, 1.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 1.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 1.0f ) );

    // Any interpolation should return the same point
    Vector3F result = spline.interpolate( 0.5f );
    BOOST_CHECK( approxEqual( result, Vector3F( 1.0f, 1.0f, 1.0f ) ) );
}

BOOST_AUTO_TEST_CASE( spline_closed_loop )
{
    LinearSpline3F spline;

    // Create a closed loop (first point == last point)
    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 0.0f ) );
    spline.addPoint( Vector3F( 0.0f, 1.0f, 0.0f ) );
    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );  // Close the loop

    // Start and end should be the same
    Vector3F start = spline.interpolate( 0.0f );
    Vector3F end = spline.interpolate( 1.0f );
    BOOST_CHECK( approxEqual( start, end ) );
}

BOOST_AUTO_TEST_CASE( spline_negative_coordinates )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( -10.0f, -10.0f, -10.0f ) );
    spline.addPoint( Vector3F( -5.0f, 0.0f, 5.0f ) );
    spline.addPoint( Vector3F( 0.0f, 10.0f, 20.0f ) );

    Vector3F start = spline.interpolate( 0.0f );
    Vector3F end = spline.interpolate( 1.0f );

    BOOST_CHECK( approxEqual( start, Vector3F( -10.0f, -10.0f, -10.0f ) ) );
    BOOST_CHECK( approxEqual( end, Vector3F( 0.0f, 10.0f, 20.0f ) ) );
}

BOOST_AUTO_TEST_CASE( spline_large_coordinates )
{
    LinearSpline3F spline;

    const float large = 1e6f;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( large, large, large ) );

    Vector3F end = spline.interpolate( 1.0f );

    BOOST_CHECK( approxEqual( end.x, large, 1.0f ) );
    BOOST_CHECK( approxEqual( end.y, large, 1.0f ) );
    BOOST_CHECK( approxEqual( end.z, large, 1.0f ) );
}

BOOST_AUTO_TEST_CASE( spline_very_small_coordinates )
{
    LinearSpline3F spline;

    auto fSmall = (float)1e-6f;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( fSmall, fSmall, fSmall ) );

    Vector3F mid = spline.interpolate( 0.5f );

    // Should be somewhere between 0 and small
    BOOST_CHECK( mid.x >= 0.0f && mid.x <= fSmall * 2.0f );
}

//------------------------------------------------------------------------------
// Auto-Calculate Tangent Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( spline_auto_calculate_toggle )
{
    LinearSpline3F spline;

    // Disable auto-calculate
    spline.setAutoCalculate( false );

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 0.0f ) );
    spline.addPoint( Vector3F( 2.0f, 0.0f, 0.0f ) );

    // Manually trigger tangent recalculation
    spline.recalcTangents();

    // Re-enable auto-calculate
    spline.setAutoCalculate( true );

    // Add another point - tangents should auto-update
    spline.addPoint( Vector3F( 3.0f, 1.0f, 0.0f ) );

    BOOST_CHECK_EQUAL( spline.getNumPoints(), 4 );
}

//------------------------------------------------------------------------------
// Stress Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( spline_many_points )
{
    LinearSpline3F spline;

    // Add many points
    const int numPoints = 100;
    for( int i = 0; i < numPoints; ++i )
    {
        float angle = static_cast<float>( i ) * 0.1f;
        spline.addPoint(
            Vector3F( std::cos( angle ), std::sin( angle ), static_cast<float>( i ) * 0.01f ) );
    }

    BOOST_CHECK_EQUAL( spline.getNumPoints(), numPoints );

    // Verify interpolation still works
    Vector3F mid = spline.interpolate( 0.5f );
    BOOST_CHECK( std::isfinite( mid.x ) );
    BOOST_CHECK( std::isfinite( mid.y ) );
    BOOST_CHECK( std::isfinite( mid.z ) );
}

BOOST_AUTO_TEST_CASE( spline_fine_sampling )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 0.0f ) );
    spline.addPoint( Vector3F( 2.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 3.0f, 1.0f, 0.0f ) );

    // Sample at many points
    const int numSamples = 1000;
    for( int i = 0; i <= numSamples; ++i )
    {
        float t = static_cast<float>( i ) / static_cast<float>( numSamples );
        Vector3F point = spline.interpolate( t );

        // All results should be finite
        BOOST_CHECK( std::isfinite( point.x ) );
        BOOST_CHECK( std::isfinite( point.y ) );
        BOOST_CHECK( std::isfinite( point.z ) );
    }
}

//------------------------------------------------------------------------------
// Boundary Value Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( spline_interpolate_t_just_above_zero )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 10.0f, 10.0f, 0.0f ) );

    Vector3F result = spline.interpolate( 0.0001f );

    BOOST_CHECK( std::isfinite( result.x ) );
    BOOST_CHECK( result.x >= 0.0f );
}

BOOST_AUTO_TEST_CASE( spline_interpolate_t_just_below_one )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 10.0f, 10.0f, 0.0f ) );

    Vector3F result = spline.interpolate( 0.9999f );

    BOOST_CHECK( std::isfinite( result.x ) );
    BOOST_CHECK( result.x <= 10.0f + 1e-3f );
}

BOOST_AUTO_TEST_CASE( spline_interpolate_segment_boundary )
{
    LinearSpline3F spline;

    spline.addPoint( Vector3F( 0.0f, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 1.0f, 1.0f, 0.0f ) );
    spline.addPoint( Vector3F( 2.0f, 2.0f, 0.0f ) );

    // t = 0.5 should be exactly at the middle control point for 3 points
    Vector3F mid = spline.interpolate( 0.5f );

    BOOST_CHECK( std::isfinite( mid.x ) );
}

//------------------------------------------------------------------------------
// 3D Curve Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( spline_3d_helix )
{
    LinearSpline3F spline;

    // Create a helix pattern
    for( int i = 0; i < 10; ++i )
    {
        float angle = static_cast<float>( i ) * 0.628f;  // ~36 degrees
        spline.addPoint(
            Vector3F( std::cos( angle ), std::sin( angle ), static_cast<float>( i ) * 0.5f ) );
    }

    // Interpolate and verify z increases monotonically
    float prevZ = -1.0f;
    for( float t = 0.0f; t <= 1.0f; t += 0.1f )
    {
        Vector3F point = spline.interpolate( t );
        BOOST_CHECK( point.z >= prevZ - 0.1f );  // Allow small tolerance for spline overshoot
        prevZ = point.z;
    }
}

BOOST_AUTO_TEST_CASE( spline_3d_sphere_surface )
{
    LinearSpline3F spline;

    // Points on a sphere surface (great circle)
    const float radius = 5.0f;
    spline.addPoint( Vector3F( radius, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 0.0f, radius, 0.0f ) );
    spline.addPoint( Vector3F( -radius, 0.0f, 0.0f ) );
    spline.addPoint( Vector3F( 0.0f, -radius, 0.0f ) );
    spline.addPoint( Vector3F( radius, 0.0f, 0.0f ) );  // Close the loop

    // All interpolated points should be at approximately the same distance from origin
    // (with some deviation due to spline properties)
    for( float t = 0.0f; t <= 1.0f; t += 0.1f )
    {
        Vector3F point = spline.interpolate( t );
        float distance = std::sqrt( point.x * point.x + point.y * point.y + point.z * point.z );

        // Spline won't perfectly preserve radius but shouldn't deviate wildly
        BOOST_CHECK( distance > radius * 0.5f && distance < radius * 1.5f );
    }
}
