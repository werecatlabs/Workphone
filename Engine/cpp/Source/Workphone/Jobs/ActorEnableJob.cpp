#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/ActorEnableJob.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ActorEnableJob, Job );

    ActorEnableJob::ActorEnableJob() = default;

    ActorEnableJob::~ActorEnableJob() = default;

    void ActorEnableJob::execute()
    {
        if( auto actor = getActor() )
        {
            auto enable = getEnable();
            actor->setEnabled( enable );
        }
    }

    SmartPtr<scene::IGameActor> ActorEnableJob::getActor() const
    {
        auto p = m_actor.load();
        return p;
    }

    void ActorEnableJob::setActor( SmartPtr<scene::IGameActor> actor )
    {
        m_actor = actor;
    }

    bool ActorEnableJob::getEnable() const
    {
        return m_enable;
    }

    void ActorEnableJob::setEnable( bool enable )
    {
        m_enable = enable;
    }

}  // namespace workphone
