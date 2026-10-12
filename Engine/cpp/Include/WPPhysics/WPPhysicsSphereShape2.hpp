#ifndef WPPHYSICSNATIVESPHERESHAPE2_HPP
#define WPPHYSICSNATIVESPHERESHAPE2_HPP

#include <WPPhysics/WPPhysicsShape2T.hpp>
#include <Workphone/Interface/Physics/ISphereShape2.hpp>

namespace workphone::physics
{
    class WPPhysicsSphereShape2 : public WPPhysicsShape2T<SphereShape2>
    {
    public:
        WPPhysicsSphereShape2();
        ~WPPhysicsSphereShape2() override;

        void setRadius( real_Num radius ) override;
        real_Num getRadius() const override;
        Sphere2<real_Num> getSphere() const override;
        AABB2<real_Num> getAABB() const override;
        void getPoints( Array<Vector2<real_Num>> &points ) const override;
        void computeMass( SmartPtr<IMassData2> massData, real_Num density ) const override;

        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        bool handleStateChanged( const SmartPtr<IStateMessage> &message ) override;
        bool handleStateChanged( SmartPtr<IState> &state ) override;
    };
}  // namespace workphone::physics

#endif
