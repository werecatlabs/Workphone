#ifndef __Application_h__
#define __Application_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Application.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/System/FSMListener.hpp>

namespace workphone
{
    namespace editor
    {

        /**
         * @brief Represents the main application for the editor.
         * The EditorApplication class is derived from the core::Application class and
         * serves as the entry point and manager for the editor application.
         */
        class EditorApplication : public core::Application
        {
        public:
            /**
             * @brief Enumerates the possible states of the editor application.
             */
            enum class State
            {
                Loading,
                /**< The application is in the loading state. */
                Editor,
                /**< The application is in the editor state. */
                Shutdown,
                /**< The application is in the shutdown state. */
                Count
                /**< The total number of states. */
            };

            static const String scriptsFolderStr;

            /**
             * @brief Default constructor for the EditorApplication class.
             */
            EditorApplication();

            /**
             * @brief Destructor for the EditorApplication class.
             */
            ~EditorApplication() override;

            /**
             * @brief Loads the editor application.
             * @param data The data to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the editor application.
             * @param data The data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Runs the editor application.
             */
            void run() override;

            /**
             * @brief Iterates the editor application.
             */
            void iterate() override;

            /**
             * @brief Updates the editor application.
             */
            void update() override;

            /**
             * @brief Handles the input event.
             */
            bool inputEvent( SmartPtr<IInputEvent> event );

            /**
             * @brief Handles the fsm event.
             */
            FSMReturnType handleEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Gets the editor manager for the application.
             * @return The editor manager for the application.
             */
            size_t getWindowHandle() const;

            /**
             * @brief Sets the window handle for the application.
             * @param windowHandle The window handle to set.
             */
            void setWindowHandle( size_t windowHandle );

            /**
             * @brief Enters play mode in the editor application.
             */
            void enterPlayMode();

            /**
             * @brief Stops play mode in the editor application.
             */
            void stopPlayMode();

            /**
             * @brief Gets the ui camera for the application.
             */
            //SmartPtr<render::IGraphicsCamera> getUiCamera() const;

            /**
             * @brief Sets the ui camera for the application.
             */
            //void setUiCamera( SmartPtr<render::IGraphicsCamera> camera );

            //SmartPtr<render::IGraphicsSceneNode> getUiCameraSceneNode() const;
            //void setUiCameraSceneNode( SmartPtr<render::IGraphicsSceneNode> sceneNode );

            SmartPtr<render::IGraphicsCamera> getCamera() const;
            void setCamera( SmartPtr<render::IGraphicsCamera> camera );

            SmartPtr<render::IGraphicsSceneNode> getCameraSceneNode() const;
            void setCameraSceneNode( SmartPtr<render::IGraphicsSceneNode> sceneNode );

            SmartPtr<IStateContext> getStateContext() const;
            void setStateContext( SmartPtr<IStateContext> stateContext );

            SmartPtr<IStateListener> getStateListener() const;
            void setStateListener( SmartPtr<IStateListener> stateListener );

            /**
             * @brief Sets the renderer type for the application.
             * @param type The renderer type to use.
             */
            void setRendererType( render::IGraphicsSystem::RenderApi type );
            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Gets the rtt viewport.
             * @return The rtt viewport.
             */
            //SmartPtr<render::IViewport> getRttViewport() const;

            /**
             * @brief Sets the rtt viewport.
             * @param rttViewport The editor manager to set.
             */
            //void setRttViewport( SmartPtr<render::IViewport> rttViewport );

            WP_CLASS_REGISTER_DECL;

        private:
            void loadEditorPreferences();
            class ApplicationListener : public IEventListener
            {
            public:
                ApplicationListener();
                ApplicationListener( EditorApplication *app );
                ~ApplicationListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                bool inputEvent( SmartPtr<IInputEvent> event );
                bool updateEvent( const SmartPtr<IInputEvent> &event );

                void setPriority( s32 priority ) override;
                s32 getPriority() const override;

                WP_CLASS_REGISTER_DECL;

            protected:
                RawPtr<EditorApplication> m_application;
            };

            class ApplicationStateListener : public IStateListener
            {
            public:
                ApplicationStateListener();
                ~ApplicationStateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                EditorApplication *getOwner() const;
                void setOwner( EditorApplication *owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                EditorApplication *m_owner = nullptr;
            };

            class ApplicationFSMListener : public FSMListener
            {
            public:
                ApplicationFSMListener();
                ~ApplicationFSMListener() override;

                FSMReturnType handleEvent( u32 state, FSMEvent eventType ) override;

                EditorApplication *getOwner() const;

                void setOwner( EditorApplication *owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                EditorApplication *m_owner = nullptr;
            };

            class EventListener : public IEventListener
            {
            public:
                EventListener();
                ~EventListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                WP_CLASS_REGISTER_DECL;
            };

            void loadDebug( SmartPtr<ISharedObject> data );
            void loadEditor( SmartPtr<ISharedObject> data );

        private:
            void loadSettings();

            void createLogManager() override;
            void createFactoryManager() override;

            void createTimer() override;

            void createFsmManager() override;
            void createFsm() override;

            void createTaskManager() override;

            void createThreadPool() override;

            void createStateManager() override;

            void createSceneManager() override;
            void createFileSystem() override;

            void createUI() override;
            bool createGraphicsSystem() override;

            void createRenderWindow() override;
            void createCamera() override;
            void createViewports() override;
            void createRenderInitJob();

            void loadGraphicsResources() override;
            void createGraphicsScene() override;
            void createTasks() override;
            void createPhysics() override;

            void createInputSystem() override;
            void createPlugins() override;

            void createDebugText();

            size_t m_windowHandle = 0;

            SmartPtr<render::IViewport> m_mainViewport;  ///< Main viewport

            SmartPtr<render::ITexture> m_renderTarget;

            SmartPtr<scene::CameraController> m_fpsCamera;
            SmartPtr<scene::SphericalCameraController> m_sphericalCamera;

            SmartPtr<scene::IGameActor> m_cameraActor;

            RawPtr<ApplicationFrame> m_frame = nullptr;
            SmartPtr<EditorManager> m_editorManager;

            SmartPtr<IStateContext> m_stateContext;
            SmartPtr<IStateListener> m_stateListener;

            // Application object listeners are registered as non-owning pointers.
            SmartPtr<IEventListener> m_editorApplicationListener;

            SmartPtr<ui::IUIApplication> m_application;
            SmartPtr<ui::IUIRenderWindow> m_renderWindow;

            // Renderer type selection
            render::IGraphicsSystem::RenderApi m_rendererType = render::IGraphicsSystem::RenderApi::DX11;
            bool m_rendererTypeExplicit = false;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // App_h__
