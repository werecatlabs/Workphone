#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/RenderTexture.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    struct CameraFallbackFixture : TestGuard
    {
        SmartPtr<ICameraManager> previousManager = applicationManager->getCameraManager();
        const bool wasEditor = applicationManager->isEditor();
        const bool wasPlaying = applicationManager->isPlaying();
        const bool wasEditorCamera = applicationManager->isEditorCamera();
        SmartPtr<CameraManager> cameras = make_ptr<CameraManager>();
        SmartPtr<IGameActor> editorActor;
        SmartPtr<Camera> editorCamera;

        CameraFallbackFixture()
        {
            scene->clear( true );
            applicationManager->setEditor( true );
            applicationManager->setPlaying( true );
            applicationManager->setEditorCamera( false );
            cameras->load( nullptr );
            applicationManager->setCameraManager( cameras );
            editorActor = sceneManager->createActor();
            editorActor->setName( "EditorCamera" );
            editorActor->setFlag( IGameActor::ActorFlagIsEditor, true );
            editorActor->setPerpetual( true );
            editorCamera = editorActor->addComponent<Camera>();
            cameras->setEditorCamera( editorActor );
        }

        ~CameraFallbackFixture()
        {
            scene->clear( true );
            cameras->setEditorCamera( nullptr );
            sceneManager->destroyActor( editorActor, true );
            editorCamera = nullptr;
            editorActor = nullptr;
            cameras->unload( nullptr );
            applicationManager->setCameraManager( previousManager );
            applicationManager->setEditor( wasEditor );
            applicationManager->setPlaying( wasPlaying );
            applicationManager->setEditorCamera( wasEditorCamera );
        }

        SmartPtr<Camera> addCamera()
        {
            auto actor = createBasicActor();
            scene->addActor( actor );
            return actor->addComponent<Camera>();
        }

        void checkFallback( bool expected )
        {
            cameras->reset();
            BOOST_CHECK_EQUAL( editorCamera->isActive(), expected );
            BOOST_CHECK_EQUAL( cameras->isEditorCameraEnabled(), expected );
            // A render fallback must not select the editor's camera controls.
            BOOST_CHECK( !applicationManager->isEditorCamera() );
            BOOST_CHECK( applicationManager->isPlaying() );
        }
    };
}

BOOST_FIXTURE_TEST_SUITE( CameraManagerFallbackTests, CameraFallbackFixture )

BOOST_AUTO_TEST_CASE( menu_camera_hands_off_and_returns_after_mission_unloads )
{
    BOOST_REQUIRE( scene->getComponents<Camera>().empty() );
    for( unsigned cycle = 0; cycle < 3; ++cycle )
    {
        checkFallback( true );
        auto gameCamera = addCamera();
        checkFallback( false );
        BOOST_CHECK( gameCamera->isActive() );
        auto actor = gameCamera->getActor();
        scene->removeActor( actor );
        sceneManager->destroyActor( actor, true );
        checkFallback( true );
    }
}

BOOST_AUTO_TEST_CASE( ineligible_and_offscreen_cameras_do_not_hide_menu )
{
    auto disabledComponent = addCamera();
    disabledComponent->setEnabled( false );
    auto disabledActor = addCamera();
    disabledActor->getActor()->setEnabled( false );
    auto excluded = addCamera();
    auto properties = excluded->getProperties();
    properties->setProperty( Camera::s_isActiveStr, false );
    properties->getPropertyObject( Camera::s_isActiveStr ).setAttribute( "manualActivation", "true" );
    excluded->setProperties( properties );
    auto offscreen = addCamera();
    offscreen->setOutputRenderTexture( make_ptr<RenderTexture>() );

    checkFallback( true );
    BOOST_CHECK( !disabledComponent->isActive() );
    BOOST_CHECK( !disabledActor->isActive() );
    BOOST_CHECK( !excluded->isActive() );
    BOOST_CHECK( offscreen->isActive() );

    // Enabling a genuine scene camera must release the fallback immediately.
    disabledComponent->setEnabled( true );
    checkFallback( false );
    BOOST_CHECK( disabledComponent->isActive() );
    disabledComponent->setEnabled( false );
    checkFallback( true );
}

BOOST_AUTO_TEST_CASE( standalone_never_activates_an_editor_fallback )
{
    applicationManager->setEditor( false );
    checkFallback( false );
    auto gameCamera = addCamera();
    checkFallback( false );
    BOOST_CHECK( gameCamera->isActive() );
}

BOOST_AUTO_TEST_CASE( explicit_editor_camera_override_still_wins )
{
    auto gameCamera = addCamera();
    checkFallback( false );
    applicationManager->setEditorCamera( true );
    cameras->reset();
    BOOST_CHECK( editorCamera->isActive() );
    BOOST_CHECK( !gameCamera->isActive() );
    applicationManager->setEditorCamera( false );
    checkFallback( false );
    BOOST_CHECK( gameCamera->isActive() );
}

BOOST_AUTO_TEST_SUITE_END()
