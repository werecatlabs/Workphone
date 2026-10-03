#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/ConstraintLimit.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace physics
    {

        WP_CLASS_REGISTER_DERIVED( workphone::physics, ConstraintLimit, IConstraintLimit );

        ConstraintLimit::ConstraintLimit() = default;

        ConstraintLimit::~ConstraintLimit() = default;

        real_Num ConstraintLimit::getRestitution() const
        {
            return m_restitution;
        }

        void ConstraintLimit::setRestitution( real_Num restitution )
        {
            m_restitution = restitution;
        }

        real_Num ConstraintLimit::getBounceThreshold() const
        {
            return m_bounceThreshold;
        }

        void ConstraintLimit::setBounceThreshold( real_Num bounceThreshold )
        {
            m_bounceThreshold = bounceThreshold;
        }

        real_Num ConstraintLimit::getStiffness() const
        {
            return m_stiffness;
        }

        void ConstraintLimit::setStiffness( real_Num stiffness )
        {
            m_stiffness = stiffness;
        }

        real_Num ConstraintLimit::getDamping() const
        {
            return m_damping;
        }

        void ConstraintLimit::setDamping( real_Num damping )
        {
            m_damping = damping;
        }

        real_Num ConstraintLimit::getContactDistance() const
        {
            return m_contactDistance;
        }

        void ConstraintLimit::setContactDistance( real_Num contactDistance )
        {
            m_contactDistance = contactDistance;
        }

    }  // namespace physics
}  // namespace workphone
