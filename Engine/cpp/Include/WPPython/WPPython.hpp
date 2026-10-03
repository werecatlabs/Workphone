#ifndef WPPython_h__
#define WPPython_h__

#include "Workphone/WPCoreAutolink.hpp"
#include "Workphone/Interface/Memory/ISharedObject.hpp"

#if FB_USE_AUTO_LINK
#ifdef _DEBUG
#    pragma comment( lib, "WPPython.lib" )
#    pragma comment( lib, "WPPythonBind.lib" )
#elif NDEBUG
#    pragma comment( lib, "WPPython.lib" )
#    pragma comment( lib, "WPPythonBind.lib" )
#else
#    pragma comment( lib, "WPPython.lib" )
#    pragma comment( lib, "WPPythonBind.lib" )
#endif
#endif

namespace lioncat
{
    class WPPython : public ISharedObject
    {
    public:
        WPPython() = default;
        ~WPPython() override = default;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
    };

}  // namespace lioncat

#endif  // WPPython_h__
