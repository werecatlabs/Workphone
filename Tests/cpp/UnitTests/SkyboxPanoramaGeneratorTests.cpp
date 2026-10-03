#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

BOOST_AUTO_TEST_CASE( skybox_panorama_generator_builds_configurable_grid )
{
    TestGuard fixture;

    // Skip if graphics system is not available
    if( !fixture.graphicsSystem )
    {
        //BOOST_TEST_MESSAGE(""Graphics system not available - skipping skybox panorama generator test"");
        BOOST_CHECK( true );
        return;
    }

    auto actor = fixture.sceneManager->createActor();
    BOOST_REQUIRE( actor );
    actor->addComponent<Skybox>();

    auto generator = actor->addComponent<SkyboxPanorama>();
    BOOST_REQUIRE( generator );

    generator->setGridColumns( 3 );
    generator->setGridRows( 2 );
    generator->setSpacingX( 8.0f );
    generator->setSpacingZ( 12.0f );
    generator->setPanoramaDirectory( "PanoramasTest" );
    generator->setFilePrefix( "road" );
    generator->rebuildPanoramas();

    BOOST_CHECK_EQUAL( generator->getPanoramaCount(), 6u );

    const auto *first = generator->getPanorama( 0 );
    const auto *last = generator->getPanorama( 5 );
    BOOST_REQUIRE( first );
    BOOST_REQUIRE( last );

    BOOST_CHECK_CLOSE( first->position.X(), -8.0f, 0.001f );
    BOOST_CHECK_CLOSE( first->position.Z(), -6.0f, 0.001f );
    BOOST_CHECK_CLOSE( last->position.X(), 8.0f, 0.001f );
    BOOST_CHECK_CLOSE( last->position.Z(), 6.0f, 0.001f );
    //BOOST_CHECK_EQUAL(first->textureNames[0], ""Panoramas/Test/road_c0_r0_front.jpg"");
    //BOOST_CHECK_EQUAL(last->textureNames[5], ""Panoramas/Test/road_c2_r1_down.jpg"");

    fixture.sceneManager->destroyActor( actor );
}

BOOST_AUTO_TEST_CASE( skybox_panorama_generator_moves_to_connected_station )
{
    TestGuard fixture;

    // Skip if graphics system is not available
    if( !fixture.graphicsSystem )
    {
        //BOOST_TEST_MESSAGE(""Graphics system not available - skipping skybox panorama generator test"");
        BOOST_CHECK( true );
        return;
    }

    auto panoramaActor = fixture.sceneManager->createActor();
    auto cameraActor = fixture.sceneManager->createActor();
    BOOST_REQUIRE( panoramaActor );
    BOOST_REQUIRE( cameraActor );

    panoramaActor->addComponent<Skybox>();
    auto generator = panoramaActor->addComponent<SkyboxPanorama>();
    BOOST_REQUIRE( generator );

    generator->setCameraActor( cameraActor );
    generator->setGridColumns( 3 );
    generator->setGridRows( 1 );
    generator->setSpacingX( 10.0f );
    generator->setMaximumLinkDistance( 11.0f );
    generator->setTransitionEnabled( false );
    generator->rebuildPanoramas();

    BOOST_REQUIRE( generator->moveToPanorama( 1 ) );
    BOOST_REQUIRE( generator->moveInDirection( Vector3<real_Num>::unitX() ) );
    BOOST_CHECK_EQUAL( generator->getActivePanorama(), 2 );

    const auto *active = generator->getPanorama( 2 );
    BOOST_REQUIRE( active );
    BOOST_CHECK_SMALL( ( cameraActor->getPosition() - active->position ).lengthSquared(), 0.001f );

    fixture.sceneManager->destroyActor( cameraActor );
    fixture.sceneManager->destroyActor( panoramaActor );
}

