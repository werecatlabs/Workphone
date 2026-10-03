//
// Created by Zane Desir on 31/10/2021.
//
#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/RigidStatic3.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Physics/IRaycastHit.hpp>
#include <Workphone/Interface/Physics/ISphereShape3.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone
{
    namespace physics
    {

        WP_CLASS_REGISTER_DERIVED( workphone::physics, RigidStatic3, RigidBody3<IRigidStatic3> );

        RigidStatic3::RigidStatic3() = default;
        RigidStatic3::~RigidStatic3() = default;

    }  // namespace physics
}  // namespace workphone
