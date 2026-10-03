#ifndef __RuntimeSettings_h__
#define __RuntimeSettings_h__

#include <WPRuntimePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{
    /**
     * @brief Container for runtime configuration values used by the engine.
     *
     * RuntimeSettings holds configurable options such as the number of worker
     * threads, frame targets and individual update rates for subsystems
     * (physics, game logic, AI, traffic) and a runtime scheme identifier.
     *
     * The class exposes simple getters and setters to allow loading and
     * applying settings at startup or dynamically during execution.
     */
    class RuntimeSettings : ISharedObject
    {
    public:
        /**
         * @brief Construct a new RuntimeSettings object with sensible defaults.
         *
         * Default values should represent a safe runtime configuration when
         * explicit settings are not provided.
         */
        RuntimeSettings();

        /**
         * @brief Virtual destructor.
         *
         * Ensures correct cleanup when used via ISharedObject pointers.
         */
        ~RuntimeSettings() override;

        /**
         * @brief Get the associated properties object.
         *
         * The properties object (if set) may contain named key/value pairs used
         * to initialize or override individual runtime settings.
         *
         * @return SmartPtr<Properties> Smart pointer to the properties object or nullptr.
         */
        SmartPtr<Properties> getProperties() const;

        /**
         * @brief Set the properties object used by these runtime settings.
         *
         * Passing a null SmartPtr clears the associated properties.
         *
         * @param properties Smart pointer to a Properties instance.
         */
        void setProperties( SmartPtr<Properties> properties );

        /**
         * @brief Number of worker threads the runtime should use.
         *
         * This is typically used to size thread pools for background work and
         * subsystem parallelism. A value of 0 may indicate "auto" depending on
         * higher-level logic (caller must interpret).
         *
         * @return u32 Current configured number of threads.
         */
        u32 getNumThreads() const;

        /**
         * @brief Set the number of worker threads.
         *
         * @param numThreads Number of threads to use for runtime tasks.
         */
        void setNumThreads( u32 numThreads );

        /**
         * @brief Target frames per second for the runtime.
         *
         * Used by the main loop to throttle or measure frame pacing.
         *
         * @return u32 Target frames per second.
         */
        u32 getTargetFrames() const;

        /**
         * @brief Set the target frames per second.
         *
         * @param targetFrames Desired target FPS.
         */
        void setTargetFrames( u32 targetFrames );

        /**
         * @brief Physics subsystem update rate (in Hz or ticks per second).
         *
         * Specifies how frequently physics should be updated. Interpretation of
         * this value (Hz vs ticks) is determined by the physics subsystem code.
         *
         * @return u32 Physics update frequency.
         */
        u32 getPhysicsUpdate() const;

        /**
         * @brief Set the physics update frequency.
         *
         * @param physicsUpdate Update frequency for physics.
         */
        void setPhysicsUpdate( u32 physicsUpdate );

        /**
         * @brief Game logic update rate.
         *
         * Controls how often game logic ticks are processed.
         *
         * @return u32 Game logic update frequency.
         */
        u32 getGameLogicUpdate() const;

        /**
         * @brief Set the game logic update frequency.
         *
         * @param gameLogicUpdate Update frequency for game logic.
         */
        void setGameLogicUpdate( u32 gameLogicUpdate );

        /**
         * @brief AI update rate.
         *
         * How frequently AI systems are stepped; lower rates reduce CPU usage
         * at the cost of responsiveness.
         *
         * @return u32 AI update frequency.
         */
        u32 getAiUpdate() const;

        /**
         * @brief Set the AI update frequency.
         *
         * @param aiUpdate Update frequency for AI systems.
         */
        void setAiUpdate( u32 aiUpdate );

        /**
         * @brief Traffic simulation update rate.
         *
         * Used for traffic and crowd simulation subsystems.
         *
         * @return u32 Traffic update frequency.
         */
        u32 getTrafficUpdate() const;

        /**
         * @brief Set the traffic simulation update frequency.
         *
         * @param trafficUpdate Update frequency for traffic systems.
         */
        void setTrafficUpdate( u32 trafficUpdate );

        /**
         * @brief Get the runtime scheme identifier.
         *
         * The scheme string can be used to select platform-specific or
         * configuration-specific behavior (for example, "development",
         * "production", or custom platform names).
         *
         * @return String Scheme identifier.
         */
        String getScheme() const;

        /**
         * @brief Set the runtime scheme identifier.
         *
         * @param scheme A string identifying the active runtime scheme.
         */
        void setScheme( const String &scheme );

    protected:
        /// Number of worker threads to use for runtime tasks.
        u32 m_numThreads;

        /// Target frames per second (frame pacing target).
        u32 m_targetFrames;

        /// Physics update frequency (Hz or ticks per second).
        u32 m_physicsUpdate;

        /// Game logic update frequency.
        u32 m_gameLogicUpdate;

        /// AI update frequency.
        u32 m_aiUpdate;

        /// Traffic simulation update frequency.
        u32 m_trafficUpdate;

        /// Runtime scheme identifier (e.g. "development", "production").
        String m_scheme;

        /// Last property set applied to this runtime settings object.
        SmartPtr<Properties> m_properties;
    };
}  // namespace workphone

#endif  // GameAppConfig_h__
