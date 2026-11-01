#pragma once
#include "RuntimeSceneMembershipContracts.hpp"
#include "RuntimeComponentMetadataContracts.hpp"
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/Resources/TerrainResource.hpp>
#include <WPGraphics/Resources/FoliageResource.hpp>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <stdexcept>

namespace runtime_terrain_component_contracts
{
    using namespace workphone;
    inline void require(bool value,const String &message){if(!value)throw std::runtime_error(message.c_str());}
    template<class F>bool rejects(F action){try{action();}catch(const std::exception &){return true;}return false;}

    /** Reuse the fixture's colour texres; all other files come from existing
     * terrain/foliage serializers and production source exporters. */
    inline void writeSources(const std::filesystem::path &folder,const resource::ResourceID &texture)
    {
        render::TerrainData terrain;terrain.dimensions={5,5};terrain.origin={-2,-2};terrain.heights.assign(25,.25f);
        terrain.layers[0].albedo=texture.str();terrain.holes.assign(16,0);terrain.holes[15]=1;
        String error;const auto data=render::serializeTerrainData(terrain);require(!data.empty(),"Cannot serialize typed terrain fixture");
        {std::ofstream file(folder/"ground.terrain",std::ios::binary);file.write(data.data(),data.size());require(bool(file),"Cannot write typed terrain fixture");}
        auto species=std::make_shared<render::FoliageSpecies>();species->id="captured-scene-tree";
        species->boundsMinimum={-.25f,-.25f,0};species->boundsMaximum={.25f,.25f,0};species->materials.resize(1);species->materials[0].albedo=texture.str();
        species->maximumDistance=32;species->shadowDistance=32;
        for(auto &mesh:species->lods){mesh.positions={{-.25f,-.25f,0},{.25f,-.25f,0},{0,.25f,0}};mesh.normals={{0,0,1},{0,0,1},{0,0,1}};mesh.uvs={{0,1},{1,1},{.5f,0}};mesh.indices={0,1,2};}
        auto world=std::make_shared<render::FoliageWorldData>();world->species.push_back(species);
        for(u32 i=0;i<2;++i){render::FoliageInstance instance;instance.id=i+1;instance.position={i?65.f:0.f,0,.5f};world->instances.push_back(instance);}
        require(render::writeFoliageSources(world,folder.generic_u8string().c_str(),resource::ResourceID("data://forest.folworld"),error),error);
    }

