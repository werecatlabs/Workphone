#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::physics
{
    const hash_type IPhysicsBody3::STATE_MESSAGE_ATTACH_SHAPE = StringUtil::getHash( "attachShape" );
    const hash_type IPhysicsBody3::STATE_MESSAGE_DETACH_SHAPE = StringUtil::getHash( "detachShape" );
    const hash_type IPhysicsBody3::STATE_MESSAGE_MASS = StringUtil::getHash( "mass" );
    const hash_type IPhysicsBody3::STATE_MESSAGE_INERTIA_TENSOR = StringUtil::getHash( "inertia" );

    const u32 IPhysicsBody3::PhysicsBodyFlagEnabled = 1 << 0;
    const u32 IPhysicsBody3::PhysicsBodyFlagKinematic = 1 << 1;

    const u32 IPhysicsBody3::PhysicsBodyMotionFlagClearForce = 1 << 0;
    const u32 IPhysicsBody3::PhysicsBodyMotionFlagClearTorque = 1 << 1;
    const u32 IPhysicsBody3::PhysicsBodyMotionFlagSetVelocity = 1 << 2;
    const u32 IPhysicsBody3::PhysicsBodyMotionFlagSetAngularVelocity = 1 << 3;

    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsBody3, ISharedObject );

    IPhysicsBody3::~IPhysicsBody3() = default;

}  // namespace workphone::physics
