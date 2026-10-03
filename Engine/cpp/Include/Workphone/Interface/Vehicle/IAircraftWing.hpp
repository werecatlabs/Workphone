#ifndef IAircraftWing_h__
#define IAircraftWing_h__

#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {
        /** Interface for an aircraft wing class. */
        class WPCore_API IAircraftWing : public IVehicleComponent
        {
        public:
            ~IAircraftWing() override;

            virtual SmartPtr<IAircraftControlSurface> getAttachedControlSurface() const = 0;
            virtual void setAttachedControlSurface(
                SmartPtr<IAircraftControlSurface> controlSurface ) = 0;

            virtual SmartPtr<IAircraftPropWash> getAttachedPropWash() const = 0;
            virtual void setAttachedPropWash( SmartPtr<IAircraftPropWash> propWash ) = 0;

            virtual bool isControlSurface() const = 0;
            virtual void setControlSurface( bool controlSurface ) = 0;

            virtual bool useCombinedControlSurface() const = 0;
            virtual void setUserCombinedControlSurface( bool useCombinedControlSurface ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IAircraftWing_h__
