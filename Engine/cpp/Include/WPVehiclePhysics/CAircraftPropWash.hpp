#ifndef PropWash_h__
#define PropWash_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropWash.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>
#include <WPVehiclePhysics/InputController.hpp>
#include <Workphone/Math/LinearSpline1.hpp>

namespace workphone::vehicle
{
    class WPVehiclePhysics_API CAircraftPropWash : public CAircraftAttachment<IAircraftPropWash>
    {
    public:
        CAircraftPropWash();
        ~CAircraftPropWash() override;

        void load(SmartPtr<ISharedObject> data) override;
        void loadFromData(void *pData);
        void unload(SmartPtr<ISharedObject> data) override;

        Vector3<real_Num> getPropWash() override;
        Vector3<real_Num> getPropWash(s32 section) override;

        SmartPtr<IAircraftPropellerUnit> getPropellerUnit() const override;
        void setPropellerUnit(SmartPtr<IAircraftPropellerUnit> propellerUnit) override;

        real_Num getStrength() const override;
        void setStrength(real_Num strength) override;

        Array<bool> &getAffectedSections() override;
        const Array<bool> &getAffectedSections() const override;
        void setAffectedSections(const Array<bool> &affectedSections) override;

        Array<float> &getSectionMultipliers() override;
        const Array<float> &getSectionMultipliers() const override;
        void setSectionMultipliers(const Array<float> &sectionMultipliers) override;

    private:
        SmartPtr<IAircraftPropellerUnit> m_propellerUnit;

        LinearSpline1<f32> m_curve;
        real_Num m_strength = 0.0;
        real_Num m_maxSpeed = 1e10;

        Array<bool> m_affectedSections;
        Array<real_Num> m_sectionMultipliers;
    };
}

#endif // PropWash_h__
