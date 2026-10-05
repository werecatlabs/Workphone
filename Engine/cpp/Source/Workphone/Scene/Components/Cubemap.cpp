#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Cubemap.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCubemap.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Core/LogManager.hpp>

#include <algorithm>
#include <cstdint>
#include <cmath>

namespace workphone::scene
{
    namespace
    {
        template <class T>
        T clampValue( T value, T minValue, T maxValue )
        {
            return std::max( minValue, std::min( value, maxValue ) );
        }

        bool isFinite( f32 value )
        {
            return std::isfinite( static_cast<double>( value ) );
        }

        f32 sanitizeFloat( f32 value, f32 fallback, f32 minValue, f32 maxValue )
        {
            if( !isFinite( value ) )
            {
                return fallback;
            }

            return clampValue( value, minValue, maxValue );
        }

        s32 sanitizeResolution( s32 resolution )
        {
            resolution = clampValue<s32>( resolution, 16, 8192 );

            s32 powerOfTwo = 16;
            while( powerOfTwo < resolution && powerOfTwo < 8192 )
            {
                powerOfTwo *= 2;
            }

            return powerOfTwo;
        }

        Cubemap::SourceType sanitizeSourceType( s32 value )
        {
            switch( static_cast<Cubemap::SourceType>( value ) )
            {
            case Cubemap::SourceType::Baked:
            case Cubemap::SourceType::Custom:
            case Cubemap::SourceType::Realtime:
                return static_cast<Cubemap::SourceType>( value );
            default:
                return Cubemap::SourceType::Realtime;
            }
        }

        Cubemap::RefreshMode sanitizeRefreshMode( s32 value )
        {
            switch( static_cast<Cubemap::RefreshMode>( value ) )
            {
            case Cubemap::RefreshMode::OnAwake:
            case Cubemap::RefreshMode::EveryFrame:
            case Cubemap::RefreshMode::ViaScripting:
            case Cubemap::RefreshMode::Interval:
                return static_cast<Cubemap::RefreshMode>( value );
            default:
                return Cubemap::RefreshMode::OnAwake;
            }
        }

        Cubemap::ProjectionMode sanitizeProjectionMode( s32 value )
        {
            switch( static_cast<Cubemap::ProjectionMode>( value ) )
            {
            case Cubemap::ProjectionMode::Infinite:
            case Cubemap::ProjectionMode::Box:
                return static_cast<Cubemap::ProjectionMode>( value );
            default:
                return Cubemap::ProjectionMode::Box;
            }
        }

        Cubemap::InfluenceShape sanitizeInfluenceShape( s32 value )
        {
            switch( static_cast<Cubemap::InfluenceShape>( value ) )
            {
            case Cubemap::InfluenceShape::Sphere:
            case Cubemap::InfluenceShape::Box:
                return static_cast<Cubemap::InfluenceShape>( value );
            default:
                return Cubemap::InfluenceShape::Sphere;
            }
        }

        Cubemap::ClearMode sanitizeClearMode( s32 value )
        {
            switch( static_cast<Cubemap::ClearMode>( value ) )
            {
            case Cubemap::ClearMode::Skybox:
            case Cubemap::ClearMode::SolidColor:
            case Cubemap::ClearMode::Transparent:
                return static_cast<Cubemap::ClearMode>( value );
            default:
                return Cubemap::ClearMode::Skybox;
            }
        }

        String getRuntimeTextureName( const Cubemap *cubemap )
        {
            const auto address = reinterpret_cast<std::uintptr_t>( cubemap );
            return "DynamicCubemap_" + StringUtil::toString( static_cast<u64>( address ) );
        }

        void configureCaptureStrategy( SmartPtr<render::IGraphicsCubemap> renderCubemap,
                                       const Cubemap &cubemap )
        {
            if( !renderCubemap )
            {
                return;
            }

            // The component owns refresh timing (OnAwake, EveryFrame, ViaScripting,
            // or Interval). The renderer only processes explicitly requested capture
            // cycles, allowing a cycle to be spread over several render frames.
            renderCubemap->setUpdateMode( render::IGraphicsCubemap::UpdateMode::OnDemand );
            renderCubemap->setUpdateInterval( 0u );
            renderCubemap->setFaceMask( cubemap.getFaceMask() );
            renderCubemap->setTimeSlicingMode(
                cubemap.getUseTimeSlicing()
                    ? render::IGraphicsCubemap::TimeSlicingMode::OneFacePerFrame
                    : render::IGraphicsCubemap::TimeSlicingMode::AllFacesAtOnce );
        }

        Vector3<real_Num> getCapturePosition( const Cubemap &cubemap )
        {
            Vector3<real_Num> position;

            if( auto actor = cubemap.getActor() )
            {
                if( auto transform = actor->getTransform() )
                {
                    position = transform->getPosition();
                }
            }

            position += Vector3<real_Num>( cubemap.getCaptureOffsetX(), cubemap.getCaptureOffsetY(),
                                           cubemap.getCaptureOffsetZ() );
            return position;
        }

