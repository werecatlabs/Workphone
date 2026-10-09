#ifndef CDifferential_h__
#define CDifferential_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IDifferential.hpp>
#include <WPVehiclePhysics/CVehicleComponent.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{
    /**
     * @brief Concrete implementation of the IDifferential interface for a vehicle.
     */
    class WPVehiclePhysics_API CDifferential : public CVehicleComponent<IDifferential>
    {
    public:
        /**
         * @brief Constructs a new CDifferential object.
         */
        CDifferential();

        /**
         * @brief Destroys the CDifferential object.
         */
        ~CDifferential() override;

        /**
         * @brief Sets the ratio of the differential.
         * @param ratio The ratio to set.
         */
        void setRatio(f32 ratio) override;

        /**
         * @brief Gets the ratio of the differential.
         * @return The ratio of the differential.
         */
        f32 getRatio() const override;

        /**
         * @brief Sets the wheel controller for the specified wheel index.
         * @param idx The index of the wheel.
         * @param wheel The wheel controller to set.
         */
        void setWheel(u32 idx, SmartPtr<IWheelComponent> wheel) override;

        /**
         * @brief Gets the wheel controller for the specified wheel index.
         * @param idx The index of the wheel.
         * @return The wheel controller for the specified wheel index.
         */
        SmartPtr<IWheelComponent> getWheel(u32 idx) const override;

        /**
         * @brief Sets the torque to be applied to the wheels of the differential.
         * @param wheelForce The torque to be applied to the wheels.
         */
        void setWheelTorque(f32 wheelForce) override;

        /**
         * @brief Gets the lock coefficient of the differential.
         * @return The lock coefficient of the differential.
         */
        f32 getLockCoefficient() const override;

        /**
         * @brief Sets the lock coefficient of the differential.
         * @param lockCoefficient The lock coefficient to set.
         */
        void setLockCoefficient(f32 lockCoefficient) override;

        SmartPtr<Properties> getProperties() const override;
        void setProperties(SmartPtr<Properties> properties) override;

        f32 getWheelTorque() const;

    protected:
        f32 m_ratio = 1.0f;
        f32 m_lockCoefficient = 0.0f;
        f32 m_wheelTorque = 0.0f;
        FixedArray<SmartPtr<IWheelComponent>, 2> m_wheels;
    };
} // namespace workphone

#endif // CDifferential_h__
