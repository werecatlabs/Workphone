#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/Renderer.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <stdexcept>

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace workphone::scene
{
    namespace
    {
        // Publication and graph edits use the same manager -> graphics -> group
        // order. The graphics lock also fences a renderer's quiesced range owner.
        struct PublicationLock
        {
            SmartPtr<IGameManager> manager;SmartPtr<render::IGraphicsSystem> graphics;
            ScopedLock<IGameManager> managerLock;ScopedLock<render::IGraphicsSystem> graphicsLock;
            explicit PublicationLock(bool enabled):
                manager(enabled&&core::IApplicationManager::instancePtr()?core::IApplicationManager::instancePtr()->getGameManager():nullptr),
                graphics(enabled&&core::IApplicationManager::instancePtr()?core::IApplicationManager::instancePtr()->getGraphicsSystem():nullptr),
                managerLock(manager),graphicsLock(graphics){}
        };
        LODLevel admittedLevel( const LODLevel &input )
        {
            LODLevel result;
            result.screenRelativeHeight = std::clamp( input.screenRelativeHeight, 0.0f, 1.0f );
            result.fadeTransitionWidth = std::max( input.fadeTransitionWidth, 0.0f );
            result.renderers.reserve( input.renderers.capacity() );
            result.renderers.assign( input.renderers.begin(), input.renderers.end() );
            return result;
        }
    }

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

    void LODGroup::configureRuntimeAssets(std::shared_ptr<const resource::RuntimeAssetContext> context)
    {
        if(!context||!context->resolve||isLoaded()||getLoadingState()==LoadingState::Loading||getLoadingState()==LoadingState::Unloading||(m_runtimeAssets&&m_runtimeAssets!=context))
            throw std::runtime_error("LODGroup runtime assets require an unloaded component and resolver");
        m_runtimeAssets=std::move(context);m_runtimeAssetsMode=true;
    }
    s32 LODGroup::getDisplayedLOD() const {RecursiveMutex::ScopedLock lock(m_lodMutex);return m_displayedLOD;}
    void LODGroup::finalizeRuntimeLOD()
    {
        if(!m_runtimeAssetsMode)return;
        PublicationLock publication(true);RecursiveMutex::ScopedLock lock(m_lodMutex);
        if(!m_runtimeAssets||m_levels.empty())throw std::runtime_error("Runtime LOD group requires its captured context and levels");
        for(auto renderer:m_levels.back().renderers){auto mesh=dynamic_pointer_cast<MeshRenderer>(renderer);
            if(!mesh||mesh->isRuntimeLODStreamed()||!mesh->hasRuntimeLODResidency())throw std::runtime_error("Runtime LOD coarsest level must contain ready non-streamed mesh renderers");}
        applyLOD(static_cast<s32>(m_levels.size()-1));
    }

    void LODGroup::load( SmartPtr<ISharedObject> data )
    {
        if(m_runtimeAssetsMode&&!m_runtimeAssets)throw std::runtime_error("LODGroup runtime assets must be reconfigured before load");
        if( getLoadingState() == LoadingState::Loaded )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );
        Component::load( data );
        sortLevels();
        if(m_runtimeAssetsMode&&!m_levels.empty())applyLOD(static_cast<s32>(m_levels.size()-1));
        setLoadingState( LoadingState::Loaded );
    }

    void LODGroup::unload( SmartPtr<ISharedObject> data )
    {
        m_runtimeAssets.reset();
        const auto loadingState = getLoadingState();
        if( loadingState == LoadingState::Unloaded || loadingState == LoadingState::Unallocated )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );
        restoreAllRenderers();
        if(m_runtimeAssetsMode){RecursiveMutex::ScopedLock lock(m_lodMutex);m_levels.clear();m_detailBounds.clear();m_displayedLOD=-1;}
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

        const auto detailBounds = getDetailBounds();
        for( const auto &bound : detailBounds )
        {
            auto child = make_ptr<Properties>();
            child->setName( "detailBound" );
            child->setProperty( "centre", bound.centre );
            child->setProperty( "diameter", bound.diameter );
            properties->addChild( child );
        }
        properties->setProperty( "detailBoundCount", static_cast<u32>( detailBounds.size() ), true );
        return properties;
    }

    void LODGroup::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        if(m_runtimeAssetsMode)
        {
            if(!m_runtimeAssets)throw std::runtime_error("Runtime LOD properties require their captured context");
            struct Resolution {bool previous;~Resolution(){Properties::exchangeObjectResolverFallback(previous);}} resolution{Properties::exchangeObjectResolverFallback(false)};
            auto point=getLocalReferencePoint();auto size=getSize(),bias=getLODBias(),hysteresis=getHysteresis();auto forced=getForcedLOD();auto cull=getCullBelowLastLOD(),enabled=isLODEnabled();
            properties->getPropertyValue(localReferencePointStr,point);properties->getPropertyValue(sizeStr,size);properties->getPropertyValue(lodBiasStr,bias);properties->getPropertyValue(hysteresisStr,hysteresis);
            properties->getPropertyValue(forcedLODStr,forced);properties->getPropertyValue(cullBelowLastLODStr,cull);properties->getPropertyValue(lodEnabledStr,enabled);
            if(!std::isfinite(point.x)||!std::isfinite(point.y)||!std::isfinite(point.z)||!std::isfinite(size)||size<=0||!std::isfinite(bias)||bias<=0||
               !std::isfinite(hysteresis)||hysteresis<0||hysteresis>.5f||properties->isButtonPressed(recalculateBoundsStr))throw std::runtime_error("Invalid runtime LOD bounds/bias/hysteresis or unsupported authoring command");
            auto children=properties->getChildrenByName(levelStr);if(children.empty()||children.size()>16)throw std::runtime_error("Runtime LOD group requires 1 to 16 levels");
            u32 declared=static_cast<u32>(children.size());properties->getPropertyValue(levelsStr,declared);if(declared!=children.size()||forced< -1||forced>=static_cast<s32>(children.size()))throw std::runtime_error("Runtime LOD count/forced level is invalid");
            Array<LODLevel> levels{Allocator<LODLevel>(sourceBudget)};levels.reserve(children.size());
            std::unordered_set<const Renderer *,std::hash<const Renderer *>,std::equal_to<const Renderer *>,SourceBudgetAllocator<const Renderer *>> assigned;f32 previous=std::numeric_limits<f32>::max();size_t total=0;
            for(auto child:children){LODLevel level;child->getPropertyValue(screenRelativeHeightStr,level.screenRelativeHeight);child->getPropertyValue(fadeTransitionWidthStr,level.fadeTransitionWidth);
                if(!std::isfinite(level.screenRelativeHeight)||level.screenRelativeHeight<0||level.screenRelativeHeight>1||level.screenRelativeHeight>=previous||level.fadeTransitionWidth!=0)
                    throw std::runtime_error("Runtime LOD thresholds must descend uniquely; cross-fading is unsupported");previous=level.screenRelativeHeight;
                Array<String> references{Allocator<String>(sourceBudget)};StringUtil::parseArray(child->getProperty(renderersStr),references);
                Array<SmartPtr<Renderer>> objects{Allocator<SmartPtr<Renderer>>(sourceBudget)};child->getPropertyValue(renderersStr,objects);
                if(objects.size()!=references.size())throw std::runtime_error("Runtime LOD reference has the wrong component type");
                if(objects.empty()||objects.size()>64||total+objects.size()>256)throw std::runtime_error("Runtime LOD renderer count exceeds its bound");total+=objects.size();
                for(auto renderer:objects){if(!renderer||!assigned.insert(renderer.get()).second)throw std::runtime_error("Runtime LOD references must be distinct graph-local renderers");level.renderers.push_back(renderer);}
                levels.push_back(std::move(level));}
            Array<LODDetailBound> bounds{Allocator<LODDetailBound>(sourceBudget)};const auto boundProperties=properties->getChildrenByName("detailBound");if(boundProperties.size()>256)throw std::runtime_error("Runtime LOD detail bounds exceed their limit");
            u32 boundCount=static_cast<u32>(boundProperties.size());properties->getPropertyValue("detailBoundCount",boundCount);if(boundCount!=boundProperties.size())throw std::runtime_error("Runtime LOD detail bound count mismatch");
            for(auto child:boundProperties){LODDetailBound bound;child->getPropertyValue("centre",bound.centre);child->getPropertyValue("diameter",bound.diameter);
                if(!std::isfinite(bound.centre.x)||!std::isfinite(bound.centre.y)||!std::isfinite(bound.centre.z)||!std::isfinite(bound.diameter)||bound.diameter<=0)throw std::runtime_error("Runtime LOD detail bound is invalid");bounds.push_back(bound);}
            Component::setProperties(properties);setLevels(levels);setDetailBounds(bounds);setLocalReferencePoint(point);setSize(size);setLODBias(bias);setHysteresis(hysteresis);setForcedLOD(forced);setCullBelowLastLOD(cull);setLODEnabled(enabled);return;
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

        u32 boundCount = 0;
        if( properties->getPropertyValue( "detailBoundCount", boundCount ) )
        {
            Array<LODDetailBound> bounds{ Allocator<LODDetailBound>( sourceBudget ) };
            for( const auto &child : properties->getChildrenByName( "detailBound" ) )
            {
                LODDetailBound bound;
                child->getPropertyValue( "centre", bound.centre );
                child->getPropertyValue( "diameter", bound.diameter );
                bounds.push_back( bound );
            }
            setDetailBounds( bounds );
        }

        const auto levelProperties = properties->getChildrenByName( levelStr );
        if( !levelProperties.empty() )
        {
            Array<LODLevel> levels{ Allocator<LODLevel>( sourceBudget ) };
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

    void LODGroup::setDetailBounds( const Array<LODDetailBound> &bounds )
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        Array<LODDetailBound> candidate{ Allocator<LODDetailBound>( sourceBudget ) };
        candidate.reserve( bounds.capacity() );
        for( auto bound : bounds )
        {
            if( !std::isfinite( bound.diameter ) || bound.diameter <= 0 ||
                !std::isfinite( bound.centre.x ) || !std::isfinite( bound.centre.y ) ||
                !std::isfinite( bound.centre.z ) )
            {
                continue;
            }
            candidate.push_back( bound );
        }
        m_detailBounds.swap( candidate );
        incrementRevision();
    }

    Array<LODDetailBound> LODGroup::getDetailBounds() const
    {
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        return m_detailBounds;
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
        PublicationLock publication(m_runtimeAssetsMode);
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        auto clampedLevel = admittedLevel( level );
        m_levels.reserve( m_levels.size() + 1 );
        restoreAllRenderers();
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
        level.renderers.reserve( renderers.capacity() );
        level.renderers.assign( renderers.begin(), renderers.end() );
        addLevel( level );
    }

    void LODGroup::setLevel( size_t index, const LODLevel &level )
    {
        PublicationLock publication(m_runtimeAssetsMode);
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        if( index >= m_levels.size() )
        {
            return;
        }

        auto candidate = admittedLevel( level );
        restoreAllRenderers();
        m_levels[index] = std::move( candidate );
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
        PublicationLock publication(m_runtimeAssetsMode);
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        if( index >= m_levels.size() )
        {
            return;
        }

        restoreAllRenderers();
        for( size_t i = index + 1; i < m_levels.size(); ++i )
            m_levels[i - 1] = std::move( m_levels[i] );
        m_levels.pop_back();
        incrementRevision();
    }

    void LODGroup::clearLevels()
    {
        PublicationLock publication(m_runtimeAssetsMode);
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
        PublicationLock publication(m_runtimeAssetsMode);
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        Array<LODLevel> candidate{ Allocator<LODLevel>( sourceBudget ) };
        candidate.reserve( levels.capacity() );
        for( const auto &level : levels ) candidate.push_back( admittedLevel( level ) );
        restoreAllRenderers();
        m_levels.swap( candidate );

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
        PublicationLock publication(m_runtimeAssetsMode);
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
        std::unordered_set<const Renderer *, std::hash<const Renderer *>, std::equal_to<const Renderer *>,
            SourceBudgetAllocator<const Renderer *>> assignedRenderers;

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
        PublicationLock publication(m_runtimeAssetsMode);
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        bool streaming=false;for(const auto &level:m_levels)for(const auto &renderer:level.renderers)if(auto mesh=dynamic_pointer_cast<MeshRenderer>(renderer);mesh&&mesh->isRuntimeLODStreamed())streaming=true;
        if(streaming)
        {
            const auto ready=[&](s32 index){if(index<0||index>=static_cast<s32>(m_levels.size()))return false;for(const auto &renderer:m_levels[index].renderers){if(!renderer)return false;if(auto mesh=dynamic_pointer_cast<MeshRenderer>(renderer);mesh&&!mesh->hasRuntimeLODResidency())return false;}return true;};
            s32 display=ready(m_displayedLOD)?m_displayedLOD:-1;
            if(lodIndex>=0&&lodIndex<static_cast<s32>(m_levels.size()))
            {
                // A demotion may retire a large old allocation immediately by
                // falling back to a ready coarser level while the new one stages.
                if(display<0||lodIndex>display)for(s32 coarse=static_cast<s32>(m_levels.size())-1;coarse>=lodIndex;--coarse)if(ready(coarse)){display=coarse;break;}
                for(size_t i=0;i<m_levels.size();++i)if(static_cast<s32>(i)!=display&&static_cast<s32>(i)!=lodIndex)
                    for(auto renderer:m_levels[i].renderers)if(auto mesh=dynamic_pointer_cast<MeshRenderer>(renderer))mesh->releaseRuntimeLOD();
                bool prepared=true;String error;for(auto renderer:m_levels[lodIndex].renderers)if(auto mesh=dynamic_pointer_cast<MeshRenderer>(renderer))prepared=mesh->prepareRuntimeLOD(error,10)&&prepared;
                if(prepared&&ready(lodIndex))display=lodIndex;
            }else display=-1;
            for(size_t i=0;i<m_levels.size();++i)for(auto renderer:m_levels[i].renderers)if(renderer){renderer->setLODVisible(static_cast<s32>(i)==display);
                if(static_cast<s32>(i)!=display&&static_cast<s32>(i)!=lodIndex)if(auto mesh=dynamic_pointer_cast<MeshRenderer>(renderer))mesh->releaseRuntimeLOD();}
            m_displayedLOD=display;return;
        }
        m_displayedLOD=lodIndex;
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
        PublicationLock publication(m_runtimeAssetsMode);
        RecursiveMutex::ScopedLock lock( m_lodMutex );
        for( auto &level : m_levels )
        {
            for( auto &renderer : level.renderers )
            {
                if( renderer )
                {
                    if(auto mesh=dynamic_pointer_cast<MeshRenderer>(renderer);mesh&&mesh->isRuntimeLODStreamed())mesh->releaseRuntimeLOD();
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
