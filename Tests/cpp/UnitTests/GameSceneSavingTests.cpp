#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Scene/Components/CollisionBox.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    constexpr auto TestTolerance = static_cast<real_Num>( 0.001 );

    void requireVectorClose( const Vector3<real_Num> &actual, const Vector3<real_Num> &expected,
                             const String &context )
    {
        BOOST_CHECK_MESSAGE( MathUtil<real_Num>::equals( actual, expected, TestTolerance ),
                             context << " expected (" << expected.X() << ", " << expected.Y() << ", "
                                     << expected.Z() << ") but got (" << actual.X() << ", " << actual.Y()
                                     << ", " << actual.Z() << ")" );
    }

    void requireQuaternionClose( const Quaternion<real_Num> &actual,
                                 const Quaternion<real_Num> &expected, const String &context )
    {
        BOOST_CHECK_MESSAGE( MathUtil<real_Num>::equals( actual, expected, TestTolerance ),
                             context << " orientation did not match" );
    }

    template <class T>
    T requireProperty( const SmartPtr<Properties> &properties, const String &name )
    {
        BOOST_REQUIRE( properties );

        T value{};
        BOOST_REQUIRE_MESSAGE( properties->getPropertyValue( name, value ),
                               "Expected property '" << name << "' to be present" );

        return value;
    }

    SmartPtr<Properties> requireNamedChild( const SmartPtr<Properties> &properties, const String &name )
    {
        BOOST_REQUIRE( properties );

        auto child = properties->getChild( name );
        BOOST_REQUIRE_MESSAGE( child, "Expected child properties named '" << name << "'" );

        return child;
    }

    SmartPtr<Properties> parseSavedScene( const String &savedText, DataFormat format )
    {
        auto properties = make_ptr<Properties>();
        DataUtil::parse( savedText, properties.get(), format );
        BOOST_REQUIRE( properties );

        return properties;
    }

    SmartPtr<Properties> findActorByName( const SmartPtr<Properties> &properties,
                                          const String &childName, const String &actorName )
    {
        BOOST_REQUIRE( properties );

        auto actors = properties->getChildrenByName( childName );
        for( auto actor : actors )
        {
            if( actor )
            {
                auto label = requireProperty<String>( actor, GameActorUtil::labelStr );
                if( label == actorName )
                {
                    return actor;
                }
            }
        }

        BOOST_FAIL( "Expected saved actor named '" << actorName << "'" );
        return nullptr;
    }

    bool componentTypeMatches( const String &componentType, const String &typeName )
    {
        const auto qualifiedTypeName = String( "::" ) + typeName;
        return componentType == typeName || ( componentType.size() >= qualifiedTypeName.size() &&
                                              componentType.rfind( qualifiedTypeName ) ==
                                                  componentType.size() - qualifiedTypeName.size() );
    }

    SmartPtr<Properties> findComponentByType( const SmartPtr<Properties> &actor, const String &typeName )
    {
        BOOST_REQUIRE( actor );

        auto components = actor->getChildrenByName( GameActorUtil::componentStr );
        for( auto component : components )
        {
            if( component )
            {
                auto componentType =
                    requireProperty<String>( component, GameActorUtil::componentTypeStr );
                if( componentTypeMatches( componentType, typeName ) )
                {
                    return component;
                }
            }
        }

        BOOST_FAIL( "Expected saved component type '" << typeName << "'" );
        return nullptr;
    }

    void requireTransformMatches( const SmartPtr<Properties> &properties,
                                  const Transform3<real_Num> &expected, const String &context )
    {
        BOOST_REQUIRE( properties );

        Transform3<real_Num> actual;
        actual.setProperties( properties );

        requireVectorClose( actual.getPosition(), expected.getPosition(), context + " position" );
        requireVectorClose( actual.getScale(), expected.getScale(), context + " scale" );
        requireQuaternionClose( actual.getOrientation(), expected.getOrientation(),
                                context + " orientation" );
    }

    struct GameSceneSavingFixture : TestGuard
    {
        GameSceneSavingFixture()
        {
            BOOST_REQUIRE( isAvailable );
            BOOST_REQUIRE( applicationManager );
            BOOST_REQUIRE( factoryManager );
            BOOST_REQUIRE( fileSystem );
            BOOST_REQUIRE( sceneManager );
            BOOST_REQUIRE( scene );

            testDirectory = Path::getWorkingDirectory() + "/gamescene_saving_tests";
            trackFilesystemPath( testDirectory );
            std::filesystem::create_directories( testDirectory.c_str() );

            originalCameraManager = applicationManager->getCameraManager();
            auto cameraManager = originalCameraManager;
            if( !cameraManager )
            {
                cameraManager = factoryManager->make_ptr<CameraManager>();
                cameraManager->load( nullptr );
                applicationManager->setCameraManager( cameraManager );
            }

            originalEditorCamera = cameraManager->getEditorCamera();
            if( !originalEditorCamera )
            {
                auto editorCamera = sceneManager->createActor();
                BOOST_REQUIRE( editorCamera );
                editorCamera->setName( "SavingTestsEditorCamera" );
                cameraManager->setEditorCamera( editorCamera );
            }

            addCleanup( [this]() {
                if( applicationManager )
                {
                    if( originalCameraManager )
                    {
                        originalCameraManager->setEditorCamera( originalEditorCamera );
                    }

                    applicationManager->setCameraManager( originalCameraManager );
                }
            } );

            scene->clear( true );
            scene->setLabel( "SavingTestsScene" );
            scene->setFilePath( String() );

            createSerializableActorGraph();
        }

        void createSerializableActorGraph()
        {
            savedActor = sceneManager->createActor();
            BOOST_REQUIRE( savedActor );
            savedActor->setName( "Saved Actor" );
            savedActor->setStatic( true );
            savedActor->setEnabled( false );
            savedActor->setVisible( false );
            savedActor->setSmoothMotion( true );
            savedActor->setCollisionMask( 0x00000013u );
            savedActor->setLayer( "Gameplay" );
            savedActor->setTags( { "save-test", "production" } );
            savedActor->setLocalPosition( Vector3<real_Num>( 1.25f, -2.5f, 3.75f ) );
            savedActor->setLocalRotation( Vector3<real_Num>( 10.0f, 20.0f, 30.0f ) );
            savedActor->setLocalScale( Vector3<real_Num>( 2.0f, 3.0f, 4.0f ) );

            savedChildActor = sceneManager->createActor();
            BOOST_REQUIRE( savedChildActor );
            savedChildActor->setName( "Saved Child" );
            savedChildActor->setLocalPosition( Vector3<real_Num>( -1.0f, 2.0f, 0.5f ) );
            savedChildActor->setLocalRotation( Vector3<real_Num>( 0.0f, 45.0f, 0.0f ) );
            savedChildActor->setLocalScale( Vector3<real_Num>( 0.5f, 0.75f, 1.25f ) );
            savedActor->addChild( savedChildActor );

            hiddenChildActor = sceneManager->createActor();
            BOOST_REQUIRE( hiddenChildActor );
            hiddenChildActor->setName( "Do Not Save" );
            hiddenChildActor->setFlag( IGameActor::ActorFlagDontSave, true );
            savedActor->addChild( hiddenChildActor );

            auto component = factoryManager->make_ptr<Component>();
            BOOST_REQUIRE( component );
            component->setEnabled( false );
            savedActor->addComponentInstance( component );

            auto collisionBox = factoryManager->make_ptr<CollisionBox>();
            BOOST_REQUIRE( collisionBox );
            collisionBox->setTrigger( true );
            collisionBox->setExtents( Vector3<real_Num>( 4.0f, 5.0f, 6.0f ) );
            collisionBox->setPosition( Vector3<real_Num>( 0.25f, 0.5f, 0.75f ) );
            collisionBox->setRadius( 1.5f );
            collisionBox->setStaticFriction( 0.7f );
            collisionBox->setDynamicFriction( 0.3f );
            collisionBox->setRestitution( 0.2f );
            savedActor->addComponentInstance( collisionBox );

            auto rigidbody = factoryManager->make_ptr<Rigidbody>();
            BOOST_REQUIRE( rigidbody );
            rigidbody->setMass( 42.5f );
            rigidbody->setKinematic( true );
            rigidbody->setMassSpaceInertiaTensor( Vector3<real_Num>( 7.0f, 8.0f, 9.0f ) );
            rigidbody->setMaxLinearVelocity( 123.0f );
            rigidbody->setMaxAngularVelocity( 456.0f );
            rigidbody->setLinearDamping( 0.15f );
            rigidbody->setAngularDamping( 0.35f );
            rigidbody->setUseGravity( false );
            rigidbody->setContinuousCollisionDetection( true );
            rigidbody->setSleepThreshold( 0.02f );
            rigidbody->setStabilizationThreshold( 0.03f );
            rigidbody->setSolverPositionIterations( 7u );
            rigidbody->setSolverVelocityIterations( 2u );
            rigidbody->setContactReportThreshold( 1.25f );
            rigidbody->setGroupMask( 0x00000022u );
            rigidbody->setCollisionMask( 0x00000044u );
            savedActor->addComponentInstance( rigidbody );

            scene->addActor( savedActor );
        }

        String path( const String &fileName ) const
        {
            return testDirectory + "/" + fileName;
        }

        String readText( const String &filePath )
        {
            auto text = fileSystem->readAllText( filePath );
            if( text.empty() && std::filesystem::exists( filePath.c_str() ) )
            {
                std::ifstream stream( filePath.c_str(), std::ios::binary );
                return String( std::istreambuf_iterator<char>( stream ),
                               std::istreambuf_iterator<char>() );
            }

            return text;
        }

        void requireSavedText( const String &filePath, String &text )
        {
            BOOST_REQUIRE_MESSAGE( std::filesystem::exists( filePath.c_str() ),
                                   "Expected saved scene file: " << filePath );

            text = readText( filePath );
            BOOST_REQUIRE_MESSAGE( !StringUtil::isNullOrEmpty( text ),
                                   "Expected saved scene file to contain data: " << filePath );
        }

        void requireSavedActorData( const String &savedText, DataFormat format )
        {
            auto root = parseSavedScene( savedText, format );
            auto actor = findActorByName( root, ApplicationUtil::actorsStr, savedActor->getName() );

            BOOST_CHECK_EQUAL( requireProperty<String>( actor, GameActorUtil::labelStr ),
                               savedActor->getName() );
            BOOST_CHECK_EQUAL( requireProperty<bool>( actor, GameActorUtil::staticStr ), true );
            BOOST_CHECK_EQUAL( requireProperty<bool>( actor, GameActorUtil::enabledStr ), false );
            BOOST_CHECK_EQUAL( requireProperty<bool>( actor, "visible" ), false );
            BOOST_CHECK_EQUAL( requireProperty<bool>( actor, GameActorUtil::smoothMotionStr ), true );
            BOOST_CHECK_EQUAL( requireProperty<u32>( actor, GameActorUtil::collisionMaskStr ),
                               0x00000013u );
            BOOST_CHECK_EQUAL( requireProperty<String>( actor, GameActorUtil::layerStr ), "Gameplay" );

            auto tags = requireProperty<Array<String>>( actor, GameActorUtil::tagsStr );
            BOOST_CHECK( std::find( tags.begin(), tags.end(), "save-test" ) != tags.end() );
            BOOST_CHECK( std::find( tags.begin(), tags.end(), "production" ) != tags.end() );

            requireTransformMatches( requireNamedChild( actor, GameActorUtil::localTransformStr ),
                                     savedActor->getLocalTransform(), "actor local transform" );
            requireTransformMatches( requireNamedChild( actor, GameActorUtil::worldTransformStr ),
                                     savedActor->getWorldTransform(), "actor world transform" );

            auto child = findActorByName( actor, GameActorUtil::childStr, savedChildActor->getName() );
            requireTransformMatches( requireNamedChild( child, GameActorUtil::localTransformStr ),
                                     savedChildActor->getLocalTransform(), "child local transform" );
            requireTransformMatches( requireNamedChild( child, GameActorUtil::worldTransformStr ),
                                     savedChildActor->getWorldTransform(), "child world transform" );

            auto savedChildren = actor->getChildrenByName( GameActorUtil::childStr );
            BOOST_CHECK_EQUAL( savedChildren.size(), 1u );

            auto component = findComponentByType( actor, "Component" );
            BOOST_CHECK_EQUAL( requireProperty<bool>( component, IComponent::enabledStr ), false );

            auto collisionBox = findComponentByType( actor, "CollisionBox" );
            BOOST_CHECK_EQUAL( requireProperty<bool>( collisionBox, Collision::isTriggerStr ), true );
            requireVectorClose(
                requireProperty<Vector3<real_Num>>( collisionBox, Collision::extentsStr ),
                Vector3<real_Num>( 4.0f, 5.0f, 6.0f ), "collision extents" );
            requireVectorClose(
                requireProperty<Vector3<real_Num>>( collisionBox, Collision::positionStr ),
                Vector3<real_Num>( 0.25f, 0.5f, 0.75f ), "collision position" );
            BOOST_CHECK_CLOSE( requireProperty<f32>( collisionBox, Collision::radiusStr ), 1.5f,
                               0.001f );
            BOOST_CHECK_CLOSE( requireProperty<f32>( collisionBox, Collision::staticFrictionStr ), 0.7f,
                               0.001f );
            BOOST_CHECK_CLOSE( requireProperty<f32>( collisionBox, Collision::dynamicFrictionStr ), 0.3f,
                               0.001f );
            BOOST_CHECK_CLOSE( requireProperty<f32>( collisionBox, Collision::restitutionStr ), 0.2f,
                               0.001f );

            auto rigidbody = findComponentByType( actor, "Rigidbody" );
            BOOST_CHECK_CLOSE( requireProperty<real_Num>( rigidbody, Rigidbody::MassStr ), 42.5f,
                               0.001f );
            BOOST_CHECK_EQUAL( requireProperty<bool>( rigidbody, Rigidbody::KinematicStr ), true );
            requireVectorClose(
                requireProperty<Vector3<real_Num>>( rigidbody, Rigidbody::MassSpaceInertiaTensorStr ),
                Vector3<real_Num>( 7.0f, 8.0f, 9.0f ), "rigidbody inertia tensor" );
            BOOST_CHECK_CLOSE( requireProperty<real_Num>( rigidbody, Rigidbody::MaxLinearVelocityStr ),
                               123.0f, 0.001f );
            BOOST_CHECK_CLOSE( requireProperty<real_Num>( rigidbody, Rigidbody::MaxAngularVelocityStr ),
                               456.0f, 0.001f );
            BOOST_CHECK_CLOSE( requireProperty<real_Num>( rigidbody, Rigidbody::LinearDampingStr ),
                               0.15f, 0.001f );
            BOOST_CHECK_CLOSE( requireProperty<real_Num>( rigidbody, Rigidbody::AngularDampingStr ),
                               0.35f, 0.001f );
            BOOST_CHECK_EQUAL( requireProperty<bool>( rigidbody, Rigidbody::UseGravityStr ), false );
            BOOST_CHECK_EQUAL(
                requireProperty<bool>( rigidbody, Rigidbody::ContinuousCollisionDetectionStr ), true );
            BOOST_CHECK_CLOSE( requireProperty<real_Num>( rigidbody, Rigidbody::SleepThresholdStr ),
                               0.02f, 0.001f );
            BOOST_CHECK_CLOSE(
                requireProperty<real_Num>( rigidbody, Rigidbody::StabilizationThresholdStr ), 0.03f,
                0.001f );
            BOOST_CHECK_EQUAL( requireProperty<u32>( rigidbody, Rigidbody::SolverPositionIterationsStr ),
                               7u );
            BOOST_CHECK_EQUAL( requireProperty<u32>( rigidbody, Rigidbody::SolverVelocityIterationsStr ),
                               2u );
            BOOST_CHECK_CLOSE(
                requireProperty<real_Num>( rigidbody, Rigidbody::ContactReportThresholdStr ), 1.25f,
                0.001f );
            BOOST_CHECK_EQUAL( requireProperty<u32>( rigidbody, Rigidbody::GroupMaskStr ), 0x00000022u );
            BOOST_CHECK_EQUAL( requireProperty<u32>( rigidbody, Rigidbody::CollisionMaskStr ),
                               0x00000044u );
        }

        String testDirectory;
        SmartPtr<ICameraManager> originalCameraManager;
        SmartPtr<IGameActor> originalEditorCamera;
        SmartPtr<IGameActor> savedActor;
        SmartPtr<IGameActor> savedChildActor;
        SmartPtr<IGameActor> hiddenChildActor;
    };
}  // namespace

