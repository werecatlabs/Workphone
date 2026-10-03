#ifndef __ParticleEmitter_h__
#define __ParticleEmitter_h__

#include <Workphone/Interface/Graphics/IParticleEmitter.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @enum EmitterType
         * @brief Defines the shape of the emission volume for a particle emitter.
         */
        enum class EmitterType
        {
            Point = 0,  ///< Emit from a single point
            Box,        ///< Emit from within a box volume
            Sphere,     ///< Emit from within or on a sphere
            Cone,       ///< Emit from within a cone
            Mesh        ///< Emit from mesh surface (requires mesh data)
        };

        /**
         * @class ParticleEmitter
         * @brief Concrete implementation of the IParticleEmitter interface for controlling particle
         * emission.
         *
         * The ParticleEmitter class manages the emission of particles in a particle system.
         * It provides control over various emission parameters such as particle size, direction,
         * emission rate, time to live, and velocity. This class allows for both fixed and ranged
         * values for these parameters, enabling flexible and dynamic particle effects.
         *
         * Features:
         * - Multiple emitter shapes (point, box, sphere, cone, mesh)
         * - Continuous and burst emission modes
         * - Parameter randomization (velocity, size, TTL, direction)
         * - Spread angle control for cone-shaped emission
         * - Emission limits (finite or infinite)
         * - Tangent/binormal basis for oriented emission
         *
         * @see IParticleEmitter
         */
        class WPCore_API ParticleEmitter : public IParticleEmitter
        {
        public:
            ParticleEmitter();

            /**
             * @brief Destructor for ParticleEmitter.
             */
            ~ParticleEmitter() override;

            /**
             * @brief Updates the state of the particle emitter.
             *
             * This method should be called every frame to update emission logic and internal state.
             * It handles both continuous and burst emission modes.
             */
            void update() override;

            /**
             * @brief Gets the default size of particles emitted.
             * @return The default particle size as a Vector3.
             */
            Vector3<real_Num> getParticleSize() const override;

            /**
             * @brief Sets the default size of particles emitted.
             * @param size The new default particle size.
             */
            void setParticleSize( const Vector3<real_Num> &size ) override;

            /**
             * @brief Gets the minimum size of particles emitted.
             * @return The minimum particle size as a Vector3.
             */
            Vector3<real_Num> getParticleSizeMin() const override;

            /**
             * @brief Sets the minimum size of particles emitted.
             * @param minSize The minimum particle size.
             */
            void setParticleSizeMin( const Vector3<real_Num> &minSize ) override;

            /**
             * @brief Gets the maximum size of particles emitted.
             * @return The maximum particle size as a Vector3.
             */
            Vector3<real_Num> getParticleSizeMax() const override;

            /**
             * @brief Sets the maximum size of particles emitted.
             * @param maxSize The maximum particle size.
             */
            void setParticleSizeMax( const Vector3<real_Num> &maxSize ) override;

            /**
             * @brief Gets the emission direction of the particles.
             * @return The direction vector for emitted particles.
             */
            Vector3<real_Num> getDirection() const override;

            /**
             * @brief Sets the emission direction of the particles.
             * @param direction The direction vector for emitted particles.
             */
            void setDirection( const Vector3<real_Num> &direction ) override;

            /**
             * @brief Gets the default emission rate (particles per second).
             * @return The emission rate.
             */
            f32 getEmissionRate() const override;

            /**
             * @brief Sets the default emission rate (particles per second).
             * @param rate The emission rate.
             */
            void setEmissionRate( f32 rate ) override;

            /**
             * @brief Gets the minimum emission rate (particles per second).
             * @return The minimum emission rate.
             */
            f32 getEmissionRateMin() const override;

            /**
             * @brief Sets the minimum emission rate (particles per second).
             * @param minRate The minimum emission rate.
             */
            void setEmissionRateMin( f32 minRate ) override;

            /**
             * @brief Gets the maximum emission rate (particles per second).
             * @return The maximum emission rate.
             */
            f32 getEmissionRateMax() const override;

            /**
             * @brief Sets the maximum emission rate (particles per second).
             * @param maxRate The maximum emission rate.
             */
            void setEmissionRateMax( f32 maxRate ) override;

            /**
             * @brief Gets the default time to live for emitted particles (in seconds).
             * @return The time to live.
             */
            f32 getTimeToLive() const override;

            /**
             * @brief Sets the default time to live for emitted particles (in seconds).
             * @param timeToLive The time to live.
             */
            void setTimeToLive( f32 timeToLive ) override;

            /**
             * @brief Gets the minimum time to live for emitted particles (in seconds).
             * @return The minimum time to live.
             */
            f32 getTimeToLiveMin() const override;

            /**
             * @brief Sets the minimum time to live for emitted particles (in seconds).
             * @param minTimeToLive The minimum time to live.
             */
            void setTimeToLiveMin( f32 minTimeToLive ) override;

            /**
             * @brief Gets the maximum time to live for emitted particles (in seconds).
             * @return The maximum time to live.
             */
            f32 getTimeToLiveMax() const override;

            /**
             * @brief Sets the maximum time to live for emitted particles (in seconds).
             * @param maxTimeToLive The maximum time to live.
             */
            void setTimeToLiveMax( f32 maxTimeToLive ) override;

            /**
             * @brief Gets the default velocity of emitted particles.
             * @return The velocity.
             */
            f32 getVelocity() const override;

            /**
             * @brief Sets the default velocity of emitted particles.
             * @param velocity The velocity.
             */
            void setVelocity( f32 velocity ) override;

            /**
             * @brief Gets the minimum velocity of emitted particles.
             * @return The minimum velocity.
             */
            f32 getVelocityMin() const override;

            /**
             * @brief Sets the minimum velocity of emitted particles.
             * @param minVelocity The minimum velocity.
             */
            void setVelocityMin( f32 minVelocity ) override;

            /**
             * @brief Gets the maximum velocity of emitted particles.
             * @return The maximum velocity.
             */
            f32 getVelocityMax() const override;

            /**
             * @brief Sets the maximum velocity of emitted particles.
             * @param maxVelocity The maximum velocity.
             */
            void setVelocityMax( f32 maxVelocity ) override;

            /**
             * @brief Gets the particle system associated with this emitter.
             * @return A smart pointer to the associated IParticleSystem.
             */
            SmartPtr<IParticleSystem> getParticleSystem() const override;

            /**
             * @brief Sets the particle system associated with this emitter.
             * @param particleSystem A smart pointer to the IParticleSystem to associate.
             */
            void setParticleSystem( SmartPtr<IParticleSystem> particleSystem ) override;

            // Additional configuration methods

            /**
             * @brief Sets burst emission parameters.
             * @param count Number of particles to emit per burst.
             * @param interval Time between bursts in seconds (0 for single burst).
             */
            void setEmissionBurst( u32 count, f32 interval );

            /**
             * @brief Gets the burst emission count.
             * @return Number of particles emitted per burst.
             */
            u32 getEmissionBurstCount() const;

            /**
             * @brief Gets the burst emission interval.
             * @return Time between bursts in seconds.
             */
            f32 getEmissionBurstInterval() const;

            /**
             * @brief Enables or disables the emitter.
             * @param enabled True to enable, false to disable.
             */
            void setEmitterEnabled( bool enabled );

            /**
             * @brief Checks if the emitter is enabled.
             * @return True if enabled, false otherwise.
             */
            bool isEmitterEnabled() const;

            /**
             * @brief Sets whether emission is infinite or limited.
             * @param infinite True for infinite emission, false for limited.
             */
            void setInfiniteEmission( bool infinite );

            /**
             * @brief Checks if emission is infinite.
             * @return True if infinite, false if limited.
             */
            bool isInfiniteEmission() const;

            /**
             * @brief Sets the total number of emissions (for limited emission).
             * @param total Total number of particles to emit (0 for infinite).
             */
            void setTotalEmissions( u32 total );

            /**
             * @brief Gets the total emission limit.
             * @return Total number of particles to emit.
             */
            u32 getTotalEmissions() const;

            /**
             * @brief Gets the current number of emissions.
             * @return Number of particles emitted so far.
             */
            u32 getCurrentEmissions() const;

            /**
             * @brief Resets the emission counter and accumulators.
             */
            void resetEmissions();

            /**
             * @brief Sets the emitter type (shape).
             * @param type The emitter type.
             */
            void setEmitterType( EmitterType type );

            /**
             * @brief Gets the emitter type.
             * @return The current emitter type.
             */
            EmitterType getEmitterType() const;

            /**
             * @brief Sets the spread angle for directional randomization.
             * @param angle Spread angle in degrees (0-180).
             */
            void setSpreadAngle( f32 angle );

            /**
             * @brief Gets the spread angle.
             * @return Spread angle in degrees.
             */
            f32 getSpreadAngle() const;

            /**
             * @brief Sets the spread angle variance (0-1).
             * @param variance Variance factor (0=no variance, 1=full variance).
             */
            void setSpreadAngleVariance( f32 variance );

            /**
             * @brief Gets the spread angle variance.
             * @return Variance factor.
             */
            f32 getSpreadAngleVariance() const;

            /**
             * @brief Sets the emission shape size.
             * @param size Size vector (interpretation depends on emitter type).
             */
            void setEmissionShapeSize( const Vector3<real_Num> &size );

            /**
             * @brief Gets the emission shape size.
             * @return Size vector.
             */
            Vector3<real_Num> getEmissionShapeSize() const;

            /**
             * @brief Sets the tangent vector for orientation.
             * @param tangent Tangent vector (will be normalized).
             */
            void setTangent( const Vector3<real_Num> &tangent );

            /**
             * @brief Gets the tangent vector.
             * @return Tangent vector.
             */
            Vector3<real_Num> getTangent() const;

            /**
             * @brief Sets the binormal vector for orientation.
             * @param binormal Binormal vector (will be normalized).
             */
            void setBinormal( const Vector3<real_Num> &binormal );

            /**
             * @brief Gets the binormal vector.
             * @return Binormal vector.
             */
            Vector3<real_Num> getBinormal() const;

            /**
             * @brief Enables or disables direction randomization.
             * @param randomize True to enable, false to disable.
             */
            void setRandomizeDirection( bool randomize );

            /**
             * @brief Checks if direction randomization is enabled.
             * @return True if enabled, false otherwise.
             */
            bool isRandomizeDirection() const;

            /**
             * @brief Enables or disables velocity randomization.
             * @param randomize True to enable, false to disable.
             */
            void setRandomizeVelocity( bool randomize );

            /**
             * @brief Checks if velocity randomization is enabled.
             * @return True if enabled, false otherwise.
             */
            bool isRandomizeVelocity() const;

            /**
             * @brief Enables or disables size randomization.
             * @param randomize True to enable, false to disable.
             */
            void setRandomizeSize( bool randomize );

            /**
             * @brief Checks if size randomization is enabled.
             * @return True if enabled, false otherwise.
             */
            bool isRandomizeSize() const;

            /**
             * @brief Enables or disables TTL (time-to-live) randomization.
             * @param randomize True to enable, false to disable.
             */
            void setRandomizeTTL( bool randomize );

            /**
             * @brief Checks if TTL randomization is enabled.
             * @return True if enabled, false otherwise.
             */
            bool isRandomizeTTL() const;

            WP_CLASS_REGISTER_DECL;

        private:
            /// The particle system this emitter is associated with.
            WeakPtr<IParticleSystem> m_particleSystem;

            /// Default particle size.
            Vector3<real_Num> m_particleSize;
            /// Minimum particle size.
            Vector3<real_Num> m_particleSizeMin;
            /// Maximum particle size.
            Vector3<real_Num> m_particleSizeMax;
            /// Direction of particle emission.
            Vector3<real_Num> m_direction;
            /// Default emission rate (particles per second).
            f32 m_emissionRate;
            /// Minimum emission rate (particles per second).
            f32 m_emissionRateMin;
            /// Maximum emission rate (particles per second).
            f32 m_emissionRateMax;
            /// Default time to live for particles (seconds).
            f32 m_timeToLive;
            /// Minimum time to live for particles (seconds).
            f32 m_timeToLiveMin;
            /// Maximum time to live for particles (seconds).
            f32 m_timeToLiveMax;
            /// Default velocity of particles.
            f32 m_velocity;
            /// Minimum velocity of particles.
            f32 m_velocityMin;
            /// Maximum velocity of particles.
            f32 m_velocityMax;

            /// Burst emission count.
            u32 m_emissionBurstCount;
            /// Burst emission interval (seconds).
            f32 m_emissionBurstInterval;
            /// Burst timer accumulator.
            f32 m_burstTimer;
            /// Emission accumulator for continuous mode.
            f32 m_emissionAccumulator = 0.0f;

            /// Emitter enabled flag.
            bool m_emitterEnabled;
            /// Infinite emission flag.
            bool m_infiniteEmission;
            /// Total emissions limit.
            u32 m_totalEmissions;
            /// Current emissions count.
            u32 m_currentEmissions;

            /// Emitter type (shape).
            EmitterType m_emitterType;
            /// Spread angle in degrees.
            f32 m_spreadAngle;
            /// Spread angle variance (0-1).
            f32 m_spreadAngleVariance;
            /// Emission shape size.
            Vector3<real_Num> m_emissionShapeSize;
            /// Tangent vector for orientation.
            Vector3<real_Num> m_tangent;
            /// Binormal vector for orientation.
            Vector3<real_Num> m_binormal;

            /// Randomization flags.
            bool m_randomizeDirection;
            bool m_randomizeVelocity;
            bool m_randomizeSize;
            bool m_randomizeTTL;

            /**
             * @brief Emits a single particle.
             * @param particleSystem The particle system to emit into.
             * @param timeToLive Particle lifetime.
             * @param velocity Particle velocity.
             */
            void emitParticle( SmartPtr<IParticleSystem> particleSystem, f32 timeToLive, f32 velocity );

            /**
             * @brief Emits a burst of particles.
             * @param count Number of particles to emit.
             * @param particleSystem The particle system to emit into.
             * @param timeToLive Particle lifetime.
             * @param velocity Particle velocity.
             */
            void emitBurst( u32 count, SmartPtr<IParticleSystem> particleSystem, f32 timeToLive,
                            f32 velocity );

            /**
             * @brief Calculates the emission position based on emitter type.
             * @return The calculated emission position.
             */
            Vector3<real_Num> calculateEmissionPosition() const;

            /**
             * @brief Calculates the emission direction with optional spread.
             * @return The calculated emission direction vector.
             */
            Vector3<real_Num> calculateEmissionDirection() const;

            /**
             * @brief Generates a random float between 0 and 1.
             * @return Random float value.
             */
            f32 getRandomFloat() const;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // ParticleEmitter_h__
