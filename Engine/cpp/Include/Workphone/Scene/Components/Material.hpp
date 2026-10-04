#ifndef MaterialComponent_h__
#define MaterialComponent_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Represents a material component attached to a scene node.
         *
         * The Material component holds a reference to a renderable material, an
         * optional path to material data, and an index that can be used when a
         * node contains multiple materials or sub-meshes. It integrates with the
         * engine's event and state systems to react to material resource
         * changes and to update dependent components (for example image or
         * renderer components) when the material is changed or reloaded.
         *
         * Responsibilities:
         * - Load and unload material data from shared objects.
         * - Maintain and expose the current material and material path.
         * - Notify and update dependent components when the material changes.
         * - Provide listeners for object and state events related to material
         *   resources.
         *
         * @class Material
         * @ingroup Scene
         */
        class WPCore_API Material : public Component
        {
        public:
            /**
             * @brief Event listener that observes shared-object lifecycle events for
             *        material resources.
             *
             * This listener is attached to material resource objects and receives
             * unload and generic event callbacks from the engine. It keeps a
             * weak reference to its owning Material component so it can forward
             * or trigger updates without owning the component strongly and
             * preventing its destruction.
             */
            class MaterialStateObjectListener : public IEventListener
            {
            public:
                /** Default constructor. */
                MaterialStateObjectListener();

                /** Virtual destructor. */
                ~MaterialStateObjectListener() override;

                /**
                 * Called when the observed shared object is being unloaded.
                 * @param data The shared object that is unloading.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * Generic event handler forwarded from the engine event bus.
                 * Implementations should handle events relevant to material
                 * resource changes and propagate them to the owning Material.
                 *
                 * @param eventType Type of the event.
                 * @param eventValue Numeric event value (hash or id).
                 * @param arguments Event arguments.
                 * @param sender The sender of the event.
                 * @param object The object associated with the event.
                 * @param event The full event object.
                 * @return A Parameter value used by the event system.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * Get the owning Material component (strong reference).
                 * @return SmartPtr to the owner or null if owner expired.
                 */
                SmartPtr<Material> getOwner() const;

                /**
                 * Set the owning Material component (strong reference).
                 * @param owner The Material component that owns this listener.
                 */
                void setOwner( SmartPtr<Material> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                AtomicWeakPtr<Material> m_owner;  ///< Weak pointer to the owning Material component.
            };

            /**
             * @brief Listener for state-related notifications for material
             *        resources.
             *
             * Receives state messages and change notifications and informs the
             * owning Material component so it can refresh or rebind stateful
             * material properties.
             */
            class MaterialStateListener : public IStateListener
            {
            public:
                /** Default constructor. */
                MaterialStateListener();

                /**
                 * Construct with a raw owner pointer. This does not create a
                 * strong ownership relationship; the listener stores a weak
                 * reference internally.
                 * @param owner Raw pointer to the owning Material.
                 */
                explicit MaterialStateListener( Material *owner );

                /** Virtual destructor. */
                ~MaterialStateListener() override;

                /**
                 * Handle incoming state messages.
                 * @param message The state message to process.
                 * @return True if the message was handled.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * Called when the observed state object changes.
                 * @param state The new state object.
                 * @return True if the change was handled.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /** Get the owning Material component (strong reference). */
                SmartPtr<Material> getOwner() const;

                /** Set the owning Material component. */
                void setOwner( SmartPtr<Material> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                AtomicWeakPtr<Material> m_owner;  ///< Weak pointer to the owning Material component.
            };

            /**
             * @brief Get the property key used for the main texture parameter.
             * @return The main texture property key string.
             */
            String getMainTextureStr() const;

            /**
             * @brief Set the property key used for the main texture parameter.
             * @param key The new key string.
             */
            void setMainTextureStr( const String &key );

            /**
             * @brief Get the property key used for the material resource.
             * @return The material property key string.
             */
            String getMaterialStr() const;

            /**
             * @brief Set the property key used for the material resource.
             * @param key The new key string.
             */
            void setMaterialStr( const String &key );

            /**
             * @brief Get the property key used for the material path.
             * @return The material path property key string.
             */
            String getMaterialPathStr() const;

            /**
             * @brief Set the property key used for the material path.
             * @param key The new key string.
             */
            void setMaterialPathStr( const String &key );

            /**
             * @brief Get the property key used for the material index.
             * @return The index property key string.
             */
            String getIndexStr() const;

            /**
             * @brief Set the property key used for the material index.
             * @param key The new key string.
             */
            void setIndexStr( const String &key );

            /**
             * @brief Construct a Material component.
             *
             * Initializes internal listeners and state. The created component is
             * initially empty (no material set) and must be configured via
             * setMaterialPath or setMaterial.
             */
            Material();

            /** Destructor. */
            ~Material() override;

            /**
             * @copydoc Component::load
             *
             * Loads material properties from the given shared object. The
             * implementation should read material path, index and create any
             * necessary listeners for resource lifetime management.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::unload
             *
             * Unregisters listeners and releases the material reference.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::updateDirty
             *
             * Called when component flags change; used to react to changes that
             * affect material binding or visibility.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @copydoc Component::getProperties
             *
             * Returns a Properties object describing the current material
             * configuration (material path, index, etc.).
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc Component::setProperties
             *
             * Apply properties previously returned by getProperties or loaded
             * from serialized scene data. Typical properties include material
             * path and index.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc Component::getChildObjects
             *
             * Returns child shared objects that this component depends on, such
             * as the material resource object so the scene system can track
             * dependencies.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Get the file or resource path associated with this material.
             * @return The material path string. Empty if no path is set.
             */
            String getMaterialPath() const;

            /**
             * @brief Set the file/resource path for the material and trigger
             *        loading or binding as required.
             * @param path The material path to use.
             */
            void setMaterialPath( const String &path );

            /**
             * @brief Get the currently bound render material.
             * @return SmartPtr to the render::IMaterial instance or null if
             *         none is bound.
             */
            SmartPtr<render::IMaterial> getMaterial() const;

            /**
             * @brief Set the render material directly.
             * @param material SmartPtr to the render::IMaterial to assign.
             */
            void setMaterial( SmartPtr<render::IMaterial> material );

            /**
             * @brief Refresh internal bindings and state for the current
             *        material. Called after material data changes or is
             *        reloaded.
             */
            void updateMaterial();

            /**
             * @brief Notify and update components that depend on this
             *        material (for example image or renderer components).
             */
            void updateDependentComponents() override;

            /**
             * @brief Update a linked image component (if any) to reflect the
             *        material's main texture or other texture parameters.
             */
            void updateImageComponent();

            /**
             * @brief Get the material index used when multiple materials exist
             *        for a single renderable (sub-mesh index).
             * @return The index value.
             */
            u32 getIndex() const;

            /**
             * @brief Set the material index for multi-material renderables.
             * @param index The index to set.
             */
            void setIndex( u32 index );

            /**
             * @brief Get the event listener attached to the material resource
             *        object.
             * @return SmartPtr to the IEventListener used for material object events.
             */
            SmartPtr<IEventListener> getMaterialObjectListener() const;

            /**
             * @brief Set the event listener used to observe material resource
             *        object events. The listener will be retained by the
             *        component.
             * @param materialObjectListener The listener to attach.
             */
            void setMaterialObjectListener( SmartPtr<IEventListener> materialObjectListener );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Internal helper used to recompute or apply flags that
             *        affect the material component. This is different from
             *        the public updateFlags override which receives old/new
             *        flag values from the component base class.
             */
            void updateFlags();

            /**
             * @copydoc Component::handleComponentEvent
             *
             * Handles FSM-level events targeted at this component (for
             * example activation/deactivation) and updates internal state as
             * needed.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            AtomicSmartPtr<IEventListener>
                m_materialObjectListener;  ///< Listener attached to the material resource object.
            SmartPtr<IStateListener> m_materialListener;  ///< Listener for material state changes.
            SmartPtr<render::IMaterial> m_material;       ///< Currently assigned render material.
            FixedString<256> m_materialPath;              ///< Path to the material resource.
            atomic_s32 m_index;  ///< Material index for multi-material meshes (default 0).

            FixedString<32> m_mainTextureStr;   ///< Property key for the main texture parameter.
            FixedString<32> m_materialStr;      ///< Property key for the material resource.
            FixedString<32> m_materialPathStr;  ///< Property key for the material path.
            FixedString<32> m_indexStr;         ///< Property key for the material index.
        };
    }  // namespace scene
}  // namespace workphone

#endif  // MaterialComponent_h__