BOOST_FIXTURE_TEST_SUITE( GameSceneSavingTests, GameSceneSavingFixture )

BOOST_AUTO_TEST_CASE( gameactor_data_round_trip_preserves_visibility )
{
    auto source = sceneManager->createActor();
    BOOST_REQUIRE( source );
    source->setName( "Visibility Source" );
    source->setVisible( false );

    auto data = GameActorUtil::toData( source );
    BOOST_REQUIRE( data );
    BOOST_CHECK_EQUAL( requireProperty<bool>( data, "visible" ), false );

    auto restored = sceneManager->createActor();
    BOOST_REQUIRE( restored );
    BOOST_CHECK( restored->isVisible() );

    GameActorUtil::loadFromData( restored, data, false );

    BOOST_CHECK_EQUAL( restored->getName(), "Visibility Source" );
    BOOST_CHECK( !restored->isVisible() );
}

BOOST_AUTO_TEST_CASE( gamescene_save_fbscene_writes_json_scene )
{
    const auto filePath = path( "json_scene.fbscene" );

    scene->saveScene( filePath );

    String savedText;
    requireSavedText( filePath, savedText );

    BOOST_CHECK_EQUAL( scene->getFilePath(), filePath );
    BOOST_CHECK_EQUAL( scene->getLabel(), "json_scene" );
    BOOST_CHECK( savedText.find( "{" ) == 0 );
    BOOST_CHECK( savedText.find( "\"children\"" ) != String::npos );
    BOOST_CHECK( savedText.find( "Saved Actor" ) != String::npos );
    BOOST_CHECK( savedText.find( "<?xml" ) == String::npos );
    requireSavedActorData( savedText, DataFormat::JSON );
}