        SmartPtr<render::IGraphicsSystem> getGraphicsSystem()
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
            {
                return nullptr;
            }

            return applicationManager->getGraphicsSystem();
        }

        u32 getReflectionTextureSlot()
        {
            return static_cast<u32>( PbsTextureTypes::PBSM_REFLECTION );
        }

        SmartPtr<render::IMaterial> getActorMaterial( const Cubemap &cubemap )
        {
            if( auto actor = cubemap.getActor() )
            {
                if( auto materialComponent = actor->getComponent<Material>() )
                {
                    return materialComponent->getMaterial();
                }
            }

            return nullptr;
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Cubemap, Component );

    Cubemap::Cubemap() = default;

    Cubemap::~Cubemap() = default;

    void Cubemap::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            if( data && data->isDerived<Properties>() )
            {
                setProperties( workphone::static_pointer_cast<Properties>( data ) );
            }

            resetRuntimeState();
            markDirty();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void Cubemap::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            clearCaptureRequest();
            Component::unload( data );
            Component::load( data );

            if( data && data->isDerived<Properties>() )
            {
                setProperties( workphone::static_pointer_cast<Properties>( data ) );
            }

            resetRuntimeState();
            markDirty();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void Cubemap::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            clearCaptureRequest();
            m_runtimeActive = false;
            clearMaterialCubemap();

            if( m_renderCubemap && m_renderCubemap->getEnable() )
            {
                m_renderCubemap->setEnable( false );
            }

            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Cubemap::update()
    {
        auto task = Thread::getCurrentTask();
        if( task != TaskId::Application )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return;
        }

        auto timer = applicationManager->getTimer();
        WP_ASSERT( timer );
        const auto now = timer ? static_cast<double>( timer->now() ) : 0.0;

        applyRenderState();

        if( m_runtimeActive && m_sourceType == SourceType::Realtime && m_renderCubemap &&
            consumeCaptureRequest() )
        {
            m_renderCubemap->requestUpdate();
            m_lastRefreshRequestTime = now;
            ++m_refreshCount;
        }

