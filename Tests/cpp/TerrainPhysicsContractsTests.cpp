#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Scene/Components/Terrain/CollisionTerrain.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/StateManager.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <Workphone/System/CommandManagerMT.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainEditing.hpp>
#include <WPPhysics/WPPhysics.hpp>
#include <Workphone/Physics/TerrainShape.hpp>
#include <Workphone/Physics/PhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Interface/Physics/IBoxShape3.hpp>
#include <workphone_physics_scene.h>
#include <workphone_physics_narrowphase.h>
#include <workphone_physics_rigidbody.h>
#include <workphone_physics_collisionshape.h>
#include <cmath>
#include <cstdio>
#include <limits>
#include <future>
#include <chrono>
#include <stdexcept>

using namespace workphone;

namespace
{
    void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }

    wp_vec3f nativePoint( const Vector3<real_Num> &point )
    {
        return { f32( point.x ), f32( point.y ), f32( point.z ) };
    }

    Vector3<real_Num> worldPoint( const Transform3<real_Num> &placement, const Vector3<real_Num> &local )
    {
        return placement.getPosition() + placement.getOrientation() * ( local * placement.getScale() );
    }

    template <class Native, class Interface> Native *nativeHandle( Interface *object )
    {
        void *handle = nullptr;
        if( object ) object->_getObject( &handle );
        return static_cast<Native *>( handle );
    }

    struct NativeScene
    {
        SmartPtr<physics::IPhysicsScene3> owner;
        wp_physics_scene *scene = nullptr;
        explicit NativeScene( SmartPtr<physics::IPhysicsManager> manager ) : owner( manager->addScene() )
        {
            scene = nativeHandle<wp_physics_scene>( owner.get() );
            require( owner && scene, "Registered backend scene exposes its native handle" );
        }
    };

    struct Probe
    {
        wp_rigidbody *body = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
        wp_collision_shape *shape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE );
        wp_narrowphase *narrowphase = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
        Probe()
        {
            require( body && shape && narrowphase, "Contact probe allocation" );
            wp_collision_shape_set_sphere_radius( shape, .2f );
            wp_rigidbody_add_shape( body, shape );
        }
        ~Probe()
        {
            wp_narrowphase_destroy( narrowphase );
            wp_rigidbody_destroy( body );
            wp_collision_shape_destroy( shape );
        }
    };

    void checkRay( wp_physics_scene *scene, const render::TerrainData &data,
                   const Transform3<real_Num> &placement, Vector3<real_Num> start,
                   Vector3<real_Num> end )
    {
        const auto delta = end - start;
        Vector3<real_Num> expected;
        require( render::intersectTerrain( data, placement,
                                           Ray3F( { f32( start.x ), f32( start.y ), f32( start.z ) },
                                                  { f32( delta.x ), f32( delta.y ), f32( delta.z ) } ),
                                           expected ),
                 "Authoritative terrain query hits fixture" );
        wp_vec3f point{}, normal{};
        require( wp_physics_scene_intersects( scene, nativePoint( start ), nativePoint( end ), &point,
                                              &normal, 0, 0 ) != 0,
                 "Real physics ray must hit terrain geometry" );
        require( ( Vector3<real_Num>( point.x, point.y, point.z ) - expected ).length() < .003,
                 "Physics ray and authoritative exact triangle query agree" );
    }

    void run( SmartPtr<core::ApplicationManager> app,
              SmartPtr<physics::IPhysicsManager> manager )
    {
        auto terrain = make_ptr<scene::TerrainSystem>();
        render::TerrainData data;
        data.dimensions = { 2, 2 };
        data.spacing = { 4, 6 };
        data.origin = { -2, -3 };
        data.heights = { 0, 0, 0, 2 };  // Saddle distinguishes triangle and bilinear interpolation.
        String error;
        {
            auto unattached = make_ptr<scene::TerrainSystem>();
            auto pendingRuntime = make_ptr<render::Terrain>();
            pendingRuntime->setDeferredTerrainPublication( true );
            Transform3<real_Num> requestedPlacement;
            requestedPlacement.setPosition( { 5, 6, 7 } );
            requestedPlacement.setScale( { 2, 3, 4 } );
            pendingRuntime->setWorldTransform( requestedPlacement );
            unattached->setTerrain( pendingRuntime );
            require( pendingRuntime->getRequestedTerrainWorldTransform() == requestedPlacement,
                     "Attaching without an actor preserves the renderer's pending placement" );
            unattached->setTerrain( nullptr );
        }
        require( terrain->applyTerrainData( data, error ), "Initial authoritative terrain" );
        auto actor = make_ptr<scene::GameActor>();
        auto actorTransform = make_ptr<scene::Transform>();
        actor->setTransform( actorTransform );
        actor->addComponentInstance( terrain );
        auto collision = make_ptr<scene::CollisionTerrain>();
        collision->setEnabled( true );
        actor->addComponentInstance( collision );
        auto graphics = make_ptr<render::Terrain>();
        terrain->setTerrain( graphics );
        terrain->updateTransform();
        require( collision->getTerrainSnapshot() == terrain->getTerrainSnapshot(),
                 "Component collision publishes the authoritative immutable snapshot" );
        require( collision->getTerrainRevision() == graphics->getTerrainSnapshot()->revision,
                 "Query and collision revisions agree" );

        auto rigid = manager->addRigidStatic( Transform3<real_Num>() );
        require( rigid != nullptr, "Real WPPhysics static body" );
        rigid->addShape( collision->getShape() );
        auto nativeBody = rigid;
        require( nativeBody && nativeBody->getNumShapes() == 1, "Terrain body attachment" );
        NativeScene world( manager );
        world.owner->addActor( rigid );
        require( world.scene && world.owner->hasActor( rigid ), "Real terrain scene registration" );
        for( const auto &local : { Vector3<real_Num>( -2, 0, -3 ), Vector3<real_Num>( 2, 0, 3 ),
                                   Vector3<real_Num>( 0, 0, -3 ), Vector3<real_Num>( 0, 0, 0 ),
                                   Vector3<real_Num>( 1, 0, 1.5 ) } )
            checkRay( world.scene, *terrain->getTerrainSnapshot(), {},
                      local + Vector3<real_Num>( 0, 20, 0 ), local - Vector3<real_Num>( 0, 20, 0 ) );
        checkRay( world.scene, *terrain->getTerrainSnapshot(), {}, { -8, 1, 1.5 }, { 8, 1, 1.5 } );

        Transform3<real_Num> placement;
        placement.setPosition( { 10, 7, 20 } );
        placement.setScale( { 2, 3, 4 } );
        placement.setOrientation( { real_Num( std::cos( .2 ) ), 0, real_Num( std::sin( .2 ) ), 0 } );
        actorTransform->setWorldTransform( placement );
        terrain->updateTransform();
        require( collision->getPublishedTerrainTransform() == placement &&
                     graphics->getWorldTransform() == placement && nativeBody->getNumShapes() == 1,
                 "Nonuniform scale and yaw replace one attached shape at accepted placement" );
        for( const auto &local : { Vector3<real_Num>( 0, 0, 0 ), Vector3<real_Num>( 1, 0, 1.5 ),
                                   Vector3<real_Num>( -1, 0, -1.5 ) } )
        {
            const auto point = worldPoint( placement, local );
            checkRay( world.scene, *terrain->getTerrainSnapshot(), placement,
                      point + Vector3<real_Num>( 0, 30, 0 ), point - Vector3<real_Num>( 0, 30, 0 ) );
        }
        checkRay( world.scene, *terrain->getTerrainSnapshot(), placement,
                  worldPoint( placement, { -8, 1, 1.5 } ), worldPoint( placement, { 8, 1, 1.5 } ) );
        wp_vec3f hit{}, normal{};
        const auto outside = worldPoint( placement, { 2.1, 0, 0 } );
        require( !wp_physics_scene_intersects(
                     world.scene, nativePoint( outside + Vector3<real_Num>( 0, 30, 0 ) ),
                     nativePoint( outside - Vector3<real_Num>( 0, 30, 0 ) ), &hit, &normal, 0, 0 ),
                 "Physics ray outside finite terrain misses" );

        auto source = terrain->getTerrainSnapshot();
        auto shape = collision->getShape();
        auto nativeShape = dynamic_cast<physics::TerrainShape *>( shape.get() );
        require( nativeShape && nativeShape->getTerrainSnapshot() == source,
                 "Backend revision identity" );
        Probe probe;
        const auto ground = worldPoint( placement, { -1, 0, -1.5 } );
        wp_rigidbody_set_position( probe.body, nativePoint( ground + Vector3<real_Num>( 0, .15, 0 ) ) );
        wp_contact_manifold manifold{};
        require( wp_narrowphase_test_pair( probe.narrowphase, probe.body, probe.shape,
                                           nativeHandle<wp_rigidbody>( nativeBody.get() ), nativeHandle<wp_collision_shape>( nativeShape ), &manifold ) &&
                     manifold.contact_count > 0,
                 "Actual physics narrowphase contacts terrain from the same revision" );
        require( std::abs( manifold.contacts[0].position_world_on_b.y - ground.y ) < .003,
                 "Contact surface matches exact terrain plane height" );
        wp_rigidbody_set_position( probe.body, nativePoint( ground + Vector3<real_Num>( 0, 2, 0 ) ) );
        require( wp_physics_scene_add_actor( world.scene, probe.body ),
                 "Dynamic contact probe scene registration" );
        for( int step = 0; step < 240; ++step )
            wp_physics_scene_simulate( world.scene, 1.0f / 120.0f );
        const auto settled = wp_rigidbody_get_position( probe.body );
        require( std::abs( settled.y - ( ground.y + .2 ) ) < .035,
                 "Real physics integration settles the falling probe on terrain" );
        wp_physics_scene_remove_actor( world.scene, probe.body );

        auto invalid = std::make_shared<render::TerrainData>( *source );
        invalid->heights[0] = std::numeric_limits<f32>::quiet_NaN();
        auto invalidPayload = make_ptr<physics::TerrainCollisionData>();
        invalidPayload->terrain = invalid;
        invalidPayload->scale = placement.getScale();
        bool rejected = false;
        try
        {
            nativeShape->load( invalidPayload );
        }
        catch( const std::exception & )
        {
            rejected = true;
        }
        require( rejected && nativeShape->getTerrainSnapshot() == source && nativeShape->isValid(),
                 "Malformed backend reload retains last-good snapshot and native collision" );
        nativeShape->load( nullptr );
        require( nativeShape->getTerrainSnapshot() == source,
                 "Null reload preserves last-good geometry" );
        auto budgetPayload = make_ptr<physics::TerrainCollisionData>();
        budgetPayload->terrain = source;
        budgetPayload->maximumTriangles = 1;
        rejected = false;
        try
        {
            nativeShape->load( budgetPayload );
        }
        catch( const std::exception & )
        {
            rejected = true;
        }
        require( rejected && nativeShape->getTerrainSnapshot() == source,
                 "Solid triangles above the lowering-only collision budget preserve the old shape" );
        budgetPayload->maximumTriangles = 2;
        auto budgetShape = dynamic_pointer_cast<physics::TerrainShape>(
            manager->addCollisionShape<physics::ITerrainShape>( budgetPayload ) );
        require( budgetShape != nullptr, "Registered terrain shape accepts its bounded source" );
        require( budgetShape->isValid(), "Exact collision triangle budget boundary is accepted" );
        auto emptyBudgetSource = std::make_shared<render::TerrainData>( *source );
        emptyBudgetSource->holes = { 1 };
        budgetPayload->terrain = emptyBudgetSource;
        budgetPayload->maximumTriangles = 0;
        budgetShape->load( budgetPayload );
        require( budgetShape->getTerrainSnapshot() == emptyBudgetSource && budgetShape->isValid(),
                 "Collision budget counts solid triangles exactly and permits an all-hole source" );
        auto tooLarge = *source;
        tooLarge.dimensions = { 514, 513 };
        tooLarge.heights.assign( size_t( 514 ) * 513, 0 );
        require( !terrain->applyTerrainData( tooLarge, error ) && !error.empty() &&
                     terrain->getTerrainSnapshot() == source && collision->getShape() == shape &&
                     graphics->getActiveTerrainSnapshot() == source,
                 "Default collision budget rejects oversized terrain before mesh allocation and retains "
                 "all active consumers" );
        auto streaming = WorldStreamingScheduler::shared();
        const auto collisionClass = size_t( StreamMemoryClass::Collision );
        const auto sourceClass = size_t( StreamMemoryClass::SourceCpu );
        const auto admitted = streaming->stats();
        require( admitted.used[collisionClass] > 0 && admitted.used[sourceClass] > 0 &&
                     streaming->setBudget( StreamMemoryClass::Collision, admitted.used[collisionClass], error ),
                 "Live terrain collision participates in shared source and geometry budgets" );
        auto deniedEdit = *source;
        deniedEdit.heights[3] += .25f;
        const bool denied = !terrain->applyTerrainData( deniedEdit, error );
        require( streaming->setBudget( StreamMemoryClass::Collision, admitted.limits[collisionClass], error ),
                 "Restore collision budget after admission test" );
        require( denied && terrain->getTerrainSnapshot() == source && collision->getShape() == shape &&
                     graphics->getActiveTerrainSnapshot() == source &&
                     streaming->stats().used[collisionClass] == admitted.used[collisionClass] &&
                     streaming->stats().used[sourceClass] == admitted.used[sourceClass],
                 "Shared collision exhaustion rejects a candidate and retains complete active consumers without leaking source admission" );
        checkRay( world.scene, *source, placement, ground + Vector3<real_Num>( 0, 30, 0 ),
                  ground - Vector3<real_Num>( 0, 30, 0 ) );
        auto clone = dynamic_pointer_cast<physics::TerrainShape>( nativeShape->clone() );
        require( clone && clone->getTerrainSnapshot() == source &&
                     clone->getTerrainScale() == placement.getScale(),
                 "Clone retains source revision and baked scale" );
        require( streaming->stats().used[collisionClass] > admitted.used[collisionClass] &&
                     streaming->stats().used[sourceClass] > admitted.used[sourceClass],
                 "Independent cloned native terrain retains its own shared reservations" );
        clone->unload( nullptr );
        clone = nullptr;
        require( streaming->stats().used[collisionClass] == admitted.used[collisionClass] &&
                     streaming->stats().used[sourceClass] == admitted.used[sourceClass],
                 "Terrain clone unload releases native arrays and source before returning reservations" );
        const auto nativeMesh = wp_collision_shape_get_mesh_data( nativeHandle<wp_collision_shape>( nativeShape ) );
        const auto oldMesh = wp_collision_shape_get_triangle_mesh( nativeHandle<wp_collision_shape>( nativeShape ) );
        const wp_u32 badIndices[] = { 0, 1, nativeMesh->vertex_count };
        auto badMesh = *nativeMesh;
        badMesh.indices = badIndices;
        badMesh.triangle_count = 1;
        require( !wp_collision_shape_try_set_mesh_data( nativeHandle<wp_collision_shape>( nativeShape ), &badMesh ) &&
                     wp_collision_shape_get_triangle_mesh( nativeHandle<wp_collision_shape>( nativeShape ) ) == oldMesh,
                 "Native invalid-index replacement preserves acceleration structure" );

        auto invalidPlacement = placement;
        invalidPlacement.setOrientation(
            { real_Num( std::cos( .2 ) ), real_Num( std::sin( .2 ) ), 0, 0 } );
        actorTransform->setWorldTransform( invalidPlacement );
        terrain->updateTransform();
        require( collision->getShape() == shape &&
                     collision->getPublishedTerrainTransform() == placement &&
                     graphics->getWorldTransform() == placement,
                 "Unsupported pitch leaves render and collision placement unchanged" );
        actorTransform->setWorldTransform( placement );
        app->setPhysicsManager( nullptr );
        auto edit = *source;
        edit.heights[3] += 1;
        require( !terrain->applyTerrainData( edit, error ) && !error.empty() &&
                     terrain->getTerrainSnapshot() == source && collision->getShape() == shape &&
                     graphics->getTerrainSnapshot()->heights == source->heights,
                 "Unavailable backend prevents source/render/collision edit publication" );
        app->setPhysicsManager( manager );
        world.owner->lock();
        auto busyEdit = std::async( std::launch::async, [&] {
            String waiting;
            return terrain->applyTerrainData( edit, waiting, source->revision );
        } );
        const auto editReturned =
            busyEdit.wait_for( std::chrono::seconds( 1 ) ) == std::future_status::ready;
        world.owner->unlock();
        const auto editedWhileBusy = busyEdit.get();
        require( editReturned && !editedWhileBusy && terrain->getTerrainSnapshot() == source &&
                     collision->getShape() == shape,
                 "Busy physics manager rejects collision preparation without blocking or changing the "
                 "source" );
        require( terrain->applyTerrainData( edit, error, source->revision ) &&
                     terrain->getTerrainRevision() == collision->getTerrainRevision() &&
                     terrain->getTerrainRevision() == graphics->getTerrainSnapshot()->revision &&
                     collision->getShape() != shape && nativeBody->getNumShapes() == 1,
                 "Edit publishes exact shared revision and replaces attached native geometry" );

        graphics->setDeferredTerrainPublication( true );
        const auto beforeDeferred = terrain->getTerrainSnapshot();
        const auto beforeDeferredShape = collision->getShape();
        world.owner->lock();
        auto busyStroke = std::async( std::launch::async, [&] {
            String waiting;
            auto history = make_ptr<CommandManagerMT>();
            scene::TerrainBrush brush;
            brush.centre = {0, 0};
            brush.radius = 10;
            return terrain->beginEditStroke( history, waiting ) &&
                terrain->updateEditStroke( brush, waiting ) && terrain->cancelEditStroke( waiting );
        } );
        const auto strokeReturned = busyStroke.wait_for( std::chrono::seconds( 1 ) ) == std::future_status::ready;
        world.owner->unlock();
        const auto cancelledWhileBusy = busyStroke.get();
        require( strokeReturned && cancelledWhileBusy &&
                     terrain->getTerrainSnapshot()->heights == beforeDeferred->heights &&
                     graphics->getActiveTerrainSnapshot() == beforeDeferred && collision->getShape() == beforeDeferredShape,
                 "Deferred stroke cancellation restores requested samples while physics is busy and retains complete active consumers" );
        edit = *beforeDeferred;
        edit.heights[3] += 1;
        world.owner->lock();
        auto deferredEdit = std::async( std::launch::async, [&] {
            String waiting;
            return terrain->applyTerrainData( edit, waiting );
        } );
        const auto deferredReturned = deferredEdit.wait_for( std::chrono::seconds( 1 ) ) == std::future_status::ready;
        world.owner->unlock();
        require( deferredReturned && deferredEdit.get(), "Renderer queues a deferred edit while physics is busy" );
        const auto requested = terrain->getTerrainSnapshot();
        require( requested != beforeDeferred && graphics->getTerrainSnapshot() == requested &&
                     terrain->getActiveTerrainSnapshot() == beforeDeferred &&
                     graphics->getActiveTerrainSnapshot() == beforeDeferred &&
                     collision->getShape() == beforeDeferredShape,
                 "Pending render upload retains active query and collision revision" );
        bool committedRender = false;
        world.owner->lock();
        auto blocked = std::async( std::launch::async, [&] {
            String waiting;
            return graphics->publishPendingTerrain(
                requested, [&] { committedRender = true; }, waiting );
        } );
        const auto publishReturned =
            blocked.wait_for( std::chrono::seconds( 1 ) ) == std::future_status::ready;
        world.owner->unlock();
        const bool publishedWhileBusy = blocked.get();
        require( publishReturned && !publishedWhileBusy && !committedRender &&
                     collision->getShape() == beforeDeferredShape,
                 "Busy physics scene defers publication without waiting or discarding the prepared "
                 "generation" );
        require( graphics->publishPendingTerrain(
                     requested, [&] { committedRender = true; }, error ) &&
                     committedRender && collision->getTerrainSnapshot() == requested &&
                     graphics->getActiveTerrainSnapshot() == requested &&
                     terrain->getActiveTerrainSnapshot() == requested,
                 "Render readiness publishes source, queries and collision in one guarded boundary" );
        edit.heights[3] += 1;
        require( terrain->applyTerrainData( edit, error ), "First pending edit" );
        const auto superseded = terrain->getTerrainSnapshot();
        edit.heights[3] += 1;
        require( terrain->applyTerrainData( edit, error ), "Second pending edit" );
        committedRender = false;
        require( !graphics->publishPendingTerrain(
                     superseded, [&] { committedRender = true; }, error ) &&
                     !committedRender && collision->getTerrainSnapshot() == requested,
                 "Superseded render candidate cannot publish stale collision" );
        require( graphics->publishPendingTerrain( terrain->getTerrainSnapshot(), {}, error ),
                 "Latest staged candidate publishes" );
        graphics->setDeferredTerrainPublication( false );
        auto solid = terrain->getTerrainSnapshot();
        auto pending = std::make_shared<render::TerrainData>( *solid );
        pending->revision += 1;
        pending->heights[0] += 1;
        require( collision->prepareTerrain( pending, placement, error ),
                 "Collision can stage future revision" );
        collision->discardPreparedTerrain();
        require( collision->getTerrainSnapshot() == solid, "Discard leaves active collision unchanged" );
        edit = *solid;
        edit.holes = { 1 };
        require( terrain->applyTerrainData( edit, error ),
                 "All-hole terrain publishes valid empty collision" );
        require( collision->getShape()->isValid() &&
                     !wp_physics_scene_intersects(
                         world.scene, nativePoint( ground + Vector3<real_Num>( 0, 30, 0 ) ),
                         nativePoint( ground - Vector3<real_Num>( 0, 30, 0 ) ), &hit, &normal, 0, 0 ),
                 "Hole removes both native triangles, including accelerated ray query" );
        auto holeShape = collision->getShape();
        wp_rigidbody_set_position( probe.body, nativePoint( ground + Vector3<real_Num>( 0, .15, 0 ) ) );
        require( !wp_narrowphase_test_pair( probe.narrowphase, probe.body, probe.shape,
                                            nativeHandle<wp_rigidbody>( nativeBody.get() ), nativeHandle<wp_collision_shape>( holeShape.get() ), &manifold ),
                 "Hole removes contact support as well as ray geometry" );
        edit.holes.clear();
        require( terrain->applyTerrainData( edit, error ),
                 "Restoring terrain solid cells republishes collision" );
        world.owner->removeActor( rigid );
        manager->removePhysicsBody( rigid );
        collision->setLoadingState( LoadingState::Loaded );
        collision->unload( nullptr );
        require( !collision->getShape() && !collision->getTerrainSnapshot(),
                 "Unload releases active collision source" );
        actor->removeComponentInstance( collision );
        actor->removeComponentInstance( terrain );
        terrain->setTerrain( nullptr );
        {
            auto initialSource = make_ptr<scene::TerrainSystem>();
            require( initialSource->applyTerrainData( data, error ), "Initial deferred fixture source" );
            auto initialActor = make_ptr<scene::GameActor>();
            initialActor->setTransform( make_ptr<scene::Transform>() );
            initialActor->addComponentInstance( initialSource );
            auto initialCollision = make_ptr<scene::CollisionTerrain>();
            initialCollision->setEnabled( true );
            initialActor->addComponentInstance( initialCollision );
            auto bodyComponent = make_ptr<scene::Rigidbody>();
            auto waitingBody = manager->addRigidStatic( Transform3<real_Num>() );
            bodyComponent->setRigidStatic( waitingBody );
            initialActor->addComponentInstance( bodyComponent );
            auto initialGraphics = make_ptr<render::Terrain>();
            initialGraphics->setDeferredTerrainPublication( true );
            initialSource->setTerrain( initialGraphics );
            initialSource->updateTransform();
            NativeScene waitingScene( manager );
            waitingScene.owner->addActor( waitingBody );
            require( !initialCollision->getShape() && waitingBody->getNumShapes() == 0,
                     "Initial deferred collision remains pending until terrain rendering is ready" );
            waitingScene.owner->lock();
            auto busyInitial = std::async( std::launch::async, [&] {
                String waiting;
                return initialGraphics->publishPendingTerrain( initialSource->getTerrainSnapshot(), {},
                                                               waiting );
            } );
            const auto initialReturned =
                busyInitial.wait_for( std::chrono::seconds( 1 ) ) == std::future_status::ready;
            waitingScene.owner->unlock();
            const auto escapedInitialLock = busyInitial.get();
            require( initialReturned && !escapedInitialLock && !initialCollision->getShape(),
                     "Initial publication also observes the waiting body's physics scene lock" );
            Array<SmartPtr<physics::IPhysicsShape3>> fillerShapes;
            for( u32 i = 0; i < 16; ++i )
            {
                auto filler = manager->addCollisionShape<physics::IBoxShape3>( nullptr );
                require( filler != nullptr, "Full-body fixture shape allocation" );
                waitingBody->addShape( filler );
                fillerShapes.push_back( filler );
            }
            const auto priorActive = initialGraphics->getActiveTerrainSnapshot();
            require( waitingBody->getNumShapes() == 16 &&
                         !initialGraphics->publishPendingTerrain( initialSource->getTerrainSnapshot(),
                                                                  {}, error ) &&
                         !initialCollision->getShape() && waitingBody->getNumShapes() == 16 &&
                         initialGraphics->getActiveTerrainSnapshot() == priorActive,
                     "Full body rejects initial terrain publication without changing collision or "
                     "active rendering" );
            for( auto filler : fillerShapes )
            {
                waitingBody->removeShape( filler, false );
                manager->removeCollisionShape( filler );
            }
            require( initialGraphics->publishPendingTerrain( initialSource->getTerrainSnapshot(), {},
                                                             error ) &&
                         initialCollision->getShape() && waitingBody->getNumShapes() == 1 &&
                         initialCollision->getShape()->getActor() == waitingBody,
                     "First publication attaches its terrain shape to the pre-existing Rigidbody" );
            waitingScene.owner->removeActor( waitingBody );
            manager->removePhysicsBody( waitingBody );
            initialCollision->unload( nullptr );
            initialActor->removeComponentInstance( initialCollision );
            initialActor->removeComponentInstance( bodyComponent );
            initialActor->removeComponentInstance( initialSource );
            initialSource->setTerrain( nullptr );
        }
        std::puts(
            "Terrain collision source/revision, transformed rays, contacts, holes and lifecycle: PASS" );
    }
}  // namespace

