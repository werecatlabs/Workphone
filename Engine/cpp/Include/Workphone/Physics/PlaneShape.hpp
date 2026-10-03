#ifndef PlaneShape_h__
#define PlaneShape_h__

#include <Workphone/Interface/Physics/IPlaneShape3.hpp>
#include <Workphone/Physics/PhysicsShape3.hpp>

namespace workphone
{
    namespace physics
    {

        /* * Implementation of a plane shape. */
        class WPCore_API PlaneShape : public PhysicsShape3<IPlaneShape3>
        {
        public:
            /** Constructor. */
            PlaneShape();

            /** Destructor. */
            ~PlaneShape() override;

            /** @copydoc IPlaneShape3::getDistance */
            real_Num getDistance() const override;

            /** @copydoc IPlaneShape3::setDistance */
            void setDistance( real_Num distance ) override;

            /** @copydoc IPlaneShape3::getNormal */
            Vector3<real_Num> getNormal() const override;

            /** @copydoc IPlaneShape3::setNormal */
            void setNormal( const Vector3<real_Num> &normal ) override;

            /** @copydoc IPlaneShape3::getPlane */
            Plane3<real_Num> getPlane() const override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace physics
}  // namespace workphone

#endif  // PlaneShape_h__
