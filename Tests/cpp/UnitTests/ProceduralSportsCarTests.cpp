#include <WPProcedural/WPVehicleGenerator.hpp>
#include <WPProcedural/WPVehicleGeometry.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

using namespace workphone;
using namespace workphone::procedural;

BOOST_AUTO_TEST_SUITE( procedural_sports_car )

BOOST_AUTO_TEST_CASE( refined_body_keeps_axles_bounds_tangent_frames_and_lod_reduction )
{
    for( bool details : { false, true } )
    {
        VehicleGenerationConfig request;
        request.physicsPreset = VehiclePhysicsPreset::RoadSport;
        request.appearance.quality = VehicleAppearanceQuality::Preview;
        request.geometry.includeFineDetails = details;
        const auto car = WPVehicleGenerator::generate( request );
        for( const auto &issue : car.issues )
        {
            BOOST_CHECK_MESSAGE( !issue.error, issue.component + ": " + issue.message );
        }
        BOOST_REQUIRE( car.isValid() );
        BOOST_CHECK_EQUAL( car.contentHash, WPVehicleGenerator::generate( request ).contentHash );
        BOOST_REQUIRE_EQUAL( car.geometry.lods.size(), 3u );
        BOOST_CHECK_SMALL( car.geometry.frontAxle.z - float( car.physics.wheels[0].hubPosition.z ),
                           .001f );
        BOOST_CHECK_SMALL( car.geometry.rearAxle.z - float( car.physics.wheels[2].hubPosition.z ),
                           .001f );
        std::size_t previous = std::numeric_limits<std::size_t>::max();
        for( const auto &lod : car.geometry.lods )
        {
            std::size_t triangles = 0;
            bool cabin = false, hardware = false, panelDetails = false, interior = false,
                 calipers = false;
            bool smoothBody = false, crownedBody = false, leftLamp = false, rightLamp = false;
            for( const auto &section : lod.sections )
            {
                triangles += section.indices.size() / 3;
                cabin |= section.name == "sports_cabin_glass";
                hardware |= section.name == "sports_exhaust_and_hardware";
                panelDetails |= section.name == "sports_panel_details";
                interior |= section.name == "sports_cabin_interior";
                calipers |= section.material == VehicleMaterial::Caliper;
                std::vector<unsigned> references( section.vertices.size(), 0 );
                for( auto index : section.indices )
                {
                    BOOST_REQUIRE_LT( index, section.vertices.size() );
                    ++references[index];
                }
                for( std::size_t i = 0; i < section.vertices.size(); ++i )
                {
                    const auto &v = section.vertices[i];
                    BOOST_REQUIRE( std::isfinite( v.position.x ) && std::isfinite( v.position.y ) &&
                                   std::isfinite( v.position.z ) && std::isfinite( v.uv.x ) &&
                                   std::isfinite( v.uv.y ) );
                    BOOST_CHECK_SMALL( v.normal.length() - 1.f, .002f );
                    BOOST_CHECK_SMALL( v.tangent.length() - 1.f, .002f );
                    BOOST_CHECK_SMALL( v.normal.dotProduct( v.tangent ), .002f );
                    for( unsigned axis = 0; axis < 3; ++axis )
                    {
                        BOOST_CHECK_GE( v.position[axis], car.geometry.boundsMin[axis] );
                        BOOST_CHECK_LE( v.position[axis], car.geometry.boundsMax[axis] );
                    }
                    if( section.name == "sports_body_paint" )
                    {
                        smoothBody |= references[i] >= 5;
                        crownedBody |= v.normal.y > .5f && std::abs( v.normal.x ) > .08f;
                    }
                    if( section.material == VehicleMaterial::Headlight )
                    {
                        BOOST_CHECK_GT( v.normal.y, .15f );
                        leftLamp |= v.position.x < 0;
                        rightLamp |= v.position.x > 0;
                    }
                }
            }
            BOOST_CHECK( cabin && smoothBody && crownedBody && leftLamp && rightLamp );
            BOOST_CHECK_EQUAL( hardware, details && lod.level < 2 );
            BOOST_CHECK_EQUAL( panelDetails, details && lod.level < 2 );
            BOOST_CHECK_EQUAL( interior, details && lod.level < 2 );
            BOOST_CHECK_EQUAL( calipers, details && lod.level < 2 );
            BOOST_CHECK_LT( triangles, previous );
            previous = triangles;
        }
    }
}

BOOST_AUTO_TEST_CASE( body_scales_and_leaves_the_wheel_openings_clear )
{
    for( float radius : { .25f, .34f, .52f } )
    {
        for( float length : { 3.8f, 5.4f } )
        {
            // Custom dimensions belong to the standalone geometry API. The
            // integrated preset deliberately rejects unscaled physics metadata.
            VehicleGeometryConfig request;
            request.bodyStyle = VehicleBodyStyle::RoadSport;
            request.wheelbase = 2.64f;
            request.bodyWidth = 1.90f;
            request.bodyLength = length;
            request.tyreRadius = radius;
            request.generateThreeLODs = false;
            const auto car = WPVehicleGeometry::generate( request );
            BOOST_REQUIRE( car.hasGeometry() );
            // At the outer panel, referenced body points above each axle must
            // clear the tyre crown, even when users change the tyre radius.
            for( const auto &section : car.lods.front().sections )
            {
                if( section.name == "sports_body_paint" )
                {
                    for( const auto &v : section.vertices )
                    {
                        for( float axle : { car.frontAxle.z, car.rearAxle.z } )
                        {
                            if( std::abs( v.position.z - axle ) < .001f &&
                                std::abs( v.position.x ) > .86f )
                            {
                                BOOST_CHECK_GE( v.position.y, radius * 2 + .045f );
                            }
                        }
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
