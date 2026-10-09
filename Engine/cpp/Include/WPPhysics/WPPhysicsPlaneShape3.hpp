#ifndef WPPHYSICSPLANESHAPE3_HPP
#define WPPHYSICSPLANESHAPE3_HPP

#include <WPPhysics/WPPhysicsShape3T.hpp>
#include <Workphone/Physics/PlaneShape.hpp>

namespace workphone::physics
{
    class WPPhysicsPlaneShape3 : public WPPhysicsShape3T<PlaneShape>
    {
    public:
        WPPhysicsPlaneShape3();

        real_Num getDistance() const override;
        void setDistance(real_Num distance) override;

        Vector3<real_Num> getNormal() const override;
        void setNormal(const Vector3<real_Num> &normal) override;

        Plane3<real_Num> getPlane() const override;
        SmartPtr<IPhysicsShape3> clone() override;
    };
} // namespace workphone::physics

#endif
