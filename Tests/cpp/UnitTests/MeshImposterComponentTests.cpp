#include "UnitTests.hpp"
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Scene/Components/MeshImposter.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Scene/CameraManager.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    template <class T, class U> void checkVector( const Vector3<T> &actual, const Vector3<U> &expected )
    {
        BOOST_CHECK_SMALL( double( actual.x - expected.x ), .0001 );
        BOOST_CHECK_SMALL( double( actual.y - expected.y ), .0001 );
        BOOST_CHECK_SMALL( double( actual.z - expected.z ), .0001 );
    }

    struct ImposterFixture : TestGuard
    {
        SmartPtr<ICameraManager> previousCameras = applicationManager->getCameraManager();
        const bool wasEditor = applicationManager->isEditor();
        const bool wasPlaying = applicationManager->isPlaying();
        const bool wasEditorCamera = applicationManager->isEditorCamera();
        const u32 previousTaskFlags = Thread::getTaskFlags();
        SmartPtr<CameraManager> cameras = make_ptr<CameraManager>();
        SmartPtr<IGameActor> parent, actor;
        SmartPtr<MeshRenderer> renderer;
        SmartPtr<render::Material> material;
        SmartPtr<MeshImposter> imposter;
        SmartPtr<LODGroup> group;
        SmartPtr<LODSystem> system;
        bool ownsSystem = false;
        MeshImposterSpriteSheet sheet;
        const Vector3F centre{ 3, 2, -4 };
        bool expectedPerspective = false, expectedRayValid = false;
        Vector3<real_Num> expectedLocalView;

        ImposterFixture()
        {
            scene->clear( true );
            // These are synchronous transform contracts. Creation-state actors
            // defer hierarchy updates, and must enter Play as production actors do.
            Thread::setTaskFlags( std::numeric_limits<u32>::max() );
            cameras->load( nullptr );
            applicationManager->setCameraManager( cameras );
            applicationManager->setEditor( false );
            applicationManager->setEditorCamera( false );
            parent = createBasicActor();
            actor = createBasicActor();
            scene->addActor( parent );
            parent->addChild( actor );
            actor->setFlag( IGameActor::ActorFlagWorldSpaceTransform, false );
            parent->setState( IGameActor::State::Play );
            actor->setState( IGameActor::State::Play );
            renderer = actor->addComponent<MeshRenderer>();
            imposter = actor->addComponent<MeshImposter>();
            material = make_ptr<render::Material>();
            material->load( nullptr );
            // UV settings live in MaterialPassStateData; a bare Material has no
            // passes and its getters can only return fallback values.
            auto technique = material->createTechnique();
            BOOST_REQUIRE( technique );
            auto pass = technique->createPass();
            BOOST_REQUIRE( pass );
            BOOST_REQUIRE( pass->getStateContext() );
            const Array<MeshImposterTriangle> triangles = {
                { { Vector3F( -1, 0, -1 ), Vector3F( 1, 0, -1 ), Vector3F( 0, 4, 0 ) }, ColourF::White },
                { { Vector3F( 1, 0, -1 ), Vector3F( 0, 0, 1 ), Vector3F( 0, 4, 0 ) }, ColourF::White },
                { { Vector3F( 0, 0, 1 ), Vector3F( -1, 0, -1 ), Vector3F( 0, 4, 0 ) }, ColourF::White }
            };
            MeshImposterSpriteSheetOptions options;
            options.tileWidth = 16; options.tileHeight = 24;
            options.azimuthViews = 8; options.elevationRings = 3; options.columns = 5;
            options.supersampling = 1;
            sheet = bakeMeshImposterSpriteSheet( triangles, { -2, -.2f, -1.1f }, { 2, 5, 1.1f }, options );
            imposter->configure( sheet.views, centre, material );
            group = parent->addComponent<LODGroup>();
            group->addLevel( 0.f, { renderer } );
            group->setCullBelowLastLOD( false );
            group->load( nullptr );
            system = dynamic_pointer_cast<LODSystem>( group->getComponentSystem() );
            ownsSystem = !system;
            if( ownsSystem )
            {
                system = make_ptr<LODSystem>();
                system->load( nullptr );
                system->addComponent( group );
            }
            system->clearViewOverride();
        }

        ~ImposterFixture()
        {
            Thread::setCurrentTask( TaskId::Primary );
            system->clearViewOverride();
            if( ownsSystem )
            {
                system->removeComponent( static_pointer_cast<IComponent>( group ) );
                system->unload( nullptr );
            }
            imposter->unload( nullptr );
            scene->clear( true );
            material->unload( nullptr );
            cameras->unload( nullptr );
            applicationManager->setCameraManager( previousCameras );
            applicationManager->setEditor( wasEditor );
            applicationManager->setPlaying( wasPlaying );
            applicationManager->setEditorCamera( wasEditorCamera );
            Thread::setTaskFlags( previousTaskFlags );
        }

        Vector3<real_Num> cameraFor( size_t view ) const
        {
            const auto local = centre + sheet.views[view].towardCamera * 100.f;
            const auto pose = parent->getWorldTransform();
            const Vector3<real_Num> localReal( local.x, local.y, local.z );
            return pose.getPosition() + pose.getOrientation() * ( localReal * pose.getScale() );
        }

        void rememberView( const LODSystem::View &view )
        {
            const auto parentPose = parent->getWorldTransform();
            expectedPerspective = !view.orthographic;
            auto local = parentPose.getOrientation().inverse() * ( view.orthographic
                ? -view.direction : view.position - parentPose.getPosition() );
            expectedLocalView = local / parentPose.getScale();
            expectedRayValid = true;
        }

        bool updateCamera( const Vector3<real_Num> &position )
        {
            const bool applied = imposter->updateForCamera( position );
            if( applied )
            {
                LODSystem::View view;
                view.position = position;
                rememberView( view );
            }
            return applied;
        }

        bool updateView( const LODSystem::View &view )
        {
            const bool applied = imposter->updateForView( view );
            if( applied ) rememberView( view );
            return applied;
        }

        void updateActive()
        {
            LODSystem::View view;
            const bool hasView = system->getView( view );
            imposter->update();
            if( hasView ) rememberView( view );
        }

        void loadImposter()
        {
            imposter->load( nullptr );
            expectedPerspective = expectedRayValid = false;
        }

        void checkFrame( size_t index )
        {
            BOOST_CHECK_EQUAL( imposter->getSelectedView(), s32( index ) );
            const auto &view = sheet.views[index];
            const auto pose = actor->getLocalTransform();
            const auto middle = ( view.minimum + view.maximum ) * .5f;
            const auto oldCentre = centre + view.right * middle.x + view.up * middle.y;
            const Vector3<real_Num> captureCentre( centre.x, centre.y, centre.z );
            const Vector3<real_Num> oldPosition( oldCentre.x, oldCentre.y, oldCentre.z );
            const Vector3<real_Num> normal( view.towardCamera.x, view.towardCamera.y, view.towardCamera.z );
            real_Num footprintScale = 1;
            real_Num expectedDepth = view.maximumDepth;
            auto expectedPosition = oldPosition;
            if( expectedPerspective )
            {
                const auto cameraDepth = ( expectedLocalView - captureCentre ).dotProduct( normal );
                if( cameraDepth > .000001 && cameraDepth - view.maximumDepth > .0001 )
                {
                    footprintScale = ( cameraDepth - view.maximumDepth ) / cameraDepth;
                    expectedPosition = expectedLocalView + ( oldPosition - expectedLocalView ) * footprintScale;
                }
                else expectedDepth = 0;
            }
            else
            {
                const auto ray = expectedRayValid ? expectedLocalView : normal;
                const auto alignment = ray.normaliseCopy().dotProduct( normal );
                if( alignment > .000001 )
                    expectedPosition += ray * ( view.maximumDepth / ray.dotProduct( normal ) );
                else expectedDepth = 0;
            }
            checkVector( pose.getPosition(), expectedPosition );
            checkVector( pose.getScale(), Vector3F( view.maximum.x - view.minimum.x,
                view.maximum.y - view.minimum.y, 1 ) * Vector3F( f32( footprintScale ), f32( footprintScale ), 1 ) );
            BOOST_CHECK_SMALL( double( ( pose.getPosition() - captureCentre ).dotProduct( normal ) -
                                      expectedDepth ), .0001 );
            checkVector( pose.getOrientation() * Vector3<real_Num>( 1, 0, 0 ), view.right );
            checkVector( pose.getOrientation() * Vector3<real_Num>( 0, 1, 0 ), view.up );
            checkVector( pose.getOrientation() * Vector3<real_Num>( 0, 0, 1 ), view.towardCamera );
            const auto tiling = material->getUVTiling(), offset = material->getUVOffset();
            BOOST_CHECK_SMALL( tiling.x - ( view.uvMaximum.x - view.uvMinimum.x ), .00001f );
            BOOST_CHECK_SMALL( tiling.y - ( view.uvMaximum.y - view.uvMinimum.y ), .00001f );
            BOOST_CHECK_SMALL( offset.x - view.uvMinimum.x, .00001f );
            BOOST_CHECK_SMALL( offset.y - view.uvMinimum.y, .00001f );
            BOOST_CHECK( actor->getComponent<MeshRenderer>() == renderer );
            BOOST_CHECK_EQUAL( actor->getComponentsByType<MeshRenderer>().size(), 1u );
        }

        SmartPtr<Camera> addCamera( size_t view, u32 priority )
        {
            auto owner = createBasicActor();
            scene->addActor( owner );
            owner->setState( IGameActor::State::Play );
            auto camera = owner->addComponent<Camera>();
            camera->setZOrder( priority );
            owner->setPosition( cameraFor( view ) );
            owner->updateTransform();
            cameras->addCamera( owner );
            camera->setActive( true );
            return camera;
        }
    };
}

