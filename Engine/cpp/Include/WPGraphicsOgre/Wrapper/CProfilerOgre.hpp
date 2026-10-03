#ifndef __CProfilerOgre_h__
#define __CProfilerOgre_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/System/IProfiler.hpp>

namespace workphone
{
    namespace render
    {
        /** A profiler implementation using the Ogre 3D graphics engine.
         */
        class CProfilerOgre : public IProfiler
        {
        public:
            /** Constructor. */
            CProfilerOgre();

            /** Destructor. */
            ~CProfilerOgre() override;

            /** Update the profiler.
             *  @param t The time since the last update.
             *  @param dt The change in time since the last update.
             */
            void update( float t, float dt );

            /** Adds a profile with a generated ID.
             *  @return A new profile instance.
             */
            SmartPtr<IProfile> addProfile();

            /** Adds a profile with a specified ID.
             *  @param id The ID for the new profile.
             *  @return A new profile instance.
             */
            SmartPtr<IProfile> addProfile( hash32 id );

            /** Removes a profile from the profiler.
             *  @param profile The profile to remove.
             */
            void removeProfile( SmartPtr<IProfile> profile ) override;

            /** Returns whether or not the profiler is visible.
             *  @return True if visible, false if hidden.
             */
            bool isVisible() const;

            /** Sets the visibility of the profiler.
             *  @param visible True to make the profiler visible, false to hide it.
             */
            void setVisible( bool visible );

            /** Returns a profile by its ID.
             *  @param id The ID of the profile to retrieve.
             *  @return The profile with the specified ID, or null if not found.
             */
            SmartPtr<IProfile> getProfile( hash32 id );

            /** Starts profiling for the specified name.
             *  @param name The name of the profile to start.
             */
            void start( const String &name );

            /** Ends profiling for the specified name.
             *  @param name The name of the profile to end.
             */
            void end( const String &name );

            /** Logs the profiling results to the console. */
            void logResults() override;

        protected:
            /** The collection of profiles being tracked by the profiler. */
            ConcurrentArray<SmartPtr<IProfile>> m_profiles;

            /** The physics plot. */
            Plot *m_physicsPlot;

            /** The overlay. */
            Ogre::Overlay *m_overlay;

            /** The next time an update should be performed. */
            float m_nextUpdate = 0.0f;

            /** The profiler instance. */
            Ogre::Profiler *m_profiler;
        };
    }  // namespace render
}  // namespace workphone

#endif  // __CProfilerOgre_h__
