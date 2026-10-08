#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Particle/CParticleSystem.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CParticleSystem, ParticleSystem );

    void CParticleSystem::load( SmartPtr<ISharedObject> data )
    {
        std::unique_lock<std::mutex> lock( m_simulationMutex );
        if( m_simulation ) return;
        if( !getTemplateName().empty() )
        {
            WP_LOG_ERROR( "Claw particles: template loading is not implemented; use explicit settings." );
            setLoadingState( LoadingState::Unloaded );
            return;
        }
        auto simulation = wp_particle_simulation_create( m_poolSize, m_seed );
        if( !simulation )
        {
            WP_LOG_ERROR( "Claw particles: invalid capacity or allocation failure." );
            setLoadingState( LoadingState::Unloaded );
            return;
        }
        if( !m_customSimulationSettings )
        {
            wp_particle_simulation_default_settings( &m_simulationSettings );
            m_simulationSettings.rate = getRate();
            const auto lifetime = getStartLifetime();
            const auto size = getStartSize();
            m_simulationSettings.lifetime_min = lifetime.X();
            m_simulationSettings.lifetime_max = lifetime.Y();
            m_simulationSettings.size_min = size.X();
            m_simulationSettings.size_max = size.Y();
            m_simulationSettings.duration = getDuration();
            m_simulationSettings.looping = getLooping();
        }
        if( !wp_particle_simulation_configure( simulation, &m_simulationSettings ) )
        {
            wp_particle_simulation_destroy( simulation );
            WP_LOG_ERROR( "Claw particles: invalid simulation settings." );
            setLoadingState( LoadingState::Unloaded );
            return;
        }
        m_simulation = simulation;
        const auto requestedState = m_state;
        lock.unlock();
        ParticleSystem::load( data );
        setLoadingState( LoadingState::Loaded );
        setState( requestedState );
    }

    void CParticleSystem::unload( SmartPtr<ISharedObject> data )
    {
        std::unique_lock<std::mutex> lock( m_simulationMutex );
        wp_particle_simulation_destroy( m_simulation );
        m_simulation = nullptr;
        lock.unlock();
        ParticleSystem::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    bool CParticleSystem::setSimulationSettings( const wp_particle_simulation_settings &settings )
    {
        std::lock_guard<std::mutex> lock( m_simulationMutex );
        // Validate unloaded settings using a tiny temporary pool; live changes allocate nothing.
        auto validator = m_simulation ? m_simulation : wp_particle_simulation_create( 1, m_seed );
        const bool valid = validator && wp_particle_simulation_configure( validator, &settings );
        if( !m_simulation ) wp_particle_simulation_destroy( validator );
        if( !valid ) return false;
        m_simulationSettings = settings;
        m_customSimulationSettings = true;
        return true;
    }

    void CParticleSystem::setState( ParticleSystemState state )
    {
        std::lock_guard<std::mutex> lock( m_simulationMutex );
        if( state == ParticleSystemState::PausedForTime || static_cast<u32>( state ) > 4u )
        {
            WP_LOG_ERROR( "Claw particles: unsupported particle state." );
            return;
        }
        const bool starting = m_simulation && state == ParticleSystemState::Started &&
            wp_particle_simulation_get_state( m_simulation ) == WORKPHONE_PARTICLE_STATE_STOPPED;
        if( m_simulation ) wp_particle_simulation_set_state( m_simulation, static_cast<wp_particle_state>( state ) );
        if( starting && getFastForwardTime() > 0.0f )
        {
            auto remaining = getFastForwardTime();
            if( !std::isfinite( remaining ) || remaining > 10.0f )
            {
                WP_LOG_ERROR( "Claw particles: prewarm must be finite and at most ten seconds." );
            }
            else while( remaining > 0.0f )
            {
                const auto slice = std::min( remaining, 1.0f );
                wp_particle_simulation_advance( m_simulation, slice );
                remaining -= slice;
            }
        }
        m_state = state;
    }

    ParticleSystemState CParticleSystem::getState() const
    {
        std::lock_guard<std::mutex> lock( m_simulationMutex );
        return m_simulation ? static_cast<ParticleSystemState>( wp_particle_simulation_get_state( m_simulation ) ) : m_state;
    }

    bool CParticleSystem::simulate( f32 seconds )
    {
        std::lock_guard<std::mutex> lock( m_simulationMutex );
        if( !m_simulation ) return false;
        if( !m_customSimulationSettings )
        {
            m_simulationSettings.rate = getRate();
            m_simulationSettings.lifetime_min = getStartLifetime().X();
            m_simulationSettings.lifetime_max = getStartLifetime().Y();
            m_simulationSettings.size_min = getStartSize().X();
            m_simulationSettings.size_max = getStartSize().Y();
            m_simulationSettings.duration = getDuration();
            m_simulationSettings.looping = getLooping();
            if( !wp_particle_simulation_configure( m_simulation, &m_simulationSettings ) ) return false;
        }
        const bool advanced = wp_particle_simulation_advance( m_simulation, seconds );
        m_state = static_cast<ParticleSystemState>( wp_particle_simulation_get_state( m_simulation ) );
        return advanced;
    }

    Array<wp_particle_sample> CParticleSystem::getRenderSnapshot() const
    {
        std::lock_guard<std::mutex> lock( m_simulationMutex );
        Array<wp_particle_sample> result;
        const auto count = wp_particle_simulation_get_count( m_simulation );
        if( count )
        {
            const auto samples = wp_particle_simulation_get_samples( m_simulation );
            result.resize( count );
            std::memcpy( result.data(), samples, static_cast<size_t>(count) * sizeof(wp_particle_sample) );
        }
        return result;
    }

    size_t CParticleSystem::getNumParticles() const
    {
        std::lock_guard<std::mutex> lock( m_simulationMutex );
        return wp_particle_simulation_get_count( m_simulation );
    }

    size_t CParticleSystem::getNumEmitters() const
    {
        std::lock_guard<std::mutex> lock( m_simulationMutex );
        return m_simulation ? 1u : 0u;
    }

    u32 CParticleSystem::getDroppedParticleCount() const
    {
        std::lock_guard<std::mutex> lock( m_simulationMutex );
        return wp_particle_simulation_get_dropped( m_simulation );
    }

    bool CParticleSystem::emitParticle( const Vector3F &position, const Vector3F &velocity,
                                       f32 size, f32 lifetime )
    {
        std::lock_guard<std::mutex> lock( m_simulationMutex );
        return wp_particle_simulation_emit( m_simulation,
            { position.x, position.y, position.z }, { velocity.x, velocity.y, velocity.z },
            size, lifetime ) != 0;
    }

    void CParticleSystem::setSeed( u32 seed )
    {
        std::lock_guard<std::mutex> lock( m_simulationMutex );
        if( m_simulation )
        {
            WP_LOG_ERROR( "Claw particles: set seed before loading or reload the effect." );
            return;
        }
        m_seed = seed;
    }
}
