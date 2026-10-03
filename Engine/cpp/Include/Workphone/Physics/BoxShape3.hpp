#ifndef BoxShape3_h__
#define BoxShape3_h__

#include <Workphone/Interface/Physics/IBoxShape3.hpp>
#include <Workphone/Physics/PhysicsShape3.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @class BoxShape3
         * @brief 3D axis-aligned box (rectangular prism) collision shape used by the physics system.
         *
         * This class represents a box collision shape in 3D. It stores the half-extents
         * (half-width, half-height, half-depth) relative to the box centre and provides
         * accessors for extents and the axis-aligned bounding box (AABB).
         *
         * The class implements the `IBoxShape3` interface and extends the generic
         * `PhysicsShape3` to provide physics-shape-specific behaviour.
         *
         * @note All vectors, extents and sizes use the project's numeric type `real_Num`.
         * @note Extents follow the convention of half-sizes from the box centre to each face.
         */
        class WPCore_API BoxShape3 : public PhysicsShape3<IBoxShape3>
        {
        public:
            /**
             * @brief Construct an empty BoxShape3 with zero extents.
             *
             * The constructed box will typically be invalid (zero volume) until extents
             * are set via `setExtents` or initialised through `load`.
             */
            BoxShape3();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper polymorphic destruction when using base pointers.
             */
            ~BoxShape3() override;

            /**
             * @brief Load shape data from a serialized/shared object.
             *
             * Expected `data` to contain properties required to initialise the box,
             * typically the extents or an AABB. Implementations should validate the
             * contents of `data` and update the internal state accordingly.
             *
             * @param data Smart pointer to shared object containing serialized data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload / release resources associated with the shape.
             *
             * This should reverse any effects of `load` and prepare the object for
             * destruction or reuse. The `data` parameter may be used to supply context
             * required when unloading.
             *
             * @param data Optional smart pointer to a shared object used when unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Get the box half-extents along each axis.
             *
             * Convention: extents are half-sizes from the box centre to the faces.
             *
             * @return Vector3<real_Num> containing (halfWidth, halfHeight, halfDepth).
             */
            Vector3<real_Num> getExtents() const override;

            /**
             * @brief Set the box half-extents.
             *
             * Passing non-positive components will make the box invalid for physics
             * queries. Implementations may clamp or reject invalid values.
             *
             * @param extents Half-width, half-height and half-depth of the box.
             */
            void setExtents( const Vector3<real_Num> &extents ) override;

            /**
             * @brief Retrieve the axis-aligned bounding box (AABB) for this shape.
             *
             * The returned AABB is expressed in the same coordinate space as the shape
             * and represents a box that fully encloses this shape.
             *
             * @return AABB3<real_Num> Axis-aligned bounding box enclosing the box shape.
             */
            AABB3<real_Num> getAABB() const override;

            /**
             * @brief Set the axis-aligned bounding box for this shape.
             *
             * This may be used to initialise or override the internal extents from an AABB.
             * Typically the extents are derived from the AABB extents and its centre.
             *
             * @param box The AABB that defines the box extents and position.
             */
            void setAABB( const AABB3<real_Num> &box ) override;

            /**
             * @brief Check whether the shape contains valid geometry.
             *
             * A BoxShape3 is considered valid when all components of its extents are
             * strictly positive (non-zero and non-negative values indicate invalid shape).
             *
             * @return true if the shape is valid and can be used for physics queries,
             *         false otherwise.
             */
            bool isValid() const override;

            /**
             * @brief Class registration macro for reflection/serialization systems.
             *
             * Expands to declarations required by the project's runtime registration
             * and factory systems. Left here to be handled by the build/reflection macros.
             */
            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace physics
}  // namespace workphone

#endif  // BoxShape3_h__