BOOST_AUTO_TEST_SUITE( MeshImposterComponentTests )

BOOST_FIXTURE_TEST_CASE( selected_frames_update_one_renderer_pose_and_material_uv, ImposterFixture )
{
    checkFrame( 0 );
    for( const auto index : { 4u, 2u, 16u, 24u, 0u } )
    {
        BOOST_REQUIRE( updateCamera( cameraFor( index ) ) );
        checkFrame( index );
    }
    BOOST_CHECK_EQUAL( imposter->getSelectionChanges(), 5u );
}

BOOST_FIXTURE_TEST_CASE( parent_rotation_and_nonuniform_scale_preserve_capture_space, ImposterFixture )
{
    parent->setPosition( { 10, -3, 20 } );
    parent->setOrientation( Quaternion<real_Num>::eulerDegrees( 25, 55, -10 ) );
    parent->setScale( { 2, 3, .5 } );
    parent->updateTransform();
    for( const auto index : { 1u, 4u, 19u, 24u } )
    {
        BOOST_REQUIRE( updateCamera( cameraFor( index ) ) );
        checkFrame( index );
        const auto local = actor->getLocalTransform();
        const auto parentPose = parent->getWorldTransform();
        checkVector( actor->getWorldTransform().getPosition(), parentPose.getPosition() +
            parentPose.getOrientation() * ( local.getPosition() * parentPose.getScale() ) );
    }
    const auto changes = imposter->getSelectionChanges();
    parent->setScale( { 0, 3, .5 } );
    parent->updateTransform();
    BOOST_CHECK( !updateCamera( { 100, 20, 30 } ) );
    BOOST_CHECK_EQUAL( imposter->getSelectedView(), 24 );
    BOOST_CHECK_EQUAL( imposter->getSelectionChanges(), changes );
}

