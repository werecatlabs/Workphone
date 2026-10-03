#pragma once

// Opt-in diagnostics for the Debug test executable. Storage uses VirtualAlloc
// so collecting allocation stacks does not allocate recursively on the CRT heap.
#if defined( _WIN32 ) && defined( _DEBUG )
#    include <windows.h>
#    include <crtdbg.h>
#    include <cstdio>
#    include <cstdlib>
#    include <cstdint>
#    include <cstring>

namespace workphone::editor::tests
{
    namespace allocationDiagnostics
    {
        struct Record
        {
            void *frames[16];
            USHORT count;
            long request;
        };
        inline constexpr long capacity = 2000000;
        inline Record *records = nullptr;
        inline HANDLE report = INVALID_HANDLE_VALUE;
        inline thread_local bool recording = false;
        inline bool recordTail = false;
        inline size_t trackedSize = 0;

        inline int allocationHook( int operation, void *, size_t size, int blockType, long request,
                                   const unsigned char *, int )
        {
            if( records && !recording && operation == _HOOK_ALLOC && blockType != _CRT_BLOCK &&
                request > 0 && ( recordTail || request < capacity ) &&
                ( !trackedSize || size == trackedSize ) )
            {
                recording = true;
                auto &record = records[recordTail ? request % capacity : request];
                record.count = CaptureStackBackTrace( 1, 16, record.frames, nullptr );
                record.request = request;
                recording = false;
            }
            return TRUE;
        }

        inline void write( const char *text )
        {
            DWORD written = 0;
            WriteFile( report, text, static_cast<DWORD>( std::strlen( text ) ), &written, nullptr );
        }

        inline int reportHook( int, char *message, int * )
        {
            if( report == INVALID_HANDLE_VALUE )
                return FALSE;
            write( message );
            long request = 0;
            if( records && std::sscanf( message, "{%ld}", &request ) == 1 && request > 0 &&
                ( recordTail || request < capacity ) )
            {
                const auto &record = records[recordTail ? request % capacity : request];
                if( record.request != request )
                    return FALSE;
                for( USHORT frame = 0; frame < record.count; ++frame )
                {
                    HMODULE module = nullptr;
                    if( GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                           reinterpret_cast<LPCSTR>( record.frames[frame] ), &module ) )
                    {
                        char path[MAX_PATH] = {};
                        GetModuleFileNameA( module, path, MAX_PATH );
                        char line[MAX_PATH + 64] = {};
                        std::snprintf( line, sizeof( line ), "  %s+0x%llx\n", path,
                                       static_cast<unsigned long long>(
                                           reinterpret_cast<uintptr_t>( record.frames[frame] ) -
                                           reinterpret_cast<uintptr_t>( module ) ) );
                        write( line );
                    }
                }
            }
            return FALSE;
        }
    }

    inline void initialiseAllocationDiagnostics()
    {
        using namespace allocationDiagnostics;
        const char *path = std::getenv( "WP_EDITOR_ALLOCATION_REPORT" );
        if( !path || !*path )
            return;
        report = CreateFileA( path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr );
        if( report == INVALID_HANDLE_VALUE )
            return;
        records = static_cast<Record *>( VirtualAlloc( nullptr, sizeof( Record ) * capacity,
                                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE ) );
        write( "CRT allocation report: up to 2,000,000 allocation requests; 16 frames per request.\n" );
        if( const char *tail = std::getenv( "WP_EDITOR_ALLOCATION_TAIL" ) )
            recordTail = *tail == '1';
        if( recordTail )
            write( "Recording the most recent allocation requests in a ring buffer.\n" );
        if( const char *size = std::getenv( "WP_EDITOR_ALLOCATION_SIZE" ) )
            trackedSize = static_cast<size_t>( std::strtoull( size, nullptr, 10 ) );
        if( const char *request = std::getenv( "WP_EDITOR_BREAK_ALLOCATION" ) )
            _CrtSetBreakAlloc( std::strtol( request, nullptr, 10 ) );
        _CrtSetAllocHook( allocationHook );
        _CrtSetReportHook( reportHook );
        _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    }
}
#else
namespace workphone::editor::tests
{
    inline void initialiseAllocationDiagnostics() {}
}
#endif
