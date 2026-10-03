#ifndef IGroundEffect_h__
#define IGroundEffect_h__

#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {

        class WPCore_API IGroundEffect : public IVehicleComponent
        {
        public:
            ~IGroundEffect() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IGroundEffect_h__
