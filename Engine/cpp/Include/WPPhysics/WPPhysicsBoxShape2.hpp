#ifndef WPPHYSICSNATIVEBOXSHAPE2_HPP
#define WPPHYSICSNATIVEBOXSHAPE2_HPP

#include <WPPhysics/WPPhysicsShape2T.hpp>
#include <Workphone/Interface/Physics/IBoxShape2.hpp>

namespace workphone::physics
{
    class WPPhysicsBoxShape2 : public WPPhysicsShape2T<BoxShape2>
    {
    public:
        WPPhysicsBoxShape2();
        ~WPPhysicsBoxShape2() override;

        Sphere2<real_Num> getSphere() const override;
        AABB2<real_Num> getAABB() const override;
        void setAABB( const AABB2<real_Num> &box ) override;
        void getPoints( Array<Vector2<real_Num>> &points ) const override;
        void computeMass( SmartPtr<IMassData2> massData, real_Num density ) const override;

        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        bool handleStateChanged( const SmartPtr<IStateMessage> &message ) override;
        bool handleStateChanged( SmartPtr<IState> &state ) override;

    private:
        AABB2<real_Num> m_aabb;
    };
}  // namespace workphone::physics

#endif
