#ifndef IAircraftBody_h__
#define IAircraftBody_h__

#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>

namespace workphone
{
    namespace vehicle
    {

        class WPCore_API IAircraftBody : public IVehicleBody
        {
        public:
            ~IAircraftBody() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IAircraftBody_h__
