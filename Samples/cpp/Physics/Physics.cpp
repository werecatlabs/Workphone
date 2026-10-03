#include "Physics.h"
#include <Workphone/Workphone.hpp>
#include <iomanip>
#include <sstream>

#ifdef _WP_STATIC_LIB_
#    include <FBOISInput/FBOISInput.hpp>
#    include <WPSQLite/WPSQLite.hpp>

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.h>
#    endif

#    if WP_BUILD_PHYSX
#        include <FBPhysx/FBPhysx.hpp>
#    elif WP_BUILD_ODE
#        include <FBODE3/CPhysicsManagerODE.hpp>
#    endif
#endif

namespace workphone
{
    namespace
    {
        constexpr size_t boxesPerAxis = 4;
        constexpr real_Num boxSpacing = static_cast<real_Num>( 1.1 );
        constexpr real_Num dropHeight = static_cast<real_Num>( 2.0 );
        constexpr hash_type debugTextId = 0x50485900;
        constexpr size_t debugTextLines = 11;

        Vector3<real_Num> getBoxPosition( size_t x, size_t y, size_t z )
        {
            const auto halfStackWidth =
                static_cast<real_Num>( boxesPerAxis - 1 ) * boxSpacing * static_cast<real_Num>( 0.5 );

            return Vector3<real_Num>(
                static_cast<real_Num>( x ) * boxSpacing - halfStackWidth,
                dropHeight + ( static_cast<real_Num>( y ) + static_cast<real_Num>( 0.5 ) ) * boxSpacing,
                static_cast<real_Num>( z ) * boxSpacing - halfStackWidth );
        }

        SmartPtr<scene::IGameActor> createBox( SmartPtr<scene::IGameManager> sceneManager,
                                             SmartPtr<render::IMaterial> material,
                                             const Vector3<real_Num> &position,
                                             bool isStatic = false,
                                             const Vector3<real_Num> &scale = Vector3<real_Num>::unit() )
        {
            auto box = sceneManager->createActor();
            WP_ASSERT( box );
            box->setName( "Box" );
            box->setStatic( isStatic );
            box->setPosition( position );
            box->setScale( scale );

            // Component creation must also run when WP_ASSERT is compiled out.
            auto collision = box->addComponent<scene::CollisionBox>();
            WP_ASSERT( collision );
            auto rigidbody = box->addComponent<scene::Rigidbody>();
            WP_ASSERT( rigidbody );
            rigidbody->setKinematic( false );
            rigidbody->setUseGravity( true );

            auto mesh = box->addComponent<scene::Mesh>();
            WP_ASSERT( mesh );
            mesh->setMeshPath( "cube_internal.fbmeshbin" );

            auto renderer = box->addComponent<scene::MeshRenderer>();
            WP_ASSERT( renderer );

            auto boxMaterial = box->addComponent<scene::Material>();
            WP_ASSERT( boxMaterial );
            boxMaterial->setMaterial( material );

            return box;
        }

    }  // namespace

    Physics::Physics()
    {
        setPluginsConfigFilePath( "wp_plugins_samples.cfg" );
    }

    Physics::~Physics()
    {
        unload( nullptr );
    }

    void Physics::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            WP_ASSERT( getLoadingState() == LoadingState::Allocated &&
                       "Physics must be unloaded before loading" );
            setLoadingState( LoadingState::Loading );

            const auto task = TaskId::Primary;
            Thread::setCurrentTask( task );

            const auto threadId = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( threadId );

            auto taskFlags = std::numeric_limits<u32>::max();
            Thread::setTaskFlags( taskFlags );

            auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
            WP_ASSERT( applicationManager && "Failed to create ApplicationManager" );

            applicationManager->load( data );
            WP_ASSERT( applicationManager && "ApplicationManager became null after load" );
            WP_ASSERT( applicationManager->isValid() && "ApplicationManager is invalid after load" );

            core::IApplicationManager::setInstance( applicationManager );
            WP_ASSERT( core::IApplicationManager::instance() == applicationManager &&
                       "Failed to set ApplicationManager instance" );
            m_applicationManager = applicationManager;

