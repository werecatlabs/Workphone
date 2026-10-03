#ifndef __MeshRenderer_h__
#define __MeshRenderer_h__

#include <Workphone/Scene/Components/Renderer.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class MeshRenderer
         * @brief Component responsible for rendering mesh objects in the scene.
         *
         * The MeshRenderer class derives from Renderer and manages the rendering of mesh objects.
         * It provides methods to load, unload, and update mesh data, as well as to handle events and
         * manage properties.
         */
        class WPCore_API MeshRenderer : public Renderer
        {
        public:
            static const String meshNodeSuffixStr;
            static const String meshPathStr;

            /**
             * @brief Default constructor.
             */
            MeshRenderer();

            /**
             * @brief Destructor.
             */
            ~MeshRenderer() override;

            /**
             * @brief Loads the mesh renderer with the given data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the mesh renderer and releases resources.
             * @param data Shared object containing data for unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates internal flags for the renderer.
             * @param flags New flags to set.
             * @param oldFlags Previous flags.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @brief Gets the child objects associated with this renderer.
             * @return Array of shared pointers to child objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the properties of the mesh renderer.
             * @return Shared pointer to the properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the mesh renderer.
             * @param properties Shared pointer to the properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Updates the materials used by the mesh renderer.
             */
            void updateMaterials() override;

            /**
             * @brief Gets the scene node associated with the mesh.
             * @return Shared pointer to the scene node.
             */
            SmartPtr<render::IGraphicsSceneNode> getMeshNode() const;

            /**
             * @brief Sets the scene node associated with the mesh.
             * @param meshNode Shared pointer to the scene node.
             */
            void setMeshNode( SmartPtr<render::IGraphicsSceneNode> meshNode );

            /**
             * @brief Handles a component-specific event for the mesh renderer.
             * @param state The current state of the component.
             * @param eventType The type of FSM event.
             * @return FSMReturnType indicating the result of the event handling.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Updates the mesh data and state.
             */
            void updateMesh() override;

            /**
             * @brief Handles an event for the mesh renderer.
             * @param eventType The type of event.
             * @param eventValue The value associated with the event.
             * @param arguments Array of event parameters.
             * @param sender Shared pointer to the sender object.
             * @param object Shared pointer to the related object.
             * @param event Shared pointer to the event object.
             * @return Parameter containing the result of the event handling.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Calculates the axis-aligned bounding box for the mesh.
             * @return The calculated bounding box.
             */
            AABB3<real_Num> calculateBoundingBox() override;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // __MeshRenderer_h__
