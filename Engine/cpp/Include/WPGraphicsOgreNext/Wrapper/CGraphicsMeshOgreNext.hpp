#ifndef _WP_CGraphicsMeshOgre_H_
#define _WP_CGraphicsMeshOgre_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsObjectOgreNext.hpp>
#include <Workphone/Graphics/GraphicsMesh.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <OgreMesh.h>

namespace Ogre
{
    class HlmsDatablock;
    class HlmsPbsDatablock;
    class Renderable;
    class TextureGpu;
}

namespace workphone
{
    namespace render
    {

        /**
         * @class CGraphicsMeshOgreNext
         * @brief Ogre (Next) implementation of a graphics mesh wrapper.
         *
         * This class wraps Ogre mesh/item/entity objects and exposes the engine's
         * IGraphicsMesh interface. It manages materials, skeletons, animation controllers,
         * resource loading and state/event listeners for mesh/material changes.
         *
         * The class derives from CGraphicsObjectOgreNext<GraphicsMesh> to integrate with
         * the shared object lifecycle and scene management used by the engine.
         */
        class CGraphicsMeshOgreNext : public CGraphicsObjectOgreNext<GraphicsMesh>
        {
        public:
            /**
             * @class MeshMaterialEventListener
             * @brief Event listener for material-related events for the mesh.
             *
             * Receives engine events (for example material loaded/unloaded) and dispatches
             * handling to the owning mesh wrapper.
             */
            class MeshMaterialEventListener : public IEventListener
            {
            public:
                MeshMaterialEventListener();
                ~MeshMaterialEventListener() override;

                /**
                 * @brief Handle an event.
                 * @param eventType The event type.
                 * @param eventValue Associated numeric event value.
                 * @param arguments Event arguments.
                 * @param sender Sender of the event.
                 * @param object Object associated with the event.
                 * @param event Event object.
                 * @return Parameter result that may carry event-specific data.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Get the owning mesh wrapper.
                 * @return Smart pointer to the owner (may be null).
                 */
                SmartPtr<CGraphicsMeshOgreNext> getOwner() const;

                /**
                 * @brief Set the owner for this event listener.
                 * @param owner Smart pointer to the owning mesh wrapper.
                 */
                void setOwner( SmartPtr<CGraphicsMeshOgreNext> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /// Weak pointer to the owner to avoid keeping it alive.
                WeakPtr<CGraphicsMeshOgreNext> m_owner;
            };

            /**
             * @class GraphicsMeshManualResourceLoader
             * @brief Manual resource loader used by Ogre to lazily load mesh data from an engine stream/resource.
             *
             * This implements Ogre::ManualResourceLoader to provide data when Ogre requests
             * a mesh resource to be loaded/prepared. The loader is intentionally stateless;
             * Ogre resources can outlive individual CGraphicsMeshOgreNext wrappers and may
             * call back from different threads.
             */
            class GraphicsMeshManualResourceLoader : public Ogre::ManualResourceLoader
            {
            public:
                GraphicsMeshManualResourceLoader();

                ~GraphicsMeshManualResourceLoader() override;

                /**
                 * @brief Called by Ogre when the resource must be loaded immediately.
                 * @param resource The Ogre resource to load.
                 */
                void loadResource( Ogre::Resource *resource ) override;

                /**
                 * @brief Called by Ogre to prepare the resource (optional, may be used to defer heavy work).
                 * @param resource The Ogre resource to prepare.
                 */
                void prepareResource( Ogre::Resource *resource ) override;

                String getFilePath() const;
                void setFilePath( const String &filePath );

                ProgressiveMeshOptions getProgressiveMeshOptions() const;
                void setProgressiveMeshOptions( const ProgressiveMeshOptions &options );

                AtomicObject<FixedString<512>> m_meshFilePath;
                AtomicObject<ProgressiveMeshOptions> m_progressiveMeshOptions;
            };

            struct AutomaticCubemapBinding
            {
                SmartPtr<IMaterial> material;
                SmartPtr<ITexture> texture;
                Ogre::Renderable *renderable = nullptr;
                Ogre::HlmsDatablock *sourceDatablock = nullptr;
                Ogre::HlmsPbsDatablock *automaticDatablock = nullptr;
                Ogre::TextureGpu *textureGpu = nullptr;
            };

            CGraphicsMeshOgreNext();
            CGraphicsMeshOgreNext( const CGraphicsMeshOgreNext &other ) = delete;
            CGraphicsMeshOgreNext( SmartPtr<IGraphicsScene> creator );

            ~CGraphicsMeshOgreNext() override;

            /**
             * @copydoc ISharedObject::load
             * @param data Optional data passed in when loading.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::reload
             * @param data Optional data passed in when reloading.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             * @param data Optional data passed in when unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Clone this graphics object.
             * @param name Optional name for the cloned object.
             * @return New graphics object (clone).
             */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Get the raw underlying engine object pointer.
             * @param ppObject Pointer to receive the underlying object pointer.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Check vertex processing requirements and update internal flags.
             *
             * This validates whether the current mesh/data requires CPU or GPU skinning and
             * sets up animation flags appropriately.
             */
            void checkVertexProcessing() override;

