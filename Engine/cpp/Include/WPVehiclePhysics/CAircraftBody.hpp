#ifndef Rigidbody_h__
#define Rigidbody_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Interface/Vehicle/IAircraftBody.hpp>
#include "WPVehiclePhysics/CAircraftAttachment.hpp"
#include <Workphone/Core/Properties.hpp>

namespace workphone::vehicle
{
    class WPVehiclePhysics_API CAircraftBody : public CAircraftAttachment<IAircraftBody>
    {
    public:
        CAircraftBody();
        ~CAircraftBody() override;

        bool isValid() const override;

        Vector3<real_Num> getVelocity() const override;
        void setVelocity(const Vector3<real_Num> &velocity) override;

        Vector3<real_Num> getAngularVelocity() const override;
        void setAngularVelocity(const Vector3<real_Num> &angularVelocity) override;

        Vector3<real_Num> getWorldCenterOfMass() const override;
        void setWorldCenterOfMass(const Vector3<real_Num> &worldCenterOfMass) override;

        void addLocalForceAtPosition(const Vector3<real_Num> &force, const Vector3<real_Num> &pos,
                                     physics::ForceModeEnum forceMode);
        void addForceAtPosition(const Vector3<real_Num> &force, const Vector3<real_Num> &pos,
                                physics::ForceModeEnum forceMode);
        void addTorque(const Vector3<real_Num> &torque) override;
        void addLocalTorque(const Vector3<real_Num> &torque) override;

        SmartPtr<IAircraft> &getParentAircraft();
        const SmartPtr<IAircraft> &getParentAircraft() const;
        void setParentAircraft(SmartPtr<IAircraft> parentAircraft);

        void update(const double &t, const double &dt) override;

        Vector3<real_Num> getPointVelocity(const Vector3<real_Num> &p) override;

        void addLocalForceAtLocalPosition(const Vector3<real_Num> &force,
                                          const Vector3<real_Num> &pos) override;
        void addForceAtPosition(const Vector3<real_Num> &force,
                                const Vector3<real_Num> &pos) override;

        bool castLocalRay(const Ray3<real_Num> &ray,
                          SmartPtr<physics::IRaycastHit> &data) override;
        bool castWorldRay(const Ray3<real_Num> &ray,
                          SmartPtr<physics::IRaycastHit> &data) override;

        real_Num getMass() const override;
        void setMass(real_Num mass) override;

        Vector3<real_Num> getLocalVelocity() const override;
        void setLocalVelocity(const Vector3<real_Num> &localVelocity) override;

        Vector3<real_Num> getLocalAngularVelocity() const override;
        void setLocalAngularVelocity(const Vector3<real_Num> &localAngularVelocity) override;

    protected:
        SmartPtr<IAircraft> m_parentAircraft;

        Vector3<real_Num> m_velocity;
        Vector3<real_Num> m_angularVelocity;

        Vector3<real_Num> m_localVelocity;
        Vector3<real_Num> m_localAngularVelocity;

        Vector3<real_Num> m_worldCenterOfMass;
        real_Num m_mass;
    };
}

#endif // Rigidbody_h__
