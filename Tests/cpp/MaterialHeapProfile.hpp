#ifndef WORKPHONE_TEST_MATERIAL_HEAP_PROFILE_HPP
#define WORKPHONE_TEST_MATERIAL_HEAP_PROFILE_HPP

// Test-only Windows allocation measurement. The shared CRT heap contains the
// engine's aligned objects, native malloc allocations, standard containers,
// shared-pointer control blocks and backing slabs of the engine's object/string
// pools. Its busy bytes include retained slab/container capacity. This is a heap
// measurement, not a sum of requested payload sizes or a claim of attribution.
//
// Capture deliberately uses no dynamic storage and prints nothing while a heap
// is locked. Run it at quiescent checkpoints in a dedicated profiling process.
// We do not walk foreign heaps returned by GetProcessHeaps: their handles can be
// invalidated by their owners. Process/VM counters cover those heaps and other
// private allocations separately, without adding them to the CRT heap total.

#if !defined( _WIN32 )
#    error MaterialHeapProfile requires Windows memory accounting APIs.
#endif

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <malloc.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

#if defined( _MSC_VER )
#    pragma comment( lib, "psapi.lib" )
#endif

namespace material_heap_profile
{
    struct Snapshot
    {
        std::uint64_t heapBusyDataBytes = 0;
        std::uint64_t heapBusyOverheadBytes = 0;
        std::uint64_t heapFreeDataBytes = 0;
        std::uint64_t heapFreeOverheadBytes = 0;
        std::uint64_t heapBusyBlocks = 0;
        std::uint64_t heapFreeBlocks = 0;
        std::uint64_t heapRegionCount = 0;
        std::uint64_t privateCommitBytes = 0;
        std::uint64_t processLifetimePeakPrivateCommitBytes = 0;
        std::uint64_t workingSetBytes = 0;
        std::uint64_t processLifetimePeakWorkingSetBytes = 0;
        std::uint64_t vmPrivateCommittedBytes = 0;
        // MEM_RESERVE ranges only; does not include the committed part of a reservation.
        std::uint64_t vmPrivateReservedBytes = 0;

        std::uint64_t heapBusyBytes() const noexcept
        {
            return heapBusyDataBytes + heapBusyOverheadBytes;
        }

        std::uint64_t heapFreeBytes() const noexcept
        {
            return heapFreeDataBytes + heapFreeOverheadBytes;
        }
    };

    // Negative deltas are meaningful after publication/view/graph retirement.
    inline std::int64_t delta( std::uint64_t value, std::uint64_t baseline ) noexcept
    {
        return value >= baseline ? static_cast<std::int64_t>( value - baseline ) :
                                   -static_cast<std::int64_t>( baseline - value );
    }

