#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/ParticleEmitter.hpp>
#include <Workphone/Interface/Graphics/IParticle.hpp>
#include <Workphone/Interface/Graphics/IParticleTechnique.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Math/Math.hpp>
#include <random>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, ParticleEmitter, IParticleEmitter );

    // Thread-local random number generator for particle emission
    static thread_local std::mt19937 s_randomEngine( std::random_device{}() );

    ParticleEmitter::ParticleEmitter() :
        m_particleSize( 1.0f, 1.0f, 1.0f ),
        m_particleSizeMin( 0.1f, 0.1f, 0.1f ),
        m_particleSizeMax( 10.0f, 10.0f, 10.0f ),
        m_direction( 0.0f, 1.0f, 0.0f ),
        m_emissionRate( 10.0f ),
        m_emissionRateMin( 1.0f ),
        m_emissionRateMax( 100.0f ),
        m_timeToLive( 5.0f ),
        m_timeToLiveMin( 1.0f ),
        m_timeToLiveMax( 10.0f ),
        m_velocity( 10.0f ),
        m_velocityMin( 1.0f ),
        m_velocityMax( 100.0f ),
        m_emissionBurstCount( 0 ),
        m_emissionBurstInterval( 0.0f ),
        m_burstTimer( 0.0f ),
        m_emitterEnabled( true ),
        m_infiniteEmission( true ),
        m_totalEmissions( 0 ),
        m_currentEmissions( 0 ),
        m_emitterType( EmitterType::Point ),
        m_spreadAngle( 0.0f ),
        m_spreadAngleVariance( 0.0f ),
        m_emissionShapeSize( 1.0f, 1.0f, 1.0f ),
        m_tangent( 1.0f, 0.0f, 0.0f ),
        m_binormal( 0.0f, 0.0f, 1.0f ),
        m_randomizeDirection( false ),
        m_randomizeVelocity( false ),
        m_randomizeSize( false ),
        m_randomizeTTL( false )
    {
    }

    ParticleEmitter::~ParticleEmitter() = default;

    void ParticleEmitter::update()
    {
        if( !m_emitterEnabled )
        {
            return;
        }

        auto particleSystem = getParticleSystem();
        if( !particleSystem )
        {
            return;
        }

        // Check emission limits
        if( !m_infiniteEmission && m_currentEmissions >= m_totalEmissions )
        {
            return;
        }

        // Calculate clamped values
        auto rate = Math<f32>::clamp( getEmissionRate(), getEmissionRateMin(), getEmissionRateMax() );
        auto timeToLive = Math<f32>::clamp( getTimeToLive(), getTimeToLiveMin(), getTimeToLiveMax() );
        auto velocity = Math<f32>::clamp( getVelocity(), getVelocityMin(), getVelocityMax() );

        // Handle burst emission
        if( m_emissionBurstCount > 0 )
        {
            m_burstTimer +=
                static_cast<f32>( core::IApplicationManager::instancePtr()->getTimer()->getTime() );

            if( m_burstTimer >= m_emissionBurstInterval && m_emissionBurstInterval > 0.0f )
            {
                m_burstTimer = 0.0f;
                emitBurst( m_emissionBurstCount, particleSystem, timeToLive, velocity );
            }
        }

        // Continuous emission based on rate
        if( rate > 0.0f )
        {
            m_emissionAccumulator +=
                static_cast<f32>( core::IApplicationManager::instancePtr()->getTimer()->getTime() );

            while( m_emissionAccumulator >= ( 1.0f / rate ) )
            {
                m_emissionAccumulator -= ( 1.0f / rate );
                emitParticle( particleSystem, timeToLive, velocity );

                if( !m_infiniteEmission )
                {
                    ++m_currentEmissions;
                    if( m_currentEmissions >= m_totalEmissions )
                    {
                        break;
                    }
                }
            }
        }
    }

    void ParticleEmitter::emitParticle( SmartPtr<IParticleSystem> particleSystem, f32 timeToLive,
                                        f32 velocity )
    {
        auto particle = particleSystem->addParticle();
        if( !particle )
        {
            return;
        }

        // Calculate emission position based on emitter type
        Vector3<real_Num> emitPosition = calculateEmissionPosition();

        // Calculate emission direction with optional randomization
        Vector3<real_Num> emitDirection = calculateEmissionDirection();

        // Apply variance to parameters if randomization is enabled
        f32 finalVelocity = m_randomizeVelocity
                                ? Math<f32>::lerp( m_velocityMin, m_velocityMax, getRandomFloat() )
                                : velocity;

        f32 finalTTL = m_randomizeTTL
                           ? Math<f32>::lerp( m_timeToLiveMin, m_timeToLiveMax, getRandomFloat() )
                           : timeToLive;

        Vector3<real_Num> finalSize =
            m_randomizeSize
                ? Vector3<real_Num>(
                      Math<f32>::lerp( m_particleSizeMin.x, m_particleSizeMax.x, getRandomFloat() ),
                      Math<f32>::lerp( m_particleSizeMin.y, m_particleSizeMax.y, getRandomFloat() ),
                      Math<f32>::lerp( m_particleSizeMin.z, m_particleSizeMax.z, getRandomFloat() ) )
                : getParticleSize();

        // Initialize particle data if available
        void *dataPtr = particle->getData();
        if( dataPtr )
        {
            // ParticleData initialization would go here
            // This is intentionally left abstract for renderer-specific implementations
        }

        // Set initial particle state through interface
        // Note: Actual particle state setting depends on IParticle implementation
    }

    void ParticleEmitter::emitBurst( u32 count, SmartPtr<IParticleSystem> particleSystem, f32 timeToLive,
                                     f32 velocity )
    {
        for( u32 i = 0; i < count; ++i )
        {
            emitParticle( particleSystem, timeToLive, velocity );

            if( !m_infiniteEmission )
            {
                ++m_currentEmissions;
                if( m_currentEmissions >= m_totalEmissions )
                {
                    break;
                }
            }
        }
    }

    Vector3<real_Num> ParticleEmitter::calculateEmissionPosition() const
    {
        Vector3<real_Num> basePos = getPosition();

        switch( m_emitterType )
        {
        case EmitterType::Point:
            return basePos;

        case EmitterType::Box:
        {
            // Random position within box
            f32 x = Math<f32>::lerp( -m_emissionShapeSize.x * 0.5f, m_emissionShapeSize.x * 0.5f,
                                     getRandomFloat() );
            f32 y = Math<f32>::lerp( -m_emissionShapeSize.y * 0.5f, m_emissionShapeSize.y * 0.5f,
                                     getRandomFloat() );
            f32 z = Math<f32>::lerp( -m_emissionShapeSize.z * 0.5f, m_emissionShapeSize.z * 0.5f,
                                     getRandomFloat() );
            return basePos + Vector3<real_Num>( x, y, z );
        }

        case EmitterType::Sphere:
        {
            // Random position on sphere surface or volume
            f32 radius = m_emissionShapeSize.x * 0.5f;
            f32 theta = getRandomFloat() * Math<f32>::pi() * 2.0f;
            f32 phi = std::acos( 1.0f - 2.0f * getRandomFloat() );

            f32 x = radius * std::sin( phi ) * std::cos( theta );
            f32 y = radius * std::sin( phi ) * std::sin( theta );
            f32 z = radius * std::cos( phi );

            return basePos + Vector3<real_Num>( x, y, z );
        }

        case EmitterType::Cone:
        {
            // Random position within cone
            f32 height = m_emissionShapeSize.y;
            f32 radius = m_emissionShapeSize.x * 0.5f;
            f32 h = getRandomFloat() * height;
            f32 r = ( h / height ) * radius;
            f32 theta = getRandomFloat() * Math<f32>::pi() * 2.0f;

            f32 x = r * std::cos( theta );
            f32 z = r * std::sin( theta );

            return basePos + Vector3<real_Num>( x, h - height * 0.5f, z );
        }

        case EmitterType::Mesh:
        {
            // Mesh-based emission would require mesh data
            // Return base position as fallback
            return basePos;
        }

        default:
            return basePos;
        }
    }

    Vector3<real_Num> ParticleEmitter::calculateEmissionDirection() const
    {
        Vector3<real_Num> dir = getDirection();

        if( m_randomizeDirection || m_spreadAngle > 0.0f )
        {
            // Apply spread angle variance
            f32 spreadRad = Math<f32>::DegToRad( m_spreadAngle );
            f32 variance =
                m_spreadAngleVariance > 0.0f ? m_spreadAngleVariance * getRandomFloat() : 0.0f;

            // Create random offset within cone
            f32 angle = spreadRad * variance * getRandomFloat();
            f32 azimuth = getRandomFloat() * Math<f32>::pi() * 2.0f;

            // Rotate direction vector
            Vector3<real_Num> offset( std::sin( angle ) * std::cos( azimuth ),
                                      std::sin( angle ) * std::sin( azimuth ), std::cos( angle ) );

            // Apply rotation using tangent/binormal basis
            dir = offset.x * m_tangent + offset.y * dir + offset.z * m_binormal;
        }

        // Normalize to ensure consistent velocity application
        if( dir.length() > Math<f32>::epsilon() )
        {
            dir.normalise();
        }

        return dir;
    }

    f32 ParticleEmitter::getRandomFloat() const
    {
        std::uniform_real_distribution<f32> dist( 0.0f, 1.0f );
        return dist( s_randomEngine );
    }

    void ParticleEmitter::setVelocityMax( f32 maxVelocity )
    {
        m_velocityMax = Math<f32>::max( 0.0f, maxVelocity );
    }

    f32 ParticleEmitter::getVelocityMax() const
    {
        return m_velocityMax;
    }

    void ParticleEmitter::setVelocityMin( f32 minVelocity )
    {
        m_velocityMin = Math<f32>::max( 0.0f, minVelocity );
    }

    f32 ParticleEmitter::getVelocityMin() const
    {
        return m_velocityMin;
    }

    void ParticleEmitter::setVelocity( f32 velocity )
    {
        m_velocity = Math<f32>::max( 0.0f, velocity );
    }

    f32 ParticleEmitter::getVelocity() const
    {
        return m_velocity;
    }

    void ParticleEmitter::setTimeToLiveMax( f32 maxTimeToLive )
    {
        m_timeToLiveMax = Math<f32>::max( 0.0f, maxTimeToLive );
    }

    f32 ParticleEmitter::getTimeToLiveMax() const
    {
        return m_timeToLiveMax;
    }

    void ParticleEmitter::setTimeToLiveMin( f32 minTimeToLive )
    {
        m_timeToLiveMin = Math<f32>::max( 0.0f, minTimeToLive );
    }

    f32 ParticleEmitter::getTimeToLiveMin() const
    {
        return m_timeToLiveMin;
    }

    void ParticleEmitter::setTimeToLive( f32 timeToLive )
    {
        m_timeToLive = Math<f32>::max( 0.0f, timeToLive );
    }

    f32 ParticleEmitter::getTimeToLive() const
    {
        return m_timeToLive;
    }

    void ParticleEmitter::setEmissionRateMax( f32 maxRate )
    {
        m_emissionRateMax = Math<f32>::max( 0.0f, maxRate );
    }

    f32 ParticleEmitter::getEmissionRateMax() const
    {
        return m_emissionRateMax;
    }

    void ParticleEmitter::setEmissionRateMin( f32 minRate )
    {
        m_emissionRateMin = Math<f32>::max( 0.0f, minRate );
    }

    f32 ParticleEmitter::getEmissionRateMin() const
    {
        return m_emissionRateMin;
    }

    void ParticleEmitter::setEmissionRate( f32 rate )
    {
        m_emissionRate = Math<f32>::max( 0.0f, rate );
    }

    f32 ParticleEmitter::getEmissionRate() const
    {
        return m_emissionRate;
    }

    void ParticleEmitter::setDirection( const Vector3<real_Num> &direction )
    {
        m_direction = direction;
        if( m_direction.length() > Math<f32>::epsilon() )
        {
            m_direction.normalise();
        }
    }

    Vector3<real_Num> ParticleEmitter::getDirection() const
    {
        return m_direction;
    }

    void ParticleEmitter::setParticleSizeMax( const Vector3<real_Num> &maxSize )
    {
        m_particleSizeMax = maxSize;
        // Ensure min <= max
        m_particleSizeMin.x = Math<f32>::min( m_particleSizeMin.x, maxSize.x );
        m_particleSizeMin.y = Math<f32>::min( m_particleSizeMin.y, maxSize.y );
        m_particleSizeMin.z = Math<f32>::min( m_particleSizeMin.z, maxSize.z );
    }

    Vector3<real_Num> ParticleEmitter::getParticleSizeMax() const
    {
        return m_particleSizeMax;
    }

    void ParticleEmitter::setParticleSizeMin( const Vector3<real_Num> &minSize )
    {
        m_particleSizeMin = minSize;
        // Ensure min <= max
        m_particleSizeMax.x = Math<f32>::max( m_particleSizeMax.x, minSize.x );
        m_particleSizeMax.y = Math<f32>::max( m_particleSizeMax.y, minSize.y );
        m_particleSizeMax.z = Math<f32>::max( m_particleSizeMax.z, minSize.z );
    }

    Vector3<real_Num> ParticleEmitter::getParticleSizeMin() const
    {
        return m_particleSizeMin;
    }

    void ParticleEmitter::setParticleSize( const Vector3<real_Num> &size )
    {
        m_particleSize = size;
    }

    Vector3<real_Num> ParticleEmitter::getParticleSize() const
    {
        return m_particleSize;
    }

    void ParticleEmitter::setParticleSystem( SmartPtr<IParticleSystem> particleSystem )
    {
        m_particleSystem = particleSystem;
    }

    SmartPtr<IParticleSystem> ParticleEmitter::getParticleSystem() const
    {
        return m_particleSystem.lock();
    }

    // Additional emitter configuration methods
    void ParticleEmitter::setEmissionBurst( u32 count, f32 interval )
    {
        m_emissionBurstCount = count;
        m_emissionBurstInterval = interval;
    }

    u32 ParticleEmitter::getEmissionBurstCount() const
    {
        return m_emissionBurstCount;
    }

    f32 ParticleEmitter::getEmissionBurstInterval() const
    {
        return m_emissionBurstInterval;
    }

    void ParticleEmitter::setEmitterEnabled( bool enabled )
    {
        m_emitterEnabled = enabled;
    }

    bool ParticleEmitter::isEmitterEnabled() const
    {
        return m_emitterEnabled;
    }

    void ParticleEmitter::setInfiniteEmission( bool infinite )
    {
        m_infiniteEmission = infinite;
        if( infinite )
        {
            m_currentEmissions = 0;
        }
    }

    bool ParticleEmitter::isInfiniteEmission() const
    {
        return m_infiniteEmission;
    }

    void ParticleEmitter::setTotalEmissions( u32 total )
    {
        m_totalEmissions = total;
        m_infiniteEmission = ( total == 0 );
    }

    u32 ParticleEmitter::getTotalEmissions() const
    {
        return m_totalEmissions;
    }

    u32 ParticleEmitter::getCurrentEmissions() const
    {
        return m_currentEmissions;
    }

    void ParticleEmitter::resetEmissions()
    {
        m_currentEmissions = 0;
        m_emissionAccumulator = 0.0f;
        m_burstTimer = 0.0f;
    }

    void ParticleEmitter::setEmitterType( EmitterType type )
    {
        m_emitterType = type;
    }

    EmitterType ParticleEmitter::getEmitterType() const
    {
        return m_emitterType;
    }

    void ParticleEmitter::setSpreadAngle( f32 angle )
    {
        m_spreadAngle = Math<f32>::clamp( angle, 0.0f, 180.0f );
    }

    f32 ParticleEmitter::getSpreadAngle() const
    {
        return m_spreadAngle;
    }

    void ParticleEmitter::setSpreadAngleVariance( f32 variance )
    {
        m_spreadAngleVariance = Math<f32>::clamp( variance, 0.0f, 1.0f );
    }

    f32 ParticleEmitter::getSpreadAngleVariance() const
    {
        return m_spreadAngleVariance;
    }

    void ParticleEmitter::setEmissionShapeSize( const Vector3<real_Num> &size )
    {
        m_emissionShapeSize = size;
    }

    Vector3<real_Num> ParticleEmitter::getEmissionShapeSize() const
    {
        return m_emissionShapeSize;
    }

    void ParticleEmitter::setTangent( const Vector3<real_Num> &tangent )
    {
        m_tangent = tangent;
        if( m_tangent.length() > Math<f32>::epsilon() )
        {
            m_tangent.normalise();
        }
    }

    Vector3<real_Num> ParticleEmitter::getTangent() const
    {
        return m_tangent;
    }

    void ParticleEmitter::setBinormal( const Vector3<real_Num> &binormal )
    {
        m_binormal = binormal;
        if( m_binormal.length() > Math<f32>::epsilon() )
        {
            m_binormal.normalise();
        }
    }

    Vector3<real_Num> ParticleEmitter::getBinormal() const
    {
        return m_binormal;
    }

    void ParticleEmitter::setRandomizeDirection( bool randomize )
    {
        m_randomizeDirection = randomize;
    }

    bool ParticleEmitter::isRandomizeDirection() const
    {
        return m_randomizeDirection;
    }

    void ParticleEmitter::setRandomizeVelocity( bool randomize )
    {
        m_randomizeVelocity = randomize;
    }

    bool ParticleEmitter::isRandomizeVelocity() const
    {
        return m_randomizeVelocity;
    }

    void ParticleEmitter::setRandomizeSize( bool randomize )
    {
        m_randomizeSize = randomize;
    }

    bool ParticleEmitter::isRandomizeSize() const
    {
        return m_randomizeSize;
    }

    void ParticleEmitter::setRandomizeTTL( bool randomize )
    {
        m_randomizeTTL = randomize;
    }

    bool ParticleEmitter::isRandomizeTTL() const
    {
        return m_randomizeTTL;
    }

}  // namespace workphone::render
