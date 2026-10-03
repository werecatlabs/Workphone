#ifndef WX_FB_APP_H
#define WX_FB_APP_H

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIApplication.hpp>
//#include <wx/wxprec.h>
#include <wx/app.h>

namespace workphone
{
    namespace ui
    {

        /* Base class for a wxwidgets application. */
        class WxApplication : public IUIApplication, public wxApp
        {
        public:
            class wxApplicationObject : public wxApp
            {
            public:
                wxApplicationObject();
                ~wxApplicationObject();

                /* wxApp override */
                virtual bool OnInit() override;

                /* wxApp override */
                virtual int OnExit() override;

                WxApplication *getApplication() const;
                void setApplication( WxApplication *application );

            private:
                WxApplication *m_application = nullptr;
            };

            WxApplication();
            virtual ~WxApplication();

            /* wxApp override
             */
            virtual bool OnInit();

            /* wxApp override, not called if OnInit returns false
             */
            virtual int OnExit();

            virtual SmartPtr<IUIToolbar> getToolbar() const;
            virtual void setToolbar( SmartPtr<IUIToolbar> toolbar );

        protected:
            //
            //Functions to create the components.
            //
            virtual void createLogManager();
            virtual void createFactoryManager();

            virtual void createFileSystem();
            virtual void createTimer();

            virtual void createTaskManager();
            virtual void createThreadPool();

            virtual void createTasks();

            virtual void createStateManager();

            /** */
            virtual bool createGraphicsSystem();

            /** */
            virtual void chooseSceneManager();

            /** Called to create graphics system camera. */
            virtual void createCamera();

            /** */
            virtual void createViewports();

            /** */
            virtual bool createScriptManager();

            /** */
            virtual bool createEntitySystem();

            /** */
            virtual bool createSoundManager();

            /** */
            virtual bool createCameraCtrlManager();

            /** */
            virtual void createFrameListener();

            /** */
            virtual void createCoreComponents();

            /** */
            virtual void createPhysics();

            // Optional to override this
            virtual void destroyScene();

            /// Method which will define the source of resources (other than current folder)
            virtual void setupResources();

            /// Optional override method where you can create resource listeners (e.g. for loading screens)
            virtual void createResourceListener();

            /// Optional override method where you can perform resource group loading
            /// Must at least do ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
            virtual void loadResources();
        };

    }  // end namespace ui
}  // namespace workphone

#endif
