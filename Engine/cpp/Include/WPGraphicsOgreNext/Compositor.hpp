#ifndef _CCompositor_H_
#define _CCompositor_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class Compositor
         * @brief Manages the composition of graphical elements in a scene using OgreNext's compositor system.
         *
         * The Compositor class is responsible for setting up and managing compositor workspaces, handling events and state changes,
         * and managing associated graphics objects such as scenes, windows, cameras, and target textures.
         *
         * @note This class inherits from SharedGraphicsObject<ISharedObject>.
         */
        class WPGraphicsOgreNext_API Compositor : public SharedGraphicsObject<ISharedObject>
        {
        public:
            /**
             * @class EventListener
             * @brief Listens for and handles events related to the Compositor.
             *
             * The EventListener class provides a mechanism to respond to various events that may affect the compositor.
             */
            class EventListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                EventListener();
                /**
                 * @brief Destructor.
                 */
                ~EventListener() override;

                /**
                 * @param data The data to use for loading.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handles an event dispatched to the listener.
                 * @param eventType The type of the event.
                 * @param eventValue The value associated with the event.
                 * @param arguments Additional arguments for the event.
                 * @param sender The sender of the event.
                 * @param object The object associated with the event.
                 * @param event The event object itself.
                 * @return A Parameter representing the result of the event handling.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                Compositor *getOwnerPtr() const;

                /**
                 * @brief Gets the owner Compositor of this listener.
                 * @return A smart pointer to the owner Compositor.
                 */
                SmartPtr<Compositor> getOwner() const;

                /**
                 * @brief Sets the owner Compositor of this listener.
                 * @param owner A smart pointer to the owner Compositor.
                 */
                void setOwner( SmartPtr<Compositor> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /// Weak pointer to the owner Compositor.
                AtomicWeakPtr<Compositor> m_owner;
            };

            /**
             * @brief Default constructor.
             */
            Compositor();

            /**
             * @brief Constructor.
             */
            Compositor( CompositorManager *mgr );

            /**
             * @brief Destructor.
             */
            ~Compositor() override;

            /**
             * @copydoc ISharedObject::load
             * @brief Loads the compositor with the provided data.
             * @param data The data to use for loading.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::reload
             * @brief Reloads the compositor with the provided data.
             * @param data The data to use for reloading.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             * @brief Unloads the compositor with the provided data.
             * @param data The data to use for unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Stops the compositor and releases associated resources.
             */
            void stopCompositor();

            /**
             * @brief Sets up the enabled state of the compositor.
             * @param enabled Whether the compositor should be enabled or not.
             * @return True if the setup was successful, false otherwise.
             */
            bool setupWorkspace( bool enabled );

            /**
             * @brief Checks if the compositor is enabled.
             * @return True if the compositor is enabled, false otherwise.
             */
            bool isEnabled() const;

            /**
             * @brief Sets the enabled state of the compositor.
             * @param enabled Whether the compositor should be enabled or not.
             * @param updateSetup Whether to update the setup after changing the enabled state.
             */
            void setEnabled( bool enabled, bool updateSetup = false );

            /**
             * @brief Gets the Ogre compositor workspace managed by this compositor.
             * @return Pointer to the Ogre::CompositorWorkspace.
             */
            Ogre::CompositorWorkspace *getCompositorWorkspace() const;

            /**
             * @brief Sets the Ogre compositor workspace managed by this compositor.
             * @param compositorWorkspace Pointer to the new Ogre::CompositorWorkspace.
             */
            void setCompositorWorkspace( Ogre::CompositorWorkspace *compositorWorkspace );

            /**
             * @brief Gets the scene manager associated with this compositor.
             * @return Smart pointer to the IGraphicsScene.
             */
            SmartPtr<IGraphicsScene> getSceneManager() const;

            /**
             * @brief Sets the scene manager associated with this compositor.
             * @param sceneManager Smart pointer to the new IGraphicsScene.
             */
            void setSceneManager( SmartPtr<IGraphicsScene> sceneManager );

            /**
             * @brief Gets the window associated with this compositor.
             * @return Pointer to the IGraphicsWindow.
             */
            IGraphicsWindow *getWindowPtr() const;

            /**
             * @brief Gets the window associated with this compositor.
             * @return Smart pointer to the IGraphicsWindow.
             */
            SmartPtr<IGraphicsWindow> getWindow() const;

            /**
             * @brief Sets the window associated with this compositor.
             * @param window Smart pointer to the new IGraphicsWindow.
             */
            void setWindow( SmartPtr<IGraphicsWindow> window );

            /**
             * @brief Gets the camera associated with this compositor.
             * @return Pointer to the IGraphicsCamera.
             */
            IGraphicsCamera *getCameraPtr() const;

            /**
             * @brief Gets the camera associated with this compositor.
             * @return Smart pointer to the IGraphicsCamera.
             */
            SmartPtr<IGraphicsCamera> getCamera() const;

            /**
             * @brief Sets the camera associated with this compositor.
             * @param camera Smart pointer to the new IGraphicsCamera.
             */
            void setCamera( SmartPtr<IGraphicsCamera> camera );

            /**
             * @brief Gets the name of the compositor workspace.
             * @return The workspace name as a String.
             */
            String getWorkspaceName() const;

            /**
             * @brief Sets the name of the compositor workspace.
             * @param workspaceName The new workspace name.
             */
            void setWorkspaceName( const String &workspaceName );

            /**
             * @brief Sets up a test compositor workspace for debugging or development purposes.
             * @return Pointer to the test Ogre::CompositorWorkspace.
             */
            Ogre::CompositorWorkspace *setupTestCompositor();

            /**
             * @copydoc CGraphicsObjectOgreNext<IGraphicsCamera>::getProperties
             * @brief Gets the properties of the compositor.
             * @return Smart pointer to the Properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc CGraphicsObjectOgreNext<IGraphicsCamera>::setProperties
             * @brief Sets the properties of the compositor.
             * @param properties Smart pointer to the Properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc CGraphicsObjectOgreNext<IGraphicsCamera>::getChildObjects
             * @brief Gets the child objects of the compositor.
             * @return Array of smart pointers to child ISharedObject instances.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the Ogre texture GPU associated with this compositor.
             * @return Pointer to the Ogre::TextureGpu.
             */
            Ogre::TextureGpu *getTextureGpu() const;

            /**
             * @brief Sets the Ogre texture GPU associated with this compositor.
             * @param textureGpu Pointer to the new Ogre::TextureGpu.
             */
            void setTextureGpu( Ogre::TextureGpu *textureGpu );

            /**
             * @brief Gets the target texture associated with this compositor.
             * @return Smart pointer to the ITexture.
             */
            SmartPtr<ITexture> getTargetTexture() const;
            /**
             * @brief Sets the target texture associated with this compositor.
             * @param texture Smart pointer to the new ITexture.
             */
            void setTargetTexture( SmartPtr<ITexture> texture );

            /**
             * @brief Marks the compositor as dirty, indicating it needs to be updated.
             */
            void makeDirty();

            /**
             * @brief Handles a state message.
             * @param message The state message to handle.
             * @return True if the message was handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handles a state change.
             * @param state The new state.
             * @return True if the state change was handled, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            /**
             * @brief Gets the owner CompositorManager of this compositor.
             * @return Pointer to the owner CompositorManager.
             */
            CompositorManager *getOwner() const;

            /**
             * @brief Sets the owner CompositorManager of this compositor.
             * @param owner Pointer to the owner CompositorManager.
             */
            void setOwner( CompositorManager *owner );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Handles an event for the compositor.
             * @param eventType The type of the event.
             * @param eventValue The value associated with the event.
             * @param arguments Additional arguments for the event.
             * @param sender The sender of the event.
             * @param object The object associated with the event.
             * @param event The event object itself.
             * @return A Parameter representing the result of the event handling.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event );

            /**
             * @brief Sets up the compositor workspace definition.
             * @return Pointer to the Ogre::CompositorWorkspaceDef.
             */
            Ogre::CompositorWorkspaceDef *setupCompositor();

            void destroyCompositeMaterial();

            void setupStateContext();

            /** Registers the compositor listener with its current camera dependencies. */
            void refreshEventSources();

            /** Removes the compositor listener from all previously observed dependencies. */
            void clearEventSources();

            /** Returns true when an event sender contributes to this compositor workspace. */
            bool isRelatedEventSource( SmartPtr<ISharedObject> source ) const;

            CompositorManager *m_owner = nullptr;

            /// Weak pointer to the target texture.
            AtomicWeakPtr<ITexture> m_targetTexture;

            /// Pointer to the Ogre texture GPU.
            Ogre::TextureGpu *m_textureGpu = nullptr;

            /**
             * @var Ogre::CompositorWorkspace *m_compositorWorkspace
             * @brief Pointer to the Ogre compositor workspace managed by this compositor.
             */
            Ogre::CompositorWorkspace *m_compositorWorkspace = nullptr;

            /// Pointer to the Ogre compositor workspace definition.
            Ogre::CompositorWorkspaceDef *m_compositorWorkspaceDef = nullptr;
            bool m_ownsWorkspaceDefinition = false;

            /**
             * @var Ogre::TerraWorkspaceListener *mTerraWorkspaceListener
             * @brief Pointer to the Terra workspace listener associated with this compositor.
             */
            Ogre::TerraWorkspaceListener *mTerraWorkspaceListener = nullptr;

            /// Pointer to the scene pass definition.
            Ogre::CompositorPassSceneDef *m_scenePassDef = nullptr;

            /// Pointer to the scene UI pass definition.
            Ogre::CompositorPassDef *m_sceneUiPassDef = nullptr;

            /// Pointer to the application UI pass definition.
            Ogre::CompositorPassDef *m_applicationUiPassDef = nullptr;

            Ogre::CompositorTargetDef *m_targetDef = nullptr;

            /// Per-camera clone of the built-in composite material.
            String m_compositeMaterialName;

            /// Smart pointer to the event listener.
            SmartPtr<EventListener> m_eventListener;

            /// Weak references to dependency objects currently publishing events to this compositor.
            Array<WeakPtr<ISharedObject>> m_eventSources;

            /**
             * @var SmartPtr<IGraphicsSceneManager> m_sceneManager
             * @brief Weak pointer to the scene manager associated with this compositor.
             */
            AtomicWeakPtr<IGraphicsScene> m_sceneManager;

            /**
             * @var SmartPtr<IGraphicsWindow> m_window
             * @brief Weak pointer to the window associated with this compositor.
             */
            AtomicWeakPtr<IGraphicsWindow> m_window;

            /**
             * @var SmartPtr<IGraphicsCamera> m_camera
             * @brief Weak pointer to the camera associated with this compositor.
             */
            AtomicWeakPtr<IGraphicsCamera> m_camera;

            /**
             * @var String m_workspaceName
             * @brief The name of the compositor workspace managed by this compositor.
             */
            String m_workspaceName;

            /// Static extension for  uniqueness.
            static u32 m_idExt;
        };

        WPForceInline IGraphicsCamera *Compositor::getCameraPtr() const
        {
            return m_camera.get();
        }

        WPForceInline IGraphicsWindow *Compositor::getWindowPtr() const
        {
            return m_window.get();
        }

        WPForceInline Compositor *Compositor::EventListener::getOwnerPtr() const
        {
            return m_owner.get();
        }

    }  // end namespace render
}  // namespace workphone

#endif
