#ifndef IParticleEmitter_h__
#define IParticleEmitter_h__

#include <Workphone/Interface/Graphics/IParticleNode.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class IParticleEmitter
         * @brief Interface class for a particle emitter, which is responsible for controlling the
         * emission of particles in a rendering system.
         *
         * This interface provides methods to manage various parameters of the particle emitter, such as
         * particle size, direction, emission rate, velocity, and time-to-live.
         */
        class WPCore_API IParticleEmitter : public IParticleNode
        {
        public:
            /**
             * @brief Destructor.
             */
            ~IParticleEmitter() override;

            /**
             * @brief Gets the default size of particles emitted.
             * @return A Vector3 representing the size of the particles.
             */
            virtual Vector3<real_Num> getParticleSize() const = 0;

            /**
             * @brief Sets the default size of particles emitted.
             * @param size The new particle size as a Vector3.
             */
            virtual void setParticleSize( const Vector3<real_Num> &size ) = 0;

            /**
             * @brief Gets the minimum size of particles emitted.
             * @return A Vector3 representing the minimum size of the particles.
             */
            virtual Vector3<real_Num> getParticleSizeMin() const = 0;

            /**
             * @brief Sets the minimum size of particles emitted.
             * @param minSize The new minimum particle size as a Vector3.
             */
            virtual void setParticleSizeMin( const Vector3<real_Num> &minSize ) = 0;

            /**
             * @brief Gets the maximum size of particles emitted.
             * @return A Vector3 representing the maximum size of the particles.
             */
            virtual Vector3<real_Num> getParticleSizeMax() const = 0;

            /**
             * @brief Sets the maximum size of particles emitted.
             * @param maxSize The new maximum particle size as a Vector3.
             */
            virtual void setParticleSizeMax( const Vector3<real_Num> &maxSize ) = 0;

            /**
             * @brief Gets the direction of particles emitted.
             * @return A Vector3 representing the direction of the emitted particles.
             */
            virtual Vector3<real_Num> getDirection() const = 0;

            /**
             * @brief Sets the direction of particles emitted.
             * @param direction The new direction for emitted particles as a Vector3.
             */
            virtual void setDirection( const Vector3<real_Num> &direction ) = 0;

            /**
             * @brief Gets the emission rate of particles.
             * @return The emission rate as a floating-point value.
             */
            virtual f32 getEmissionRate() const = 0;

            /**
             * @brief Sets the emission rate of particles.
             * @param rate The new emission rate as a floating-point value.
             */
            virtual void setEmissionRate( f32 rate ) = 0;

            /**
             * @brief Gets the minimum emission rate of particles.
             * @return The minimum emission rate as a floating-point value.
             */
            virtual f32 getEmissionRateMin() const = 0;

            /**
             * @brief Sets the minimum emission rate of particles.
             * @param minRate The new minimum emission rate as a floating-point value.
             */
            virtual void setEmissionRateMin( f32 minRate ) = 0;

            /**
             * @brief Gets the maximum emission rate of particles.
             * @return The maximum emission rate as a floating-point value.
             */
            virtual f32 getEmissionRateMax() const = 0;

            /**
             * @brief Sets the maximum emission rate of particles.
             * @param maxRate The new maximum emission rate as a floating-point value.
             */
            virtual void setEmissionRateMax( f32 maxRate ) = 0;

            /**
             * @brief Gets the time-to-live of particles emitted.
             * @return The time-to-live of the particles as a floating-point value.
             */
            virtual f32 getTimeToLive() const = 0;

            /**
             * @brief Sets the time-to-live of particles emitted.
             * @param timeToLive The new time-to-live for particles as a floating-point value.
             */
            virtual void setTimeToLive( f32 timeToLive ) = 0;

            /**
             * @brief Gets the minimum time-to-live of particles emitted.
             * @return The minimum time-to-live of the particles as a floating-point value.
             */
            virtual f32 getTimeToLiveMin() const = 0;

            /**
             * @brief Sets the minimum time-to-live of particles emitted.
             * @param minTimeToLive The new minimum time-to-live for particles as a floating-point value.
             */
            virtual void setTimeToLiveMin( f32 minTimeToLive ) = 0;

            /**
             * @brief Gets the maximum time-to-live of particles emitted.
             * @return The maximum time-to-live of the particles as a floating-point value.
             */
            virtual f32 getTimeToLiveMax() const = 0;

            /**
             * @brief Sets the maximum time-to-live of particles emitted.
             * @param maxTimeToLive The new maximum time-to-live for particles as a floating-point value.
             */
            virtual void setTimeToLiveMax( f32 maxTimeToLive ) = 0;

            /**
             * @brief Gets the velocity of particles emitted.
             * @return The velocity of the particles as a floating-point value.
             */
            virtual f32 getVelocity() const = 0;

            /**
             * @brief Sets the velocity of particles emitted.
             * @param velocity The new velocity for particles as a floating-point value.
             */
            virtual void setVelocity( f32 velocity ) = 0;

            /**
             * @brief Gets the minimum velocity of particles emitted.
             * @return The minimum velocity of the particles as a floating-point value.
             */
            virtual f32 getVelocityMin() const = 0;

            /**
             * @brief Sets the minimum velocity of particles emitted.
             * @param minVelocity The new minimum velocity for particles as a floating-point value.
             */
            virtual void setVelocityMin( f32 minVelocity ) = 0;

            /**
             * @brief Gets the maximum velocity of particles emitted.
             * @return The maximum velocity of the particles as a floating-point value.
             */
            virtual f32 getVelocityMax() const = 0;

            /**
             * @brief Sets the maximum velocity of particles emitted.
             * @param maxVelocity The new maximum velocity for particles as a floating-point value.
             */
            virtual void setVelocityMax( f32 maxVelocity ) = 0;

            /**
             * @brief Declares class registration for reflection systems.
             */
            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // IParticleEmitter_h__
