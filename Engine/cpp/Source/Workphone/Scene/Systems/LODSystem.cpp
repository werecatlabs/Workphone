#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, LODSystem, ComponentSystem );

    LODSystem::LODSystem() = default;

    LODSystem::~LODSystem() = default;

    void LODSystem::load( SmartPtr<ISharedObject> data )
    {
        if( ISharedObject::getLoadingState() == LoadingState::Loaded )
        {
            return;
        }

        IComponentSystem::setLoadingState( LoadingState::Loading );
        ComponentSystem::load( data );
        IComponentSystem::setLoadingState( LoadingState::Loaded );
    }

    void LODSystem::unload( SmartPtr<ISharedObject> data )
    {
        if( ISharedObject::getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }

        IComponentSystem::setLoadingState( LoadingState::Unloading );

        ScopedLock lock( this );
        for( auto *group : m_groups )
        {
            if( group )
            {
                group->restoreAllRenderers();
                group->setComponentSystem( nullptr );
            }
        }

        // Jobs only retain their immutable batch, never the system, so it is safe to release this
        // reference without blocking engine shutdown.
        m_pendingBatch.reset();
        m_hasViewOverride = false;
        m_groupSlots.clear();
        m_groups.clear();
        m_generations.clear();
        m_currentLODs.clear();
        m_appliedRevisions.clear();
        m_components.clear();
        m_loadingStates.clear();
        m_dirtyComponents.clear();
        m_size = 0;

        ComponentSystem::unload( data );
        IComponentSystem::setLoadingState( LoadingState::Unloaded );
    }

    void LODSystem::update()
    {
        if( !isLoaded() )
        {
            return;
        }

        ScopedLock lock( this );

        if( m_pendingBatch )
        {
            if( m_pendingBatch->remainingJobs.load( memory_semantics::acquire ) != 0 )
            {
                return;
            }

            applyBatch( m_pendingBatch );
            m_pendingBatch.reset();
        }

        auto batch = buildBatch();
        if( !batch || batch->slots.empty() )
        {
            return;
        }

        dispatchBatch( batch );
        if( batch->remainingJobs.load( memory_semantics::acquire ) == 0 )
        {
            applyBatch( batch );
        }
        else
        {
            m_pendingBatch = std::move( batch );
        }
    }

    u32 LODSystem::addComponent( SmartPtr<IComponent> component )
    {
        if( !component || !component->isDerived<LODGroup>() )
        {
            return std::numeric_limits<u32>::max();
        }

        ScopedLock lock( this );
        auto group = workphone::static_pointer_cast<LODGroup>( component );
        const auto existing = m_groupSlots.find( group.get() );
        if( existing != m_groupSlots.end() )
        {
            return existing->second;
        }

        const auto slot = ComponentSystem::addComponent( component );
        if( slot == std::numeric_limits<u32>::max() )
        {
            return slot;
        }

        if( slot >= m_groups.size() )
        {
            reserveData( getSize() );
        }

        m_groups[slot] = group.get();
        m_generations[slot] = m_nextGeneration++;
        m_currentLODs[slot] = -1;
        m_appliedRevisions[slot] = 0;
        m_groupSlots[group.get()] = slot;

        group->setComponentSystem( this );
        return slot;
    }

    void LODSystem::removeComponent( SmartPtr<IComponent> component )
    {
        if( !component || !component->isDerived<LODGroup>() )
        {
            return;
        }

        ScopedLock lock( this );
        auto group = workphone::static_pointer_cast<LODGroup>( component );
        const auto it = m_groupSlots.find( group.get() );
        if( it == m_groupSlots.end() )
        {
            return;
        }

        const auto slot = it->second;
        group->restoreAllRenderers();
        group->setComponentSystem( nullptr );
        group->setComponentSystemData( nullptr );

        m_groupSlots.erase( it );
        m_groups[slot] = nullptr;
        m_generations[slot] = m_nextGeneration++;
        m_currentLODs[slot] = -1;
        m_appliedRevisions[slot] = 0;

        setLoadingState( slot, LoadingState::Unallocated );
        setObject( slot, nullptr );
        m_lastFreeSlot = std::min( m_lastFreeSlot.load(), slot );

        m_dirtyComponents.erase(
            std::remove( m_dirtyComponents.begin(), m_dirtyComponents.end(), component.get() ),
            m_dirtyComponents.end() );
    }

    void LODSystem::removeComponent( u32 id )
    {
        ScopedLock lock( this );
        if( id >= m_groups.size() || !m_groups[id] )
        {
            return;
        }

        SmartPtr<IComponent> component = m_groups[id];
        removeComponent( component );
    }

    void LODSystem::setGlobalLODBias( f32 bias )
    {
        ScopedLock lock( this );
        m_globalLODBias = std::max( bias, 0.01f );
    }

    f32 LODSystem::getGlobalLODBias() const
    {
        ScopedLock lock( this );
        return m_globalLODBias;
    }

    void LODSystem::setViewOverride( const View &view )
    {
        ScopedLock lock( this );
        m_viewOverride = view;
        m_hasViewOverride = true;
    }

    void LODSystem::clearViewOverride()
    {
        ScopedLock lock( this );
        m_hasViewOverride = false;
    }

    void LODSystem::setGrainSize( u32 grainSize )
    {
        ScopedLock lock( this );
        m_grainSize = std::max( grainSize, 1u );
    }

    u32 LODSystem::getGrainSize() const
    {
        ScopedLock lock( this );
        return m_grainSize;
    }

    f32 LODSystem::calculatePerspectiveScreenRelativeHeight( f32 worldRadius, f32 distance,
                                                             f32 verticalFovRadians, f32 lodBias )
    {
        const auto safeDistance = std::max( distance, 0.001f );
        const auto tangent =
            std::tan( std::clamp( verticalFovRadians, 0.001f, Math<f32>::pi() - 0.001f ) * 0.5f );
        return worldRadius / std::max( safeDistance * tangent, 0.001f ) * std::max( lodBias, 0.0f );
    }

    f32 LODSystem::calculateOrthographicScreenRelativeHeight( f32 worldRadius, f32 orthographicHeight,
                                                              f32 lodBias )
    {
        return ( worldRadius * 2.0f ) / std::max( orthographicHeight, 0.001f ) *
               std::max( lodBias, 0.0f );
    }

    s32 LODSystem::selectLOD( f32 screenRelativeHeight, const Array<f32> &thresholds, s32 previousLOD,
                              s32 forcedLOD, f32 hysteresis, bool cullBelowLastLOD )
    {
        return selectLOD( screenRelativeHeight, thresholds.data(), thresholds.size(), previousLOD,
                          forcedLOD, hysteresis, cullBelowLastLOD );
    }

    void LODSystem::reserveData( size_t size )
    {
        m_groups.resize( size, nullptr );
        m_generations.resize( size, 0 );
        m_currentLODs.resize( size, -1 );
        m_appliedRevisions.resize( size, 0 );
    }

    SharedPtr<LODSystem::LODBatch> LODSystem::buildBatch() const
    {
        auto batch = workphone::make_shared<LODBatch>();
        auto cameraLODBias = 1.0f;
        if( m_hasViewOverride )
        {
            batch->cameraPosition = m_viewOverride.position;
            batch->verticalFovRadians = m_viewOverride.verticalFovRadians;
            batch->orthographicHeight = m_viewOverride.orthographicHeight;
            batch->nearClipDistance = std::max( m_viewOverride.nearClipDistance, 0.001f );
            batch->orthographic = m_viewOverride.orthographic;
            cameraLODBias = std::max( m_viewOverride.lodBias, 0.01f );
        }
        else
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return nullptr;
            }

            auto cameraManager = applicationManager->getCameraManager();
            if( !cameraManager )
            {
                return nullptr;
            }

            SmartPtr<IGameActor> cameraActor;
            SmartPtr<Camera> cameraComponent;

            const auto tryCamera = [&]( const SmartPtr<IGameActor> &actor ) {
                if( !actor )
                {
                    return false;
                }

                auto camera = actor->getComponent<Camera>();
                if( camera && camera->isActive() )
                {
                    cameraActor = actor;
                    cameraComponent = camera;
                    return true;
                }

                return false;
            };

            if( !tryCamera( cameraManager->getEditorCamera() ) )
            {
                for( const auto &actor : cameraManager->getCameras() )
                {
                    if( tryCamera( actor ) )
                    {
                        break;
                    }
                }
            }

            if( !cameraActor || !cameraComponent )
            {
                return nullptr;
            }

            batch->cameraPosition = cameraActor->getWorldTransform().getPosition();
            batch->verticalFovRadians =
                Math<f32>::DegToRad( std::max( cameraComponent->getFOV(), 0.001f ) );
            batch->orthographicHeight = cameraComponent->getOrthoWindowHeight();
            batch->nearClipDistance = std::max( cameraComponent->getNearClipDistance(), 0.001f );
            batch->orthographic = batch->orthographicHeight > 0.001f;

            if( auto renderCamera = cameraComponent->getCamera() )
            {
                cameraLODBias = std::max( renderCamera->getLodBias(), 0.01f );
            }
        }

        const auto groupCount = m_groupSlots.size();
        batch->worldReferencePoints.reserve( groupCount );
        batch->worldRadii.reserve( groupCount );
        batch->lodBiases.reserve( groupCount );
        batch->hysteresis.reserve( groupCount );
        batch->previousLODs.reserve( groupCount );
        batch->forcedLODs.reserve( groupCount );
        batch->cullBelowLastLOD.reserve( groupCount );
        batch->slots.reserve( groupCount );
        batch->generations.reserve( groupCount );
        batch->revisions.reserve( groupCount );
        batch->thresholdOffsets.reserve( groupCount );
        batch->thresholdCounts.reserve( groupCount );

        for( u32 slot = 0; slot < m_groups.size(); ++slot )
        {
            auto *group = m_groups[slot];
            if( !group || !group->isLoaded() || !group->isEnabled() )
            {
                continue;
            }

            auto actor = group->getActor();
            if( !actor || !actor->isEnabledInScene() )
            {
                continue;
            }

            RecursiveMutex::ScopedLock groupLock( group->m_lodMutex );
            if( !group->m_lodEnabled || group->m_levels.empty() )
            {
                continue;
            }

            const auto worldTransform = actor->getWorldTransform();
            const auto scale = worldTransform.getScale();
            const auto maximumScale = static_cast<f32>( std::max(
                Math<real_Num>::Abs( scale.X() ),
                std::max( Math<real_Num>::Abs( scale.Y() ), Math<real_Num>::Abs( scale.Z() ) ) ) );

            batch->detailOffsets.push_back( static_cast<u32>( batch->worldRadii.size() ) );
            batch->detailCounts.push_back(
                static_cast<u32>( std::max<size_t>( group->m_detailBounds.size(), 1 ) ) );
            if( group->m_detailBounds.empty() )
            {
                batch->worldReferencePoints.push_back(
                    worldTransform.convertLocalToWorldPosition( group->m_localReferencePoint ) );
                batch->worldRadii.push_back( std::max( group->m_size * maximumScale * 0.5f, 0.0005f ) );
            }
            else
            {
                for( const auto &bound : group->m_detailBounds )
                {
                    batch->worldReferencePoints.push_back(
                        worldTransform.convertLocalToWorldPosition( bound.centre ) );
                    batch->worldRadii.push_back(
                        std::max( bound.diameter * maximumScale * .5f, .0005f ) );
                }
            }
            batch->lodBiases.push_back( group->m_lodBias * m_globalLODBias * cameraLODBias );
            batch->hysteresis.push_back( group->m_hysteresis );
            batch->previousLODs.push_back( m_currentLODs[slot] );
            batch->forcedLODs.push_back( group->m_forcedLOD );
            batch->cullBelowLastLOD.push_back( group->m_cullBelowLastLOD ? 1u : 0u );
            batch->slots.push_back( slot );
            batch->generations.push_back( m_generations[slot] );
            batch->revisions.push_back( group->m_revision.load() );
            batch->thresholdOffsets.push_back( static_cast<u32>( batch->thresholds.size() ) );
            batch->thresholdCounts.push_back( static_cast<u32>( group->m_levels.size() ) );

            for( const auto &level : group->m_levels )
            {
                batch->thresholds.push_back( level.screenRelativeHeight );
            }
        }

        batch->results.resize( batch->slots.size(), -1 );
        return batch;
    }

    void LODSystem::dispatchBatch( const SharedPtr<LODBatch> &batch )
    {
        const auto itemCount = batch->slots.size();
        if( itemCount == 0 )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto jobQueue = applicationManager ? applicationManager->getJobQueuePtr() : nullptr;
        // One small batch is cheaper to calculate directly than queue and consume next frame.
        if( itemCount <= m_grainSize || !jobQueue || !jobQueue->isRunning() )
        {
            calculateRange( batch, 0, itemCount );
            return;
        }

        const auto grainSize = static_cast<size_t>( std::max( m_grainSize, 1u ) );
        const auto jobCount = static_cast<u32>( ( itemCount + grainSize - 1 ) / grainSize );
        batch->remainingJobs.store( jobCount, memory_semantics::release );

        for( size_t begin = 0; begin < itemCount; begin += grainSize )
        {
            const auto end = std::min( begin + grainSize, itemCount );
            auto job = jobQueue->startJob( [batch, begin, end]() {
                calculateRange( batch, begin, end );
                batch->remainingJobs.fetch_sub( 1, memory_semantics::release );
            } );

            if( !job )
            {
                calculateRange( batch, begin, end );
                batch->remainingJobs.fetch_sub( 1, memory_semantics::release );
            }
        }
    }

    void LODSystem::applyBatch( const SharedPtr<LODBatch> &batch )
    {
        for( size_t index = 0; index < batch->slots.size(); ++index )
        {
            const auto slot = batch->slots[index];
            if( slot >= m_groups.size() || !m_groups[slot] ||
                m_generations[slot] != batch->generations[index] )
            {
                continue;
            }

            auto *group = m_groups[slot];
            auto actor = group->getActor();
            if( !actor || !actor->isEnabledInScene() || !group->isEnabled() || !group->isLODEnabled() )
            {
                group->restoreAllRenderers();
                continue;
            }

            const auto lodIndex = batch->results[index];
            const auto revision = batch->revisions[index];
            RecursiveMutex::ScopedLock groupLock( group->m_lodMutex );
            if( group->getRevision() != revision )
            {
                continue;
            }

            if( m_currentLODs[slot] != lodIndex || m_appliedRevisions[slot] != revision )
            {
                m_currentLODs[slot] = lodIndex;
                m_appliedRevisions[slot] = revision;
                group->applyLOD( lodIndex );
            }
        }
    }

    void LODSystem::calculateRange( const SharedPtr<LODBatch> &batch, size_t begin, size_t end )
    {
        for( auto index = begin; index < end; ++index )
        {
            f32 screenRelativeHeight = 0.0f;
            const auto detailEnd = batch->detailOffsets[index] + batch->detailCounts[index];
            for( auto detail = batch->detailOffsets[index]; detail < detailEnd; ++detail )
            {
                f32 memberHeight;
                if( batch->orthographic )
                {
                    memberHeight = calculateOrthographicScreenRelativeHeight(
                        batch->worldRadii[detail], batch->orthographicHeight, batch->lodBiases[index] );
                }
                else
                {
                    const auto offset = batch->worldReferencePoints[detail] - batch->cameraPosition;
                    const auto distance =
                        std::max( static_cast<f32>( offset.length() ), batch->nearClipDistance );
                    memberHeight = calculatePerspectiveScreenRelativeHeight(
                        batch->worldRadii[detail], distance, batch->verticalFovRadians,
                        batch->lodBiases[index] );
                }
                screenRelativeHeight = std::max( screenRelativeHeight, memberHeight );
            }

            const auto offset = batch->thresholdOffsets[index];
            const auto count = batch->thresholdCounts[index];
            batch->results[index] =
                selectLOD( screenRelativeHeight, batch->thresholds.data() + offset, count,
                           batch->previousLODs[index], batch->forcedLODs[index],
                           batch->hysteresis[index], batch->cullBelowLastLOD[index] != 0 );
        }
    }

    s32 LODSystem::selectLOD( f32 screenRelativeHeight, const f32 *thresholds, size_t thresholdCount,
                              s32 previousLOD, s32 forcedLOD, f32 hysteresis, bool cullBelowLastLOD )
    {
        if( !thresholds || thresholdCount == 0 )
        {
            return -1;
        }

        const auto maximumLOD = static_cast<s32>( thresholdCount - 1 );
        if( forcedLOD >= 0 )
        {
            return std::clamp( forcedLOD, 0, maximumLOD );
        }

        const auto clampedHysteresis = std::clamp( hysteresis, 0.0f, 0.5f );
        auto lod = previousLOD;
        if( lod < 0 || lod > maximumLOD )
        {
            for( size_t index = 0; index < thresholdCount; ++index )
            {
                if( screenRelativeHeight >= thresholds[index] )
                {
                    return static_cast<s32>( index );
                }
            }

            return cullBelowLastLOD ? -1 : maximumLOD;
        }

        while( lod > 0 )
        {
            const auto upgradeThreshold =
                thresholds[static_cast<size_t>( lod - 1 )] * ( 1.0f + clampedHysteresis );
            if( screenRelativeHeight < upgradeThreshold )
            {
                break;
            }
            --lod;
        }

        while( lod < maximumLOD )
        {
            const auto downgradeThreshold =
                thresholds[static_cast<size_t>( lod )] * ( 1.0f - clampedHysteresis );
            if( screenRelativeHeight >= downgradeThreshold )
            {
                break;
            }
            ++lod;
        }

        if( cullBelowLastLOD && lod == maximumLOD )
        {
            const auto cullThreshold = thresholds[thresholdCount - 1] * ( 1.0f - clampedHysteresis );
            if( screenRelativeHeight < cullThreshold )
            {
                return -1;
            }
        }

        return lod;
    }
}  // namespace workphone::scene
