#ifndef CProfileOgre_h__
#define CProfileOgre_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/System/IProfile.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class CProfileOgre
         * @brief Lightweight profiler entry used by the Ogre renderer wrapper.
         *
         * This class implements the IProfile interface and records a start and end
         * time for a profiling span. It stores a textual label and description and
         * can compute the elapsed time between the start and end calls.
         *
         * Typical usage:
         * - call `start()` at the beginning of the profiled section
         * - call `end()` at the end of the profiled section
         * - query label/description via `getLabel()`/`getDescription()`
         * - obtain elapsed time with `getTimeTaken()`
         *
         * The class does not manage any external resources and is intended to be
         * small and copyable/movable depending on surrounding code conventions.
         */
        class CProfileOgre : public IProfile
        {
        public:
            /** @brief Construct an empty profile entry. */
            CProfileOgre();
            /** @brief Virtual destructor. */
            ~CProfileOgre() override;

            /**
             * @brief Mark the beginning of the profiled period.
             *
             * Records the current time into the internal start time field.
             */
            void start() override;

            /**
             * @brief Mark the end of the profiled period.
             *
             * Records the current time into the internal end time field and updates
             * the stored elapsed time value.
             */
            void end() override;

            /**
             * @brief Get a human readable description for the profile entry.
             * @return The description string.
             */
            String getDescription() const override;

            /**
             * @brief Set a human readable description for the profile entry.
             * @param description A textual description explaining the profiled work.
             */
            void setDescription( const String &description ) override;

            /**
             * @brief Get the time taken (end - start) for this profile entry.
             * @return Time interval representing elapsed time between start and end.
             *
             * If `end()` has not been called yet this may return a value based on
             * the last recorded times; callers should ensure `start()`/`end()` were
             * invoked appropriately.
             */
            time_interval getTimeTaken() const;

            /**
             * @brief Set the stored time taken value explicitly.
             * @param timeTaken Time interval to set as the elapsed time.
             *
             * This setter allows external code to provide a precomputed elapsed
             * time instead of using the internally recorded start/end times.
             */
            void setTimeTaken( time_interval timeTaken );

            /**
             * @brief Get the short label/name for this profile entry.
             * @return The label string.
             */
            String getLabel() const override;

            /**
             * @brief Set the short label/name for this profile entry.
             * @param label Short identifier for the profiled section.
             */
            void setLabel( const String &label ) override;

        protected:
            /** @brief Time recorded when `start()` is called. */
            time_interval m_start;

            /** @brief Time recorded when `end()` is called. */
            time_interval m_end;

            /** @brief Cached elapsed time (end - start). */
            time_interval m_timeTaken;

            /**
             * @brief Next update time used by any throttled update logic.
             *
             * Not used directly by the public interface but kept for consumers that
             * rely on periodic profiling updates.
             */
            time_interval m_nextUpdate;

            /** @brief Longer, descriptive text explaining the profiled section. */
            String m_description;

            /** @brief Short label or name for this profile entry. */
            String m_label;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CProfileOgre_h__
