#ifndef _MAINAPP_H
#define _MAINAPP_H

#include <Workphone/WorkphoneHeaders.hpp>
#include <Workphone/Application.hpp>

namespace workphone
{

    class Application : public core::Application
    {
    public:
        enum class ElementId
        {
            Open,
            Exit,

            Count
        };

        Application();
        ~Application() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

    protected:
        void createPlugins() override;

        void createScene() override;

        void createUI() override;

        void createRenderWindow();

        SmartPtr<ui::IUIApplication> m_application;
        SmartPtr<ui::IUIRenderWindow> m_renderWindow;

        SmartPtr<render::IGraphicsObject> m_box;
        SmartPtr<render::IGraphicsSceneNode> m_node;

        SmartPtr<IFrameStatistics> m_frameStatistics;
    };

}  // namespace workphone

#endif
