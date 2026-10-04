#ifndef WPPHYSICSSHAPE3ADAPTER_HPP
#define WPPHYSICSSHAPE3ADAPTER_HPP

#include <WPPhysics/WPPhysicsShape3.hpp>

namespace workphone::physics
{
    /** Shares the backend-independent IPhysicsShape3 forwarding code used by concrete C shapes. */
    template <class TInterface>
    class WPPhysicsShape3Adapter : public WPPhysicsShape3, public TInterface
    {
    public:
        explicit WPPhysicsShape3Adapter( wp_collision_shape_type type ) : WPPhysicsShape3( type )
        {
        }

        SmartPtr<IPhysicsMaterial3> getMaterial() const override
        {
            return WPPhysicsShape3::getMaterial();
        }

        void setMaterial( SmartPtr<IPhysicsMaterial3> material ) override
        {
            WPPhysicsShape3::setMaterial( material );
        }

        void setLocalPose( const Transform3<real_Num> &pose ) override
        {
            WPPhysicsShape3::setLocalPose( pose );
        }

        Transform3<real_Num> getLocalPose() const override
        {
            return WPPhysicsShape3::getLocalPose();
        }

        void setSimulationFilterData( const FilterData &data ) override
        {
            WPPhysicsShape3::setSimulationFilterData( data );
        }

        FilterData getSimulationFilterData() const override
        {
            return WPPhysicsShape3::getSimulationFilterData();
        }

        void setActor( SmartPtr<IPhysicsBody3> body ) override
        {
            WPPhysicsShape3::setActor( body );
        }

        SmartPtr<IPhysicsBody3> getActor() const override
        {
            return WPPhysicsShape3::getActor();
        }

        void _getObject( void **object ) const override
        {
            WPPhysicsShape3::_getObject( object );
        }

        bool hasShapeData() const override
        {
            return WPPhysicsShape3::hasShapeData();
        }

        bool isAttached() const override
        {
            return WPPhysicsShape3::isAttached();
        }

        void setEnabled( bool enabled ) override
        {
            WPPhysicsShape3::setEnabled( enabled );
        }

        bool isEnabled() const override
        {
            return WPPhysicsShape3::isEnabled();
        }

        void setTrigger( bool trigger ) override
        {
            WPPhysicsShape3::setTrigger( trigger );
        }

        bool isTrigger() const override
        {
            return WPPhysicsShape3::isTrigger();
        }

        void setStateContext( SmartPtr<IStateContext> context ) override
        {
            WPPhysicsShape3::setStateContext( context );
        }

        SmartPtr<IStateContext> getStateContext() const override
        {
            return WPPhysicsShape3::getStateContext();
        }

        void setStateListener( SmartPtr<IStateListener> listener ) override
        {
            WPPhysicsShape3::setStateListener( listener );
        }

        SmartPtr<IStateListener> getStateListener() const override
        {
            return WPPhysicsShape3::getStateListener();
        }

        void setCollisionType( u32 mask ) override
        {
            WPPhysicsShape3::setCollisionType( mask );
        }

        u32 getCollisionType() const override
        {
            return WPPhysicsShape3::getCollisionType();
        }

        void setCollisionMask( u32 mask ) override
        {
            WPPhysicsShape3::setCollisionMask( mask );
        }

        u32 getCollisionMask() const override
        {
            return WPPhysicsShape3::getCollisionMask();
        }

        bool handleStateChanged( const SmartPtr<IStateMessage> &message ) override
        {
            return WPPhysicsShape3::handleStateChanged( message );
        }

        bool handleStateChanged( SmartPtr<IState> &state ) override
        {
            return WPPhysicsShape3::handleStateChanged( state );
        }

        void lock() override
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
            {
                physicsManager->lock();
            }
        }

        bool try_lock() override
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
            {
                return physicsManager->try_lock();
            }

            return false;
        }

        void unlock() override
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
            {
                physicsManager->unlock();
            }
        }
    };
}  // namespace workphone::physics

#endif
