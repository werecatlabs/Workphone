#ifndef WPImGuiPrerequisites_h__
#define WPImGuiPrerequisites_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneConfig.hpp>

#if 0
#    ifdef WP_PLATFORM_WIN32
#        ifndef _WP_STATIC_LIB_
#            ifdef WPImGui_EXPORTS
#                define WPImGui_API __declspec( dllexport )
#            else
#                define WPImGui_API __declspec( dllimport )
#            endif  // WP_EXPORT
#        else
#            define WPImGui_API
#        endif  // _WP_STATIC_LIB_
#    else
#        define WPImGui_API
#    endif
#else
#    define WPImGui_API
#endif

#if WP_BUILD_SDL2
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;
#endif

class ImguiManagerOgre;

namespace OIS
{
    class JoyStick;
}

namespace Ogre
{
    class MetalDevice;
    class MetalWindow;
}  // namespace Ogre

namespace Ogre
{
    class RenderTargetListener;
}

namespace workphone
{
    class ImGuiOverlayOgre;

    namespace ui
    {
        class WPImGui;

        class ImGuiApplication;
        class ImGuiApplicationOSX;
        class ImGuiRenderWindow;

        template <class T>
        class ImGuiWindowT;

        class ImGuiMenu;
        class ImGuiMenuBar;
        class ImGuiMenuItem;
    }  // end namespace ui
}  // namespace workphone

#endif  // WPImGuiPrerequisites_h__
