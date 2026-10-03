#ifndef __ParticleSystem_h__
#define __ParticleSystem_h__

#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Graphics/GraphicsObject.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class ParticleSystem
         * @brief Represents a particle system object for managing and simulating particle effects.
         *
         * The ParticleSystem class provides an interface for creating, updating, and controlling
         * particle-based visual effects. It supports techniques, templates, scaling, fast-forwarding,
         * and state management for advanced particle system behaviors.
         */
        class WPCore_API ParticleSystem : public GraphicsObject<IParticleSystem>
        {
        public:
            static const String TemplateNameStr;
            static const String ScaleStr;
            static const String StateStr;
            static const String FastForwardTimeStr;
            static const String FastForwardIntervalStr;
            static const String StartLifetimeStr;
            static const String StartSizeStr;
            static const String RateStr;
            static const String RateVarianceStr;
            static const String AngleStr;
            static const String AngleVarianceStr;
            static const String ShapeTypeStr;
            static const String ShapeSizeStr;
            static const String ShapeSizeVarianceStr;
            static const String DurationStr;
            static const String LoopingStr;

            /**
             * @brief Constructs a new ParticleSystem instance.
             */
            ParticleSystem();

            /**
             * @brief Destroys the ParticleSystem instance and releases resources.
             */
            ~ParticleSystem() override;

            /**
             * @brief Loads the particle system with the specified data.
             * @param data Shared object containing initialization data.
             * @copydoc IParticleSystem::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the particle system and releases associated resources.
             * @param data Shared object containing unload data.
             * @copydoc IParticleSystem::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reloads the particle system with the specified data.
             * @param data Shared object containing reload data.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the particle system state, advancing simulation and rendering.
             */
            void update() override;

            /**
             * @brief Fast-forwards the particle system simulation.
             * @param time The total time to fast-forward.
             * @param interval The simulation step interval.
             */
            void setFastForward( f32 time, f32 interval ) override;

            /**
             * @brief Gets the total fast-forward time set for the simulation.
             * @return The fast-forward time in seconds.
             */
            f32 getFastForwardTime() const override;

            /**
             * @brief Gets the interval used for fast-forward simulation steps.
             * @return The fast-forward interval in seconds.
             */
            f32 getFastForwardInterval() const override;

            /**
             * @brief Sets the template name used for this particle system.
             * @param templateName The name of the particle system template.
             */
            void setTemplateName( const String &templateName ) override;

            /**
             * @brief Gets the template name used by this particle system.
             * @return The template name as a String.
             */
            String getTemplateName() const override;

            /**
             * @brief Sets the scale of the particle system.
             * @param scale The scale vector to apply.
             */
            void setScale( const Vector3<real_Num> &scale ) override;

            /**
             * @brief Gets the current scale of the particle system.
             * @return The scale as a Vector3.
             */
            Vector3<real_Num> getScale() const override;

            /**
             * @brief Gets the current state of the particle system.
             * @return The state as an unsigned integer.
             */
            ParticleSystemState getState() const override;

            /**
             * @brief Sets the state of the particle system.
             * @param state The new state value.
             */
            void setState( ParticleSystemState state ) override;

            size_t getNumParticles() const override;

            size_t getNumEmitters() const override;

            SmartPtr<Properties> getProperties() const override;

            void setProperties( SmartPtr<Properties> properties ) override;

            Vector2<real_Num> getStartLifetime() const;

            void setStartLifetime( const Vector2<real_Num> &startLifetime );

            Vector2<real_Num> getStartSize() const;

            void setStartSize( const Vector2<real_Num> &startSize );

            f32 getRate() const;

            void setRate( f32 rate );

            f32 getRateVariance() const;

            void setRateVariance( f32 rateVariance );

            f32 getAngle() const;

            void setAngle( f32 angle );

            f32 getAngleVariance() const;

            void setAngleVariance( f32 angleVariance );

            f32 getShapeType() const;

            void setShapeType( f32 shapeType );

            f32 getShapeSize() const;

            void setShapeSize( f32 shapeSize );

            f32 getShapeSizeVariance() const;

            void setShapeSizeVariance( f32 shapeSizeVariance );

            f32 getDuration() const;

            void setDuration( f32 duration );

            bool getLooping() const;

            void setLooping( bool looping );

            u32 getNumTechniques() const;

            /**
             * @brief Adds a new particle technique to the system.
             * @return A smart pointer to the created IParticleTechnique.
             */
            SmartPtr<IParticleTechnique> addTechnique() override;

            /**
             * @brief Adds a new particle technique with a specific name.
             * @param name The name of the technique.
             * @return A smart pointer to the created IParticleTechnique.
             */
            SmartPtr<IParticleTechnique> addTechnique( const String &name ) override;

            /**
             * @brief Removes a particle technique from the system.
             * @param technique The technique to remove.
             */
            void removeTechnique( SmartPtr<IParticleTechnique> technique ) override;

            /**
             * @brief Retrieves a particle technique by name.
             * @param name The name of the technique.
             * @return A smart pointer to the found IParticleTechnique, or nullptr if not found.
             */
            SmartPtr<IParticleTechnique> getTechnique( const String &name ) const override;

            /**
             * @brief Adds a new particle to the system.
             * @return A smart pointer to the created IParticle.
             */
            SmartPtr<IParticle> addParticle() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief List of particle techniques used by this system.
             */
            Array<SmartPtr<IParticleTechnique>> m_techniques;

            /**
             * @brief List of particles managed by this system.
             */
            Array<SmartPtr<IParticle>> m_particles;

            /**
             * @brief Range for the start lifetime of particles.
             */
            Vector2<real_Num> m_startLifetime = Vector2<real_Num>( 1.0f, 5.0f );

            /**
             * @brief Range for the start size of particles.
             */
            Vector2<real_Num> m_startSize = Vector2<real_Num>( 1.0f, 1.0f );

            /**
             * @brief Scale applied to the particle system.
             */
            Vector3<real_Num> m_scale = Vector3<real_Num>( 1.0f, 1.0f, 1.0f );

            /**
             * @brief Emission rate of particles per second.
             */
            f32 m_rate = 5.0f;

            /**
             * @brief Variance in the emission rate.
             */
            f32 m_rateVariance = 0.0f;

            /**
             * @brief Emission angle in degrees.
             */
            f32 m_angle = 0.0f;

            /**
             * @brief Variance in the emission angle.
             */
            f32 m_angleVariance = 0.0f;

            /**
             * @brief Type of the emission shape.
             */
            f32 m_shapeType = 0;

            /**
             * @brief Size of the emission shape.
             */
            f32 m_shapeSize = 0.0f;

            /**
             * @brief Variance in the emission shape size.
             */
            f32 m_shapeSizeVariance = 0.0f;

            /**
             * @brief Duration of the particle system in seconds.
             */
            f32 m_duration = 5.0f;

            /**
             * @brief Time to fast-forward the simulation.
             */
            f32 m_fastForwardTime = 0.0f;

            /**
             * @brief Interval for fast-forward simulation steps.
             */
            f32 m_fastForwardInterval = 0.0f;

            /**
             * @brief Indicates whether the particle system should loop after finishing.
             */
            ParticleSystemState m_state = ParticleSystemState::Stopped;

            /**
             * @brief Indicates whether the particle system loops after finishing.
             */
            bool m_looping = true;

            /**
             * @brief Name of the template used for this particle system.
             */
            String m_templateName;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ParticleSystem_h__
