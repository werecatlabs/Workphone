#ifndef WPPHYSICSVEHICLE3_HPP
#define WPPHYSICSVEHICLE3_HPP

#include <WPPhysics/WPPhysicsConversions3.hpp>
#include <WPPhysics/WPPhysicsVehicleWheel.hpp>
#include <Workphone/Physics/PhysicsManager.hpp>

namespace workphone
{
    namespace physics
    {
        class WPPhysicsVehicle3 final : public IPhysicsVehicle3
        {
        public:
            explicit WPPhysicsVehicle3( SmartPtr<IRigidBody3> chassis );

            ~WPPhysicsVehicle3() override;

            void unload( SmartPtr<ISharedObject> ) override;

            IPhysicsVehicleWheel3 *addWheel() override;

            IPhysicsVehicleWheel3 *getWheel( u32 wheelIndex ) const override;

            u32 getNumWheels() const override;

            void finalize() override;

            void applyEngineForce( f32 engineForce, u32 wheelIndex ) override;

            void setBrake( f32 brakeForce, u32 wheelIndex ) override;

            void setSteeringValue( f32 steeringValue, u32 wheelIndex ) override;

            void setPosition( const Vector3<real_Num> &position ) override;

            Vector3<real_Num> getPosition() const override;

            void setOrientation( const Quaternion<real_Num> &orientation ) override;

            Quaternion<real_Num> getOrientation() const override;

            void setVelocity( const Vector3<real_Num> &velocity ) override;

            Vector3<real_Num> getVelocity() const override;

            void setMaterialId( u32 materialId ) override;

            u32 getMaterialId() const override;

            AABB3F getLocalAABB() const override;

            AABB3F getWorldAABB() const override;

            void setEnabled( bool enabled ) override;

            bool isEnabled() const override;

            SmartPtr<IPhysicsVehicleInput3> &getVehicleInput() override;

            const SmartPtr<IPhysicsVehicleInput3> &getVehicleInput() const override;

            Array<Transform3F> getWheelTransformations() const override;

            SmartPtr<IRigidBody3> getChassis() const;

        private:
            WPPhysicsVehicleWheel *getNativeWheel( u32 wheelIndex ) const;

            SmartPtr<IRigidBody3> m_chassis;
            Array<SmartPtr<WPPhysicsVehicleWheel>> m_wheels;
            SmartPtr<IPhysicsVehicleInput3> m_vehicleInput;
            u32 m_materialId = 0;
            bool m_enabled = true;
            bool m_finalized = false;
        };

    }  // namespace physics
}  // namespace workphone

#endif  // WPPHYSICSVEHICLE3_HPP
