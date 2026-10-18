#pragma once

#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <iterator>

// Explicitly enabled by the test host; ordinary runs do not replace the process
// exception filter. Leave the exception unhandled so CTest still reports failure.
namespace graphics_crash_evidence
{
    inline wchar_t dumpPath[32768]{};

    inline LONG WINAPI writeDump( EXCEPTION_POINTERS *exception )
    {
        const auto file = CreateFileW( dumpPath, GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                      FILE_ATTRIBUTE_NORMAL, nullptr );
        if( file != INVALID_HANDLE_VALUE )
        {
            MINIDUMP_EXCEPTION_INFORMATION info{};
            info.ThreadId = GetCurrentThreadId();
            info.ExceptionPointers = exception;
            const auto written = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(),
                file, MiniDumpNormal, &info, nullptr, nullptr );
            std::fprintf( stderr, "Graphics crash dump %s (Win32 error %lu).\n",
                          written ? "written" : "failed", written ? 0ul : GetLastError() );
            CloseHandle( file );
        }
        else
        {
            std::fprintf( stderr, "Graphics crash dump could not be created (Win32 error %lu).\n",
                          GetLastError() );
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

    inline void install()
    {
        const auto length = GetEnvironmentVariableW( L"WP_GRAPHICS_TEST_CRASH_DUMP", dumpPath,
                                                     static_cast<DWORD>( std::size( dumpPath ) ) );
        if( length && length < std::size( dumpPath ) )
        {
            SetErrorMode( GetErrorMode() | SEM_NOGPFAULTERRORBOX );
            SetUnhandledExceptionFilter( writeDump );
        }
    }
}
