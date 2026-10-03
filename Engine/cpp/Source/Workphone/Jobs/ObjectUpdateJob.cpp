#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/ObjectUpdateJob.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ObjectUpdateJob, Job );

    ObjectUpdateJob::ObjectUpdateJob() = default;

    ObjectUpdateJob::~ObjectUpdateJob() = default;

    void ObjectUpdateJob::execute()
    {
        m_owner->update();
    }

    auto ObjectUpdateJob::getOwner() const -> ISharedObject *
    {
        return m_owner.get();
    }

    void ObjectUpdateJob::setOwner( ISharedObject *owner )
    {
        m_owner = owner;
    }

}  // namespace workphone
