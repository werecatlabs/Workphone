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
        require( std::wstring( credentials::openAI.editorTarget ) != credentials::meshy.editorTarget,
                 "Providers must use separate production credential targets" );
        require( std::string( credentials::openAI.environment ) == "OPENAI_API_KEY" &&
                     std::string( credentials::meshy.environment ) == "MESHY_API_KEY",
                 "Provider environment variables changed or overlap" );
        std::string key, error;
        unsigned reads = 0;
        auto saved = [&]( std::string &value, std::string &diagnostic ) {
            ++reads;
            value = "msy_test_saved";
            diagnostic.clear();
            return true;
        };
        require( credentials::resolve( true, saved, "msy_test_env", key, error, credentials::meshy ) &&
                     key == "msy_test_saved" && error.empty(), "Saved Meshy key must win in the Editor" );
        require( credentials::resolve( false, saved, "msy_test_env", key, error, credentials::meshy ) &&
                     key == "msy_test_env" && reads == 1, "Runtime must not read personal Meshy credentials" );
        auto missing = []( std::string &value, std::string &diagnostic ) {
            value.clear(); diagnostic.clear(); return true;
        };
        require( credentials::resolve( true, missing, "msy_test_env", key, error, credentials::meshy ) &&
                     key == "msy_test_env", "Missing Meshy key must allow environment fallback" );
        require( !credentials::resolve( true, missing, nullptr, key, error, credentials::meshy ) &&
                     key.empty() && error.find( "MESHY_API_KEY" ) != std::string::npos &&
                     error.find( "OpenAI" ) == std::string::npos,
                 "Missing Meshy configuration must identify its own settings and environment" );
        auto inaccessible = []( std::string &value, std::string &diagnostic ) {
            value.clear(); diagnostic.clear(); return false;
        };
        require( !credentials::resolve( true, inaccessible, "msy_test_env", key, error, credentials::meshy ) &&
                     key.empty() && error.find( "Meshy" ) != std::string::npos,
                 "Saved-key read failure must not silently switch Meshy accounts" );
        auto corrupt = []( std::string &value, std::string &diagnostic ) {
            value = "msy_private\nheader"; diagnostic.clear(); return true;
        };
        require( !credentials::resolve( true, corrupt, "msy_test_env", key, error, credentials::meshy ) &&
                     key.empty() && error.find( "msy_private" ) == std::string::npos,
                 "Invalid saved Meshy credential must fail closed without disclosure" );
        for( const auto &invalid : { std::string(), std::string( " " ), std::string( " msy_private" ),
                                     std::string( "msy_private " ), std::string( "msy_private\r\n" ),
                                     std::string( "msy\0private", 11 ), std::string( 2049, 'k' ) } )
        {
            require( !credentials::validateKey( invalid, error, credentials::meshy ) &&
                         error.find( "Meshy" ) != std::string::npos &&
                         error.find( "msy_private" ) == std::string::npos,
                     "Invalid key must produce a sanitized Meshy diagnostic" );
        }
        require( credentials::validateKey( "msy_test-valid.123", error, credentials::meshy ) && error.empty(),
                 "Printable Meshy credential was rejected" );
        require( !credentials::resolve( false, saved, "msy_private\nheader", key, error, credentials::meshy ) &&
                     key.empty() && reads == 1 && error.find( "msy_private" ) == std::string::npos,
                 "Environment key validation must reject header injection without consulting saved keys" );
    }

#ifdef _WIN32
    void isolationContracts()
    {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto root = L"Workphone/Tests/MeshyKey/" + std::to_wstring( GetCurrentProcessId() ) +
                          L"-" + std::to_wstring( nonce );
        const auto meshyTarget = root + L"/Meshy";
        credentials::CredentialStore openAI( root + L"/OpenAI", credentials::openAI );
        credentials::CredentialStore meshy( meshyTarget, credentials::meshy );
        struct Cleanup
        {
            credentials::CredentialStore &openAI;
            credentials::CredentialStore &meshy;
            ~Cleanup() { std::string ignored; openAI.remove( ignored ); meshy.remove( ignored ); }
        } cleanup{ openAI, meshy };
        std::string key, error;
        require( openAI.read( key, error ) && key.empty() && meshy.read( key, error ) && key.empty(),
                 "Disposable provider fixtures must initially be absent" );
        require( openAI.write( "sk-test-isolation", error ) && meshy.write( "msy_test_isolation", error ),
                 "Could not write disposable provider credentials" );
        credentials::CredentialStore reopenedMeshy( meshyTarget, credentials::meshy );
        require( reopenedMeshy.read( key, error ) && key == "msy_test_isolation",
                 "Meshy key did not persist across store instances" );
        require( !meshy.write( "invalid\nkey", error ) && reopenedMeshy.read( key, error ) &&
                     key == "msy_test_isolation", "Rejected update replaced the saved Meshy key" );
        require( meshy.write( "msy_test_replacement", error ) && reopenedMeshy.read( key, error ) &&
                     key == "msy_test_replacement", "Replacement Meshy key was not immediately available" );
        require( openAI.read( key, error ) && key == "sk-test-isolation",
                 "Meshy operations altered the OpenAI credential" );

        // Corrupt only the disposable target, proving the read path fails closed before fallback.
        unsigned char invalidBlob[] = { 'b', 'a', 'd', '\n' };
        CREDENTIALW corrupted{};
        corrupted.Type = CRED_TYPE_GENERIC;
        corrupted.TargetName = const_cast<wchar_t *>( meshyTarget.c_str() );
        corrupted.CredentialBlobSize = sizeof( invalidBlob );
        corrupted.CredentialBlob = invalidBlob;
        corrupted.Persist = CRED_PERSIST_LOCAL_MACHINE;
        require( CredWriteW( &corrupted, 0 ) != FALSE, "Could not create the corrupt test fixture" );
        require( !credentials::resolve( true,
                     [&]( std::string &value, std::string &diagnostic ) { return meshy.read( value, diagnostic ); },
                     "msy_test_environment", key, error, credentials::meshy ) && key.empty() &&
                     error.find( "Meshy" ) != std::string::npos,
                 "Corrupt saved Meshy credential must fail closed without environment fallback" );
        require( meshy.remove( error ) && reopenedMeshy.read( key, error ) && key.empty(),
                 "Corrupt Meshy key could not be removed" );
        require( openAI.read( key, error ) && key == "sk-test-isolation",
                 "Removing Meshy credentials removed the other provider's key" );
        require( meshy.remove( error ) && error.empty(), "Removing a missing Meshy key should be harmless" );
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
        std::cout << "PASS: Meshy credential resolution, validation, persistence and provider isolation\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
