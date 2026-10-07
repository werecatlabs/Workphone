#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/GraphicsMesh.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/Mesh/MeshConverter.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <filesystem>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <algorithm>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    class ReimportGraphicsSystem : public render::GraphicsSystem
    {
    public:
        void reloadObject( SmartPtr<ISharedObject> object, bool forceQueue ) override
        {
            reloadedObject = object;
            queued = forceQueue;
            ++reloadCount;
        }

        SmartPtr<ISharedObject> reloadedObject;
        bool queued = false;
        u32 reloadCount = 0;
    };

    class MeshImportListener : public IEventListener
    {
    public:
        Parameter handleEvent( EventType type, hash_type value, const Array<Parameter> &args,
                               SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                               SmartPtr<IEvent> event ) override
        {
            if( value == IEvent::meshLoaded )
            {
                ++notifications;
                fileWritten = std::filesystem::file_size( outputPath.c_str() ) > 0;
                renderer->handleEvent( type, value, args, sender, object, event );
            }
            return {};
        }

        SmartPtr<MeshRenderer> renderer;
        String outputPath;
        u32 notifications = 0;
        bool fileWritten = false;
    };

    class TestGraphicsMesh : public render::GraphicsMesh
    {
    public:
        void setCastShadows( bool castShadows ) override
        {
            this->castShadows = castShadows;
        }
        bool getCastShadows() const override
        {
            return castShadows;
        }
        void setReceiveShadows( bool receiveShadows ) override
        {
            this->receiveShadows = receiveShadows;
        }
        bool getReceiveShadows() const override
        {
            return receiveShadows;
        }
        void setVisible( bool visible ) override
        {
            this->visible = visible;
        }
        bool isVisible() const override
        {
            return visible;
        }
        void setZOrder( u32 zOrder ) override
        {
            this->zOrder = zOrder;
        }
        u32 getZOrder() const override
        {
            return zOrder;
        }
        void setVisibilityFlags( u32 flags ) override
        {
            visibilityFlags = flags;
        }
        u32 getVisibilityFlags() const override
        {
            return visibilityFlags;
        }

        SmartPtr<render::IGraphicsObject> clone(
            const String &name = StringUtil::EmptyString ) const override
        {
            auto clonedMesh = workphone::make_ptr<TestGraphicsMesh>();
            clonedMesh->setMeshName( name.empty() ? getMeshName() : name );
            clonedMesh->setMaterialName( getMaterialName() );
            clonedMesh->setCastShadows( getCastShadows() );
            clonedMesh->setReceiveShadows( getReceiveShadows() );
            clonedMesh->setVisible( isVisible() );
            clonedMesh->setZOrder( getZOrder() );
            clonedMesh->setVisibilityFlags( getVisibilityFlags() );
            return clonedMesh;
        }

        bool castShadows = false;
        bool receiveShadows = false;
        bool visible = true;
        u32 zOrder = 0u;
        u32 visibilityFlags = 0u;
    };

    bool skipWhenGraphicsMeshRuntimeUnavailable()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            BOOST_TEST_MESSAGE(
                "Application manager is not available - skipping mesh renderer runtime test" );
            return true;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem || !graphicsSystem->getGraphicsScene() )
        {
            BOOST_TEST_MESSAGE(
                "Graphics runtime is not available - skipping mesh renderer runtime test" );
            return true;
        }

        auto factoryManager = applicationManager->getFactoryManager();
        if( !factoryManager )
        {
            BOOST_TEST_MESSAGE(
                "Factory manager is not available - skipping mesh renderer runtime test" );
            return true;
        }

        auto factories = factoryManager->getFactories();
        for( auto factory : factories )
        {
            if( factory && factory->isObjectDerivedFrom<render::IGraphicsMesh>() )
            {
                return false;
            }
        }

        BOOST_TEST_MESSAGE(
            "Graphics mesh factory is not available - skipping mesh renderer runtime test" );
        return true;
    }

    SmartPtr<TestGraphicsMesh> makeGraphicsMesh()
    {
        return workphone::make_ptr<TestGraphicsMesh>();
    }

    SmartPtr<render::Material> makeRenderMaterial()
    {
        return workphone::make_ptr<render::Material>();
    }

    bool containsChildObject( const Array<SmartPtr<ISharedObject>> &children, ISharedObject *object )
    {
        return std::find_if( children.begin(), children.end(),
                             [object]( const SmartPtr<ISharedObject> &child ) {
                                 return child.get() == object;
                             } ) != children.end();
    }
}  // namespace

