#ifndef _CCompositorManager_H_
#define _CCompositorManager_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class CompositorManager
         * @brief Manages compositor objects and their integration with the rendering pipeline.
         *
         * The CompositorManager is responsible for creating, registering, enabling, disabling,
         * and managing compositors within a graphics scene. It also provides methods to set up
         * the renderer and manage properties related to compositors and their associated viewports.
         */
        class WPGraphicsOgreNext_API CompositorManager : public SharedGraphicsObject<ISharedObject>
        {
        public:
            class StateListener : public IStateListener
            {
            public:
                StateListener();
                ~StateListener() override;

                void unload( SmartPtr<ISharedObject> data ) override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                bool handleStateChanged( SmartPtr<IState> &state ) override;

                CompositorManager *getOwnerPtr() const;

                SmartPtr<CompositorManager> getOwner() const;

                void setOwner( SmartPtr<CompositorManager> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /** Weak pointer to the owning material to avoid circular references */
                AtomicWeakPtr<CompositorManager> m_owner;
            };

            /**
             * @brief Default constructor.
             */
            CompositorManager();

            /**
             * @brief Deleted copy constructor to prevent copying.
             */
            CompositorManager( const CompositorManager &other ) = delete;

            /**
             * @brief Destructor.
             */
            ~CompositorManager() override;

            /**
             * @brief Adds a new compositor to the manager.
             * @return A smart pointer to the newly created Compositor.
             */
            SmartPtr<Compositor> addCompositor();

            /**
             * @brief Removes a compositor from the manager.
             * @param compositor The compositor to remove.
             */
            void removeCompositor( SmartPtr<Compositor> compositor );

            /**
             * @brief Sets up the renderer with the specified scene manager, window, camera, and workspace.
             * @param pISceneManager The scene manager to use.
             * @param pIGraphicsWindow The window to render to.
             * @param pIGraphicsCamera The camera to use for rendering.
             * @param workspaceName The name of the compositor workspace.
             * @param enabled Whether renderer setup is enabled.
             */
            void setupRenderer( SmartPtr<IGraphicsScene> pISceneManager,
                                SmartPtr<IGraphicsWindow> pIGraphicsWindow,
                                SmartPtr<IGraphicsCamera> pIGraphicsCamera, String workspaceName,
                                bool enabled );

            /**
             * @copydoc ISharedObject::load
             * @brief Loads the compositor manager state from the given data object.
             * @param data The data object containing state to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::reload
             * @brief Reloads the compositor manager state from the given data object.
             * @param data The data object containing state to reload.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             * @brief Unloads the compositor manager state using the given data object.
             * @param data The data object for unloading state.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::getProperties
             * @brief Gets the properties of the compositor manager.
             * @return A smart pointer to the Properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc ISharedObject::setProperties
             * @brief Sets the properties of the compositor manager.
             * @param properties The properties to set.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc ISharedObject::getChildObjects
             * @brief Gets the child objects managed by the compositor manager.
             * @return An array of smart pointers to child shared objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the current scene manager.
             * @return A smart pointer to the current IGraphicsScene.
             */
            SmartPtr<IGraphicsScene> getSceneManager() const;

            /**
             * @brief Sets the scene manager.
             * @param sceneManager The scene manager to set.
             */
            void setSceneManager( SmartPtr<IGraphicsScene> sceneManager );

            /**
             * @brief Gets the current window.
             * @return A smart pointer to the current IGraphicsWindow.
             */
            SmartPtr<IGraphicsWindow> getWindow() const;

            /**
             * @brief Sets the window.
             * @param window The window to set.
             */
            void setWindow( SmartPtr<IGraphicsWindow> window );

            /**
             * @brief Gets the current camera.
             * @return A smart pointer to the current IGraphicsCamera.
             */
            SmartPtr<IGraphicsCamera> getCamera() const;

            /**
             * @brief Sets the camera.
             * @param camera The camera to set.
             */
            void setCamera( SmartPtr<IGraphicsCamera> camera );

            WP_CLASS_REGISTER_DECL;

        protected:
            void setupStateContext();
            void createMainShadowNode();
            /**
             * @brief Array of managed compositor objects.
             */
            ConcurrentArray<SmartPtr<Compositor>> m_compositors;

            /**
             * @brief The current scene manager.
             */
            SmartPtr<IGraphicsScene> m_sceneManager;

            /**
             * @brief The current window.
             */
            SmartPtr<IGraphicsWindow> m_window;

            /**
             * @brief The current camera.
             */
            SmartPtr<IGraphicsCamera> m_camera;

            /**
             * @brief Pointer to the compositor pass provider.
             */
            CompositorPassProvider *m_passProvider = nullptr;
        };

    }  // end namespace render
}  // namespace workphone

#endif
