#ifndef WPPHYSICSSPHERESHAPE3_HPP
#define WPPHYSICSSPHERESHAPE3_HPP

#include <WPPhysics/WPPhysicsShape3.hpp>
#include <Workphone/Physics/SphereShape.hpp>
#include <WPPhysics/WPPhysicsShape3T.hpp>

namespace workphone::physics
{
    class WPPhysicsSphereShape3 : public WPPhysicsShape3T<SphereShape>
    {
    public:
        WPPhysicsSphereShape3();

        SmartPtr<IPhysicsMaterial3> getMaterial() const override;

        void setMaterial( SmartPtr<IPhysicsMaterial3> material ) override;

        void setLocalPose( const Transform3<real_Num> &pose ) override;

        Transform3<real_Num> getLocalPose() const override;

        void setSimulationFilterData( const FilterData &data ) override;
        FilterData getSimulationFilterData() const override;

        void setActor( SmartPtr<IPhysicsBody3> body ) override;

        SmartPtr<IPhysicsBody3> getActor() const override;
        void _getObject( void **ppObject ) const override;

        bool hasShapeData() const override;

        SmartPtr<IPhysicsShape3> clone() override;
        bool isAttached() const override;

        void setEnabled( bool enabled ) override;
        bool isEnabled() const override;

        void setTrigger( bool trigger ) override;
        bool isTrigger() const override;

        void setStateContext( SmartPtr<IStateContext> stateContext ) override;
        SmartPtr<IStateContext> getStateContext() const override;

        void setStateListener( SmartPtr<IStateListener> stateListener ) override;
        SmartPtr<IStateListener> getStateListener() const override;

        void setCollisionType( u32 mask ) override;
        u32 getCollisionType() const override;

        void setCollisionMask( u32 mask ) override;
        u32 getCollisionMask() const override;

        bool handleStateChanged( const SmartPtr<IStateMessage> &message ) override;
        bool handleStateChanged( SmartPtr<IState> &state ) override;

        void setRadius( real_Num radius ) override;
        real_Num getRadius() const override;
    };
}  // namespace workphone::physics

#endif
