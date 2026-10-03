#ifndef BoxShape2_h__
#define BoxShape2_h__

#include <Workphone/Interface/Physics/IBoxShape2.hpp>
#include <Workphone/Physics/PhysicsShape2.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @class BoxShape2
         * @brief 2D axis-aligned box shape used by the physics system.
         *
         * This class represents a rectangular (box) collision shape in 2D.
         * It stores extents and provides functionality to compute its axis-aligned
         * bounding box (AABB), bounding sphere, sample points, and mass properties.
         *
         * It implements the IBoxShape2 interface and extends the generic PhysicsShape2
         * providing physics-shape-specific behaviour.
         *
         * @note All vectors, extents and sizes use the project's numeric type `real_Num`.
         */
        class WPCore_API BoxShape2 : public PhysicsShape2<IBoxShape2>
        {
        public:
            /**
             * @brief Construct an empty BoxShape2 with zero extents.
             *
             * The constructed box will typically be invalid (zero area) until extents
             * are set via `setExtents` or loaded from serialized data.
             */
            BoxShape2();

            /**
             * @brief Virtual destructor.
             *
             * Cleans up any resources held by the shape. Override is declared to
             * ensure proper polymorphic destruction through base pointers.
             */
            ~BoxShape2() override;

            /**
             * @brief Load shape data from a serialized/shared object.
             *
             * Expected `data` to contain properties required to initialise the box,
             * typically extents or AABB information. Implementations should validate
             * the contents of `data`.
             *
             * @param data Smart pointer to shared object containing serialized data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload / release resources associated with the shape.
             *
             * This should reverse any effects of `load` and prepare the object for
             * destruction or reuse.
             *
             * @param data Optional smart pointer to a shared object used when unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Check whether the shape contains valid geometry.
             *
             * A BoxShape2 is considered valid when its extents are strictly positive.
             *
             * @return true if the shape is valid and can be used for physics queries,
             *         false otherwise.
             */
            bool isValid() const override;

            /**
             * @brief Get the half-extents of the box along each axis.
             *
             * Convention: extents are half-sizes from the box centre to the edges.
             *
             * @return Vector2<real_Num> containing (halfWidth, halfHeight).
             */
            Vector2<real_Num> getExtents() const;

            /**
             * @brief Set the box extents (half-sizes).
             *
             * Passing non-positive components will make the box invalid.
             *
             * @param extents Half-width and half-height of the box.
             */
            void setExtents( const Vector2<real_Num> &extents );

            /**
             * @brief Retrieve the axis-aligned bounding box (AABB) for this shape.
             *
             * The returned AABB is expressed in the same coordinate space as the shape.
             *
             * @return AABB2<real_Num> Axis-aligned bounding box enclosing the box shape.
             */
            AABB2<real_Num> getAABB() const override;

            /**
             * @brief Set the axis-aligned bounding box for this shape.
             *
             * This may be used to initialise or override the internal extents from an AABB.
             *
             * @param box The AABB that defines the box extents and position.
             */
            void setAABB( const AABB2<real_Num> &box ) override;

            /**
             * @brief Compute and return the bounding sphere of the box.
             *
             * The bounding sphere is the minimal sphere (centre and radius) that encloses
             * the box. Typically the sphere is centred at the box centre and radius is
             * computed from extents.
             *
             * @return Sphere2<real_Num> Bounding sphere enclosing the box.
             */
            Sphere2<real_Num> getSphere() const override;

            /**
             * @brief Append the box corner points to the provided container.
             *
             * The points are returned in no particular winding order. The container
             * will be appended to; it is not cleared by this method.
             *
             * @param points Output container that will receive the corner points.
             */
            void getPoints( Array<Vector2<real_Num>> &points ) const override;

            /**
             * @brief Compute the mass data (mass, centre of mass, inertia) for the box.
             *
             * The computed mass/inertia assumes uniform density. The inertia returned
             * corresponds to a rectangle of the given extents about its centre of mass.
             *
             * @param massData Output structure to receive mass-related properties.
             * @param density Material density used to compute mass (mass = area * density).
             */
            void computeMass( SmartPtr<IMassData2> massData, real_Num density ) const override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace physics
}  // namespace workphone

#endif  // BoxShape2_h__
