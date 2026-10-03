#ifndef MeshViewer_h__
#define MeshViewer_h__



#include "MeshViewerPrerequisites.hpp"
#include <FBApplication/CApplication.hpp>
#include <FBApplication/UI/CUIElementListener.hpp>



namespace fb
{	
	namespace viewer
	{



		//--------------------------------------------
		class MeshViewer : public application::CApplication
		{
		public:
			enum class ElementId
			{
				Open,
				Exit,

				Count
			};

			MeshViewer();
			~MeshViewer();

			void load(SmartPtr<ISharedObject> data);
			void unload(SmartPtr<ISharedObject> data);

			void run();

			SmartPtr<IActor> getMeshActor() const;
			void setMeshActor(SmartPtr<IActor> val);

		private:
			class CUIMenuBarListener : public ui::CUIElementListener
			{
			public:
				CUIMenuBarListener();
				~CUIMenuBarListener();

				void handleEvent(s32 id, SmartPtr<ISharedObject> data);

				void handleNoMenuItemsEnabled(SmartPtr<ui::IUIElement> item);
				void handleNoMenuItemsVisible(SmartPtr<ui::IUIElement> item);

				MeshViewer* getOwner() const;
				void setOwner(MeshViewer* val);

			private:
				MeshViewer* m_owner = nullptr;
			};

			class CUIFileBrowserListener : public ui::CUIElementListener
			{
			public:
				CUIFileBrowserListener() = default;
				~CUIFileBrowserListener() = default;

				/** @copydoc IUIFileBrowserListener::handleSelection */
				void handleSelection();

				/** @copydoc IUIFileBrowserListener::handleCancel */
				void handleCancel();

				/** @copydoc IUIFileBrowserListener::handleClose */
				void handleClose();

				/** @copydoc IUIFileBrowserListener::getFileBrowser */
				SmartPtr<ui::IUIFileBrowser> getFileBrowser() const;

				/** @copydoc IUIFileBrowserListener::setFileBrowser */
				void setFileBrowser(SmartPtr<ui::IUIFileBrowser> fileBrowser);

				MeshViewer* getOwner() const;
				void setOwner(MeshViewer* val);

			private:
				SmartPtr<ui::IUIFileBrowser> m_fileBrowser;
				MeshViewer* m_owner = nullptr;
			};

			void createLogManager();
			void createFactoryManager();
			void createTimer();
			void createTaskManager();
			void createThreadPool();
			void createStateManager();
			void createFileSystem();

			void createUI();

			bool createGraphicsSystem();
			void createRenderWindow();

			void createGraphicsSceneManager();
			void createCamera();
			void createViewports();
			void loadGraphicsResources();
			void setupRenderpipeline();			

			SmartPtr<ui::IUIApplication> m_application;
			SmartPtr<ui::IUIRenderWindow> m_renderWindow;

			SmartPtr<ui::IUIElementListener> m_menubarListener;

			SmartPtr<ui::IUIElementListener> m_fileBrowserListener;

			SmartPtr<render::IWindow> m_window;
			SmartPtr<render::ISceneManager>		m_sceneManager;
			
			SmartPtr<render::ICamera>			m_mainCamera;
			SmartPtr<render::ISceneNode>		m_mainCameraSceneNode;

			SmartPtr<render::ICamera>			m_camera;
			SmartPtr<render::ISceneNode>		m_cameraSceneNode;
			
			SmartPtr<render::IViewport>			m_viewport;
			SmartPtr<render::IViewport>			m_mainViewport;

			SmartPtr<render::ITexture>			m_renderTarget;

			SmartPtr<IActor> m_cameraActor;

			SmartPtr<IActor> m_meshActor;

			SmartPtr<IFrameStatistics> m_frameStatistics;
		};



	} // end namespace viewer
} // end namespace fb



#endif // MeshViewer_h__


