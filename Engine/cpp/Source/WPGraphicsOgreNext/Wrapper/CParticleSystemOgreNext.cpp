#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticle.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/WPGraphicsOgreNextTypes.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreHlmsManager.h>
#include <OgreHlmsUnlit.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreParticleEmitter.h>

namespace
{
    constexpr auto DefaultParticleMaterialName = "Workphone/Particles/Default";

    Ogre::HlmsDatablock *ensureDefaultParticleDatablock()
    {
        auto root = Ogre::Root::getSingletonPtr();
        if( !root )
        {
            return nullptr;
        }

        auto hlmsManager = root->getHlmsManager();
        if( !hlmsManager )
        {
            return nullptr;
        }

        if( auto datablock =
                hlmsManager->getDatablockNoDefault( DefaultParticleMaterialName ) )
        {
            return datablock;
        }

        auto hlmsUnlit =
            static_cast<Ogre::HlmsUnlit *>( hlmsManager->getHlms( Ogre::HLMS_UNLIT ) );
        if( !hlmsUnlit )
        {
            return nullptr;
        }

        Ogre::HlmsMacroblock macroblock;
        macroblock.mDepthWrite = false;

        Ogre::HlmsBlendblock blendblock;
        blendblock.setBlendType( Ogre::SBT_TRANSPARENT_ALPHA );

        Ogre::HlmsParamVec params;
        auto datablock = static_cast<Ogre::HlmsUnlitDatablock *>(
            hlmsUnlit->createDatablock( DefaultParticleMaterialName,
                                        DefaultParticleMaterialName, macroblock, blendblock,
                                        params ) );
        datablock->setUseColour( true );
        datablock->setColour( Ogre::ColourValue::White );
        return datablock;
    }
}  // namespace

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CParticleSystemOgreNext, ParticleSystem );

    const String CParticleSystemOgreNext::ResourceGroupNameStr = "resourceGroupName";
    const String CParticleSystemOgreNext::InitialParticleQuotaStr = "initialParticleQuota";
    const String CParticleSystemOgreNext::ParticleQuotaStr = "particleQuota";
    const String CParticleSystemOgreNext::EmittedEmitterQuotaStr = "emittedEmitterQuota";
    const String CParticleSystemOgreNext::MaterialNameStr = "materialName";
    const String CParticleSystemOgreNext::RendererNameStr = "rendererName";
    const String CParticleSystemOgreNext::DefaultDimensionsStr = "defaultDimensions";
    const String CParticleSystemOgreNext::SpeedFactorStr = "speedFactor";
    const String CParticleSystemOgreNext::IterationIntervalStr = "iterationInterval";
    const String CParticleSystemOgreNext::NonVisibleUpdateTimeoutStr = "nonVisibleUpdateTimeout";
    const String CParticleSystemOgreNext::CullIndividuallyStr = "cullIndividually";
    const String CParticleSystemOgreNext::SortingEnabledStr = "sortingEnabled";
    const String CParticleSystemOgreNext::KeepParticlesInLocalSpaceStr = "keepParticlesInLocalSpace";
    const String CParticleSystemOgreNext::BoundsAutoUpdatedStr = "boundsAutoUpdated";
    const String CParticleSystemOgreNext::BoundsUpdateTimeStr = "boundsUpdateTime";
    const String CParticleSystemOgreNext::EmittingStr = "emitting";
    const String CParticleSystemOgreNext::TranslateParticleDirectionIntoWorldSpaceStr =
        "translateParticleDirectionIntoWorldSpace";

    CParticleSystemOgreNext::CParticleSystemOgreNext()
    {
        setupStateObject();

#if !WP_FINAL
        String debugStr = getDebugStr() + DebugUtil::getStackTrace();
        setDebugStr( debugStr );
#endif
    }

    CParticleSystemOgreNext::CParticleSystemOgreNext( IGraphicsScene *creator )
    {
        setCreator( creator );
        setupStateObject();

#if !WP_FINAL
        String debugStr = getDebugStr() + DebugUtil::getStackTrace();
        setDebugStr( debugStr );
#endif
    }

    CParticleSystemOgreNext::~CParticleSystemOgreNext()
    {
        if( auto stateContext = getStateContext() )
        {
            if( stateContext->getStateById( getId() ) )
            {
                stateContext->removeStatesById( getId() );
            }
        }
    }

    void CParticleSystemOgreNext::setupStateObject()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        auto scene = getCreator();
        if( !graphicsSystem || !scene )
        {
            return;
        }

        auto factoryManager = graphicsSystem->getFactoryManagerPtr();
        auto stateContext = scene->getGraphicsObjectContextPtr( getTypeInfo() );
        if( !factoryManager || !stateContext )
        {
            return;
        }

        if( auto currentStateContext = getStateContext() )
        {
            if( currentStateContext.get() != stateContext )
            {
                currentStateContext->removeStatesById( getId() );
            }
        }

        setStateContext( stateContext );

        if( !stateContext->getStateById( getId() ) )
        {
            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );

            auto stateData = factoryManager->make_ptr<GraphicsObjectData>();
            state->setData( stateData );
            stateContext->addState( state );
        }
    }

    void CParticleSystemOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            ParticleSystem::load( data );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            ScopedLock lock( graphicsSystem );

            Ogre::SceneManager *smgr = nullptr;

            if( auto creator = getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &smgr ) );
            }

            if( smgr )
            {
                if( !getParticleSystem() )
                {
                    auto particleSystem = createOgreParticleSystem( smgr );
                    if( !particleSystem )
                    {
                        setLoadingState( LoadingState::Error );
                        return;
                    }

                    setGraphicsObject( particleSystem );
                }

                if( auto owner = getOwner() )
                {
                    Ogre::SceneNode *ownerNode = nullptr;
                    owner->_getObject( reinterpret_cast<void **>( &ownerNode ) );

                    if( ownerNode && !getParticleSystem()->isAttached() )
                    {
                        getParticleSystem()->setStatic( owner->isStatic() );
                        ownerNode->attachObject( getParticleSystem() );
                    }
                }

                // Ogre skips particle updates while a system is unattached, so settings
                // that fast-forward initial emission must be applied after attachment.
                applyParticleSystemSettings();
                setState( getState() );
            }
            else
            {
                setLoadingState( LoadingState::Error );
                return;
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Error );
        }
    }

    void CParticleSystemOgreNext::reload( SmartPtr<ISharedObject> data )
    {
        auto creator = getCreator();
        auto owner = getOwner();
        auto state = getState();

        unload( data );

        setCreator( creator );
        load( data );

        if( isLoaded() )
        {
            if( auto particleSystem = getParticleSystem() )
            {
                if( owner )
                {
                    Ogre::SceneNode *ownerNode = nullptr;
                    owner->_getObject( reinterpret_cast<void **>( &ownerNode ) );

                    if( ownerNode )
                    {
                        if( particleSystem->isAttached() )
                        {
                            particleSystem->detachFromParent();
                        }

                        particleSystem->setStatic( owner->isStatic() );
                        ownerNode->attachObject( particleSystem );
                        setOwner( owner );
                    }
                }
            }

            setState( state );
        }
    }

    void CParticleSystemOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            ScopedLock lock( graphicsSystem );

            Ogre::SceneManager *smgr = nullptr;

            if( auto creator = getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &smgr ) );
            }

            if( auto particleSystem = getParticleSystem() )
            {
                particleSystem->detachFromParent();

                if( smgr )
                {
                    smgr->destroyParticleSystem( particleSystem );
                }

                setGraphicsObject( nullptr );
            }

            ParticleSystem::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IGraphicsObject> CParticleSystemOgreNext::clone( const String &name ) const
    {
        auto particleSystem = workphone::make_ptr<CParticleSystemOgreNext>();
        particleSystem->setName( !name.empty() ? name : getName() );
        particleSystem->setTemplateName( getTemplateName() );
        particleSystem->setScale( getScale() );
        particleSystem->setFastForward( getFastForwardTime(), getFastForwardInterval() );
        particleSystem->setStartLifetime( getStartLifetime() );
        particleSystem->setStartSize( getStartSize() );
        particleSystem->setRate( getRate() );
        particleSystem->setRateVariance( getRateVariance() );
        particleSystem->setAngle( getAngle() );
        particleSystem->setAngleVariance( getAngleVariance() );
        particleSystem->setShapeType( getShapeType() );
        particleSystem->setShapeSize( getShapeSize() );
        particleSystem->setShapeSizeVariance( getShapeSizeVariance() );
        particleSystem->setDuration( getDuration() );
        particleSystem->setLooping( getLooping() );
        particleSystem->setResourceGroupName( getResourceGroupName() );
        particleSystem->setInitialParticleQuota( getInitialParticleQuota() );
        particleSystem->setParticleQuota( getParticleQuota() );
        particleSystem->setEmittedEmitterQuota( getEmittedEmitterQuota() );
        particleSystem->setMaterialName( getMaterialName() );
        particleSystem->setRendererName( getRendererName() );
        particleSystem->setDefaultDimensions( getDefaultDimensions() );
        particleSystem->setSpeedFactor( getSpeedFactor() );
        particleSystem->setIterationInterval( getIterationInterval() );
        particleSystem->setNonVisibleUpdateTimeout( getNonVisibleUpdateTimeout() );
        particleSystem->setCullIndividually( getCullIndividually() );
        particleSystem->setSortingEnabled( getSortingEnabled() );
        particleSystem->setKeepParticlesInLocalSpace( getKeepParticlesInLocalSpace() );
        particleSystem->setBoundsAutoUpdated( getBoundsAutoUpdated() );
        particleSystem->setBoundsUpdateTime( getBoundsUpdateTime() );
        particleSystem->setEmitting( getEmitting() );
        particleSystem->setTranslateParticleDirectionIntoWorldSpace(
            getTranslateParticleDirectionIntoWorldSpace() );
        return particleSystem;
    }

    void CParticleSystemOgreNext::_getObject( void **ppObject ) const
    {
        *ppObject = (Ogre::ParticleSystem *)getGraphicsObject();
    }

    void CParticleSystemOgreNext::setState( ParticleSystemState state )
    {
        const auto previousState = getState();
        ParticleSystem::setState( state );

        switch( state )
        {
        case ParticleSystemState::Started:
        {
            if( auto particleSystem = getParticleSystem() )
            {
                particleSystem->setSpeedFactor( getSpeedFactor() );

                if( previousState == ParticleSystemState::Stopped ||
                    previousState == ParticleSystemState::StoppedFade )
                {
                    particleSystem->clear();

                    for( auto i = 0u; i < particleSystem->getNumEmitters(); ++i )
                    {
                        particleSystem->getEmitter( static_cast<unsigned short>( i ) )
                            ->setEnabled( true );
                    }
                }
            }

            setEmitting( true );
        }
        break;
        case ParticleSystemState::Stopped:
        {
            setEmitting( false );

            if( auto particleSystem = getParticleSystem() )
            {
                particleSystem->clear();
            }
        }
        break;
        case ParticleSystemState::Paused:
        case ParticleSystemState::PausedForTime:
        {
            setEmitting( false );

            if( auto particleSystem = getParticleSystem() )
            {
                particleSystem->setSpeedFactor( 0.0f );
            }
        }
        break;
        case ParticleSystemState::StoppedFade:
            setEmitting( false );
            break;
        }
    }

    size_t CParticleSystemOgreNext::getNumParticles() const
    {
        if( auto particleSystem = getParticleSystem() )
        {
            return particleSystem->getNumParticles();
        }

        return 0u;
    }

    size_t CParticleSystemOgreNext::getNumEmitters() const
    {
        if( auto particleSystem = getParticleSystem() )
        {
            return particleSystem->getNumEmitters();
        }

        return 0u;
    }

    SmartPtr<IParticle> CParticleSystemOgreNext::addParticle()
    {
        if( isLoaded() )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = graphicsSystem->getFactoryManager();

            if( auto particleSystem = getParticleSystem() )
            {
                auto p = particleSystem->createParticle();

                auto particle = workphone::make_ptr<CParticle>();
                particle->setParticle( p );
                m_particles.push_back( particle );

                return particle;
            }
        }

        return nullptr;
    }

    Ogre::ParticleSystem *CParticleSystemOgreNext::getParticleSystem() const
    {
        return (Ogre::ParticleSystem *)getGraphicsObject();
    }

    SmartPtr<Properties> CParticleSystemOgreNext::getProperties() const
    {
        auto properties = ParticleSystem::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( ResourceGroupNameStr, getResourceGroupName() );
        properties->setProperty( InitialParticleQuotaStr,
                                 static_cast<u32>( getInitialParticleQuota() ) );
        properties->setProperty( ParticleQuotaStr, static_cast<u32>( getParticleQuota() ) );
        properties->setProperty( EmittedEmitterQuotaStr, static_cast<u32>( getEmittedEmitterQuota() ) );
        properties->setProperty( MaterialNameStr, getMaterialName() );
        properties->setProperty( RendererNameStr, getRendererName() );
        properties->setProperty( DefaultDimensionsStr, getDefaultDimensions() );
        properties->setProperty( SpeedFactorStr, getSpeedFactor() );
        properties->setProperty( IterationIntervalStr, getIterationInterval() );
        properties->setProperty( NonVisibleUpdateTimeoutStr, getNonVisibleUpdateTimeout() );
        properties->setProperty( CullIndividuallyStr, getCullIndividually() );
        properties->setProperty( SortingEnabledStr, getSortingEnabled() );
        properties->setProperty( KeepParticlesInLocalSpaceStr, getKeepParticlesInLocalSpace() );
        properties->setProperty( BoundsAutoUpdatedStr, getBoundsAutoUpdated() );
        properties->setProperty( BoundsUpdateTimeStr, getBoundsUpdateTime() );
        properties->setProperty( EmittingStr, getEmitting() );
        properties->setProperty( TranslateParticleDirectionIntoWorldSpaceStr,
                                 getTranslateParticleDirectionIntoWorldSpace() );

        return properties;
    }

    void CParticleSystemOgreNext::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        auto oldTemplateName = getTemplateName();
        auto oldResourceGroupName = getResourceGroupName();
        auto oldInitialParticleQuota = getInitialParticleQuota();
        auto oldRendererName = getRendererName();

        ParticleSystem::setProperties( properties );

        auto resourceGroupName = getResourceGroupName();
        auto initialParticleQuota = static_cast<u32>( getInitialParticleQuota() );
        auto particleQuota = static_cast<u32>( getParticleQuota() );
        auto emittedEmitterQuota = static_cast<u32>( getEmittedEmitterQuota() );
        auto materialName = getMaterialName();
        auto rendererName = getRendererName();
        auto defaultDimensions = getDefaultDimensions();
        auto speedFactor = getSpeedFactor();
        auto iterationInterval = getIterationInterval();
        auto nonVisibleUpdateTimeout = getNonVisibleUpdateTimeout();
        auto cullIndividually = getCullIndividually();
        auto sortingEnabled = getSortingEnabled();
        auto keepParticlesInLocalSpace = getKeepParticlesInLocalSpace();
        auto boundsAutoUpdated = getBoundsAutoUpdated();
        auto boundsUpdateTime = getBoundsUpdateTime();
        auto emitting = getEmitting();
        auto translateParticleDirectionIntoWorldSpace = getTranslateParticleDirectionIntoWorldSpace();

        properties->getPropertyValue( ResourceGroupNameStr, resourceGroupName );
        properties->getPropertyValue( InitialParticleQuotaStr, initialParticleQuota );
        properties->getPropertyValue( ParticleQuotaStr, particleQuota );
        properties->getPropertyValue( EmittedEmitterQuotaStr, emittedEmitterQuota );
        properties->getPropertyValue( MaterialNameStr, materialName );
        properties->getPropertyValue( RendererNameStr, rendererName );
        properties->getPropertyValue( DefaultDimensionsStr, defaultDimensions );
        properties->getPropertyValue( SpeedFactorStr, speedFactor );
        properties->getPropertyValue( IterationIntervalStr, iterationInterval );
        properties->getPropertyValue( NonVisibleUpdateTimeoutStr, nonVisibleUpdateTimeout );
        properties->getPropertyValue( CullIndividuallyStr, cullIndividually );
        properties->getPropertyValue( SortingEnabledStr, sortingEnabled );
        properties->getPropertyValue( KeepParticlesInLocalSpaceStr, keepParticlesInLocalSpace );
        properties->getPropertyValue( BoundsAutoUpdatedStr, boundsAutoUpdated );
        properties->getPropertyValue( BoundsUpdateTimeStr, boundsUpdateTime );
        properties->getPropertyValue( EmittingStr, emitting );
        properties->getPropertyValue( TranslateParticleDirectionIntoWorldSpaceStr,
                                      translateParticleDirectionIntoWorldSpace );

        setResourceGroupName( resourceGroupName );
        setInitialParticleQuota( initialParticleQuota );
        setParticleQuota( particleQuota );
        setEmittedEmitterQuota( emittedEmitterQuota );
        setMaterialName( materialName );
        setRendererName( rendererName );
        setDefaultDimensions( defaultDimensions );
        setSpeedFactor( speedFactor );
        setIterationInterval( iterationInterval );
        setNonVisibleUpdateTimeout( nonVisibleUpdateTimeout );
        setCullIndividually( cullIndividually );
        setSortingEnabled( sortingEnabled );
        setKeepParticlesInLocalSpace( keepParticlesInLocalSpace );
        setBoundsAutoUpdated( boundsAutoUpdated );
        setBoundsUpdateTime( boundsUpdateTime );
        setEmitting( emitting );
        setTranslateParticleDirectionIntoWorldSpace( translateParticleDirectionIntoWorldSpace );

        auto recreateRequired =
            oldTemplateName != getTemplateName() || oldResourceGroupName != getResourceGroupName() ||
            oldInitialParticleQuota != getInitialParticleQuota() || oldRendererName != getRendererName();

        if( recreateRequired && isLoaded() )
        {
            reload( nullptr );
        }
        else
        {
            applyParticleSystemSettings();
        }

        setState( getState() );
    }

    Ogre::ParticleSystem *CParticleSystemOgreNext::createOgreParticleSystem(
        Ogre::SceneManager *sceneManager ) const
    {
        Ogre::ParticleSystem *ps = nullptr;

        try
        {
            if( !sceneManager )
            {
                return nullptr;
            }

            auto templateName = getTemplateName();
            if( !templateName.empty() )
            {
                ps = sceneManager->createParticleSystem( templateName.c_str() );
            }
            else
            {
                auto quota = getInitialParticleQuota() < static_cast<size_t>( 1u )
                                 ? static_cast<size_t>( 1u )
                                 : getInitialParticleQuota();

                Ogre::String resourceGroupName = getResourceGroupName();
                ps = sceneManager->createParticleSystem( quota, resourceGroupName );
            }
        }
        catch( Ogre::Exception &e )
        {
            auto message = e.getFullDescription();
            WP_LOG_ERROR( message.c_str() );

            if( ps )
            {
                sceneManager->destroyParticleSystem( ps );
                ps = nullptr;
            }
        }

        return ps;
    }

    void CParticleSystemOgreNext::applyParticleSystemSettings()
    {
        auto particleSystem = getParticleSystem();
        if( !particleSystem )
        {
            return;
        }

        if( !getRendererName().empty() && particleSystem->getRendererName() != getRendererName() )
        {
            particleSystem->setRenderer( getRendererName().c_str() );
        }

        if( getMaterialName() == DefaultParticleMaterialName )
        {
            ensureDefaultParticleDatablock();
        }

        if( !getMaterialName().empty() )
        {
            particleSystem->setMaterialName( getMaterialName().c_str(), getResourceGroupName().c_str() );
        }

        particleSystem->setParticleQuota( getParticleQuota() );
        particleSystem->setEmittedEmitterQuota( getEmittedEmitterQuota() );
        particleSystem->setDefaultDimensions( getDefaultDimensions().x, getDefaultDimensions().y );
        particleSystem->setSpeedFactor( getSpeedFactor() );
        particleSystem->setIterationInterval( getIterationInterval() );
        particleSystem->setNonVisibleUpdateTimeout( getNonVisibleUpdateTimeout() );
        particleSystem->setCullIndividually( getCullIndividually() );
        particleSystem->setSortingEnabled( getSortingEnabled() );
        particleSystem->setKeepParticlesInLocalSpace( getKeepParticlesInLocalSpace() );
        particleSystem->setVisible( isVisible() );
        particleSystem->setVisibilityFlags( getVisibilityFlags() );
        particleSystem->setRenderQueueGroup(
            static_cast<Ogre::uint8>( RenderQueueGroupID::RENDER_QUEUE_MAIN ) );
        particleSystem->setBoundsAutoUpdated( getBoundsAutoUpdated(), getBoundsUpdateTime() );
        particleSystem->setEmitting( getEmitting() );
        particleSystem->setTranslateParticleDirectionIntoWorldSpace(
            getTranslateParticleDirectionIntoWorldSpace() );

        configureDefaultEmitter();

        if( getFastForwardTime() > 0.0f )
        {
            auto interval = getFastForwardInterval() > 0.0f ? getFastForwardInterval() : 0.1f;
            particleSystem->fastForward( getFastForwardTime(), interval );
        }
    }

    void CParticleSystemOgreNext::configureDefaultEmitter()
    {
        auto particleSystem = getParticleSystem();
        if( !particleSystem || !getTemplateName().empty() )
        {
            return;
        }

        auto emitter = particleSystem->getNumEmitters() > 0 ? particleSystem->getEmitter( 0 )
                                                            : particleSystem->addEmitter( "Point" );
        if( !emitter )
        {
            return;
        }

        emitter->setEmissionRate( getRate() );
        const auto maxAngle =
            Math<f32>::min( 180.0f, Math<f32>::max( 0.0f, getAngle() + getAngleVariance() ) );
        emitter->setAngle( Ogre::Degree( maxAngle ) );
        emitter->setDirection( Ogre::Vector3::UNIT_Y );

        const auto speed = getShapeSize() > 0.0f ? getShapeSize() : 1.0f;
        const auto minSpeed = Math<f32>::max( 0.0f, speed - getShapeSizeVariance() );
        const auto maxSpeed = Math<f32>::max( minSpeed, speed + getShapeSizeVariance() );
        emitter->setParticleVelocity( minSpeed, maxSpeed );

        const auto startLifetime = getStartLifetime();
        const auto minLifetime = Math<real_Num>::max( static_cast<real_Num>( 0 ), startLifetime.x );
        const auto maxLifetime = Math<real_Num>::max( minLifetime, startLifetime.y );
        emitter->setTimeToLive( minLifetime, maxLifetime );

        const auto startSize = getStartSize();
        const auto minSize = Math<real_Num>::max( static_cast<real_Num>( 0 ), startSize.x );
        const auto maxSize = Math<real_Num>::max( minSize, startSize.y );
        const auto defaultSize = ( minSize + maxSize ) * static_cast<real_Num>( 0.5 );
        particleSystem->setDefaultDimensions( defaultSize, defaultSize );

        emitter->setRepeatDelay( 0.0f );
        emitter->setDuration( getLooping() ? 0.0f : Math<f32>::max( 0.0f, getDuration() ) );
    }

    String CParticleSystemOgreNext::getResourceGroupName() const
    {
        return m_resourceGroupName;
    }

    void CParticleSystemOgreNext::setResourceGroupName( const String &resourceGroupName )
    {
        m_resourceGroupName = resourceGroupName.empty() ? String( "General" ) : resourceGroupName;
    }

    size_t CParticleSystemOgreNext::getInitialParticleQuota() const
    {
        return m_initialParticleQuota;
    }

    void CParticleSystemOgreNext::setInitialParticleQuota( size_t initialParticleQuota )
    {
        m_initialParticleQuota = initialParticleQuota < static_cast<size_t>( 1u )
                                     ? static_cast<size_t>( 1u )
                                     : initialParticleQuota;
    }

    size_t CParticleSystemOgreNext::getParticleQuota() const
    {
        return m_particleQuota;
    }

    void CParticleSystemOgreNext::setParticleQuota( size_t particleQuota )
    {
        m_particleQuota =
            particleQuota < static_cast<size_t>( 1u ) ? static_cast<size_t>( 1u ) : particleQuota;

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setParticleQuota( m_particleQuota );
        }
    }

    size_t CParticleSystemOgreNext::getEmittedEmitterQuota() const
    {
        return m_emittedEmitterQuota;
    }

    void CParticleSystemOgreNext::setEmittedEmitterQuota( size_t emittedEmitterQuota )
    {
        m_emittedEmitterQuota = emittedEmitterQuota;

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setEmittedEmitterQuota( m_emittedEmitterQuota );
        }
    }

    String CParticleSystemOgreNext::getMaterialName() const
    {
        return m_materialName;
    }

    void CParticleSystemOgreNext::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;

        if( auto particleSystem = getParticleSystem() )
        {
            if( !m_materialName.empty() )
            {
                particleSystem->setMaterialName( m_materialName.c_str(),
                                                 getResourceGroupName().c_str() );
            }
        }
    }

    String CParticleSystemOgreNext::getRendererName() const
    {
        return m_rendererName;
    }

    void CParticleSystemOgreNext::setRendererName( const String &rendererName )
    {
        m_rendererName = rendererName;
    }

    Vector2<real_Num> CParticleSystemOgreNext::getDefaultDimensions() const
    {
        return m_defaultDimensions;
    }

    void CParticleSystemOgreNext::setDefaultDimensions( const Vector2<real_Num> &defaultDimensions )
    {
        m_defaultDimensions.x = Math<real_Num>::max( static_cast<real_Num>( 0 ), defaultDimensions.x );
        m_defaultDimensions.y = Math<real_Num>::max( static_cast<real_Num>( 0 ), defaultDimensions.y );

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setDefaultDimensions( m_defaultDimensions.x, m_defaultDimensions.y );
        }
    }

    f32 CParticleSystemOgreNext::getSpeedFactor() const
    {
        return m_speedFactor;
    }

    void CParticleSystemOgreNext::setSpeedFactor( f32 speedFactor )
    {
        m_speedFactor = Math<f32>::max( 0.0f, speedFactor );

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setSpeedFactor( m_speedFactor );
        }
    }

    f32 CParticleSystemOgreNext::getIterationInterval() const
    {
        return m_iterationInterval;
    }

    void CParticleSystemOgreNext::setIterationInterval( f32 iterationInterval )
    {
        m_iterationInterval = Math<f32>::max( 0.0f, iterationInterval );

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setIterationInterval( m_iterationInterval );
        }
    }

    f32 CParticleSystemOgreNext::getNonVisibleUpdateTimeout() const
    {
        return m_nonVisibleUpdateTimeout;
    }

    void CParticleSystemOgreNext::setNonVisibleUpdateTimeout( f32 nonVisibleUpdateTimeout )
    {
        m_nonVisibleUpdateTimeout = Math<f32>::max( 0.0f, nonVisibleUpdateTimeout );

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setNonVisibleUpdateTimeout( m_nonVisibleUpdateTimeout );
        }
    }

    bool CParticleSystemOgreNext::getCullIndividually() const
    {
        return m_cullIndividually;
    }

    void CParticleSystemOgreNext::setCullIndividually( bool cullIndividually )
    {
        m_cullIndividually = cullIndividually;

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setCullIndividually( m_cullIndividually );
        }
    }

    bool CParticleSystemOgreNext::getSortingEnabled() const
    {
        return m_sortingEnabled;
    }

    void CParticleSystemOgreNext::setSortingEnabled( bool sortingEnabled )
    {
        m_sortingEnabled = sortingEnabled;

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setSortingEnabled( m_sortingEnabled );
        }
    }

    bool CParticleSystemOgreNext::getKeepParticlesInLocalSpace() const
    {
        return m_keepParticlesInLocalSpace;
    }

    void CParticleSystemOgreNext::setKeepParticlesInLocalSpace( bool keepParticlesInLocalSpace )
    {
        m_keepParticlesInLocalSpace = keepParticlesInLocalSpace;

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setKeepParticlesInLocalSpace( m_keepParticlesInLocalSpace );
        }
    }

    bool CParticleSystemOgreNext::getBoundsAutoUpdated() const
    {
        return m_boundsAutoUpdated;
    }

    void CParticleSystemOgreNext::setBoundsAutoUpdated( bool boundsAutoUpdated )
    {
        m_boundsAutoUpdated = boundsAutoUpdated;

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setBoundsAutoUpdated( m_boundsAutoUpdated, m_boundsUpdateTime );
        }
    }

    f32 CParticleSystemOgreNext::getBoundsUpdateTime() const
    {
        return m_boundsUpdateTime;
    }

    void CParticleSystemOgreNext::setBoundsUpdateTime( f32 boundsUpdateTime )
    {
        m_boundsUpdateTime = Math<f32>::max( 0.0f, boundsUpdateTime );

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setBoundsAutoUpdated( m_boundsAutoUpdated, m_boundsUpdateTime );
        }
    }

    bool CParticleSystemOgreNext::getEmitting() const
    {
        return m_emitting;
    }

    void CParticleSystemOgreNext::setEmitting( bool emitting )
    {
        m_emitting = emitting;

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setEmitting( m_emitting );
        }
    }

    bool CParticleSystemOgreNext::getTranslateParticleDirectionIntoWorldSpace() const
    {
        return m_translateParticleDirectionIntoWorldSpace;
    }

    void CParticleSystemOgreNext::setTranslateParticleDirectionIntoWorldSpace(
        bool translateParticleDirectionIntoWorldSpace )
    {
        m_translateParticleDirectionIntoWorldSpace = translateParticleDirectionIntoWorldSpace;

        if( auto particleSystem = getParticleSystem() )
        {
            particleSystem->setTranslateParticleDirectionIntoWorldSpace(
                m_translateParticleDirectionIntoWorldSpace );
        }
    }

}  // namespace workphone::render
