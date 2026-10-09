#ifndef GearBoxStandard_h__
#define GearBoxStandard_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IGearBox.hpp>
#include "WPVehiclePhysics/CVehicleComponent.hpp"

namespace workphone
{
    class WPVehiclePhysics_API CGearBox : public CVehicleComponent<IGearBox>
    {
    public:
        CGearBox();
        ~CGearBox() override;

        void setRatios(const Array<f32> &ratios) override;
        Array<f32> getRatios() const override;

        f32 getRatio(u32 gear) const override;

        u32 getNumGears() const override;

        void decreamentSelectedGear() override;
        void increamentSelectedGear() override;

        u32 getCurrentGear() const override;

        void setCurrentGear(u32 currentGear);

        SmartPtr<Properties> getProperties() const override;
        void setProperties(SmartPtr<Properties> properties) override;

    protected:
        Array<f32> m_ratios;
        u32 m_currentGear = 0;
    };
} // namespace workphone

#endif // GearBoxStandard_h__