BOOST_FIXTURE_TEST_CASE( camera_updates_hysteresis_and_invalid_views_preserve_the_previous_frame, ImposterFixture )
{
    const auto direction = [&]( float degrees ) {
        const float radians = degrees * 3.14159265358979323846f / 180.f;
        return Vector3<real_Num>( centre.x + 100 * std::sin( radians ), centre.y,
                                  centre.z + 100 * std::cos( radians ) );
    };
    BOOST_REQUIRE( updateCamera( direction( 22.6f ) ) );
    checkFrame( 0 );
    BOOST_CHECK_EQUAL( imposter->getSelectionChanges(), 0u );
    BOOST_REQUIRE( updateCamera( direction( 35.f ) ) );
    checkFrame( 1 );
    BOOST_REQUIRE( updateCamera( direction( 35.f ) ) );
    BOOST_CHECK_EQUAL( imposter->getSelectionChanges(), 1u );

    auto first = addCamera( 0, 1 );
    loadImposter();
    Thread::setCurrentTask( TaskId::Application );
    updateActive();
    checkFrame( 0 );
    auto second = addCamera( 4, 2 );
    updateActive();
    checkFrame( 4 );
    first->setActive( false ); second->setActive( false );
    const auto changes = imposter->getSelectionChanges();
    updateActive();
    BOOST_CHECK_EQUAL( imposter->getSelectionChanges(), changes );
    checkFrame( 4 );
    LODSystem::View view;
    view.position = cameraFor( 24 );
    system->setViewOverride( view );
    updateActive();
    checkFrame( 24 );
    view.position.x = std::numeric_limits<real_Num>::quiet_NaN();
    system->setViewOverride( view );
    updateActive();
    checkFrame( 24 );
    BOOST_CHECK( !updateCamera( view.position ) );
    BOOST_CHECK( !updateCamera( { centre.x, centre.y, centre.z } ) );
    system->clearViewOverride();
    first->setActive( true );
    updateActive();
    checkFrame( 0 );
}

