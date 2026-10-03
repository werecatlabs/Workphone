#ifndef MeshConverter_h__
#define MeshConverter_h__

#include "MeshConverterPrerequisites.hpp"
#include <Workphone/Application.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace viewer
    {
        /**
         * @brief MeshViewer application for viewing and inspecting 3D mesh files.
         *
         * This application provides a simple interface for loading and viewing
         * mesh files in various formats (e.g., .fbx). It includes a menu bar
         * for file operations and automatic camera positioning.
         */
        class MeshViewer : public core::Application
        {
        public:
            /**
             * @brief Menu element identifiers.
             */
            enum class ElementId
            {
                Open,  ///< Open file menu item
                Exit,  ///< Exit application menu item

                Count
            };

            /**
             * @brief Default constructor.
             */
            MeshViewer();

            /**
             * @brief Destructor.
             */
            ~MeshViewer() override;

            /**
             * @brief Loads and initializes the mesh viewer application.
             * @param data Optional shared data for initialization.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads and cleans up the mesh viewer application.
             * @param data Optional shared data for cleanup.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the currently loaded mesh actor.
             * @return Smart pointer to the mesh actor, or nullptr if no mesh is loaded.
             */
            SmartPtr<scene::IGameActor> getMeshActor() const;

            /**
             * @brief Sets the mesh actor to be displayed.
             * @param meshActor Smart pointer to the mesh actor to display.
             *
             * This method will unload any previously loaded mesh and automatically
             * center the camera on the new mesh.
             */
            void setMeshActor( SmartPtr<scene::IGameActor> meshActor );

            /**
             * @brief Loads a mesh from a file path.
             * @param filePath Path to the mesh file to load.
             */
            void loadMeshFromFile( const String &filePath );

            WP_CLASS_REGISTER_DECL;

        private:
            /**
             * @brief Event listener for menu bar interactions.
             *
             * Handles user interactions with the menu bar, including
             * file open operations and application exit requests.
             */
            class CUIMenuBarListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                CUIMenuBarListener();

                /**
                 * @brief Destructor.
                 */
                ~CUIMenuBarListener() override;

                /**
                 * @brief Handles menu bar events.
                 * @param eventType Type of the event.
                 * @param eventValue Hash value of the event.
                 * @param arguments Event arguments.
                 * @param sender Object that sent the event.
                 * @param object Object associated with the event.
                 * @param event The event object.
                 * @return Event result parameter.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner MeshViewer instance.
                 * @return Smart pointer to the owner, or nullptr if owner is no longer valid.
                 */
                SmartPtr<MeshViewer> getOwner() const;

                /**
                 * @brief Sets the owner MeshViewer instance.
                 * @param owner Smart pointer to the MeshViewer owner.
                 */
                void setOwner( SmartPtr<MeshViewer> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                /**
                 * @brief Handles the "Open File" menu action.
                 * @param owner Pointer to the MeshViewer owner.
                 * @param fileSystem Pointer to the file system interface.
                 */
                void handleOpenFile( SmartPtr<MeshViewer> owner, SmartPtr<IFileSystem> fileSystem );

                /**
                 * @brief Handles the "Exit" menu action.
                 */
                void handleExit();

                WeakPtr<MeshViewer> m_owner;  ///< Weak pointer to the owner MeshViewer
            };

            /**
             * @brief Creates and initializes application plugins.
             */
            void createPlugins() override;

            /**
             * @brief Creates the user interface system.
             */
            void createUI() override;

            /**
             * @brief Creates the render window for displaying the mesh.
             */
            void createRenderWindow() override;

            /**
             * @brief Sets up the user interface components including menu bar.
             */
            void setupUI();

            /**
             * @brief Configures camera settings for mesh viewing.
             */
            void setupCamera();

            /**
             * @brief Configures viewport settings.
             */
            void setupViewport();

            /**
             * @brief Centers the camera to view the loaded mesh.
             *
             * Automatically adjusts the camera position and orientation
             * to frame the currently loaded mesh in the viewport.
             */
            void centerCameraOnMesh();

            SmartPtr<ui::IUIApplication> m_application;          ///< UI application instance
            SmartPtr<ui::IUIRenderWindow> m_renderWindow;        ///< Render window for displaying meshes
            SmartPtr<IEventListener> m_menubarListener;          ///< Menu bar event listener
            SmartPtr<render::IGraphicsWindow> m_window;                  ///< Graphics window
            SmartPtr<render::IGraphicsScene> m_sceneManager;     ///< Scene manager
            SmartPtr<render::IGraphicsCamera> m_mainCamera;              ///< Main camera
            SmartPtr<render::IGraphicsSceneNode> m_mainCameraSceneNode;  ///< Main camera scene node
            SmartPtr<render::IGraphicsSceneNode> m_cameraSceneNode;      ///< Camera scene node
            SmartPtr<render::IViewport> m_mainViewport;          ///< Main viewport
            SmartPtr<render::ITexture> m_renderTarget;           ///< Render target texture
            SmartPtr<scene::IGameActor> m_cameraActor;           ///< Camera actor
            SmartPtr<scene::IGameActor> m_meshActor;             ///< Currently loaded mesh actor
        };
    }  // end namespace viewer
}  // end namespace workphone

#endif  // MeshViewer_h__