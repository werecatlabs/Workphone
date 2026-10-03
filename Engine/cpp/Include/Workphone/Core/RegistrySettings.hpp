#ifndef RegistryManager_h__
#define RegistryManager_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

#ifdef WP_PLATFORM_WIN32
#    include <minwindef.h>
#endif

namespace workphone
{
    /** Class to manage registry settings. */
    class RegistrySettings : public ISharedObject
    {
    public:
        RegistrySettings();

        ~RegistrySettings() override;

        SmartPtr<Properties> getProperties() const override;

        void setProperties( SmartPtr<Properties> properties ) override;

    protected:
#ifdef WP_PLATFORM_WIN32
        static String getStringKeyValue( const String &strValueName );
        static StringW getStringKeyValue( const StringW &strValueName );

        static bool setStringKeyValue( HKEY hRootKey, const StringW &keyname, const StringW &name,
                                       const StringW &value );
        static void getStringKeyValue( HKEY hRootKey, const StringW &keyname, const StringW &name,
                                       StringW &value );

        static bool hasKeyValue( HKEY hRootKey, const StringW &keyname, const StringW &name );

        static LONG getDWORDRegKey( HKEY hKey, const StringW &strValueName, DWORD &nValue,
                                    DWORD nDefaultValue );
        static LONG getBoolRegKey( HKEY hKey, const StringW &strValueName, bool &bValue,
                                   bool bDefaultValue );
        static LONG getStringRegKey( HKEY hKey, const StringW &strValueName, StringW &strValue,
                                     const StringW &strDefaultValue );
#endif
    };
}  // namespace workphone

#endif  // RegistryManager_h__