BOOST_FIXTURE_TEST_CASE( configure_is_transactional_reload_preserves_views_and_destruction_retires_metadata, ImposterFixture )
{
    auto invalid = sheet.views;
    invalid[0].uvMaximum.x = 2.f;
    BOOST_CHECK_THROW( imposter->configure( invalid, centre, material ), std::invalid_argument );
    invalid = sheet.views;
    invalid[0].maximumDepth = std::numeric_limits<f32>::quiet_NaN();
    BOOST_CHECK_THROW( imposter->configure( invalid, centre, material ), std::invalid_argument );
    BOOST_CHECK_THROW( imposter->configure( sheet.views, centre, nullptr ), std::invalid_argument );
    checkFrame( 0 );

    auto scheduler = WorldStreamingScheduler::shared();
    const auto source = size_t( StreamMemoryClass::SourceCpu );
    const auto originalLimit = scheduler->stats().limits[source];
    addCleanup( [scheduler, originalLimit]() {
        String error;
        scheduler->setBudget( StreamMemoryClass::SourceCpu, originalLimit, error );
    } );
    const auto before = scheduler->stats().used[source];
    String error;
    BOOST_REQUIRE( scheduler->setBudget( StreamMemoryClass::SourceCpu, before, error ) );
    BOOST_CHECK_THROW( imposter->configure( sheet.views, centre, material ), std::bad_alloc );
    BOOST_CHECK_EQUAL( scheduler->stats().used[source], before );
    BOOST_REQUIRE( scheduler->setBudget( StreamMemoryClass::SourceCpu, originalLimit, error ) );
    BOOST_CHECK_EQUAL( imposter->getViewCount(), sheet.views.size() );
    checkFrame( 0 );

    loadImposter();
    BOOST_REQUIRE( updateCamera( cameraFor( 4 ) ) );
    imposter->unload( nullptr );
    BOOST_CHECK_EQUAL( imposter->getViewCount(), sheet.views.size() );
    checkFrame( 4 );
    // Component::unload detaches its actor; the actor's reconstruction/attachment
    // path supplies the owner again before loading retained component data.
    imposter->setActor( actor );
    loadImposter();
    BOOST_REQUIRE( imposter->getActor() == actor );
    checkFrame( 4 );
    BOOST_REQUIRE( updateCamera( cameraFor( 24 ) ) );
    checkFrame( 24 );
    BOOST_CHECK( actor->getComponent<MeshRenderer>() == renderer );

    // A detached component owns the same admitted metadata without scene/FSM
    // lifetimes masking its final release. Unload intentionally retains it for
    // Editor stop/play; the last owner must retire it.
    auto detached = make_ptr<MeshImposter>();
    detached->configure( sheet.views, centre, material );
    BOOST_CHECK_EQUAL( detached->getViewCount(), sheet.views.size() );
    const auto retainedViews = sourceBudgetStorageBytes( sheet.views.capacity() * sizeof( MeshImposterView ),
                                                        alignof( MeshImposterView ) );
    const auto beforeDestroy = scheduler->stats().used[source];
    detached = nullptr;
    BOOST_CHECK_LE( scheduler->stats().used[source] + retainedViews, beforeDestroy );
}

