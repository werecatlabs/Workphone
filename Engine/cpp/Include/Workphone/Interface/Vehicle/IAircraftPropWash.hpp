#ifndef IAircraftPropWash_h__
#define IAircraftPropWash_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace vehicle
    {
        class WPCore_API IAircraftPropWash : public IVehicleComponent
        {
        public:
            ~IAircraftPropWash() override;

            virtual Vector3<real_Num> getPropWash() = 0;
            virtual Vector3<real_Num> getPropWash( s32 section ) = 0;

            virtual SmartPtr<IAircraftPropellerUnit> getPropellerUnit() const = 0;
            virtual void setPropellerUnit( SmartPtr<IAircraftPropellerUnit> propellerUnit ) = 0;

            virtual Array<bool> &getAffectedSections() = 0;
            virtual const Array<bool> &getAffectedSections() const = 0;
            virtual void setAffectedSections( const Array<bool> &affectedSections ) = 0;

            virtual Array<float> &getSectionMultipliers() = 0;
            virtual const Array<float> &getSectionMultipliers() const = 0;
            virtual void setSectionMultipliers( const Array<float> &sectionMultipliers ) = 0;

            virtual real_Num getStrength() const = 0;
            virtual void setStrength( real_Num strength ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IAircraftPropWash_h__
