//
// Created by Zane Desir on 31/10/2021.
//
#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/RigidDynamic3.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Physics/IRaycastHit.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, RigidDynamic3, RigidBody3<IRigidDynamic3> );

    RigidDynamic3::RigidDynamic3() = default;
    RigidDynamic3::~RigidDynamic3() = default;

    void RigidDynamic3::setKinematicTarget( const Transform3<real_Num> &destination )
    {
    }

    auto RigidDynamic3::getKinematicTarget( Transform3<real_Num> &target ) -> bool
    {
        return false;
    }

    void RigidDynamic3::setLinearDamping( real_Num damping )
    {
    }

    auto RigidDynamic3::getLinearDamping() const -> real_Num
    {
        return 0;
    }

    void RigidDynamic3::setAngularDamping( real_Num damping )
    {
    }

    auto RigidDynamic3::getAngularDamping() const -> real_Num
    {
        return 0;
    }

    void RigidDynamic3::setMaxAngularVelocity( real_Num maxAngVel )
    {
    }

    auto RigidDynamic3::getMaxAngularVelocity() const -> real_Num
    {
        return 0;
    }

    auto RigidDynamic3::isSleeping() const -> bool
    {
        return false;
    }

    void RigidDynamic3::setSleepThreshold( real_Num threshold )
    {
    }

    auto RigidDynamic3::getSleepThreshold() const -> real_Num
    {
        return 0;
    }

    void RigidDynamic3::setStabilizationThreshold( real_Num threshold )
    {
    }

    auto RigidDynamic3::getStabilizationThreshold() const -> real_Num
    {
        return 0;
    }

    void RigidDynamic3::setWakeCounter( real_Num wakeCounterValue )
    {
    }

    auto RigidDynamic3::getWakeCounter() const -> real_Num
    {
        return 0;
    }

    void RigidDynamic3::wakeUp()
    {
    }

    void RigidDynamic3::putToSleep()
    {
    }

    void RigidDynamic3::setSolverIterationCounts( u32 minPositionIters, u32 minVelocityIters /*= 1 */ )
    {
    }

    void RigidDynamic3::getSolverIterationCounts( u32 &minPositionIters, u32 &minVelocityIters ) const
    {
    }

    auto RigidDynamic3::getContactReportThreshold() const -> real_Num
    {
        return 0;
    }

    void RigidDynamic3::setContactReportThreshold( real_Num threshold )
    {
    }

}  // namespace workphone::physics