BOOST_FIXTURE_TEST_CASE( orthographic_rays_select_consistent_views_across_translated_and_transformed_patches, ImposterFixture )
{
    auto otherParent = createBasicActor();
    auto otherActor = createBasicActor();
    scene->addActor( otherParent );
    otherParent->addChild( otherActor );
    otherActor->setFlag( IGameActor::ActorFlagWorldSpaceTransform, false );
    otherParent->setState( IGameActor::State::Play );
    otherActor->setState( IGameActor::State::Play );
    otherParent->setPosition( { 1000, 60, -500 } );
    otherParent->updateTransform();
    auto otherMaterial = make_ptr<render::Material>();
    otherMaterial->load( nullptr );
    auto technique = otherMaterial->createTechnique();
    BOOST_REQUIRE( technique );
    BOOST_REQUIRE( technique->createPass() );
    auto otherImposter = otherActor->addComponent<MeshImposter>();
    otherImposter->configure( sheet.views, { 50, 20, -30 }, otherMaterial );

    LODSystem::View view;
    view.orthographic = true;
    view.direction = { 0, 0, -1 };
    view.position = { 50000, -1000, -50000 };
    BOOST_REQUIRE( updateView( view ) );
    BOOST_REQUIRE( otherImposter->updateForView( view ) );
    checkFrame( 0 );
    BOOST_CHECK_EQUAL( otherImposter->getSelectedView(), 0 );
    // Opposite camera translations cannot change the shared parallel rays.
    view.position = { -50000, 1000, 50000 };
    BOOST_REQUIRE( updateView( view ) );
    BOOST_REQUIRE( otherImposter->updateForView( view ) );
    checkFrame( 0 );
    BOOST_CHECK_EQUAL( otherImposter->getSelectedView(), 0 );
    BOOST_CHECK_EQUAL( imposter->getSelectionChanges(), 0u );
    BOOST_CHECK_EQUAL( otherImposter->getSelectionChanges(), 0u );
    view.direction = { -1, 0, 0 };
    BOOST_REQUIRE( updateView( view ) );
    BOOST_REQUIRE( otherImposter->updateForView( view ) );
    checkFrame( 2 );
    BOOST_CHECK_EQUAL( otherImposter->getSelectedView(), 2 );
    view.direction = { 0, -1, 0 };
    BOOST_REQUIRE( updateView( view ) );
    BOOST_REQUIRE( otherImposter->updateForView( view ) );
    checkFrame( 24 );
    BOOST_CHECK_EQUAL( otherImposter->getSelectedView(), 24 );

    parent->setPosition( { 100, -50, 20 } );
    parent->setOrientation( Quaternion<real_Num>::eulerDegrees( 25, 55, -10 ) );
    parent->setScale( { 2, 3, .5 } );
    parent->updateTransform();
    const auto parentPose = parent->getWorldTransform();
    const auto &captureDirection = sheet.views[19].towardCamera;
    const Vector3<real_Num> localRay( captureDirection.x, captureDirection.y, captureDirection.z );
    view.direction = -( parentPose.getOrientation() * ( localRay * parentPose.getScale() ) ).normaliseCopy();
    BOOST_REQUIRE( updateView( view ) );
    checkFrame( 19 );
    parent->setPosition( { -2000, 300, 900 } );
    parent->updateTransform();
    view.position = { 20000, 7000, -20000 };
    BOOST_REQUIRE( updateView( view ) );
    checkFrame( 19 );
    const auto changes = imposter->getSelectionChanges();
    view.direction = {};
    BOOST_CHECK( !updateView( view ) );
    checkFrame( 19 );
    view.direction.x = std::numeric_limits<real_Num>::quiet_NaN();
    BOOST_CHECK( !updateView( view ) );
    BOOST_CHECK_EQUAL( imposter->getSelectionChanges(), changes );
    checkFrame( 19 );

    // Perspective callers still use camera position, even if direction is unset.
    view.orthographic = false;
    view.position = cameraFor( 4 );
    BOOST_REQUIRE( updateView( view ) );
    checkFrame( 4 );
}

