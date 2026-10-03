#ifndef IElectricMotor_h__
#define IElectricMotor_h__

#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {
        class WPCore_API IElectricMotor : public IVehicleComponent
        {
        public:
            ~IElectricMotor() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IElectricMotor_h__