BOOST_AUTO_TEST_CASE( components_mesh_renderer_reimport_refreshes_unchanged_path )
{
    TestGuard guard;
    auto app = guard.applicationManager;
    const auto oldGraphics = app->getGraphicsSystem();
    const auto oldPool = app->getThreadPool();
    const auto oldCache = app->getCachePath();
    const auto cache = String( ( std::filesystem::temp_directory_path() /
                                StringUtil::getUUID().c_str() ).generic_string() );
    guard.trackFilesystemPath( cache );
    guard.addCleanup( [app, oldGraphics, oldPool, oldCache]() mutable {
        app->setGraphicsSystem( oldGraphics );
        app->setThreadPool( oldPool );
        app->setCachePath( oldCache );
    } );

    auto actor = guard.sceneManager->createActor();
    auto component = actor->addComponent<scene::Mesh>();
    auto resource = workphone::dynamic_pointer_cast<IMeshResource>(
        app->getMeshManager()->create( StringUtil::getUUID() ) );
    BOOST_REQUIRE( resource );
    resource->setFilePath( "Cache/reimport_test.fbmeshbin" );
    auto mesh = MeshUtil::createBox();
    mesh->setName( resource->getFilePath() );
    resource->setMesh( mesh );
    component->setMeshResource( resource );

    auto renderer = actor->addComponent<MeshRenderer>();
    auto graphicsMesh = makeGraphicsMesh();
    graphicsMesh->setMeshName( resource->getFilePath() );
    renderer->setGraphicsObject( graphicsMesh );

    auto graphics = workphone::make_ptr<ReimportGraphicsSystem>();
    app->setGraphicsSystem( graphics );
    app->setThreadPool( nullptr ); // Deliver deterministically without background tasks.
    app->setCachePath( cache );

    auto listener = workphone::make_ptr<MeshImportListener>();
    listener->renderer = renderer;
    listener->outputPath = cache + "/reimport_test.fbmeshbin";
    app->addObjectListener( listener );
    guard.addCleanup( [app, listener]() mutable { app->removeObjectListener( listener ); } );

    MeshConverter converter;
    converter.writeMesh( actor );
    BOOST_CHECK_EQUAL( listener->notifications, 1u );
    BOOST_CHECK( listener->fileWritten );
    BOOST_CHECK_EQUAL( graphics->reloadCount, 1u );
    BOOST_CHECK( graphics->queued );
    BOOST_CHECK( graphics->reloadedObject == graphicsMesh );
    BOOST_CHECK_EQUAL( graphicsMesh->getMeshName(), resource->getFilePath() );

    for( u32 attempt = 2; attempt <= 3; ++attempt )
    {
        auto updatedMesh = MeshUtil::createBox( static_cast<f32>( attempt ), 1.f, 1.f );
        updatedMesh->setName( resource->getFilePath() );
        resource->setMesh( updatedMesh );
        converter.writeMesh( actor );
        BOOST_CHECK_EQUAL( listener->notifications, attempt );
        BOOST_CHECK_EQUAL( graphics->reloadCount, attempt );
        BOOST_CHECK( listener->fileWritten );
        BOOST_CHECK( graphics->queued );
        BOOST_CHECK( graphics->reloadedObject == graphicsMesh );
    }

    auto unrelated = workphone::dynamic_pointer_cast<IMeshResource>(
        app->getMeshManager()->create( StringUtil::getUUID() ) );
    BOOST_REQUIRE( unrelated );
    unrelated->setFilePath( "Cache/other.fbmeshbin" );
    renderer->handleEvent( EventType::Renderer, IEvent::meshLoaded, {}, nullptr, unrelated, nullptr );
    renderer->handleEvent( EventType::Renderer, IEvent::meshLoaded, {}, nullptr, nullptr, nullptr );
    renderer->handleEvent( EventType::Renderer, IEvent::meshLoaded, {}, nullptr, mesh, nullptr );
    BOOST_CHECK_EQUAL( graphics->reloadCount, 3u );
    renderer->setGraphicsObject( nullptr );
}

