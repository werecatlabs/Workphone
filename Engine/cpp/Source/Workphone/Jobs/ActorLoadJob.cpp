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

    ActorLoadJob::ActorLoadJob() = default;

    ActorLoadJob::~ActorLoadJob() = default;

    void ActorLoadJob::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto gameManager = applicationManager->getGameManagerPtr();
            WP_ASSERT( gameManager );

            auto gameScene = gameManager->getCurrentScenePtr();
            WP_ASSERT( gameScene );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto prefabManager = applicationManager->getPrefabManager();
            WP_ASSERT( prefabManager );

            auto jobQueue = applicationManager->getJobQueue();
            WP_ASSERT( jobQueue );

            auto actorJobs = Array<SmartPtr<ActorLoadJob>>();
            actorJobs.reserve( 12 );

            if( auto actorData = getProperties() )
            {
                auto createChildJobs = getCreateChildJobs();
                if( createChildJobs )
                {
                    auto actor = gameManager->createActor();
                    WP_ASSERT( actor );

                    if( auto parent = getParent() )
                    {
                        parent->addChild( actor );
                    }

                    gameManager->loadObject( actor, actorData, true );
                    setActor( actor );

                    auto childrenData =
                        actorData->getChildrenByName( scene::GameActorUtil::childrenStr );
                    auto childrenDataAlt =
                        actorData->getChildrenByName( scene::GameActorUtil::childStr );
                    childrenData.insert( childrenData.end(), childrenDataAlt.begin(),
                                         childrenDataAlt.end() );

                    for( auto &childData : childrenData )
                    {
                        auto actorJob = factoryManager->make_ptr<ActorLoadJob>();
                        actorJob->setProperties( childData );
                        actorJob->setParent( actor );
                        actorJob->setCreateChildJobs( false );
                        jobQueue->addJob( actorJob );
                        actorJobs.push_back( actorJob );
                    }
                }
                else
                {
                    auto actor = gameManager->createActor();
                    WP_ASSERT( actor );

                    if( auto parent = getParent() )
                    {
                        parent->addChild( actor );
                    }

                    scene::GameActorUtil::loadFromData( actor, actorData, true );
                    setActor( actor );
                }
            }

            auto parent = getParent();
            if( !parent )
            {
                if( auto actor = getActor() )
                {
                    gameScene->addActor( actor );
                }
            }

            setChildJobs( actorJobs );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
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
