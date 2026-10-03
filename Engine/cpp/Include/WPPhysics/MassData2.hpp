#ifndef MassData2_h__
#define MassData2_h__

#include "Workphone/Interface/Physics/IMassData2.hpp"

namespace workphone::physics
{

    class MassData2 : public IMassData2
    {
    public:
        MassData2();

        ~MassData2();

        real_Num getMass() const;

        void setMass( real_Num mass );

        Vector2<real_Num> getCenter() const;

        void setCenter( Vector2<real_Num> center );

        real_Num getInertia() const;

        void setInertia( real_Num inertia );

    private:
        real_Num          m_mass;
        Vector2<real_Num> m_center;
        real_Num          m_inertia;
    };
} // namespace workphone::physics
#endif // IMassData2_h__
