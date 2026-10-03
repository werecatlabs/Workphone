#ifndef IDrone_h__
#define IDrone_h__

#include <Workphone/Interface/Vehicle/IVehicle.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief An interface for a drone.
         */
        class WPCore_API IDrone : public IVehicle
        {
        public:
            ~IDrone() override;

            virtual f32 getGroundEffectMultiplier() const = 0;
            virtual void setGroundEffectMultiplier( f32 multiplier ) = 0;

            virtual Vector3<real_Num> getDrag() const = 0;
            virtual void setDrag( const Vector3<real_Num> &drag ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IDrone_h__
