#include <WPProcedural/WPVehicleGenerator.hpp>
#include <WPProcedural/WPVehicleGeometry.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <limits>

using namespace workphone;
using namespace workphone::procedural;

BOOST_AUTO_TEST_SUITE( procedural_gt_car )

BOOST_AUTO_TEST_CASE( race_silhouette_survives_lods_and_optional_detail_switch )
{
    for( bool details : { false, true } )
    {
        VehicleGenerationConfig request;
        request.physicsPreset = VehiclePhysicsPreset::GT;
        request.appearance.quality = VehicleAppearanceQuality::Preview;
        request.geometry.includeFineDetails = details;
        const auto car = WPVehicleGenerator::generate( request );
        for( const auto &issue : car.issues )
        {
            BOOST_CHECK_MESSAGE( !issue.error, issue.component + ": " + issue.message );
        }
        BOOST_REQUIRE( car.isValid() );
        BOOST_CHECK_EQUAL( car.contentHash, WPVehicleGenerator::generate( request ).contentHash );
        BOOST_CHECK_CLOSE( car.physics.wheelbaseM, 2.78, .01 );
        BOOST_CHECK_SMALL( car.geometry.frontAxle.z - float( car.physics.wheels[0].hubPosition.z ),
                           .001f );
        BOOST_CHECK_SMALL( car.geometry.rearAxle.z - float( car.physics.wheels[2].hubPosition.z ),
                           .001f );
        BOOST_REQUIRE_EQUAL( car.geometry.lods.size(), 3u );
        std::size_t previous = std::numeric_limits<std::size_t>::max();
        for( const auto &lod : car.geometry.lods )
        {
            bool wing = false, splitter = false, diffuser = false, roof = false, rain = false;
            bool cabin = false, vents = false, cage = false, tow = false, smoothBody = false;
            bool leftLight = false, rightLight = false;
            std::size_t triangles = 0;
            for( const auto &section : lod.sections )
            {
                wing |= section.name == "gt_rear_wing" && section.hasGeometry();
                splitter |= section.name == "gt_front_splitter" && section.hasGeometry();
                diffuser |= section.name == "gt_rear_diffuser" && section.hasGeometry();
                roof |= section.name == "gt_carbon_roof" && section.hasGeometry();
                rain |= section.name == "gt_rain_light" && section.hasGeometry();
                cabin |= section.name == "gt_cabin_glass" && section.hasGeometry();
                vents |= section.name == "gt_cooling_louvres";
                cage |= section.name == "gt_roll_cage";
                tow |= section.name == "gt_tow_loops";
                triangles += section.indices.size() / 3;
                std::vector<unsigned> references( section.vertices.size(), 0 );
                for( auto index : section.indices )
                {
                    BOOST_REQUIRE_LT( index, section.vertices.size() );
                    ++references[index];
                }
                for( std::size_t i = 0; i < section.vertices.size(); ++i )
                {
                    const auto &v = section.vertices[i];
                    for( unsigned axis = 0; axis < 3; ++axis )
                    {
                        BOOST_REQUIRE( std::isfinite( v.position[axis] ) );
                        BOOST_CHECK_GE( v.position[axis], car.geometry.boundsMin[axis] );
                        BOOST_CHECK_LE( v.position[axis], car.geometry.boundsMax[axis] );
                    }
                    BOOST_CHECK_SMALL( v.normal.length() - 1.f, .002f );
                    BOOST_CHECK_SMALL( v.tangent.length() - 1.f, .002f );
                    BOOST_CHECK_SMALL( v.normal.dotProduct( v.tangent ), .002f );
                    if( section.name == "gt_body_paint" )
                    {
                        smoothBody |= references[i] >= 5;
                    }
                    if( section.material == VehicleMaterial::Headlight )
                    {
                        BOOST_CHECK_GT( v.normal.y, .15f );
                        leftLight |= v.position.x < 0;
                        rightLight |= v.position.x > 0;
                    }
                }
            }
            BOOST_CHECK( wing && splitter && diffuser && roof && rain && cabin && smoothBody );
            BOOST_CHECK( leftLight && rightLight );
            BOOST_CHECK_EQUAL( vents, details && lod.level < 2 );
            BOOST_CHECK_EQUAL( cage, details && lod.level < 2 );
            BOOST_CHECK_EQUAL( tow, details && lod.level < 2 );
            BOOST_CHECK_LT( triangles, previous );
            previous = triangles;
        }
    }
}

BOOST_AUTO_TEST_CASE( standalone_dimensions_keep_all_race_surfaces_finite_and_non_degenerate )
{
    for( float radius : { .25f, .50f } )
    {
        for( float length : { 4.0f, 5.6f } )
        {
            VehicleGeometryConfig request;
            request.bodyStyle = VehicleBodyStyle::GT;
            request.wheelbase = 2.78f;
            request.bodyLength = length;
            request.bodyWidth = 2.01f;
            request.tyreRadius = radius;
            const auto car = WPVehicleGeometry::generate( request );
            BOOST_REQUIRE( car.hasGeometry() );
            for( const auto &lod : car.lods )
            {
                for( const auto &section : lod.sections )
                {
                    for( std::size_t i = 0; i + 2 < section.indices.size(); i += 3 )
                    {
                        const auto &a = section.vertices.at( section.indices[i] ).position;
                        const auto &b = section.vertices.at( section.indices[i + 1] ).position;
                        const auto &c = section.vertices.at( section.indices[i + 2] ).position;
                        const auto cross = ( b - a ).crossProduct( c - a );
                        BOOST_REQUIRE( std::isfinite( cross.x ) && std::isfinite( cross.y ) &&
                                       std::isfinite( cross.z ) );
                        BOOST_CHECK_GT( cross.dotProduct( cross ), 1e-14f );
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
