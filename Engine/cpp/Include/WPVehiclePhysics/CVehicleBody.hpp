#ifndef CVehicleBody_h__
#define CVehicleBody_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <WPVehiclePhysics/CVehicleComponent.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>

namespace workphone
{
    class WPVehiclePhysics_API CVehicleBody : public CVehicleComponent<IVehicleBody>
    {
    public:
        CVehicleBody();
        ~CVehicleBody() override;

        void unload( SmartPtr<ISharedObject> data ) override;

        bool isValid() const override;

        Vector3<physics_Num> getVelocity() const override;
        void                 setVelocity( const Vector3<physics_Num> &velocity ) override;

        Vector3<physics_Num> getAngularVelocity() const override;
        void                 setAngularVelocity( const Vector3<physics_Num> &angularVelocity ) override;

        Vector3<physics_Num> getWorldCenterOfMass() const override;
        void setWorldCenterOfMass( const Vector3<physics_Num> &worldCenterOfMass ) override;

        void addLocalForceAtPosition( const Vector3<physics_Num> &force, const Vector3<physics_Num> &pos,
                                      physics::ForceModeEnum forceMode );
        void addForceAtPosition( const Vector3<physics_Num> &force, const Vector3<physics_Num> &pos,
                                 physics::ForceModeEnum forceMode );
        void addTorque( const Vector3<physics_Num> &torque ) override;
        void addLocalTorque( const Vector3<physics_Num> &torque ) override;

        SmartPtr<IVehicle>       &getParentVehicle();
        const SmartPtr<IVehicle> &getParentVehicle() const;
        void                      setParentVehicle( SmartPtr<IVehicle> parentVehicle );

        void update() override;

        Vector3<physics_Num> getPointVelocity( const Vector3<physics_Num> &p ) override;

        void addLocalForceAtLocalPosition( const Vector3<physics_Num> &force,
                                           const Vector3<physics_Num> &pos ) override;
        void addForceAtPosition( const Vector3<physics_Num> &force,
                                 const Vector3<physics_Num> &pos ) override;

        bool castLocalRay( const Ray3<physics_Num> &ray, SmartPtr<physics::IRaycastHit> &data ) override;
        bool castWorldRay( const Ray3<physics_Num> &ray, SmartPtr<physics::IRaycastHit> &data ) override;

        physics_Num getMass() const override;
        void        setMass( physics_Num mass ) override;

        Vector3<physics_Num> getLocalVelocity() const override;
        void                 setLocalVelocity( const Vector3<physics_Num> &localVelocity ) override;

        Vector3<physics_Num> getLocalAngularVelocity() const override;
        void setLocalAngularVelocity( const Vector3<physics_Num> &localAngularVelocity ) override;

    protected:
        SmartPtr<IVehicle> m_parentVehicle;

        Vector3<physics_Num> m_velocity;
        Vector3<physics_Num> m_angularVelocity;

        Vector3<physics_Num> m_localVelocity;
        Vector3<physics_Num> m_localAngularVelocity;

        Vector3<physics_Num> m_worldCenterOfMass;
        physics_Num          m_mass = static_cast<physics_Num>( 1500.0 );
    };
} // namespace workphone

#endif // Rigidbody_h__
