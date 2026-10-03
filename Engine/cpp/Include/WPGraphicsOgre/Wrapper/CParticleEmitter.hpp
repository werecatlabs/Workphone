#ifndef CParticleEmitter_h__
#define CParticleEmitter_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IParticleEmitter.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CParticleNode.hpp>
#include <Workphone/Core/HashMap.hpp>

#if WP_OGRE_USE_PARTICLE_UNIVERSE
#    include "ParticleUniverseSystemManager.h"
#endif

namespace workphone
{
    namespace render
    {

        class PUParticleEmitter : public CParticleNode<IParticleEmitter>
        {
        public:
            PUParticleEmitter();

            PUParticleEmitter( ParticleUniverse::ParticleEmitter *emitter );

            ~PUParticleEmitter();

            ParticleUniverse::ParticleEmitter *getEmitter() const;

            void setEmitter( ParticleUniverse::ParticleEmitter *emitter );

            //
            // IScriptObjects
            //

            /** Gets an object call script functions. */
            virtual SmartPtr<IScriptInvoker> &getInvoker();

            /** Gets an object call script functions. */
            virtual const SmartPtr<IScriptInvoker> &getInvoker() const;

            /** Sets an object call script functions. */
            virtual void setInvoker( SmartPtr<IScriptInvoker> invoker );

            /** Gets an object to receive script calls. */
            virtual SmartPtr<IScriptReceiver> &getReceiver();

            /** Gets an object to receive script calls. */
            virtual const SmartPtr<IScriptReceiver> &getReceiver() const;

            /** Sets an object to receive script calls. */
            virtual void setReceiver( SmartPtr<IScriptReceiver> receiver );

            Vector3F getParticleSize() const override;

            void setParticleSize( const Vector3F &particleSize ) override;

            Vector3F getParticleSizeMin() const override;

            void setParticleSizeMin( const Vector3F &particleSizeMin ) override;

            Vector3F getParticleSizeMax() const override;

            void setParticleSizeMax( const Vector3F &particleSizeMax ) override;

            Vector3F getDirection() const override;

            void setDirection( const Vector3F &direction ) override;

            f32 getEmissionRate() const override;

            void setEmissionRate( f32 emissionRate ) override;

            f32 getEmissionRateMin() const override;

            void setEmissionRateMin( f32 emissionRateMin ) override;

            f32 getEmissionRateMax() const override;

            void setEmissionRateMax( f32 emissionRateMax ) override;

            f32 getTimeToLive() const override;

            void setTimeToLive( f32 timeToLive ) override;

            f32 getTimeToLiveMin() const override;

            void setTimeToLiveMin( f32 timeToLiveMin ) override;

            f32 getTimeToLiveMax() const override;

            void setTimeToLiveMax( f32 timeToLiveMax ) override;

            f32 getVelocity() const override;

            void setVelocity( f32 velocity ) override;

            f32 getVelocityMin() const override;

            void setVelocityMin( f32 velocityMin ) override;

            f32 getVelocityMax() const override;

            void setVelocityMax( f32 velocityMax ) override;

            virtual void calculateState( SmartPtr<IParticle> particle, u32 stateIndex,
                                         void *data = nullptr );

            SmartPtr<IParticleSystem> getParticleSystem() const override;
            void setParticleSystem( SmartPtr<IParticleSystem> particleSystem ) override;

        protected:
            SmartPtr<IScriptInvoker> m_scriptInvoker;
            SmartPtr<IScriptReceiver> m_scriptReceiver;
            ParticleUniverse::ParticleEmitter *m_emitter;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CParticleEmitter_h__
