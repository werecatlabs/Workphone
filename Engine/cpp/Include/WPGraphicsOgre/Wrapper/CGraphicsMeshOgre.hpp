#ifndef _WP_CGraphicsMeshOgre_H_
#define _WP_CGraphicsMeshOgre_H_

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <Workphone/Interface/Script/IScriptReceiver.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>
#include <OgreMesh.h>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Ogre-backed implementation of IGraphicsMesh.
         *
         * This class wraps an Ogre::Entity/Ogre::Mesh and exposes the engine's
         * IGraphicsMesh interface. It manages materials, skeletons, animation
         * controllers, script receivers and state listeners required for the mesh.
         *
         * Responsibilities:
         * - Load/unload mesh resources.
         * - Manage per-submesh materials and material updates.
         * - Provide access to the underlying Ogre::Entity for low-level operations.
         *
         * @see IGraphicsMesh
         */
        class CGraphicsMeshOgre : public CGraphicsObjectOgre<IGraphicsMesh>
        {
        public:
            /**
             * @brief Script receiver for mesh properties and script-driven control.
             *
             * Receives script property set/get calls for this mesh instance and
             * forwards them into the mesh object. Typically used by scripting
             * subsystems to manipulate mesh properties at runtime.
             */
            class ScriptReceiver : public IScriptReceiver
            {
            public:
                /**
                 * @brief Construct a ScriptReceiver bound to a mesh.
                 * @param meshObject Pointer to the owning mesh object (may be nullptr).
                 */
                ScriptReceiver( CGraphicsMeshOgre *meshObject );

                /**
                 * @brief Set a string property by hash.
                 * @param hash Property identifier.
                 * @param value Property value as string.
                 * @return A status code (implementation-defined).
                 */
                s32 setProperty( hash_type hash, const String &value ) override;

                /**
                 * @brief Set a property by hash using a Parameter wrapper.
                 * @param hash Property identifier.
                 * @param param Property value as Parameter.
                 * @return A status code (implementation-defined).
                 */
                s32 setProperty( hash_type hash, const Parameter &param ) override;

                /**
                 * @brief Get a property by hash.
                 * @param id Property identifier.
                 * @param param Output parameter to receive the property value.
                 * @return A status code (implementation-defined).
                 */
                s32 getProperty( hash_type id, Parameter &param ) const override;

                WP_CLASS_REGISTER_DECL;

            protected:
                /// Non-owning pointer to the associated mesh object.
                CGraphicsMeshOgre *m_meshObject = nullptr;
            };

            /**
             * @brief Listener that responds to state messages targeted at the mesh.
             *
             * Inherits from the engine's StateListener base used by various Ogre wrappers.
             * Handles messages and state changes that affect mesh-level behaviour.
             */
            class MeshStateListener : public GraphicsObjectOgreStateListener
            {
            public:
                MeshStateListener();
                ~MeshStateListener() override;

                /**
                 * @brief Handle a state message.
                 * @param message The state message to handle.
                 * @return True if the message was handled, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle notification that a state has changed.
                 * @param state The state that changed.
                 * @return True if the change was handled, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                WP_CLASS_REGISTER_DECL;
            };

            /**
             * @brief Material state listener which notifies the mesh when materials change.
             *
             * This listener keeps a (atomic) reference to the owning mesh and is used to
             * react to material reload/unload or runtime material parameter changes.
             */
            class MaterialStateListener : public IStateListener
            {
            public:
                MaterialStateListener();
                /**
                 * @brief Create a material listener bound to an owner.
                 * @param owner Pointer to the owning mesh instance.
                 */
                MaterialStateListener( CGraphicsMeshOgre *owner );
                ~MaterialStateListener() override;

                /**
                 * @brief Called when associated material data should be unloaded.
                 * @param data Optional data passed by the state system.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle a state message for the material.
                 * @param message The incoming state message.
                 * @return True if the message was handled.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle notification that a material state has changed.
                 * @param state The state object that changed.
                 * @return True if the change was handled.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the atomic owner reference.
                 * @return SmartPtr to the owning CGraphicsMeshOgre instance.
                 */
                SmartPtr<CGraphicsMeshOgre> getOwner() const;

                /**
                 * @brief Set the owner of this listener.
                 * @param owner SmartPtr to the owning mesh.
                 */
                void setOwner( SmartPtr<CGraphicsMeshOgre> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /// Atomic smart pointer to the owning mesh to avoid races during destruction.
                AtomicSmartPtr<CGraphicsMeshOgre> m_owner;
            };

            CGraphicsMeshOgre();
            ~CGraphicsMeshOgre() override;

            /**
             * @brief Load mesh data / resources.
             * @param data Optional data required for loading.
             *
             * Implementations should acquire Ogre resources and set up internal state.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload mesh data / resources.
             * @param data Optional data passed for unload operations.
             *
             * Implementations should release Ogre resources and clear internal state.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Update mesh state. Called every frame or when registered for updates.
             *
             * Use this to refresh animations, material updates or other per-frame logic.
             */
            void update() override;

            /**
             * @brief Register or unregister this mesh for per-frame updates.
             * @param registerObject True to register for updates, false to unregister.
             */
            void registerForUpdates( bool registerObject );

            /**
             * @brief Query whether this mesh is registered for updates.
             * @return True if registered for per-frame updates.
             */
            bool isRegisteredForUpdates();

            /**
             * @brief Enable or disable occlusion testing for this mesh.
             * @param testOcclusion True to enable occlusion testing.
             */
            void setTestOcclusion( bool testOcclusion );

            /**
             * @brief Check whether occlusion testing is enabled.
             * @return True if occlusion testing is enabled.
             */
            bool getTestOcclusion() const;

            /**
             * @brief Mark this mesh as an occluder or not.
             * @param occluder True to treat mesh as occluder.
             */
            void setIsOccluder( bool occluder );

            /**
             * @brief Query whether this mesh is an occluder.
             * @return True if the mesh is an occluder.
             */
            bool isOccluder() const;

            /**
             * @brief Detach the underlying Ogre entity/node from its parent.
             *
             * This leaves the entity un-parented so it can be attached elsewhere or destroyed safely.
             */
            void detachFromParent();

            /**
             * @brief Attach this mesh to a scene node parent.
             * @param parent The scene node to attach to.
             */
            void attachToParent( SmartPtr<IGraphicsSceneNode> parent );

            /**
             * @brief Set the material name for the mesh (or submesh index).
             * @param materialName Name of the material to assign.
             * @param index Submesh index (-1 to set default/all).
             */
            void setMaterialName( const String &materialName, s32 index = -1 ) override;

            /**
             * @brief Get the material name for a submesh or default.
             * @param index Submesh index (-1 for default).
             * @return The material name string.
             */
            String getMaterialName( s32 index = -1 ) const override;

            /** @copydoc IGraphicsMesh::setMaterial */
            void setMaterial( SmartPtr<IMaterial> material, s32 index = -1 ) override;

            /** @copydoc IGraphicsMesh::getMaterial */
            SmartPtr<IMaterial> getMaterial( s32 index = -1 ) const override;

            /**
             * @brief Create a deep or shallow clone of this graphics object.
             * @param name Optional name for the cloned object.
             * @return New graphics object representing the cloned mesh.
             */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Retrieve the raw underlying object pointer.
             * @param ppObject Output pointer to receive the underlying object (Ogre-specific).
             *
             * @note The function writes a pointer to the engine-specific object, for example an
             * Ogre::Entity or Ogre::Mesh reference, into the provided pointer reference.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Enable/disable hardware skinning/animation support.
             * @param enabled True to enable hardware animation.
             */
            void setHardwareAnimationEnabled( bool enabled ) override;

            /**
             * @brief Ensure vertex processing is correct for the current render path.
             *
             * This checks whether vertex processing (software vs hardware) is valid for the mesh
             * and updates internal flags/state accordingly.
             */
            void checkVertexProcessing() override;

            /**
             * @brief Get or create the animation controller for this mesh.
             * @return SmartPtr to the animation controller or nullptr if none.
             */
            SmartPtr<IAnimationController> getAnimationController() override;

            /**
             * @brief Get the underlying Ogre::Entity if one exists.
             * @return Raw pointer to the Ogre::Entity, or nullptr if not created.
             * @note Do not take ownership of the returned pointer.
             */
            Ogre::Entity *getEntity() const;

            /**
             * @brief Generic event handler hooked into the engine's event system.
             * @param event Event to handle.
             */
            void handleEvent( SmartPtr<IEvent> event );

            /**
             * @brief Get the mesh resource name used by this object.
             * @return The mesh name string.
             */
            String getMeshName() const override;

            /**
             * @brief Set the mesh resource name for this object.
             * @param meshName Name of the mesh resource to use.
             */
            void setMeshName( const String &meshName ) override;

            ProgressiveMeshOptions getProgressiveMeshOptions() const override;

            void setProgressiveMeshOptions( const ProgressiveMeshOptions &options ) override;

            /**
             * @brief Get mesh-specific properties as a Properties object.
             * @return Properties object containing mesh metadata.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Set mesh-specific properties from a Properties object.
             * @param properties Properties object to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get child shared objects owned by this mesh (materials, skeletons, etc.).
             * @return Array of child shared objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Get the graphics skeleton associated with this mesh.
             * @return SmartPtr to the skeleton or nullptr if none.
             */
            SmartPtr<IGraphicsSkeleton> getSkeleton() const override;

            /**
             * @brief Set the graphics skeleton to use for this mesh.
             * @param skeleton Skeleton object to assign.
             */
            void setSkeleton( SmartPtr<IGraphicsSkeleton> skeleton ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Callback invoked when a material is loaded or updated.
             * @param material The material that was loaded/updated.
             */
            void materialLoaded( SmartPtr<IMaterial> material );

            /**
             * @brief Create or retrieve an Ogre::Mesh with the given name.
             * @param meshName Name of the mesh resource.
             * @return Ogre::MeshPtr referencing the created or found mesh.
             */
            Ogre::MeshPtr createMesh( const String &meshName );

            /**
             * @brief Setup the object's default state listener / state object.
             *
             * Called during load/setup to register state listeners and initial state.
             */
            void setupStateObject() override;

            /// Invoker used to call scripted functions on this mesh.
            SmartPtr<IScriptInvoker> m_scriptInvoker;
            /// Receiver used to accept script property calls.
            SmartPtr<IScriptReceiver> m_scriptReceiver;

            /// Listener used to watch material state changes.
            SmartPtr<IStateListener> m_materialStateListener;

            /// Animation controller for running mesh animations.
            SmartPtr<IAnimationController> m_animationController;

            /// Default material assigned to the mesh.
            SmartPtr<IMaterial> m_material;

            /// Skeleton used by the mesh (if any).
            SmartPtr<IGraphicsSkeleton> m_skeleton;

            /// Raw pointer to the underlying Ogre::Entity (non-owning).
            Ogre::Entity *m_entity = nullptr;

            /// Timestamp of the last material update (atomic for thread-safety).
            atomic_u32 m_lastMaterialUpdate;
            /// Timestamp of the last material request (atomic for thread-safety).
            atomic_u32 m_lastMaterialRequest;

            /// Flag tracking whether vertex processing should be checked/updated.
            bool m_checkVertProcessing;
            /// Whether hardware animation/skin is enabled.
            bool m_hardwareAnimationEnabled;

            /// Mesh resource name.
            String m_meshName;
            /// Default material name used by the mesh.
            String m_materialName;

            AtomicObject<ProgressiveMeshOptions> m_progressiveMeshOptions;

            /// Per-submesh material list.
            Array<SmartPtr<IMaterial>> m_materials;
        };
    }  // end namespace render
}  // namespace workphone

#endif