BOOST_AUTO_TEST_CASE( components_mesh_renderer_default_state_and_properties )
{
    try
    {
        TestGuard guard;

        auto renderer = workphone::make_ptr<MeshRenderer>();
        BOOST_REQUIRE( renderer );

        BOOST_CHECK( !renderer->isLoaded() );
        BOOST_CHECK( !renderer->getGraphicsObject() );
        BOOST_CHECK( !renderer->getGraphicsNode() );
        BOOST_CHECK( !renderer->getMeshNode() );
        BOOST_CHECK( renderer->getCastShadows() == Renderer::CastShadows::On );
        BOOST_CHECK( renderer->getRecieveShadows() == Renderer::RecieveShadows::On );
        BOOST_CHECK( renderer->getReflections() == Renderer::Reflections::Default );
        BOOST_CHECK( renderer->getOcculsion() == Renderer::Occulsion::On );
        BOOST_CHECK_EQUAL( renderer->getZOrder(), 0u );
        BOOST_CHECK_EQUAL( renderer->getVisibilityFlags(), 0xFFFFFFFFu );
        BOOST_CHECK_EQUAL( MeshRenderer::meshNodeSuffixStr, "_MeshComponent" );
        BOOST_CHECK_EQUAL( MeshRenderer::meshPathStr, "meshPath" );

        auto properties = renderer->getProperties();
        BOOST_REQUIRE( properties );

        s32 castShadows = -1;
        s32 recieveShadows = -1;
        s32 reflections = -1;
        s32 occulsion = -1;
        u32 zOrder = 99u;
        u32 visibilityFlags = 0u;
        String materialName = "not-default";

        BOOST_CHECK( properties->getPropertyValue( Renderer::castShadowsStr, castShadows ) );
        BOOST_CHECK( properties->getPropertyValue( Renderer::recieveShadowsStr, recieveShadows ) );
        BOOST_CHECK( properties->getPropertyValue( Renderer::reflectionsStr, reflections ) );
        BOOST_CHECK( properties->getPropertyValue( Renderer::occulsionStr, occulsion ) );
        BOOST_CHECK( properties->getPropertyValue( Renderer::zOrderStr, zOrder ) );
        BOOST_CHECK( properties->getPropertyValue( Renderer::visibilityFlagsStr, visibilityFlags ) );
        BOOST_CHECK( properties->getPropertyValue( Renderer::materialNameStr, materialName ) );

        BOOST_CHECK_EQUAL( castShadows, static_cast<s32>( Renderer::CastShadows::On ) );
        BOOST_CHECK_EQUAL( recieveShadows, static_cast<s32>( Renderer::RecieveShadows::On ) );
        BOOST_CHECK_EQUAL( reflections, static_cast<s32>( Renderer::Reflections::Default ) );
        BOOST_CHECK_EQUAL( occulsion, static_cast<s32>( Renderer::Occulsion::On ) );
        BOOST_CHECK_EQUAL( zOrder, 0u );
        BOOST_CHECK_EQUAL( visibilityFlags, 0xFFFFFFFFu );
        BOOST_CHECK( materialName.empty() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_renderer_set_properties_updates_renderer_state )
{
    try
    {
        TestGuard guard;

        auto renderer = workphone::make_ptr<MeshRenderer>();
        auto graphicsMesh = makeGraphicsMesh();
        BOOST_REQUIRE( renderer );
        BOOST_REQUIRE( graphicsMesh );
        renderer->setGraphicsObject( graphicsMesh );

        auto properties = workphone::make_ptr<Properties>();
        BOOST_REQUIRE( properties );
        properties->setProperty( Renderer::castShadowsStr,
                                 static_cast<s32>( Renderer::CastShadows::Off ) );
        properties->setProperty( Renderer::recieveShadowsStr,
                                 static_cast<s32>( Renderer::RecieveShadows::Off ) );
        properties->setProperty( Renderer::reflectionsStr,
                                 static_cast<s32>( Renderer::Reflections::Simple ) );
        properties->setProperty( Renderer::occulsionStr,
                                 static_cast<s32>( Renderer::Occulsion::Dynamic ) );
        properties->setProperty( Renderer::zOrderStr, 42u );
        properties->setProperty( Renderer::visibilityFlagsStr, 0x00FF00FFu );
        properties->setProperty( Renderer::materialNameStr, "UnitTestMaterial" );

        renderer->setProperties( properties );

        BOOST_CHECK( renderer->getCastShadows() == Renderer::CastShadows::Off );
        BOOST_CHECK( renderer->getRecieveShadows() == Renderer::RecieveShadows::Off );
        BOOST_CHECK( renderer->getReflections() == Renderer::Reflections::Simple );
        BOOST_CHECK( renderer->getOcculsion() == Renderer::Occulsion::Dynamic );
        BOOST_CHECK_EQUAL( renderer->getZOrder(), 42u );
        BOOST_CHECK_EQUAL( renderer->getVisibilityFlags(), 0x00FF00FFu );
        BOOST_CHECK_EQUAL( renderer->getMaterialName(), "UnitTestMaterial" );
        BOOST_CHECK_EQUAL( graphicsMesh->getCastShadows(), false );
        BOOST_CHECK_EQUAL( graphicsMesh->getReceiveShadows(), false );
        BOOST_CHECK_EQUAL( graphicsMesh->getZOrder(), 42u );
        BOOST_CHECK_EQUAL( graphicsMesh->getVisibilityFlags(), 0x00FF00FFu );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_renderer_child_objects_include_runtime_dependencies )
{
    try
    {
        TestGuard guard;

        auto renderer = workphone::make_ptr<MeshRenderer>();
        auto graphicsMesh = makeGraphicsMesh();
        auto sharedMaterial = makeRenderMaterial();
        BOOST_REQUIRE( renderer );
        BOOST_REQUIRE( graphicsMesh );
        BOOST_REQUIRE( sharedMaterial );

        renderer->setGraphicsObject( graphicsMesh );
        renderer->setSharedMaterial( sharedMaterial );

        auto children = renderer->getChildObjects();
        BOOST_CHECK( containsChildObject( children, graphicsMesh.get() ) );
        BOOST_CHECK( containsChildObject( children, sharedMaterial.get() ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_renderer_update_materials_binds_actor_material_slots )
{
    if( skipWhenGraphicsMeshRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;
        BOOST_REQUIRE( guard.sceneManager );

        auto actor = guard.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto renderer = actor->addComponent<MeshRenderer>();
        BOOST_REQUIRE( renderer );

        auto material0 = actor->addComponent<scene::Material>();
        auto material2 = actor->addComponent<scene::Material>();
        BOOST_REQUIRE( material0 );
        BOOST_REQUIRE( material2 );

        auto renderMaterial0 = makeRenderMaterial();
        auto renderMaterial2 = makeRenderMaterial();
        auto graphicsMesh = makeGraphicsMesh();
        BOOST_REQUIRE( renderMaterial0 );
        BOOST_REQUIRE( renderMaterial2 );
        BOOST_REQUIRE( graphicsMesh );

        material0->setIndex( 0u );
        material0->setMaterial( renderMaterial0 );
        material2->setIndex( 2u );
        material2->setMaterial( renderMaterial2 );
        renderer->setGraphicsObject( graphicsMesh );

        renderer->updateMaterials();

        BOOST_CHECK_MESSAGE( graphicsMesh->getMaterial( 0 ).get() == renderMaterial0.get(),
                             "Material slot 0 was not bound from the actor material component" );
        BOOST_CHECK_MESSAGE( !graphicsMesh->getMaterial( 1 ),
                             "Material slot 1 should remain unassigned" );
        BOOST_CHECK_MESSAGE( graphicsMesh->getMaterial( 2 ).get() == renderMaterial2.get(),
                             "Material slot 2 was not bound from the actor material component" );

        material0->setMaterial( nullptr );
        material2->setMaterial( nullptr );
        actor->removeComponentInstance( material0 );
        actor->removeComponentInstance( material2 );
        renderer->setGraphicsObject( nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_renderer_lifecycle_is_idempotent_without_actor )
{
    try
    {
        TestGuard guard;

        auto renderer = workphone::make_ptr<MeshRenderer>();
        BOOST_REQUIRE( renderer );

        BOOST_CHECK_NO_THROW( renderer->unload( nullptr ) );
        BOOST_CHECK( !renderer->isLoaded() );

        BOOST_CHECK_NO_THROW( renderer->load( nullptr ) );
        BOOST_CHECK( renderer->isLoaded() );
        BOOST_CHECK_NO_THROW( renderer->load( nullptr ) );
        BOOST_CHECK( renderer->isLoaded() );

        BOOST_CHECK_NO_THROW( renderer->unload( nullptr ) );
        BOOST_CHECK( !renderer->isLoaded() );
        BOOST_CHECK_NO_THROW( renderer->unload( nullptr ) );
        BOOST_CHECK( !renderer->isLoaded() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_renderer_update_mesh_requires_mesh_component_with_path )
{
    try
    {
        TestGuard guard;
        BOOST_REQUIRE( guard.sceneManager );

        auto actorWithoutMesh = guard.sceneManager->createActor();
        BOOST_REQUIRE( actorWithoutMesh );
        auto rendererWithoutMesh = actorWithoutMesh->addComponent<MeshRenderer>();
        BOOST_REQUIRE( rendererWithoutMesh );

        BOOST_CHECK_NO_THROW( rendererWithoutMesh->updateMesh() );
        BOOST_CHECK( !rendererWithoutMesh->getGraphicsObject() );
        BOOST_CHECK( !rendererWithoutMesh->getMeshNode() );

        auto actorWithEmptyMesh = guard.sceneManager->createActor();
        BOOST_REQUIRE( actorWithEmptyMesh );
        auto mesh = actorWithEmptyMesh->addComponent<scene::Mesh>();
        auto rendererWithEmptyMesh = actorWithEmptyMesh->addComponent<MeshRenderer>();
        BOOST_REQUIRE( mesh );
        BOOST_REQUIRE( rendererWithEmptyMesh );
        mesh->setMeshPath( StringUtil::EmptyString );

        BOOST_CHECK_NO_THROW( rendererWithEmptyMesh->updateMesh() );
        BOOST_CHECK( !rendererWithEmptyMesh->getGraphicsObject() );
        BOOST_CHECK( !rendererWithEmptyMesh->getMeshNode() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_renderer_visibility_updates_graphics_mesh )
{
    try
    {
        TestGuard guard;
        BOOST_REQUIRE( guard.sceneManager );

        auto actor = guard.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto renderer = actor->addComponent<MeshRenderer>();
        auto graphicsMesh = makeGraphicsMesh();
        BOOST_REQUIRE( renderer );
        BOOST_REQUIRE( graphicsMesh );

        renderer->setGraphicsObject( graphicsMesh );
        renderer->updateVisibility();
        BOOST_CHECK( graphicsMesh->isVisible() );

        renderer->setEnabled( false );
        renderer->updateVisibility();
        BOOST_CHECK( !graphicsMesh->isVisible() );

        renderer->setEnabled( true );
        renderer->updateVisibility();
        BOOST_CHECK( graphicsMesh->isVisible() );

        guard.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_renderer_runtime_load_creates_mesh_object_and_node )
{
    if( skipWhenGraphicsMeshRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;
        BOOST_REQUIRE( guard.sceneManager );

        auto actor = guard.sceneManager->createActor();
        BOOST_REQUIRE( actor );
        actor->setName( "MeshRendererRuntimeActor" );

        auto mesh = actor->addComponent<scene::Mesh>();
        auto renderer = actor->addComponent<MeshRenderer>();
        BOOST_REQUIRE( mesh );
        BOOST_REQUIRE( renderer );

        const String meshPath = "UnitTests/MeshRendererRuntime.mesh";
        mesh->setMeshPath( meshPath );

        renderer->load( nullptr );

        auto graphicsObject = renderer->getGraphicsObjectByType<render::IGraphicsMesh>();
        BOOST_REQUIRE( graphicsObject );
        BOOST_REQUIRE( renderer->getMeshNode() );
        BOOST_CHECK_EQUAL( graphicsObject->getMeshName(), meshPath );
        BOOST_CHECK_EQUAL( graphicsObject->getVisibilityFlags(), render::IGraphicsObject::SceneFlag );
        BOOST_CHECK_EQUAL( renderer->getMeshNode()->getNumObjects(), 1u );

        renderer->unload( nullptr );
        BOOST_CHECK( !renderer->getGraphicsObject() );
        BOOST_CHECK( !renderer->getMeshNode() );

        guard.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_renderer_runtime_unload_clears_existing_runtime_objects )
{
    if( skipWhenGraphicsMeshRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;
        BOOST_REQUIRE( guard.sceneManager );

        auto actor = guard.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto mesh = actor->addComponent<scene::Mesh>();
        auto renderer = actor->addComponent<MeshRenderer>();
        BOOST_REQUIRE( mesh );
        BOOST_REQUIRE( renderer );

        mesh->setMeshPath( "UnitTests/MeshRendererUnload.mesh" );
        renderer->load( nullptr );

        BOOST_REQUIRE( renderer->getGraphicsObject() );
        BOOST_REQUIRE( renderer->getMeshNode() );

        BOOST_CHECK_NO_THROW( renderer->unload( nullptr ) );
        BOOST_CHECK( !renderer->isLoaded() );
        BOOST_CHECK( !renderer->getGraphicsObject() );
        BOOST_CHECK( !renderer->getMeshNode() );
        BOOST_CHECK_NO_THROW( renderer->unload( nullptr ) );

        guard.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}