            applicationManager->setApplication( this );

            Application::load( data );

            WP_ASSERT( applicationManager->isValid() &&
                       "ApplicationManager invalid after Application::load" );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager && "GameManager is null" );
            WP_ASSERT( sceneManager->isValid() && "GameManager is invalid" );

            applicationManager->setPlaying( true );
            applicationManager->setPaused( false );
            sceneManager->play();

            setLoadingState( LoadingState::Loaded );

            WP_ASSERT( sceneManager->isValid() && "GameManager invalid after play" );
            WP_ASSERT( applicationManager->isValid() && "ApplicationManager invalid after play" );
            WP_ASSERT( getLoadingState() == LoadingState::Loaded && "Loading state not set correctly" );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void Physics::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded || loadingState == LoadingState::Unloading )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            if( auto applicationManager = core::IApplicationManager::instancePtr() )
            {
                if( auto graphics = applicationManager->getGraphicsSystem() )
                {
                    if( auto debug = graphics->getDebug() )
                    {
                        for( size_t i = 0; i < debugTextLines; ++i )
                        {
                            debug->drawText( debugTextId + i, Vector2<real_Num>::zero(), "", 0 );
                        }
                    }
                }
            }

            m_boxGround = nullptr;
            m_cameraActor = nullptr;
            m_boxes.clear();

            if( core::IApplicationManager::instance() )
            {
                Application::unload( data );
            }

            m_applicationEventListener = nullptr;
            m_applicationManager = nullptr;
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void Physics::update()
    {
        Application::update();

        const auto task = Thread::getCurrentTask();
        if( task == TaskId::Physics )
        {
            ++m_physicsUpdates;
        }
        if( task != TaskId::Render )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager ? applicationManager->getTimer() : nullptr;
        if( !timer || timer->getTime() < m_nextDebugUpdate )
        {
            return;
        }
        const auto now = timer->getTime();
        m_nextDebugUpdate = now + 0.25;

        auto physicsManager = applicationManager->getPhysicsManager();
        auto physicsScene = physicsManager ? physicsManager->getPhysicsScene() : nullptr;
        size_t bodies = 0, moving = 0, sleeping = 0;
        size_t belowGround = 0;
        auto lowestY = std::numeric_limits<real_Num>::max();
        auto highestY = std::numeric_limits<real_Num>::lowest();
        for( const auto &box : m_boxes )
        {
            if( auto rigidbody = box->getComponent<scene::Rigidbody>() )
            {
                if( rigidbody->hasRigidDynamic() )
                {
                    ++bodies;
                    const auto height = rigidbody->getRigidDynamic()->getTransform().getPosition().Y();
                    lowestY = std::min( lowestY, height );
                    highestY = std::max( highestY, height );
                    belowGround += !std::isfinite( height ) || height < 0.25f ? 1 : 0;
                    sleeping += rigidbody->isSleeping() ? 1 : 0;
                    moving += rigidbody->getLinearVelocity().lengthSquared() > 0.0001f ? 1 : 0;
                }
            }
        }

        if( m_smokeTest && ( belowGround > 0 || timer->getTimeSinceSceneLoad() >= 12.0 ) )
        {
            m_smokeTestPassed = belowGround == 0 && bodies == boxesPerAxis * boxesPerAxis * boxesPerAxis &&
                                lowestY >= 0.25f && highestY < 5.0f && m_physicsUpdates > 10;
            std::ostringstream result;
            result << "Physics smoke test " << ( m_smokeTestPassed ? "passed" : "FAILED" )
                   << ": bodies=" << bodies << " below ground=" << belowGround
                   << " lowest Y=" << lowestY << " highest Y=" << highestY;
            WP_LOG( String( result.str().c_str() ) );
            applicationManager->setQuit( true );
        }

        std::ostringstream text;
        text << std::fixed << std::setprecision( 2 );
        text << "Physics: " << ( applicationManager->isPlaying() ? "PLAYING" : "STOPPED" )
             << ( applicationManager->isPaused() ? " (paused)" : "" )
             << " | scene age " << timer->getTimeSinceSceneLoad() << " s\n";
        text << "Physics task ticks: " << m_physicsUpdates.load()
             << " | dt " << timer->getDeltaTime( TaskId::Physics ) * 1000.0 << " ms\n";
        text << "Scene: " << ( physicsScene ? "ready" : "MISSING" )
             << " | dynamic " << ( physicsScene ? physicsScene->numDynamicActors() : 0 )
             << " | static " << ( physicsScene ? physicsScene->numStaticActors() : 0 ) << '\n';
        text << "Boxes: " << m_boxes.size() << " | bodies " << bodies
             << " | missing " << m_boxes.size() - bodies << '\n';
        text << "Moving: " << moving << " | sleeping: " << sleeping
             << " | lowest Y: " << ( bodies ? lowestY : 0 )
             << " | below ground: " << belowGround << '\n';
        const auto gravity = physicsScene ? physicsScene->getGravity() : Vector3<real_Num>::zero();
        text << "Gravity: " << gravity.X() << ", " << gravity.Y() << ", " << gravity.Z() << '\n';
        if( !m_boxes.empty() )
        {
            const auto box = m_boxes.back();
            const auto rigidbody = box->getComponent<scene::Rigidbody>();
            const auto body = rigidbody ? rigidbody->getRigidDynamic() : nullptr;
            text << "Top box Y: actor " << box->getPosition().Y() << " | body ";
            if( body )
            {
                text << body->getTransform().getPosition().Y();
            }
            else
            {
                text << "MISSING";
            }
            text << '\n';
            const auto velocity = rigidbody ? rigidbody->getLinearVelocity() : Vector3<real_Num>::zero();
            text << "Velocity: " << velocity.X() << ", " << velocity.Y() << ", " << velocity.Z() << '\n';
            text << "Shapes: " << ( rigidbody ? rigidbody->getNumShapes() : 0 )
                 << " | gravity " << ( rigidbody && rigidbody->getUseGravity() ? "on" : "off" )
                 << " | kinematic " << ( rigidbody && rigidbody->isKinematic() ? "yes" : "no" );
        }
        if( m_boxGround )
        {
            auto rigidbody = m_boxGround->getComponent<scene::Rigidbody>();
            auto body = rigidbody ? rigidbody->getRigidStatic() : nullptr;
            text << "\nGround Y: actor " << m_boxGround->getPosition().Y() << " | body ";
            if( body )
            {
                text << body->getTransform().getPosition().Y();
            }
            else
            {
                text << "MISSING";
            }
            if( auto collision = m_boxGround->getComponent<scene::CollisionBox>() )
            {
                if( auto shape = workphone::dynamic_pointer_cast<physics::IBoxShape3>( collision->getShape() ) )
                {
                    const auto dimensions = shape->getExtents() * shape->getLocalPose().getScale();
                    text << "\nGround collider size: " << dimensions.X() << ", "
                         << dimensions.Y() << ", " << dimensions.Z();
                }
            }
        }

        // Use the renderer's debug text API, which also supports the Claw UI backend.
        if( auto graphics = applicationManager->getGraphicsSystem() )
        {
            if( auto debug = graphics->getDebug() )
            {
                std::istringstream lines( text.str() );
                std::string line;
                for( size_t i = 0; i < debugTextLines && std::getline( lines, line ); ++i )
                {
                    debug->drawText( debugTextId + i,
                                     Vector2<real_Num>( 0.02f, 0.14f + static_cast<f32>( i ) * 0.04f ),
                                     String( line.c_str() ), 0 );
                }
            }
        }

        if( now >= m_nextDebugLog )
        {
            WP_LOG( String( text.str().c_str() ) );
            m_nextDebugLog = now + 2.0;
        }
    }

    void Physics::createScene()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager = applicationManager->getPhysicsManager();
        WP_ASSERT( physicsManager );

        // Application::createPhysics normally creates the scene. Keep a fallback so the sample also
        // works with applications that provide a physics manager without a default scene.
        if( !physicsManager->getPhysicsScene() )
        {
            physicsManager->setPhysicsScene( physicsManager->addScene() );
        }
        WP_ASSERT( physicsManager->getPhysicsScene() );
        physicsManager->getPhysicsScene()->setGravity( Vector3<real_Num>( 0.0f, -9.81f, 0.0f ) );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        if( m_viewport )
        {
            m_viewport->setActive( true );
        }

        const auto cameraPosition = Vector3<real_Num>( 10.0f, 8.0f, 15.0f );
        const auto stackCentre = Vector3<real_Num>(
            0.0f, dropHeight + static_cast<real_Num>( boxesPerAxis ) * boxSpacing * 0.5f, 0.0f );
        WP_ASSERT( m_cameraSceneNode );
        m_cameraSceneNode->setPosition( cameraPosition );
        m_cameraSceneNode->lookAt( stackCentre );

        ApplicationUtil::createDefaultSky();

        if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
        {
            if( auto graphicsScene = graphicsSystem->getGraphicsScene() )
            {
                graphicsScene->setAmbientLight( ColourF::White * 0.25f );
            }
        }

        auto light = ApplicationUtil::createDirectionalLight();
        WP_ASSERT( light );
        light->setOrientation( QuaternionF::eulerDegrees( -60.0f, -30.0f, 0.0f ) );

        auto stackMaterial = ApplicationUtil::createDefaultMaterial();
        WP_ASSERT( stackMaterial );

        // Set the ground transform before its physics components are created. Its top is Y=0.
        m_boxGround = createBox( sceneManager, stackMaterial,
                                 Vector3<real_Num>( 0.0f, -0.5f, 0.0f ), true,
                                 Vector3<real_Num>( 500.0f, 1.0f, 500.0f ) );
        m_boxGround->setName( "Ground" );
        scene->addActor( m_boxGround );

        constexpr auto boxCount = boxesPerAxis * boxesPerAxis * boxesPerAxis;
        m_boxes.clear();
        m_boxes.reserve( boxCount );
        m_physicsUpdates = 0;
        m_nextDebugUpdate = 0.0;
        m_nextDebugLog = 0.0;

        for( size_t y = 0; y < boxesPerAxis; ++y )
        {
            for( size_t x = 0; x < boxesPerAxis; ++x )
            {
                for( size_t z = 0; z < boxesPerAxis; ++z )
                {
                    auto box = createBox( sceneManager, stackMaterial, getBoxPosition( x, y, z ) );
                    scene->addActor( box );
                    m_boxes.push_back( box );
                }
            }
        }

        WP_ASSERT( m_boxes.size() == boxCount );
    }

    void Physics::createPlugins()
    {
        Application::createPlugins();

#ifdef _WP_STATIC_LIB_
        auto applicationManager = core::ApplicationManager::instance();
        WP_ASSERT( applicationManager && "ApplicationManager is null during plugin creation" );
        WP_ASSERT( applicationManager->isValid() &&
                   "ApplicationManager is invalid during plugin creation" );

        auto corePlugin = workphone::make_ptr<WPCore>();
        WP_ASSERT( corePlugin && "Failed to create WPCore plugin" );
        applicationManager->addPlugin( corePlugin );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
        auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgreNext>();
        WP_ASSERT( graphicsPlugin && "Failed to create WPGraphicsOgreNext plugin" );
        applicationManager->addPlugin( graphicsPlugin );
#    elif WP_GRAPHICS_SYSTEM_OGRE
        auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgre>();
        WP_ASSERT( graphicsPlugin && "Failed to create WPGraphicsOgre plugin" );
        applicationManager->addPlugin( graphicsPlugin );
#    endif

#    if WP_BUILD_PHYSX
        auto physxPlugin = workphone::make_ptr<physics::FBPhysx>();
        WP_ASSERT( physxPlugin && "Failed to create FBPhysx plugin" );
        applicationManager->addPlugin( physxPlugin );
#    elif WP_BUILD_ODE
#    endif

        auto databasePlugin = workphone::make_ptr<SQLitePlugin>();
        WP_ASSERT( databasePlugin && "Failed to create SQLitePlugin" );
        applicationManager->addPlugin( databasePlugin );

        auto inputPlugin = workphone::make_ptr<OISInput>();
        WP_ASSERT( inputPlugin && "Failed to create OISInput plugin" );
        applicationManager->addPlugin( inputPlugin );
#endif
    }

}  // namespace workphone

