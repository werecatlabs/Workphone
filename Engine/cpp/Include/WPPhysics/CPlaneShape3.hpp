#ifndef WP_CPLANESHAPE3_HPP
#define WP_CPLANESHAPE3_HPP

#include <WPPhysics/CPhysicsShape3Adapter.hpp>
#include <Workphone/Interface/Physics/IPlaneShape3.hpp>

namespace workphone::physics
{
    class CPlaneShape3 : public CPhysicsShape3Adapter<IPlaneShape3>
    {
    public:
        CPlaneShape3();

        real_Num getDistance() const override;
        void     setDistance( real_Num distance ) override;

        Vector3<real_Num> getNormal() const override;
        void              setNormal( const Vector3<real_Num> &normal ) override;

        Plane3<real_Num>         getPlane() const override;
        SmartPtr<IPhysicsShape3> clone() override;
    };
} // namespace workphone::physics

#endif