BOOST_FIXTURE_TEST_CASE( managed_orthographic_camera_rotation_publishes_direction_and_updates_imposters, ImposterFixture )
{
    auto camera = addCamera( 0, 1 );
    camera->setOrthoWindowHeight( 100.f );
    loadImposter();
    Thread::setCurrentTask( TaskId::Application );
    const auto checkManaged = [&]( const Vector3<real_Num> &rotation, size_t expected ) {
        const auto orientation = Quaternion<real_Num>::eulerDegrees( rotation.x, rotation.y, rotation.z );
        camera->getActor()->setOrientation( orientation );
        camera->getActor()->updateTransform();
        LODSystem::View resolved;
        BOOST_REQUIRE( system->getView( resolved ) );
        BOOST_CHECK( resolved.orthographic );
        checkVector( resolved.direction, orientation * Vector3<real_Num>( 0, 0, -1 ) );
        updateActive();
        checkFrame( expected );
    };
    checkManaged( { 0, 0, 0 }, 0 );
    camera->getActor()->setPosition( { -5000, 2000, -4000 } );
    camera->getActor()->updateTransform();
    updateActive();
    checkFrame( 0 );
    checkManaged( { 0, 90, 0 }, 2 );
    checkManaged( { -90, 0, 0 }, 24 );
    camera->getActor()->setPosition( { 3000, -1000, 5000 } );
    camera->getActor()->updateTransform();
    updateActive();
    checkFrame( 24 );
}

BOOST_FIXTURE_TEST_CASE( front_depth_preserves_perspective_footprint_while_same_frame_camera_moves, ImposterFixture )
{
    const auto &frame = sheet.views[0];
    const Vector3<real_Num> n( frame.towardCamera.x, frame.towardCamera.y, frame.towardCamera.z );
    const Vector3<real_Num> r( frame.right.x, frame.right.y, frame.right.z );
    const Vector3<real_Num> u( frame.up.x, frame.up.y, frame.up.z );
    const Vector3<real_Num> captureCentre( centre.x, centre.y, centre.z );
    const auto state = material->getRenderState();
    BOOST_REQUIRE( state );
    const auto rendererIdentity = renderer.get();
    Vector3<real_Num> previousPosition;
    for( const auto depth : { 80., 40. } )
    {
        // Off-axis eyes detect an incorrect normal-only shift or radial-distance
        // scale. Every moved corner must remain on its original camera ray.
        const auto eye = captureCentre + n * real_Num( depth ) + r * real_Num( 5 ) + u * real_Num( 7 );
        BOOST_REQUIRE( updateCamera( eye ) );
        checkFrame( 0 );
        const auto pose = actor->getLocalTransform();
        if( depth == 40. ) BOOST_CHECK_GT( ( pose.getPosition() - previousPosition ).length(), .01 );
        previousPosition = pose.getPosition();
        for( u32 corner = 0; corner < 4; ++corner )
        {
            const auto x = corner & 1 ? frame.maximum.x : frame.minimum.x;
            const auto y = corner & 2 ? frame.maximum.y : frame.minimum.y;
            const auto original = captureCentre + r * real_Num( x ) + u * real_Num( y );
            const Vector3<real_Num> unitCorner( corner & 1 ? .5 : -.5, corner & 2 ? .5 : -.5, 0 );
            const auto moved = pose.getPosition() + pose.getOrientation() * ( unitCorner * pose.getScale() );
            checkVector( ( moved - eye ).normaliseCopy(), ( original - eye ).normaliseCopy() );
            BOOST_CHECK_SMALL( double( ( moved - captureCentre ).dotProduct( n ) - frame.maximumDepth ), .0001 );
        }
        BOOST_CHECK_EQUAL( imposter->getSelectionChanges(), 0u );
        BOOST_CHECK( material->getRenderState() == state );
        BOOST_CHECK( actor->getComponent<MeshRenderer>().get() == rendererIdentity );
    }

    // Inside the support plane, retain the current directional chart at its
    // original non-inverted plane; nearby detail supplies the actual volume.
    BOOST_REQUIRE( updateCamera( captureCentre + n * real_Num( frame.maximumDepth * .5f ) ) );
    checkFrame( 0 );
    BOOST_CHECK_GT( actor->getLocalTransform().getScale().x, 0 );
    const auto retained = actor->getLocalTransform();
    BOOST_CHECK( !updateCamera( { std::numeric_limits<real_Num>::quiet_NaN(), 0, 0 } ) );
    BOOST_CHECK( !updateCamera( { std::numeric_limits<real_Num>::max(), centre.y, centre.z } ) );
    checkVector( actor->getLocalTransform().getPosition(), retained.getPosition() );
    checkVector( actor->getLocalTransform().getScale(), retained.getScale() );
    BOOST_CHECK( material->getRenderState() == state );
}

