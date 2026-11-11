#pragma once

#include <cstddef>
#include <string>
#include <utility>

#ifdef _WIN32
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <Windows.h>
#    include <wincred.h>
#    pragma comment( lib, "advapi32.lib" )
#endif

namespace workphone::openai_credentials::detail
{
    constexpr std::size_t maximumKeySize = 2048;

    inline void wipeString( std::string &value )
    {
        if( !value.empty() )
        {
            volatile char *bytes = &value[0];
            for( std::size_t i = 0; i < value.size(); ++i ) bytes[i] = 0;
        }
        value.clear();
    }

    struct SecretGuard
    {
        std::string &value;
        ~SecretGuard() { wipeString( value ); }
    };

    inline bool validateKey( const std::string &key, std::string &error )
    {
        error.clear();
        if( key.empty() )
        {
            error = "Enter an OpenAI API key. Use Remove Saved Key to clear the saved credential.";
            return false;
        }
        if( key.size() > maximumKeySize )
        {
            error = "The OpenAI API key exceeds the supported length.";
            return false;
        }
        for( unsigned char character : key )
        {
            if( character <= 32 || character >= 127 )
            {
                error = "The OpenAI API key contains whitespace or invalid characters.";
                return false;
            }
        }
        return true;
    }

    // Explicit targets allow offline tests to exercise persistence using a unique,
    // disposable credential without reading or modifying the user's Editor key.
    class CredentialStore
    {
    public:
        explicit CredentialStore( std::wstring target ) : m_target( std::move( target ) ) {}

        bool read( std::string &key, std::string &error ) const
        {
            wipeString( key );
            error.clear();
#ifdef _WIN32
            PCREDENTIALW credential = nullptr;
            if( !CredReadW( m_target.c_str(), CRED_TYPE_GENERIC, 0, &credential ) )
            {
                if( GetLastError() == ERROR_NOT_FOUND ) return true;
                error = "Could not read the saved OpenAI API key from Windows Credential Manager.";
                return false;
            }
            struct CredentialGuard
            {
                PCREDENTIALW value;
                ~CredentialGuard()
                {
                    if( value->CredentialBlob && value->CredentialBlobSize )
                        SecureZeroMemory( value->CredentialBlob, value->CredentialBlobSize );
                    CredFree( value );
                }
            } guard{ credential };
            if( !credential->CredentialBlob || credential->CredentialBlobSize == 0 ||
                credential->CredentialBlobSize > maximumKeySize )
            {
                error = "The saved OpenAI API key is invalid. Replace it in File > Settings > Editor.";
                return false;
            }
            key.assign( reinterpret_cast<const char *>( credential->CredentialBlob ),
                        credential->CredentialBlobSize );
            if( !validateKey( key, error ) )
            {
                wipeString( key );
                error = "The saved OpenAI API key is invalid. Replace it in File > Settings > Editor.";
                return false;
            }
#endif
            return true;
        }

        bool write( const std::string &key, std::string &error ) const
        {
            if( !validateKey( key, error ) ) return false;
#ifdef _WIN32
            CREDENTIALW credential{};
            credential.Type = CRED_TYPE_GENERIC;
            credential.TargetName = const_cast<wchar_t *>( m_target.c_str() );
            credential.CredentialBlobSize = static_cast<DWORD>( key.size() );
            credential.CredentialBlob = reinterpret_cast<LPBYTE>( const_cast<char *>( key.data() ) );
            credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
            if( CredWriteW( &credential, 0 ) ) return true;
            error = "Could not save the OpenAI API key in Windows Credential Manager. The previous key was kept.";
#else
            error = "Saving an OpenAI API key in Editor settings is currently supported on Windows. Use OPENAI_API_KEY on this platform.";
#endif
            return false;
        }

        bool remove( std::string &error ) const
        {
            error.clear();
#ifdef _WIN32
            if( CredDeleteW( m_target.c_str(), CRED_TYPE_GENERIC, 0 ) ||
                GetLastError() == ERROR_NOT_FOUND ) return true;
            error = "Could not remove the saved OpenAI API key from Windows Credential Manager.";
            return false;
#else
            return true;
#endif
        }

    private:
        std::wstring m_target;
    };

    template <class Reader>
    bool resolve( bool useEditorKey, Reader readSaved, const char *environmentKey,
                  std::string &key, std::string &error )
    {
        wipeString( key );
        error.clear();
        if( useEditorKey )
        {
            if( !readSaved( key, error ) )
            {
                wipeString( key );
                if( error.empty() ) error = "Could not read the saved OpenAI API key.";
                return false;
            }
            if( !key.empty() )
            {
                if( validateKey( key, error ) ) return true;
                wipeString( key );
                return false;
            }
        }
        if( !environmentKey || !*environmentKey )
        {
            error = useEditorKey
                        ? "Add an OpenAI API key in File > Settings > Editor, or set OPENAI_API_KEY."
                        : "Set OPENAI_API_KEY to use OpenAI.";
            return false;
        }
        key = environmentKey;
        if( validateKey( key, error ) ) return true;
        wipeString( key );
        return false;
    }
}
