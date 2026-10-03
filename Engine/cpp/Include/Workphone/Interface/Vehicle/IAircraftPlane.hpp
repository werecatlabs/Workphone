#ifndef IAircraftPlane_h__
#define IAircraftPlane_h__

#include <Workphone/Interface/Vehicle/IAircraft.hpp>

namespace workphone
{
    namespace vehicle
    {

        class IAircraftPlane : public IAircraft
        {
        public:
            ~IAircraftPlane() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle

}  // namespace workphone

#endif  // IAircraftPlane_h__
