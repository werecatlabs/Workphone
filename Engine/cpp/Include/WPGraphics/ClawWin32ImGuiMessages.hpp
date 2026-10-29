#pragma once

#if defined WP_PLATFORM_WIN32
#    include <Windows.h>

namespace workphone::render::detail
{
    inline bool consumeImGuiSystemMenuMessage( UINT message, WPARAM command,
                                               bool platformBackendActive )
    {
        // ImGui draws its own menu bar. DefWindowProc turns an unhandled F10
        // key-up into SC_KEYMENU and may enter a native modal menu loop before
        // the next ImGui frame can dispatch the shortcut. Other system commands
        // (including SC_CLOSE / Alt+F4) keep their normal native behavior.
        return platformBackendActive && message == WM_SYSCOMMAND &&
               ( command & 0xfff0 ) == SC_KEYMENU;
    }
}
#endif
