#ifndef WORKPHONE_INTERACTION_BACKEND_PROVIDERS_HPP
#define WORKPHONE_INTERACTION_BACKEND_PROVIDERS_HPP
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
namespace workphone
{
    /** Plugins register an exact factory type after installing their existing
     * ISoundManager/IInputDeviceManager factory. Empty names unregister. The
     * profile is delivered through setProperties before load. */
    WPCore_API void registerProjectSoundBackend( s32 selection, const String &factoryType );
    WPCore_API void registerProjectInputBackend( s32 selection, const String &factoryType );
    WPCore_API bool hasProjectSoundBackend( SmartPtr<IFactoryManager> factories, s32 selection );
    WPCore_API bool hasProjectInputBackend( SmartPtr<IFactoryManager> factories, s32 selection );
    WPCore_API bool matchesProjectSoundBackend( SmartPtr<IFactoryManager> factories,
        SmartPtr<ISoundManager> instance, s32 selection );
    WPCore_API bool matchesProjectInputBackend( SmartPtr<IFactoryManager> factories,
        SmartPtr<IInputDeviceManager> instance, s32 selection );
    WPCore_API SmartPtr<ISoundManager> createProjectSoundBackend(
        SmartPtr<IFactoryManager> factories, SmartPtr<Properties> profile );
    WPCore_API SmartPtr<IInputDeviceManager> createProjectInputBackend(
        SmartPtr<IFactoryManager> factories, SmartPtr<Properties> profile );
}
#endif