    inline Snapshot capture()
    {
#if defined( _MSC_VER ) && !defined( _DLL )
        throw std::runtime_error( "material heap profile requires the shared /MD CRT" );
#endif
#if ( defined( WP_USE_TBB ) && WP_USE_TBB ) || \
    ( defined( WP_USE_ONETBB ) && WP_USE_ONETBB )
        throw std::runtime_error( "material heap profile does not cover the TBB allocator" );
#endif
        Snapshot result;
        PROCESS_MEMORY_COUNTERS_EX process{};
        process.cb = sizeof( process );
        if( !GetProcessMemoryInfo( GetCurrentProcess(),
                                  reinterpret_cast<PROCESS_MEMORY_COUNTERS *>( &process ),
                                  sizeof( process ) ) )
            throw std::runtime_error( "material heap profile GetProcessMemoryInfo failed" );
        result.privateCommitBytes = process.PrivateUsage;
        result.processLifetimePeakPrivateCommitBytes = process.PeakPagefileUsage;
        result.workingSetBytes = process.WorkingSetSize;
        result.processLifetimePeakWorkingSetBytes = process.PeakWorkingSetSize;

        const auto heap = reinterpret_cast<HANDLE>( _get_heap_handle() );
        if( !heap || heap == INVALID_HANDLE_VALUE || !HeapLock( heap ) )
            throw std::runtime_error( "material heap profile could not lock the CRT heap" );
        PROCESS_HEAP_ENTRY entry{};
        while( HeapWalk( heap, &entry ) )
        {
            if( ( entry.wFlags & PROCESS_HEAP_ENTRY_BUSY ) != 0 )
            {
                result.heapBusyDataBytes += entry.cbData;
                result.heapBusyOverheadBytes += entry.cbOverhead;
                ++result.heapBusyBlocks;
            }
            else if( ( entry.wFlags & PROCESS_HEAP_REGION ) != 0 )
            {
                ++result.heapRegionCount;
                // Region commitment is optional, and large heap blocks have no
                // region marker. PSAPI/VirtualQuery provide commitment instead.
            }
            else if( ( entry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE ) == 0 )
            {
                result.heapFreeDataBytes += entry.cbData;
                result.heapFreeOverheadBytes += entry.cbOverhead;
                ++result.heapFreeBlocks;
            }
        }
        const auto walkError = GetLastError();
        const auto unlocked = HeapUnlock( heap );
        if( !unlocked )
            throw std::runtime_error( "material heap profile could not unlock the CRT heap" );
        if( walkError != ERROR_NO_MORE_ITEMS )
            throw std::runtime_error( "material heap profile CRT heap enumeration was incomplete" );

        SYSTEM_INFO system{};
        GetSystemInfo( &system );
        auto address = reinterpret_cast<std::uintptr_t>( system.lpMinimumApplicationAddress );
        const auto maximum = reinterpret_cast<std::uintptr_t>( system.lpMaximumApplicationAddress );
        while( address <= maximum )
        {
            MEMORY_BASIC_INFORMATION region{};
            if( VirtualQuery( reinterpret_cast<const void *>( address ), &region, sizeof( region ) ) !=
                sizeof( region ) )
                throw std::runtime_error( "material heap profile VirtualQuery enumeration was incomplete" );
            if( region.Type == MEM_PRIVATE )
            {
                if( region.State == MEM_COMMIT ) result.vmPrivateCommittedBytes += region.RegionSize;
                if( region.State == MEM_RESERVE ) result.vmPrivateReservedBytes += region.RegionSize;
            }
            const auto start = reinterpret_cast<std::uintptr_t>( region.BaseAddress );
            if( region.RegionSize == 0 || start > ( std::numeric_limits<std::uintptr_t>::max )() - region.RegionSize )
                throw std::runtime_error( "material heap profile encountered an invalid VM region" );
            const auto next = start + region.RegionSize;
            if( next <= address )
                throw std::runtime_error( "material heap profile VM enumeration did not advance" );
            address = next;
        }
        return result;
    }

    struct Peaks
    {
        Snapshot maxima;
        std::size_t samples = 0;

        void observe( const Snapshot &value ) noexcept
        {
            // Component maxima may come from different checkpoints. They must
            // never be added together to manufacture a simultaneous peak.
#define WP_MATERIAL_PROFILE_MAXIMUM( field ) \
    if( value.field > maxima.field ) maxima.field = value.field
            WP_MATERIAL_PROFILE_MAXIMUM( heapBusyDataBytes );
            WP_MATERIAL_PROFILE_MAXIMUM( heapBusyOverheadBytes );
            WP_MATERIAL_PROFILE_MAXIMUM( heapFreeDataBytes );
            WP_MATERIAL_PROFILE_MAXIMUM( heapFreeOverheadBytes );
            WP_MATERIAL_PROFILE_MAXIMUM( heapBusyBlocks );
            WP_MATERIAL_PROFILE_MAXIMUM( heapFreeBlocks );
            WP_MATERIAL_PROFILE_MAXIMUM( heapRegionCount );
            WP_MATERIAL_PROFILE_MAXIMUM( privateCommitBytes );
            WP_MATERIAL_PROFILE_MAXIMUM( processLifetimePeakPrivateCommitBytes );
            WP_MATERIAL_PROFILE_MAXIMUM( workingSetBytes );
            WP_MATERIAL_PROFILE_MAXIMUM( processLifetimePeakWorkingSetBytes );
            WP_MATERIAL_PROFILE_MAXIMUM( vmPrivateCommittedBytes );
            WP_MATERIAL_PROFILE_MAXIMUM( vmPrivateReservedBytes );
#undef WP_MATERIAL_PROFILE_MAXIMUM
            const auto busy = value.heapBusyBytes();
            if( busy > sampledHeapBusyBytes ) sampledHeapBusyBytes = busy;
            ++samples;
        }

