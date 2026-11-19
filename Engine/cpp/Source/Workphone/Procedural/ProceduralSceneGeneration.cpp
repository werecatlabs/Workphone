#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::procedural_scene
{
    using namespace scene_build;

    bool activateGeneratedScene( const SceneAssets &assets, SmartPtr<scene::IGameActor> owner,
                                 String &error )
    {
        error.clear();
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        auto currentScene = manager ? manager->getCurrentScene() : nullptr;
        if( !owner || !currentScene )
        {
            error = "Generated scene activation has no owner/current scene.";
            return false;
        }
        if( currentScene->getSceneLoadingState() != scene::IGameScene::SceneLoadingState::Loaded ||
            !Thread::getTaskFlag( Thread::Application_Flag ) )
        {
            return false;
        }
        try
        {
            // Children are not scene roots: Lua reparents generated scenery and
            // the Sample attaches render children before adding its car actor.
            // Require a live scene-owned ancestor, rather than hierarchy-enabled
            // alone (which also accepts removed, still-loaded actors).
            const auto inCurrentHierarchy = [&]( SmartPtr<scene::IGameActor> candidate ) {
                bool anchored = false;
                size_t depth = 0;
                for( ; candidate && depth < 1024; candidate = candidate->getParent(), ++depth )
                {
                    if( !candidate->isLoaded() )
                    {
                        return false;
                    }
                    const auto candidateScene = candidate->getScene();
                    if( candidateScene && candidateScene != currentScene )
                    {
                        return false;
                    }
                    anchored |= candidateScene == currentScene &&
                                candidate->getFlag( scene::IGameActor::ActorFlagInScene );
                }
                return !candidate && anchored;
            };
            // Initial Sample construction and scene restoration can precede
            // attaching the owner. Leave readiness false until that boundary.
            if( !inCurrentHierarchy( owner ) )
            {
                return false;
            }
            SourceBudgetVector<SmartPtr<scene::IGameActor>> cohort;
            cohort.reserve( assets.actors.size() + 2 );
            // Persistent anchors live outside assets.actors and must also have
            // completed lifecycle activation after an explicit terrain rebind.
            const auto terrainActor = assets.roadTerrain ? assets.roadTerrain->getActor() : nullptr;
            const auto sourceActor = assets.roadAuthoring ? assets.roadAuthoring->getActor() : nullptr;
            if( terrainActor )
            {
                cohort.push_back( terrainActor );
            }
            if( sourceActor && sourceActor != terrainActor )
            {
                cohort.push_back( sourceActor );
            }
            for( const auto &generated : assets.actors )
            {
                if( generated && generated != terrainActor && generated != sourceActor )
                {
                    cohort.push_back( generated );
                }
            }
            for( const auto &generated : cohort )
            {
                if( !inCurrentHierarchy( generated ) )
                {
                    throw std::runtime_error( "Generated actor is detached from the current scene: " +
                                              generated->getName() );
                }
            }
            const auto actorState =
                app->isPlaying() ? scene::IGameActor::State::Play : scene::IGameActor::State::Edit;
            const auto componentState =
                app->isPlaying() ? scene::IComponent::State::Play : scene::IComponent::State::Edit;
            SourceBudgetVector<SmartPtr<scene::IGameActor>> ancestors;
            ancestors.reserve( 16 );
            // Resolve parent poses directly before a Rigidbody Enter event can
            // create native contacts. Do not pump unrelated scene updates.
            for( const auto &generated : cohort )
            {
                ancestors.clear();
                auto parent = generated;
                while( parent && ancestors.size() < 1024 )
                {
                    ancestors.push_back( parent );
                    parent = parent->getParent();
                }
                if( parent )
                {
                    throw std::runtime_error(
                        "Generated scene activation found an invalid hierarchy depth/cycle." );
                }
                for( auto it = ancestors.rbegin(); it != ancestors.rend(); ++it )
                {
                    if( auto transform = ( *it )->getTransform() )
                    {
                        transform->update();
                    }
                }
            }
            for( auto generated : cohort )
            {
                if( !generated->isLoaded() )
                {
                    throw std::runtime_error( "Generated actor is not loaded: " + generated->getName() );
                }
                generated->setState( actorState, false );
                // Same-state actors can still contain newly added components.
                for( auto component : generated->getComponents() )
                {
                    if( component && component->isLoaded() )
                    {
                        component->setState( componentState );
                    }
                }
                generated->updateTransform();
                if( generated->getState() != actorState )
                {
                    throw std::runtime_error( "Generated actor lifecycle activation is deferred: " +
                                              generated->getName() );
                }
            }
            auto physicsManager = app->getPhysicsManager();
            auto physics = physicsManager ? physicsManager->getPhysicsScene() : nullptr;
            if( !physics )
            {
                throw std::runtime_error(
                    "Generated scene activation requires a native physics scene." );
            }
            if( assets.terrainGameplaySource )
            {
                const auto terrain =
                    assets.ground ? assets.ground->getComponent<TerrainSystem>() : nullptr;
                const auto collision =
                    assets.ground ? assets.ground->getComponent<CollisionTerrain>() : nullptr;
                if( !terrain || !collision )
                {
                    throw std::runtime_error( "Terrain gameplay ground lost its native components." );
                }
                const auto renderer = dynamic_cast<render::Terrain *>( terrain->getTerrain().get() );
                const auto requested = terrain->getTerrainSnapshot();
                render::TerrainSnapshot active;
                Transform3<real_Num> placement;
                u64 serial = 0;
                bool detached = true;
                terrain->getTerrainIntegrationState( active, placement, serial, detached );
                if( !renderer || !requested || !serial || detached || active != requested ||
                    renderer->hasPendingTerrainPublication() ||
                    renderer->getTerrainSnapshot() != requested ||
                    renderer->getActiveTerrainSnapshot() != requested ||
                    collision->getTerrainSnapshot() != requested )
                {
                    // A newly generated deferred terrain has no native shape until
                    // the render transaction publishes all consumers together.
                    return false;
                }
            }
            for( const auto &generated : cohort )
            {
                if( generated->isEnabledInScene() )
                {
                    auto collision = generated->getComponent<scene::Collision>();
                    if( !collision || !collision->isEnabled() )
                    {
                        continue;
                    }
                    auto rigidbody = generated->getComponent<scene::Rigidbody>();
                    const bool trafficActor =
                        std::any_of( assets.cityTrafficActors.begin(), assets.cityTrafficActors.end(),
                                     [&]( const auto &entry ) { return entry.actor == generated; } );
                    if( trafficActor )
                    {
                        auto body = rigidbody ? rigidbody->getRigidDynamic() : nullptr;
                        auto controller = generated->getComponent<scene::FourWheelVehicle>();
                        auto native = controller ? controller->getVehicleController() : nullptr;
                        if( generated->isStatic() || !body || !collision->getShape() || !native ||
                            !collision->getShape()->isAttached() || body->getScene() != physics ||
                            body->getNumShapes() == 0 || rigidbody->getState() != componentState ||
                            collision->getState() != componentState ||
                            controller->getState() != componentState )
                        {
                            throw std::runtime_error(
                                "Generated traffic physics publication is incomplete: " +
                                generated->getName() );
                        }
                        const bool playing = componentState == scene::IComponent::State::Play;
                        if( rigidbody->isKinematic() == playing )
                        {
                            rigidbody->setKinematic( !playing );
                            if( playing )
                            {
                                body->wakeUp();
                            }
                        }
                        const auto wheels = generated->getComponentsInChildren<scene::WheelController>();
                        if( wheels.size() != 4 )
                        {
                            throw std::runtime_error( "Generated traffic actor needs four wheels: " +
                                                      generated->getName() );
                        }
                        for( u32 wheel = 0; wheel < wheels.size(); ++wheel )
                        {
                            auto nativeWheel = native->getWheelController( wheel );
                            if( !nativeWheel || wheels[wheel]->getWheelController() != nativeWheel )
                            {
                                throw std::runtime_error( "Generated traffic wheel is missing: " +
                                                          generated->getName() );
                            }
                        }
                        continue;
                    }
                    auto body = rigidbody ? rigidbody->getRigidStatic() : nullptr;
                    if( !generated->isStatic() || !rigidbody || !body || !collision->getShape() ||
                        !collision->getShape()->isAttached() || body->getScene() != physics ||
                        body->getNumShapes() == 0 || rigidbody->getState() != componentState ||
                        collision->getState() != componentState )
                    {
                        throw std::runtime_error(
                            "Generated static collision publication is incomplete: " +
                            generated->getName() );
                    }
                    const auto expected = generated->getWorldTransform(), actual = body->getTransform();
                    if( ( expected.getPosition() - actual.getPosition() ).length() > .001f ||
                        ( expected.getOrientation() * Vector3F::unitX() -
                          actual.getOrientation() * Vector3F::unitX() )
                                .length() > .001f )
                    {
                        throw std::runtime_error( "Generated static collision pose is stale: " +
                                                  generated->getName() );
                    }
                }
            }
            return true;
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return false;
        }
    }

    namespace
    {
        void buildFlatGround( SceneAssets &assets )
        {
            reserveOwnerSlot( assets.actors );
            auto ground = core::IApplicationManager::instance()->getGameManager()->createActor();
            if( !ground )
            {
                throw std::runtime_error( "Cannot create procedural collision actor." );
            }
            assets.actors.push_back( ground );
            assets.ground = ground;
            ground->setName( "Circuit collision plane" );
            ground->setStatic( true );
            ground->setPosition( { 0, -.5f, 0 } );
            ground->addComponent<CollisionBox>()->setExtents( { 1100, 1, 1100 } );
            ground->addComponent<Rigidbody>();
        }

        void attachGeneratedActors( SceneAssets &assets, SmartPtr<IGameActor> owner )
        {
            auto manager = core::IApplicationManager::instance()->getGameManager();
            auto scene = manager->getCurrentScene();
            reserveOwnerSlot( assets.actors );
            auto scenery = manager->createActor();
            if( !scenery )
            {
                throw std::runtime_error( "Cannot create procedural scenery group." );
            }
            assets.sceneryRoot = scenery;
            assets.actors.push_back( scenery );
            scenery->setName( "Procedural scenery" );
            scenery->setStatic( true );
            scenery->setFlag( IGameActor::ActorFlagDontSave, true );
            scenery->setScene( scene );

            // Build the hierarchy before entering Edit/Play. The world's poses
            // remain relative to this identity group, independent of the owner.
            for( auto actor : assets.actors )
            {
                if( actor == scenery )
                {
                    continue;
                }
                actor->setFlag( IGameActor::ActorFlagDontSave, true );
                if( !actor->getParent() )
                {
                    scenery->addChild( actor );
                }
                actor->setScene( scene );
                scene->registerAllUpdates( actor );
            }
            if( !owner->getScene() )
            {
                scenery->setScene( nullptr );
            }
            attachWorldActor( scene, owner, scenery );
        }

        void buildVehicleShadow( SceneAssets &assets )
        {
            TextureBuffer shade( 64, 64 );
            for( u32 y = 0; y < 64; ++y )
            {
                for( u32 x = 0; x < 64; ++x )
                {
                    auto pixel = shade.pixel( x, y );
                    const auto dx = ( x - 31.5f ) / 31.5f, dz = ( y - 31.5f ) / 31.5f;
                    pixel[0] = pixel[1] = pixel[2] = 0;
                    pixel[3] = u8( 130 * std::pow( std::max( 0.f, 1.f - dx * dx - dz * dz ), 2 ) );
                }
            }
            auto shadowMaterial = material( assets, ColourF::White, 1 );
            shadowMaterial->setTexture( upload( assets, shade ), 0 );
            shadowMaterial->setTransparent( true );
            shadowMaterial->setBlendMode( 1 );
            shadowMaterial->setDepthWrite( false );
            Geometry shadowGeometry;
            shadowGeometry.quad( { -1.6f, .025f, -3.7f }, { 1.6f, .025f, -3.7f }, { 1.6f, .025f, 3.7f },
                                 { -1.6f, .025f, 3.7f }, { 0, 1, 0 } );
            assets.shadow = mesh( assets, "Vehicle contact shadow", shadowGeometry, shadowMaterial );
        }
    }  // namespace

    void buildCircuitScene( SceneAssets &assets, SmartPtr<IGameActor> owner, u32 seed,
                            VehicleAppearanceQuality quality )
    {
        assets.resourcePrefix = "__procedural/circuit/" + StringUtil::getUUID();
        if( assets.terrainGameplaySource )
        {
            buildTerrainGameplayScene( assets );
        }
        else
        {
            buildFlatGround( assets );
            buildTrack( assets, seed, quality );
        }
        attachGeneratedActors( assets, owner );
    }

    void buildCityScene( SceneAssets &assets, SmartPtr<IGameActor> owner, u32 seed,
                         VehicleAppearanceQuality quality )
    {
        assets.resourcePrefix = "__procedural/city/" + StringUtil::getUUID();
        assets.openCity = true;
        if( !assets.cityTerrainRoads )
        {
            buildFlatGround( assets );
        }
        buildCity( assets, owner, nullptr, seed, quality );
        attachGeneratedActors( assets, owner );
    }

    void buildScene( SceneAssets &assets, SmartPtr<IGameActor> actor, u32 seed,
                     VehicleAppearanceQuality quality, SmartPtr<IGameActor> citySourceOwner )
    {
        assets.resourcePrefix = "__procedural/race/" + StringUtil::getUUID();
        const auto started = std::chrono::steady_clock::now();
        if( !assets.ground && !assets.cityRoadShowcase && !assets.cityTerrainRoads &&
            !assets.terrainGameplaySource )
        {
            buildFlatGround( assets );
        }
        buildCar( assets, actor, seed, quality );
        if( assets.terrainGameplaySource )
        {
            buildTerrainGameplayScene( assets );
        }
        else if( assets.cityRoadShowcase )
        {
            buildRoadAuthoringShowcase( assets, actor, seed, quality );
        }
        else if( assets.openCity )
        {
            buildCity( assets, citySourceOwner ? citySourceOwner : actor, actor, seed, quality );
        }
        else
        {
            buildTrack( assets, seed, quality );
        }
        buildSky( assets, actor, seed );
        buildVehicleShadow( assets );
        attachGeneratedActors( assets, actor );

        String activationError;
        if( !activateGeneratedScene( assets, actor, activationError ) && !activationError.empty() )
        {
            throw std::runtime_error( activationError.c_str() );
        }
        const auto generationMs =
            std::chrono::duration<double, std::milli>( std::chrono::steady_clock::now() - started )
                .count();
        WP_LOG( String( "VehicleAdvanced generation milliseconds=" ) +
                StringUtil::toString( generationMs ) );
        WP_LOG( String( "VehicleAdvanced generated: seed=" ) + StringUtil::toString( seed ) +
                " hash=" + StringUtil::toString( assets.vehicle.contentHash ) +
                " triangles=" + StringUtil::toString( assets.triangles ) +
                " texture bytes=" + StringUtil::toString( assets.textureBytes ) +
                " circuit metres=" + StringUtil::toString( assets.circuit.length ) );
    }

    void clearScene( SceneAssets &assets, SmartPtr<IGameActor> owner )
    {
        SceneAssets retired;
        assets.swap( retired );
        // Release producer storage before retirement callbacks need new allocations.
        retired.vehicle = {};
        retired.circuit = {};
        retired.cityLayout = {};
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;

        if( retired.vehicleLOD )
        {
            if( auto system =
                    dynamic_pointer_cast<LODSystem>( retired.vehicleLOD->getComponentSystem() ) )
            {
                system->clearViewOverride();
            }
            if( owner )
            {
                owner->removeComponentInstance( retired.vehicleLOD );
            }
        }

        if( manager )
        {
            if( retired.sceneryRoot )
            {
                try
                {
                    retired.sceneryRoot->removeChildren();
                }
                catch( const std::bad_alloc & )
                {
                    // Retire actors individually if the bulk snapshot cannot be admitted.
                }
            }
            for( auto it = retired.actors.rbegin(); it != retired.actors.rend(); ++it )
            {
                if( *it == retired.sceneryRoot )
                {
                    continue;
                }
                try
                {
                    manager->destroyActor( *it );
                }
                catch( const std::bad_alloc & )
                {
                }
            }
            if( retired.sceneryRoot )
            {
                try
                {
                    manager->destroyActor( retired.sceneryRoot );
                }
                catch( const std::bad_alloc & )
                {
                }
            }
        }

        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        if( graphics )
        {
            auto scene = graphics->getGraphicsScene();
            if( retired.foliageTerrain )
            {
                String error;
                retired.foliageTerrain->setFoliageSnapshot( {}, error );
                if( scene && ( !retired.cityTerrainRoads || !retired.roadTerrain ||
                               retired.roadTerrain->getTerrain() != retired.foliageTerrain ) )
                {
                    scene->removeGraphicsObject( retired.foliageTerrain );
                }
            }

            if( scene && retired.sky )
            {
                scene->removeGraphicsObject( retired.sky );
            }

            if( scene && retired.sun )
            {
                scene->removeGraphicsObject( retired.sun );
            }

            auto materials = graphics->getMaterialManager();
            if( materials )
            {
                for( auto material : retired.materials )
                {
                    try
                    {
                        materials->destroyResource( material );
                    }
                    catch( const std::bad_alloc & )
                    {
                    }
                }
            }

            auto textures = graphics->getTextureManager();
            if( textures )
            {
                for( auto texture : retired.textures )
                {
                    // TextureManager detaches the resource before notifying it.
                    try
                    {
                        textures->destroyResource( texture );
                    }
                    catch( const std::bad_alloc & )
                    {
                    }
                }
            }
        }

        auto meshes = app ? dynamic_pointer_cast<MeshManager>( app->getMeshManager() ) : nullptr;
        if( meshes )
        {
            for( auto mesh : retired.meshes )
            {
                try
                {
                    meshes->removeMeshResource( mesh );
                }
                catch( const std::bad_alloc & )
                {
                }
            }
        }
    }

    bool validateReflection( const SceneAssets &assets )
    {
        if( !assets.reflectionTexture )
        {
            return true;
        }
        auto component =
            assets.reflectionActor ? assets.reflectionActor->getComponent<scene::Cubemap>() : nullptr;
        auto probe = component ? component->getRenderCubemap() : nullptr;
        if( !component || !component->isEnabled() || !probe ||
            probe->getTexture() != assets.reflectionTexture || assets.vehicleMaterials.empty() )
        {
            return false;
        }
        const auto slot = static_cast<u32>( PbsTextureTypes::PBSM_REFLECTION );
        for( const auto &material : assets.vehicleMaterials )
        {
            if( material->getTexture( slot ) != assets.reflectionTexture )
            {
                return false;
            }
        }
        void *native = nullptr;
        assets.reflectionTexture->getTextureFinal( &native );
        return native != nullptr;
    }

    void configurePhysics( const SceneAssets &assets, SmartPtr<scene::IGameActor> actor )
    {
        const auto &config = assets.vehicle.physics;
        auto car = actor->getComponent<scene::FourWheelVehicle>();
        auto vehicle = car->getVehicleController();
        auto body = actor->getComponent<scene::Rigidbody>();
        auto com = vector( config.massProperties.centreOfMass );
        const bool hasHandlingPreset = car->hasHandlingPreset();
        if( !hasHandlingPreset )
        {
            car->setMass( float( config.massProperties.massKg ) );
            vehicle->setMass( float( config.massProperties.massKg ) );
            body->setMass( float( config.massProperties.massKg ) );
            car->setDriveType( VehicleDriveType::RearWheelDrive );
            // Brush tyre forces use negative steering yaw. Adapt the rendered wheels.
            auto visualProperties = car->getProperties();
            visualProperties->setProperty( "Visual Steering Sign", -1.f );
            car->setProperties( visualProperties );
        }
        car->setMOI( { float( config.massProperties.inertia.xx ),
                       float( config.massProperties.inertia.yy ),
                       float( config.massProperties.inertia.zz ) } );
        auto controllerProperties = vehicle->getProperties();
        controllerProperties->setProperty( "Keyboard Input", false );
        controllerProperties->setProperty( "Steering Rate", 90.f );
        controllerProperties->setProperty( "Steering Wheelbase", float( config.wheelbaseM ) );
        controllerProperties->setProperty( "Steering Acceleration", 14.f );
        controllerProperties->setProperty( "Play Steering Scale",
                                           float( config.wheels[0].maxSteerRad * 180.0 / pi ) );
        vehicle->setProperties( controllerProperties );
        if( auto drive = vehicle->getDriveTrain() )
        {
            auto props = drive->getProperties();
            String ratios = "0,-2.5";
            for( auto ratio : config.drivetrain.forwardGearRatios )
            {
                ratios += "," + StringUtil::toString( ratio );
            }
            props->setProperty( "Gear Ratios", ratios );
            props->setProperty( "Final Drive Ratio", float( config.drivetrain.finalDriveRatio ) );
            props->setProperty( "Min RPM", float( config.drivetrain.idleRpm ) );
            props->setProperty( "Max RPM", float( config.drivetrain.redlineRpm ) );
            props->setProperty( "Max Torque", float( config.drivetrain.peakTorqueNm ) );
            props->setProperty( "Max Power", float( config.drivetrain.peakPowerW ) );
            props->setProperty( "Torque RPM", float( config.drivetrain.redlineRpm * .6 ) );
            props->setProperty( "Power RPM", float( config.drivetrain.redlineRpm * .85 ) );
            drive->setProperties( props );
        }
        // The chassis is centred on the computed COM. Wheel hubs and all render vertices use
        // the same origin. Keep ray mounts clear of the narrow monocoque collision box.
        actor->getComponent<scene::CollisionBox>()->setExtents( { .9f, .24f, 3.1f } );
        for( u32 i = 0; i < 4; ++i )
        {
            const auto &w = config.wheels[i];
            auto wheel = vehicle->getWheelController( i );
            auto p = vector( w.hubPosition ) - com;
            wheel->setLocalTransform( Transform3<real_Num>( p, QuaternionF::identity() ) );
            wheel->setRadius( float( w.tire.radiusM ) );
            wheel->setMass( float( config.massProperties.massKg *
                                   ( i < 2 ? config.massProperties.frontStaticWeightFraction
                                           : 1 - config.massProperties.frontStaticWeightFraction ) *
                                   .5 ) );
            wheel->setSpringRate( float( w.suspension.springRateNPerM * w.suspension.motionRatio *
                                         w.suspension.motionRatio ) );
            wheel->setDamping( float( w.suspension.damperCompressionNPerMps * w.suspension.motionRatio *
                                      w.suspension.motionRatio ) );
            wheel->setSuspensionTravel(
                float( w.suspension.bumpTravelM + w.suspension.reboundTravelM ) );
            wheel->setSuspensionDistance(
                float( w.suspension.bumpTravelM + w.suspension.reboundTravelM ) );
            wheel->setSteeringWheel( w.steerable );
            wheel->setPoweredWheel( w.driven );
            auto wheelProps = wheel->getProperties();
            wheelProps->setProperty( "Stable Contacts", true );
            wheelProps->setProperty( "Traction Control", true );
            wheelProps->setProperty( "Anti Lock Brakes", true );
            const auto &inertia = config.massProperties.inertia;
            const auto effectiveMass = 1.0 / ( 1.0 / config.massProperties.massKg +
                                               p.x * p.x / inertia.zz + p.z * p.z / inertia.xx );
            wheelProps->setProperty( "Contact Effective Mass", float( effectiveMass ) );
            wheelProps->setProperty( "Inertia", float( w.tire.wheelInertiaKgM2 ) );
            wheelProps->setProperty( "Brake Friction Torque", float( w.brakeTorqueNm ) );
            // Keep startup traction consistent with the assisted Grand Prix setup
            // and ProceduralRaceScene::updateSurfaceGrip's on-road multiplier.
            wheelProps->setProperty( "Grip", 1.25f );
            // A small rear reserve makes breakaway progressive under power.
            const auto axleGrip = i < 2 ? 1.0 : 1.06;
            wheelProps->setProperty(
                "Static Friction Coefficient",
                float( std::min( w.tire.peakLongitudinalFriction, w.tire.peakLateralFriction ) *
                       axleGrip ) );
            wheelProps->setProperty(
                "Sliding Friction Coefficient",
                float( std::min( w.tire.peakLongitudinalFriction, w.tire.peakLateralFriction ) *
                       axleGrip * .94 ) );
            wheelProps->setProperty( "Longitudinal Stiffness",
                                     float( w.tire.longitudinalStiffnessNPerSlip ) );
            wheelProps->setProperty( "Lateral Stiffness", float( w.tire.corneringStiffnessNPerRad *
                                                                 ( i < 2 ? 1.0 : 1.10 ) ) );
            wheel->setProperties( wheelProps );
            auto wheelActor = assets.wheels[i];
            wheelActor->setLocalPosition( p );
        }
        car->refreshHandlingPreset();
    }
}  // namespace workphone::scene::race
