#ifndef IFourWheelVehicle_h__
#define IFourWheelVehicle_h__

#include <Workphone/Interface/Vehicle/IVehicle.hpp>

namespace workphone
{
    namespace vehicle
    {

        class IFourWheelVehicle : public IVehicle
        {
        public:
            /** Virtual destructor. */
            ~IFourWheelVehicle() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IFourWheelVehicle_h__
