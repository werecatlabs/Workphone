#include "AI/EditorCredentialStore.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    namespace credentials = workphone::api_credentials::detail;

    void require( bool value, const char *message )
    {
        if( !value ) throw std::runtime_error( message );
    }

    void resolutionContracts()
    {
        require( std::wstring( credentials::openAI.editorTarget ) != credentials::trellis2.editorTarget &&
                     std::wstring( credentials::meshy.editorTarget ) != credentials::trellis2.editorTarget,
                 "Providers must use separate production credential targets" );
        require( std::string( credentials::openAI.environment ) == "OPENAI_API_KEY" &&
                     std::string( credentials::trellis2.environment ) == "FAL_KEY" &&
                     std::string( credentials::meshy.environment ) == "MESHY_API_KEY",
                 "Provider environment variables changed or overlap" );
        std::string key, error;
        unsigned reads = 0;
        auto saved = [&]( std::string &value, std::string &diagnostic ) {
            ++reads;
            value = "fal_test_saved";
            diagnostic.clear();
            return true;
        };
        require( credentials::resolve( true, saved, "fal_test_env", key, error, credentials::trellis2 ) &&
                     key == "fal_test_saved" && error.empty(), "Saved Trellis2 key must win in the Editor" );
        require( credentials::resolve( false, saved, "fal_test_env", key, error, credentials::trellis2 ) &&
                     key == "fal_test_env" && reads == 1, "Runtime must not read personal Trellis2 credentials" );
        auto missing = []( std::string &value, std::string &diagnostic ) {
            value.clear(); diagnostic.clear(); return true;
        };
        require( credentials::resolve( true, missing, "fal_test_env", key, error, credentials::trellis2 ) &&
                     key == "fal_test_env", "Missing Trellis2 key must allow environment fallback" );
        require( !credentials::resolve( true, missing, nullptr, key, error, credentials::trellis2 ) &&
                     key.empty() && error.find( "FAL_KEY" ) != std::string::npos &&
                     error.find( "OpenAI" ) == std::string::npos,
                 "Missing Trellis2 configuration must identify its own settings and environment" );
        auto inaccessible = []( std::string &value, std::string &diagnostic ) {
            value.clear(); diagnostic.clear(); return false;
        };
        require( !credentials::resolve( true, inaccessible, "fal_test_env", key, error, credentials::trellis2 ) &&
                     key.empty() && error.find( "fal.ai" ) != std::string::npos,
                 "Saved-key read failure must not silently switch Trellis2 accounts" );
        auto corrupt = []( std::string &value, std::string &diagnostic ) {
            value = "fal_private\nheader"; diagnostic.clear(); return true;
        };
        require( !credentials::resolve( true, corrupt, "fal_test_env", key, error, credentials::trellis2 ) &&
                     key.empty() && error.find( "fal_private" ) == std::string::npos,
                 "Invalid saved Trellis2 credential must fail closed without disclosure" );
        for( const auto &invalid : { std::string(), std::string( " " ), std::string( " fal_private" ),
                                     std::string( "fal_private " ), std::string( "fal_private\r\n" ),
                                     std::string( "fal\0private", 11 ), std::string( 2049, 'k' ) } )
        {
            require( !credentials::validateKey( invalid, error, credentials::trellis2 ) &&
                         error.find( "fal.ai" ) != std::string::npos &&
                         error.find( "fal_private" ) == std::string::npos,
                     "Invalid key must produce a sanitized Trellis2 diagnostic" );
        }
        require( credentials::validateKey( "test-key-id:test-key-secret", error, credentials::trellis2 ) && error.empty(),
                 "fal.ai key ID/secret separator must be preserved" );
        require( !credentials::resolve( false, saved, "fal_private\nheader", key, error, credentials::trellis2 ) &&
                     key.empty() && reads == 1 && error.find( "fal_private" ) == std::string::npos,
                 "Environment key validation must reject header injection without consulting saved keys" );
    }

