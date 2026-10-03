#ifndef WP_CPHYSICSSHAPE3ADAPTER_HPP
#define WP_CPHYSICSSHAPE3ADAPTER_HPP

#include <WPPhysics/CPhysicsShape3.hpp>

namespace workphone::physics
{
    /** Shares the backend-independent IPhysicsShape3 forwarding code used by concrete C shapes. */
    template <class TInterface>
    class CPhysicsShape3Adapter : public CPhysicsShape3, public TInterface
    {
    public:
        explicit CPhysicsShape3Adapter( wp_collision_shape_type type ) : CPhysicsShape3( type )
        {
        }

        SmartPtr<IPhysicsMaterial3> getMaterial() const override
        {
            return CPhysicsShape3::getMaterial();
        }

        void setMaterial( SmartPtr<IPhysicsMaterial3> material ) override
        {
            CPhysicsShape3::setMaterial( material );
        }

        void setLocalPose( const Transform3<real_Num> &pose ) override
        {
            CPhysicsShape3::setLocalPose( pose );
        }

        Transform3<real_Num> getLocalPose() const override
        {
            return CPhysicsShape3::getLocalPose();
        }

        void setSimulationFilterData( const FilterData &data ) override
        {
            CPhysicsShape3::setSimulationFilterData( data );
        }

        FilterData getSimulationFilterData() const override
        {
            return CPhysicsShape3::getSimulationFilterData();
        }

        void setActor( SmartPtr<IPhysicsBody3> body ) override
        {
            CPhysicsShape3::setActor( body );
        }

        SmartPtr<IPhysicsBody3> getActor() const override
        {
            return CPhysicsShape3::getActor();
        }

        void _getObject( void **object ) const override
        {
            CPhysicsShape3::_getObject( object );
        }

        bool hasShapeData() const override
        {
            return CPhysicsShape3::hasShapeData();
        }

        bool isAttached() const override
        {
            return CPhysicsShape3::isAttached();
        }

        void setEnabled( bool enabled ) override
        {
            CPhysicsShape3::setEnabled( enabled );
        }

        bool isEnabled() const override
        {
            return CPhysicsShape3::isEnabled();
        }

        void setTrigger( bool trigger ) override
        {
            CPhysicsShape3::setTrigger( trigger );
        }

        bool isTrigger() const override
        {
            return CPhysicsShape3::isTrigger();
        }

        void setStateContext( SmartPtr<IStateContext> context ) override
        {
            CPhysicsShape3::setStateContext( context );
        }

        SmartPtr<IStateContext> getStateContext() const override
        {
            return CPhysicsShape3::getStateContext();
        }

        void setStateListener( SmartPtr<IStateListener> listener ) override
        {
            CPhysicsShape3::setStateListener( listener );
        }

        SmartPtr<IStateListener> getStateListener() const override
        {
            return CPhysicsShape3::getStateListener();
        }

        void setCollisionType( u32 mask ) override
        {
            CPhysicsShape3::setCollisionType( mask );
        }

        u32 getCollisionType() const override
        {
            return CPhysicsShape3::getCollisionType();
        }

        void setCollisionMask( u32 mask ) override
        {
            CPhysicsShape3::setCollisionMask( mask );
        }

        u32 getCollisionMask() const override
        {
            return CPhysicsShape3::getCollisionMask();
        }

        bool handleStateChanged( const SmartPtr<IStateMessage> &message ) override
        {
            return CPhysicsShape3::handleStateChanged( message );
        }

        bool handleStateChanged( SmartPtr<IState> &state ) override
        {
            return CPhysicsShape3::handleStateChanged( state );
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
} // namespace workphone::physics

#endif
