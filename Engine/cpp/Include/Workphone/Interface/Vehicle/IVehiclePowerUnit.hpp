#ifndef IEngine_h__
#define IEngine_h__

#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * Interface for a vehicle power unit.
         */
        class WPCore_API IVehiclePowerUnit : public IVehicleComponent
        {
        public:
            /** Virtual destructor. */
            ~IVehiclePowerUnit() override;

            /** Sets the current rpm of the engine. */
            virtual void setRPM( real_Num rpm ) = 0;

            /** Returns the current rpm of the engine. */
            virtual real_Num getRPM() const = 0;

            /** Returns the maximum rpm of the engine. */
            virtual real_Num getMaxRPM() const = 0;

            /** Returns the current torque of the engine. */
            virtual real_Num getTorque() const = 0;

            /** Returns the torque produced by the engine given the throttle position.
             * @param throttlePosition The position of the throttle, from 0.0 to 1.0.
             * @return The torque produced by the engine.
             */
            virtual f32 getTorque( f32 throttlePosition ) const = 0;

            /** Returns the maximum torque from the engine if the throttle position is 1.0.
             * @param rpm The current RPM of the engine.
             * @return The maximum torque produced by the engine.
             */
            virtual f32 getMaxTorque( u32 rpm ) const = 0;

            /** Returns the minimum torque from the engine if the throttle position is 0.0.
             * @param rpm The current RPM of the engine.
             * @return The minimum torque produced by the engine.
             */
            virtual f32 getMinTorque( u32 rpm ) const = 0;

            /** Returns the current throttle position.
             * @return The current throttle position, from 0.0 to 1.0.
             */
            virtual real_Num getThrottle() const = 0;

            /** Sets the throttle position.
             * @param throttle The position of the throttle, from 0.0 to 1.0.
             */
            virtual void setThrottle( real_Num throttle ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IEngine_h__
