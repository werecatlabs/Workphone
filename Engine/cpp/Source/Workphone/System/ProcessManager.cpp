#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ProcessManager.hpp>
#include <Workphone/Core/StringUtil.hpp>

#if defined WP_PLATFORM_WIN32
#    include <processthreadsapi.h>
#    include <windef.h>
#    include <shellapi.h>
#    include <io.h>
#    include <windows.h>
#    include <tlhelp32.h>
#elif defined WP_PLATFORM_APPLE
#    include <libproc.h>
#    include <signal.h>
#    include <sys/wait.h>
#    include <unistd.h>
#elif defined WP_PLATFORM_LINUX || defined WP_PLATFORM_ANDROID
#    include <cctype>
#    include <dirent.h>
#    include <fstream>
#    include <signal.h>
#    include <sys/types.h>
#    include <sys/wait.h>
#    include <unistd.h>
#endif

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ProcessManager, IProcessManager );

    ProcessManager::ProcessManager() = default;

    ProcessManager::~ProcessManager() = default;

    void ProcessManager::createProcess( const String &applicationName )
    {
#ifdef WP_PLATFORM_WIN32
        // additional information
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        // set the size of the structures
        ZeroMemory( &si, sizeof( si ) );
        si.cb = sizeof( si );
        ZeroMemory( &pi, sizeof( pi ) );

        // start the program up
        CreateProcessA( applicationName.c_str(),  // the path
                        nullptr,                  // Command line
                        nullptr,                  // Process handle not inheritable
                        nullptr,                  // Thread handle not inheritable
                        FALSE,                    // Set handle inheritance to FALSE
                        0,                        // No creation flags
                        nullptr,                  // Use parent's environment block
                        nullptr,                  // Use parent's starting directory
                        &si,                      // Pointer to STARTUPINFO structure
                        &pi );                    // Pointer to PROCESS_INFORMATION structure

        // Close process and thread handles.
        // CloseHandle(pi.hProcess);
        // CloseHandle(pi.hThread);
#else
        pid_t pid = fork();
        if( pid == 0 )
        {
            execl( applicationName.c_str(), applicationName.c_str(), nullptr );
            _exit( EXIT_FAILURE );
        }
#endif
    }

    void ProcessManager::createProcess( const StringW &applicationName )
    {
#ifdef WP_PLATFORM_WIN32
        // additional information
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;

        // set the size of the structures
        ZeroMemory( &si, sizeof( si ) );
        si.cb = sizeof( si );
        ZeroMemory( &pi, sizeof( pi ) );

        // start the program up
        CreateProcessW( applicationName.c_str(),  // the path
                        nullptr,                  // Command line
                        nullptr,                  // Process handle not inheritable
                        nullptr,                  // Thread handle not inheritable
                        FALSE,                    // Set handle inheritance to FALSE
                        0,                        // No creation flags
                        nullptr,                  // Use parent's environment block
                        nullptr,                  // Use parent's starting directory
                        &si,                      // Pointer to STARTUPINFO structure
                        &pi );                    // Pointer to PROCESS_INFORMATION structure

        // Close process and thread handles.
        // CloseHandle(pi.hProcess);
        // CloseHandle(pi.hThread);
#else
        auto appUTF8 = StringUtil::toUTF16to8( applicationName );
        pid_t pid = fork();
        if( pid == 0 )
        {
            execl( appUTF8.c_str(), appUTF8.c_str(), nullptr );
            _exit( EXIT_FAILURE );
        }
#endif
    }

    void ProcessManager::createProcess( const StringW &applicationName, const Array<StringW> &args )
    {
#ifdef WP_PLATFORM_WIN32
        StringW argStr;
        for( const auto &arg : args )
        {
            argStr += arg + L" ";
        }

        // additional information
        //STARTUPINFOW si;
        //PROCESS_INFORMATION pi;

        STARTUPINFOW si = { sizeof( STARTUPINFOW ) };
        PROCESS_INFORMATION pi = {};

        // set the size of the structures
        //ZeroMemory( &si, sizeof( si ) );
        //si.cb = sizeof( si );
        //ZeroMemory( &pi, sizeof( pi );

        // start the program up
        CreateProcessW( applicationName.c_str(),  // the path
                        (LPWSTR)argStr.c_str(),   // Command line
                        nullptr,                  // Process handle not inheritable
                        nullptr,                  // Thread handle not inheritable
                        FALSE,                    // Set handle inheritance to FALSE
                        0,                        // No creation flags
                        nullptr,                  // Use parent's environment block
                        nullptr,                  // Use parent's starting directory
                        &si,                      // Pointer to STARTUPINFO structure
                        &pi );                    // Pointer to PROCESS_INFORMATION structure
                                                  // Close process and thread handles.
                                                  // CloseHandle(pi.hProcess);
                                                  // CloseHandle(pi.hThread);
#else
        auto appUTF8 = StringUtil::toUTF16to8( applicationName );
        std::vector<String> argStrings;
        argStrings.push_back( appUTF8 );
        for( const auto &arg : args )
            argStrings.push_back( StringUtil::toUTF16to8( arg ) );

        std::vector<char *> argv;
        for( auto &s : argStrings )
            argv.push_back( const_cast<char *>( s.c_str() ) );
        argv.push_back( nullptr );

        pid_t pid = fork();
        if( pid == 0 )
        {
            execv( appUTF8.c_str(), argv.data() );
            _exit( EXIT_FAILURE );
        }
#endif
    }

    void ProcessManager::shellExecute( const String &applicationName )
    {
#ifdef WP_PLATFORM_WIN32
        SHELLEXECUTEINFOA shExInfo = { 0 };
        shExInfo.cbSize = sizeof( shExInfo );
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.hwnd = nullptr;
        shExInfo.lpVerb = "runas";                  // Operation to perform
        shExInfo.lpFile = applicationName.c_str();  // Application to start
        shExInfo.lpParameters = "";                 // Additional parameters
        shExInfo.lpDirectory = nullptr;
        shExInfo.nShow = SW_SHOW;
        shExInfo.hInstApp = nullptr;

        if( ShellExecuteExA( &shExInfo ) )
        {
            WaitForSingleObject( shExInfo.hProcess, INFINITE );
            CloseHandle( shExInfo.hProcess );
        }
#else
        pid_t pid = fork();
        if( pid == 0 )
        {
            execl( applicationName.c_str(), applicationName.c_str(), nullptr );
            _exit( EXIT_FAILURE );
        }
        else if( pid > 0 )
        {
            int status;
            waitpid( pid, &status, 0 );
        }
#endif
    }

    void ProcessManager::shellExecute( const StringW &applicationName )
    {
#ifdef WP_PLATFORM_WIN32
        SHELLEXECUTEINFOW shExInfo = { 0 };
        shExInfo.cbSize = sizeof( shExInfo );
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.hwnd = nullptr;
        shExInfo.lpVerb = L"runas";                 // Operation to perform
        shExInfo.lpFile = applicationName.c_str();  // Application to start
        shExInfo.lpParameters = L"";                // Additional parameters
        shExInfo.lpDirectory = nullptr;
        shExInfo.nShow = SW_SHOW;
        shExInfo.hInstApp = nullptr;

        if( ShellExecuteExW( &shExInfo ) )
        {
            WaitForSingleObject( shExInfo.hProcess, INFINITE );
            CloseHandle( shExInfo.hProcess );
        }
#else
        auto appUTF8 = StringUtil::toUTF16to8( applicationName );
        pid_t pid = fork();
        if( pid == 0 )
        {
            execl( appUTF8.c_str(), appUTF8.c_str(), nullptr );
            _exit( EXIT_FAILURE );
        }
        else if( pid > 0 )
        {
            int status;
            waitpid( pid, &status, 0 );
        }
#endif
    }

    void ProcessManager::shellExecute( const StringW &applicationName, const Array<StringW> &args )
    {
#ifdef WP_PLATFORM_WIN32
        StringW argStr;

        for( const auto &arg : args )
        {
            argStr += arg + L" ";
        }

        SHELLEXECUTEINFOW shExInfo = { 0 };
        shExInfo.cbSize = sizeof( shExInfo );
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.hwnd = nullptr;
        //shExInfo.lpVerb = L"run";
        shExInfo.lpVerb = L"runas";                 // Operation to perform
        shExInfo.lpFile = applicationName.c_str();  // Application to start
        shExInfo.lpParameters = argStr.c_str();     // Additional parameters
        shExInfo.lpDirectory = nullptr;
        shExInfo.nShow = SW_SHOW;
        shExInfo.hInstApp = nullptr;

        if( ShellExecuteExW( &shExInfo ) )
        {
            WaitForSingleObject( shExInfo.hProcess, INFINITE );
            CloseHandle( shExInfo.hProcess );
        }
#else
        auto appUTF8 = StringUtil::toUTF16to8( applicationName );
        std::vector<String> argStrings;
        argStrings.push_back( appUTF8 );
        for( const auto &arg : args )
            argStrings.push_back( StringUtil::toUTF16to8( arg ) );

        std::vector<char *> argv;
        for( auto &s : argStrings )
            argv.push_back( const_cast<char *>( s.c_str() ) );
        argv.push_back( nullptr );

        pid_t pid = fork();
        if( pid == 0 )
        {
            execv( appUTF8.c_str(), argv.data() );
            _exit( EXIT_FAILURE );
        }
        else if( pid > 0 )
        {
            int status;
            waitpid( pid, &status, 0 );
        }
#endif
    }

    void ProcessManager::shellExecute( const StringW &applicationName, const StringW &directory,
                                       const Array<StringW> &args )
    {
#ifdef WP_PLATFORM_WIN32
        StringW argStr;

        for( const auto &arg : args )
        {
            argStr += arg + L" ";
        }

        SHELLEXECUTEINFOW shExInfo = { 0 };
        shExInfo.cbSize = sizeof( shExInfo );
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.hwnd = nullptr;
        shExInfo.lpVerb = L"runas";                 // Operation to perform
        shExInfo.lpFile = applicationName.c_str();  // Application to start
        shExInfo.lpParameters = argStr.c_str();     // Additional parameters
        shExInfo.lpDirectory = directory.c_str();
        shExInfo.nShow = SW_SHOW;
        shExInfo.hInstApp = nullptr;

        if( ShellExecuteExW( &shExInfo ) )
        {
            // WaitForSingleObject(shExInfo.hProcess, INFINITE);
            CloseHandle( shExInfo.hProcess );
        }
#else
        auto appUTF8 = StringUtil::toUTF16to8( applicationName );
        auto dirUTF8 = StringUtil::toUTF16to8( directory );
        std::vector<String> argStrings;
        argStrings.push_back( appUTF8 );
        for( const auto &arg : args )
            argStrings.push_back( StringUtil::toUTF16to8( arg ) );

        std::vector<char *> argv;
        for( auto &s : argStrings )
            argv.push_back( const_cast<char *>( s.c_str() ) );
        argv.push_back( nullptr );

        pid_t pid = fork();
        if( pid == 0 )
        {
            if( !dirUTF8.empty() )
                chdir( dirUTF8.c_str() );
            execv( appUTF8.c_str(), argv.data() );
            _exit( EXIT_FAILURE );
        }
#endif
    }

    auto ProcessManager::isProcessRunning( const String &processName ) -> bool
    {
        return isProcessRunning( StringUtil::toStringW( processName ) );
    }

    auto ProcessManager::isProcessRunning( const StringW &processName ) -> bool
    {
#ifdef WP_PLATFORM_WIN32
        bool exists = false;
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof( PROCESSENTRY32W );

        HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, NULL );

        if( Process32FirstW( snapshot, &entry ) )
        {
            while( Process32NextW( snapshot, &entry ) )
            {
                if( StringW( entry.szExeFile ) == processName )
                {
                    exists = true;
                }
            }
        }

        CloseHandle( snapshot );
        return exists;