/*
BOOST_AUTO_TEST_CASE( skybox_panorama_generator_interpolates_camera_transition )
{
    TestGuard fixture;

    // Skip if graphics system is not available
    if (!fixture.graphicsSystem)
    {
        BOOST_TEST_MESSAGE(""Graphics system not available - skipping skybox panorama generator test"");
        BOOST_CHECK(true);
        return;
    }

    auto panoramaActor = fixture.sceneManager->createActor();
    auto cameraActor = fixture.sceneManager->createActor();
    BOOST_REQUIRE(panoramaActor);
    BOOST_REQUIRE(cameraActor);

    panoramaActor->addComponent<Skybox>();
    auto generator = panoramaActor->addComponent<SkyboxPanorama>();
    BOOST_REQUIRE(generator);

    generator->setCameraActor(cameraActor);
    generator->setGridColumns(2);
    generator->setGridRows(1);
    generator->setSpacingX(10.0f);
    generator->setTransitionDuration(2.0f);
    generator->setTransitionEasing(SkyboxPanorama::TransitionEasing::Linear);
    generator->setTransitionArcHeight(2.0f);
    generator->setPanoramaBlendMode(SkyboxPanorama::PanoramaBlendMode::TimedSwitch);
    generator->rebuildPanoramas();

    BOOST_REQUIRE(generator->moveToPanorama(0));
    BOOST_REQUIRE(generator->moveToPanorama(1));
    BOOST_CHECK(generator->isTransitioning());
    BOOST_CHECK_EQUAL(generator->getTargetPanorama(), 1);

    generator->advanceTransition(1.0f);
    BOOST_CHECK_CLOSE(generator->getTransitionProgress(), 0.5f, 0.001f);
    BOOST_CHECK_CLOSE(cameraActor->getPosition().X(), 0.0f, 0.001f);
    BOOST_CHECK_CLOSE(cameraActor->getPosition().Y(), 3.7f, 0.001f);

    generator->advanceTransition(1.0f);
    BOOST_CHECK(!generator->isTransitioning());
    BOOST_CHECK_EQUAL(generator->getActivePanorama(), 1);

    const auto *target = generator->getPanorama(1);
    BOOST_REQUIRE(target);
    BOOST_CHECK_SMALL((cameraActor->getPosition() - target->position).lengthSquared(), 0.001f);

    fixture.sceneManager->destroyActor(cameraActor);
    fixture.sceneManager->destroyActor(panoramaActor);
}*/

BOOST_AUTO_TEST_CASE( skybox_panorama_generator_round_trips_properties )
{
    TestGuard fixture;

    // Skip if graphics system is not available
    if( !fixture.graphicsSystem )
    {
        //BOOST_TEST_MESSAGE(""Graphics system not available - skipping skybox panorama generator test"");
        BOOST_CHECK( true );
        return;
    }

    auto actor = fixture.sceneManager->createActor();
    BOOST_REQUIRE( actor );
    actor->addComponent<Skybox>();

    auto generator = actor->addComponent<SkyboxPanorama>();
    BOOST_REQUIRE( generator );

    auto properties = generator->getProperties();
    properties->setProperty( SkyboxPanorama::gridColumnsStr, 4u );
    properties->setProperty( SkyboxPanorama::gridRowsStr, 3u );
    properties->setProperty( SkyboxPanorama::spacingXStr, 7.5f );
    properties->setProperty( SkyboxPanorama::spacingZStr, 9.5f );
    properties->setProperty( SkyboxPanorama::switchDistanceStr, 2.5f );
    properties->setProperty( SkyboxPanorama::transitionDurationStr, 1.25f );
    properties->setProperty( SkyboxPanorama::transitionEasingStr,
                             static_cast<s32>( SkyboxPanorama::TransitionEasing::EaseInOut ) );
    properties->setProperty( SkyboxPanorama::crossFadeStartStr, 0.2f );
    properties->setProperty( SkyboxPanorama::crossFadeEndStr, 0.8f );
    generator->setProperties( properties );

    BOOST_CHECK_EQUAL( generator->getPanoramaCount(), 12u );
    BOOST_CHECK_CLOSE( generator->getSpacingX(), 7.5f, 0.001f );
    BOOST_CHECK_CLOSE( generator->getSpacingZ(), 9.5f, 0.001f );
    BOOST_CHECK_CLOSE( generator->getSwitchDistance(), 2.5f, 0.001f );
    BOOST_CHECK_CLOSE( generator->getTransitionDuration(), 1.25f, 0.001f );
    BOOST_CHECK( generator->getTransitionEasing() == SkyboxPanorama::TransitionEasing::EaseInOut );
    BOOST_CHECK_CLOSE( generator->getCrossFadeStart(), 0.2f, 0.001f );
    BOOST_CHECK_CLOSE( generator->getCrossFadeEnd(), 0.8f, 0.001f );

    fixture.sceneManager->destroyActor( actor );
}