        std::uint64_t sampledHeapBusyBytes = 0;
    };

    inline void report( const char *phase, const Snapshot &value, const Snapshot &baseline )
    {
        std::cout << "material_heap_profile phase=" << phase
                  << " heap_busy_data_bytes=" << value.heapBusyDataBytes
                  << " heap_busy_overhead_bytes=" << value.heapBusyOverheadBytes
                  << " heap_busy_bytes=" << value.heapBusyBytes()
                  << " heap_busy_delta_bytes=" << delta( value.heapBusyBytes(), baseline.heapBusyBytes() )
                  << " heap_busy_blocks=" << value.heapBusyBlocks
                  << " heap_free_data_bytes=" << value.heapFreeDataBytes
                  << " heap_free_overhead_bytes=" << value.heapFreeOverheadBytes
                  << " heap_free_bytes=" << value.heapFreeBytes()
                  << " heap_free_blocks=" << value.heapFreeBlocks
                  << " heap_regions=" << value.heapRegionCount
                  << " private_commit_bytes=" << value.privateCommitBytes
                  << " private_commit_delta_bytes=" << delta( value.privateCommitBytes, baseline.privateCommitBytes )
                  << " process_lifetime_peak_private_commit_bytes=" << value.processLifetimePeakPrivateCommitBytes
                  << " working_set_bytes=" << value.workingSetBytes
                  << " process_lifetime_peak_working_set_bytes=" << value.processLifetimePeakWorkingSetBytes
                  << " vm_private_committed_bytes=" << value.vmPrivateCommittedBytes
                  << " vm_private_reserved_uncommitted_bytes=" << value.vmPrivateReservedBytes
                  << " heap_scope=shared_crt process_scope=all_private_commit"
                  << " vm_scope=mem_private_excludes_image_copy_on_write\n";
    }

    inline void reportPeaks( const char *label, const Peaks &peaks, const Snapshot &baseline )
    {
        std::cout << "material_heap_profile_peaks label=" << label << " checkpoint_samples=" << peaks.samples
                  << " sampled_heap_busy_peak_bytes=" << peaks.sampledHeapBusyBytes
                  << " sampled_heap_busy_peak_delta_bytes=" << delta( peaks.sampledHeapBusyBytes, baseline.heapBusyBytes() )
                  << " sampled_private_commit_peak_bytes=" << peaks.maxima.privateCommitBytes
                  << " sampled_vm_private_committed_peak_bytes=" << peaks.maxima.vmPrivateCommittedBytes
                  << " process_lifetime_peak_private_commit_bytes=" << peaks.maxima.processLifetimePeakPrivateCommitBytes
                  << " transient_heap_peak_measured=0\n";
    }