#elif defined WP_PLATFORM_APPLE
        auto nameUTF8 = StringUtil::toUTF16to8( processName );
        int pids[4096];
        int count = proc_listallpids( pids, sizeof( pids ) );
        char nameBuf[PROC_PIDPATHINFO_MAXSIZE];
        for( int i = 0; i < count; ++i )
        {
            proc_name( pids[i], nameBuf, sizeof( nameBuf ) );
            if( nameUTF8 == nameBuf )
                return true;
        }
        return false;
#elif defined WP_PLATFORM_LINUX
        auto nameUTF8 = StringUtil::toUTF16to8( processName );
        DIR *dir = opendir( "/proc" );
        if( !dir )
            return false;
        struct dirent *entry;
        while( ( entry = readdir( dir ) ) != nullptr )
        {
            bool isPid = ( entry->d_name[0] != '\0' );
            for( const char *p = entry->d_name; *p && isPid; ++p )
                isPid = ( isdigit( static_cast<unsigned char>( *p ) ) != 0 );
            if( !isPid )
                continue;
            std::ifstream commFile( std::string( "/proc/" ) + entry->d_name + "/comm" );
            std::string comm;
            if( std::getline( commFile, comm ) && comm == nameUTF8 )
            {
                closedir( dir );
                return true;
            }
        }
        closedir( dir );
        return false;
