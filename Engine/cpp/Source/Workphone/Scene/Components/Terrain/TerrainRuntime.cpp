#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <stdexcept>

#ifndef WP_TYPED_DOMAIN_RESOURCE
#    define WP_TYPED_DOMAIN_RESOURCE 1
#endif

#if WP_TYPED_DOMAIN_RESOURCE
namespace workphone::scene
{
    namespace
    {
        String assetUUID(String value,bool optional=false)
        {
            if(optional&&value.empty())return value;
            if(value.size()!=36)throw std::runtime_error("Terrain assets require canonical UUID references");
            for(size_t i=0;i<value.size();++i){const auto c=value[i];const bool dash=i==8||i==13||i==18||i==23;
                if(dash?c!='-':!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))
                    throw std::runtime_error("Terrain assets require canonical UUID references");}
            value=StringUtil::make_lower(value);
            if(value=="00000000-0000-0000-0000-000000000000")throw std::runtime_error("Terrain asset UUID cannot be empty");
            return value;
        }
    }

    void TerrainSystem::configureRuntimeAssets(std::shared_ptr<const resource::RuntimeAssetContext> context)
    {
        if(!context||!context->resolve||isLoaded()||getLoadingState()==LoadingState::Loading||
           getLoadingState()==LoadingState::Unloading||(m_runtimeAssets&&m_runtimeAssets!=context))
            throw std::runtime_error("Terrain runtime assets require an unloaded component and captured resolver");
        m_runtimeAssets=std::move(context);m_runtimeAssetsMode=true;
    }

    void TerrainSystem::loadRuntimeTerrain(SmartPtr<ISharedObject> data)
    {
        if(!m_runtimeAssets)throw std::runtime_error("Terrain runtime assets must be reconfigured before load");
        if(isLoaded())return;
        try {
            if(!getActor())throw std::runtime_error("Runtime terrain requires its actor");
            setLoadingState(LoadingState::Loading);Component::load(data);
            if(!m_terrain||!m_runtimeTerrainResource)throw std::runtime_error("Runtime terrain requires a captured terrainAssetUuid");
            if( auto app = core::IApplicationManager::instancePtr() )
            {
                if( auto manager = app->getGameManager() )
                {
                    manager->registerComponentUpdate( TaskId::Physics, Thread::UpdateState::Update,
                                                      this );
                }
            }
            setLoadingState(LoadingState::Loaded);updateVisibility();
        }catch(...){unload(nullptr);throw;}
    }

    void TerrainSystem::setRuntimeTerrainProperties(SmartPtr<Properties> properties)
    {
        if(!properties)return;
        if(!m_runtimeAssets)throw std::runtime_error("Runtime terrain properties require a configured captured context");
        for(const auto &property:properties->getPropertiesAsArray()){
            const auto &key=property.getName();
            if(key!="terrainAssetUuid"&&key!="foliageWorldAssetUuid"&&key!=ShowWireframeStr&&key!="uuid"&&
               key!="componentType"&&key!=enabledStr&&key!=componentFlagsStr&&key!=stateStr&&key!="FSM Priority"&&key!=nameStr&&key!="handle_name")
                throw std::runtime_error("Runtime terrain does not accept authoring properties or source paths");
        }
        if(!properties->getChildren().empty())throw std::runtime_error("Runtime terrain does not accept inline authoring data");
        auto terrainUuid=m_runtimeTerrainUuid,foliageUuid=m_runtimeFoliageUuid;
        properties->getPropertyValue("terrainAssetUuid",terrainUuid);properties->getPropertyValue("foliageWorldAssetUuid",foliageUuid);
        terrainUuid=assetUUID(terrainUuid);foliageUuid=assetUUID(foliageUuid,true);
        auto wireframe=m_showWireframe;properties->getPropertyValue(ShowWireframeStr,wireframe);
        String error;auto resource=m_runtimeAssets->resolve(terrainUuid,error);
        if(!resource||resource->header.resourceType!=resource::ResourceTypeID("terrain")||resource->header.resourceId.isSubResource())
            throw std::runtime_error(error.empty()?"terrainAssetUuid requires a terrain resource":error.c_str());
        std::shared_ptr<const resource::RuntimeResource> foliage;
        if(!foliageUuid.empty()){
            foliage=m_runtimeAssets->resolve(foliageUuid,error);
            if(!foliage||foliage->header.resourceType!=resource::ResourceTypeID("folworld")||foliage->header.resourceId.isSubResource())
                throw std::runtime_error(error.empty()?"foliageWorldAssetUuid requires a folworld resource":error.c_str());
        }
        auto app=core::IApplicationManager::instancePtr();auto graphics=app?app->getGraphicsSystem():nullptr;
        auto scene=graphics?graphics->getGraphicsScene():nullptr;
        if(!graphics||!scene||!getActor())throw std::runtime_error("Runtime terrain requires its actor and graphics scene");
        ScopedLock graphicsLock(graphics);
        if(m_terrain&&resource==m_runtimeTerrainResource&&terrainUuid==m_runtimeTerrainUuid&&foliageUuid==m_runtimeFoliageUuid){
            Component::setProperties(properties);m_showWireframe=wireframe;m_terrain->setShowWireframe(wireframe);updateVisibility();return;}
        auto sourceAdmission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,65536+8*resource->payload.size(),error);
        if(!sourceAdmission)throw std::runtime_error(error.c_str());
        auto candidate=scene->addGraphicsObjectByType<render::IGraphicsTerrain>();
        if(!candidate)throw std::runtime_error("Cannot allocate native runtime terrain");
        struct Retire {
            SmartPtr<render::IGraphicsScene> scene;SmartPtr<render::IGraphicsTerrain> terrain;bool committed=false;
            ~Retire(){if(!committed&&terrain){terrain->setVisible(false);terrain->unload(nullptr);scene->removeGraphicsObject(terrain);}}
        } retire{scene,candidate};
        candidate->setVisible(false);candidate->load(nullptr);
        if(!candidate->loadRuntimeResource(resource,foliage,m_runtimeAssets->mount,m_runtimeAssets->target,m_runtimeAssets->packagedBuild,error))
            throw std::runtime_error(error.empty()?"Cannot stage captured runtime terrain":error.c_str());
        auto concrete=dynamic_pointer_cast<render::Terrain>(candidate);
        auto snapshot=concrete?concrete->getTerrainSnapshot():render::TerrainSnapshot{};
        if(!snapshot)throw std::runtime_error("Runtime terrain backend must expose its authoritative heightfield");
        struct SourceOwner {StreamBudget admission;render::TerrainSnapshot source;};
        auto sourceOwner=std::make_shared<SourceOwner>();sourceOwner->admission=std::move(sourceAdmission);sourceOwner->source=snapshot;
        snapshot=render::TerrainSnapshot(sourceOwner,snapshot.get());
        auto previous=m_terrain;const auto previousData=getTerrainSnapshot();
        {std::lock_guard<std::mutex> lock(m_terrainDataMutex);m_terrainData=snapshot;}
        setTerrain(candidate);
        if(m_terrain!=candidate){std::lock_guard<std::mutex> lock(m_terrainDataMutex);m_terrainData=previousData;throw std::runtime_error("Runtime terrain placement was rejected");}
        retire.committed=true;
        m_heightMapSize=snapshot->dimensions;m_heightScale=snapshot->heightScale;m_showWireframe=wireframe;
        m_runtimeTerrainResource=std::move(resource);m_runtimeTerrainUuid=std::move(terrainUuid);m_runtimeFoliageUuid=std::move(foliageUuid);
        candidate->setShowWireframe(wireframe);Component::setProperties(properties);updateTransform();updateVisibility();
        if(previous){previous->setVisible(false);previous->unload(nullptr);scene->removeGraphicsObject(previous);}
    }
}
#else
namespace workphone::scene
{
    void TerrainSystem::configureRuntimeAssets(
        std::shared_ptr<const resource::RuntimeAssetContext> )
    {
        throw std::runtime_error("Cooked terrain resources are unavailable in this build");
    }
    void TerrainSystem::loadRuntimeTerrain(SmartPtr<ISharedObject>)
    {
        throw std::runtime_error("Cooked terrain resources are unavailable in this build");
    }
    void TerrainSystem::setRuntimeTerrainProperties(SmartPtr<Properties>)
    {
        throw std::runtime_error("Cooked terrain resources are unavailable in this build");
    }
}
#endif
