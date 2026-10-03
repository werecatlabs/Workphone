#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/SkyboxPanorama.hpp>

#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ISkybox.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Scene/Components/Skybox.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, SkyboxPanorama, Component );

    const String SkyboxPanorama::panoramaDirectoryStr = "Panorama Directory";
    const String SkyboxPanorama::filePrefixStr = "File Prefix";
    const String SkyboxPanorama::fileExtensionStr = "File Extension";
    const String SkyboxPanorama::cameraActorNameStr = "Camera Actor";
    const String SkyboxPanorama::gridColumnsStr = "Grid Columns";
    const String SkyboxPanorama::gridRowsStr = "Grid Rows";
    const String SkyboxPanorama::spacingXStr = "Panorama Spacing X";
    const String SkyboxPanorama::spacingZStr = "Panorama Spacing Z";
    const String SkyboxPanorama::eyeHeightStr = "Eye Height";
    const String SkyboxPanorama::originOffsetStr = "Origin Offset";
    const String SkyboxPanorama::centreGridStr = "Centre Grid";
    const String SkyboxPanorama::snapCameraStr = "Snap Camera";
    const String SkyboxPanorama::followCameraStr = "Follow Camera";
    const String SkyboxPanorama::switchDistanceStr = "Automatic Switch Distance";
    const String SkyboxPanorama::maximumLinkDistanceStr = "Maximum Link Distance";
    const String SkyboxPanorama::directionToleranceStr = "Direction Tolerance Degrees";
    const String SkyboxPanorama::activePanoramaStr = "Active Panorama";
    const String SkyboxPanorama::panoramaCountStr = "Panorama Count";
    const String SkyboxPanorama::transitionEnabledStr = "Transition Enabled";
    const String SkyboxPanorama::transitionDurationStr = "Transition Duration";
    const String SkyboxPanorama::transitionEasingStr = "Transition Easing";
    const String SkyboxPanorama::interpolateCameraStr = "Interpolate Camera";
    const String SkyboxPanorama::transitionArcHeightStr = "Transition Arc Height";
    const String SkyboxPanorama::transitionFovOffsetStr = "Transition FOV Offset";
    const String SkyboxPanorama::panoramaBlendModeStr = "Panorama Blend Mode";
    const String SkyboxPanorama::imageSwitchPointStr = "Image Switch Point";
    const String SkyboxPanorama::crossFadeStartStr = "Cross Fade Start";
    const String SkyboxPanorama::crossFadeEndStr = "Cross Fade End";
    const String SkyboxPanorama::crossFadeDistanceOffsetStr = "Cross Fade Distance Offset";
    const String SkyboxPanorama::allowTransitionInterruptionStr = "Allow Transition Interruption";
    const String SkyboxPanorama::isTransitioningStr = "Is Transitioning";
    const String SkyboxPanorama::transitionProgressStr = "Transition Progress";
    const String SkyboxPanorama::targetPanoramaStr = "Target Panorama";

    namespace
    {
        constexpr u32 maximumPanoramaCount = 100000u;
        constexpr real_Num minimumSpacing = static_cast<real_Num>( 0.01 );
        constexpr real_Num directionEpsilon = static_cast<real_Num>( 1.0e-6 );
        constexpr real_Num transitionEpsilon = static_cast<real_Num>( 1.0e-4 );
        constexpr real_Num pi = static_cast<real_Num>( 3.14159265358979323846 );
        constexpr real_Num degreesToRadians = pi / static_cast<real_Num>( 180.0 );

        const std::array<String, 6> faceNames = { "front", "back", "left", "right", "up", "down" };
    }  // namespace

    SkyboxPanorama::SkyboxPanorama() = default;

    SkyboxPanorama::~SkyboxPanorama() = default;

    void SkyboxPanorama::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Loaded )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );
            Component::load( data );

            if( data && data->isDerived<Properties>() )
            {
                setProperties( workphone::static_pointer_cast<Properties>( data ) );
            }

            resolveRuntimeObjects();
            resolveSkyboxes();
            rebuildPanoramas();
            setLoadingState( LoadingState::Loaded );

            if( !m_panoramas.empty() )
            {
                const auto index = static_cast<u32>( std::max<s32>( 0, m_activePanorama ) );
                activatePanorama( std::min<u32>( index, getPanoramaCount() - 1u ), false );
            }
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Loaded );
        }
    }

    void SkyboxPanorama::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );
        cancelTransition();
        m_cameraActor = nullptr;
        m_visibleSkybox = nullptr;
        m_hiddenSkybox = nullptr;
        m_panoramas.clear();
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void SkyboxPanorama::update()
    {
        if( Thread::getCurrentTask() != TaskId::Application )
        {
            return;
        }

        if( m_gridDirty )
        {
            rebuildPanoramas();
        }

        auto applicationManager = core::IApplicationManager::instance();
        auto timer = applicationManager ? applicationManager->getTimer() : nullptr;
        const auto deltaTime =
            timer ? static_cast<real_Num>( timer->getDeltaTime() ) : static_cast<real_Num>( 0.0 );
        advanceTransition( deltaTime );

        if( !m_cameraActor )
        {
            resolveRuntimeObjects();
        }

        if( m_isTransitioning || !m_followCamera || !m_cameraActor || m_panoramas.empty() )
        {
            return;
        }

        const auto cameraPosition = m_cameraActor->getPosition();
        const auto nearestIndex = findNearestPanorama( cameraPosition );
        if( nearestIndex < 0 || nearestIndex == m_activePanorama )
        {
            return;
        }

        const auto &node = m_panoramas[static_cast<u32>( nearestIndex )];
        const auto distanceSquared = ( node.position - cameraPosition ).lengthSquared();
        const auto switchDistance = getEffectiveSwitchDistance();
        if( distanceSquared <= switchDistance * switchDistance )
        {
            // Follow mode is for a freely moving camera, so blend images without moving it.
            moveToPanorama( static_cast<u32>( nearestIndex ), false );
        }
    }

    auto SkyboxPanorama::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Component::getProperties();
        properties->setProperty( panoramaDirectoryStr, String( m_panoramaDirectory.c_str() ) );
        properties->setProperty( filePrefixStr, String( m_filePrefix.c_str() ) );
        properties->setProperty( fileExtensionStr, String( m_fileExtension.c_str() ) );
        properties->setProperty( cameraActorNameStr, String( m_cameraActorName.c_str() ) );
        properties->setProperty( gridColumnsStr, m_gridColumns );
        properties->setProperty( gridRowsStr, m_gridRows );
        properties->setProperty( spacingXStr, m_spacingX );
        properties->setProperty( spacingZStr, m_spacingZ );
        properties->setProperty( eyeHeightStr, m_eyeHeight );
        properties->setProperty( originOffsetStr, m_originOffset );
        properties->setProperty( centreGridStr, m_centreGrid );
        properties->setProperty( snapCameraStr, m_snapCamera );
        properties->setProperty( followCameraStr, m_followCamera );
        properties->setProperty( switchDistanceStr, m_switchDistance );
        properties->setProperty( maximumLinkDistanceStr, m_maximumLinkDistance );
        properties->setProperty( directionToleranceStr, m_directionToleranceDegrees );
        properties->setProperty( activePanoramaStr, m_activePanorama );
        properties->setProperty( panoramaCountStr, getPanoramaCount(), true );
        properties->setProperty( transitionEnabledStr, m_transitionEnabled );
        properties->setProperty( transitionDurationStr, m_transitionDuration );
        properties->setProperty( transitionEasingStr, static_cast<s32>( m_transitionEasing ) );
        properties->setProperty( interpolateCameraStr, m_interpolateCamera );
        properties->setProperty( transitionArcHeightStr, m_transitionArcHeight );
        properties->setProperty( transitionFovOffsetStr, m_transitionFovOffset );
        properties->setProperty( panoramaBlendModeStr, static_cast<s32>( m_panoramaBlendMode ) );
        properties->setProperty( imageSwitchPointStr, m_imageSwitchPoint );
        properties->setProperty( crossFadeStartStr, m_crossFadeStart );
        properties->setProperty( crossFadeEndStr, m_crossFadeEnd );
        properties->setProperty( crossFadeDistanceOffsetStr, m_crossFadeDistanceOffset );
        properties->setProperty( allowTransitionInterruptionStr, m_allowTransitionInterruption );
        properties->setProperty( isTransitioningStr, m_isTransitioning, true );
        properties->setProperty( transitionProgressStr, m_transitionProgress, true );
        properties->setProperty( targetPanoramaStr, m_targetPanorama, true );
        return properties;
    }

    void SkyboxPanorama::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        cancelTransition();
        Component::setProperties( properties );

        String directory = m_panoramaDirectory.c_str();
        String prefix = m_filePrefix.c_str();
        String extension = m_fileExtension.c_str();
        String cameraName = m_cameraActorName.c_str();
        u32 columns = m_gridColumns;
        u32 rows = m_gridRows;
        real_Num spacingX = m_spacingX;
        real_Num spacingZ = m_spacingZ;
        real_Num eyeHeight = m_eyeHeight;
        Vector3<real_Num> originOffset = m_originOffset;
        bool centreGrid = m_centreGrid;

        properties->getPropertyValue( panoramaDirectoryStr, directory );
        properties->getPropertyValue( filePrefixStr, prefix );
        properties->getPropertyValue( fileExtensionStr, extension );
        properties->getPropertyValue( cameraActorNameStr, cameraName );
        properties->getPropertyValue( gridColumnsStr, columns );
        properties->getPropertyValue( gridRowsStr, rows );
        properties->getPropertyValue( spacingXStr, spacingX );
        properties->getPropertyValue( spacingZStr, spacingZ );
        properties->getPropertyValue( eyeHeightStr, eyeHeight );
        properties->getPropertyValue( originOffsetStr, originOffset );
        properties->getPropertyValue( centreGridStr, centreGrid );

        const bool gridChanged =
            directory != m_panoramaDirectory.c_str() || prefix != m_filePrefix.c_str() ||
            extension != m_fileExtension.c_str() || columns != m_gridColumns || rows != m_gridRows ||
            spacingX != m_spacingX || spacingZ != m_spacingZ || eyeHeight != m_eyeHeight ||
            originOffset != m_originOffset || centreGrid != m_centreGrid;

        m_panoramaDirectory = directory;
        m_filePrefix = prefix;
        m_fileExtension = extension;
        m_cameraActorName = cameraName;
        m_gridColumns = std::max<u32>( 1u, columns );
        m_gridRows = std::max<u32>( 1u, rows );
        m_spacingX = std::max<real_Num>( minimumSpacing, spacingX );
        m_spacingZ = std::max<real_Num>( minimumSpacing, spacingZ );
        m_eyeHeight = eyeHeight;
        m_originOffset = originOffset;
        m_centreGrid = centreGrid;

        properties->getPropertyValue( snapCameraStr, m_snapCamera );
        properties->getPropertyValue( followCameraStr, m_followCamera );
        properties->getPropertyValue( switchDistanceStr, m_switchDistance );
        properties->getPropertyValue( maximumLinkDistanceStr, m_maximumLinkDistance );
        properties->getPropertyValue( directionToleranceStr, m_directionToleranceDegrees );
        properties->getPropertyValue( activePanoramaStr, m_activePanorama );
        properties->getPropertyValue( transitionEnabledStr, m_transitionEnabled );
        properties->getPropertyValue( transitionDurationStr, m_transitionDuration );
        properties->getPropertyValue( interpolateCameraStr, m_interpolateCamera );
        properties->getPropertyValue( transitionArcHeightStr, m_transitionArcHeight );
        properties->getPropertyValue( transitionFovOffsetStr, m_transitionFovOffset );
        properties->getPropertyValue( imageSwitchPointStr, m_imageSwitchPoint );
        properties->getPropertyValue( crossFadeStartStr, m_crossFadeStart );
        properties->getPropertyValue( crossFadeEndStr, m_crossFadeEnd );
        properties->getPropertyValue( crossFadeDistanceOffsetStr, m_crossFadeDistanceOffset );
        properties->getPropertyValue( allowTransitionInterruptionStr, m_allowTransitionInterruption );

        s32 easing = static_cast<s32>( m_transitionEasing );
        if( properties->getPropertyValue( transitionEasingStr, easing ) )
        {
            easing = std::clamp<s32>( easing, static_cast<s32>( TransitionEasing::Linear ),
                                      static_cast<s32>( TransitionEasing::EaseInOut ) );
            m_transitionEasing = static_cast<TransitionEasing>( easing );
        }

        s32 blendMode = static_cast<s32>( m_panoramaBlendMode );
        if( properties->getPropertyValue( panoramaBlendModeStr, blendMode ) )
        {
            blendMode = std::clamp<s32>( blendMode, static_cast<s32>( PanoramaBlendMode::TimedSwitch ),
                                         static_cast<s32>( PanoramaBlendMode::CrossFade ) );
            m_panoramaBlendMode = static_cast<PanoramaBlendMode>( blendMode );
        }

        m_switchDistance = std::max<real_Num>( 0, m_switchDistance );
        m_maximumLinkDistance = std::max<real_Num>( 0, m_maximumLinkDistance );
        m_directionToleranceDegrees = std::clamp<real_Num>( m_directionToleranceDegrees, 1, 89 );
        m_transitionDuration = std::max<real_Num>( 0, m_transitionDuration );
        m_imageSwitchPoint = std::clamp<real_Num>( m_imageSwitchPoint, 0, 1 );
        m_crossFadeStart = std::clamp<real_Num>( m_crossFadeStart, 0, 1 );
        m_crossFadeEnd = std::clamp<real_Num>( m_crossFadeEnd, m_crossFadeStart, 1 );
        m_crossFadeDistanceOffset = std::max<real_Num>( 0, m_crossFadeDistanceOffset );

        if( gridChanged )
        {
            markGridDirty();
            rebuildPanoramas();
        }

        m_cameraActor = nullptr;
        resolveRuntimeObjects();
        resolveSkyboxes();

        if( !m_panoramas.empty() )
        {
            m_activePanorama =
                std::clamp<s32>( m_activePanorama, 0, static_cast<s32>( m_panoramas.size() - 1 ) );
            activatePanorama( static_cast<u32>( m_activePanorama ), false );
        }
    }

    void SkyboxPanorama::rebuildPanoramas()
    {
        cancelTransition();
        m_panoramas.clear();

        m_gridColumns = std::clamp<u32>( m_gridColumns, 1u, maximumPanoramaCount );
        m_gridRows = std::max<u32>( 1u, m_gridRows );
        m_spacingX = std::max<real_Num>( minimumSpacing, m_spacingX );
        m_spacingZ = std::max<real_Num>( minimumSpacing, m_spacingZ );

        const auto requestedCount = static_cast<u64>( m_gridColumns ) * m_gridRows;
        if( requestedCount > maximumPanoramaCount )
        {
            WP_LOG_WARNING( "SkyboxPanoramaGenerator grid exceeds 100000 stations; truncating rows." );
            m_gridRows = std::max<u32>( 1u, maximumPanoramaCount / m_gridColumns );
        }

        const auto origin = getGridOrigin();
        const auto xOffset =
            m_centreGrid ? static_cast<real_Num>( m_gridColumns - 1u ) * m_spacingX * 0.5f : 0.0f;
        const auto zOffset =
            m_centreGrid ? static_cast<real_Num>( m_gridRows - 1u ) * m_spacingZ * 0.5f : 0.0f;

        m_panoramas.reserve( static_cast<size_t>( m_gridColumns ) * m_gridRows );
        for( u32 row = 0; row < m_gridRows; ++row )
        {
            for( u32 column = 0; column < m_gridColumns; ++column )
            {
                PanoramaNode node;
                node.index = static_cast<u32>( m_panoramas.size() );
                node.column = column;
                node.row = row;
                node.position =
                    origin + Vector3<real_Num>( static_cast<real_Num>( column ) * m_spacingX - xOffset,
                                                m_eyeHeight,
                                                static_cast<real_Num>( row ) * m_spacingZ - zOffset );

                for( u32 face = 0; face < faceNames.size(); ++face )
                {
                    node.textureNames[face] = makeTextureName( column, row, faceNames[face] );
                }

                m_panoramas.push_back( node );
            }
        }

        m_activePanorama =
            std::clamp<s32>( m_activePanorama, 0, static_cast<s32>( m_panoramas.size() - 1 ) );
        m_gridDirty = false;
    }

    u32 SkyboxPanorama::getPanoramaCount() const
    {
        return static_cast<u32>( m_panoramas.size() );
    }

    auto SkyboxPanorama::getPanorama( u32 index ) const -> const PanoramaNode *
    {
        return index < m_panoramas.size() ? &m_panoramas[index] : nullptr;
    }

    auto SkyboxPanorama::getPanoramas() const -> const Array<PanoramaNode> &
    {
        return m_panoramas;
    }

    s32 SkyboxPanorama::getActivePanorama() const
    {
        return m_activePanorama;
    }

    bool SkyboxPanorama::moveToPanorama( u32 index, bool snapCamera )
    {
        const auto moveCamera = snapCamera && m_snapCamera;
        if( !m_transitionEnabled || m_transitionDuration <= transitionEpsilon ||
            static_cast<s32>( index ) == m_activePanorama )
        {
            return activatePanorama( index, moveCamera );
        }

        return beginTransition( index, moveCamera );
    }

    bool SkyboxPanorama::moveToNearestPanorama( const Vector3<real_Num> &worldPosition, bool snapCamera )
    {
        const auto index = findNearestPanorama( worldPosition );
        return index >= 0 && moveToPanorama( static_cast<u32>( index ), snapCamera );
    }

    bool SkyboxPanorama::moveInDirection( const Vector3<real_Num> &worldDirection )
    {
        if( m_panoramas.empty() || m_activePanorama < 0 ||
            static_cast<size_t>( m_activePanorama ) >= m_panoramas.size() )
        {
            return false;
        }

        auto direction = worldDirection;
        direction.Y() = 0;
        if( direction.lengthSquared() <= directionEpsilon )
        {
            return false;
        }
        direction.normalise();

        const auto currentIndex = static_cast<u32>( m_activePanorama );
        const auto currentPosition = m_panoramas[currentIndex].position;
        const auto maximumDistance = getEffectiveMaximumLinkDistance();
        const auto maximumDistanceSquared = maximumDistance * maximumDistance;
        const auto minimumAlignment = std::cos( m_directionToleranceDegrees * degreesToRadians );

        s32 bestIndex = -1;
        real_Num bestAlignment = minimumAlignment;
        real_Num bestDistanceSquared = std::numeric_limits<real_Num>::max();

        for( const auto &candidate : m_panoramas )
        {
            if( candidate.index == currentIndex )
            {
                continue;
            }

            auto offset = candidate.position - currentPosition;
            offset.Y() = 0;
            const auto distanceSquared = offset.lengthSquared();
            if( distanceSquared <= directionEpsilon || distanceSquared > maximumDistanceSquared )
            {
                continue;
            }

            offset.normalise();
            const auto alignment = direction.dotProduct( offset );
            if( alignment > bestAlignment ||
                ( alignment == bestAlignment && distanceSquared < bestDistanceSquared ) )
            {
                bestIndex = static_cast<s32>( candidate.index );
                bestAlignment = alignment;
                bestDistanceSquared = distanceSquared;
            }
        }

        return bestIndex >= 0 && moveToPanorama( static_cast<u32>( bestIndex ), true );
    }

    bool SkyboxPanorama::moveForward()
    {
        resolveRuntimeObjects();
        return m_cameraActor &&
               moveInDirection( m_cameraActor->getOrientation() * Vector3<real_Num>::unitZ() );
    }

    bool SkyboxPanorama::moveBackward()
    {
        resolveRuntimeObjects();
        return m_cameraActor &&
               moveInDirection( m_cameraActor->getOrientation() * -Vector3<real_Num>::unitZ() );
    }

    bool SkyboxPanorama::moveLeft()
    {
        resolveRuntimeObjects();
        return m_cameraActor &&
               moveInDirection( m_cameraActor->getOrientation() * -Vector3<real_Num>::unitX() );
    }

    bool SkyboxPanorama::moveRight()
    {
        resolveRuntimeObjects();
        return m_cameraActor &&
               moveInDirection( m_cameraActor->getOrientation() * Vector3<real_Num>::unitX() );
    }

    void SkyboxPanorama::advanceTransition( real_Num deltaTime )
    {
        if( !m_isTransitioning || m_targetPanorama < 0 ||
            static_cast<size_t>( m_targetPanorama ) >= m_panoramas.size() )
        {
            return;
        }

        m_transitionElapsed += std::max<real_Num>( 0, deltaTime );
        m_transitionProgress =
            m_transitionDuration > transitionEpsilon
                ? std::clamp<real_Num>( m_transitionElapsed / m_transitionDuration, 0, 1 )
                : static_cast<real_Num>( 1.0 );

        const auto easedProgress = applyTransitionEasing( m_transitionProgress );

        if( m_transitionMovesCamera && m_cameraActor )
        {
            auto position = m_transitionStartPosition +
                            ( m_transitionEndPosition - m_transitionStartPosition ) * easedProgress;
            position.Y() += static_cast<real_Num>( 4.0 ) * m_transitionProgress *
                            ( static_cast<real_Num>( 1.0 ) - m_transitionProgress ) *
                            m_transitionArcHeight;
            m_cameraActor->setPosition( position );
            m_cameraActor->updateTransform();

            if( auto camera = getCameraComponent() )
            {
                const auto fovPulse = std::sin( m_transitionProgress * pi );
                camera->setFOV(
                    static_cast<f32>( m_transitionStartFov + fovPulse * m_transitionFovOffset ) );
            }
        }

        if( m_crossFadeActive )
        {
            applyCrossFade( getCrossFadeWeight( m_transitionProgress ) );
        }
        else if( !m_imageSwitched && m_transitionProgress >= m_imageSwitchPoint )
        {
            applyPanoramaTextures( m_panoramas[static_cast<u32>( m_targetPanorama )], m_visibleSkybox );
            m_imageSwitched = true;
        }

        if( m_transitionProgress >= static_cast<real_Num>( 1.0 ) )
        {
            finishTransition();
        }
    }

    bool SkyboxPanorama::isTransitioning() const
    {
        return m_isTransitioning;
    }

    real_Num SkyboxPanorama::getTransitionProgress() const
    {
        return m_transitionProgress;
    }

    s32 SkyboxPanorama::getTargetPanorama() const
    {
        return m_targetPanorama;
    }

    void SkyboxPanorama::finishTransition()
    {
        if( !m_isTransitioning || m_targetPanorama < 0 ||
            static_cast<size_t>( m_targetPanorama ) >= m_panoramas.size() )
        {
            return;
        }

        if( m_crossFadeActive )
        {
            applyCrossFade( 1 );
            std::swap( m_visibleSkybox, m_hiddenSkybox );
            resetCrossFade();
        }
        else if( !m_imageSwitched )
        {
            applyPanoramaTextures( m_panoramas[static_cast<u32>( m_targetPanorama )], m_visibleSkybox );
        }

        if( m_transitionMovesCamera && m_cameraActor )
        {
            m_cameraActor->setPosition( m_transitionEndPosition );
            m_cameraActor->updateTransform();
        }

        if( auto camera = getCameraComponent() )
        {
            camera->setFOV( static_cast<f32>( m_transitionStartFov ) );
        }

        m_activePanorama = m_targetPanorama;
        m_targetPanorama = -1;
        m_transitionElapsed = 0;
        m_transitionProgress = 1;
        m_isTransitioning = false;
        m_transitionMovesCamera = false;
        m_imageSwitched = false;
        m_crossFadeActive = false;
    }

    void SkyboxPanorama::cancelTransition()
    {
        if( !m_isTransitioning )
        {
            return;
        }

        if( m_crossFadeActive )
        {
            applyCrossFade( 0 );
            resetCrossFade();
        }
        else if( m_imageSwitched && m_activePanorama >= 0 &&
                 static_cast<size_t>( m_activePanorama ) < m_panoramas.size() )
        {
            applyPanoramaTextures( m_panoramas[static_cast<u32>( m_activePanorama )], m_visibleSkybox );
        }

        if( auto camera = getCameraComponent() )
        {
            camera->setFOV( static_cast<f32>( m_transitionStartFov ) );
        }

        m_targetPanorama = -1;
        m_transitionElapsed = 0;
        m_transitionProgress = 0;
        m_isTransitioning = false;
        m_transitionMovesCamera = false;
        m_imageSwitched = false;
        m_crossFadeActive = false;
    }

    auto SkyboxPanorama::getCameraActor() const -> SmartPtr<IGameActor>
    {
        return m_cameraActor;
    }

    void SkyboxPanorama::setCameraActor( SmartPtr<IGameActor> cameraActor )
    {
        m_cameraActor = cameraActor;
        if( cameraActor )
        {
            m_cameraActorName = cameraActor->getName();
        }
    }

    String SkyboxPanorama::getPanoramaDirectory() const
    {
        return m_panoramaDirectory.c_str();
    }

    void SkyboxPanorama::setPanoramaDirectory( const String &directory )
    {
        if( directory != m_panoramaDirectory.c_str() )
        {
            m_panoramaDirectory = directory;
            markGridDirty();
        }
    }

    String SkyboxPanorama::getFilePrefix() const
    {
        return m_filePrefix.c_str();
    }

    void SkyboxPanorama::setFilePrefix( const String &prefix )
    {
        if( prefix != m_filePrefix.c_str() )
        {
            m_filePrefix = prefix;
            markGridDirty();
        }
    }

    String SkyboxPanorama::getFileExtension() const
    {
        return m_fileExtension.c_str();
    }

    void SkyboxPanorama::setFileExtension( const String &extension )
    {
        if( extension != m_fileExtension.c_str() )
        {
            m_fileExtension = extension;
            markGridDirty();
        }
    }

    u32 SkyboxPanorama::getGridColumns() const
    {
        return m_gridColumns;
    }

    void SkyboxPanorama::setGridColumns( u32 columns )
    {
        columns = std::max<u32>( 1u, columns );
        if( columns != m_gridColumns )
        {
            m_gridColumns = columns;
            markGridDirty();
        }
    }

    u32 SkyboxPanorama::getGridRows() const
    {
        return m_gridRows;
    }

    void SkyboxPanorama::setGridRows( u32 rows )
    {
        rows = std::max<u32>( 1u, rows );
        if( rows != m_gridRows )
        {
            m_gridRows = rows;
            markGridDirty();
        }
    }

    real_Num SkyboxPanorama::getSpacingX() const
    {
        return m_spacingX;
    }

    void SkyboxPanorama::setSpacingX( real_Num spacing )
    {
        spacing = std::max<real_Num>( minimumSpacing, spacing );
        if( spacing != m_spacingX )
        {
            m_spacingX = spacing;
            markGridDirty();
        }
    }

    real_Num SkyboxPanorama::getSpacingZ() const
    {
        return m_spacingZ;
    }

    void SkyboxPanorama::setSpacingZ( real_Num spacing )
    {
        spacing = std::max<real_Num>( minimumSpacing, spacing );
        if( spacing != m_spacingZ )
        {
            m_spacingZ = spacing;
            markGridDirty();
        }
    }

    real_Num SkyboxPanorama::getEyeHeight() const
    {
        return m_eyeHeight;
    }

    void SkyboxPanorama::setEyeHeight( real_Num eyeHeight )
    {
        if( eyeHeight != m_eyeHeight )
        {
            m_eyeHeight = eyeHeight;
            markGridDirty();
        }
    }

    real_Num SkyboxPanorama::getSwitchDistance() const
    {
        return m_switchDistance;
    }

    void SkyboxPanorama::setSwitchDistance( real_Num distance )
    {
        m_switchDistance = std::max<real_Num>( 0, distance );
    }

    real_Num SkyboxPanorama::getMaximumLinkDistance() const
    {
        return m_maximumLinkDistance;
    }

    void SkyboxPanorama::setMaximumLinkDistance( real_Num distance )
    {
        m_maximumLinkDistance = std::max<real_Num>( 0, distance );
    }

    bool SkyboxPanorama::getSnapCamera() const
    {
        return m_snapCamera;
    }

    void SkyboxPanorama::setSnapCamera( bool snapCamera )
    {
        m_snapCamera = snapCamera;
    }

    bool SkyboxPanorama::getFollowCamera() const
    {
        return m_followCamera;
    }

    void SkyboxPanorama::setFollowCamera( bool followCamera )
    {
        m_followCamera = followCamera;
    }

    bool SkyboxPanorama::getTransitionEnabled() const
    {
        return m_transitionEnabled;
    }

    void SkyboxPanorama::setTransitionEnabled( bool enabled )
    {
        if( !enabled )
        {
            finishTransition();
        }
        m_transitionEnabled = enabled;
    }

    real_Num SkyboxPanorama::getTransitionDuration() const
    {
        return m_transitionDuration;
    }

    void SkyboxPanorama::setTransitionDuration( real_Num duration )
    {
        m_transitionDuration = std::max<real_Num>( 0, duration );
    }

    auto SkyboxPanorama::getTransitionEasing() const -> TransitionEasing
    {
        return m_transitionEasing;
    }

    void SkyboxPanorama::setTransitionEasing( TransitionEasing easing )
    {
        const auto value =
            std::clamp<s32>( static_cast<s32>( easing ), static_cast<s32>( TransitionEasing::Linear ),
                             static_cast<s32>( TransitionEasing::EaseInOut ) );
        m_transitionEasing = static_cast<TransitionEasing>( value );
    }

    bool SkyboxPanorama::getInterpolateCamera() const
    {
        return m_interpolateCamera;
    }

    void SkyboxPanorama::setInterpolateCamera( bool interpolateCamera )
    {
        m_interpolateCamera = interpolateCamera;
    }

    real_Num SkyboxPanorama::getTransitionArcHeight() const
    {
        return m_transitionArcHeight;
    }

    void SkyboxPanorama::setTransitionArcHeight( real_Num height )
    {
        m_transitionArcHeight = height;
    }

    real_Num SkyboxPanorama::getTransitionFovOffset() const
    {
        return m_transitionFovOffset;
    }

    void SkyboxPanorama::setTransitionFovOffset( real_Num offset )
    {
        m_transitionFovOffset = offset;
    }

    auto SkyboxPanorama::getPanoramaBlendMode() const -> PanoramaBlendMode
    {
        return m_panoramaBlendMode;
    }

    void SkyboxPanorama::setPanoramaBlendMode( PanoramaBlendMode mode )
    {
        const auto value = std::clamp<s32>( static_cast<s32>( mode ),
                                            static_cast<s32>( PanoramaBlendMode::TimedSwitch ),
                                            static_cast<s32>( PanoramaBlendMode::CrossFade ) );
        m_panoramaBlendMode = static_cast<PanoramaBlendMode>( value );
    }

    real_Num SkyboxPanorama::getImageSwitchPoint() const
    {
        return m_imageSwitchPoint;
    }

    void SkyboxPanorama::setImageSwitchPoint( real_Num switchPoint )
    {
        m_imageSwitchPoint = std::clamp<real_Num>( switchPoint, 0, 1 );
    }

    real_Num SkyboxPanorama::getCrossFadeStart() const
    {
        return m_crossFadeStart;
    }

    void SkyboxPanorama::setCrossFadeStart( real_Num start )
    {
        m_crossFadeStart = std::clamp<real_Num>( start, 0, 1 );
        m_crossFadeEnd = std::max( m_crossFadeStart, m_crossFadeEnd );
    }

    real_Num SkyboxPanorama::getCrossFadeEnd() const
    {
        return m_crossFadeEnd;
    }

    void SkyboxPanorama::setCrossFadeEnd( real_Num end )
    {
        m_crossFadeEnd = std::clamp<real_Num>( end, m_crossFadeStart, 1 );
    }

    real_Num SkyboxPanorama::getCrossFadeDistanceOffset() const
    {
        return m_crossFadeDistanceOffset;
    }

    void SkyboxPanorama::setCrossFadeDistanceOffset( real_Num offset )
    {
        m_crossFadeDistanceOffset = std::max<real_Num>( 0, offset );
    }

    bool SkyboxPanorama::getAllowTransitionInterruption() const
    {
        return m_allowTransitionInterruption;
    }

    void SkyboxPanorama::setAllowTransitionInterruption( bool allow )
    {
        m_allowTransitionInterruption = allow;
    }

    String SkyboxPanorama::makeTextureName( u32 column, u32 row, const String &face ) const
    {
        String directory = m_panoramaDirectory.c_str();
        if( !directory.empty() && directory.back() != '/' && directory.back() != '\\' )
        {
            directory += "/";
        }

        return directory + String( m_filePrefix.c_str() ) + "_c" + StringUtil::toString( column ) +
               "_r" + StringUtil::toString( row ) + "_" + face + String( m_fileExtension.c_str() );
    }

    Vector3<real_Num> SkyboxPanorama::getGridOrigin() const
    {
        auto origin = m_originOffset;
        if( auto actor = getActor() )
        {
            origin += actor->getPosition();
        }
        return origin;
    }

    real_Num SkyboxPanorama::getEffectiveSwitchDistance() const
    {
        if( m_switchDistance > 0 )
        {
            return m_switchDistance;
        }
        return std::min( m_spacingX, m_spacingZ ) * static_cast<real_Num>( 0.45 );
    }

    real_Num SkyboxPanorama::getEffectiveMaximumLinkDistance() const
    {
        if( m_maximumLinkDistance > 0 )
        {
            return m_maximumLinkDistance;
        }
        return std::max( m_spacingX, m_spacingZ ) * static_cast<real_Num>( 1.1 );
    }

    s32 SkyboxPanorama::findNearestPanorama( const Vector3<real_Num> &worldPosition ) const
    {
        s32 nearestIndex = -1;
        auto nearestDistanceSquared = std::numeric_limits<real_Num>::max();
        for( const auto &node : m_panoramas )
        {
            const auto distanceSquared = ( node.position - worldPosition ).lengthSquared();
            if( distanceSquared < nearestDistanceSquared )
            {
                nearestIndex = static_cast<s32>( node.index );
                nearestDistanceSquared = distanceSquared;
            }
        }
        return nearestIndex;
    }

    void SkyboxPanorama::resolveRuntimeObjects()
    {
        if( m_cameraActor )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            return;
        }

        auto cameraManager = applicationManager->getCameraManager();
        if( !cameraManager )
        {
            return;
        }

        const String cameraName = m_cameraActorName.c_str();
        if( !cameraName.empty() )
        {
            m_cameraActor = cameraManager->findCamera( cameraName );
        }

        if( m_cameraActor )
        {
            return;
        }

        const auto cameras = cameraManager->getCameras();
        for( const auto &cameraActor : cameras )
        {
            if( cameraActor )
            {
                if( auto camera = cameraActor->getComponent<Camera>() )
                {
                    if( camera->isActive() )
                    {
                        m_cameraActor = cameraActor;
                        return;
                    }
                }
            }
        }

        if( !cameras.empty() )
        {
            m_cameraActor = cameras.front();
        }
    }

    void SkyboxPanorama::resolveSkyboxes()
    {
        if( m_visibleSkybox && m_hiddenSkybox )
        {
            return;
        }

        if( auto actor = getActor() )
        {
            const auto skyboxes = actor->getComponentsByType<Skybox>();
            if( !m_visibleSkybox && !skyboxes.empty() )
            {
                m_visibleSkybox = skyboxes[0];
            }

            for( const auto &skybox : skyboxes )
            {
                if( skybox && skybox != m_visibleSkybox )
                {
                    m_hiddenSkybox = skybox;
                    resetCrossFade();
                    break;
                }
            }
        }
    }

    bool SkyboxPanorama::beginTransition( u32 index, bool moveCamera )
    {
        if( m_gridDirty )
        {
            rebuildPanoramas();
        }

        if( index >= m_panoramas.size() )
        {
            return false;
        }

        if( m_isTransitioning )
        {
            if( !m_allowTransitionInterruption )
            {
                return false;
            }

            // Resolve the image blend to a valid station, but retain the current in-flight
            // camera position so the replacement transition starts without a spatial jump.
            const auto hadCamera = static_cast<bool>( m_cameraActor );
            const auto interruptedPosition =
                hadCamera ? m_cameraActor->getPosition() : Vector3<real_Num>::zero();
            finishTransition();
            if( hadCamera )
            {
                m_cameraActor->setPosition( interruptedPosition );
                m_cameraActor->updateTransform();
            }
        }

        resolveRuntimeObjects();
        resolveSkyboxes();

        m_targetPanorama = static_cast<s32>( index );
        m_transitionElapsed = 0;
        m_transitionProgress = 0;
        m_transitionMovesCamera = moveCamera && m_interpolateCamera && m_cameraActor;
        m_imageSwitched = false;
        m_crossFadeActive = false;

        if( m_cameraActor )
        {
            m_transitionStartPosition = m_cameraActor->getPosition();
        }
        else if( m_activePanorama >= 0 && static_cast<size_t>( m_activePanorama ) < m_panoramas.size() )
        {
            m_transitionStartPosition = m_panoramas[static_cast<u32>( m_activePanorama )].position;
        }
        m_transitionEndPosition = m_panoramas[index].position;

        if( auto camera = getCameraComponent() )
        {
            m_transitionStartFov = camera->getFOV();
        }

        if( m_panoramaBlendMode == PanoramaBlendMode::CrossFade )
        {
            m_crossFadeActive = prepareCrossFade( m_panoramas[index] );
        }

        m_isTransitioning = true;
        return true;
    }

    real_Num SkyboxPanorama::applyTransitionEasing( real_Num progress ) const
    {
        progress = std::clamp<real_Num>( progress, 0, 1 );
        switch( m_transitionEasing )
        {
        case TransitionEasing::Linear:
            return progress;
        case TransitionEasing::SmoothStep:
            return progress * progress *
                   ( static_cast<real_Num>( 3.0 ) - static_cast<real_Num>( 2.0 ) * progress );
        case TransitionEasing::EaseIn:
            return progress * progress;
        case TransitionEasing::EaseOut:
        {
            const auto inverse = static_cast<real_Num>( 1.0 ) - progress;
            return static_cast<real_Num>( 1.0 ) - inverse * inverse;
        }
        case TransitionEasing::EaseInOut:
            if( progress < static_cast<real_Num>( 0.5 ) )
            {
                return static_cast<real_Num>( 4.0 ) * progress * progress * progress;
            }
            else
            {
                const auto inverse =
                    static_cast<real_Num>( -2.0 ) * progress + static_cast<real_Num>( 2.0 );
                return static_cast<real_Num>( 1.0 ) -
                       inverse * inverse * inverse / static_cast<real_Num>( 2.0 );
            }
        default:
            return progress;
        }
    }

    real_Num SkyboxPanorama::getCrossFadeWeight( real_Num progress ) const
    {
        const auto range = m_crossFadeEnd - m_crossFadeStart;
        if( range <= transitionEpsilon )
        {
            return progress >= m_crossFadeEnd ? static_cast<real_Num>( 1.0 )
                                              : static_cast<real_Num>( 0.0 );
        }

        auto weight = std::clamp<real_Num>( ( progress - m_crossFadeStart ) / range, 0, 1 );
        // Smooth the opacity even when camera movement uses linear easing.
        return weight * weight *
               ( static_cast<real_Num>( 3.0 ) - static_cast<real_Num>( 2.0 ) * weight );
    }

    bool SkyboxPanorama::prepareCrossFade( const PanoramaNode &target )
    {
        resolveSkyboxes();
        if( !m_visibleSkybox || !m_hiddenSkybox )
        {
            if( !m_warnedMissingBlendSkybox )
            {
                WP_LOG_WARNING(
                    "Panorama cross-fade requires two Skybox components on the generator actor; "
                    "using the timed image switch fallback." );
                m_warnedMissingBlendSkybox = true;
            }
            return false;
        }

        auto visibleMaterial = m_visibleSkybox->getMaterial();
        auto hiddenMaterial = m_hiddenSkybox->getMaterial();
        if( !visibleMaterial || !hiddenMaterial || visibleMaterial == hiddenMaterial )
        {
            if( !m_warnedMissingBlendSkybox )
            {
                WP_LOG_WARNING(
                    "Panorama cross-fade requires two Skybox components with separate materials; "
                    "using the timed image switch fallback." );
                m_warnedMissingBlendSkybox = true;
            }
            return false;
        }

        applyPanoramaTextures( target, m_hiddenSkybox );

        const auto baseDistance =
            std::max( m_visibleSkybox->getDistance(), m_hiddenSkybox->getDistance() );
        const auto incomingDistance =
            std::max<f32>( 1.0f, baseDistance - static_cast<f32>( m_crossFadeDistanceOffset ) );
        m_visibleSkybox->setDistance( baseDistance );
        m_hiddenSkybox->setDistance( incomingDistance );
        if( auto renderSkybox = m_visibleSkybox->getSkyboxPtr() )
        {
            renderSkybox->setDistance( baseDistance );
        }
        if( auto renderSkybox = m_hiddenSkybox->getSkyboxPtr() )
        {
            renderSkybox->setDistance( incomingDistance );
        }

        visibleMaterial->setOpacity( 1.0f );
        hiddenMaterial->setOpacity( 0.0f );
        return true;
    }

    void SkyboxPanorama::applyCrossFade( real_Num weight )
    {
        if( !m_visibleSkybox || !m_hiddenSkybox )
        {
            return;
        }

        weight = std::clamp<real_Num>( weight, 0, 1 );
        if( auto material = m_visibleSkybox->getMaterial() )
        {
            material->setOpacity( static_cast<f32>( static_cast<real_Num>( 1.0 ) - weight ) );
        }
        if( auto material = m_hiddenSkybox->getMaterial() )
        {
            material->setOpacity( static_cast<f32>( weight ) );
        }
    }

    void SkyboxPanorama::resetCrossFade()
    {
        if( m_visibleSkybox )
        {
            if( auto material = m_visibleSkybox->getMaterial() )
            {
                material->setOpacity( 1.0f );
            }
        }
        if( m_hiddenSkybox )
        {
            if( auto material = m_hiddenSkybox->getMaterial() )
            {
                material->setOpacity( 0.0f );
            }
        }
    }

    auto SkyboxPanorama::getCameraComponent() const -> SmartPtr<Camera>
    {
        return m_cameraActor ? m_cameraActor->getComponent<Camera>() : nullptr;
    }

    bool SkyboxPanorama::activatePanorama( u32 index, bool snapCamera )
    {
        cancelTransition();

        if( m_gridDirty )
        {
            rebuildPanoramas();
        }

        if( index >= m_panoramas.size() )
        {
            return false;
        }

        resolveRuntimeObjects();
        resolveSkyboxes();
        m_activePanorama = static_cast<s32>( index );
        m_targetPanorama = -1;
        m_transitionProgress = 1;
        const auto &node = m_panoramas[index];
        applyPanoramaTextures( node, m_visibleSkybox );
        resetCrossFade();

        if( snapCamera && m_cameraActor )
        {
            // Only position changes: the user's current look direction is retained.
            m_cameraActor->setPosition( node.position );
            m_cameraActor->updateTransform();
        }

        return true;
    }

    void SkyboxPanorama::applyPanoramaTextures( const PanoramaNode &node, SmartPtr<Skybox> skybox )
    {
        if( !skybox )
        {
            resolveSkyboxes();
            skybox = m_visibleSkybox;
        }

        if( !skybox )
        {
            if( !m_warnedMissingSkybox )
            {
                WP_LOG_WARNING(
                    "SkyboxPanoramaGenerator requires a Skybox component on the same actor." );
                m_warnedMissingSkybox = true;
            }
            return;
        }

        for( u8 face = 0; face < static_cast<u8>( node.textureNames.size() ); ++face )
        {
            skybox->setTextureByName( node.textureNames[face], face );
        }
    }

    void SkyboxPanorama::markGridDirty()
    {
        m_gridDirty = true;
    }
}  // namespace workphone::scene
