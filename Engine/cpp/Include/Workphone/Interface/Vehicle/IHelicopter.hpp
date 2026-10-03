#ifndef IHelicopter_h__
#define IHelicopter_h__

#include <Workphone/Interface/Vehicle/IAircraft.hpp>

namespace workphone
{
    namespace vehicle
    {

        class WPCore_API IHelicopter : public IAircraft
        {
        public:
            ~IHelicopter() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IHelicopter_h__
