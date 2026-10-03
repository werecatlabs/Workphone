#ifndef _Sphere2d_H
#define _Sphere2d_H

#include <Workphone/Interface/Physics/ISphereShape2.hpp>
#include <Workphone/Math/Transform2.hpp>
#include "WPPhysicsPrerequisites.hpp"

extern "C"
{
#include <WorkphonePhysics/workphone_physics_2d.h>
}

namespace workphone::physics
{

    class SphereShape2 : public ISphereShape2
    {
    public:
        SphereShape2();

        ~SphereShape2() override;

        Sphere2<real_Num> getSphere() const override;

        AABB2<real_Num> getAABB() const override;

        void setRadius( real_Num radius ) override;

        real_Num getRadius() const override;

        u8 getType() const override;

        void getPoints( Array<Vector2<real_Num>> &points ) const override;

        void getPoints( Array<Vector2<real_Num>> &points, const Transform2<real_Num> &tranform ) const;

        void computeMass( SmartPtr<IMassData2> massData, real_Num density ) const override;

        void _getObject( void **ppObject ) const override;

        SmartPtr<Properties> getProperties() const override;

        void setProperties( SmartPtr<Properties> properties ) override;

        /** @copydoc IPhysicsShape::setEnabled */
        virtual void setEnabled( bool enabled ) override;

        /** @copydoc IPhysicsShape::setEnabled */
        virtual bool isEnabled() const override;

    protected:
        wp_collision_shape *m_shape = nullptr;
    };

} // namespace workphone::physics

// end namespace

#endif