        if( m_runtimeActive && shouldRequestCapture( now ) )
        {
            requestCapture();
        }
    }

    void Cubemap::updateMaterials()
    {
        syncMaterialCubemap();
    }

    bool Cubemap::isValid() const
    {
        if( m_nearClipDistance <= 0.0f )
        {
            return false;
        }

        if( m_farClipDistance <= m_nearClipDistance )
        {
            return false;
        }

        if( m_resolution < 16 )
        {
            return false;
        }

        if( m_influenceShape == InfluenceShape::Sphere && m_sphereRadius <= 0.0f )
        {
            return false;
        }

        if( m_influenceShape == InfluenceShape::Box &&
            ( m_boxExtentX <= 0.0f || m_boxExtentY <= 0.0f || m_boxExtentZ <= 0.0f ) )
        {
            return false;
        }

        return Component::isValid();
    }

    auto Cubemap::getRenderCubemap() const -> SmartPtr<render::IGraphicsCubemap>
    {
        return m_renderCubemap;
    }

    void Cubemap::setRenderCubemap( SmartPtr<render::IGraphicsCubemap> renderCubemap )
    {
        m_renderCubemap = renderCubemap;
        m_renderStateDirty = true;
        applyRenderState();
    }

    bool Cubemap::isEnabled() const
    {
        return m_enabled;
    }

    void Cubemap::setEnabled( bool enabled )
    {
        if( m_enabled != enabled )
        {
            m_enabled = enabled;
            markDirty();
            applyRenderState();
        }
    }

    String Cubemap::getProbeName() const
    {
        return String( m_probeName.c_str() );
    }

    void Cubemap::setProbeName( const String &probeName )
    {
        m_probeName = probeName;
    }

    auto Cubemap::getSourceType() const -> SourceType
    {
        return m_sourceType;
    }

    void Cubemap::setSourceType( SourceType sourceType )
    {
        sourceType = sanitizeSourceType( static_cast<s32>( sourceType ) );
        if( m_sourceType != sourceType )
        {
            m_sourceType = sourceType;
            markDirty();
        }
    }

    void Cubemap::setSourceType( s32 sourceType )
    {
        setSourceType( sanitizeSourceType( sourceType ) );
    }

    auto Cubemap::getRefreshMode() const -> RefreshMode
    {
        return m_refreshMode;
    }

    void Cubemap::setRefreshMode( RefreshMode refreshMode )
    {
        refreshMode = sanitizeRefreshMode( static_cast<s32>( refreshMode ) );
        if( m_refreshMode != refreshMode )
        {
            m_refreshMode = refreshMode;
            markDirty();
        }
    }

    void Cubemap::setRefreshMode( s32 refreshMode )
    {
        setRefreshMode( sanitizeRefreshMode( refreshMode ) );
    }

    auto Cubemap::getProjectionMode() const -> ProjectionMode
    {
        return m_projectionMode;
    }

    void Cubemap::setProjectionMode( ProjectionMode projectionMode )
    {
        projectionMode = sanitizeProjectionMode( static_cast<s32>( projectionMode ) );
        if( m_projectionMode != projectionMode )
        {
            m_projectionMode = projectionMode;
            markDirty();
        }
    }

    void Cubemap::setProjectionMode( s32 projectionMode )
    {
        setProjectionMode( sanitizeProjectionMode( projectionMode ) );
    }

    auto Cubemap::getInfluenceShape() const -> InfluenceShape
    {
        return m_influenceShape;
    }

    void Cubemap::setInfluenceShape( InfluenceShape influenceShape )
    {
        influenceShape = sanitizeInfluenceShape( static_cast<s32>( influenceShape ) );
        if( m_influenceShape != influenceShape )
        {
            m_influenceShape = influenceShape;
            markDirty();
        }
    }

    void Cubemap::setInfluenceShape( s32 influenceShape )
    {
        setInfluenceShape( sanitizeInfluenceShape( influenceShape ) );
    }

    auto Cubemap::getClearMode() const -> ClearMode
    {
        return m_clearMode;
    }

    void Cubemap::setClearMode( ClearMode clearMode )
    {
        clearMode = sanitizeClearMode( static_cast<s32>( clearMode ) );
        if( m_clearMode != clearMode )
        {
            m_clearMode = clearMode;
            markDirty();
        }
    }

    void Cubemap::setClearMode( s32 clearMode )
    {
        setClearMode( sanitizeClearMode( clearMode ) );
    }

    String Cubemap::getCubemapPath() const
    {
        return String( m_cubemapPath.c_str() );
    }

    void Cubemap::setCubemapPath( const String &cubemapPath )
    {
        if( m_cubemapPath.str() != cubemapPath )
        {
            m_cubemapPath = cubemapPath;
            markDirty();
        }
    }

    s32 Cubemap::getResolution() const
    {
        return m_resolution;
    }

    void Cubemap::setResolution( s32 resolution )
    {
        resolution = sanitizeResolution( resolution );
        if( m_resolution != resolution )
        {
            m_resolution = resolution;
            markDirty();
        }
    }

    f32 Cubemap::getIntensity() const
    {
        return m_intensity;
    }

    void Cubemap::setIntensity( f32 intensity )
    {
        const auto value = sanitizeFloat( intensity, m_intensity, 0.0f, 64.0f );
        if( m_intensity != value )
        {
            m_intensity = value;
            markDirty();
        }
    }

    f32 Cubemap::getBlendDistance() const
    {
        return m_blendDistance;
    }

    void Cubemap::setBlendDistance( f32 blendDistance )
    {
        const auto value = sanitizeFloat( blendDistance, m_blendDistance, 0.0f, 100000.0f );
        if( m_blendDistance != value )
        {
            m_blendDistance = value;
            markDirty();
        }
    }

    f32 Cubemap::getImportance() const
    {
        return m_importance;
    }

    void Cubemap::setImportance( f32 importance )
    {
        m_importance = sanitizeFloat( importance, m_importance, 0.0f, 1000000.0f );
    }

    f32 Cubemap::getNearClipDistance() const
    {
        return m_nearClipDistance;
    }

    void Cubemap::setNearClipDistance( f32 nearClipDistance )
    {
        m_nearClipDistance = sanitizeFloat( nearClipDistance, m_nearClipDistance, 0.001f,
                                            std::max( 0.001f, m_farClipDistance - 0.001f ) );
        markDirty();
    }

    f32 Cubemap::getFarClipDistance() const
    {
        return m_farClipDistance;
    }

    void Cubemap::setFarClipDistance( f32 farClipDistance )
    {
        m_farClipDistance =
            sanitizeFloat( farClipDistance, m_farClipDistance, m_nearClipDistance + 0.001f, 1000000.0f );
        markDirty();
    }

    f32 Cubemap::getShadowDistance() const
    {
        return m_shadowDistance;
    }

    void Cubemap::setShadowDistance( f32 shadowDistance )
    {
        m_shadowDistance = sanitizeFloat( shadowDistance, m_shadowDistance, 0.0f, m_farClipDistance );
        markDirty();
    }

    u32 Cubemap::getCullingMask() const
    {
        return m_cullingMask;
    }

    void Cubemap::setCullingMask( u32 cullingMask )
    {
        if( m_cullingMask != cullingMask )
        {
            m_cullingMask = cullingMask;
            markDirty();
        }
    }

    u32 Cubemap::getFaceMask() const
    {
        return m_faceMask;
    }

    void Cubemap::setFaceMask( u32 faceMask )
    {
        faceMask &= FaceAll;
        if( m_faceMask != faceMask )
        {
            m_faceMask = faceMask;
            markDirty();
        }
    }

    bool Cubemap::getCaptureStaticObjects() const
    {
        return m_captureStaticObjects;
    }

    void Cubemap::setCaptureStaticObjects( bool captureStaticObjects )
    {
        if( m_captureStaticObjects != captureStaticObjects )
        {
            m_captureStaticObjects = captureStaticObjects;
            markDirty();
        }
    }

    bool Cubemap::getCaptureDynamicObjects() const
    {
        return m_captureDynamicObjects;
    }

    void Cubemap::setCaptureDynamicObjects( bool captureDynamicObjects )
    {
        if( m_captureDynamicObjects != captureDynamicObjects )
        {
            m_captureDynamicObjects = captureDynamicObjects;
            markDirty();
        }
    }

    bool Cubemap::getUseHDR() const
    {
        return m_useHDR;
    }

    void Cubemap::setUseHDR( bool useHDR )
    {
        if( m_useHDR != useHDR )
        {
            m_useHDR = useHDR;
            markDirty();
        }
    }

    bool Cubemap::getGenerateMipmaps() const
    {
        return m_generateMipmaps;
    }

    void Cubemap::setGenerateMipmaps( bool generateMipmaps )
    {
        if( m_generateMipmaps != generateMipmaps )
        {
            m_generateMipmaps = generateMipmaps;
            markDirty();
        }
    }

    bool Cubemap::getUseTimeSlicing() const
    {
        return m_useTimeSlicing;
    }

    void Cubemap::setUseTimeSlicing( bool useTimeSlicing )
    {
        if( m_useTimeSlicing != useTimeSlicing )
        {
            m_useTimeSlicing = useTimeSlicing;
            markDirty();
        }
    }

    bool Cubemap::getHighQualityFiltering() const
    {
        return m_highQualityFiltering;
    }

    void Cubemap::setHighQualityFiltering( bool highQualityFiltering )
    {
        if( m_highQualityFiltering != highQualityFiltering )
        {
            m_highQualityFiltering = highQualityFiltering;
            markDirty();
        }
    }

    bool Cubemap::getAutoEnableByDistance() const
    {
        return m_autoEnableByDistance;
    }

    void Cubemap::setAutoEnableByDistance( bool autoEnableByDistance )
    {
        if( m_autoEnableByDistance != autoEnableByDistance )
        {
            m_autoEnableByDistance = autoEnableByDistance;
            m_renderStateDirty = true;
            applyRenderState();
        }
    }

    f32 Cubemap::getCameraDistance() const
    {
        return m_cameraDistance;
    }

    void Cubemap::setCameraDistance( f32 cameraDistance )
    {
        m_cameraDistance = sanitizeFloat( cameraDistance, m_cameraDistance, 0.0f, 100000000.0f );
        applyRenderState();
    }

    f32 Cubemap::getEnableDistanceThreshold() const
    {
        return m_distanceThreshold;
    }

    void Cubemap::setEnableDistanceThreshold( f32 distanceThreshold )
    {
        const auto sanitized =
            sanitizeFloat( distanceThreshold, m_distanceThreshold, 0.0f, 100000000.0f );
        if( m_distanceThreshold != sanitized )
        {
            m_distanceThreshold = sanitized;
            m_renderStateDirty = true;
        }
        applyRenderState();
    }

    f32 Cubemap::getEnableDistanceTheshold() const
    {
        return getEnableDistanceThreshold();
    }

    void Cubemap::setEnableDistanceTheshold( f32 distanceTheshold )
    {
        setEnableDistanceThreshold( distanceTheshold );
    }

    f32 Cubemap::getSphereRadius() const
    {
        return m_sphereRadius;
    }

    void Cubemap::setSphereRadius( f32 sphereRadius )
    {
        m_sphereRadius = sanitizeFloat( sphereRadius, m_sphereRadius, 0.001f, 1000000.0f );
        markDirty();
    }

    f32 Cubemap::getBoxExtentX() const
    {
        return m_boxExtentX;
    }

    f32 Cubemap::getBoxExtentY() const
    {
        return m_boxExtentY;
    }

    f32 Cubemap::getBoxExtentZ() const
    {
        return m_boxExtentZ;
    }

    void Cubemap::setBoxExtents( f32 x, f32 y, f32 z )
    {
        m_boxExtentX = sanitizeFloat( x, m_boxExtentX, 0.001f, 1000000.0f );
        m_boxExtentY = sanitizeFloat( y, m_boxExtentY, 0.001f, 1000000.0f );
        m_boxExtentZ = sanitizeFloat( z, m_boxExtentZ, 0.001f, 1000000.0f );
        markDirty();
    }

    f32 Cubemap::getCaptureOffsetX() const
    {
        return m_captureOffsetX;
    }

    f32 Cubemap::getCaptureOffsetY() const
    {
        return m_captureOffsetY;
    }

    f32 Cubemap::getCaptureOffsetZ() const
    {
        return m_captureOffsetZ;
    }

    void Cubemap::setCaptureOffset( f32 x, f32 y, f32 z )
    {
        m_captureOffsetX = sanitizeFloat( x, m_captureOffsetX, -1000000.0f, 1000000.0f );
        m_captureOffsetY = sanitizeFloat( y, m_captureOffsetY, -1000000.0f, 1000000.0f );
        m_captureOffsetZ = sanitizeFloat( z, m_captureOffsetZ, -1000000.0f, 1000000.0f );
        markDirty();
    }

    f32 Cubemap::getClearColourR() const
    {
        return m_clearColourR;
    }

    f32 Cubemap::getClearColourG() const
    {
        return m_clearColourG;
    }

    f32 Cubemap::getClearColourB() const
    {
        return m_clearColourB;
    }

    f32 Cubemap::getClearColourA() const
    {
        return m_clearColourA;
    }

    void Cubemap::setClearColour( f32 r, f32 g, f32 b, f32 a )
    {
        m_clearColourR = sanitizeFloat( r, m_clearColourR, 0.0f, 1.0f );
        m_clearColourG = sanitizeFloat( g, m_clearColourG, 0.0f, 1.0f );
        m_clearColourB = sanitizeFloat( b, m_clearColourB, 0.0f, 1.0f );
        m_clearColourA = sanitizeFloat( a, m_clearColourA, 0.0f, 1.0f );
        markDirty();
    }

    f32 Cubemap::getUpdateInterval() const
    {
        return m_updateInterval;
    }

    void Cubemap::setUpdateInterval( f32 updateInterval )
    {
        m_updateInterval = sanitizeFloat( updateInterval, m_updateInterval, 0.0f, 3600.0f );
    }

    bool Cubemap::isRuntimeActive() const
    {
        return m_runtimeActive;
    }

    bool Cubemap::isDirty() const
    {
        return m_dirty;
    }

    bool Cubemap::isCaptureRequested() const
    {
        return m_captureRequested;
    }

    u32 Cubemap::getRefreshCount() const
    {
        return m_refreshCount;
    }

    double Cubemap::getLastRefreshRequestTime() const
    {
        return m_lastRefreshRequestTime;
    }

    void Cubemap::markDirty()
    {
        m_dirty = true;
        m_renderStateDirty = true;

        if( m_sourceType == SourceType::Realtime && m_refreshMode != RefreshMode::ViaScripting )
        {
            m_captureRequested = true;
        }
    }

    void Cubemap::requestCapture()
    {
        if( m_sourceType != SourceType::Realtime )
        {
            return;
        }

        if( m_faceMask == 0u )
        {
            return;
        }

        m_dirty = true;
        m_captureRequested = true;
    }

    void Cubemap::clearCaptureRequest()
    {
        m_captureRequested = false;
        m_dirty = false;
        m_hasCaptured = true;
    }

    bool Cubemap::consumeCaptureRequest()
    {
        const auto requested = m_captureRequested;
        if( requested )
        {
            clearCaptureRequest();
        }

        return requested;
    }

    void Cubemap::resetRuntimeState()
    {
        m_renderStateDirty = true;
        m_runtimeActive = false;
        m_captureRequested = false;
        m_dirty = true;
        m_hasCaptured = false;
        m_refreshCount = 0u;
        m_lastRefreshRequestTime = 0.0;
    }

    bool Cubemap::shouldBeActive() const
    {
        if( !m_enabled )
        {
            return false;
        }

        if( m_autoEnableByDistance && m_distanceThreshold > 0.0f &&
            m_cameraDistance > m_distanceThreshold )
        {
            return false;
        }

        return true;
    }

    bool Cubemap::shouldRequestCapture( double now ) const
    {
        if( m_sourceType != SourceType::Realtime )
        {
            return false;
        }

        if( m_captureRequested )
        {
            return false;
        }

        if( m_faceMask == 0u )
        {
            return false;
        }

        switch( m_refreshMode )
        {
        case RefreshMode::OnAwake:
            return !m_hasCaptured;
        case RefreshMode::EveryFrame:
            return true;
        case RefreshMode::ViaScripting:
            return false;
        case RefreshMode::Interval:
        {
            if( m_updateInterval <= 0.0f )
            {
                return true;
            }

            return ( now - m_lastRefreshRequestTime ) >= static_cast<double>( m_updateInterval );
        }
        default:
            return false;
        }
    }

    void Cubemap::applyRenderState()
    {
        const auto active = shouldBeActive();
        const auto renderActive = active && m_sourceType == SourceType::Realtime;
        const auto activeChanged = m_runtimeActive != active;
        m_runtimeActive = active;
        if( m_renderCubemap && m_renderCubemap->getSceneManager() &&
            !m_renderStateDirty && !activeChanged )
        {
            // Moving probes still follow their actor, and late-loaded/replaced
            // materials still receive the reflection. Capture settings are unchanged.
            const auto position = getCapturePosition( *this );
            if( m_renderCubemap->getPosition() != position )
                m_renderCubemap->setPosition( position );
            if( active )
                syncMaterialCubemap();
            return;
        }
        if( !active )
        {
            clearMaterialCubemap();
        }

        auto graphicsSystem = getGraphicsSystem();
        if( !m_renderCubemap && active && graphicsSystem )
        {
            if( auto textureManager = graphicsSystem->getTextureManager() )
            {
                m_renderCubemap = textureManager->addCubemap();
                if( m_renderCubemap )
                {
                    m_renderCubemap->setTextureName( m_cubemapPath.empty()
                                                         ? getRuntimeTextureName( this )
                                                         : String( m_cubemapPath.c_str() ) );
                    m_renderCubemap->setSceneManager( graphicsSystem->getGraphicsScene() );
                    configureCaptureStrategy( m_renderCubemap, *this );
                    m_renderCubemap->setEnable( renderActive );
                    graphicsSystem->loadObject( m_renderCubemap );
                }
            }
        }

        if( m_renderCubemap )
        {
            const auto textureName =
                m_cubemapPath.empty() ? getRuntimeTextureName( this ) : String( m_cubemapPath.c_str() );
            if( m_renderCubemap->getTextureName() != textureName )
            {
                m_renderCubemap->setTextureName( textureName );
            }

            if( graphicsSystem && !m_renderCubemap->getSceneManager() )
            {
                m_renderCubemap->setSceneManager( graphicsSystem->getGraphicsScene() );
            }

            m_renderCubemap->setPosition( getCapturePosition( *this ) );
            m_renderCubemap->setVisibilityMask( m_cullingMask );
            configureCaptureStrategy( m_renderCubemap, *this );
            m_renderCubemap->setAutoApplyToMaterials( active );
            m_renderCubemap->setProjectionMode( m_projectionMode == ProjectionMode::Infinite
                                                    ? render::IGraphicsCubemap::ProjectionMode::Infinite
                                                    : render::IGraphicsCubemap::ProjectionMode::Box );
            m_renderCubemap->setInfluenceShape( m_influenceShape == InfluenceShape::Box
                                                    ? render::IGraphicsCubemap::InfluenceShape::Box
                                                    : render::IGraphicsCubemap::InfluenceShape::Sphere );
            m_renderCubemap->setSphereRadius( m_sphereRadius );
            m_renderCubemap->setBoxExtents(
                Vector3<real_Num>( m_boxExtentX, m_boxExtentY, m_boxExtentZ ) );
            m_renderCubemap->setBlendDistance( m_blendDistance );
            m_renderCubemap->setImportance( m_importance );
            m_renderCubemap->setIntensity( m_intensity );
            m_renderCubemap->setAutoEnableByDistance( m_autoEnableByDistance );
            m_renderCubemap->setEnableDistanceThreshold( m_distanceThreshold );

            if( m_renderCubemap->getEnable() != renderActive )
            {
                m_renderCubemap->setEnable( renderActive );
            }

            if( active )
            {
                syncMaterialCubemap();
            }
            else
            {
                clearMaterialCubemap();
            }
            m_renderStateDirty = false;
        }
    }

    void Cubemap::syncMaterialCubemap()
    {
        if( !m_runtimeActive || !m_renderCubemap )
        {
            clearMaterialCubemap();
            return;
        }

        auto cubemapTexture = m_renderCubemap->getTexture();
        if( !cubemapTexture )
        {
            return;
        }

        auto material = getActorMaterial( *this );
        if( !material )
        {
            clearMaterialCubemap();
            return;
        }

        if( m_appliedMaterial && m_appliedMaterial != material )
        {
            clearMaterialCubemap();
        }

        const auto reflectionSlot = getReflectionTextureSlot();
        if( material->getTexture( reflectionSlot ) != cubemapTexture )
        {
            material->setTexture( cubemapTexture, reflectionSlot );
        }

        m_appliedMaterial = material;
        m_appliedTexture = cubemapTexture;
    }

    void Cubemap::clearMaterialCubemap()
    {
        if( m_appliedMaterial && m_appliedTexture )
        {
            const auto reflectionSlot = getReflectionTextureSlot();
            if( m_appliedMaterial->getTexture( reflectionSlot ) == m_appliedTexture )
            {
                m_appliedMaterial->setTexture( SmartPtr<render::ITexture>(), reflectionSlot );
            }
        }

        m_appliedMaterial = nullptr;
        m_appliedTexture = nullptr;
    }

    SmartPtr<Properties> Cubemap::getProperties() const
    {
        auto properties = Component::getProperties();
        WP_ASSERT( properties );

        properties->setProperty( "Probe Name", getProbeName() );
        properties->setProperty( "Enabled", isEnabled() );
        properties->setProperty( "Source Type", static_cast<s32>( getSourceType() ) );
        properties->setProperty( "Refresh Mode", static_cast<s32>( getRefreshMode() ) );
        properties->setProperty( "Projection Mode", static_cast<s32>( getProjectionMode() ) );
        properties->setProperty( "Influence Shape", static_cast<s32>( getInfluenceShape() ) );
        properties->setProperty( "Clear Mode", static_cast<s32>( getClearMode() ) );
        properties->setProperty( "Cubemap Path", getCubemapPath() );
        properties->setProperty( "Resolution", getResolution() );
        properties->setProperty( "Intensity", getIntensity() );
        properties->setProperty( "Blend Distance", getBlendDistance() );
        properties->setProperty( "Importance", getImportance() );
        properties->setProperty( "Near Clip Distance", getNearClipDistance() );
        properties->setProperty( "Far Clip Distance", getFarClipDistance() );
        properties->setProperty( "Shadow Distance", getShadowDistance() );
        properties->setProperty( "Culling Mask", static_cast<s32>( getCullingMask() ) );
        properties->setProperty( "Face Mask", static_cast<s32>( getFaceMask() ) );
        properties->setProperty( "Capture Static Objects", getCaptureStaticObjects() );
        properties->setProperty( "Capture Dynamic Objects", getCaptureDynamicObjects() );
        properties->setProperty( "Use HDR", getUseHDR() );
        properties->setProperty( "Generate Mipmaps", getGenerateMipmaps() );
        properties->setProperty( "Use Time Slicing", getUseTimeSlicing() );
        properties->setProperty( "High Quality Filtering", getHighQualityFiltering() );
        properties->setProperty( "Auto Enable By Distance", getAutoEnableByDistance() );
        properties->setProperty( "Enable Distance Threshold", getEnableDistanceThreshold() );
        properties->setProperty( "Sphere Radius", getSphereRadius() );
        properties->setProperty( "Box Extent X", getBoxExtentX() );
        properties->setProperty( "Box Extent Y", getBoxExtentY() );
        properties->setProperty( "Box Extent Z", getBoxExtentZ() );
        properties->setProperty( "Capture Offset X", getCaptureOffsetX() );
        properties->setProperty( "Capture Offset Y", getCaptureOffsetY() );
        properties->setProperty( "Capture Offset Z", getCaptureOffsetZ() );
        properties->setProperty( "Clear Colour R", getClearColourR() );
        properties->setProperty( "Clear Colour G", getClearColourG() );
        properties->setProperty( "Clear Colour B", getClearColourB() );
        properties->setProperty( "Clear Colour A", getClearColourA() );
        properties->setProperty( "Update Interval", getUpdateInterval() );

        // Runtime/read-only editor diagnostics. The third argument follows the existing
        // Workphone pattern used for non-authoring values such as Slip Ratio.
        properties->setProperty( "Camera Distance", getCameraDistance(), true );
        properties->setProperty( "Runtime Active", isRuntimeActive(), true );
        properties->setProperty( "Dirty", isDirty(), true );
        properties->setProperty( "Capture Requested", isCaptureRequested(), true );
        properties->setProperty( "Refresh Count", static_cast<s32>( getRefreshCount() ), true );
        properties->setProperty( "Last Refresh Request Time",
                                 static_cast<f32>( getLastRefreshRequestTime() ), true );

        return properties;
    }

    void Cubemap::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            WP_LOG_ERROR( "Cubemap::setProperties received null properties." );
            return;
        }

        Component::setProperties( properties );

        auto probeName = getProbeName();
        auto enabled = isEnabled();
        auto sourceType = static_cast<s32>( getSourceType() );
        auto refreshMode = static_cast<s32>( getRefreshMode() );
        auto projectionMode = static_cast<s32>( getProjectionMode() );
        auto influenceShape = static_cast<s32>( getInfluenceShape() );
        auto clearMode = static_cast<s32>( getClearMode() );
        auto cubemapPath = getCubemapPath();
        auto resolution = getResolution();
        auto intensity = getIntensity();
        auto blendDistance = getBlendDistance();
        auto importance = getImportance();
        auto nearClipDistance = getNearClipDistance();
        auto farClipDistance = getFarClipDistance();
        auto shadowDistance = getShadowDistance();
        auto cullingMask = static_cast<s32>( getCullingMask() );
        auto faceMask = static_cast<s32>( getFaceMask() );
        auto captureStaticObjects = getCaptureStaticObjects();
        auto captureDynamicObjects = getCaptureDynamicObjects();
        auto useHDR = getUseHDR();
        auto generateMipmaps = getGenerateMipmaps();
        auto useTimeSlicing = getUseTimeSlicing();
        auto highQualityFiltering = getHighQualityFiltering();
        auto autoEnableByDistance = getAutoEnableByDistance();
        auto distanceThreshold = getEnableDistanceThreshold();
        auto sphereRadius = getSphereRadius();
        auto boxExtentX = getBoxExtentX();
        auto boxExtentY = getBoxExtentY();
        auto boxExtentZ = getBoxExtentZ();
        auto captureOffsetX = getCaptureOffsetX();
        auto captureOffsetY = getCaptureOffsetY();
        auto captureOffsetZ = getCaptureOffsetZ();
        auto clearColourR = getClearColourR();
        auto clearColourG = getClearColourG();
        auto clearColourB = getClearColourB();
        auto clearColourA = getClearColourA();
        auto updateInterval = getUpdateInterval();

        properties->getPropertyValue( "Probe Name", probeName );
        properties->getPropertyValue( "Enabled", enabled );
        properties->getPropertyValue( "Source Type", sourceType );
        properties->getPropertyValue( "Refresh Mode", refreshMode );
        properties->getPropertyValue( "Projection Mode", projectionMode );
        properties->getPropertyValue( "Influence Shape", influenceShape );
        properties->getPropertyValue( "Clear Mode", clearMode );
        properties->getPropertyValue( "Cubemap Path", cubemapPath );
        properties->getPropertyValue( "Resolution", resolution );
        properties->getPropertyValue( "Intensity", intensity );
        properties->getPropertyValue( "Blend Distance", blendDistance );
        properties->getPropertyValue( "Importance", importance );
        properties->getPropertyValue( "Near Clip Distance", nearClipDistance );
        properties->getPropertyValue( "Far Clip Distance", farClipDistance );
        properties->getPropertyValue( "Shadow Distance", shadowDistance );
        properties->getPropertyValue( "Culling Mask", cullingMask );
        properties->getPropertyValue( "Face Mask", faceMask );
        properties->getPropertyValue( "Capture Static Objects", captureStaticObjects );
        properties->getPropertyValue( "Capture Dynamic Objects", captureDynamicObjects );
        properties->getPropertyValue( "Use HDR", useHDR );
        properties->getPropertyValue( "Generate Mipmaps", generateMipmaps );
        properties->getPropertyValue( "Use Time Slicing", useTimeSlicing );
        properties->getPropertyValue( "High Quality Filtering", highQualityFiltering );
        properties->getPropertyValue( "Auto Enable By Distance", autoEnableByDistance );
        properties->getPropertyValue( "Enable Distance Threshold", distanceThreshold );
        properties->getPropertyValue( "Enable Distance Theshold", distanceThreshold );  // legacy typo
        properties->getPropertyValue( "Sphere Radius", sphereRadius );
        properties->getPropertyValue( "Box Extent X", boxExtentX );
        properties->getPropertyValue( "Box Extent Y", boxExtentY );
        properties->getPropertyValue( "Box Extent Z", boxExtentZ );
        properties->getPropertyValue( "Capture Offset X", captureOffsetX );
        properties->getPropertyValue( "Capture Offset Y", captureOffsetY );
        properties->getPropertyValue( "Capture Offset Z", captureOffsetZ );
        properties->getPropertyValue( "Clear Colour R", clearColourR );
        properties->getPropertyValue( "Clear Colour G", clearColourG );
        properties->getPropertyValue( "Clear Colour B", clearColourB );
        properties->getPropertyValue( "Clear Colour A", clearColourA );
        properties->getPropertyValue( "Update Interval", updateInterval );

        setProbeName( probeName );
        setEnabled( enabled );
        setSourceType( sourceType );
        setRefreshMode( refreshMode );
        setProjectionMode( projectionMode );
        setInfluenceShape( influenceShape );
        setClearMode( clearMode );
        setCubemapPath( cubemapPath );
        setResolution( resolution );
        setIntensity( intensity );
        setBlendDistance( blendDistance );
        setImportance( importance );
        setNearClipDistance( nearClipDistance );
        setFarClipDistance( farClipDistance );
        setShadowDistance( shadowDistance );
        setCullingMask( static_cast<u32>( cullingMask ) );
        setFaceMask( static_cast<u32>( faceMask ) );
        setCaptureStaticObjects( captureStaticObjects );
        setCaptureDynamicObjects( captureDynamicObjects );
        setUseHDR( useHDR );
        setGenerateMipmaps( generateMipmaps );
        setUseTimeSlicing( useTimeSlicing );
        setHighQualityFiltering( highQualityFiltering );
        setAutoEnableByDistance( autoEnableByDistance );
        setEnableDistanceThreshold( distanceThreshold );
        setSphereRadius( sphereRadius );
        setBoxExtents( boxExtentX, boxExtentY, boxExtentZ );
        setCaptureOffset( captureOffsetX, captureOffsetY, captureOffsetZ );
        setClearColour( clearColourR, clearColourG, clearColourB, clearColourA );
        setUpdateInterval( updateInterval );

        applyRenderState();
    }

    Array<SmartPtr<ISharedObject>> Cubemap::getChildObjects() const
    {
        auto children = Component::getChildObjects();

        if( m_renderCubemap )
        {
            children.push_back( m_renderCubemap );
        }

        return children;
    }

}  // namespace workphone::scene
