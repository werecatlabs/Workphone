#ifndef MeshComponent_h__
#define MeshComponent_h__

#include <Workphone/Mesh/ProgressiveMeshOptions.hpp>
#include <Workphone/Scene/Components/Component.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Component representing a renderable mesh attached to an entity.
         *
         * The Mesh component stores the path to a mesh asset, an optional loaded
         * mesh resource and an optional skeleton for skinning/animation. It
         * integrates with the component FSM to react to lifecycle events (load,
         * unload, activate, deactivate, etc.) and exposes property serialization
         * helpers used by the scene/property editors.
         */
        class WPCore_API Mesh : public Component
        {
        public:
            /** Property key for the mesh file path. */
            static const String m_meshPathStr;

            /** Property key for the mesh resource reference. */
            static const String m_meshStr;

            /** Property key for the skeleton resource reference. */
            static const String m_skeletonStr;

            /** Property key for the component FSM update priority. */
            static const String m_fsmPriorityStr;

            /** Property group containing this component's progressive mesh settings. */
            static const String m_progressiveMeshOptionsStr;

            /**
             * @brief Construct a Mesh component.
             *
             * Initializes internal state; does not load external resources.
             */
            Mesh();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of derived Component state.
             */
            ~Mesh() override;

            /**
             * @copydoc Component::load
             *
             * Expects the provided data to contain properties describing the mesh
             * (e.g. mesh path, resource id, skeleton). This will attempt to
             * resolve and/or load the mesh resource if a path or resource id is
             * provided.
             *
             * @param data Shared object containing serialized component data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::unload
             *
             * Releases any loaded mesh resources and clears transient state.
             *
             * @param data Shared object that may be used to persist state before unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Get the configured mesh file path.
             * @return The mesh file path as a String. May be empty if unset.
             */
            String getMeshPath() const;

            /**
             * @brief Set the mesh file path.
             *
             * Setting the path does not necessarily load the mesh immediately;
             * load
             * behavior depends on the component lifecycle state and the implementation of {@link load}.
             *
             * @param meshPath Path to the mesh asset file.
             */
            void setMeshPath( const String &meshPath );

            /**
             * @brief Get the currently assigned mesh resource.
             * @return Smart pointer to the mesh resource or nullptr if none.
             */
            SmartPtr<IMeshResource> getMeshResource() const;

            /**
             * @brief Assign a mesh resource directly.
             *
             * Useful when the resource is resolved/loaded externally and should
             * be reused by this component.
             *
             * @param meshResource Smart pointer to the mesh resource to assign.
             */
            void setMeshResource( SmartPtr<IMeshResource> meshResource );

            /**
             * @brief Get the skeleton used for skinning/animation.
             * @return Smart pointer to the skeleton or nullptr if none assigned.
             */
            SmartPtr<ISkeleton> getSkeleton() const;

            /**
             * @brief Set the skeleton used for skinning/animation.
             * @param skeleton Smart pointer to the skeleton to assign.
             */
            void setSkeleton( SmartPtr<ISkeleton> skeleton );

            const ProgressiveMeshOptions &getProgressiveMeshOptions() const;

            void setProgressiveMeshOptions( const ProgressiveMeshOptions &options );

            /**
             * @brief Serialize the component to a properties object.
             *
             * Returns a Properties object containing keys such as
             * meshPathStr, meshStr and skeletonStr so editors and serializers can
             * inspect and persist component state.
             *
             * @return Properties object representing this component's state.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Deserialize component state from a Properties object.
             *
             * The provided properties are used to configure the mesh path,
             * references to resources and other mesh-specific data.
             *
             * @param properties Properties object containing persisted state.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the FSM update priority for this component.
             * @return The priority value used when registering with the component FSM.
             */
            s32 getFsmPriority() const;

            /**
             * @brief Set the FSM update priority for this component.
             *
             * Higher values are processed earlier. Takes effect on the next load.
             *
             * @param priority The FSM priority value.
             */
            void setFsmPriority( s32 priority );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Handle internal component finite-state-machine events.
             *
             * Called by the Component base class when state transitions or
             * component-specific events occur. Implementations should respond to
             * load/unload and other lifecycle events as appropriate.
             *
             * @param state Current FSM state identifier.
             * @param eventType Event type raised for the component FSM.
             * @return FSMReturnType indicating how the FSM should proceed.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /** Smart pointer to the loaded or assigned mesh resource. */
            SmartPtr<IMeshResource> m_meshResource;

            /** Smart pointer to the skeleton used for skinning/animation. */
            SmartPtr<ISkeleton> m_skeleton;

            /** Configured mesh file path (may be empty). */
            FixedString<256> m_meshPath;

            /** FSM update priority applied when the component is loaded. */
            s32 m_fsmPriority = 15000;

            /** Per-component settings, initially copied from mesh import metadata. */
            ProgressiveMeshOptions m_progressiveMeshOptions;

            /**
             * Component ID extension used to generate unique component identifiers
             * when multiple Mesh components are present. Incremented by the class
             * implementation.
             */
            static u32 m_idExt;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // MeshComponent_h__
