#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include <WPWxWidgets/WPWxApplication.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace ui
    {

        WxApplication::WxApplication() = default;

        //--------------------------------------------
        WxApplication::~WxApplication()
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( applicationManager )
            {
                auto guiManager = applicationManager->getUI();
                if( guiManager )
                {
                    guiManager->clear();
                }

                applicationManager->setConsole( nullptr );
                applicationManager->setUI( nullptr );
                applicationManager->setCameraManager( nullptr );
                applicationManager->setGraphicsSystem( nullptr );
                //applicationManager->setEventManager(EventManagerPtr::NULL_PTR);
                applicationManager->setFileSystem( nullptr );
                applicationManager->setThreadPool( nullptr );
                applicationManager->setTaskManager( nullptr );
                applicationManager->setTimer( nullptr );
            }
        }

        //--------------------------------------------
        bool WxApplication::OnInit()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            //if (!wxApp::OnInit())
            //{
            //	return false;
            //}

            createLogManager();
            createFactoryManager();
            createFileSystem();

            WP_LOG( "Initializing base components." );

            createTimer();
            WP_LOG( "Timer created." );

            createThreadPool();

            WP_LOG( "EventManager created." );
            createTaskManager();

            createTasks();

            createStateManager();

            //SmartPtr<IFileSystem>& fileSystem = m_fbSystem->setFileSystem(SmartPtr<IFileSystem>(new FileSystem, true));
            //LOG_MESSAGE("WxApplication", "FileSystem created.");

            WP_LOG( "GameInputManager created." );

            createSoundManager();
            WP_LOG( "GameInputManager created." );

            createScriptManager();
            WP_LOG( "GameInputManager created." );

            createGraphicsSystem();
            WP_LOG( "GameInputManager created." );

            setupResources();
            WP_LOG( "setupResources." );

            chooseSceneManager();
            WP_LOG( "SceneManager(s) created." );

            createCamera();
            WP_LOG( "Cameras created." );

            createViewports();
            WP_LOG( "Viewports created." );

            createResourceListener();
            WP_LOG( "ResourceListener created." );

            loadResources();
            WP_LOG( "loadResources." );

            createFrameListener();
            WP_LOG( "FrameListener created." );

            createCameraCtrlManager();
            WP_LOG( "CameraCtrlManager created." );

            createEntitySystem();
            WP_LOG( "EntitySystem created." );

            createPhysics();
            WP_LOG( "Create physics." );

            WP_LOG( "Finished creating base components." );

            return true;
        }

        //--------------------------------------------
        int WxApplication::OnExit()
        {
            return 0;
        }

        //--------------------------------------------
        SmartPtr<IUIToolbar> WxApplication::getToolbar() const
        {
            return nullptr;
        }

        //--------------------------------------------
        void WxApplication::setToolbar( SmartPtr<IUIToolbar> toolbar )
        {
        }

        //--------------------------------------------
        void WxApplication::createLogManager()
        {
        }
        //--------------------------------------------
        void WxApplication::createFactoryManager()
        {
        }
        //--------------------------------------------
        void WxApplication::createFileSystem()
        {
        }

        void WxApplication::createTimer()
        {
        }

        void WxApplication::createTaskManager()
        {
        }

        void WxApplication::createThreadPool()
        {
        }

        void WxApplication::createTasks()
        {
        }

        void WxApplication::createStateManager()
        {
        }

        //--------------------------------------------
        bool WxApplication::createGraphicsSystem()
        {
            return true;
        }

        //--------------------------------------------
        void WxApplication::chooseSceneManager()
        {
        }

        //--------------------------------------------
        void WxApplication::createCamera()
        {
        }

        bool WxApplication::createEntitySystem()
        {
            return true;
        }

        bool WxApplication::createSoundManager()
        {
            return true;
        }

        bool WxApplication::createCameraCtrlManager()
        {
            return true;
        }

        //--------------------------------------------
        void WxApplication::createFrameListener()
        {
        }

        //--------------------------------------------
        void WxApplication::createViewports()
        {
        }

        //--------------------------------------------
        bool WxApplication::createScriptManager()
        {
            return true;
        }

        //--------------------------------------------
        void WxApplication::setupResources()
        {
        }

        //--------------------------------------------
        void WxApplication::createResourceListener()
        {
        }

        //--------------------------------------------
        void WxApplication::loadResources()
        {
        }

        //--------------------------------------------
        void WxApplication::createCoreComponents()
        {
            //auto applicationManager = core::ApplicationManager::instance();
            //auto platformMgr = applicationManager->getPlatformManager();

            //applicationManager->setTimer(platformMgr->createTimer());

            ////m_fbSystem->setProfiler(ProfilerPtr(new Profiler, true));
            //LOG_MESSAGE("GameApplication", "Profiler created.");

            //applicationManager->setThreadPool(platformMgr->createThreadPool());
            //LOG_MESSAGE("GameApplication", "ThreadPool created.");

            //applicationManager->setTaskManager(platformMgr->createTaskManager());
            //LOG_MESSAGE("GameApplication", "TaskManager created.");
        }

        void WxApplication::createPhysics()
        {
        }

        void WxApplication::destroyScene()
        {
        }

        WxApplication::wxApplicationObject::wxApplicationObject() = default;

        WxApplication::wxApplicationObject::~wxApplicationObject() = default;

        /* wxApp override */
        bool WxApplication::wxApplicationObject::OnInit()
        {
            return m_application->OnInit();
        }

        /* wxApp override */
        int WxApplication::wxApplicationObject::OnExit()
        {
            return m_application->OnExit();
        }

        WxApplication *WxApplication::wxApplicationObject::getApplication() const
        {
            return m_application;
        }

        void WxApplication::wxApplicationObject::setApplication( WxApplication *application )
        {
            m_application = application;
        }

    }  // end namespace ui
}  // namespace workphone
