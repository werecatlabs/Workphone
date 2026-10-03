#ifndef __WP_MemoryTracker_h__
#define __WP_MemoryTracker_h__

#include <Workphone/WorkphoneConfig.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentHashMap.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <unordered_map>

namespace workphone
{
#if WP_ENABLE_MEMORY_TRACKER

    /**
     * @class MemoryTracker
     * @brief Tracks dynamic memory allocations and deallocations for debugging.
     *
     * MemoryTracker records each allocation made via the engine's memory
     * subsystem and can report statistics and leaks on demand or at exit.
     *
     * This class is intended for use in debug builds only (guarded by
     * WP_ENABLE_MEMORY_TRACKER). It is used by the memory subsystem and
     * is thread-safe: internal data is protected by a recursive mutex.
     *
     * Typical usage:
     *  - The memory allocation routines call `_recordAlloc` when memory is
     *    allocated and `_recordDealloc` when memory is freed.
     *  - Call `reportLeaks()` to print or write a leak report.
     *
     * @note The implementation keeps per-pointer records; avoid calling the
     * same pointer allocation routine twice without an intervening dealloc.
     * @threadsafe All public methods lock `m_mutex` where necessary.
     */
    class WPCore_API MemoryTracker
    {
    public:
        /**
         * @brief Internal record for a single allocation.
         *
         * Stores the allocation size, originating pool and source location.
         */
        struct Alloc
        {
            /** Default constructor - creates an empty record. */
            Alloc();

            /**
             * @brief Construct an allocation record.
             * @param sz Size of the allocation in bytes.
             * @param p  Pool id for the allocation.
             * @param file Source filename (may be nullptr).
             * @param ln Source line number.
             * @param func Source function name (may be nullptr).
             */
            Alloc( u32 sz, u32 p, const c8 *file, u32 ln, const c8 *func );

            u32 pool;        /**< Memory pool identifier. */
            size_t bytes;    /**< Size of the allocation in bytes. */
            size_t line;     /**< Source line number where allocation occurred. */
            String filename; /**< Source filename (empty if unknown). */
            String function; /**< Source function (empty if unknown). */
        };

        /** Default constructor. Initializes internal counters and containers. */
        MemoryTracker();

        /** Destructor. May emit a report depending on configuration. */
        ~MemoryTracker();

        /**
         * @brief Check for outstanding allocations and report memory leaks.
         *
         * If any allocations remain recorded (i.e. were not deallocated)
         * this function will write a formatted leak report either to the
         * configured report file, to stdout/stderr or both depending on
         * the configuration set via `setReportFileName` and
         * `setReportToStdOut`.
         *
         * This function is safe to call from multiple threads.
         */
        void reportLeaks();

        /**
         * @brief Set the filename used to write the leak report on exit.
         * @param name File path where the report will be written. If empty,
         *             no file will be produced.
         */
        void setReportFileName( const String &name );

        /**
         * @brief Get the filename that will be used for the leak report.
         * @return The configured report filename as a `String`. May be empty.
         */
        String getReportFileName() const;

        /**
         * @brief Enable or disable writing the memory report to stdout.
         * @param rep true to also dump the report to stdout; false otherwise.
         *
         * When enabled the report will be duplicated to stdout in addition
         * to any file output configured via `setReportFileName`.
         */
        void setReportToStdOut( bool rep );

        /**
         * @brief Query whether report output is sent to stdout.
         * @return true if reports are written to stdout.
         */
        bool getReportToStdOut() const;

        /**
         * @brief Get total number of bytes currently allocated (tracked).
         * @return Current total allocated bytes across all pools.
         */
        u32 getTotalMemoryAllocated() const;

        /**
         * @brief Get number of bytes allocated for a specific memory pool.
         * @param pool ID/index of the memory pool.
         * @return Allocated bytes for the given pool (0 if pool out of range).
         */
        u32 getMemoryAllocatedForPool( u32 pool ) const;

        /**
         * @brief Record an allocation event.
         *
         * This method should be called by the memory management subsystem
         * whenever a new allocation is created. The tracker stores the size,
         * pool, source file, line number and function name for later reporting.
         *
         * @param ptr Pointer returned by the allocation. Must be unique for
         *            active allocations.
         * @param sz Size of the allocation in bytes.
         * @param pool Memory pool identifier (defaults to 0).
         * @param file Source filename where allocation occurred (optional).
         * @param ln Source line number where allocation occurred (optional).
         * @param func Source function name where allocation occurred (optional).
         *
         * @note If `ptr` is nullptr this function will return without recording.
         * @threadsafe This method locks the internal mutex.
         */
        void recordAlloc( void *ptr, u32 sz, u32 pool = 0, const c8 *file = nullptr, u32 ln = 0,
                          const c8 *func = nullptr );

        /**
         * @brief Record a deallocation event.
         *
         * Removes the allocation record for `ptr` and updates accounting.
         * If `ptr` is not found (double-free or non-tracked allocation) the
         * call is ignored.
         *
         * @param ptr Pointer being deallocated.
         * @threadsafe This method locks the internal mutex.
         */
        void recordDealloc( void *ptr );

        /**
         * @brief Enable or disable recording of allocations/deallocations.
         *
         * When recording is disabled (false) calls to `_recordAlloc` /
         * `_recordDealloc` become no-ops. Use this to temporarily suspend
         * tracking during operations that must not be recorded.
         *
         * @param recordEnable true to enable recording; false to disable.
         */
        void setRecordEnable( bool recordEnable );

        /**
         * @brief Query whether allocation/deallocation recording is enabled.
         * @return true if recording is enabled.
         */
        bool getRecordEnable() const;

        /**
         * @brief Get the global MemoryTracker instance.
         *
         * Returns a reference to the singleton MemoryTracker used by the
         * memory subsystem. The instance is created the first time this is
         * called and lives until program exit.
         *
         * @return Reference to the global MemoryTracker.
         */
        static MemoryTracker &get();

    protected:
        atomic_u32 m_totalAllocations = 0; /**< Total currently allocated bytes tracked. */

        atomic_bool m_recordEnable = true;  /**< When false, allocation events are ignored. */
        atomic_bool m_dumpToStdOut = false; /**< When true, reports are also written to stdout. */

        AtomicObject<String> m_fileName; /**< Report file name (if non-empty, used on report). */

        ConcurrentHashMap<void *, Alloc> m_allocations; /**< Active allocations keyed by pointer. */
        ConcurrentArray<u32> m_allocationsByPool;       /**< Aggregated allocation sizes per pool. */
    };

#endif
}  // namespace workphone

#endif  // MemoryTracker_h__
