#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/WPImGui.hpp>
#include <WPImGui/ImGuiProfilerWindow.hpp>
#include <WPImGui/ImGuiProfileWindow.hpp>
#include <WPImGui/ImGuiManager.hpp>
#include <WPImGui/ImGuiTreeNode.hpp>
#include <WPImGui/ImGuiTreeCtrl.hpp>
#include <WPImGui/ImGuiText.hpp>
#include <Workphone/Workphone.hpp>

#if defined WP_PLATFORM_WIN32
#    include <windows.h>
#endif

/*
#if defined WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
int WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID )
{
    return 1;
}
#    endif
#endif
*/

namespace workphone::ui
{
    SmartPtr<WPImGui> WPImGui::m_sPlugin;

    WPImGui::WPImGui() = default;

    WPImGui::~WPImGui() = default;

    void WPImGui::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        FactoryUtil::addFactory<ImGuiManager>();
    }

    void WPImGui::unload( SmartPtr<ISharedObject> data )
    {
        FactoryUtil::removeFactory<ImGuiManager>();
    }

    SmartPtr<IUIManager> WPImGui::createUI()
    {
        return workphone::make_ptr<ImGuiManager>();
    }

    SmartPtr<WPImGui> WPImGui::instance()
    {
        return m_sPlugin;
    }

    void WPImGui::setInstance( SmartPtr<WPImGui> plugin )
    {
        m_sPlugin = plugin;
    }
}  // namespace workphone::ui

/*
#ifndef _WP_STATIC_LIB_
extern "C" {

WP_INTERFACE_EXPORT void WP_INTERFACE_API workphone_get_version( int *major, int *minor, int *patch )
{
    *major = WP_VERSION_MAJOR;
    *minor = WP_VERSION_MINOR;
    *patch = WP_VERSION_PATCH;
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
loadPlugin( workphone::core::IApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace ui;

    auto plugin = workphone::make_ptr<WPImGui>();
    plugin->load( nullptr );
    WPImGui::setInstance( plugin );
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
unloadPlugin( workphone::core::IApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace ui;

    if( auto plugin = WPImGui::instance() )
    {
        plugin->unload( nullptr );
        WPImGui::setInstance( nullptr );
    }
}
}
#endif
*/