#else
        return false;
#endif
    }

    auto ProcessManager::terminateProcess( const String &processName ) -> bool
    {
        return terminateProcess( StringUtil::toStringW( processName ) );
    }

    auto ProcessManager::terminateProcess( const StringW &processName ) -> bool
    {
#ifdef WP_PLATFORM_WIN32
        bool exists = false;
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof( PROCESSENTRY32W );

        HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, NULL );

        if( Process32FirstW( snapshot, &entry ) )
        {
            while( Process32NextW( snapshot, &entry ) )
            {
                if( StringW( entry.szExeFile ) == processName )
                {
                    internalTerminateProcess( entry.th32ProcessID, 0 );
                    exists = true;
                }
            }
        }

        CloseHandle( snapshot );
        return exists;
#elif defined WP_PLATFORM_APPLE
        auto nameUTF8 = StringUtil::toUTF16to8( processName );
        int pids[4096];
        int count = proc_listallpids( pids, sizeof( pids ) );
        char nameBuf[PROC_PIDPATHINFO_MAXSIZE];
        bool terminated = false;
        for( int i = 0; i < count; ++i )
        {
            proc_name( pids[i], nameBuf, sizeof( nameBuf ) );
            if( nameUTF8 == nameBuf )
            {
                internalTerminateProcess( static_cast<unsigned long>( pids[i] ), 0 );
                terminated = true;
            }
        }
        return terminated;
