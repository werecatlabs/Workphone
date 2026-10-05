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
#include <Workphone/Scene/GameScene.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ActorLoadJob, Job );

    ActorLoadJob::ActorLoadJob()
    {
        setPrimary( true );
        if( auto app = core::IApplicationManager::instancePtr() )
            if( auto manager = app->getGameManager() )
                setScene( manager->getCurrentScene() );
    }

    ActorLoadJob::~ActorLoadJob() = default;

    void ActorLoadJob::execute()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app->getGameManager();
        auto parent = getParent();
        auto scene = getScene();
        if( !scene || !getProperties() ) 
            return;
        
        ScopedLock lock( scene.get() );
        auto concrete = workphone::dynamic_pointer_cast<scene::GameScene>( scene );
        if( !scene->isLoaded() || ( concrete && concrete->getLoadGeneration() != m_sceneGeneration ) )
            return;

        // One owner loads the entire hierarchy; no nested worker jobs or timed waits.
        auto actors = scene::GameActorUtil::loadSceneActors( { getProperties() }, scene );
        auto actor = actors.front();
        if( !scene->isLoaded() || ( concrete && concrete->getLoadGeneration() != m_sceneGeneration ) )
        {
            manager->destroyActor( actor );
            return;
        }
        try {
            if( parent ) parent->addChild(actor);
            else scene->addActor(actor);
        } catch(...) {
            manager->destroyActor(actor);
            throw;
        }
        if( !scene->isLoaded() || ( concrete && concrete->getLoadGeneration() != m_sceneGeneration ) )
        {
            manager->destroyActor( actor );
            return;
        }
        setActor(actor);
        setChildJobs({});
    }

    SmartPtr<scene::IGameScene> ActorLoadJob::getScene() const
    {
        return m_scene;
    }

    void ActorLoadJob::setScene( SmartPtr<scene::IGameScene> scene )
    {
        m_scene = scene;
        auto concrete = workphone::dynamic_pointer_cast<scene::GameScene>( scene );
        m_sceneGeneration = concrete ? concrete->getLoadGeneration() : 0;
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
        if( parent && parent->getScene() )
            setScene( parent->getScene() );
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
