#include "AI/OpenAICredentialStore.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    namespace credentials = workphone::openai_credentials::detail;

    void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    void resolutionContracts()
    {
        std::string key, error;
        unsigned reads = 0;
        auto saved = [&]( std::string &value, std::string &diagnostic ) {
            ++reads;
            value = "sk-test-saved";
            diagnostic.clear();
            return true;
        };
        require( credentials::resolve( true, saved, "sk-test-environment", key, error ) &&
                 key == "sk-test-saved" && error.empty(),
                 "Saved Editor key must take precedence over environment" );
        require( credentials::resolve( false, saved, "sk-test-environment", key, error ) &&
                 key == "sk-test-environment" && reads == 1,
                 "Runtime must not read the personal Editor credential" );
        auto missing = []( std::string &value, std::string &diagnostic ) {
            value.clear(); diagnostic.clear(); return true;
        };
        require( credentials::resolve( true, missing, "sk-test-environment", key, error ) &&
                 key == "sk-test-environment", "Missing saved key must allow environment fallback" );
        require( !credentials::resolve( true, missing, nullptr, key, error ) &&
                 key.empty() && !error.empty(), "Missing configuration must report an error" );
        auto inaccessible = []( std::string &value, std::string &diagnostic ) {
            value.clear(); diagnostic = "Credential store unavailable."; return false;
        };
        require( !credentials::resolve( true, inaccessible, "sk-test-environment", key, error ) &&
                 key.empty() && !error.empty(),
                 "Credential read failure must not silently change the API account" );
        auto invalidSaved = []( std::string &value, std::string &diagnostic ) {
            value = "sk-test-invalid\r\nheader"; diagnostic.clear(); return true;
        };
        require( !credentials::resolve( true, invalidSaved, "sk-test-environment", key, error ) &&
                 key.empty() && error.find( "sk-test-invalid" ) == std::string::npos,
                 "Invalid stored credential must be rejected without disclosure" );
        for( const auto &value : { std::string(), std::string( " " ), std::string( " sk-test" ),
                                   std::string( "sk-test " ), std::string( "sk\ttest" ),
                                   std::string( "sk\r\ntest" ), std::string( "sk\0test", 7 ),
                                   std::string( 2049, 'a' ) } )
        {
            require( !credentials::validateKey( value, error ) && !error.empty(),
                     "Invalid credential was accepted" );
        }
        require( credentials::validateKey( "sk-test_valid-key.123", error ) && error.empty(),
                 "Printable credential was rejected" );
        require( !credentials::resolve( false, saved, "sk-test-invalid\r\nheader", key, error ) &&
                 key.empty() && error.find( "sk-test-invalid" ) == std::string::npos,
                 "Invalid environment credential must be rejected without disclosure" );
    }

#ifdef _WIN32
    void persistenceContracts()
    {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto target = L"Workphone/Tests/OpenAIKey/" + std::to_wstring( GetCurrentProcessId() ) +
                            L"-" + std::to_wstring( nonce );
        credentials::CredentialStore store( target );
        struct Cleanup
        {
            credentials::CredentialStore &store;
            ~Cleanup() { std::string ignored; store.remove( ignored ); }
        } cleanup{ store };
        std::string key, error;
        require( store.read( key, error ) && key.empty() && error.empty(),
                 "Unique credential fixture must initially be absent" );
        require( store.write( "sk-test-persistent", error ), "Could not store test credential" );
        credentials::CredentialStore reopened( target );
        require( reopened.read( key, error ) && key == "sk-test-persistent" && error.empty(),
                 "Saved key did not persist across credential store instances" );
        PCREDENTIALW entry = nullptr;
        require( CredReadW( target.c_str(), CRED_TYPE_GENERIC, 0, &entry ) != FALSE,
                 "Could not inspect test credential metadata" );
        const auto persistence = entry->Persist;
        if( entry->CredentialBlob ) SecureZeroMemory( entry->CredentialBlob, entry->CredentialBlobSize );
        CredFree( entry );
        require( persistence == CRED_PERSIST_LOCAL_MACHINE, "Credential must persist for this user/machine" );
        require( !store.write( "invalid\ncredential", error ), "Invalid update must fail" );
        require( reopened.read( key, error ) && key == "sk-test-persistent",
                 "Invalid update replaced the previous credential" );
        require( store.write( "sk-test-replacement", error ) &&
                 reopened.read( key, error ) && key == "sk-test-replacement",
                 "Updated key was not immediately available" );
        require( store.remove( error ) && reopened.read( key, error ) && key.empty(),
                 "Removing the key did not clear persistent storage" );
        require( store.remove( error ) && error.empty(), "Removing an absent key should be harmless" );
    }
#endif
}

int main()
{
    try
    {
        resolutionContracts();
#ifdef _WIN32
        persistenceContracts();
#endif
        std::cout << "PASS: OpenAI credential precedence, validation, persistence and removal\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
