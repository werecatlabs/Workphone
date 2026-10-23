#include <EditorPCH.hpp>
#include <editor/ProjectSettingsRuntime.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/Workphone.hpp>

#if WP_EDITOR_TESTS
#    include <boost/test/unit_test.hpp>
#    include <Workphone/System/ApplicationManager.hpp>
#    include <Workphone/System/TaskManager.hpp>
#    include <Workphone/System/Task.hpp>
#    include <Workphone/Sound/SoundManager.hpp>
#    include <Workphone/Physics/PhysicsManager.hpp>
#    include <Workphone/Physics/PhysicsScene3.hpp>
#    include <Workphone/Scene/Directors/GraphicsSettingsDirector.hpp>

using namespace workphone;
using namespace workphone::editor;

namespace
{
    // These probes exercise the public subsystem contracts without starting threads,
    // opening a graphics device, or depending on an installed audio output device.
    class SettingsTaskProbe : public Task
    {
    public:
        f64 getTargetFPS() const override { return rate; }
        void setTargetFPS( f64 value ) override { rate = value; }
        f64 rate = 60;
    };

    class SettingsTasksProbe : public TaskManager
    {
    public:
        SmartPtr<ITask> getTask( TaskId id ) const override
        {
            switch( id )
            {
            case TaskId::Primary: return primary;
            case TaskId::Application: return application;
            case TaskId::Render: return render;
            case TaskId::Physics: return physics;
            default: return nullptr;
            }
        }
        SmartPtr<SettingsTaskProbe> primary = workphone::make_ptr<SettingsTaskProbe>();
        SmartPtr<SettingsTaskProbe> application = workphone::make_ptr<SettingsTaskProbe>();
        SmartPtr<SettingsTaskProbe> render = workphone::make_ptr<SettingsTaskProbe>();
        SmartPtr<SettingsTaskProbe> physics = workphone::make_ptr<SettingsTaskProbe>();
    };

    class SettingsSoundProbe : public SoundManager
    {
    public:
        void setVolume( f32 value ) override { volume = value; }
        f32 getVolume() const override { return volume; }
        f32 volume = 1;
    };

    class SettingsPhysicsSceneProbe : public physics::PhysicsScene3
    {
    public:
        void setGravity( const Vector3<real_Num> &value ) override { gravity = value; }
        Vector3<real_Num> getGravity() const override { return gravity; }
        Vector3<real_Num> gravity = Vector3<real_Num>( 2, -9.8f, 3 );
    };

    class SettingsPhysicsProbe : public physics::PhysicsManager
    {
    public:
        SmartPtr<physics::IPhysicsShape3> addCollisionShapeByType(
            hash64, SmartPtr<ISharedObject> ) override
        {
            return nullptr;
        }
    };

    struct SettingsRuntimeGuard
    {
        SmartPtr<core::IApplicationManager> previousApp = core::IApplicationManager::instance();
        SmartPtr<EditorManager> previousEditor = EditorManager::getSingleton();
        SmartPtr<core::ApplicationManager> app = workphone::make_ptr<core::ApplicationManager>();

        SettingsRuntimeGuard()
        {
            EditorManager::setSingletonPtr( nullptr );
            core::IApplicationManager::setInstance( app );
        }

        ~SettingsRuntimeGuard()
        {
            app->setTaskManager( nullptr );
            app->setSoundManager( nullptr );
            app->setPhysicsManager( nullptr );
            EditorManager::setSingletonPtr( previousEditor );
            core::IApplicationManager::setInstance( previousApp );
        }
    };
}

