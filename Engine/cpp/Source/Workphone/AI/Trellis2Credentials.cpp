#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/AI/Trellis2Credentials.hpp>
#include "EditorCredentialStore.hpp"
#include <cstdlib>
#include <exception>

namespace workphone::trellis2_credentials
{
    namespace detail = api_credentials::detail;
    namespace
    {
        detail::CredentialStore editorStore()
        {
            return detail::CredentialStore( detail::trellis2.editorTarget, detail::trellis2 );
        }
    }

    bool saveEditorKey( const String &key, String &error )
    {
        error.clear();
        try
        {
            std::string secret( key.data(), key.size() ), diagnostic;
            detail::SecretGuard guard{ secret };
            const auto saved = editorStore().write( secret, diagnostic );
            error = diagnostic.c_str();
            return saved;
        }
        catch( const std::exception & )
        {
            error = "Could not save the fal.ai API key. The previous key was kept.";
            return false;
        }
    }

    bool clearEditorKey( String &error )
    {
        error.clear();
        try
        {
            std::string diagnostic;
            const auto removed = editorStore().remove( diagnostic );
            error = diagnostic.c_str();
            return removed;
        }
        catch( const std::exception & )
        {
            error = "Could not remove the saved fal.ai API key.";
            return false;
        }
    }

    bool hasEditorKey( String &error )
    {
        error.clear();
        try
        {
            std::string secret, diagnostic;
            detail::SecretGuard guard{ secret };
            const auto loaded = editorStore().read( secret, diagnostic );
            error = diagnostic.c_str();
            return loaded && !secret.empty();
        }
        catch( const std::exception & )
        {
            error = "Could not read the saved fal.ai API key.";
            return false;
        }
    }

    bool resolveKey( bool useEditorKey, String &key, String &error )
    {
        key.clear();
        error.clear();
        try
        {
            std::string secret, diagnostic;
            detail::SecretGuard guard{ secret };
            const auto found = detail::resolve(
                useEditorKey,
                []( std::string &saved, std::string &failure ) {
                    return editorStore().read( saved, failure );
                },
                std::getenv( "FAL_KEY" ), secret, diagnostic, detail::trellis2 );
            if( found ) key.assign( secret.data(), secret.size() );
            error = diagnostic.c_str();
            return found;
        }
        catch( const std::exception & )
        {
            key.clear();
            error = "Could not access the fal.ai API key.";
            return false;
        }
    }
}
