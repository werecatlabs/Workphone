#include <EditorPCH.hpp>
#include <scene/SceneBuilderBindings.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Scene/IGameSceneBuilder.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IComponent.hpp>
#include <Workphone/Interface/Scene/IComponentSystem.hpp>
#include <Workphone/Interface/Scene/IBuildDirector.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/Interface/Procedural/IProceduralManager.hpp>
#include <Workphone/Interface/Procedural/IWorldGenerator.hpp>
#include <Workphone/Interface/Procedural/ICityGenerator.hpp>
#include <Workphone/Interface/Procedural/ITerrainGenerator.hpp>
#include <Workphone/Interface/Procedural/IRoadNetwork.hpp>
#include <Workphone/Interface/Procedural/IProceduralScene.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER( workphone::editor::SceneBuilderBindings );

    SceneBuilderBindings::SceneBuilderBindings() = default;
    SceneBuilderBindings::~SceneBuilderBindings() = default;

    // -------------------------------------------------------------------------
    // Scene management

    SmartPtr<scene::IGameScene> SceneBuilderBindings::createScene( const String &name )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;

        auto builder = appMgr->getSceneBuilder();
        if( !builder ) return nullptr;

        auto scene = builder->getScene();
        if( !scene )
        {
            scene = workphone::make_ptr<scene::IGameScene>();
            builder->setScene( scene );
        }
        if( scene && !name.empty() )
            scene->setName( name );
        return scene;
    }

    SmartPtr<scene::IGameScene> SceneBuilderBindings::getCurrentScene()
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;
        auto builder = appMgr->getSceneBuilder();
        if( !builder ) return nullptr;
        return builder->getScene();
    }

    void SceneBuilderBindings::saveScene( SmartPtr<scene::IGameScene> scene,
                                          const String &path )
    {
        WP_UNUSED( scene );
        WP_UNUSED( path );
        // ref: Esoterica/Code/EngineTools/MapEditor/ — scene serialisation
        // Deferred: runtime scene I/O needs a serialiser implementation.
    }

    SmartPtr<scene::IGameScene> SceneBuilderBindings::loadScene( const String &path )
    {
        WP_UNUSED( path );
        // Deferred: runtime scene I/O. Stub keeps the binding contract.
        return nullptr;
    }

    // -------------------------------------------------------------------------
    // Actor management

    SmartPtr<scene::IGameActor> SceneBuilderBindings::addActor(
        SmartPtr<scene::IGameScene> scene,
        const String &actorName,
        const String &actorType )
    {
        if( !scene ) return nullptr;
        auto actor = scene->addActor();
        if( !actor ) return nullptr;
        if( !actorName.empty() )
            actor->setName( actorName );
        WP_UNUSED( actorType );  // deferred: type-based factory
        return actor;
    }

    void SceneBuilderBindings::removeActor( SmartPtr<scene::IGameScene> scene,
                                            SmartPtr<scene::IGameActor> actor )
    {
        if( !scene || !actor ) return;
        scene->removeActor( actor );
    }

    SmartPtr<scene::IGameActor> SceneBuilderBindings::duplicateActor(
        SmartPtr<scene::IGameActor> actor )
    {
        if( !actor ) return nullptr;
        auto copy = actor->clone();
        if( copy )
            copy->setName( actor->getName() + "_copy" );
        return copy;
    }

    // -------------------------------------------------------------------------
    // Component management

    SmartPtr<scene::IComponent> SceneBuilderBindings::addComponent(
        SmartPtr<scene::IGameActor> actor,
        const String &componentClassName )
    {
        if( !actor ) return nullptr;
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;
        auto compSys = appMgr->getComponentSystem();
        if( !compSys ) return nullptr;
        return compSys->createComponent( componentClassName, actor );
    }

    void SceneBuilderBindings::removeComponent( SmartPtr<scene::IGameActor> actor,
                                               SmartPtr<scene::IComponent> component )
    {
        if( !actor || !component ) return;
        actor->removeComponent( component );
    }

    // -------------------------------------------------------------------------
    // Transform

    void SceneBuilderBindings::setActorTransform(
        SmartPtr<scene::IGameActor> actor,
        const Vector3F &position,
        const QuaternionF &rotation,
        const Vector3F &scale )
    {
        if( !actor ) return;
        actor->setPosition( position );
        actor->setRotation( rotation );
        actor->setScale( scale );
    }

    Vector3F SceneBuilderBindings::getActorPosition( SmartPtr<scene::IGameActor> actor )
    {
        if( !actor ) return Vector3F::ZERO;
        return actor->getPosition();
    }

    QuaternionF SceneBuilderBindings::getActorRotation( SmartPtr<scene::IGameActor> actor )
    {
        if( !actor ) return QuaternionF::IDENTITY;
        return actor->getRotation();
    }

    Vector3F SceneBuilderBindings::getActorScale( SmartPtr<scene::IGameActor> actor )
    {
        if( !actor ) return Vector3F::ONE;
        return actor->getScale();
    }

    // -------------------------------------------------------------------------
    // Prefab

    SmartPtr<scene::IGamePrefab> SceneBuilderBindings::createPrefab(
        SmartPtr<scene::IGameActor> actor,
        const String &prefabName )
    {
        if( !actor ) return nullptr;
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return nullptr;
        auto prefabMgr = appMgr->getPrefabManager();
        if( !prefabMgr ) return nullptr;
        return prefabMgr->createPrefab( actor, prefabName );
    }

    SmartPtr<scene::IGameActor> SceneBuilderBindings::instantiatePrefab(
        SmartPtr<scene::IGameScene> scene,
        SmartPtr<scene::IGamePrefab> prefab,
        const Vector3F &position )
    {
        if( !scene || !prefab ) return nullptr;
        auto actor = prefab->instantiate();
        if( actor )
        {
            actor->setPosition( position );
            scene->addActor( actor );
        }
        return actor;
    }

    // -------------------------------------------------------------------------
    // Procedural world

    void SceneBuilderBindings::generateWorld( const String &seed, int worldSize,
                                              real_Num seaLevel, real_Num heightScale,
                                              const String &biomeParams )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto procMgr = appMgr->getProceduralManager();
        if( !procMgr ) return;
        auto terrainGen = procMgr->getTerrainGenerator();
        if( !terrainGen ) return;

        WP_UNUSED( seed );
        WP_UNUSED( worldSize );
        WP_UNUSED( seaLevel );
        WP_UNUSED( heightScale );
        WP_UNUSED( biomeParams );

        // ref: Esoterica/Code/EngineTools/MapEditor/ — procedural world generation
        // Deferred: runtime needs actual terrain/world generation params wired through.
    }

    void SceneBuilderBindings::generateCity( int seed, int radius, int density,
                                             int blockSize, int roadWidth )
    {
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto procMgr = appMgr->getProceduralManager();
        if( !procMgr ) return;
        auto cityGen = procMgr->getCityGenerator();
        if( !cityGen ) return;

        WP_UNUSED( seed );
        WP_UNUSED( radius );
        WP_UNUSED( density );
        WP_UNUSED( blockSize );
        WP_UNUSED( roadWidth );

        // Deferred: runtime city generator params.
    }

    void SceneBuilderBindings::buildRoadNetwork( SmartPtr<procedural::IRoadNetwork> network )
    {
        if( !network ) return;
        auto appMgr = core::IApplicationManager::instance();
        if( !appMgr ) return;
        auto meshGen = appMgr->getMeshGenerator();
        if( meshGen )
        {
            meshGen->clear();
            meshGen->setGenerateCollision( true );
            meshGen->setGenerateUVs( true );
            meshGen->setGenerateNormals( true );
            meshGen->setGenerateTangents( true );
            // road → mesh conversion deferred to runtime
        }
        WP_UNUSED( network );
    }

}  // namespace workphone::editor
