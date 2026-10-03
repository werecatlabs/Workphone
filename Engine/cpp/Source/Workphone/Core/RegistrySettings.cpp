#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/RegistrySettings.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <iostream>

#ifdef WP_PLATFORM_WIN32
#    include <windows.h>
#endif

namespace workphone
{
    RegistrySettings::RegistrySettings() = default;

    RegistrySettings::~RegistrySettings() = default;

    auto RegistrySettings::getProperties() const -> SmartPtr<Properties>
    {
        return nullptr;
    }

    void RegistrySettings::setProperties( SmartPtr<Properties> properties )
    {
    }

#ifdef WP_PLATFORM_WIN32
    auto RegistrySettings::getStringRegKey( HKEY hKey, const StringW &strValueName, StringW &strValue,
                                            const StringW &strDefaultValue ) -> LONG
    {
        strValue = strDefaultValue;
        WCHAR szBuffer[512];
        DWORD dwBufferSize = sizeof( szBuffer );
        ULONG nError;
        nError = RegQueryValueExW( hKey, strValueName.c_str(), nullptr, nullptr,
                                   reinterpret_cast<LPBYTE>( szBuffer ), &dwBufferSize );
        if( ERROR_SUCCESS == nError )
        {
            strValue = szBuffer;
        }
        return nError;
    }

    auto RegistrySettings::getBoolRegKey( HKEY hKey, const StringW &strValueName, bool &bValue,
                                          bool bDefaultValue ) -> LONG
    {
        DWORD nDefValue( ( bDefaultValue ) ? 1 : 0 );
        DWORD nResult( nDefValue );
        LONG nError = getDWORDRegKey( hKey, strValueName.c_str(), nResult, nDefValue );
        if( ERROR_SUCCESS == nError )
        {
            bValue = ( nResult != 0 ) ? true : false;
        }
        return nError;
    }

    auto RegistrySettings::getDWORDRegKey( HKEY hKey, const StringW &strValueName, DWORD &nValue,
                                           DWORD nDefaultValue ) -> LONG
    {
        nValue = nDefaultValue;
        DWORD dwBufferSize( sizeof( DWORD ) );
        DWORD nResult( 0 );
        LONG nError = RegQueryValueExW( hKey, strValueName.c_str(), nullptr, nullptr,
                                        reinterpret_cast<LPBYTE>( &nResult ), &dwBufferSize );
        if( ERROR_SUCCESS == nError )
        {
            nValue = nResult;
        }
        return nError;
    }

    auto RegistrySettings::getStringKeyValue( const String &strValueName ) -> String
    {
        auto strValueNameW = StringUtil::toStringW( strValueName );

        HKEY hKey;
        LONG lRes = RegOpenKeyExW( HKEY_CLASSES_ROOT, strValueNameW.c_str(), 0, KEY_READ, &hKey );
        bool bExistsAndSuccess( lRes == ERROR_SUCCESS );
        bool bDoesNotExistsSpecifically( lRes == ERROR_FILE_NOT_FOUND );
        StringW strValueOfBinDir;
        StringW strKeyDefaultValue;
        if( ERROR_SUCCESS == getStringRegKey( hKey, strValueNameW.c_str(), strValueOfBinDir, L"" ) )
        {
            return StringUtil::toUTF16to8( strValueOfBinDir );
        }

        return {};
    }

    auto RegistrySettings::getStringKeyValue( const StringW &strValueName ) -> StringW
    {
        HKEY hKey;
        LONG lRes = RegOpenKeyExW( HKEY_CLASSES_ROOT, strValueName.c_str(), 0, KEY_READ, &hKey );
        bool bExistsAndSuccess( lRes == ERROR_SUCCESS );
        bool bDoesNotExistsSpecifically( lRes == ERROR_FILE_NOT_FOUND );
        StringW strValueOfBinDir;
        StringW strKeyDefaultValue;
        if( ERROR_SUCCESS == getStringRegKey( hKey, strValueName.c_str(), strValueOfBinDir, L"" ) )
        {
            return strValueOfBinDir;
        }

        return {};
    }

    auto RegistrySettings::setStringKeyValue( HKEY hRootKey, const StringW &keyname, const StringW &name,
                                              const StringW &value ) -> bool
    {
        bool retValue = false;

        HKEY hKey;

        LONG openRes = RegOpenKeyExW( hRootKey, keyname.c_str(), 0, KEY_ALL_ACCESS, &hKey );

        if( openRes == ERROR_SUCCESS )
        {
            std::cout << "Success opening key." << std::endl;
        }
        else
        {
            std::cout << "Error opening key." << std::endl;
        }

        if( openRes == ERROR_FILE_NOT_FOUND )
        {
            openRes = RegCreateKeyExW( hRootKey, keyname.c_str(), NULL, nullptr, REG_OPTION_NON_VOLATILE,
                                       KEY_ALL_ACCESS, nullptr, &hKey, nullptr );
        }

        auto cbData = static_cast<DWORD>( sizeof( StringW::value_type ) * value.length() + 1 );
        const auto setRes =
            RegSetValueExW( hKey, name.c_str(), 0, REG_SZ, (BYTE *)value.c_str(), cbData );

        if( setRes == ERROR_SUCCESS )
        {
            std::cout << "Success writing to Registry." << std::endl;
            retValue = true;
        }
        else
        {
            std::cout << "Error writing to Registry." << std::endl;
        }

        LONG closeOut = RegCloseKey( hKey );

        if( closeOut == ERROR_SUCCESS )
        {
            std::cout << "Success closing key." << std::endl;
        }
        else
        {
            std::cout << "Error closing key." << std::endl;
        }

        return retValue;
    }

    void RegistrySettings::getStringKeyValue( HKEY hRootKey, const StringW &keyname, const StringW &name,
                                              StringW &value )
    {
        HKEY hKey;
        LONG lRes = RegOpenKeyExW( hRootKey, keyname.c_str(), 0, KEY_READ, &hKey );
        bool bExistsAndSuccess( lRes == ERROR_SUCCESS );
        bool bDoesNotExistsSpecifically( lRes == ERROR_FILE_NOT_FOUND );
        StringW strValueOfBinDir;
        StringW strKeyDefaultValue;
        if( ERROR_SUCCESS == getStringRegKey( hKey, name.c_str(), strValueOfBinDir, L"" ) )
        {
            value = strValueOfBinDir;
        }
    }

    auto RegistrySettings::hasKeyValue( HKEY hRootKey, const StringW &keyname, const StringW &name )
        -> bool
    {
        HKEY hKey;
        LONG lRes = RegOpenKeyExW( hRootKey, keyname.c_str(), 0, KEY_READ, &hKey );
        bool bExistsAndSuccess( lRes == ERROR_SUCCESS );
        bool bDoesNotExistsSpecifically( lRes == ERROR_FILE_NOT_FOUND );
        StringW strValueOfBinDir;
        StringW strKeyDefaultValue;
        if( ERROR_SUCCESS == getStringRegKey( hKey, name.c_str(), strValueOfBinDir, L"" ) )
        {
            return true;
        }

        return false;
    }
#endif
}  // namespace workphone