#ifdef _WIN32
    void isolationContracts()
    {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto root = L"Workphone/Tests/Trellis2Key/" + std::to_wstring( GetCurrentProcessId() ) +
                          L"-" + std::to_wstring( nonce );
        const auto trellis2Target = root + L"/Trellis2";
        credentials::CredentialStore openAI( root + L"/OpenAI", credentials::openAI );
        credentials::CredentialStore meshy( root + L"/Meshy", credentials::meshy );
        credentials::CredentialStore trellis2( trellis2Target, credentials::trellis2 );
        struct Cleanup
        {
            credentials::CredentialStore &openAI;
            credentials::CredentialStore &meshy;
            credentials::CredentialStore &trellis2;
            ~Cleanup() { std::string ignored; openAI.remove( ignored ); meshy.remove( ignored ); trellis2.remove( ignored ); }
        } cleanup{ openAI, meshy, trellis2 };
        std::string key, error;
        require( openAI.read( key, error ) && key.empty() && meshy.read( key, error ) && key.empty() &&
                     trellis2.read( key, error ) && key.empty(),
                 "Disposable provider fixtures must initially be absent" );
        require( openAI.write( "sk-test-isolation", error ) && meshy.write( "msy_test_isolation", error ) &&
                     trellis2.write( "fal_test_isolation", error ),
                 "Could not write disposable provider credentials" );
        credentials::CredentialStore reopenedTrellis2( trellis2Target, credentials::trellis2 );
        require( reopenedTrellis2.read( key, error ) && key == "fal_test_isolation",
                 "Trellis2 key did not persist across store instances" );
        require( !trellis2.write( "invalid\nkey", error ) && reopenedTrellis2.read( key, error ) &&
                     key == "fal_test_isolation", "Rejected update replaced the saved Trellis2 key" );
        require( trellis2.write( "fal_test_replacement", error ) && reopenedTrellis2.read( key, error ) &&
                     key == "fal_test_replacement", "Replacement Trellis2 key was not immediately available" );
        require( openAI.read( key, error ) && key == "sk-test-isolation" &&
                     meshy.read( key, error ) && key == "msy_test_isolation",
                 "fal.ai operations altered another provider credential" );

        // Corrupt only the disposable target, proving the read path fails closed before fallback.
        unsigned char invalidBlob[] = { 'b', 'a', 'd', '\n' };
        CREDENTIALW corrupted{};
        corrupted.Type = CRED_TYPE_GENERIC;
        corrupted.TargetName = const_cast<wchar_t *>( trellis2Target.c_str() );
        corrupted.CredentialBlobSize = sizeof( invalidBlob );
        corrupted.CredentialBlob = invalidBlob;
        corrupted.Persist = CRED_PERSIST_LOCAL_MACHINE;
        require( CredWriteW( &corrupted, 0 ) != FALSE, "Could not create the corrupt test fixture" );
        require( !credentials::resolve( true,
                     [&]( std::string &value, std::string &diagnostic ) { return trellis2.read( value, diagnostic ); },
                     "fal_test_environment", key, error, credentials::trellis2 ) && key.empty() &&
                     error.find( "fal.ai" ) != std::string::npos,
                 "Corrupt saved Trellis2 credential must fail closed without environment fallback" );
        require( trellis2.remove( error ) && reopenedTrellis2.read( key, error ) && key.empty(),
                 "Corrupt Trellis2 key could not be removed" );
        require( openAI.read( key, error ) && key == "sk-test-isolation" &&
                     meshy.read( key, error ) && key == "msy_test_isolation",
                 "Removing fal.ai credentials removed another provider's key" );
        require( trellis2.remove( error ) && error.empty(), "Removing a missing Trellis2 key should be harmless" );
    }
#endif
}

int main()
{
    try
    {
        resolutionContracts();
#ifdef _WIN32
        isolationContracts();
#endif
        std::cout << "PASS: TRELLIS 2 fal.ai credential resolution, validation, persistence and provider isolation\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