BOOST_FIXTURE_TEST_CASE( off_axis_orthographic_depth_shift_preserves_parallel_projection_and_same_frame_uv, ImposterFixture )
{
    const auto &frame = sheet.views[0];
    const Vector3<real_Num> n( frame.towardCamera.x, frame.towardCamera.y, frame.towardCamera.z );
    const Vector3<real_Num> r( frame.right.x, frame.right.y, frame.right.z );
    const Vector3<real_Num> u( frame.up.x, frame.up.y, frame.up.z );
    const Vector3<real_Num> captureCentre( centre.x, centre.y, centre.z );
    const auto state = material->getRenderState();
    BOOST_REQUIRE( state );
    Vector3<real_Num> previousPosition;
    for( const auto offset : { .1, .2 } )
    {
        const auto ray = ( n + r * real_Num( offset ) + u * real_Num( .1 ) ).normaliseCopy();
        LODSystem::View view;
        view.orthographic = true;
        view.direction = -ray;
        view.position = { 5000, -4000, 2000 };
        BOOST_REQUIRE( updateView( view ) );
        checkFrame( 0 );
        const auto pose = actor->getLocalTransform();
        if( offset == .2 ) BOOST_CHECK_GT( ( pose.getPosition() - previousPosition ).length(), .01 );
        previousPosition = pose.getPosition();
        for( u32 corner = 0; corner < 4; ++corner )
        {
            const auto x = corner & 1 ? frame.maximum.x : frame.minimum.x;
            const auto y = corner & 2 ? frame.maximum.y : frame.minimum.y;
            const auto original = captureCentre + r * real_Num( x ) + u * real_Num( y );
            const Vector3<real_Num> unitCorner( corner & 1 ? .5 : -.5, corner & 2 ? .5 : -.5, 0 );
            const auto moved = pose.getPosition() + pose.getOrientation() * ( unitCorner * pose.getScale() );
            BOOST_CHECK_SMALL( double( ( moved - original ).crossProduct( ray ).length() ), .0001 );
            BOOST_CHECK_SMALL( double( ( moved - captureCentre ).dotProduct( n ) - frame.maximumDepth ), .0001 );
        }
        BOOST_CHECK_EQUAL( imposter->getSelectionChanges(), 0u );
        BOOST_CHECK( material->getRenderState() == state );
    }
}

BOOST_AUTO_TEST_SUITE_END()
