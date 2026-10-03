#ifndef _IFrustum_H
#define _IFrustum_H

#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for a view frustum used in culling and visibility determination.
         *
         * The frustum defines the visible region of space for a camera or viewport, typically used
         * for culling objects that are outside the camera's field of view. This interface provides
         * methods to configure the frustum's near and far clipping planes, aspect ratio, and to test
         * whether objects or points are visible within the frustum.
         */
        class WPCore_API IFrustum : public IGraphicsObject
        {
        public:
            IFrustum();

            IFrustum( u32 poolTypeId );

            /**
             * @brief Virtual destructor for safe polymorphic destruction.
             */
            ~IFrustum() override;

            /**
             * @brief Sets the distance to the near clipping plane.
             * @param nearDist The distance from the frustum origin to the near clipping plane.
             */
            virtual void setNearClipDistance( f32 nearDist ) = 0;

            /**
             * @brief Gets the distance to the near clipping plane.
             * @return The distance from the frustum origin to the near clipping plane.
             */
            virtual f32 getNearClipDistance() const = 0;

            /**
             * @brief Sets the distance to the far clipping plane.
             * @param farDist The distance from the frustum origin to the far clipping plane.
             */
            virtual void setFarClipDistance( f32 farDist ) = 0;

            /**
             * @brief Gets the distance to the far clipping plane.
             * @return The distance from the frustum origin to the far clipping plane.
             */
            virtual f32 getFarClipDistance() const = 0;

            /**
             * @brief Sets the aspect ratio of the frustum viewport.
             * @param ratio The aspect ratio (width divided by height).
             */
            virtual void setAspectRatio( f32 ratio ) = 0;

            /**
             * @brief Gets the current aspect ratio of the frustum viewport.
             * @return The aspect ratio (width divided by height).
             */
            virtual f32 getAspectRatio() const = 0;

            /**
             * @brief Checks if an axis-aligned bounding box (AABB) is visible within the frustum.
             * @param bound The AABB to test for visibility.
             * @return True if the bounding box is at least partially visible, false otherwise.
             */
            virtual bool isObjectVisible( const AABB3<real_Num> &bound ) const = 0;

            /**
             * @brief Checks if a sphere is visible within the frustum.
             * @param bound The sphere to test for visibility.
             * @return True if the sphere is at least partially visible, false otherwise.
             */
            virtual bool isObjectVisible( const Sphere3<real_Num> &bound ) const = 0;

            /**
             * @brief Checks if a point is visible within the frustum.
             * @param vert The point to test for visibility.
             * @return True if the point is inside the frustum, false otherwise.
             */
            virtual bool isObjectVisible( const Vector3<real_Num> &vert ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