    inline void runSelfTest()
    {
        // A fixed pointer table keeps the probe's bookkeeping off the heap.
        // Enough allocations in each small size class exercise the LFH path
        // used by control blocks and container nodes on a normal release CRT.
        constexpr std::size_t smallSizes[] = { 16, 40, 80, 192 };
        constexpr std::size_t smallPerSize = 1024;
        constexpr std::size_t smallCount = smallPerSize * ( sizeof( smallSizes ) / sizeof( smallSizes[0] ) );
        constexpr std::size_t smallBytes = smallPerSize * ( 16 + 40 + 80 + 192 );
        // Every successful allocation is retired even if a later check fails.
        struct Probes
        {
            void *smallBlocks[smallCount]{};
            void *ordinary = nullptr;
            void *aligned = nullptr;
            void *virtualMemory = nullptr;
            ~Probes()
            {
                for( auto pointer : smallBlocks ) std::free( pointer );
                std::free( ordinary );
                _aligned_free( aligned );
                if( virtualMemory ) VirtualFree( virtualMemory, 0, MEM_RELEASE );
            }
        } probes;
        constexpr std::size_t ordinaryBytes = 2u * 1024u * 1024u;
        constexpr std::size_t alignedBytes = 2u * 1024u * 1024u;
        constexpr std::size_t virtualBytes = 8u * 1024u * 1024u;

        const auto beforeSmall = capture();
        std::size_t smallIndex = 0;
        for( auto bytes : smallSizes )
        {
            for( std::size_t i = 0; i < smallPerSize; ++i )
            {
                auto pointer = std::malloc( bytes );
                if( !pointer )
                    throw std::runtime_error( "material heap profile small-allocation self-test failed" );
                probes.smallBlocks[smallIndex++] = pointer;
                // Keep the allocations observable without adding any heap work.
                auto memory = static_cast<volatile unsigned char *>( pointer );
                memory[0] = 0x5a;
                memory[bytes - 1] = 0xa5;
            }
        }
        const auto liveSmall = capture();
        if( delta( liveSmall.heapBusyDataBytes, beforeSmall.heapBusyDataBytes ) <
                static_cast<std::int64_t>( smallBytes ) ||
            delta( liveSmall.heapBusyBlocks, beforeSmall.heapBusyBlocks ) <
                static_cast<std::int64_t>( smallCount ) )
            throw std::runtime_error( "material heap profile did not observe small/LFH allocations" );
        for( auto &pointer : probes.smallBlocks )
        {
            std::free( pointer );
            pointer = nullptr;
        }
        const auto retiredSmall = capture();
        // LFH metadata and free capacity may remain above the original baseline.
        // Require the known live blocks/bytes to disappear instead of requiring
        // that retained allocator commitment return to its pre-probe value.
        if( delta( liveSmall.heapBusyDataBytes, retiredSmall.heapBusyDataBytes ) <
                static_cast<std::int64_t>( smallBytes ) ||
            delta( liveSmall.heapBusyBlocks, retiredSmall.heapBusyBlocks ) <
                static_cast<std::int64_t>( smallCount ) )
            throw std::runtime_error( "material heap profile did not observe small/LFH probe retirement" );

        // Keep separate large/aligned/VirtualAlloc coverage: small-block heap
        // visibility alone cannot establish coverage of direct private commit.
        const auto before = capture();
        probes.ordinary = std::malloc( ordinaryBytes );
        probes.aligned = _aligned_malloc( alignedBytes, 256 );
        if( !probes.ordinary || !probes.aligned )
            throw std::runtime_error( "material heap profile self-test allocation failed" );
        const auto allocated = capture();
        if( delta( allocated.heapBusyDataBytes, before.heapBusyDataBytes ) <
                static_cast<std::int64_t>( ordinaryBytes + alignedBytes ) ||
            allocated.heapBusyBlocks < before.heapBusyBlocks + 2 )
            throw std::runtime_error( "material heap profile did not observe malloc/aligned allocations" );
        probes.virtualMemory = VirtualAlloc( nullptr, virtualBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
        if( !probes.virtualMemory )
            throw std::runtime_error( "material heap profile VirtualAlloc self-test failed" );
        const auto committed = capture();
        if( delta( committed.vmPrivateCommittedBytes, allocated.vmPrivateCommittedBytes ) <
                static_cast<std::int64_t>( virtualBytes ) ||
            delta( committed.privateCommitBytes, allocated.privateCommitBytes ) <
                static_cast<std::int64_t>( virtualBytes ) )
            throw std::runtime_error( "material heap profile did not observe nonheap private commitment" );
        if( !VirtualFree( probes.virtualMemory, 0, MEM_RELEASE ) )
            throw std::runtime_error( "material heap profile VirtualFree self-test failed" );
        probes.virtualMemory = nullptr;
        std::free( probes.ordinary ); probes.ordinary = nullptr;
        _aligned_free( probes.aligned ); probes.aligned = nullptr;
        const auto retired = capture();
        if( delta( allocated.heapBusyDataBytes, retired.heapBusyDataBytes ) <
                static_cast<std::int64_t>( ordinaryBytes + alignedBytes ) ||
            delta( committed.vmPrivateCommittedBytes, retired.vmPrivateCommittedBytes ) <
                static_cast<std::int64_t>( virtualBytes ) )
            throw std::runtime_error( "material heap profile did not observe probe retirement" );
    }
}  // namespace material_heap_profile

#endif
