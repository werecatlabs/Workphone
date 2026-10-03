#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/Renderer.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

#include <algorithm>
#include <limits>
#include <unordered_set>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, LODGroup, Component );

    const String LODGroup::levelsStr = "lodLevels";
    const String LODGroup::levelStr = "lodLevel";
    const String LODGroup::screenRelativeHeightStr = "screenRelativeHeight";
    const String LODGroup::renderersStr = "renderers";
    const String LODGroup::fadeTransitionWidthStr = "fadeTransitionWidth";
    const String LODGroup::localReferencePointStr = "localReferencePoint";
    const String LODGroup::sizeStr = "size";
    const String LODGroup::lodBiasStr = "lodBias";
    const String LODGroup::hysteresisStr = "hysteresis";
    const String LODGroup::forcedLODStr = "forcedLOD";
    const String LODGroup::cullBelowLastLODStr = "cullBelowLastLOD";
    const String LODGroup::lodEnabledStr = "lodEnabled";
    const String LODGroup::recalculateBoundsStr = "Recalculate Bounds";

    LODGroup::LODGroup() = default;

    LODGroup::~LODGroup() = default;

    void LODGroup::load( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Loaded )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );
        Component::load( data );
        sortLevels();
        setLoadingState( LoadingState::Loaded );
    }

    void LODGroup::unload( SmartPtr<ISharedObject> data )
    {
        const auto loadingState = getLoadingState();
        if( loadingState == LoadingState::Unloaded || loadingState == LoadingState::Unallocated )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );
        restoreAllRenderers();
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<Properties> LODGroup::getProperties() const
    {
        auto properties = Component::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        const auto levels = getLevels();
        properties->setProperty( localReferencePointStr, getLocalReferencePoint() );
        properties->setProperty( sizeStr, getSize() );
        properties->setProperty( lodBiasStr, getLODBias() );
        properties->setProperty( hysteresisStr, getHysteresis() );
        properties->setProperty( forcedLODStr, getForcedLOD() );
        properties->setProperty( cullBelowLastLODStr, getCullBelowLastLOD() );
        properties->setProperty( lodEnabledStr, isLODEnabled() );
        properties->setProperty( levelsStr, static_cast<u32>( levels.size() ), true );
        properties->setButtonPressed( recalculateBoundsStr );

        for( const auto &level : levels )
        {
            auto levelProperties = workphone::make_ptr<Properties>();
            levelProperties->setName( levelStr );
            levelProperties->setProperty( screenRelativeHeightStr, level.screenRelativeHeight );
            levelProperties->setProperty( fadeTransitionWidthStr, level.fadeTransitionWidth );
            levelProperties->setProperty( renderersStr, level.renderers );
            properties->addChild( levelProperties );
        }

        return properties;
    }

    void LODGroup::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        Component::setProperties( properties );

        auto referencePoint = getLocalReferencePoint();
        auto size = getSize();
        auto lodBias = getLODBias();
        auto hysteresis = getHysteresis();
        auto forcedLOD = getForcedLOD();
        auto cullBelowLastLOD = getCullBelowLastLOD();
        auto lodEnabled = isLODEnabled();

        properties->getPropertyValue( localReferencePointStr, referencePoint );
        properties->getPropertyValue( sizeStr, size );
        properties->getPropertyValue( lodBiasStr, lodBias );
        properties->getPropertyValue( hysteresisStr, hysteresis );
        properties->getPropertyValue( forcedLODStr, forcedLOD );
        properties->getPropertyValue( cullBelowLastLODStr, cullBelowLastLOD );
        properties->getPropertyValue( lodEnabledStr, lodEnabled );

        setLocalReferencePoint( referencePoint );
        setSize( size );
        setLODBias( lodBias );
        setHysteresis( hysteresis );
        setForcedLOD( forcedLOD );
        setCullBelowLastLOD( cullBelowLastLOD );
        setLODEnabled( lodEnabled );

        const auto levelProperties = properties->getChildrenByName( levelStr );
        if( !levelProperties.empty() )
        {
            Array<LODLevel> levels;
            levels.reserve( levelProperties.size() );

            for( const auto &levelData : levelProperties )
            {
                LODLevel level;
                levelData->getPropertyValue( screenRelativeHeightStr, level.screenRelativeHeight );
                levelData->getPropertyValue( fadeTransitionWidthStr, level.fadeTransitionWidth );
                levelData->getPropertyValue( renderersStr, level.renderers );
                levels.push_back( std::move( level ) );
            }

            setLevels( levels );
        }

        if( properties->isButtonPressed( recalculateBoundsStr ) )
        {
            recalculateBounds();
        }
    }

    void LODGroup::updateVisibility()
    {
        auto actor = getActorPtr();
        if( !isEnabled() || !actor || !actor->isEnabledInScene() )
        {
            restoreAllRenderers();
        }

        incrementRevision();
    }

    void LODGroup::addLevel( const LODLevel &level )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        restoreAllRenderers();

        auto clampedLevel = level;
        clampedLevel.screenRelativeHeight = std::clamp( clampedLevel.screenRelativeHeight, 0.0f, 1.0f );
        clampedLevel.fadeTransitionWidth = std::max( clampedLevel.fadeTransitionWidth, 0.0f );
        m_levels.push_back( std::move( clampedLevel ) );

        std::sort( m_levels.begin(), m_levels.end(), []( const LODLevel &a, const LODLevel &b ) {
            return a.screenRelativeHeight > b.screenRelativeHeight;
        } );
        incrementRevision();
    }

    void LODGroup::addLevel( f32 screenRelativeHeight, const Array<SmartPtr<Renderer>> &renderers )
    {
        LODLevel level;
        level.screenRelativeHeight = screenRelativeHeight;
        level.renderers = renderers;
        addLevel( level );
    }

    void LODGroup::setLevel( size_t index, const LODLevel &level )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        if( index >= m_levels.size() )
        {
            return;
        }

        restoreAllRenderers();
        m_levels[index] = level;
        m_levels[index].screenRelativeHeight =
            std::clamp( m_levels[index].screenRelativeHeight, 0.0f, 1.0f );
        m_levels[index].fadeTransitionWidth = std::max( m_levels[index].fadeTransitionWidth, 0.0f );
        std::sort( m_levels.begin(), m_levels.end(), []( const LODLevel &a, const LODLevel &b ) {
            return a.screenRelativeHeight > b.screenRelativeHeight;
        } );
        incrementRevision();
    }

    void LODGroup::removeLevel( size_t index )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        if( index >= m_levels.size() )
        {
            return;
        }

        restoreAllRenderers();
        m_levels.erase( m_levels.begin() + static_cast<ptrdiff_t>( index ) );
        incrementRevision();
    }

    void LODGroup::clearLevels()
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        restoreAllRenderers();
        m_levels.clear();
        incrementRevision();
    }

    Array<LODLevel> LODGroup::getLevels() const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        return m_levels;
    }

    void LODGroup::setLevels( const Array<LODLevel> &levels )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        restoreAllRenderers();
        m_levels = levels;

        for( auto &level : m_levels )
        {
            level.screenRelativeHeight = std::clamp( level.screenRelativeHeight, 0.0f, 1.0f );
            level.fadeTransitionWidth = std::max( level.fadeTransitionWidth, 0.0f );
        }

        std::sort( m_levels.begin(), m_levels.end(), []( const LODLevel &a, const LODLevel &b ) {
            return a.screenRelativeHeight > b.screenRelativeHeight;
        } );
        incrementRevision();
    }

    size_t LODGroup::getNumLevels() const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        return m_levels.size();
    }

    void LODGroup::setLocalReferencePoint( const Vector3<real_Num> &referencePoint )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        m_localReferencePoint = referencePoint;
        incrementRevision();
    }

    Vector3<real_Num> LODGroup::getLocalReferencePoint() const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        return m_localReferencePoint;
    }

    void LODGroup::setSize( f32 size )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        m_size = std::max( size, 0.001f );
        incrementRevision();
    }

    f32 LODGroup::getSize() const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        return m_size;
    }

    void LODGroup::setLODBias( f32 bias )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        m_lodBias = std::max( bias, 0.01f );
        incrementRevision();
    }

    f32 LODGroup::getLODBias() const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        return m_lodBias;
    }

    void LODGroup::setHysteresis( f32 hysteresis )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        m_hysteresis = std::clamp( hysteresis, 0.0f, 0.5f );
        incrementRevision();
    }

    f32 LODGroup::getHysteresis() const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        return m_hysteresis;
    }

    void LODGroup::setForcedLOD( s32 lod )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        m_forcedLOD = std::max( lod, -1 );
        incrementRevision();
    }

    s32 LODGroup::getForcedLOD() const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        return m_forcedLOD;
    }

    void LODGroup::setCullBelowLastLOD( bool cull )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        m_cullBelowLastLOD = cull;
        incrementRevision();
    }

    bool LODGroup::getCullBelowLastLOD() const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        return m_cullBelowLastLOD;
    }

    void LODGroup::setLODEnabled( bool enabled )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        if( m_lodEnabled == enabled )
        {
            return;
        }

        m_lodEnabled = enabled;
        if( !enabled )
        {
            restoreAllRenderers();
        }
        incrementRevision();
    }

    bool LODGroup::isLODEnabled() const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        return m_lodEnabled;
    }

    void LODGroup::sortLevels()
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        std::sort( m_levels.begin(), m_levels.end(), []( const LODLevel &a, const LODLevel &b ) {
            return a.screenRelativeHeight > b.screenRelativeHeight;
        } );
        incrementRevision();
    }

    bool LODGroup::validate( String *errorMessage ) const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );

        if( m_levels.empty() )
        {
            if( errorMessage )
            {
                *errorMessage = "LODGroup has no levels.";
            }
            return false;
        }

        auto previousThreshold = std::numeric_limits<f32>::max();
        std::unordered_set<const Renderer *> assignedRenderers;

        for( const auto &level : m_levels )
        {
            if( level.screenRelativeHeight < 0.0f || level.screenRelativeHeight > 1.0f )
            {
                if( errorMessage )
                {
                    *errorMessage = "LOD thresholds must be between zero and one.";
                }
                return false;
            }

            if( level.screenRelativeHeight >= previousThreshold )
            {
                if( errorMessage )
                {
                    *errorMessage = "LOD thresholds must be unique and in descending order.";
                }
                return false;
            }

            previousThreshold = level.screenRelativeHeight;

            for( const auto &renderer : level.renderers )
            {
                if( renderer && !assignedRenderers.insert( renderer.get() ).second )
                {
                    if( errorMessage )
                    {
                        *errorMessage = "A renderer is assigned to more than one LOD level.";
                    }
                    return false;
                }
            }
        }

        return true;
    }

    void LODGroup::recalculateBounds()
    {
        if( auto actor = getActorPtr() )
        {
            const auto bounds = actor->getLocalAABB();
            if( bounds.isFinite() && bounds.isValid() && !bounds.isNull() )
            {
                const auto dimensions = bounds.getSize();
                setLocalReferencePoint( bounds.getCenter() );
                setSize( static_cast<f32>(
                    std::max( dimensions.X(), std::max( dimensions.Y(), dimensions.Z() ) ) ) );
                return;
            }
        }

        setLocalReferencePoint( Vector3<real_Num>::zero() );
        setSize( 1.0f );
    }

    void LODGroup::applyLOD( s32 lodIndex )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        for( size_t levelIndex = 0; levelIndex < m_levels.size(); ++levelIndex )
        {
            const auto visible = lodIndex >= 0 && static_cast<size_t>( lodIndex ) == levelIndex;
            for( auto &renderer : m_levels[levelIndex].renderers )
            {
                if( renderer )
                {
                    renderer->setLODVisible( visible );
                }
            }
        }
    }

    void LODGroup::restoreAllRenderers()
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        for( auto &level : m_levels )
        {
            for( auto &renderer : level.renderers )
            {
                if( renderer )
                {
                    renderer->setLODVisible( true );
                }
            }
        }
    }

    u64 LODGroup::getRevision() const
    {
        return m_revision.load();
    }

    void LODGroup::incrementRevision()
    {
        ++m_revision;
    }
}  // namespace workphone::scene
