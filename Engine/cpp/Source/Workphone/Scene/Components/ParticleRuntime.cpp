#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ParticleSystem.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <stdexcept>

#ifndef WP_TYPED_DOMAIN_RESOURCE
#    define WP_TYPED_DOMAIN_RESOURCE 1
#endif

#if WP_TYPED_DOMAIN_RESOURCE
namespace workphone::scene
{
    void ParticleSystem::configureRuntimeAssets(std::shared_ptr<const resource::RuntimeAssetContext> context)
    {
        if(!context||!context->resolve||isLoaded()||getLoadingState()==LoadingState::Loading||getLoadingState()==LoadingState::Unloading||(m_runtimeAssets&&m_runtimeAssets!=context))
            throw std::runtime_error("Particle runtime assets require an unloaded component and captured resolver");
        m_runtimeAssets=std::move(context);m_runtimeAssetsMode=true;
    }
    void ParticleSystem::loadRuntimeParticle(SmartPtr<ISharedObject> data)
    {
        if(!m_runtimeAssets)throw std::runtime_error("Particle runtime assets must be reconfigured before load");
        if(isLoaded())return;
        try {setLoadingState(LoadingState::Loading);if(!getActor())throw std::runtime_error("Runtime particles require their actor");Component::load(data);
            if(!m_particleSystem||!m_runtimeResource)throw std::runtime_error("Runtime particles require a captured particleAssetUuid");
            setLoadingState(LoadingState::Loaded);updateVisibility();
        }catch(...){unload(nullptr);throw;}
    }
    void ParticleSystem::updateFlags(u32 flags,u32 oldFlags)
    {
        Component::updateFlags(flags,oldFlags);if(m_runtimeAssetsMode)updateVisibility();
    }
    void ParticleSystem::setRuntimeParticleProperties(SmartPtr<Properties> properties)
    {
        if(!properties)return;if(!m_runtimeAssets)throw std::runtime_error("Runtime particle properties require a configured captured context");
        for(const auto &property:properties->getPropertiesAsArray()){
            const auto &key=property.getName();if(key!="particleAssetUuid"&&key!=playOnLoadStr&&key!="uuid"&&key!="componentType"&&key!=enabledStr&&key!=componentFlagsStr&&key!=stateStr&&key!="FSM Priority"&&key!=nameStr&&key!="handle_name")
                throw std::runtime_error("Runtime particles do not accept template names or authoring overrides");}
        if(!properties->getChildren().empty())throw std::runtime_error("Runtime particles do not accept inline authoring techniques");
        auto identity=m_runtimeParticleUuid;properties->getPropertyValue("particleAssetUuid",identity);identity=StringUtil::make_lower(identity);
        if(identity.size()!=36||identity=="00000000-0000-0000-0000-000000000000")throw std::runtime_error("Particle assets require canonical UUID references");
        for(size_t i=0;i<36;++i){const auto c=identity[i];const bool dash=i==8||i==13||i==18||i==23;if(dash?c!='-':!((c>='0'&&c<='9')||(c>='a'&&c<='f')))throw std::runtime_error("Particle assets require canonical UUID references");}
        auto playOnLoad=m_playOnLoad;properties->getPropertyValue(playOnLoadStr,playOnLoad);String error;
        auto resource=m_runtimeAssets->resolve(identity,error);
        if(!resource||resource->header.resourceType!=resource::ResourceTypeID("partres")||resource->header.resourceId.isSubResource())throw std::runtime_error(error.empty()?"particleAssetUuid requires a partres resource":error.c_str());
        auto app=core::IApplicationManager::instancePtr();auto graphics=app?app->getGraphicsSystem():nullptr;auto scene=graphics?graphics->getGraphicsScene():nullptr;
        if(!graphics||!scene||!getActor())throw std::runtime_error("Runtime particles require their actor and graphics scene");
        ScopedLock graphicsLock(graphics);
        if(m_particleSystem&&resource==m_runtimeResource&&identity==m_runtimeParticleUuid){Component::setProperties(properties);if(m_playOnLoad!=playOnLoad){m_playOnLoad=playOnLoad;m_runtimeWantsPlay=playOnLoad;m_runtimePaused=false;}updateVisibility();return;}
        auto candidate=scene->addGraphicsObjectByType<render::IParticleSystem>();if(!candidate)throw std::runtime_error("Cannot allocate native runtime particle system");
        struct Retire {SmartPtr<render::IGraphicsScene> scene;SmartPtr<render::IParticleSystem> system;SmartPtr<render::IGraphicsSceneNode> node;bool committed=false;
            ~Retire(){if(!committed){if(node&&system)node->detachObject(system);if(system){system->setVisible(false);system->unload(nullptr);scene->removeGraphicsObject(system);}if(node)scene->removeSceneNode(node);}}
        } retire{scene,candidate};
        candidate->setVisible(false);
        if(!candidate->loadRuntimeResource(resource,m_runtimeAssets->mount,m_runtimeAssets->target,m_runtimeAssets->packagedBuild,error))throw std::runtime_error(error.empty()?"Cannot stage captured native particles":error.c_str());
        auto root=scene->getRootSceneNode();auto node=root?root->addChildSceneNode():nullptr;if(!node)throw std::runtime_error("Cannot allocate runtime particle scene node");retire.node=node;node->load(nullptr);
        if(auto transform=getActor()->getTransform())node->setTransform(transform->getWorldTransform());node->attachObject(candidate);
        Component::setProperties(properties);
        auto previous=m_particleSystem;auto previousNode=m_graphicsNode;
        m_particleSystem=candidate;m_graphicsObject=candidate;m_graphicsNode=node;m_runtimeResource=std::move(resource);m_runtimeParticleUuid=std::move(identity);
        m_playOnLoad=playOnLoad;m_runtimeWantsPlay=playOnLoad;m_runtimePaused=false;retire.committed=true;updateVisibility();
        if(previous){previous->setVisible(false);previous->setState(render::ParticleSystemState::Stopped);if(previousNode)previousNode->detachObject(previous);previous->unload(nullptr);scene->removeGraphicsObject(previous);}
        if(previousNode)scene->removeSceneNode(previousNode);
    }
}
#else
namespace workphone::scene
{
    void ParticleSystem::configureRuntimeAssets(
        std::shared_ptr<const resource::RuntimeAssetContext>)
    {
        throw std::runtime_error("Cooked particle resources are unavailable in this build");
    }
    void ParticleSystem::loadRuntimeParticle(SmartPtr<ISharedObject>)
    {
        throw std::runtime_error("Cooked particle resources are unavailable in this build");
    }
    void ParticleSystem::setRuntimeParticleProperties(SmartPtr<Properties>)
    {
        throw std::runtime_error("Cooked particle resources are unavailable in this build");
    }
    void ParticleSystem::updateFlags(u32 flags,u32 oldFlags)
    {
        Component::updateFlags(flags,oldFlags);
    }
}
#endif
