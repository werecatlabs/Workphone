#pragma once
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <chrono>
#include <thread>
#include <stdexcept>

namespace runtime_mesh_lod_contracts
{
    using namespace workphone;
    inline void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
    inline void run(SmartPtr<core::IApplicationManager> app,std::shared_ptr<const resource::RuntimeAssetContext> context,
                    const String &highUuid,const String &coarseUuid)
    {
        require(app&&context&&context->mount,"LOD contracts require the Editor's captured package mount");
        auto manager=app->getGameManager();auto actor=manager->createActor();require(actor!=nullptr,"LOD fixture actor must allocate");actor->setName("RuntimeLODContract");
        auto high=make_ptr<scene::MeshRenderer>(),coarse=make_ptr<scene::MeshRenderer>();high->configureRuntimeAssets(context);coarse->configureRuntimeAssets(context);
        actor->addComponentInstance(high);actor->addComponentInstance(coarse);
        auto group=make_ptr<scene::LODGroup>();group->configureRuntimeAssets(context);group->setActor(actor);
        auto system=make_ptr<scene::LODSystem>();system->load(nullptr);
        struct Cleanup {SmartPtr<scene::IGameManager> manager;SmartPtr<scene::IGameActor> actor;SmartPtr<scene::LODGroup> group;SmartPtr<scene::LODSystem> system;
            ~Cleanup(){system->removeComponent(static_pointer_cast<scene::IComponent>(group));system->unload(nullptr);group->unload(nullptr);group->setActor(nullptr);manager->destroyActor(actor);}} cleanup{manager,actor,group,system};
        auto fine=make_ptr<Properties>();fine->setProperty("meshAssetUuid",highUuid);fine->setProperty("streamMesh",true);high->load(fine);
        auto low=make_ptr<Properties>();low->setProperty("meshAssetUuid",coarseUuid);coarse->load(low);
        require(!high->getGraphicsObject()&&coarse->getGraphicsObject(),"Inactive high LOD must not allocate native geometry while the coarse mesh remains ready");
        const auto coarseObject=coarse->getGraphicsObject();const auto actorIdentity=actor->getHandle()->getUUIDAsString();
        auto data=make_ptr<Properties>();data->setProperty("size",2.f);data->setProperty("hysteresis",.1f);data->setProperty("cullBelowLastLOD",false);
        for(const auto &entry:Array<std::pair<f32,SmartPtr<scene::Renderer>>>{{.3f,high},{0.f,coarse}}){auto level=make_ptr<Properties>();level->setName("lodLevel");level->setProperty("screenRelativeHeight",entry.first);level->setProperty("renderers",Array<String>{entry.second->getHandle()->getUUIDAsString()});data->addChild(level);}
        struct Resolver {Properties::ObjectResolver old;~Resolver(){Properties::exchangeObjectResolver(std::move(old));}} resolution{Properties::exchangeObjectResolver([high,coarse](const String &id)->SmartPtr<ISharedObject>{if(id==high->getHandle()->getUUIDAsString())return high;if(id==coarse->getHandle()->getUUIDAsString())return coarse;return nullptr;})};
        group->load(data);group->finalizeRuntimeLOD();system->addComponent(group);scene::LODSystem::View view;system->setViewOverride(view);
        manager->getCurrentScene()->addActor(actor);group->setForcedLOD(1);system->update();
        require(group->getDisplayedLOD()==1&&coarse->isLODVisible()&&!high->isLODVisible(),"Existing LOD selection must initially display its ready coarse level");
        auto scheduler=WorldStreamingScheduler::shared();const auto old=scheduler->stats().limits[size_t(StreamMemoryClass::Gpu)];String error;
        struct Restore {std::shared_ptr<WorldStreamingScheduler> scheduler;u64 limit;~Restore(){String ignored;scheduler->setBudget(StreamMemoryClass::Gpu,limit,ignored);}} restore{scheduler,old};
        require(scheduler->setBudget(StreamMemoryClass::Gpu,scheduler->stats().used[size_t(StreamMemoryClass::Gpu)],error),"LOD GPU pressure must be admitted by the real shared ledger");
        group->setForcedLOD(0);const auto wait=std::chrono::steady_clock::now()+std::chrono::milliseconds(100);
        while(std::chrono::steady_clock::now()<wait){system->update();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        require(group->getDisplayedLOD()==1&&coarse->isLODVisible()&&!high->getGraphicsObject(),"Pending/rejected high geometry must preserve the complete coarse fallback");
        require(scheduler->setBudget(StreamMemoryClass::Gpu,old,error),"LOD GPU admission must restore");
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(std::chrono::steady_clock::now()<deadline&&group->getDisplayedLOD()!=0){system->update();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        auto native=dynamic_pointer_cast<render::ClawMesh>(high->getGraphicsObject());require(native&&native->getNativeMesh()&&group->getDisplayedLOD()==0&&high->isLODVisible()&&!coarse->isLODVisible(),"Verified async high mesh must publish through the existing LOD selector only after native staging");
        group->setForcedLOD(1);system->update();require(group->getDisplayedLOD()==1&&!high->getGraphicsObject()&&!native->getNativeMesh()&&coarse->getGraphicsObject()==coarseObject,
            "LOD demotion must release high native geometry while preserving its original ready coarse object");
        require(actor->getHandle()->getUUIDAsString()==actorIdentity&&high->getProperties()->getProperty("meshAssetUuid")==highUuid,"Streaming residency must preserve actor/component asset identity");
        // Hysteresis remains the existing pure selector, independent of IO readiness.
        const Array<f32> thresholds{.3f,0.f};require(scene::LODSystem::selectLOD(.31f,thresholds,1,-1,.1f,false)==1&&scene::LODSystem::selectLOD(.34f,thresholds,1,-1,.1f,false)==0,
            "Residency integration must preserve original LOD hysteresis thresholds");
        group->setForcedLOD(0);system->update();group->unload(nullptr);require(!high->getGraphicsObject()&&coarse->getGraphicsObject()==coarseObject,"Unloading a group must cancel its pending high demand without retiring the coarse renderer");
    }
}
