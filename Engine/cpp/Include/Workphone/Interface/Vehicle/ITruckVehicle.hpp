#ifndef ITruckVehicle_h__
#define ITruckVehicle_h__

#include <Workphone/Interface/Vehicle/IVehicle.hpp>

namespace workphone
{
    namespace vehicle
    {

        class ITruckVehicle : public IVehicle
        {
        public:
            /** Virtual destructor. */
            ~ITruckVehicle() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // ITruckVehicle_h__
