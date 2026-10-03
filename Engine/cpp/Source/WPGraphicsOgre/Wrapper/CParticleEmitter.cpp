#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CParticleEmitter.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {

        PUParticleEmitter::PUParticleEmitter( ParticleUniverse::ParticleEmitter *emitter ) :
            m_emitter( emitter )
        {
            // setInvoker(SmartPtr<IScriptInvoker>(new ScriptInvokerStandard, true));
            // setReceiver(SmartPtr<IScriptReceiver>(new ScriptReceiver(this), true));
        }

        PUParticleEmitter::PUParticleEmitter()
        {
            // setInvoker(SmartPtr<IScriptInvoker>(new ScriptInvokerStandard, true));
            // setReceiver(SmartPtr<IScriptReceiver>(new ScriptReceiver(this), true));
        }

        PUParticleEmitter::~PUParticleEmitter()
        {
        }

        ParticleUniverse::ParticleEmitter *PUParticleEmitter::getEmitter() const
        {
            return m_emitter;
        }

        void PUParticleEmitter::setEmitter( ParticleUniverse::ParticleEmitter *emitter )
        {
            m_emitter = emitter;
        }

        const workphone::SmartPtr<workphone::IScriptInvoker> &PUParticleEmitter::getInvoker() const
        {
            return m_scriptInvoker;
        }

        workphone::SmartPtr<workphone::IScriptInvoker> &PUParticleEmitter::getInvoker()
        {
            return m_scriptInvoker;
        }

        void PUParticleEmitter::setInvoker( SmartPtr<IScriptInvoker> invoker )
        {
            m_scriptInvoker = invoker;
        }

        const workphone::SmartPtr<workphone::IScriptReceiver> &PUParticleEmitter::getReceiver() const
        {
            return m_scriptReceiver;
        }

        workphone::SmartPtr<workphone::IScriptReceiver> &PUParticleEmitter::getReceiver()
        {
            return m_scriptReceiver;
        }

        void PUParticleEmitter::setReceiver( SmartPtr<IScriptReceiver> receiver )
        {
            m_scriptReceiver = receiver;
        }

        Vector3F PUParticleEmitter::getParticleSize() const
        {
            return Vector3F();
        }

        void PUParticleEmitter::setParticleSize( const Vector3F &particleSize )
        {
        }

        Vector3F PUParticleEmitter::getParticleSizeMin() const
        {
            return Vector3F();
        }

        void PUParticleEmitter::setParticleSizeMin( const Vector3F &particleSizeMin )
        {
        }

        Vector3F PUParticleEmitter::getParticleSizeMax() const
        {
            return Vector3F();
        }

        void PUParticleEmitter::setParticleSizeMax( const Vector3F &particleSizeMax )
        {
        }

        Vector3F PUParticleEmitter::getDirection() const
        {
            return Vector3F();
        }

        void PUParticleEmitter::setDirection( const Vector3F &direction )
        {
        }

        f32 PUParticleEmitter::getEmissionRate() const
        {
            return 0.f;
        }

        void PUParticleEmitter::setEmissionRate( f32 emissionRate )
        {
        }

        f32 PUParticleEmitter::getEmissionRateMin() const
        {
            return 0.f;
        }

        void PUParticleEmitter::setEmissionRateMin( f32 emissionRateMin )
        {
        }

        f32 PUParticleEmitter::getEmissionRateMax() const
        {
            return 0.f;
        }

        void PUParticleEmitter::setEmissionRateMax( f32 emissionRateMax )
        {
        }

        f32 PUParticleEmitter::getTimeToLive() const
        {
            return 0.f;
        }

        void PUParticleEmitter::setTimeToLive( f32 timeToLive )
        {
        }

        f32 PUParticleEmitter::getTimeToLiveMin() const
        {
            return 0.f;
        }

        void PUParticleEmitter::setTimeToLiveMin( f32 timeToLiveMin )
        {
        }

        f32 PUParticleEmitter::getTimeToLiveMax() const
        {
            return 0.f;
        }

        void PUParticleEmitter::setTimeToLiveMax( f32 timeToLiveMax )
        {
        }

        f32 PUParticleEmitter::getVelocity() const
        {
            return 0.f;
        }

        void PUParticleEmitter::setVelocity( f32 velocity )
        {
        }

        f32 PUParticleEmitter::getVelocityMin() const
        {
            return 0.0f;
        }

        void PUParticleEmitter::setVelocityMin( f32 velocityMin )
        {
        }

        f32 PUParticleEmitter::getVelocityMax() const
        {
            return 0.0f;
        }

        void PUParticleEmitter::setVelocityMax( f32 velocityMax )
        {
        }

        void PUParticleEmitter::calculateState( SmartPtr<IParticle> particle, u32 stateIndex,
                                                void *data /*= nullptr*/ )
        {
        }

        SmartPtr<IParticleSystem> PUParticleEmitter::getParticleSystem() const
        {
            return nullptr;
        }

        void PUParticleEmitter::setParticleSystem( SmartPtr<IParticleSystem> particleSystem )
        {
        }

    }  // namespace render
}  // namespace workphone