int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto app = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( app );
    auto factories = make_ptr<FactoryManager>();
    factories->load( nullptr );
    app->setFactoryManager( factories );
    app->setStateManager( make_ptr<StateManager>() );
    app->setGameManager( make_ptr<scene::GameManager>() );
    auto timer = make_ptr<TimerMT>();
    timer->load( nullptr );
    app->setTimer( timer );
    auto plugin = make_ptr<physics::WPPhysics>();
    plugin->load( nullptr );
    // WPPhysicsManager3 currently inherits PhysicsManager's runtime type ID.
    // Resolve that exact loaded factory; the concrete C++ name is not registered
    // and a hint-based derived-type search can select an unloaded backend.
    auto physics = factories->make_object<physics::IPhysicsManager>( physics::PhysicsManager::typeInfo() );
    app->setPhysicsManager( physics );
    int result = 0;
    try
    {
        require( physics != nullptr, "Exported plugin registers the terrain physics manager factory" );
        run( app, physics );
    }
    catch( const std::exception &exception )
    {
        std::fprintf( stderr, "Terrain physics contracts failed: %s\n", exception.what() );
        result = 1;
    }
    if( physics ) physics->unload( nullptr );
    app->setPhysicsManager( nullptr );
    physics = nullptr;
    plugin->unload( nullptr );
    plugin = nullptr;
    app->setGameManager( nullptr );
    app->setStateManager( nullptr );
    app->setTimer( nullptr );
    timer->unload( nullptr );
    timer = nullptr;
    app->setFactoryManager( nullptr );
    factories->unload( nullptr );
    factories = nullptr;
    core::IApplicationManager::setInstance( nullptr );
    app = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    return result;
}
