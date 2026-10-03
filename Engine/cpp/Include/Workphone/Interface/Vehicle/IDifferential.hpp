#ifndef IDifferential_h__
#define IDifferential_h__

#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief Interface for a differential component of a vehicle.
         */
        class WPCore_API IDifferential : public IVehicleComponent
        {
        public:
            /** */
            ~IDifferential() override;

            /**
             * @brief Sets the ratio of the differential.
             * @param ratio The ratio to set.
             */
            virtual void setRatio( f32 ratio ) = 0;

            /**
             * @brief Gets the ratio of the differential.
             * @return The ratio of the differential.
             */
            virtual f32 getRatio() const = 0;

            /**
             * @brief Sets the wheel controller for the specified wheel index.
             * @param idx The index of the wheel.
             * @param wheel The wheel controller to set.
             */
            virtual void setWheel( u32 idx, SmartPtr<IWheelComponent> wheel ) = 0;

            /**
             * @brief Gets the wheel controller for the specified wheel index.
             * @param idx The index of the wheel.
             * @return The wheel controller for the specified wheel index.
             */
            virtual SmartPtr<IWheelComponent> getWheel( u32 idx ) const = 0;

            /**
             * @brief Sets the torque to be applied to the wheels of the differential.
             * @param wheelForce The torque to be applied to the wheels.
             */
            virtual void setWheelTorque( f32 wheelForce ) = 0;

            /**
             * @brief Gets the lock coefficient of the differential.
             * @return The lock coefficient of the differential.
             */
            virtual f32 getLockCoefficient() const = 0;

            /**
             * @brief Sets the lock coefficient of the differential.
             * @param lockCoefficient The lock coefficient to set.
             */
            virtual void setLockCoefficient( f32 lockCoefficient ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IDifferential_h__
