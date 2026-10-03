#ifndef Frustum3_h__
#define Frustum3_h__

#include <Workphone/Math/Plane3.hpp>
#include <Workphone/Math/Matrix4.hpp>

namespace workphone
{

    /**
     * A class that represents a frustum, which is a truncated pyramid with the apex at the camera's
     * position and the base equal to the near plane of the camera's view volume.
     *
     * @tparam T The type of the coordinates.
     */
    template <class T>
    class WPCore_API Frustum3
    {
    public:
        /**
         * Default constructor that initializes an empty frustum.
         */
        Frustum3();

        /**
         * Default destructor.
         */
        ~Frustum3();

        /**
         * Set the view-projection matrix of the frustum.
         *
         * @param viewProjection The view-projection matrix.
         */
        void setViewProjection( const Matrix4<T> &viewProjection );

        /**
         * Check if a box intersects with the frustum.
         *
         * @param centre The centre of the box.
         * @param extents The extents of the box.
         * @return True if the box intersects with the frustum, false otherwise.
         */
        bool intersects( const Vector3<T> &centre, const Vector3<T> &extents ) const;

    protected:
        /**
         * The planes that define the frustum.
         */
        Plane3<T> m_planes[6];
    };

    using Frustum3F = Frustum3<f32>;
    using Frustum3D = Frustum3<f64>;

}  // namespace workphone

#endif  // Frustum3_h__
