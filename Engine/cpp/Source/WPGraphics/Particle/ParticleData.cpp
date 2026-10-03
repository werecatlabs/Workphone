#include "WPGraphics/WPClawHammerPCH.hpp"

#include "WPGraphics/Particle/ParticleData.hpp"
#include "WPGraphics/Particle/ParticleState.hpp"

namespace workphone
{
    namespace render
    {
        ParticleData::ParticleData() :
            m_owner( nullptr ),
            m_renderer( nullptr ),
            m_technique( nullptr ),
            m_emitter( nullptr ),
            m_lifeTime( 0.0f ),
            m_maxLifeTime( 0.0f )
        {
            m_previousState = new ParticleState;
            m_currentState = new ParticleState;
        }

        ParticleData::~ParticleData()
        {
        }

        void ParticleData::update( time_interval dt )
        {
            // m_lifeTime += dt;

            // time_interval normalisedTime = m_lifeTime / m_maxLifeTime;
            // if ( normalisedTime >= 1.0f )
            //{
            //	normalisedTime = 1.0f;

            //	if ( m_technique )
            //		m_technique->removeParticle(getOwner());

            //	if ( m_renderer )
            //		m_renderer->removeParticle(getOwner());
            //}

            // m_currentState->m_position = m_currentState->m_position + (m_currentState->m_velocity *
            // dt);

            // m_currentState->m_colour = m_startState->m_colour + ((m_endState->m_colour -
            // m_startState->m_colour) * normalisedTime); m_currentState->m_position =
            // m_startState->m_position + ((m_endState->m_position - m_startState->m_position) *
            // normalisedTime); m_currentState->m_scale = m_startState->m_scale + ((m_endState->m_scale -
            // m_startState->m_scale) * normalisedTime); m_currentState->m_velocity =
            // m_startState->m_velocity + ((m_endState->m_velocity - m_startState->m_velocity) *
            // normalisedTime); m_currentState->m_orientation = QuaternionF::slerp(normalisedTime,
            // m_startState->m_orientation, m_endState->m_orientation);
        }

        time_interval ParticleData::getLifeTime() const
        {
            return m_lifeTime;
        }

        void ParticleData::setLifeTime( time_interval lifeTime )
        {
            m_lifeTime = lifeTime;
        }

        void ParticleData::addLifeTime( time_interval lifeTime )
        {
            m_lifeTime += lifeTime;
        }

        time_interval ParticleData::getMaxLifeTime() const
        {
            return m_maxLifeTime;
        }

        void ParticleData::setMaxLifeTime( time_interval maxLifeTime )
        {
            m_maxLifeTime = maxLifeTime;
        }

        IParticleEmitter *ParticleData::getEmitter() const
        {
            return m_emitter;
        }

        void ParticleData::setEmitter( IParticleEmitter *emitter )
        {
            m_emitter = emitter;
        }

        ParticleState *ParticleData::getPreviousState() const
        {
            return m_previousState;
        }

        ParticleState *ParticleData::getCurrentState() const
        {
            return m_currentState;
        }

        IParticle *ParticleData::getOwner() const
        {
            return m_owner;
        }

        void ParticleData::setOwner( IParticle *owner )
        {
            m_owner = owner;
        }

        IParticleRenderer *ParticleData::getRenderer() const
        {
            return m_renderer;
        }

        void ParticleData::setRenderer( IParticleRenderer *renderer )
        {
            m_renderer = renderer;
        }

        IParticleTechnique *ParticleData::getTechnique() const
        {
            return m_technique;
        }

        void ParticleData::setTechnique( IParticleTechnique *technique )
        {
            m_technique = technique;
        }
    }  // namespace render
}  // namespace workphone