            /**
             * @brief Get the animation controller for this mesh (if any).
             * @return Animation controller smart pointer or null.
             */
            SmartPtr<IAnimationController> getAnimationController() override;

            /**
             * @copydoc IGraphicsObject::setCreator
             */
            void setCreator( SmartPtr<IGraphicsScene> creator ) override;

            /**
             * @copydoc IGraphicsObject::getProperties
             * @return Properties describing the mesh.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IGraphicsObject::setProperties
             * @param properties Properties to apply to the mesh object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc IGraphicsObject::getChildObjects
             * @return Array of child shared objects (materials, skeletons, etc.).
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Get the legacy Ogre v1 Entity pointer (if used).
             * @return Pointer to Ogre::v1::Entity or nullptr.
             */
            Ogre::v1::Entity *getEntity() const;

            /**
             * @brief Get the Ogre::Item for Ogre Next rendering.
             * @return Pointer to Ogre::Item or nullptr.
             */
            Ogre::Item *getItem() const;

            /**
             * @brief Set the Ogre::Item used to render this mesh.
             * @param item Raw pointer to the Ogre::Item. The class does not take ownership.
             */
            void setItem( Ogre::Item *item );

            /**
             * @brief Get the associated graphics skeleton (if any).
             * @return Smart pointer to the skeleton.
             */
            SmartPtr<IGraphicsSkeleton> getSkeleton() const override;

            /**
             * @brief Set the graphics skeleton to use for this mesh.
             * @param skeleton Skeleton to attach to this mesh.
             */
            void setSkeleton( SmartPtr<IGraphicsSkeleton> skeleton ) override;

            /**
             * @brief Handle a state message sent to this mesh object.
             * @param message The state message to handle.
             * @return True if the message was handled.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handle a state object change notification.
             * @param state The state that changed.
             * @return True if the change was handled.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Callback invoked when a material shared object is loaded.
             * @param material Material that was loaded.
             */
            void materialLoaded( SmartPtr<IMaterial> material );

            /**
             * @brief Apply the best automatic cubemap for this mesh's current world position.
             */
            void updateAutomaticCubemap();

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Setup the state object for this mesh (register listeners, initial state).
             *
             * Called during load/initialization to connect material and other state listeners.
             */
            void setupStateObject() override;

            /**
             * @brief Create or retrieve an Ogre mesh with the given name.
             * @param meshName The name to use for the Ogre mesh resource.
             * @return Ogre::MeshPtr to the created or existing mesh.
             */
            Ogre::MeshPtr createMesh( const String &meshName );

            void addMaterialListeners( SmartPtr<IMaterial> material );

            void removeMaterialListeners( SmartPtr<IMaterial> material );

            void cacheMaterial( SmartPtr<IMaterial> material, s32 index );

            bool isMaterialUsed( SmartPtr<IMaterial> material, s32 ignoredIndex ) const;

            void applyMaterial( SmartPtr<IMaterial> material, s32 index );
            void applyAutomaticCubemap( SmartPtr<IMaterial> material, s32 index );
            void clearAutomaticCubemapBinding( AutomaticCubemapBinding &binding );
            AutomaticCubemapBinding &getAutomaticCubemapBinding( s32 index );
            Vector3F getAutomaticCubemapSamplePosition() const;

            void setMaterialName( const String &materialName, s32 index );
            String getMaterialName( s32 index ) const;

            void setMaterial( SmartPtr<IMaterial> material, s32 index );
            SmartPtr<IMaterial> getMaterial( s32 index = -1 ) const;

            /// Listener for material state changes.
            SmartPtr<IStateListener> m_materialStateListener;

            /// Animation controller for this mesh (if animated).
            SmartPtr<IAnimationController> m_animationController;

            /// Listener for material shared object events.
            SmartPtr<MeshMaterialEventListener> m_materialSharedObjectListener;

            /// Legacy v1 Entity pointer (nullable).
            Ogre::v1::Entity *m_entity = nullptr;

            /// Ogre::Item pointer used by Ogre Next; atomic wrapper for thread-safety.
            AtomicValue<Ogre::Item *> m_item = nullptr;

            AutomaticCubemapBinding m_autoCubemapBinding;
            Array<AutomaticCubemapBinding> m_autoSubCubemapBindings;

            /// Internal counters/flags used for vertex processing checks and hardware animation.
            s32 m_checkVertProcessing = 0;
            s32 m_hardwareAnimationEnabled = 0;

            GraphicsMeshManualResourceLoader m_graphicsMeshManualResourceLoader;
        };
    }  // end namespace render
}  // namespace workphone

#endif
