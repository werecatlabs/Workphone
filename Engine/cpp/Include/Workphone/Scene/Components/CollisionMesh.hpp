#ifndef __Component_CollisionMesh_h__
#define __Component_CollisionMesh_h__

#include <Workphone/Scene/Components/Collision.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Component that represents a collision shape generated from a mesh.
         *
         * This component wraps a mesh resource and creates an appropriate physics
         * collision shape from its geometry. The mesh can be used as a convex or
         * concave collision shape depending on the `m_isConvex` flag. The component
         * manages loading/unloading of the mesh resource and provides properties
         * for serialization and editor integration.
         */
        class WPCore_API CollisionMesh : public Collision
        {
        public:
            static const String meshStr;
            static const String meshPathStr;
            static const String isConvexStr;

            /**
             * @brief Default constructor.
             *
             * Initializes internal state to default values. The mesh resource is
             * initially empty and the convex flag is false.
             */
            CollisionMesh();

            /**
             * @brief Destructor.
             *
             * Cleans up any owned resources. Override ensures proper polymorphic
             * destruction through base class pointers.
             */
            ~CollisionMesh() override;

            /**
             * @copydoc Collision::load
             *
             * Loads component data from the given shared object. Expected data
             * includes mesh path and convex flag. This will schedule or perform
             * the mesh resource acquisition used to build the physics shape.
             *
             * @param data Shared object containing serialized component data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Collision::unload
             *
             * Releases or resets any runtime resources associated with the mesh
             * when the component is being removed or the scene is unloading.
             *
             * @param data Shared object that may receive unload information.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Collision::getProperties
             *
             * Returns a properties object describing the mesh path and convex
             * state for editor/serialization purposes.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc Collision::setProperties
             *
             * Applies property values (such as mesh path and convex flag) to the
             * component and updates internal state accordingly.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the file path to the mesh used by this collision component.
             *
             * @return Path to the mesh resource as a String.
             */
            String getMeshPath() const;

            /**
             * @brief Set the file path to the mesh used for collision geometry.
             *
             * Setting a new mesh path will not necessarily load the resource
             * immediately; it updates the component state so the resource can be
             * loaded when required by the scene or physics system.
             *
             * @param meshPath Path to the mesh resource.
             */
            void setMeshPath( const String &meshPath );

            /**
             * @brief Get the mesh resource currently associated with this component.
             *
             * This may be null if the resource has not been loaded yet.
             *
             * @return SmartPtr to the IMeshResource or null.
             */
            SmartPtr<IMeshResource> getMeshResource() const;

            /**
             * @brief Associate a loaded mesh resource with this component.
             *
             * When a mesh resource is provided directly it will be used to build
             * or update the physics collision shape.
             *
             * @param meshResource Smart pointer to the mesh resource to use.
             */
            void setMeshResource( SmartPtr<IMeshResource> meshResource );

            /**
             * @brief Query whether the collision mesh should be treated as convex.
             *
             * Convex meshes use a simplified collision representation suitable
             * for dynamic objects, while concave meshes are typically used for
             * static level geometry.
             *
             * @return True if the mesh is marked convex; false otherwise.
             */
            bool isConvex() const;

            /**
             * @brief Set whether the mesh should be treated as convex when creating
             * the physics collision shape.
             *
             * @param convex True to use a convex collision shape; false for concave.
             */
            void setConvex( bool convex );

            /**
             * @copydoc Collision::isValid
             *
             * A collision mesh is valid when it has an associated mesh resource
             * that can produce a physics shape.
             */
            bool isValid() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @copydoc Collision::handleComponentEvent
             *
             * Handles component lifecycle and scene events that may affect the
             * collision mesh (for example: load, unload, or property change).
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @copydoc Collision::createPhysicsShape
             *
             * Constructs the physics engine shape from the current mesh resource
             * and convex flag.
             */
            void createPhysicsShape() override;

            /**
             * @brief Create or update internal data structures required for the
             * physics mesh based on the associated mesh resource.
             *
             * This performs the actual preparation of vertex/index data and
             * any conversion required by the underlying physics engine.
             */
            void setupMesh();

            /**
             * @brief True when the component should treat the mesh as convex.
             *
             * Default is false (concave).
             */
            bool m_isConvex = false;

            /**
             * @brief Path to the mesh asset used by this component.
             *
             * This is typically a relative path into the project's asset system
             * and is used to locate and load the mesh resource when needed.
             */
            FixedString<256> m_meshPath;

            /**
             * @brief Cached pointer to the loaded mesh resource.
             *
             * May be null if the mesh has not been loaded or was unloaded.
             */
            SmartPtr<IMeshResource> m_meshResource;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CollisionMesh_h__