    inline void run(SmartPtr<core::IApplicationManager> app,std::shared_ptr<const resource::RuntimeAssetContext> context,
                    const String &terrainUuid,const String &foliageUuid,const String &wrongUuid,const String &scriptUuid={})
    {
        require(app&&context&&context->resolve&&context->mount,"Terrain component tests require the Editor's captured package context");
        auto manager=app->getGameManager();auto graphics=app->getGraphicsSystem();
        auto renderer=dynamic_pointer_cast<render::ClawRendererDX11>(graphics->getRenderer());auto nativeScene=dynamic_pointer_cast<render::ClawScene>(graphics->getGraphicsScene());
        require(manager&&renderer&&nativeScene,"Terrain component tests require the existing native Editor scene");
        ScopedLock managerLock(manager);ScopedLock graphicsLock(graphics);
        auto actor=manager->createActor();require(actor!=nullptr,"Terrain fixture actor allocation failed");actor->setName("CapturedTerrainComponent");
        struct Cleanup {SmartPtr<scene::IGameManager> manager;SmartPtr<scene::IGameActor> actor;~Cleanup(){if(actor)manager->destroyActor(actor);}} cleanup{manager,actor};
        auto component=make_ptr<scene::TerrainSystem>();component->configureRuntimeAssets(context);actor->addComponentInstance(component);
        auto data=make_ptr<Properties>();data->setProperty("terrainAssetUuid",terrainUuid);data->setProperty("foliageWorldAssetUuid",foliageUuid);data->setProperty("showWireframe",false);data->setProperty("state",static_cast<s32>(scene::IComponent::State::Edit));
        component->load(data);auto terrain=dynamic_pointer_cast<render::ClawTerrain>(component->getTerrain());
        require(terrain&&terrain->getRuntimeFoliageCandidate(),"Captured terrain component must stage its native terrain and lazy foliage candidate");
        require(!terrain->isVisible()&&component->getTerrainSnapshot()->dimensions==Vector2I(5,5),"Unpublished terrain must remain hidden and expose the typed authoritative heightfield");
        const auto foliage=terrain->getRuntimeFoliageCandidate();require(foliage->source.snapshot->instances.empty()&&foliage->source.cells.size()==2,"Scene foliage must retain a lazy directory instead of eagerly decoding every page");
        auto escaped=component->getTerrainSnapshot();manager->getCurrentScene()->addActor(actor);
        runtime_scene_membership_contracts::nested(manager,actor,[&](bool visible){require(terrain->isVisible()==visible,"Nested terrain must follow its real scene ancestor's publication and enabled state");});
        render::TerrainTileView terrainView;terrainView.cull=false;String error;
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(std::chrono::steady_clock::now()<deadline&&terrain->getNativeRenderTiles().empty()){
            require(terrain->prepareRenderTiles(*renderer,{terrainView},error),error);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        require(!terrain->getNativeRenderTiles().empty()&&terrain->getTerrainResidency().usedCookedTileManifest,"Captured terrain must publish real streamed native geometry from its sealed tile manifest");
        const auto oldTarget=renderer->getRenderTarget();const auto oldCamera=renderer->getCamera();
        struct RestoreRenderer {SmartPtr<render::ClawRendererDX11> renderer;SmartPtr<render::IRenderTarget> target;SmartPtr<render::IGraphicsCamera> camera;~RestoreRenderer(){renderer->setCamera(camera);renderer->setRenderTarget(target);}} restoreRenderer{renderer,oldTarget,oldCamera};
        auto target=make_ptr<render::ClawRenderTarget>();target->setSize({64,64});renderer->setRenderTarget(target);renderer->setCamera(nullptr);renderer->disableShadows();
        auto native=wp_renderer_get_dx11(renderer->getNativeRenderer());require(native!=nullptr,"Terrain native consumer requires its existing DX11 device");
        wp_mat4f identity{};for(int i=0;i<4;++i)identity.m[i][i]=1;
        wp_renderer_dx11_set_world_matrix(native,&identity);wp_renderer_dx11_set_view_matrix(native,&identity);wp_renderer_dx11_set_projection_matrix(native,&identity);
        wp_renderer_dx11_set_cull_mode(native,WORKPHONE_CULL_MODE_NONE);wp_renderer_dx11_set_depth_test_enabled(native,0);wp_renderer_dx11_set_depth_write_enabled(native,0);
        renderer->setSceneLighting(ColourF::White,{0,0,-1},ColourF::White,0);
        const auto drawDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        do {nativeScene->render(static_cast<render::IRenderer *>(renderer.get()));if(terrain->getFoliageDiagnostics().actualDraws)break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<drawDeadline);
        require(terrain->getFoliageDiagnostics().actualDraws>0&&terrain->getFoliageDiagnostics().residentInstances>0,"Captured terrain foliage must flow through the existing scene inventory, asynchronous page loader and actual native draws");
        auto invalid=make_ptr<Properties>();invalid->setProperty("terrainAssetUuid",wrongUuid);
        require(rejects([&]{component->setProperties(invalid);})&&component->getTerrain()==terrain,"Wrong typed UUID must preserve the installed terrain");
        invalid=make_ptr<Properties>();invalid->setProperty("terrainDataV1","must-not-parse");
        require(rejects([&]{component->setProperties(invalid);})&&component->getTerrain()==terrain,"Runtime terrain must reject inline authoring/source data");
        auto scheduler=WorldStreamingScheduler::shared();const auto before=scheduler->stats();
        struct RestoreBudget {std::shared_ptr<WorldStreamingScheduler> scheduler;u64 limit;~RestoreBudget(){String ignored;scheduler->setBudget(StreamMemoryClass::Gpu,limit,ignored);}} restoreBudget{scheduler,before.limits[size_t(StreamMemoryClass::Gpu)]};
        require(scheduler->setBudget(StreamMemoryClass::Gpu,before.used[size_t(StreamMemoryClass::Gpu)],error),error);
        auto replacement=make_ptr<Properties>();replacement->setProperty("terrainAssetUuid",terrainUuid);replacement->setProperty("foliageWorldAssetUuid",String());
        require(rejects([&]{component->setProperties(replacement);})&&component->getTerrain()==terrain&&component->getProperties()->getProperty("foliageWorldAssetUuid")==foliageUuid,"Native staging pressure must retain the complete previous terrain/foliage generation");
        require(scheduler->setBudget(StreamMemoryClass::Gpu,restoreBudget.limit,error),error);
        if(!scriptUuid.empty()){
            auto staged=make_ptr<Properties>();staged->setName("actors");staged->setProperty("uuid",StringUtil::getUUID());staged->setProperty("name","RejectedTerrainAppend");
            auto terrainData=make_ptr<Properties>();terrainData->setName("component");terrainData->setProperty("uuid",StringUtil::getUUID());terrainData->setProperty("componentType","workphone::scene::TerrainSystem");terrainData->setProperty("terrainAssetUuid",terrainUuid);terrainData->setProperty("foliageWorldAssetUuid",foliageUuid);staged->addChild(terrainData);
            auto scriptData=make_ptr<Properties>();scriptData->setName("component");scriptData->setProperty("uuid",StringUtil::getUUID());scriptData->setProperty("componentType","workphone::scene::Script");scriptData->setProperty("scriptAssetUuid",scriptUuid);scriptData->setProperty("className","MissingTerrainRollbackClass");staged->addChild(scriptData);
            require(rejects([&]{scene::GameActorUtil::loadSceneActors({staged},manager->getCurrentScene(),context);})&&component->getTerrain()==terrain&&terrain->isVisible(),"A later failing Script must roll back the staged domain component without changing the last ready terrain");
        }
        component->setEnabled(false);auto roundtrip=runtime_component_metadata_contracts::cook(component,"workphone::scene::TerrainSystem",context,{terrainUuid,foliageUuid});
        component->unload(nullptr);require(!terrain->getRuntimeFoliageCandidate()&&escaped->dimensions==Vector2I(5,5),"Retirement must release native foliage while escaped authoritative snapshots remain valid");
        require(rejects([&]{component->load(data);}),"Plain runtime reload must require explicit captured reconfiguration");
        component->setActor(actor);component->configureRuntimeAssets(context);component->load(roundtrip);
        require(component->getTerrain()!=nullptr&&!component->isEnabled()&&!component->getTerrain()->isVisible(),"Actual toData/cook/reload must consume Enabled=false and recreate hidden native terrain without authoring storage");
        // A queued graphics request can be the last native owner after the
        // component's pooled storage has been returned. Its gate destruction
        // must not consult that reclaimed component (the original teardown AV).
        auto queuedTerrain=dynamic_pointer_cast<render::ClawTerrain>(component->getTerrain());
        require(queuedTerrain!=nullptr,"Queued terrain retirement requires the actual native terrain");graphics->loadObject(queuedTerrain,true);
        manager->destroyActor(actor);cleanup.actor=nullptr;actor=nullptr;component=nullptr;
        auto replacementComponent=make_ptr<scene::TerrainSystem>();
        void *nativeObject=queuedTerrain.get();queuedTerrain->_getObject(&nativeObject);
        require(!queuedTerrain->isVisible()&&!nativeObject&&!queuedTerrain->getRuntimeFoliageCandidate()&&
                queuedTerrain->getNativeRenderTiles().empty()&&!queuedTerrain->getRenderedTerrainSnapshot()&&
                replacementComponent->getTerrain()==nullptr,
                "Retained queued terrain must retire independently of a new pooled component");
        queuedTerrain=nullptr;terrain=nullptr;graphics->clearObjectQueues();
        require(replacementComponent->getTerrain()==nullptr&&escaped->dimensions==Vector2I(5,5),
                "Destroying queued native publication gates after component retirement must preserve independent owners");
    }
}