BOOST_AUTO_TEST_CASE( project_settings_runtime_subsystem_contract_test )
{
    SettingsRuntimeGuard guard;
    auto tasks = workphone::make_ptr<SettingsTasksProbe>();
    auto sound = workphone::make_ptr<SettingsSoundProbe>();
    auto physics = workphone::make_ptr<SettingsPhysicsProbe>();
    auto scene = workphone::make_ptr<SettingsPhysicsSceneProbe>();
    physics->setPhysicsScene( scene );
    guard.app->setTaskManager( tasks );
    guard.app->setSoundManager( sound );
    guard.app->setPhysicsManager( physics );

    auto settings = workphone::make_ptr<Properties>();
    settings->setProperty( "graphics.frameRateLimit", 144 );
    settings->setProperty( "physics.simulationRate", 120 );
    settings->setProperty( "physics.gravity", -12.5f );
    settings->setProperty( "physics.debugDraw", true );
    settings->setProperty( "sound.masterVolume", 25 );
    auto result = applyProjectSettingsRuntime( settings );
    BOOST_REQUIRE_MESSAGE( result->getPropertyAsBool( "success" ), result->getProperty( "status" ) );
    BOOST_CHECK_EQUAL( tasks->primary->rate, 144 );
    BOOST_CHECK_EQUAL( tasks->application->rate, 144 );
    BOOST_CHECK_EQUAL( tasks->render->rate, 144 );
    BOOST_CHECK_EQUAL( tasks->physics->rate, 120 );
    BOOST_CHECK_CLOSE( sound->volume, 0.25f, 0.001f );
    BOOST_CHECK_CLOSE( scene->gravity.Y(), -12.5f, 0.001f );
    BOOST_CHECK_EQUAL( scene->gravity.X(), 2 );
    BOOST_CHECK_EQUAL( scene->gravity.Z(), 3 );
    BOOST_CHECK( physics->getEnableDebugDraw() );
    BOOST_CHECK( result->getProperty( "unavailableKeys" ).empty() );

    // A bad renderer choice must reject the complete draft before any subsystem changes.
    settings->setProperty( "graphics.renderer.pc", 2 );
    settings->setProperty( "sound.masterVolume", 80 );
    settings->setProperty( "physics.gravity", -2.0f );
    settings->setProperty( "graphics.frameRateLimit", 30 );
    result = applyProjectSettingsRuntime( settings );
    BOOST_CHECK( !result->getPropertyAsBool( "success" ) );
    BOOST_CHECK_CLOSE( sound->volume, 0.25f, 0.001f );
    BOOST_CHECK_CLOSE( scene->gravity.Y(), -12.5f, 0.001f );
    BOOST_CHECK_EQUAL( tasks->primary->rate, 144 );
}