BOOST_AUTO_TEST_CASE( gamescene_save_fbscenexml_writes_xml_scene )
{
    const auto filePath = path( "xml_scene.fbscenexml" );

    scene->saveScene( filePath );

    String savedText;
    requireSavedText( filePath, savedText );

    BOOST_CHECK_EQUAL( scene->getFilePath(), filePath );
    BOOST_CHECK_EQUAL( scene->getLabel(), "xml_scene" );
    BOOST_CHECK( savedText.find( "<?xml" ) == 0 );
    BOOST_CHECK( savedText.find( "<Root" ) != String::npos );
    BOOST_CHECK( savedText.find( "Saved Actor" ) != String::npos );
    BOOST_CHECK( savedText.find( "\"children\"" ) == String::npos );
    requireSavedActorData( savedText, DataFormat::XML );
}

BOOST_AUTO_TEST_CASE( gamescene_save_fbscenebin_writes_binary_properties_scene )
{
    const auto filePath = path( "binary_scene.fbscenebin" );

    scene->saveScene( filePath );

    BOOST_REQUIRE_MESSAGE( std::filesystem::exists( filePath.c_str() ),
                           "Expected saved binary scene file: " << filePath );
    auto bytes = fileSystem->readAllBytes( filePath );
    BOOST_REQUIRE( !bytes.empty() );
    BOOST_CHECK( PropertiesBinarySerializer::hasBinaryHeader( bytes.data(), bytes.size() ) );

    auto root = make_ptr<Properties>();
    String parseError;
    BOOST_REQUIRE_MESSAGE( PropertiesBinarySerializer::deserialize( bytes, *root, &parseError ),
                           parseError );

    auto actor = findActorByName( root, ApplicationUtil::actorsStr, savedActor->getName() );
    BOOST_REQUIRE( actor );
    BOOST_CHECK_EQUAL( requireProperty<String>( actor, GameActorUtil::labelStr ), "Saved Actor" );
    BOOST_CHECK( findComponentByType( actor, "CollisionBox" ) );
    BOOST_CHECK( findComponentByType( actor, "Rigidbody" ) );
    BOOST_CHECK_EQUAL( scene->getFilePath(), filePath );
    BOOST_CHECK_EQUAL( scene->getLabel(), "binary_scene" );
}

