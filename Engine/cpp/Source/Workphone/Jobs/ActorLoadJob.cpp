#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/ActorLoadJob.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ActorLoadJob, Job );

    ActorLoadJob::ActorLoadJob() { setPrimary( true ); }

    ActorLoadJob::~ActorLoadJob() = default;

    void ActorLoadJob::execute()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app->getGameManager();
        auto parent = getParent();
        auto scene = parent ? parent->getScene() : manager->getCurrentScene();
        if( !scene || !getProperties() ) 
            return;
        
        ScopedLock lock( scene.get() );
        
        // One owner loads the entire hierarchy; no nested worker jobs or timed waits.
        auto actors = scene::GameActorUtil::loadSceneActors({getProperties()});
        auto actor = actors.front();
        if( parent ) parent->addChild(actor);
        else scene->addActor(actor);
        setActor(actor);
        setChildJobs({});
    }

    SmartPtr<scene::IGameActor> ActorLoadJob::getActor() const
    {
        return m_actor;
    }

    void ActorLoadJob::setActor( SmartPtr<scene::IGameActor> actor )
    {
        m_actor = actor;
    }

    SmartPtr<scene::IGameActor> ActorLoadJob::getParent() const
    {
        return m_parent;
    }

    void ActorLoadJob::setParent( SmartPtr<scene::IGameActor> parent )
    {
        m_parent = parent;
    }

    Array<SmartPtr<ActorLoadJob>> ActorLoadJob::getChildJobs() const
    {
        return m_childJobs.snapshot();
    }

    void ActorLoadJob::setChildJobs( const Array<SmartPtr<ActorLoadJob>> &childJobs )
    {
        m_childJobs = ConcurrentArray<SmartPtr<ActorLoadJob>>( childJobs.begin(), childJobs.end() );
    }

    SmartPtr<Properties> ActorLoadJob::getProperties() const
    {
        return m_properties;
    }

    void ActorLoadJob::setProperties( SmartPtr<Properties> properties )
    {
        m_properties = properties;
    }

    bool ActorLoadJob::getCreateChildJobs() const
    {
        return m_createChildJobs;
    }

    void ActorLoadJob::setCreateChildJobs( bool createChildJobs )
    {
        m_createChildJobs = createChildJobs;
    }
}  //  namespace workphone