BOOST_AUTO_TEST_CASE( project_settings_runtime_graphics_director_test )
{
    auto director = workphone::make_ptr<scene::GraphicsSettingsDirector>();
    auto initial = workphone::make_ptr<Properties>();
    initial->setProperty( "Render API", 0 );
    director->setProperties( initial );

    auto settings = workphone::make_ptr<Properties>();
    settings->setProperty( "graphics.renderer.pc", 0 );
    settings->setProperty( "quality.preset", 0 );
    settings->setProperty( "graphics.antiAliasing", 2 );
    settings->setProperty( "graphics.shadowQuality", 0 );
    settings->setProperty( "graphics.shadowResolution", 3 );
    settings->setProperty( "graphics.postProcessing", true );
    settings->setProperty( "graphics.bloom", true );
    settings->setProperty( "graphics.motionBlur", true );
    settings->setProperty( "graphics.ambientOcclusion", true );
    updateProjectGraphicsSettings( settings, director );
    auto properties = director->getProperties();
    BOOST_CHECK_EQUAL( properties->getPropertyAsInt( "Render API" ), 0 );
    BOOST_CHECK_EQUAL( properties->getPropertyAsInt( "Quality" ), 0 );
    BOOST_CHECK_EQUAL( properties->getPropertyAsInt( "Shadow map size" ), 4096 );
    BOOST_CHECK( properties->getPropertyAsBool( "Temporal anti-aliasing" ) );
    BOOST_CHECK( !properties->getPropertyAsBool( "Cascaded shadows" ) );
    BOOST_CHECK( properties->getPropertyAsBool( "Bloom" ) );
    BOOST_CHECK( properties->getPropertyAsBool( "Motion blur" ) );
    BOOST_CHECK( properties->getPropertyAsBool( "Ambient occlusion" ) );

    // Custom is not the engine's fifth (Cinematic) quality preset.
    settings->setProperty( "quality.preset", 4 );
    settings->setProperty( "graphics.renderer.pc", 1 );
    settings->setProperty( "graphics.postProcessing", false );
    updateProjectGraphicsSettings( settings, director );
    properties = director->getProperties();
    BOOST_CHECK_EQUAL( properties->getPropertyAsInt( "Render API" ), 1 );
    BOOST_CHECK_EQUAL( properties->getPropertyAsInt( "Quality" ), 0 );
    BOOST_CHECK( !properties->getPropertyAsBool( "Bloom" ) );
    BOOST_CHECK( !properties->getPropertyAsBool( "Motion blur" ) );
    BOOST_CHECK( !properties->getPropertyAsBool( "Ambient occlusion" ) );

    settings->setProperty( "graphics.renderer.pc", 5 );
    updateProjectGraphicsSettings( settings, director );
    properties = director->getProperties();
    BOOST_CHECK_EQUAL( properties->getPropertyAsInt( "Render API" ), 0 );

    // These are exactly the fields serialized to GameGraphics.settings and loaded by Application.
    auto restoredProperties = workphone::make_ptr<Properties>();
    DataUtil::parse( DataUtil::toString( properties.get(), true ), restoredProperties.get() );
    auto restored = workphone::make_ptr<scene::GraphicsSettingsDirector>();
    restored->setProperties( restoredProperties );
    restored->setProperties( restoredProperties );
    BOOST_CHECK_EQUAL( restored->getProperties()->getPropertyAsInt( "Shadow map size" ), 4096 );
    BOOST_CHECK_EQUAL( restored->getProperties()->getPropertyAsInt( "Quality" ), 0 );
    BOOST_CHECK( !restored->getProperties()->getPropertyAsBool( "Bloom" ) );
}

BOOST_AUTO_TEST_CASE( project_settings_runtime_validation_and_capabilities_test )
{
    SettingsRuntimeGuard guard;
    auto settings = workphone::make_ptr<Properties>();
    for( const char *value : { "nan", "inf", "-inf", "-12garbage", "" } )
    {
        settings->setProperty( "physics.gravity", String( value ) );
        BOOST_CHECK( !validateProjectSettingsRuntime( settings )->getPropertyAsBool( "success" ) );
    }
    settings->setProperty( "physics.gravity", -9.8f );
    settings->setProperty( "graphics.antiAliasing", 1 );
    BOOST_CHECK( !validateProjectSettingsRuntime( settings )->getPropertyAsBool( "success" ) );
    settings->setProperty( "graphics.antiAliasing", 2 );
    BOOST_CHECK( validateProjectSettingsRuntime( settings )->getPropertyAsBool( "success" ) );

    auto capabilities = workphone::make_ptr<Properties>();
    describeProjectSettingsCapabilities( capabilities );
    BOOST_CHECK( capabilities->getPropertyAsBool( "capability.sound.masterVolume" ) );
    BOOST_CHECK( capabilities->getPropertyAsBool( "capability.graphics.shadowResolution" ) );
    BOOST_CHECK( !capabilities->hasProperty( "capability.physics.threads" ) );
    BOOST_CHECK( !capabilities->getProperty( "unavailable.physics.threads" ).empty() );
    BOOST_CHECK( !capabilities->hasProperty( "capability.graphics.shadowDistance" ) );

    core::IApplicationManager::setInstance( nullptr );
    const auto result = applyProjectSettingsRuntime( settings );
    BOOST_CHECK( result->getPropertyAsBool( "success" ) );
    BOOST_CHECK_EQUAL( result->getProperty( "status" ),
                       "Project settings stored; runtime is not loaded." );
}

#endif