int main( int argc, char *argv[] )
{
    using namespace workphone;

    TypeManager *typeManager = nullptr;
    bool ownsTypeManager = false;
    SmartPtr<Physics> app;
    bool smokeTest = false;
    String pluginsConfig;
    for( int i = 1; i < argc; ++i )
    {
        if( String( argv[i] ) == "--smoke-test" )
            smokeTest = true;
        else if( String( argv[i] ) == "--plugins" && i + 1 < argc )
            pluginsConfig = argv[++i];
    }
    int exitCode = 1;

    auto unloadApplication = [&app]() {
        if( app )
        {
            if( app->getLoadingState() != LoadingState::Unloaded )
            {
                app->unload( nullptr );
            }

            WP_ASSERT( app->getLoadingState() == LoadingState::Unloaded );
            app = nullptr;
        }

        core::IApplicationManager::setInstance( nullptr );
    };

    auto unloadTypeManager = [&typeManager, &ownsTypeManager]() {
        if( ownsTypeManager && typeManager )
        {
            typeManager->unload();
            delete typeManager;
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
            ownsTypeManager = false;
        }
    };

#if 1
    try
    {
        typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            WP_ASSERT( typeManager && "Failed to allocate TypeManager" );

            typeManager->load();
            WP_ASSERT( typeManager && "TypeManager became null after load" );

            TypeManager::setInstance( typeManager );
            WP_ASSERT( TypeManager::instance() == typeManager &&
                       "TypeManager instance not set correctly" );
            ownsTypeManager = true;
        }

        WP_ASSERT( TypeManager::instance() && "TypeManager instance is null before app creation" );

        app = workphone::make_ptr<Physics>();
        app->setSmokeTest( smokeTest );
        if( !pluginsConfig.empty() )
            app->setPluginsConfigFilePath( pluginsConfig );
        WP_ASSERT( app && "Failed to create Physics application" );
        WP_ASSERT( app->getLoadingState() == LoadingState::Allocated && "App should start unloaded" );

        const auto threads = Thread::hardware_concurrency();
        WP_ASSERT( threads > 0 && "Hardware concurrency returned 0" );
        //app.setActiveThreads( threads );
        app->setActiveThreads( 0 );

        app->load( nullptr );
        WP_ASSERT( app->getLoadingState() == LoadingState::Loaded && "App failed to load" );

        app->run();
        exitCode = smokeTest && !app->smokeTestPassed() ? 1 : 0;

        unloadApplication();
        WP_ASSERT( !app && "App should be null after unload" );

        unloadTypeManager();
        WP_ASSERT( !ownsTypeManager && "ownsTypeManager should be false after cleanup" );
    }
    catch( Exception &e )
    {
        unloadApplication();
        unloadTypeManager();

        WP_LOG_ERROR( e.what() );
        if( !smokeTest )
            MessageBoxUtil::show( e.what() );
    }
    catch( std::exception &e )
    {
        unloadApplication();
        unloadTypeManager();

        WP_LOG_ERROR( e.what() );
        if( !smokeTest )
            MessageBoxUtil::show( e.what() );
    }
    catch( ... )
    {
        unloadApplication();
        unloadTypeManager();

        WP_LOG_ERROR( "Unknown error" );
        if( !smokeTest )
            MessageBoxUtil::show( "Unknown error" );
    }
#else
    // Create application object
    Physics app;
    app.load( nullptr );
    app.run();
    app.unload( nullptr );
#endif

    return exitCode;
}
