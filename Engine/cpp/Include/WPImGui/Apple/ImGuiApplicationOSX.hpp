#ifndef _FB_ImGuiApplicationOSX_H
#define _FB_ImGuiApplicationOSX_H

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/Graphics/IWindowListener.hpp>

namespace workphone
{
    namespace ui
    {

        class ImGuiApplicationOSX
        {
        public:
            void load();
            void update();
            void postUpdate();

            void handleWindowEvent(SmartPtr<render::IWindowEvent> event);

            void draw(SmartPtr<IUIRenderWindow> renderWindow);

        protected:
            class WindowListener : public render::IWindowListener
            {
            public:
                WindowListener() = default;
                ~WindowListener() = default;

                virtual void handleEvent(SmartPtr<render::IWindowEvent> event);

                void setOwner( ImGuiApplicationOSX *owner )
                {
                    m_owner = owner;
                }

                ImGuiApplicationOSX *getOwner() const
                {
                    return m_owner;
                }

            protected:
                ImGuiApplicationOSX *m_owner = nullptr;
            };

            Ogre::MetalDevice* m_device = nullptr;
            SmartPtr<render::IWindow> m_window;
        };
    }
}

#endif