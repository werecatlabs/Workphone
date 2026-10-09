#ifndef WPPHYSICSMASSDATA2_HPP
#define WPPHYSICSMASSDATA2_HPP

#include "Workphone/Interface/Physics/IMassData2.hpp"

namespace workphone::physics
{
    class WPPhysicsMassData2 : public IMassData2
    {
    public:
        WPPhysicsMassData2();

        ~WPPhysicsMassData2() override;

        real_Num getMass() const override;

        void setMass(real_Num mass) override;

        Vector2<real_Num> getCenter() const override;

        void setCenter(Vector2<real_Num> center) override;

        real_Num getInertia() const override;

        void setInertia(real_Num inertia) override;

    private:
        real_Num m_mass;
        Vector2<real_Num> m_center;
        real_Num m_inertia;
    };
} // namespace workphone::physics
#endif  // IMassData2_h__
