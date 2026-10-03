#ifndef CParticleEmitter_h__
#define CParticleEmitter_h__

#include <Workphone/Interface/Graphics/IParticleEmitter.hpp>
#include "WPGraphics/Particle/CParticleNode.hpp"

namespace workphone
{
    namespace render
    {
        template <class T>
        class CParticleEmitter : public CParticleNode<T>
        {
        public:
            CParticleEmitter();
            ~CParticleEmitter() override;

            virtual void calculateState( SmartPtr<IParticle> particle, u32 stateIndex,
                                         void *data = nullptr )
            {
            }

            void setTimeToLiveMax( f32 timeToLiveMax )
            {
                m_timeToLiveMax = timeToLiveMax;
            }

            f32 getTimeToLiveMax() const
            {
                return m_timeToLiveMax;
            }

            void setTimeToLiveMin( f32 timeToLiveMin )
            {
                m_timeToLiveMin = timeToLiveMin;
            }

            f32 getTimeToLiveMin() const
            {
                return m_timeToLiveMin;
            }

            void setTimeToLive( f32 timeToLive )
            {
                m_timeToLive = timeToLive;
            }

            f32 getTimeToLive() const
            {
                return m_timeToLive;
            }

            void setEmissionRate( f32 emissionRate )
            {
                m_emissionRate = emissionRate;
            }

            f32 getEmissionRate() const
            {
                return m_emissionRate;
            }

            f32 getEmissionRateMin() const
            {
                return m_emissionRateMin;
            }

            void setEmissionRateMin( f32 emissionRateMin )
            {
                m_emissionRateMin = emissionRateMin;
            }

            f32 getEmissionRateMax() const
            {
                return m_emissionRateMax;
            }

            void setEmissionRateMax( f32 emissionRateMax )
            {
                m_emissionRateMax = emissionRateMax;
            }

            void setDirection( const Vector3F &direction )
            {
                m_direction = direction;
            }

            Vector3<real_Num> getDirection() const
            {
                return m_direction;
            }

            void setParticleSize( const Vector3<real_Num> &particleSize )
            {
                m_particleSize = particleSize;
            }

            Vector3<real_Num> getParticleSize() const
            {
                return m_particleSize;
            }

            Vector3<real_Num> getParticleSizeMin() const
            {
                return m_particleSizeMin;
            }

            void setParticleSizeMin( const Vector3<real_Num> &particleSizeMin )
            {
                m_particleSizeMin = particleSizeMin;
            }

            Vector3<real_Num> getParticleSizeMax() const
            {
                return m_particleSizeMax;
            }

            void setParticleSizeMax( const Vector3<real_Num> &particleSizeMax )
            {
                m_particleSizeMax = particleSizeMax;
            }

            f32 getVelocity() const
            {
                return m_velocity;
            }

            void setVelocity( f32 velocity )
            {
                m_velocity = velocity;
            }

            f32 getVelocityMin() const
            {
                return m_velocityMin;
            }

            void setVelocityMin( f32 velocityMin )
            {
                m_velocityMin = velocityMin;
            }

            f32 getVelocityMax() const
            {
                return m_velocityMax;
            }

            void setVelocityMax( f32 velocityMax )
            {
                m_velocityMax = velocityMax;
            }

        protected:
            IParticleSystem *m_particleSystem;
            Vector3<real_Num> m_particleSize;
            Vector3<real_Num> m_particleSizeMin;
            Vector3<real_Num> m_particleSizeMax;
            Vector3<real_Num> m_direction;
            f32 m_timeToLive;
            f32 m_timeToLiveMin;
            f32 m_timeToLiveMax;

            f32 m_emissionRate;
            f32 m_emissionRateMin;
            f32 m_emissionRateMax;

            f32 m_velocity;
            f32 m_velocityMin;
            f32 m_velocityMax;

            static int m_nameExt;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CParticleEmitter_h__