#elif defined WP_PLATFORM_LINUX
        auto nameUTF8 = StringUtil::toUTF16to8( processName );
        DIR *dir = opendir( "/proc" );
        if( !dir )
            return false;
        struct dirent *entry;
        bool terminated = false;
        while( ( entry = readdir( dir ) ) != nullptr )
        {
            bool isPid = ( entry->d_name[0] != '\0' );
            for( const char *p = entry->d_name; *p && isPid; ++p )
                isPid = ( isdigit( static_cast<unsigned char>( *p ) ) != 0 );
            if( !isPid )
                continue;
            std::ifstream commFile( std::string( "/proc/" ) + entry->d_name + "/comm" );
            std::string comm;
            if( std::getline( commFile, comm ) && comm == nameUTF8 )
            {
                pid_t pid = static_cast<pid_t>( std::stoi( entry->d_name ) );
                internalTerminateProcess( static_cast<unsigned long>( pid ), 0 );
                terminated = true;
            }
        }
        closedir( dir );
        return terminated;
#else
        return false;
#endif
    }

    auto ProcessManager::internalTerminateProcess( unsigned long dwProcessId, u32 uExitCode ) -> bool
    {
#ifdef WP_PLATFORM_WIN32
        DWORD dwDesiredAccess = PROCESS_TERMINATE;
        BOOL bInheritHandle = FALSE;
        HANDLE hProcess = OpenProcess( dwDesiredAccess, bInheritHandle, dwProcessId );
        if( hProcess == nullptr )
        {
            return false;
        }

        BOOL result = TerminateProcess( hProcess, uExitCode );

        CloseHandle( hProcess );

        return result == TRUE;
#else
        pid_t pid = static_cast<pid_t>( dwProcessId );
        return kill( pid, SIGTERM ) == 0;
#endif
    }
}  // namespace workphone
