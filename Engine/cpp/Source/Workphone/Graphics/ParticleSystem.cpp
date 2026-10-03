#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/ParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IParticle.hpp>
#include <Workphone/Interface/Graphics/IParticleTechnique.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone::render, ParticleSystem, GraphicsObject<IParticleSystem> );

        const String ParticleSystem::TemplateNameStr = "templateName";
        const String ParticleSystem::ScaleStr = "scale";
        const String ParticleSystem::StateStr = "state";
        const String ParticleSystem::FastForwardTimeStr = "fastForwardTime";
        const String ParticleSystem::FastForwardIntervalStr = "fastForwardInterval";
        const String ParticleSystem::StartLifetimeStr = "startLifetime";
        const String ParticleSystem::StartSizeStr = "startSize";
        const String ParticleSystem::RateStr = "rate";
        const String ParticleSystem::RateVarianceStr = "rateVariance";
        const String ParticleSystem::AngleStr = "angle";
        const String ParticleSystem::AngleVarianceStr = "angleVariance";
        const String ParticleSystem::ShapeTypeStr = "shapeType";
        const String ParticleSystem::ShapeSizeStr = "shapeSize";
        const String ParticleSystem::ShapeSizeVarianceStr = "shapeSizeVariance";
        const String ParticleSystem::DurationStr = "duration";
        const String ParticleSystem::LoopingStr = "looping";

        ParticleSystem::ParticleSystem()
        {
        }

        ParticleSystem::~ParticleSystem()
        {
        }

        void ParticleSystem::load( SmartPtr<ISharedObject> data )
        {
            GraphicsObject<IParticleSystem>::load( data );
        }

        void ParticleSystem::unload( SmartPtr<ISharedObject> data )
        {
            GraphicsObject<IParticleSystem>::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }

        void ParticleSystem::reload( SmartPtr<ISharedObject> data )
        {
            unload( data );
            load( data );
        }

        void ParticleSystem::update()
        {
            if( getState() != ParticleSystemState::Started )
            {
                return;
            }

            for( auto technique : m_techniques )
            {
                if( technique )
                {
                    technique->update();
                }
            }
        }

        void ParticleSystem::setFastForward( f32 time, f32 interval )
        {
            m_fastForwardTime = time;
            m_fastForwardInterval = interval;
        }

        f32 ParticleSystem::getFastForwardTime() const
        {
            return m_fastForwardTime;
        }

        f32 ParticleSystem::getFastForwardInterval() const
        {
            return m_fastForwardInterval;
        }

        void ParticleSystem::setTemplateName( const String &templateName )
        {
            m_templateName = templateName;
        }

        String ParticleSystem::getTemplateName() const
        {
            return m_templateName;
        }

        void ParticleSystem::setScale( const Vector3<real_Num> &scale )
        {
            m_scale = scale;
        }

        Vector3<real_Num> ParticleSystem::getScale() const
        {
            return m_scale;
        }

        ParticleSystemState ParticleSystem::getState() const
        {
            return m_state;
        }

        void ParticleSystem::setState( ParticleSystemState state )
        {
            m_state = state;
        }

        size_t ParticleSystem::getNumParticles() const
        {
            return m_particles.size();
        }

        size_t ParticleSystem::getNumEmitters() const
        {
            return 0u;
        }

        SmartPtr<Properties> ParticleSystem::getProperties() const
        {
            auto properties = GraphicsObject<IParticleSystem>::getProperties();
            if( !properties )
            {
                return nullptr;
            }

            properties->setProperty( TemplateNameStr, getTemplateName() );
            properties->setProperty( ScaleStr, getScale() );
            properties->setProperty( StateStr, (u32)getState() );
            properties->setProperty( FastForwardTimeStr, getFastForwardTime() );
            properties->setProperty( FastForwardIntervalStr, getFastForwardInterval() );
            properties->setProperty( StartLifetimeStr, getStartLifetime() );
            properties->setProperty( StartSizeStr, getStartSize() );
            properties->setProperty( RateStr, getRate() );
            properties->setProperty( RateVarianceStr, getRateVariance() );
            properties->setProperty( AngleStr, getAngle() );
            properties->setProperty( AngleVarianceStr, getAngleVariance() );
            properties->setProperty( ShapeTypeStr, getShapeType() );
            properties->setProperty( ShapeSizeStr, getShapeSize() );
            properties->setProperty( ShapeSizeVarianceStr, getShapeSizeVariance() );
            properties->setProperty( DurationStr, getDuration() );
            properties->setProperty( LoopingStr, getLooping() );

            return properties;
        }

        void ParticleSystem::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                return;
            }

            GraphicsObject<IParticleSystem>::setProperties( properties );

            auto templateName = getTemplateName();
            auto scale = getScale();
            auto state = (u32)getState();
            auto fastForwardTime = getFastForwardTime();
            auto fastForwardInterval = getFastForwardInterval();
            auto startLifetime = getStartLifetime();
            auto startSize = getStartSize();
            auto rate = getRate();
            auto rateVariance = getRateVariance();
            auto angle = getAngle();
            auto angleVariance = getAngleVariance();
            auto shapeType = getShapeType();
            auto shapeSize = getShapeSize();
            auto shapeSizeVariance = getShapeSizeVariance();
            auto duration = getDuration();
            auto looping = getLooping();

            properties->getPropertyValue( TemplateNameStr, templateName );
            properties->getPropertyValue( ScaleStr, scale );
            properties->getPropertyValue( StateStr, state );
            properties->getPropertyValue( FastForwardTimeStr, fastForwardTime );
            properties->getPropertyValue( FastForwardIntervalStr, fastForwardInterval );
            properties->getPropertyValue( StartLifetimeStr, startLifetime );
            properties->getPropertyValue( StartSizeStr, startSize );
            properties->getPropertyValue( RateStr, rate );
            properties->getPropertyValue( RateVarianceStr, rateVariance );
            properties->getPropertyValue( AngleStr, angle );
            properties->getPropertyValue( AngleVarianceStr, angleVariance );
            properties->getPropertyValue( ShapeTypeStr, shapeType );
            properties->getPropertyValue( ShapeSizeStr, shapeSize );
            properties->getPropertyValue( ShapeSizeVarianceStr, shapeSizeVariance );
            properties->getPropertyValue( DurationStr, duration );
            properties->getPropertyValue( LoopingStr, looping );

            setTemplateName( templateName );
            setScale( scale );
            setState( (ParticleSystemState)state );
            setFastForward( fastForwardTime, fastForwardInterval );
            setStartLifetime( startLifetime );
            setStartSize( startSize );
            setRate( rate );
            setRateVariance( rateVariance );
            setAngle( angle );
            setAngleVariance( angleVariance );
            setShapeType( shapeType );
            setShapeSize( shapeSize );
            setShapeSizeVariance( shapeSizeVariance );
            setDuration( duration );
            setLooping( looping );
        }

        Vector2<real_Num> ParticleSystem::getStartLifetime() const
        {
            return m_startLifetime;
        }

        void ParticleSystem::setStartLifetime( const Vector2<real_Num> &startLifetime )
        {
            m_startLifetime = startLifetime;
        }

        Vector2<real_Num> ParticleSystem::getStartSize() const
        {
            return m_startSize;
        }

        void ParticleSystem::setStartSize( const Vector2<real_Num> &startSize )
        {
            m_startSize = startSize;
        }

        f32 ParticleSystem::getRate() const
        {
            return m_rate;
        }

        void ParticleSystem::setRate( f32 rate )
        {
            m_rate = Math<f32>::max( 0.0f, rate );
        }

        f32 ParticleSystem::getRateVariance() const
        {
            return m_rateVariance;
        }

        void ParticleSystem::setRateVariance( f32 rateVariance )
        {
            m_rateVariance = Math<f32>::max( 0.0f, rateVariance );
        }

        f32 ParticleSystem::getAngle() const
        {
            return m_angle;
        }

        void ParticleSystem::setAngle( f32 angle )
        {
            m_angle = angle;
        }

        f32 ParticleSystem::getAngleVariance() const
        {
            return m_angleVariance;
        }

        void ParticleSystem::setAngleVariance( f32 angleVariance )
        {
            m_angleVariance = Math<f32>::max( 0.0f, angleVariance );
        }

        f32 ParticleSystem::getShapeType() const
        {
            return m_shapeType;
        }

        void ParticleSystem::setShapeType( f32 shapeType )
        {
            m_shapeType = shapeType;
        }

        f32 ParticleSystem::getShapeSize() const
        {
            return m_shapeSize;
        }

        void ParticleSystem::setShapeSize( f32 shapeSize )
        {
            m_shapeSize = Math<f32>::max( 0.0f, shapeSize );
        }

        f32 ParticleSystem::getShapeSizeVariance() const
        {
            return m_shapeSizeVariance;
        }

        void ParticleSystem::setShapeSizeVariance( f32 shapeSizeVariance )
        {
            m_shapeSizeVariance = Math<f32>::max( 0.0f, shapeSizeVariance );
        }

        f32 ParticleSystem::getDuration() const
        {
            return m_duration;
        }

        void ParticleSystem::setDuration( f32 duration )
        {
            m_duration = Math<f32>::max( 0.0f, duration );
        }

        bool ParticleSystem::getLooping() const
        {
            return m_looping;
        }

        void ParticleSystem::setLooping( bool looping )
        {
            m_looping = looping;
        }

        u32 ParticleSystem::getNumTechniques() const
        {
            return static_cast<u32>( m_techniques.size() );
        }

        SmartPtr<IParticleTechnique> ParticleSystem::addTechnique()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return nullptr;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( !graphicsSystem )
            {
                return nullptr;
            }

            if( auto factoryManager = graphicsSystem->getFactoryManagerPtr() )
            {
                if( auto technique = factoryManager->make_object<IParticleTechnique>() )
                {
                    technique->setParticleSystem( this );
                    m_techniques.push_back( technique );
                    return technique;
                }
            }

            return nullptr;
        }

        SmartPtr<IParticleTechnique> ParticleSystem::addTechnique( const String &name )
        {
            if( auto technique = addTechnique() )
            {
                technique->setName( name );
                return technique;
            }

            return nullptr;
        }

        void ParticleSystem::removeTechnique( SmartPtr<IParticleTechnique> technique )
        {
            m_techniques.erase( std::remove( m_techniques.begin(), m_techniques.end(), technique ),
                                m_techniques.end() );
        }

        SmartPtr<IParticleTechnique> ParticleSystem::getTechnique( const String &name ) const
        {
            for( auto technique : m_techniques )
            {
                if( technique && technique->getName() == name )
                {
                    return technique;
                }
            }

            return nullptr;
        }

        SmartPtr<IParticle> ParticleSystem::addParticle()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return nullptr;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( !graphicsSystem )
            {
                return nullptr;
            }

            if( auto factoryManager = graphicsSystem->getFactoryManagerPtr() )
            {
                if( auto particle = factoryManager->make_object<IParticle>() )
                {
                    m_particles.push_back( particle );
                    return particle;
                }
            }

            return nullptr;
        }

    }  // namespace render
}  // namespace workphone
