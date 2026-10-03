#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/PhysicsMaterial3.hpp>
#include <Workphone/Interface/Physics/IRigidBody3.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/State/States/PhysicsMaterialStateData.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysicsMaterial3, IPhysicsMaterial3 );

    PhysicsMaterial3::PhysicsMaterial3() = default;

    PhysicsMaterial3::~PhysicsMaterial3() = default;

    f32 PhysicsMaterial3::getFriction( s32 direction ) const
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                return state->staticFriction[direction];
            }
        }

        return 0.0f;
    }

    void PhysicsMaterial3::setFriction( f32 friction, s32 direction )
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                state->staticFriction[direction] = friction;
            }
        }
    }

    f32 PhysicsMaterial3::getDynamicFriction( s32 direction ) const
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                return state->dynamicFriction[direction];
            }
        }

        return 0.0f;
    }

    void PhysicsMaterial3::setDynamicFriction( f32 friction, s32 direction )
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                state->dynamicFriction[direction] = friction;
            }
        }
    }

    f32 PhysicsMaterial3::getStaticFriction( s32 direction ) const
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                return state->staticFriction[direction];
            }
        }

        return 0.0f;
    }

    void PhysicsMaterial3::setStaticFriction( f32 friction, s32 direction )
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                state->staticFriction[direction] = friction;
            }
        }
    }

    f32 PhysicsMaterial3::getRestitution() const
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                return state->restitution;
            }
        }

        return 0.0f;
    }

    void PhysicsMaterial3::setRestitution( f32 restitution )
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                state->restitution = restitution;
            }
        }
    }

    f32 PhysicsMaterial3::getRollingFriction() const
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                return state->rollingFriction;
            }
        }
        return 0.0f;
    }
    void PhysicsMaterial3::setRollingFriction( f32 friction )
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                state->rollingFriction = friction;
            }
        }
    }
    FrictionCombineMode PhysicsMaterial3::getFrictionCombineMode() const
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                return state->frictionCombineMode;
            }
        }
        return FrictionCombineMode::Average;
    }
    void PhysicsMaterial3::setFrictionCombineMode( FrictionCombineMode mode )
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                state->frictionCombineMode = mode;
            }
        }
    }
    RestitutionCombineMode PhysicsMaterial3::getRestitutionCombineMode() const
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                return state->restitutionCombineMode;
            }
        }
        return RestitutionCombineMode::Average;
    }
    void PhysicsMaterial3::setRestitutionCombineMode( RestitutionCombineMode mode )
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                state->restitutionCombineMode = mode;
            }
        }
    }
    String PhysicsMaterial3::getMaterialName() const
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                return state->materialName;
            }
        }
        return String();
    }
    void PhysicsMaterial3::setMaterialName( const String &name )
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                state->materialName = name;
            }
        }
    }

    Vector3<real_Num> PhysicsMaterial3::getContactPosition() const
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                return state->contactPosition;
            }
        }

        return workphone::Vector3<real_Num>();
    }

    Vector3<real_Num> PhysicsMaterial3::getContactNormal() const
    {
        if( auto stateContext = getStateContext() )
        {
            auto state = stateContext->getStateByType<PhysicsMaterialStateData>();
            if( state )
            {
                return state->contactNormal;
            }
        }

        return workphone::Vector3<real_Num>();
    }

    SmartPtr<IRigidBody3> PhysicsMaterial3::getPhysicsBodyA() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateByType<PhysicsMaterialStateData>() )
            {
                return state->physicsBodyA;
            }
        }

        return nullptr;
    }

    SmartPtr<IRigidBody3> PhysicsMaterial3::getPhysicsBodyB() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateByType<PhysicsMaterialStateData>() )
            {
                return state->physicsBodyB;
            }
        }

        return nullptr;
    }

    SmartPtr<IStateContext> PhysicsMaterial3::getStateContext() const
    {
        return m_stateContext;
    }

    void PhysicsMaterial3::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    bool PhysicsMaterial3::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    bool PhysicsMaterial3::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

}  // namespace workphone::physics
