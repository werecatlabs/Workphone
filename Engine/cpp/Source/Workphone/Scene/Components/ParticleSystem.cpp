#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Graphics/ParticleSystem.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ParticleSystem, Component );

    // Static const string definitions
    const String ParticleSystem::lifetimeStr = String( "lifetime" );
    const String ParticleSystem::durationStr = String( "duration" );
    const String ParticleSystem::loopingStr = String( "looping" );
    const String ParticleSystem::playStr = String( "play" );
    const String ParticleSystem::stopStr = String( "stop" );
    const String ParticleSystem::templateNameStr = String( "templateName" );
    const String ParticleSystem::techniqueNameStr = String( "techniqueName" );
    const String ParticleSystem::emitterNameStr = String( "emitterName" );
    const String ParticleSystem::playOnLoadStr = String( "playOnLoad" );
    const String ParticleSystem::fastForwardTimeStr = String( "fastForwardTime" );
    const String ParticleSystem::fastForwardIntervalStr = String( "fastForwardInterval" );
    const String ParticleSystem::startLifetimeStr = String( "startLifetime" );
    const String ParticleSystem::startSizeStr = String( "startSize" );
    const String ParticleSystem::scaleStr = String( "scale" );
    const String ParticleSystem::emissionStr = String( "Emission" );
    const String ParticleSystem::rateStr = String( "rate" );
    const String ParticleSystem::rateVarianceStr = String( "rateVariance" );
    const String ParticleSystem::angleStr = String( "angle" );
    const String ParticleSystem::angleVarianceStr = String( "angleVariance" );
    const String ParticleSystem::shapeStr = String( "Shape" );
    const String ParticleSystem::typeStr = String( "type" );
    const String ParticleSystem::sizeStr = String( "size" );
    const String ParticleSystem::sizeVarianceStr = String( "sizeVariance" );
    const String ParticleSystem::shapeTypeStr = String( "shapeType" );
    const String ParticleSystem::shapeSizeStr = String( "shapeSize" );
    const String ParticleSystem::shapeSizeVarianceStr = String( "shapeSizeVariance" );

    ParticleSystem::ParticleSystem() = default;

    ParticleSystem::~ParticleSystem() = default;

    void ParticleSystem::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            createGraphicsParticleSystem();

            if( !getParticleSystem() )
            {
                WP_LOG(
                    "ParticleSystem::load - Component loaded but renderer particle system was not "
                    "created. Some features may be unavailable." );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            auto message = String( "Failed to load ParticleSystem component: " ) + e.what();
            WP_LOG_ERROR( message );
            setLoadingState( LoadingState::Error );
        }
    }

    void ParticleSystem::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            destroyGraphicsParticleSystem();

            m_particleSystem = nullptr;
            m_graphicsObject = nullptr;
            m_graphicsNode = nullptr;

            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            WP_LOG_ERROR( "ParticleSystem::unload - Exception occurred during component unload" );
            setLoadingState( LoadingState::Error );
        }
    }

    Array<SmartPtr<ISharedObject>> ParticleSystem::getChildObjects() const
    {
        auto childObjects = Component::getChildObjects();
        childObjects.push_back( m_particleSystem );
        childObjects.push_back( m_graphicsObject );
        childObjects.push_back( m_graphicsNode );
        return childObjects;
    }

    SmartPtr<Properties> ParticleSystem::getProperties() const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "ParticleSystem::getProperties - ApplicationManager is null" );
            return nullptr;
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "ParticleSystem::getProperties - FactoryManager is null" );
            return nullptr;
        }

        auto properties = Component::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( templateNameStr, getTemplateName() );
        properties->setProperty( techniqueNameStr, getTechniqueName() );
        properties->setProperty( emitterNameStr, getEmitterName() );
        properties->setProperty( playOnLoadStr, getPlayOnLoad() );
        properties->setProperty( lifetimeStr, m_lifetime );
        properties->setProperty( durationStr, m_duration );
        properties->setProperty( loopingStr, m_looping );
        properties->setProperty( fastForwardTimeStr, m_fastForwardTime );
        properties->setProperty( fastForwardIntervalStr, m_fastForwardInterval );
        properties->setProperty( startLifetimeStr, m_startLifetime );
        properties->setProperty( startSizeStr, m_startSize );
        properties->setProperty( scaleStr, m_scale );

        properties->setButtonPressed( playStr, false );
        properties->setButtonPressed( stopStr, false );

        auto emissionProperties = factoryManager->make_ptr<Properties>();
        if( emissionProperties )
        {
            emissionProperties->setName( emissionStr );
            emissionProperties->setProperty( rateStr, m_rate );
            emissionProperties->setProperty( rateVarianceStr, m_rateVariance );
            emissionProperties->setProperty( angleStr, m_angle );
            emissionProperties->setProperty( angleVarianceStr, m_angleVariance );
            properties->addChild( emissionProperties );
        }

        auto shapeProperties = factoryManager->make_ptr<Properties>();
        if( shapeProperties )
        {
            shapeProperties->setName( shapeStr );
            shapeProperties->setProperty( shapeTypeStr, m_shapeType );
            shapeProperties->setProperty( shapeSizeStr, m_shapeSize );
            shapeProperties->setProperty( shapeSizeVarianceStr, m_shapeSizeVariance );
            properties->addChild( shapeProperties );
        }

        return properties;
    }

    void ParticleSystem::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        Component::setProperties( properties );

        auto oldTemplateName = getTemplateName();
        auto oldTechniqueName = getTechniqueName();
        auto oldEmitterName = getEmitterName();

        auto templateName = getTemplateName();
        auto techniqueName = getTechniqueName();
        auto emitterName = getEmitterName();
        auto playOnLoad = getPlayOnLoad();
        auto lifetime = getLifetime();
        auto duration = getDuration();
        auto looping = isLooping();
        auto fastForwardTime = getFastForwardTime();
        auto fastForwardInterval = getFastForwardInterval();
        auto startLifetime = getStartLifetime();
        auto startSize = getStartSize();
        auto scale = getScale();

        properties->getPropertyValue( templateNameStr, templateName );
        properties->getPropertyValue( techniqueNameStr, techniqueName );
        properties->getPropertyValue( emitterNameStr, emitterName );
        properties->getPropertyValue( playOnLoadStr, playOnLoad );
        properties->getPropertyValue( lifetimeStr, lifetime );
        properties->getPropertyValue( durationStr, duration );
        properties->getPropertyValue( loopingStr, looping );
        properties->getPropertyValue( fastForwardTimeStr, fastForwardTime );
        properties->getPropertyValue( fastForwardIntervalStr, fastForwardInterval );
        properties->getPropertyValue( startLifetimeStr, startLifetime );
        properties->getPropertyValue( startSizeStr, startSize );
        properties->getPropertyValue( scaleStr, scale );

        // Validation and Clamping
        if( lifetime < 0.0f )
            lifetime = 0.0f;
        if( duration < 0.0f )
            duration = 0.0f;
        if( fastForwardTime < 0.0f )
            fastForwardTime = 0.0f;
        if( fastForwardInterval < 0.0f )
            fastForwardInterval = 0.0f;

        setTemplateName( templateName );
        setTechniqueName( techniqueName );
        setEmitterName( emitterName );
        setPlayOnLoad( playOnLoad );
        setLifetime( lifetime );
        setDuration( duration );
        setLooping( looping );
        setFastForwardTime( fastForwardTime );
        setFastForwardInterval( fastForwardInterval );
        setStartLifetime( startLifetime );
        setStartSize( startSize );
        setScale( scale );

        if( auto emissionProperties = properties->getChild( emissionStr ) )
        {
            auto rate = getRate();
            auto rateVariance = getRateVariance();
            auto angle = getAngle();
            auto angleVariance = getAngleVariance();

            emissionProperties->getPropertyValue( rateStr, rate );
            emissionProperties->getPropertyValue( rateVarianceStr, rateVariance );
            emissionProperties->getPropertyValue( angleStr, angle );
            emissionProperties->getPropertyValue( angleVarianceStr, angleVariance );

            if( rate < 0.0f )
                rate = 0.0f;
            if( rateVariance < 0.0f )
                rateVariance = 0.0f;
            if( angleVariance < 0.0f )
                angleVariance = 0.0f;

            setRate( rate );
            setRateVariance( rateVariance );
            setAngle( angle );
            setAngleVariance( angleVariance );
        }

        if( auto shapeProperties = properties->getChild( shapeStr ) )
        {
            auto shapeType = getShapeType();
            auto shapeSize = getShapeSize();
            auto shapeSizeVariance = getShapeSizeVariance();

            shapeProperties->getPropertyValue( shapeTypeStr, shapeType );
            shapeProperties->getPropertyValue( shapeSizeStr, shapeSize );
            shapeProperties->getPropertyValue( shapeSizeVarianceStr, shapeSizeVariance );

            if( shapeSize < 0.0f )
                shapeSize = 0.0f;
            if( shapeSizeVariance < 0.0f )
                shapeSizeVariance = 0.0f;

            setShapeType( shapeType );
            setShapeSize( shapeSize );
            setShapeSizeVariance( shapeSizeVariance );
        }

        auto recreateRequired = oldTemplateName != getTemplateName() ||
                                oldTechniqueName != getTechniqueName() ||
                                oldEmitterName != getEmitterName();

        if( recreateRequired && isLoaded() )
        {
            try
            {
                destroyGraphicsParticleSystem();
                createGraphicsParticleSystem();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                WP_LOG_ERROR(
                    "ParticleSystem::setProperties - Failed to recreate particle system after property "
                    "change" );
            }
        }
        else
        {
            applyParticleSystemProperties();
        }

        if( properties->isButtonPressed( playStr ) )
        {
            play();
        }
        else if( properties->isButtonPressed( stopStr ) )
        {
            stop();
        }
    }

    void ParticleSystem::updateTransform()
    {
        if( auto actor = getActorPtr() )
        {
            if( auto actorTransform = actor->getTransformPtr() )
            {
                updateTransform( actorTransform->getWorldTransform() );
            }
        }
    }

    void ParticleSystem::updateTransform( const Transform3<real_Num> &transform )
    {
        if( auto graphicsNode = getGraphicsNode() )
        {
            graphicsNode->setTransform( transform );
        }
    }

    void ParticleSystem::updateVisibility()
    {
        if( auto actor = getActorPtr() )
        {
            if( auto particleSystem = getParticleSystem() )
            {
                particleSystem->setVisible( isEnabled() && actor->isEnabledInScene() );
            }
        }
    }

    SmartPtr<render::IGraphicsObject> ParticleSystem::getGraphicsObject() const
    {
        return m_graphicsObject;
    }

    void ParticleSystem::setGraphicsObject( SmartPtr<render::IGraphicsObject> graphicsObject )
    {
        m_graphicsObject = graphicsObject;
    }

    SmartPtr<render::IGraphicsSceneNode> ParticleSystem::getGraphicsNode() const
    {
        return m_graphicsNode;
    }

    void ParticleSystem::setGraphicsNode( SmartPtr<render::IGraphicsSceneNode> graphicsNode )
    {
        m_graphicsNode = graphicsNode;
    }

    void ParticleSystem::setParticleSystem( SmartPtr<render::IParticleSystem> particleSystem )
    {
        m_particleSystem = particleSystem;
    }

    SmartPtr<render::IParticleSystem> ParticleSystem::getParticleSystem() const
    {
        return m_particleSystem;
    }

    const String &ParticleSystem::getTemplateName() const
    {
        return m_templateName;
    }

    void ParticleSystem::setTemplateName( const String &templateName )
    {
        m_templateName = templateName;

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setTemplateName( m_templateName );
        }
    }

    const String &ParticleSystem::getTechniqueName() const
    {
        return m_techniqueName;
    }

    void ParticleSystem::setTechniqueName( const String &techniqueName )
    {
        m_techniqueName = techniqueName;
    }

    const String &ParticleSystem::getEmitterName() const
    {
        return m_emitterName;
    }

    void ParticleSystem::setEmitterName( const String &emitterName )
    {
        m_emitterName = emitterName;
    }

    bool ParticleSystem::getPlayOnLoad() const
    {
        return m_playOnLoad;
    }

    void ParticleSystem::setPlayOnLoad( bool playOnLoad )
    {
        m_playOnLoad = playOnLoad;
    }

    void ParticleSystem::play()
    {
        try
        {
            if( !getParticleSystem() )
            {
                createGraphicsParticleSystem();
            }

            if( auto particleSystem = getParticleSystem() )
            {
                // Creation commands may configure a component before its actor
                // is inserted into the scene. Refresh these scene-dependent
                // values whenever playback starts so a stale hidden state
                // cannot suppress an otherwise healthy native particle system.
                updateTransform();
                updateVisibility();
                particleSystem->setState( render::ParticleSystemState::Started );
            }
            else
            {
                WP_LOG_ERROR( "ParticleSystem::play - Failed to acquire particle system handle" );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            WP_LOG_ERROR( "ParticleSystem::play - Exception occurred while starting particle system" );
        }
    }

    void ParticleSystem::stop()
    {
        try
        {
            if( auto particleSystem = getParticleSystem() )
            {
                particleSystem->setState( render::ParticleSystemState::Stopped );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            WP_LOG_ERROR( "ParticleSystem::stop - Exception occurred while stopping particle system" );
        }
    }

    void ParticleSystem::pause()
    {
        try
        {
            if( auto particleSystem = getParticleSystem() )
            {
                particleSystem->setState( render::ParticleSystemState::Paused );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            WP_LOG_ERROR( "ParticleSystem::pause - Exception occurred while pausing particle system" );
        }
    }

    void ParticleSystem::resume()
    {
        play();
    }

    void ParticleSystem::rebuild()
    {
        const auto wasPlaying = isPlaying();
        destroyGraphicsParticleSystem();
        createGraphicsParticleSystem();

        if( wasPlaying )
        {
            play();
        }
    }

    bool ParticleSystem::isPlaying() const
    {
        auto particleSystem = getParticleSystem();
        return particleSystem && particleSystem->getState() == render::ParticleSystemState::Started;
    }

    void ParticleSystem::createGraphicsParticleSystem()
    {
        if( auto particleSystem = getParticleSystem() )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "ParticleSystem::createGraphicsParticleSystem - ApplicationManager is null" );
            return;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "ParticleSystem::createGraphicsParticleSystem - GraphicsSystem is null" );
            return;
        }

        auto smgr = graphicsSystem->getGraphicsScenePtr();
        if( !smgr )
        {
            WP_LOG_ERROR( "ParticleSystem::createGraphicsParticleSystem - GraphicsScene is null" );
            return;
        }

        try
        {
            auto particleSystem = smgr->addGraphicsObjectByType<render::IParticleSystem>();
            if( particleSystem )
            {
                const auto templateName = getTemplateName();
                particleSystem->setTemplateName( templateName );

                if( !templateName.empty() && particleSystem->isLoaded() )
                {
                    particleSystem->reload( nullptr );
                    if( !particleSystem->isLoaded() )
                    {
                        WP_LOG_ERROR(
                            "ParticleSystem::createGraphicsParticleSystem - Failed to reload "
                            "template: " +
                            templateName );
                        smgr->removeGraphicsObject( particleSystem );
                        return;
                    }
                }

                auto rootNode = smgr->getRootSceneNode();
                if( !rootNode )
                {
                    WP_LOG_ERROR(
                        "ParticleSystem::createGraphicsParticleSystem - RootSceneNode is null" );
                    smgr->removeGraphicsObject( particleSystem );
                    return;
                }

                auto node = rootNode->addChildSceneNode();
                if( !node )
                {
                    WP_LOG_ERROR(
                        "ParticleSystem::createGraphicsParticleSystem - Failed to create child scene "
                        "node" );
                    smgr->removeGraphicsObject( particleSystem );
                    return;
                }

                setGraphicsNode( node );
                setGraphicsObject( particleSystem );
                setParticleSystem( particleSystem );

                applyParticleSystemProperties();
                updateTransform();
                updateVisibility();

                if( getPlayOnLoad() )
                {
                    particleSystem->setState( render::ParticleSystemState::Started );
                }

                // Attaching queues native creation. Keep it last so the render task
                // sees a fully configured preset on its first load.
                node->attachObject( particleSystem );
            }
            else
            {
                WP_LOG_ERROR(
                    "ParticleSystem::createGraphicsParticleSystem - Failed to add IParticleSystem "
                    "graphics object" );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            WP_LOG_ERROR(
                "ParticleSystem::createGraphicsParticleSystem - Exception occurred during renderer "
                "object creation" );
        }
    }

    void ParticleSystem::destroyGraphicsParticleSystem()
    {
        try
        {
            if( auto particleSystem = getParticleSystem() )
            {
                if( auto graphicsNode = getGraphicsNode() )
                {
                    graphicsNode->detachObject( particleSystem );
                }

                if( auto smgr = particleSystem->getCreator() )
                {
                    smgr->removeGraphicsObject( particleSystem );
                }
                else
                {
                    WP_LOG(
                        "ParticleSystem::destroyGraphicsParticleSystem - ParticleSystem creator is "
                        "null" );
                }
            }

            if( auto graphicsNode = getGraphicsNode() )
            {
                if( auto smgr = graphicsNode->getCreator() )
                {
                    smgr->removeSceneNode( graphicsNode );
                }
                else
                {
                    WP_LOG(
                        "ParticleSystem::destroyGraphicsParticleSystem - GraphicsNode creator is null" );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            WP_LOG_ERROR(
                "ParticleSystem::destroyGraphicsParticleSystem - Exception occurred during renderer "
                "cleanup" );
        }

        // Always release the component's handles, even if the renderer has
        // already been torn down or reported an error during cleanup.
        setParticleSystem( nullptr );
        setGraphicsObject( nullptr );
        setGraphicsNode( nullptr );
    }

    void ParticleSystem::applyParticleSystemProperties()
    {
        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setTemplateName( getTemplateName() );
            particleSystem->setScale( getScale() );
            particleSystem->setFastForward( getFastForwardTime(), getFastForwardInterval() );
            applyRendererProperties();
        }
    }

    void ParticleSystem::applyRendererProperties()
    {
        auto particleSystem = getParticleSystem();
        if( !particleSystem )
        {
            return;
        }

        auto properties = workphone::make_ptr<Properties>();
        properties->setProperty( render::ParticleSystem::TemplateNameStr, getTemplateName() );
        properties->setProperty( render::ParticleSystem::ScaleStr, getScale() );
        properties->setProperty( render::ParticleSystem::FastForwardTimeStr, getFastForwardTime() );
        properties->setProperty( render::ParticleSystem::FastForwardIntervalStr,
                                 getFastForwardInterval() );
        properties->setProperty( render::ParticleSystem::StartLifetimeStr, getStartLifetime() );
        properties->setProperty( render::ParticleSystem::StartSizeStr, getStartSize() );
        properties->setProperty( render::ParticleSystem::RateStr, getRate() );
        properties->setProperty( render::ParticleSystem::RateVarianceStr, getRateVariance() );
        properties->setProperty( render::ParticleSystem::AngleStr, getAngle() );
        properties->setProperty( render::ParticleSystem::AngleVarianceStr, getAngleVariance() );
        properties->setProperty( render::ParticleSystem::ShapeTypeStr, getShapeType() );
        properties->setProperty( render::ParticleSystem::ShapeSizeStr, getShapeSize() );
        properties->setProperty( render::ParticleSystem::ShapeSizeVarianceStr, getShapeSizeVariance() );
        properties->setProperty( render::ParticleSystem::DurationStr, getDuration() );
        properties->setProperty( render::ParticleSystem::LoopingStr, isLooping() );

        particleSystem->setProperties( properties );
    }

    // --- Lifetime / duration -----------------------------------------------

    f32 ParticleSystem::getLifetime() const
    {
        return m_lifetime;
    }

    void ParticleSystem::setLifetime( f32 lifetime )
    {
        m_lifetime = Math<f32>::max( 0.0f, lifetime );
        applyParticleSystemProperties();
    }

    f32 ParticleSystem::getDuration() const
    {
        return m_duration;
    }

    void ParticleSystem::setDuration( f32 duration )
    {
        m_duration = Math<f32>::max( 0.0f, duration );
        applyParticleSystemProperties();
    }

    bool ParticleSystem::isLooping() const
    {
        return m_looping;
    }

    void ParticleSystem::setLooping( bool looping )
    {
        m_looping = looping;
        applyParticleSystemProperties();
    }

    // --- Fast-forward -------------------------------------------------------

    f32 ParticleSystem::getFastForwardTime() const
    {
        return m_fastForwardTime;
    }

    void ParticleSystem::setFastForwardTime( f32 time )
    {
        m_fastForwardTime = Math<f32>::max( 0.0f, time );
        if( auto ps = getParticleSystem() )
            ps->setFastForward( m_fastForwardTime, m_fastForwardInterval );
    }

    f32 ParticleSystem::getFastForwardInterval() const
    {
        return m_fastForwardInterval;
    }

    void ParticleSystem::setFastForwardInterval( f32 interval )
    {
        m_fastForwardInterval = Math<f32>::max( 0.0f, interval );
        if( auto ps = getParticleSystem() )
            ps->setFastForward( m_fastForwardTime, m_fastForwardInterval );
    }

    // --- Start ranges -------------------------------------------------------

    Vector2<real_Num> ParticleSystem::getStartLifetime() const
    {
        return m_startLifetime;
    }

    void ParticleSystem::setStartLifetime( const Vector2<real_Num> &startLifetime )
    {
        m_startLifetime.x = Math<real_Num>::max( static_cast<real_Num>( 0 ), startLifetime.x );
        m_startLifetime.y = Math<real_Num>::max( m_startLifetime.x, startLifetime.y );
        applyParticleSystemProperties();
    }

    Vector2<real_Num> ParticleSystem::getStartSize() const
    {
        return m_startSize;
    }

    void ParticleSystem::setStartSize( const Vector2<real_Num> &startSize )
    {
        m_startSize.x = Math<real_Num>::max( static_cast<real_Num>( 0 ), startSize.x );
        m_startSize.y = Math<real_Num>::max( m_startSize.x, startSize.y );
        applyParticleSystemProperties();
    }

    Vector3<real_Num> ParticleSystem::getScale() const
    {
        return m_scale;
    }

    void ParticleSystem::setScale( const Vector3<real_Num> &scale )
    {
        m_scale = scale;
        if( auto ps = getParticleSystem() )
            ps->setScale( m_scale );
    }

    // --- Emission -----------------------------------------------------------

    f32 ParticleSystem::getRate() const
    {
        return m_rate;
    }

    void ParticleSystem::setRate( f32 rate )
    {
        m_rate = Math<f32>::max( 0.0f, rate );
        applyParticleSystemProperties();
    }

    f32 ParticleSystem::getRateVariance() const
    {
        return m_rateVariance;
    }

    void ParticleSystem::setRateVariance( f32 rateVariance )
    {
        m_rateVariance = Math<f32>::max( 0.0f, rateVariance );
        applyParticleSystemProperties();
    }

    f32 ParticleSystem::getAngle() const
    {
        return m_angle;
    }

    void ParticleSystem::setAngle( f32 angle )
    {
        m_angle = angle;
        applyParticleSystemProperties();
    }

    f32 ParticleSystem::getAngleVariance() const
    {
        return m_angleVariance;
    }

    void ParticleSystem::setAngleVariance( f32 angleVariance )
    {
        m_angleVariance = Math<f32>::max( 0.0f, angleVariance );
        applyParticleSystemProperties();
    }

    // --- Emitter shape ------------------------------------------------------

    f32 ParticleSystem::getShapeType() const
    {
        return m_shapeType;
    }

    void ParticleSystem::setShapeType( f32 shapeType )
    {
        m_shapeType = shapeType;
        applyParticleSystemProperties();
    }

    f32 ParticleSystem::getShapeSize() const
    {
        return m_shapeSize;
    }

    void ParticleSystem::setShapeSize( f32 shapeSize )
    {
        m_shapeSize = Math<f32>::max( 0.0f, shapeSize );
        applyParticleSystemProperties();
    }

    f32 ParticleSystem::getShapeSizeVariance() const
    {
        return m_shapeSizeVariance;
    }

    void ParticleSystem::setShapeSizeVariance( f32 shapeSizeVariance )
    {
        m_shapeSizeVariance = Math<f32>::max( 0.0f, shapeSizeVariance );
        applyParticleSystemProperties();
    }

}  // namespace workphone::scene
