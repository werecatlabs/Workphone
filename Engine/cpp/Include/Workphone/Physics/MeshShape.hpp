#ifndef MeshShape_h__
#define MeshShape_h__

#include <Workphone/Interface/Physics/IMeshShape.hpp>
#include <Workphone/Physics/PhysicsShape3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @file MeshShape.hpp
         * @brief Mesh collision shape used by the physics system.
         *
         * This header declares the MeshShape class which wraps mesh-based collision
         * geometry used by the physics manager. The shape can represent either a
         * triangle mesh or a convex hull depending on configuration.
         */

        /**
         * @class MeshShape
         * @brief Concrete mesh-based physics shape implementation.
         *
         * MeshShape implements the IMeshShape interface through the PhysicsShape3
         * CRTP base. It manages the lifetime of underlying mesh data used for
         * collision detection and exposes methods to load/unload that data and
         * query or modify convexity settings.
         *
         * Typical usage:
         *  - call load() with a mesh shared object to create the physics geometry
         *  - call isValid() to check if the physics geometry is ready
         *  - call setConvex(true) to request a convex representation if supported
         *
         * @note Changing convexity may require the underlying physics representation
         * to be rebuilt. The exact rebuild behaviour depends on the physics backend.
         */
        class WPCore_API MeshShape : public PhysicsShape3<IMeshShape>
        {
        public:
            /**
             * @brief Construct an empty MeshShape.
             *
             * The constructed shape is initially invalid until data is loaded.
             */
            MeshShape();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived cleanup runs and any physics resources are released.
             */
            ~MeshShape() override;

            /**
             * @brief Load mesh data into the physics shape.
             *
             * Loads or binds the provided shared object (mesh resource) to this
             * physics shape. The exact accepted object type is dependent on the
             * engine's resource system; callers should pass a SmartPtr to a mesh
             * or mesh-like shared object.
             *
             * @param data SmartPtr to an ISharedObject representing mesh data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload mesh data from the physics shape.
             *
             * Releases or unbinds the mesh resource associated with this shape.
             * After unload() the shape should become invalid until load() is
             * called again with valid data.
             *
             * @param data SmartPtr to the previously loaded ISharedObject. Implementations
             *             may ignore the parameter if they track the bound resource internally.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Query whether this shape has valid physics geometry.
             *
             * Returns true when the underlying physics representation (triangle
             * mesh, convex hull, etc.) has been successfully created and is ready
             * for simulation or queries.
             *
             * @return true if the physics geometry is valid and usable; false otherwise.
             */
            bool isValid() const override;

            /**
             * @brief Query whether this mesh shape is treated as convex.
             *
             * Convex shapes are typically faster for collision/test queries and may
             * use different internal representations compared to triangle meshes.
             *
             * @return true if the shape is configured as convex; false if it is a triangle mesh.
             */
            bool isConvex() const override;

            /**
             * @brief Set whether this mesh should be treated as convex.
             *
             * Setting convexity may trigger a rebuild of internal physics data.
             * Callers should ensure the mesh is appropriate for a convex hull
             * (e.g., roughly convex in shape) before requesting a convex representation.
             *
             * @param convex true to treat the mesh as convex; false to treat as triangle mesh.
             */
            void setConvex( bool convex ) override;

            /**
             * @brief Macro to register the class with the engine's runtime/type system.
             *
             * The specific behaviour of this macro depends on the engine's reflection
             * and serialization facilities.
             */
            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace physics
}  // namespace workphone

#endif  // MeshShape_h__