BOOST_AUTO_TEST_CASE( gamescene_save_without_extension_defaults_to_fbscene_json )
{
    const auto extensionlessPath = path( "extensionless_scene" );
    const auto expectedFilePath = extensionlessPath + ApplicationUtil::builtinSceneExt;

    scene->saveScene( extensionlessPath );

    String savedText;
    requireSavedText( expectedFilePath, savedText );

    BOOST_CHECK( !std::filesystem::exists( extensionlessPath.c_str() ) );
    BOOST_CHECK_EQUAL( scene->getFilePath(), extensionlessPath );
    BOOST_CHECK_EQUAL( scene->getLabel(), "extensionless_scene" );
    BOOST_CHECK( savedText.find( "{" ) == 0 );
    BOOST_CHECK( savedText.find( "<?xml" ) == String::npos );
    requireSavedActorData( savedText, DataFormat::JSON );
}

BOOST_AUTO_TEST_CASE( gamescene_save_uses_current_file_path_when_no_path_is_supplied )
{
    const auto filePath = path( "current_path_scene.fbscene" );
    scene->setFilePath( filePath );

    scene->saveScene();

    String savedText;
    requireSavedText( filePath, savedText );

    BOOST_CHECK_EQUAL( scene->getFilePath(), filePath );
    BOOST_CHECK_EQUAL( scene->getLabel(), "current_path_scene" );
    BOOST_CHECK( savedText.find( "{" ) == 0 );
    requireSavedActorData( savedText, DataFormat::JSON );
}

BOOST_AUTO_TEST_SUITE_END()
