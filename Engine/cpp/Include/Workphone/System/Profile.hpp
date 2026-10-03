#ifndef Profile_h__
#define Profile_h__

#include <Workphone/Interface/System/IProfile.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{

    /**
     * @brief Profiling information for a unit of code.
     *
     * The `Profile` class stores timing and meta-data for a measured section of code.
     * It tracks start/end timestamps, last measured duration, running total and
     * a circular buffer of recent samples used to compute averaged timings.
     *
     * Thread-safety: internal state is protected by `m_mutex` where necessary.
     */
    class WPCore_API Profile : public IProfile
    {
    public:
        /** Default number of samples used to compute moving averages. */
        static const s32 DEFAULT_NUM_SAMPLES;

        /** Construct an empty `Profile`. */
        Profile();

        /** Virtual destructor. */
        ~Profile() override;

        /**
         * @brief Mark the beginning of the profiled section.
         *
         * Records a high-resolution start timestamp. Calling `start()` twice
         * without an intervening `end()` will overwrite the previous start time.
         */
        void start() override;

        /**
         * @brief Mark the end of the profiled section.
         *
         * Captures an end timestamp, updates `m_timeTaken`, `m_total` and the
         * average sample buffers.
         */
        void end() override;

        /**
         * @brief Get the profile description text.
         * @return A copy of the description string.
         */
        String getDescription() const override;

        /**
         * @brief Set the profile description.
         * @param description Description text (stored by value).
         */
        void setDescription( const String &description ) override;

        /**
         * @brief Get the time (in seconds) measured by the last `start()`/`end()` pair.
         * @return Last measured duration in seconds as `f32`.
         */
        f32 getTimeTaken() const;

        /**
         * @brief Manually set the last measured time.
         * @param timeTaken Duration in seconds to set as the last measured time.
         */
        void setTimeTaken( float timeTaken );

        /**
         * @brief Get the last delta time value.
         * @return Delta time as `f64`.
         */
        f64 getDeltaTime() const;

        /**
         * @brief Set the last delta time value.
         * @param timeTaken Delta time in seconds.
         */
        void setDeltaTime( f64 timeTaken );

        /**
         * @brief Retrieve the opaque user pointer associated with this profile.
         * @return The stored user data pointer (may be nullptr).
         */
        void *getUserData() const override;

        /**
         * @brief Associate an opaque user pointer with this profile.
         * @param userData Pointer to user-defined data (stored as-is).
         */
        void setUserData( void *userData ) override;

        /**
         * @brief Get the short label/name for this profile.
         * @return A copy of the label string.
         */
        String getLabel() const override;

        /**
         * @brief Set a short label/name for this profile.
         * @param label Label text (stored by value).
         */
        void setLabel( const String &label ) override;

        /**
         * @brief Get the moving average of recent measured durations.
         * @return Averaged duration in seconds as `f64`.
         */
        f64 getAverageTimeTaken() const override;

        /**
         * @brief Get the moving average of recent delta times.
         * @return Averaged delta time in seconds as `f64`.
         */
        f64 getAverageDeltaTime() const override;

        /**
         * @brief Reset timing samples and aggregates to their initial state.
         *
         * This does not change `m_label` or `m_description` nor `m_userData`.
         */
        void clear() override;

        /**
         * @brief Get the cumulative total time recorded by this profile.
         * @return Total time in seconds as `f64`.
         */
        f64 getTotal() const override;

        /**
         * @brief Manually set the cumulative total time for this profile.
         * @param total Total time in seconds.
         */
        void setTotal( f64 total ) override;

        void lock() override;

        bool try_lock() override;

        void unlock() override;

    protected:
        /** Circular buffer of recent measured durations (f32 seconds). */
        Array<f32> m_averageTimeTaken;

        /** Circular buffer of recent delta times (f64 seconds). */
        Array<f64> m_averageDeltaTimes;

        /** High-resolution start timestamp (platform-specific units). */
        u64 m_start = 0;

        /** High-resolution end timestamp (platform-specific units). */
        u64 m_end = 0;

        /** Accumulated total time for this profile. */
        time_interval m_total = time_interval( 0.0 );

        /** Last measured duration in seconds (f32). */
        f32 m_timeTaken = 0.0f;

        /** Last measured delta time (atomic for lock-free reads/writes). */
        atomic_f64 m_deltaTime;

        /** Next scheduled update time (used by profiler update logic). */
        f32 m_nextUpdate = 0.0f;

        /** Opaque user pointer associated with this profile (may be nullptr). */
        void *m_userData = nullptr;

        /** Longer human-readable description of this profile entry. */
        FixedString<32> m_description;

        /** Short label/name for display and lookup. */
        FixedString<32> m_label;

        /** Mutex protecting mutable state; marked mutable for const accessors. */
        mutable RecursiveSpinMutex m_mutex;

        /** Static counter used to generate unique profile IDs. */
        static u32 m_idExt;
    };

}  // namespace workphone

#endif  // Profile_h__
