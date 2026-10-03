#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IApplication.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::core
{
    WP_CLASS_REGISTER_DERIVED( workphone::core, IApplication, ISharedObject );

#if defined WP_PLATFORM_APPLE
    const String IApplication::mediaPathStrBundle = "/Contents/Resources/";
#else
    const String IApplication::mediaPathStrBundle;
#endif

    const String IApplication::mediaPathStr = "Media";

    const u8 IApplication::frameStatisticsFlag = ( 1 << 1 );
    const u8 IApplication::debugModeFlag = ( 1 << 2 );
    const u8 IApplication::developerModeFlag = ( 1 << 3 );

    IApplication::IApplication() : ISharedObject( IApplication::typeInfo() )
    {
    }

    IApplication::~IApplication() = default;

}  // namespace workphone::core
