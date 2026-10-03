#ifndef __Application_h__
#define __Application_h__



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplication.hpp>
#include <FBCore/System/TaskListenerAdapter.hpp>
#include <FBCore/Interface/Input/IInputListener.hpp>
#include <FBCore/Interface/System/IStateListener.hpp>
#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{	
	namespace editor
	{
	

	
		//--------------------------------------------
		class Application : public ui::WxApplication
		{
		public:
			Application();
			~Application();

			bool OnInit();
			int MainLoop();
			void ExitMainLoop();

			void createLogManager();
			void createFactoryManager();
			void createSceneViews();

			void update();

			void run() {}

			bool inputEvent(SmartPtr<IInputEvent> event);
	

			SmartPtr<ui::IUIMenubar> getMenubar() const override;


			void setMenubar(SmartPtr<ui::IUIMenubar> val) override;


			Vector2I getWindowSize() const override;


			void setWindowSize(const Vector2I& size) override;


			int FilterEvent(wxEvent& event) override;

		private:
			class ApplicationInputListener : public CSharedObject<IInputListener>
			{
			public:
				ApplicationInputListener(Application* app);
				~ApplicationInputListener();

				bool inputEvent(SmartPtr<IInputEvent> event);
				bool updateEvent(const SmartPtr<IInputEvent>& event);

				void setPriority(s32 priority);
				s32 getPriority() const;

			protected:
				RawPtr<Application> m_application;
			};

			class ApplicationStateListener : public CSharedObject<IStateListener>
			{
			public:
				ApplicationStateListener() = default;
				~ApplicationStateListener();

				void handleStateChanged(const SmartPtr<IStateMessage>& message) override;
				void handleStateChanged(SmartPtr<IState>& state) override;
				void handleQuery(SmartPtr<IStateQuery>& query)  override;

				Application* getOwner() const;
				void setOwner(Application* val);

			protected:
				Application* m_owner = nullptr;
			};

			void createTimer();

			void createTaskManager();

			void createThreadPool();

			void createStateManager();

			void createFileSystem();

			bool createGraphicsSystem();

			void createRenderInitJob();

			void chooseSceneManager();
			void createTasks();
			void createCoreComponents();
			void createPhysics();
			
			bool createScriptManager();

			SmartPtr<component::CameraController> m_fpsCamera;
			SmartPtr<component::SphericalCameraController> m_sphericalCamera;

			SmartPtr<IActor> m_sphericalCameraActor;

			Array<SmartPtr<IWorkerThread>>	m_workerThreads;	

			SmartPtr<ITask> m_gfxTask;
			SmartPtr<ITask> m_appLogicTask;
			SmartPtr<ITask> m_physicsTask;

			TaskListenerAdapter<Application>* m_taskApplication = nullptr;
			TaskListenerAdapter<Application>* m_taskGraphics = nullptr;
			TaskListenerAdapter<Application>* m_taskLogic = nullptr;
			TaskListenerAdapter<Application>* m_taskPhysics = nullptr;

			RawPtr<ApplicationFrame> m_frame = nullptr;
			SmartPtr<EditorManager> m_editorManager;

			atomic_bool m_isRunning = true;

			SmartPtr<IStateObject> m_stateObject;
			SmartPtr<IStateListener> m_stateListener;

			SmartPtr<IFrameStatistics> m_frameStatistics;
		};

	
		
	} // end namespace editor
} // end namespace fb



#endif // App_h__


