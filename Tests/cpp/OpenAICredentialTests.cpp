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
    std::wstring processTestTarget( const std::string &nonce )
    {
        require( !nonce.empty() && nonce.size() <= 100 &&
                     nonce.find_first_not_of( "0123456789-" ) == std::string::npos,
                 "Invalid credential subprocess test nonce" );
        return L"Workphone/Tests/OpenAIKey/Process-" + std::wstring( nonce.begin(), nonce.end() );
    }

    void credentialProcess( const std::string &operation, const std::string &nonce )
    {
        credentials::CredentialStore store( processTestTarget( nonce ) );
        std::string key, error;
        if( operation == "--credential-save" )
        {
            require( store.read( key, error ) && key.empty(),
                     "Subprocess credential fixture must initially be absent" );
            require( store.write( "sk-test-process-persistent", error ),
                     "First process could not save the test credential" );
        }
        else
        {
            require( operation == "--credential-read-remove", "Unknown credential test operation" );
            require( store.read( key, error ) && key == "sk-test-process-persistent" && error.empty(),
                     "Saved credential did not survive the writing process exiting" );
            require( store.remove( error ) && store.read( key, error ) && key.empty(),
                     "Second process could not remove the test credential" );
        }
    }

    void runCredentialProcess( const wchar_t *operation, const std::string &nonce )
    {
        std::wstring executable( 32768, L'\0' );
        const auto length = GetModuleFileNameW( nullptr, executable.data(),
                                              static_cast<DWORD>( executable.size() ) );
        require( length > 0 && length < executable.size(), "Could not locate credential test executable" );
        executable.resize( length );
        auto command = L"\"" + executable + L"\" " + operation + L" " +
                       std::wstring( nonce.begin(), nonce.end() );
        STARTUPINFOW startup{};
        startup.cb = sizeof( startup );
        PROCESS_INFORMATION process{};
        require( CreateProcessW( executable.c_str(), command.data(), nullptr, nullptr, FALSE,
                                 CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process ) != FALSE,
                 "Could not start credential test subprocess" );
        struct ProcessCleanup
        {
            PROCESS_INFORMATION &process;
            ~ProcessCleanup() { CloseHandle( process.hThread ); CloseHandle( process.hProcess ); }
        } cleanup{ process };
        const auto wait = WaitForSingleObject( process.hProcess, 10000 );
        if( wait != WAIT_OBJECT_0 )
        {
            TerminateProcess( process.hProcess, 1 );
            WaitForSingleObject( process.hProcess, 1000 );
            require( false, "Credential test subprocess did not finish" );
        }
        DWORD exitCode = 1;
        require( GetExitCodeProcess( process.hProcess, &exitCode ) && exitCode == 0,
                 "Credential test subprocess failed" );
    }

    void processPersistenceContracts()
    {
        const auto nonce = std::to_string( GetCurrentProcessId() ) + "-" +
                           std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() );
        credentials::CredentialStore store( processTestTarget( nonce ) );
        struct Cleanup
        {
            credentials::CredentialStore &store;
            ~Cleanup() { std::string ignored; store.remove( ignored ); }
        } cleanup{ store };
        runCredentialProcess( L"--credential-save", nonce );
        runCredentialProcess( L"--credential-read-remove", nonce );
        std::string key, error;
        require( store.read( key, error ) && key.empty() && error.empty(),
                 "Subprocess removal did not persist after the removing process exited" );
    }

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

int main( int argc, char **argv )
{
    try
    {
#ifdef _WIN32
        if( argc == 3 )
        {
            credentialProcess( argv[1], argv[2] );
            return 0;
        }
#endif
        require( argc == 1, "Unexpected credential test arguments" );
        resolutionContracts();
#ifdef _WIN32
        persistenceContracts();
        processPersistenceContracts();
#endif
        std::cout << "PASS: OpenAI credential precedence, validation, persistence and removal\n";
#ifdef _WIN32
        std::cout << "PASS: Credential save, read and removal across separate processes\n";
#endif
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
