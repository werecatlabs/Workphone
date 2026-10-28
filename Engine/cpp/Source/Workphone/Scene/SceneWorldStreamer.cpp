#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/SceneWorldStreamer.hpp>
#include "SceneWorldIndex.hpp"
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/Components/Renderer.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <chrono>
#include <filesystem>
#include <set>
#include <map>
#include <cmath>
#include <limits>
#include <mutex>

namespace workphone::scene
{
    namespace
    {
        using Clock = std::chrono::steady_clock;
        using InstanceIdentities = std::map<String, String, std::less<String>,
                                            SourceBudgetAllocator<std::pair<const String, String>>>;

        // Intrusive tickets live inside admitted cleanup bodies. The registry
        // needs no allocation when a closing graph must wait for headroom.
        struct ClosingGraph
        {
            const IGameScene *scene = nullptr;
            ClosingGraph *next = nullptr;
        };
        std::mutex closingGraphsMutex;
        ClosingGraph *closingGraphs = nullptr;

        bool hasClosingGraph( const IGameScene *scene )
        {
            std::lock_guard<std::mutex> lock( closingGraphsMutex );
            for( auto *ticket = closingGraphs; ticket; ticket = ticket->next )
                if( ticket->scene == scene ) return true;
            return false;
        }

        struct CellPayload
        {
            SmartPtr<Properties> data;
        };

        struct CellRequest
        {
            WorldCell cell;
            String path;
            std::function<StreamBudget( StreamMemoryClass, u64, String & )> reserve;
            std::shared_ptr<CellPayload> payload;

            static Array<String> copyIds( const Array<String> &ids )
            {
                Array<String> result{ Allocator<String>( sourceBudget ) };
                result.reserve( ids.capacity() );
                result.insert( result.end(), ids.begin(), ids.end() );
                return result;
            }

            CellRequest( const WorldCell &source, const String &path,
                         std::function<StreamBudget( StreamMemoryClass, u64, String & )> reserve ) :
                cell{ source.id, source.file, source.minimum, source.maximum, source.key,
                      source.bytes, source.decodedBytes, source.detailKey, source.proxyKey,
                      source.actors, copyIds( source.actorIds ), copyIds( source.objectIds ) },
                path( path ), reserve( std::move( reserve ) ),
                payload( std::allocate_shared<CellPayload>( SourceBudgetAllocator<CellPayload>{} ) ) {}

            WorldStreamResult operator()( const StreamCancellation &cancel ) const
            {
                WorldStreamResult result;
                if( cancel.cancelled() ) return result;
                String error;
                auto source = reserve( StreamMemoryClass::SourceCpu, cell.bytes, error );
                if( !source ) throw std::runtime_error( error.c_str() );
                auto data = SceneWorld::readCell( path, cell, error );
                if( !data ) throw std::runtime_error( error.c_str() );
                if( cancel.cancelled() ) return result;
                payload->data = data;
                result.payload = payload;
                result.bytes = cell.decodedBytes;
                return result;
            }
        };

        bool finite( const Vector3F &v )
        {
            return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z );
        }

        SmartPtr<Properties> fingerprintData( const Properties &data )
        {
            auto copy = make_ptr<Properties>();
            copy->setName( data.getName() );
            copy->setPropertiesAsArray( data.getPropertiesAsArray() );
            // Component FSM modes follow the destination scene. A deferred
            // transition or play/edit toggle is not an authored cell edit.
            // Normalize only this hash copy; overlays retain complete payloads.
            if( data.hasProperty( GameActorUtil::componentTypeStr ) )
            {
                copy->removeProperty( IComponent::stateStr );
            }
            for( const auto &child : data.getChildren() )
            {
                if( child )
                {
                    copy->addChild( fingerprintData( *child ) );
                }
            }
            return copy;
        }

        u64 fingerprint( const Properties &data )
        {
            const auto bytes = PropertiesBinarySerializer::serialize( *fingerprintData( data ) );
            u64 hash = 14695981039346656037ull;
            for( auto byte : bytes )
            {
                hash ^= byte;
                hash *= 1099511628211ull;
            }
            return hash;
        }

        // Serialized world poses include the prefab owner's placement. Store
        // authored cell poses instead, so moving an instance neither creates an
        // edit overlay nor reapplies the placement when an overlay is reloaded.
        void canonicalInstancePose( Properties &actor, const Transform3F &parent )
        {
            Transform3F local;
            if( auto data = actor.getChild( GameActorUtil::localTransformStr ) )
            {
                local.setProperties( data );
            }
            Transform3F world( parent.getPosition() +
                                   parent.getOrientation() * ( parent.getScale() * local.getPosition() ),
                               parent.getOrientation() * local.getOrientation(),
                               parent.getScale() * local.getScale() );
            actor.removeChild( GameActorUtil::worldTransformStr );
            auto data = world.getProperties();
            data->setName( GameActorUtil::worldTransformStr );
            actor.addChild( data );
            for( auto child : actor.getChildren() )
            {
                if( child->getName() == GameActorUtil::childStr )
                {
                    canonicalInstancePose( *child, world );
                }
            }
        }

        SmartPtr<Properties> remapInstanceData( const Properties &source,
                                                const InstanceIdentities &identities )
        {
            // Match createInstanceData's exact-value/reference-array behavior;
            // only declared actor/component identities enter this map. Resource
            // UUIDs, paths and other external referents retain their identity.
            auto result = make_ptr<Properties>();
            result->setName( source.getName() );
            for( auto property : source.getPropertiesAsArray() )
            {
                auto found = identities.find( property.getValue() );
                if( found != identities.end() )
                {
                    property.setValue( found->second );
                }
                else if( property.getValue().find( ';' ) != String::npos )
                {
                    Array<String> values{ Allocator<String>( sourceBudget ) };
                    StringUtil::parseArray( property.getValue(), values );
                    bool changed = false;
                    for( auto &value : values )
                    {
                        if( auto entry = identities.find( value ); entry != identities.end() )
                        {
                            value = entry->second;
                            changed = true;
                        }
                    }
                    if( changed )
                    {
                        property.setValue( StringUtil::toString( values ) );
                    }
                }
                result->setProperty( property );
            }
            for( const auto &child : source.getChildren() )
            {
                result->addChild( remapInstanceData( *child, identities ) );
            }
            return result;
        }
    }  // namespace

    struct SceneWorldStreamer::Impl : SourceBudgetObject
    {
        struct Resident
        {
            Array<SmartPtr<IGameActor>> actors;
            SmartPtr<Properties> source;
            StreamBudget sourceBudget;
            Clock::time_point installed;
            Clock::time_point nextGraphicsRetry;
            size_t graphicsRetryCursor = 0;
            u32 count = 0;
            bool pinned = false;
            bool visible = true;
            bool rollback = false;
            u64 baseline = 0;
        };

        struct Overlay
        {
            SmartPtr<Properties> data;
            StreamBudget budget;
        };

        struct Retired
        {
            Array<SmartPtr<render::IGraphicsObject>> objects{ Allocator<SmartPtr<render::IGraphicsObject>>( workphone::sourceBudget ) };
            Array<SmartPtr<IMeshResource>> meshes{ Allocator<SmartPtr<IMeshResource>>( workphone::sourceBudget ) };
            StreamBudget sourceBudget;
        };

        std::shared_ptr<WorldStreamingScheduler> scheduler;
        std::shared_ptr<WorldStreamProducer> producer;
        SmartPtr<IGameScene> scene;
        IGameScene::State observedSceneMode = IGameScene::State::None;
        WeakPtr<IGameActor> instanceOwner;
        String instanceOwnerId;
        InstanceIdentities instanceIdentities;
        u64 sceneGeneration = 0;
        String path;
        WorldManifest manifest;
        SceneWorldIndex index;
        u32 requestLimit = 0;
        SceneWorldStreamOptions options;
        SceneWorldStreamStatistics stats;
        mutable std::mutex statisticsMutex;
        SceneWorldStreamStatistics publishedStatistics;

        struct PublishOnExit
        {
            Impl &owner;

            ~PublishOnExit()
            {
                owner.publishStatistics();
            }
        };

        StreamBudget manifestBudget;
        SourceBudgetMap<u64, Resident> residents;
        SourceBudgetMap<u64, WorldStreamResult> ready;
        WorldStreamResult pendingResult;
        bool hasPendingResult = false;
        SourceBudgetSet<u64> demand, failed;
        SourceBudgetSet<u64> invalidProxies;
        SourceBudgetMap<u64, u64> representationTargets;
        String failure;
        Array<Retired> retired{ Allocator<Retired>( sourceBudget ) };
        SourceBudgetMap<u64, Overlay> overlays;
        SourceBudgetMap<String, IMeshResource *> meshCandidates;
        String lastMeshCandidate;
        u64 lastGraphicsRetryCell = 0;
        WeakPtr<MeshManager> retirementManager;
        bool refreshRequests = false;
        bool updating = false, closing = false, closeRequested = false;
        bool opening = false, reclaiming = false;
        struct RetirementRegistration;
        SmartPtr<RetirementRegistration> retirementRegistration;
        std::shared_ptr<Impl> closingCleanup;
        ClosingGraph closingGraph;

        // Registration runs on Primary. Coroutine registration itself is not
        // thread-safe in every queue implementation, so it never occurs directly
        // from an Application-task scene close.
        struct RetirementRegistration : Job, SourceBudgetObject
        {
            using SourceBudgetObject::operator new;
            using SourceBudgetObject::operator delete;
            explicit RetirementRegistration( std::shared_ptr<Impl> state ) : state( std::move( state ) )
            {
                setPrimary( true );
            }

            void execute() override
            {
                auto app = core::IApplicationManager::instancePtr();
                if( auto queue = app ? app->getJobQueue() : nullptr; queue && queue->isRunning() )
                {
                    queue->startCoroutine( [cleanup = state]( ICoroutineData::PullType &yield ) {
                        for( ;; )
                        {
                            cleanup->reclaim();
                            if( !cleanup->hasRetirementWork() )
                            {
                                return;
                            }
                            yield();
                        }
                    } );
                }
            }

            std::shared_ptr<Impl> state;
        };

        ~Impl()
        {
            // If a queue is absent, rejects work or is cancelled during shutdown,
            // actor ownership must still leave the manager. Native/cache owners
            // retain their own lifetimes even when capture admission is exhausted.
            if( !residents.empty() )
            {
                auto app = core::IApplicationManager::instancePtr();
                auto manager = app ? app->getGameManager() : nullptr;
                if( manager )
                    for( auto &resident : residents )
                        for( auto &actor : resident.second.actors )
                            manager->destroyActor( actor );
            }
            unmarkClosingGraph();
        }

        void markClosingGraph()
        {
            std::lock_guard<std::mutex> lock( closingGraphsMutex );
            if( !closingGraph.scene && !residents.empty() )
            {
                closingGraph.scene = scene.get();
                closingGraph.next = closingGraphs;
                closingGraphs = &closingGraph;
            }
        }

        void unmarkClosingGraph()
        {
            std::lock_guard<std::mutex> lock( closingGraphsMutex );
            auto **link = &closingGraphs;
            while( *link && *link != &closingGraph ) link = &( *link )->next;
            if( *link ) *link = closingGraph.next;
            closingGraph = {};
        }

        void retryClosingCleanup()
        {
            if( auto cleanup = closingCleanup )
            {
                cleanup->reclaim();
                if( closingCleanup == cleanup && !cleanup->hasRetirementWork() ) closingCleanup.reset();
            }
        }

        bool hasRetirementWork() const
        {
            return !residents.empty() || !retired.empty() || !meshCandidates.empty() ||
                   closingCleanup != nullptr;
        }

        void publishStatistics()
        {
            auto snapshot = stats;
            snapshot.residentCells = static_cast<u32>( residents.size() );
            snapshot.residentActors = 0;
            snapshot.residentSourceBytes = 0;
            snapshot.retiringObjects = 0;
            snapshot.residentProxyCells = 0;
            snapshot.visibleProxyCells = 0;
            for( const auto &resident : residents )
            {
                snapshot.residentActors += resident.second.count;
                snapshot.residentSourceBytes += resident.second.sourceBudget->bytes();
                if( manifest.cells[index.byKey.at( resident.first )].detailKey )
                {
                    ++snapshot.residentProxyCells;
                    if( resident.second.visible )
                    {
                        ++snapshot.visibleProxyCells;
                    }
                }
            }
            for( const auto &r : retired )
            {
                snapshot.retiringObjects += static_cast<u32>( r.objects.size() );
                if( r.sourceBudget )
                {
                    snapshot.residentSourceBytes += r.sourceBudget->bytes();
                }
            }
            snapshot.pendingCells = 0;
            if( producer )
            {
                auto p = producer->stats();
                snapshot.pendingCells =
                    p.pending + p.active + p.ready + static_cast<u32>( ready.size() ) +
                    ( hasPendingResult ? 1u : 0u );
            }
            // UI/render readers never inspect owner-thread containers, and this
            // short lock is never held while pausing render or running callbacks.
            std::lock_guard<std::mutex> lock( statisticsMutex );
            publishedStatistics = snapshot;
        }

        SmartPtr<Properties> snapshot( const Resident &resident )
        {
            auto data = make_ptr<Properties>();
            for( const auto &actor : resident.actors )
            {
                if( actor && actor->isLoaded() )
                {
                    auto root = GameActorUtil::toData( actor );
                    if( !instanceOwnerId.empty() )
                    {
                        canonicalInstancePose( *root, Transform3F::identity() );
                    }
                    root->setName( ApplicationUtil::actorsStr );
                    data->addChild( root );
                }
            }
            return data;
        }

        bool dynamic( const SmartPtr<IGameActor> &actor )
        {
            if( !actor->isStatic() )
            {
                return true;
            }
            for( const auto &child : actor->getChildren() )
            {
                if( dynamic( child ) )
                {
                    return true;
                }
            }
            return false;
        }

        void registerInstanceUpdates( const SmartPtr<IGameActor> &actor )
        {
            if( !actor || !actor->isLoaded() || actor->getScene() != scene )
            {
                throw std::runtime_error(
                    "Streaming prefab actor was removed before update registration" );
            }
            // Parenting supplies placement and ownership, but does not enroll
            // children in the scene's actor update lists. Persistent gameplay
            // components need the same pre/update/post lifecycle as scene roots.
            scene->registerAllUpdates( actor );
            for( const auto &child : actor->getChildren() )
            {
                registerInstanceUpdates( child );
            }
        }

        bool synchronizeSceneMode()
        {
            const auto mode = scene->getState();
            if( mode == observedSceneMode )
            {
                return true;
            }
            // Synchronize only a scene transition. Gameplay is free to change
            // individual child states while that scene mode remains unchanged.
            observedSceneMode = mode;
            if( mode != IGameScene::State::Edit && mode != IGameScene::State::Play )
            {
                return true;
            }
            const auto actorMode =
                mode == IGameScene::State::Edit ? IGameActor::State::Edit : IGameActor::State::Play;
            for( auto &resident : residents )
            {
                for( auto actor : resident.second.actors )
                {
                    if( closeRequested )
                    {
                        return false;
                    }
                    if( actor && actor->isLoaded() )
                    {
                        actor->setState( actorMode, true );
                    }
                }
            }
            return !closeRequested;
        }

        bool graphicsReady( const SmartPtr<IGameActor> &actor ) const
        {
            for( const auto &renderer : actor->getComponentsByType<Renderer>() )
            {
                if( renderer->isEnabled() && actor->isEnabledInScene() )
                {
                    const auto object = renderer->getGraphicsObject();
                    if( !object || !object->isLoaded() )
                    {
                        return false;
                    }
                }
            }
            for( const auto &child : actor->getChildren() )
            {
                if( !graphicsReady( child ) )
                {
                    return false;
                }
            }
            return true;
        }

        bool representationReady( u64 key ) const
        {
            const auto found = residents.find( key );
            if( found == residents.end() )
            {
                return false;
            }
            const auto app = core::IApplicationManager::instancePtr();
            // Actor-only/headless scenes have no asynchronous renderer boundary.
            if( !app || !app->getGraphicsSystem() )
            {
                return true;
            }
            for( const auto &actor : found->second.actors )
            {
                if( !graphicsReady( actor ) )
                {
                    return false;
                }
            }
            return true;
        }

        void retryRepresentations()
        {
            const auto app = core::IApplicationManager::instancePtr();
            auto graphics = app ? app->getGraphicsSystem() : nullptr;
            if( !graphics )
            {
                return;
            }
            u32 allowance = 8;
            const auto now = Clock::now();
            Array<u64> retryKeys{ Allocator<u64>( sourceBudget ) };
            for( const auto &target : representationTargets )
            {
                if( representationReady( target.second ) )
                {
                    continue;
                }
                retryKeys.push_back( target.second );
                if( invalidProxies.count( target.first ) )
                {
                    continue;
                }
                const auto &detail = manifest.cells[index.byKey.at( target.first )];
                const auto fallback = target.second == detail.key ? detail.proxyKey : detail.key;
                if( fallback && residents.count( fallback ) )
                {
                    retryKeys.push_back( fallback );
                }
            }
            std::sort( retryKeys.begin(), retryKeys.end() );
            retryKeys.erase( std::unique( retryKeys.begin(), retryKeys.end() ), retryKeys.end() );
            auto begin =
                size_t( std::upper_bound( retryKeys.begin(), retryKeys.end(), lastGraphicsRetryCell ) -
                        retryKeys.begin() );
            if( begin == retryKeys.size() )
            {
                begin = 0;
            }
            for( size_t n = 0; n < retryKeys.size() && allowance; ++n )
            {
                const auto key = retryKeys[( begin + n ) % retryKeys.size()];
                auto found = residents.find( key );
                if( !allowance || found == residents.end() || found->second.nextGraphicsRetry > now ||
                    representationReady( key ) )
                {
                    continue;
                }
                auto &resident = found->second;
                lastGraphicsRetryCell = key;
                resident.nextGraphicsRetry = now + std::chrono::milliseconds( 250 );
                Array<SmartPtr<Renderer>> renderers{ Allocator<SmartPtr<Renderer>>( sourceBudget ) };
                auto collect = [&]( auto &&self, const SmartPtr<IGameActor> &actor ) -> void {
                    if( !actor->isEnabledInScene() )
                    {
                        return;
                    }
                    for( const auto &renderer : actor->getComponentsByType<Renderer>() )
                    {
                        if( renderer->isEnabled() )
                        {
                            renderers.push_back( renderer );
                        }
                    }
                    for( const auto &child : actor->getChildren() )
                    {
                        self( self, child );
                    }
                };
                for( const auto &actor : resident.actors )
                {
                    collect( collect, actor );
                }
                for( size_t n = 0; n < renderers.size() && allowance; ++n )
                {
                    const auto renderer = renderers[resident.graphicsRetryCursor++ % renderers.size()];
                    const auto object = renderer->getGraphicsObject();
                    if( object && object->getLoadingState() == LoadingState::Unloaded )
                    {
                        graphics->loadObject( object, true );
                        --allowance;
                        ++stats.graphicsRetryRequests;
                    }
                }
            }
        }

        void setRepresentationVisible( const SmartPtr<IGameActor> &actor, bool visible )
        {
            // Use the existing LOD visibility channel; authored flags and the
            // serialized edit baseline must not change during a handoff.
            for( auto renderer : actor->getComponentsByType<Renderer>() )
            {
                renderer->setLODVisible( visible );
            }
            for( const auto &child : actor->getChildren() )
            {
                setRepresentationVisible( child, visible );
            }
        }

        void publishRepresentations()
        {
            if( representationTargets.empty() )
            {
                return;
            }
            Array<std::pair<Resident *, bool>> changes{ Allocator<std::pair<Resident *, bool>>( sourceBudget ) };
            for( const auto &target : representationTargets )
            {
                const auto &detail = manifest.cells[index.byKey.at( target.first )];
                const auto other = target.second == detail.key
                                       ? ( invalidProxies.count( detail.key ) ? 0 : detail.proxyKey )
                                       : detail.key;
                const auto shown = representationReady( target.second ) ? target.second : other;
                for( auto key : { detail.key, detail.proxyKey } )
                {
                    auto found = residents.find( key );
                    if( found != residents.end() && found->second.visible != ( key == shown ) )
                    {
                        changes.emplace_back( &found->second, key == shown );
                    }
                }
            }
            if( changes.empty() )
            {
                return;
            }
            auto app = core::IApplicationManager::instancePtr();
            SourceBudgetUnique<TaskLock> pause;
            if( auto tasks = app ? app->getTaskManager() : nullptr )
            {
                if( auto renderTask = tasks->getTask( TaskId::Render );
                    renderTask && renderTask->isLoaded() )
                {
                    pause = makeSourceBudgetUnique<TaskLock>( renderTask );
                }
            }
            for( const auto &change : changes )
            {
                for( const auto &actor : change.first->actors )
                {
                    setRepresentationVisible( actor, change.second );
                }
                change.first->visible = change.second;
                ++stats.representationSwitches;
            }
        }

        bool preserve( u64 key, Resident &resident, String &error )
        {
            auto working = scheduler->reserve( StreamMemoryClass::WorkingCpu,
                                               resident.sourceBudget->bytes(), error );
            if( !working )
            {
                return false;
            }
            auto data = snapshot( resident );
            if( fingerprint( *data ) == resident.baseline )
            {
                return true;
            }
            if( manifest.cells[index.byKey.at( key )].proxyKey )
            {
                invalidProxies.insert( key );
            }
            const auto bytes =
                PropertiesBinarySerializer::serialize( *data ).size() * 8 + u64( resident.count ) * 4096;
            if( bytes > options.maximumDecodedCellBytes )
            {
                error = "Edited cell exceeds decoded capacity; retain or save it before eviction";
                return false;
            }
            auto budget = scheduler->reserve( StreamMemoryClass::SourceCpu, bytes, error );
            if( !budget )
            {
                return false;
            }
            overlays[key] = { data, std::move( budget ) };
            return true;
        }

        void collectObjects( SmartPtr<IGameActor> actor, Retired &retirement )
        {
            for( auto mesh : actor->getComponentsByType<scene::Mesh>() )
            {
                if( auto resource = mesh->getMeshResource() )
                {
                    if( std::find( retirement.meshes.begin(), retirement.meshes.end(), resource ) ==
                        retirement.meshes.end() )
                    {
                        retirement.meshes.push_back( resource );
                    }
                }
            }
            for( auto renderer : actor->getComponentsByType<Renderer>() )
            {
                if( auto object = renderer->getGraphicsObject() )
                {
                    retirement.objects.push_back( object );
                }
            }
            for( const auto &child : actor->getChildren() )
            {
                collectObjects( child, retirement );
            }
        }

        void retire( Resident &resident )
        {
            Retired retirement;
            // Stage all inventories before detaching a live source lease or actor.
            retired.reserve( retired.size() + 1 );
            for( const auto &actor : resident.actors )
                if( actor && actor->getLoadingState() != LoadingState::Unloaded )
                    collectObjects( actor, retirement );
            auto app = core::IApplicationManager::instancePtr();
            if( !retirementManager.lock() && app )
            {
                retirementManager = dynamic_pointer_cast<MeshManager>( app->getMeshManager() );
            }
            auto manager = app ? app->getGameManager() : nullptr;
            retirement.sourceBudget = std::move( resident.sourceBudget );
            for( const auto &actor : resident.actors )
            {
                if( actor && actor->getLoadingState() != LoadingState::Unloaded )
                {
                    if( manager )
                    {
                        manager->destroyActor( actor );
                    }
                }
            }
            resident.actors.clear();
            resident.source = nullptr;
            retired.push_back( std::move( retirement ) );
            ++stats.evictedCells;
        }

        void reclaim()
        {
            if( reclaiming ) return;
            reclaiming = true;
            struct Reclaiming { bool &value; ~Reclaiming() { value = false; } } guard{ reclaiming };
            retryClosingCleanup();
            try { reclaimAdmitted(); }
            catch( const std::bad_alloc & ) { /* Retain retirement work for retry. */ }
        }

        void reclaimAdmitted()
        {
            for( auto it = residents.begin(); it != residents.end(); )
            {
                // Only detached cleanup states retire their complete graph here.
                if( producer ) break;
                retire( it->second );
                it = residents.erase( it );
            }
            if( !producer && residents.empty() ) unmarkClosingGraph();
            for( auto it = retired.begin(); it != retired.end(); )
            {
                if( std::any_of( it->objects.begin(), it->objects.end(), []( const auto &object ) {
                        return object && object->getLoadingState() != LoadingState::Unloaded;
                    } ) )
                {
                    ++it;
                    continue;
                }
                for( const auto &mesh : it->meshes )
                    meshCandidates[mesh->getHandle()->getUUIDAsString()] = mesh.get();
                // A denied insertion above retains this entire retirement record.
                // Array's middle erase rebuilds storage. Compact with moves and
                // pop the tail so acknowledged retirement needs no new buffer.
                static_assert( std::is_nothrow_move_assignable_v<Retired> );
                std::move( it + 1, retired.end(), it );
                retired.pop_back();
            }
            // Native unload acknowledgement precedes cache reclamation. The
            // existing graphics owner also sweeps on each update; this pass
            // covers owner-thread/headless close and deferred Primary cleanup.
            auto app = core::IApplicationManager::instancePtr();
            if( auto graphics =
                    app ? dynamic_pointer_cast<render::GraphicsSystem>( app->getGraphicsSystem() )
                        : nullptr )
            {
                const auto released = graphics->releaseUnusedAssets();
                stats.releasedMaterialResources += released.materials;
                stats.releasedTextureResources += released.textures;
            }
            auto manager = retirementManager.lock();
            if( !manager )
            {
                meshCandidates.clear();
                return;
            }
            // Identity tickets add neither strong nor weak residency pins. This
            // also lets independent streamers retire the same shared asset.
            auto it = meshCandidates.upper_bound( lastMeshCandidate );
            const auto attempts = std::min<size_t>( 32, meshCandidates.size() );
            for( size_t i = 0; i < attempts; ++i )
            {
                if( it == meshCandidates.end() )
                {
                    it = meshCandidates.begin();
                }
                auto current = it++;
                lastMeshCandidate = current->first;
                auto mesh = dynamic_pointer_cast<IMeshResource>( manager->getById( current->first ) );
                if( mesh.get() != current->second )
                {
                    mesh = nullptr;  // A replaced cache entry owns its own lifecycle.
                }
                const bool loaded = mesh && mesh->isLoaded();
                if( !mesh || manager->releaseUnusedResource( mesh ) )
                {
                    if( loaded )
                    {
                        ++stats.releasedMeshResources;
                    }
                    meshCandidates.erase( current );
                }
            }
        }
    };

    SceneWorldStreamer::SceneWorldStreamer( std::shared_ptr<WorldStreamingScheduler> scheduler ) :
        m_impl( std::make_unique<Impl>() )
    {
        m_impl->scheduler = std::move( scheduler );
    }

    SceneWorldStreamer::~SceneWorldStreamer()
    {
        close();
    }

    bool SceneWorldStreamer::isOpen() const
    {
        return m_impl->producer != nullptr;
    }

    bool SceneWorldStreamer::open( const String &path, SmartPtr<IGameScene> scene,
                                   const SceneWorldStreamOptions &options, String &error )
    {
        return open( path, scene, options, nullptr, error );
    }

    bool SceneWorldStreamer::open( const String &path, SmartPtr<IGameScene> scene,
                                   const SceneWorldStreamOptions &options,
                                   SmartPtr<IGameActor> instanceOwner, String &error )
    {
        error.clear();
        auto &s = *m_impl;
        if( s.updating || s.closing || s.opening )
        {
            error = "Cannot reopen a streaming world during its update or close";
            return false;
        }
        Impl::PublishOnExit publish{ s };
        s.opening = true;
        struct Opening { bool &value; ~Opening() { value = false; } } opening{ s.opening };
        try
        {
            s.retryClosingCleanup();
            if( !s.scheduler || !scene || !scene->isLoaded() || !std::isfinite( options.loadRadius ) ||
                options.loadRadius < 0 || !std::isfinite( options.retainRadius ) ||
                options.retainRadius < options.loadRadius || !std::isfinite( options.proxyLoadRadius ) ||
                !std::isfinite( options.proxyRetainRadius ) || options.proxyLoadRadius < 0 ||
                options.proxyRetainRadius < options.proxyLoadRadius ||
                ( options.enableHlod && options.proxyLoadRadius < options.retainRadius ) ||
                !std::isfinite( options.installMilliseconds ) || options.installMilliseconds <= 0 ||
                !std::isfinite( options.minimumResidencySeconds ) ||
                options.minimumResidencySeconds < 0 || !options.maximumResidentActors ||
                options.maximumResidentActors > WP_MAX_ACTORS || !options.maximumResidentCells ||
                !options.maximumPendingCells || options.maximumPendingCells > 256 ||
                !options.maximumInstallsPerUpdate || !options.maximumDecodedCellBytes )
            {
                throw std::runtime_error( "Invalid scene world streaming configuration" );
            }
            const auto size = std::filesystem::file_size( std::filesystem::u8path( path.c_str() ) );
            if( size > ( 8ull << 20 ) )
            {
                throw std::runtime_error( "World index exceeds limit" );
            }
            auto budget = s.scheduler->reserve( StreamMemoryClass::SourceCpu, size * 8 + 4096, error );
            if( !budget )
            {
                return false;
            }
            WorldManifest manifest;
            if( !SceneWorld::readManifest( path, manifest, error ) )
            {
                return false;
            }
            String instanceOwnerId;
            InstanceIdentities instanceIdentities;
            if( instanceOwner )
            {
                if( !instanceOwner->isLoaded() || instanceOwner->getScene() != scene ||
                    !instanceOwner->getHandle() ||
                    instanceOwner->getHandle()->getUUIDAsString().empty() )
                {
                    throw std::runtime_error(
                        "A streaming prefab requires a loaded owner in its destination scene" );
                }
                instanceOwnerId = instanceOwner->getHandle()->getUUIDAsString();
                std::set<String, std::less<String>, SourceBudgetAllocator<String>> generated;
                for( const auto &cell : manifest.cells )
                {
                    if( cell.actorIds.size() != cell.actors || cell.objectIds.empty() )
                    {
                        throw std::runtime_error(
                            "Streaming prefab requires cooked actor and component identity inventories; "
                            "recook this world" );
                    }
                    for( const auto &identity : cell.objectIds )
                    {
                        const auto replacement = StringUtil::toString(
                            StringUtil::getUUID( instanceOwnerId + ":" + identity ) );
                        if( replacement == instanceOwnerId || !generated.insert( replacement ).second ||
                            !instanceIdentities.emplace( identity, replacement ).second )
                        {
                            throw std::runtime_error( "Streaming prefab identity namespace collides" );
                        }
                    }
                }
            }
            for( const auto &cell : manifest.cells )
            {
                if( cell.decodedBytes > options.maximumDecodedCellBytes ||
                    cell.actors > options.maximumResidentActors )
                {
                    throw std::runtime_error(
                        "World cell exceeds configured decoded or actor capacity" );
                }
            }
            auto concrete = dynamic_cast<GameScene *>( scene.get() );
            const auto generation = concrete ? concrete->getLoadGeneration() : 0;
            const bool sameSession =
                s.producer && s.path == path && s.scene == scene && s.sceneGeneration == generation &&
                s.manifest.generation == manifest.generation &&
                s.instanceOwner.lock() == instanceOwner && s.instanceOwnerId == instanceOwnerId;
            if( sameSession )
            {
                for( const auto &overlay : s.overlays )
                {
                    if( overlay.second.budget->bytes() > options.maximumDecodedCellBytes )
                    {
                        throw std::runtime_error(
                            "Existing edited cell overlay exceeds configured decoded capacity" );
                    }
                }
            }
            WorldStreamProducerLimits limits;
            const auto capacity = s.scheduler->stats();
            limits.maximumPending = std::min( options.maximumPendingCells, capacity.maximumPending );
            limits.maximumActive = std::min<u32>( 2, capacity.workers );
            limits.maximumReady = std::min<u32>( 4, capacity.maximumReady );
            if( options.maximumDecodedCellBytes > std::numeric_limits<u64>::max() / limits.maximumReady )
            {
                throw std::runtime_error( "Decoded cell capacity overflows ready budget" );
            }
            limits.maximumReadyBytes = options.maximumDecodedCellBytes * limits.maximumReady;
            auto producer = s.scheduler->createProducer( limits, error );
            if( !producer )
            {
                return false;
            }
            SceneWorldIndex index;
            index.build( manifest );
            // Admit empty coordinator sentinels and job metadata before close
            // ever needs to transfer pending retirement at a full SourceCpu cap.
            auto cleanup = std::allocate_shared<Impl>( SourceBudgetAllocator<Impl>{} );
            auto registration = make_ptr<Impl::RetirementRegistration>( cleanup );
            if( sameSession )
            {
                // Editing radii/budgets is not a new world session. Retain live
                // actors, promoted cells and state overlays across configuration.
                s.producer->drain();
                s.producer = std::move( producer );
                s.ready.clear();
                s.pendingResult = {};
                s.hasPendingResult = false;
                s.demand.clear();
                s.failed.clear();
                s.failure.clear();
                s.representationTargets.clear();
                s.refreshRequests = false;
                s.options = options;
                s.manifest = std::move( manifest );
                s.manifestBudget = std::move( budget );
                s.index = std::move( index );
                s.requestLimit = limits.maximumPending;
                s.stats.manifestBytes = size;
                s.retirementRegistration = registration;
                return true;
            }
            close();
            s.path = path;
            s.manifest = std::move( manifest );
            s.options = options;
            s.index = std::move( index );
            s.requestLimit = limits.maximumPending;
            s.manifestBudget = std::move( budget );
            s.producer = std::move( producer );
            s.scene = scene;
            s.instanceOwner = instanceOwner;
            s.instanceOwnerId = std::move( instanceOwnerId );
            s.instanceIdentities = std::move( instanceIdentities );
            s.sceneGeneration = generation;
            s.observedSceneMode = scene->getState();
            s.retirementRegistration = registration;
            s.stats = {};
            s.stats.manifestBytes = size;
            return true;
        }
        catch( const std::bad_alloc & )
        {
            error = "Out of memory";
            return false;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool SceneWorldStreamer::update( const Array<Vector3F> &worldObservers, String &error )
    {
        error.clear();
        auto &s = *m_impl;
        if( s.updating || s.closing || s.opening )
        {
            error = "Reentrant world streaming update or close";
            return false;
        }
        Impl::PublishOnExit publish{ s };
        if( !isOpen() )
        {
            error = "No streaming world is open";
            return false;
        }
        s.updating = true;

        struct UpdateOnExit
        {
            SceneWorldStreamer &stream;
            Impl &state;

            ~UpdateOnExit()
            {
                state.updating = false;
                if( state.closeRequested )
                {
                    state.closeRequested = false;
                    stream.close();
                }
            }
        } operation{ *this, s };

        auto concrete = dynamic_cast<GameScene *>( s.scene.get() );
        if( !s.scene->isLoaded() || ( concrete && concrete->getLoadGeneration() != s.sceneGeneration ) )
        {
            close();
            error = "Scene world session was replaced";
            return false;
        }
        if( worldObservers.empty() || worldObservers.size() > 8 ||
            std::any_of( worldObservers.begin(), worldObservers.end(),
                         []( auto p ) { return !finite( p ); } ) )
        {
            error = "World streaming requires one to eight finite observers";
            return false;
        }
        try
        {
            s.retryClosingCleanup();
            if( hasClosingGraph( s.scene.get() ) )
            {
                error = "World retirement pending";
                return false;
            }
            auto instanceOwner = s.instanceOwner.lock();
            if( !s.instanceOwnerId.empty() && ( !instanceOwner || !instanceOwner->isLoaded() ||
                                                instanceOwner->getScene() != s.scene ) )
            {
                close();
                error = "Streaming prefab owner was removed";
                return false;
            }
            Array<Vector3F> instanceObservers{ Allocator<Vector3F>( sourceBudget ) };
            const auto *observations = &worldObservers;
            Vector3F placementScale = Vector3F::unit();
            double inverseMinimumScale = 1;
            if( instanceOwner )
            {
                const auto placement = instanceOwner->getWorldTransform();
                const auto scale = placement.getScale();
                if( !std::isfinite( scale.x ) || !std::isfinite( scale.y ) ||
                    !std::isfinite( scale.z ) || std::abs( scale.x ) <= 1e-8 ||
                    std::abs( scale.y ) <= 1e-8 || std::abs( scale.z ) <= 1e-8 )
                {
                    throw std::runtime_error(
                        "Streaming prefab placement must have finite nonzero scale" );
                }
                placementScale = Vector3F( float( std::abs( scale.x ) ), float( std::abs( scale.y ) ),
                                           float( std::abs( scale.z ) ) );
                if( !finite( placementScale ) )
                {
                    throw std::runtime_error( "Streaming prefab scale exceeds supported coordinates" );
                }
                inverseMinimumScale =
                    1.0 / std::min( { double( placementScale.x ), double( placementScale.y ),
                                      double( placementScale.z ) } );
                // Array assignment adopts its source allocator. Borrow values
                // while retaining this bounded scratch buffer's SourceCpu policy.
                instanceObservers.assign( worldObservers.begin(), worldObservers.end() );
                for( auto &observer : instanceObservers )
                {
                    const auto local = placement.inverseTransformPoint(
                        Vector3<real_Num>( observer.x, observer.y, observer.z ) );
                    observer = Vector3F( float( local.x ), float( local.y ), float( local.z ) );
                    if( !finite( observer ) )
                    {
                        throw std::runtime_error( "Streaming prefab placement is not invertible" );
                    }
                }
                observations = &instanceObservers;
            }
            const auto &observers = *observations;
            if( !s.synchronizeSceneMode() )
            {
                throw std::runtime_error( "Streaming world was closed during a scene mode transition" );
            }
            s.reclaim();
            for( auto it = s.residents.begin(); it != s.residents.end(); )
            {
                if( it->second.rollback )
                {
                    s.retire( it->second );
                    it = s.residents.erase( it );
                }
                else ++it;
            }
            // Authored static content can become gameplay content at runtime.
            // Promote its ownership before distance eviction can destroy it.
            for( auto &resident : s.residents )
            {
                if( !s.manifest.cells[s.index.byKey.at( resident.first )].detailKey )
                {
                    for( const auto &actor : resident.second.actors )
                    {
                        if( actor && s.dynamic( actor ) )
                        {
                            resident.second.pinned = true;
                        }
                    }
                }
            }

            struct Candidate
            {
                const WorldCell *cell;
                double distance;
                bool pinned;
                const WorldCell *fallback = nullptr;
            };

            Array<Candidate> candidates{ Allocator<Candidate>( sourceBudget ) };
            const auto now = Clock::now();
            Array<u32> nearby{ Allocator<u32>( sourceBudget ) };
            s.stats.indexNodesVisited = 0;
            s.stats.indexCellsTested = 0;
            const auto queryRadius =
                s.options.enableHlod ? s.options.proxyRetainRadius : s.options.retainRadius;
            for( const auto &observer : observers )
            {
                s.index.query(
                    s.manifest, observer,
                    double( queryRadius ) * queryRadius * inverseMinimumScale * inverseMinimumScale,
                    nearby, s.stats.indexNodesVisited, s.stats.indexCellsTested );
            }
            // Existing residents must also be considered outside the query for
            // explicit pins and the minimum-residency/teleport policy.
            for( const auto &resident : s.residents )
            {
                const auto &cell = s.manifest.cells[s.index.byKey.at( resident.first )];
                nearby.push_back( s.index.byKey.at( cell.detailKey ? cell.detailKey : cell.key ) );
            }
            if( s.index.persistent != ~u32( 0 ) )
            {
                nearby.push_back( s.index.persistent );
            }
            std::sort( nearby.begin(), nearby.end() );
            nearby.erase( std::unique( nearby.begin(), nearby.end() ), nearby.end() );
            for( auto i : nearby )
            {
                const auto &cell = s.manifest.cells[i];
                double distance = std::numeric_limits<double>::max();
                for( const auto &observer : observers )
                {
                    if( !instanceOwner )
                    {
                        distance = std::min( distance, cell.distanceSquared( observer ) );
                    }
                    else
                    {
                        // Owner rotation preserves lengths; scaled local
                        // offsets give exact world distance to the placed box.
                        // The index query above expands conservatively by the
                        // smallest absolute scale, retaining world-unit radii.
                        const double x = std::max( { double( cell.minimum.x ) - observer.x, 0.0,
                                                     double( observer.x ) - cell.maximum.x } ) *
                                         placementScale.x;
                        const double y = std::max( { double( cell.minimum.y ) - observer.y, 0.0,
                                                     double( observer.y ) - cell.maximum.y } ) *
                                         placementScale.y;
                        const double z = std::max( { double( cell.minimum.z ) - observer.z, 0.0,
                                                     double( observer.z ) - cell.maximum.z } ) *
                                         placementScale.z;
                        distance = std::min( distance, x * x + y * y + z * z );
                    }
                }
                auto resident = s.residents.find( cell.key );
                const bool present = resident != s.residents.end();
                const bool pinned =
                    cell.id == "persistent" ||
                    ( present &&
                      ( resident->second.pinned ||
                        ( std::chrono::duration<double>( now - resident->second.installed ).count() <
                              s.options.minimumResidencySeconds &&
                          distance <
                              double( s.options.retainRadius ) * s.options.retainRadius * 16 ) ) );
                const auto radius = present ? s.options.retainRadius : s.options.loadRadius;
                const WorldCell *target = nullptr, *fallback = nullptr;
                const WorldCell *proxy =
                    cell.proxyKey ? &s.manifest.cells[s.index.byKey.at( cell.proxyKey )] : nullptr;
                if( pinned || distance <= double( radius ) * radius )
                {
                    target = &cell;
                }
                else if( s.options.enableHlod && proxy )
                {
                    const auto proxyRadius = s.residents.count( proxy->key )
                                                 ? s.options.proxyRetainRadius
                                                 : s.options.proxyLoadRadius;
                    if( distance <= double( proxyRadius ) * proxyRadius )
                    {
                        target = proxy;
                    }
                }
                if( !target )
                {
                    continue;
                }
                if( target == proxy )
                {
                    if( present && !s.invalidProxies.count( cell.key ) )
                    {
                        auto working =
                            s.scheduler->reserve( StreamMemoryClass::WorkingCpu,
                                                  resident->second.sourceBudget->bytes(), error );
                        if( !working )
                        {
                            return false;
                        }
                        if( fingerprint( *s.snapshot( resident->second ) ) != resident->second.baseline )
                        {
                            s.invalidProxies.insert( cell.key );
                        }
                    }
                    // A baked skyline cannot represent an edited/deleted/moved
                    // building. Keep/load its detailed overlay at proxy range;
                    // normal outside-range eviction still preserves that state.
                    if( s.invalidProxies.count( cell.key ) )
                    {
                        target = &cell;
                    }
                }
                if( proxy && !s.representationReady( target->key ) )
                {
                    // Keep a usable old representation through decoding and GPU
                    // creation. A cold near-cell request also asks for its cheap
                    // skyline proxy while the detailed graph is installed.
                    const auto other = target == proxy ? &cell : proxy;
                    if( !s.invalidProxies.count( cell.key ) &&
                        ( s.residents.count( other->key ) ||
                          ( target == &cell && s.options.enableHlod ) ) )
                    {
                        fallback = other;
                    }
                }
                candidates.push_back( { target, distance, pinned, fallback } );
            }
            std::sort( candidates.begin(), candidates.end(), []( const auto &a, const auto &b ) {
                return a.pinned != b.pinned       ? a.pinned > b.pinned
                       : a.distance != b.distance ? a.distance < b.distance
                                                  : a.cell->key < b.cell->key;
            } );
            SourceBudgetSet<u64> desired;
            SourceBudgetMap<u64, u64> targets;
            Array<Candidate> admitted{ Allocator<Candidate>( sourceBudget ) };
            u32 actors = 0;
            for( const auto &candidate : candidates )
            {
                const auto *target = candidate.cell;
                auto fallback = candidate.fallback;
                auto requiredCells = fallback ? 2u : 1u;
                auto requiredActors = u64( target->actors ) + ( fallback ? fallback->actors : 0 );
                if( fallback && !s.residents.count( fallback->key ) &&
                    ( desired.size() + requiredCells > s.options.maximumResidentCells ||
                      requiredActors > s.options.maximumResidentActors - actors ) )
                {
                    // A cold request has no old coverage to preserve. Its
                    // optional bootstrap proxy must not block detail admission.
                    fallback = nullptr;
                    requiredCells = 1;
                    requiredActors = target->actors;
                }
                if( desired.size() + requiredCells > s.options.maximumResidentCells ||
                    requiredActors > s.options.maximumResidentActors - actors )
                {
                    if( candidate.pinned )
                    {
                        error = "Pinned world demand exceeds resident capacity";
                        return false;
                    }
                    if( !fallback )
                    {
                        continue;
                    }
                    // The handoff's temporary pair must fit the same live actor
                    // cap. Keep coarse/old coverage when there is no headroom.
                    ++s.stats.hlodAdmissionFailures;
                    if( !fallback->detailKey && !s.residents.count( fallback->key ) )
                    {
                        continue;
                    }
                    target = fallback;
                    fallback = nullptr;
                    if( desired.size() >= s.options.maximumResidentCells ||
                        target->actors > s.options.maximumResidentActors - actors )
                    {
                        continue;
                    }
                }
                desired.insert( target->key );
                actors += target->actors;
                admitted.push_back( { target, candidate.distance, candidate.pinned } );
                const auto &detail = target->detailKey
                                         ? s.manifest.cells[s.index.byKey.at( target->detailKey )]
                                         : *target;
                if( detail.proxyKey )
                {
                    targets[detail.key] = target->key;
                }
                if( fallback )
                {
                    desired.insert( fallback->key );
                    actors += fallback->actors;
                    admitted.push_back( { fallback, candidate.distance, false } );
                }
            }
            candidates = std::move( admitted );
            // Compare full demand, not the set of unfinished jobs. A stationary
            // observer must not cancel/restart its reads on every update.
            if( desired != s.demand || s.refreshRequests )
            {
                const bool changed = desired != s.demand;
                Array<WorldStreamRequest> requests{ Allocator<WorldStreamRequest>( sourceBudget ) };
                auto reserve = s.producer->budgetAllocator();
                for( const auto &candidate : candidates )
                {
                    const auto &cell = *candidate.cell;
                    if( !desired.count( cell.key ) || s.residents.count( cell.key ) ||
                        s.ready.count( cell.key ) || ( !changed && s.failed.count( cell.key ) ) ||
                        ( s.hasPendingResult && s.pendingResult.key == cell.key ) )
                    {
                        continue;
                    }
                    if( requests.size() >= s.requestLimit )
                    {
                        break;
                    }
                    WorldStreamRequest request;
                    request.key = cell.key;
                    request.priority = cell.id == "persistent" ? 100 : cell.detailKey ? 10 : 0;
                    request.workingBytes = cell.bytes;
                    request.maximumResultBytes = cell.decodedBytes;
                    auto capture = std::allocate_shared<CellRequest>(
                        SourceBudgetAllocator<CellRequest>{}, cell, s.path, reserve );
                    // The owning bridge fits MSVC std::function inline storage;
                    // heavy captured state and its control block are admitted above.
                    request.execute = [capture]( const StreamCancellation &cancel ) {
                        return ( *capture )( cancel );
                    };
                    requests.push_back( std::move( request ) );
                }
                if( !s.producer->replace( requests, error ) )
                {
                    return false;
                }
                if( changed )
                {
                    for( auto it = s.ready.begin(); it != s.ready.end(); )
                    {
                        if( !desired.count( it->first ) ) it = s.ready.erase( it );
                        else ++it;
                    }
                    s.failed.clear();
                    s.failure.clear();
                }
                s.refreshRequests = false;
                s.demand = std::move( desired );
            }
            // All demand/request admission precedes publication of HLOD targets.
            s.representationTargets = std::move( targets );
            s.retryRepresentations();
            s.publishRepresentations();
            u32 retiredThisUpdate = 0;
            for( auto it = s.residents.begin(); it != s.residents.end(); )
            {
                if( !s.demand.count( it->first ) &&
                    retiredThisUpdate < s.options.maximumInstallsPerUpdate )
                {
                    if( !s.manifest.cells[s.index.byKey.at( it->first )].detailKey &&
                        !s.preserve( it->first, it->second, error ) )
                    {
                        return false;
                    }
                    s.retire( it->second );
                    it = s.residents.erase( it );
                    ++retiredThisUpdate;
                }
                else
                {
                    ++it;
                }
            }
            while( s.hasPendingResult || s.producer->take( s.pendingResult ) )
            {
                if( !s.hasPendingResult ) ++s.stats.cellReads;
                s.hasPendingResult = true;
                auto &result = s.pendingResult;
                if( !s.demand.count( result.key ) )
                {
                    s.pendingResult = {};
                    s.hasPendingResult = false;
                    continue;
                }
                if( !result.error.empty() || !result.payload )
                {
                    auto failure = result.error.empty() ? String( "Cell decode returned no payload" )
                                                        : result.error;
                    s.failed.insert( result.key );
                    s.failure = std::move( failure );
                    ++s.stats.failedCells;
                }
                else
                {
                    // Retain the taken payload if map-node admission fails.
                    auto entry = s.ready.try_emplace( result.key ).first;
                    entry->second = std::move( result );
                }
                s.pendingResult = {};
                s.hasPendingResult = false;
            }
            const auto begin = Clock::now();
            u32 installed = 0;
            auto app = core::IApplicationManager::instancePtr();
            auto manager = app->getGameManager();
            for( const auto &candidate : candidates )
            {
                const auto &cell = *candidate.cell;
                auto ready = s.ready.find( cell.key );
                if( ready == s.ready.end() || !s.demand.count( cell.key ) ||
                    s.residents.count( cell.key ) )
                {
                    continue;
                }
                if( installed >= s.options.maximumInstallsPerUpdate ||
                    std::chrono::duration<double, std::milli>( Clock::now() - begin ).count() >=
                        s.options.installMilliseconds )
                {
                    break;
                }
                if( cell.id != "persistent" )
                {
                    if( s.index.persistent != ~u32( 0 ) &&
                        !s.residents.count( s.manifest.cells[s.index.persistent].key ) )
                    {
                        continue;
                    }
                }
                if( s.residents.size() >= s.options.maximumResidentCells )
                {
                    continue;
                }
                u32 liveActors = 0;
                for( const auto &resident : s.residents )
                {
                    liveActors += resident.second.count;
                }
                if( cell.actors > s.options.maximumResidentActors -
                                      std::min( liveActors, s.options.maximumResidentActors ) )
                {
                    continue;
                }
                const auto capacity = manager->getActors().size();
                if( manager->getNumActors() > capacity ||
                    cell.actors > capacity - manager->getNumActors() )
                {
                    continue;
                }
                auto overlay = s.overlays.find( cell.key );
                const auto sourceBytes =
                    overlay == s.overlays.end() ? cell.decodedBytes : overlay->second.budget->bytes();
                auto sourceBudget =
                    s.producer->reserve( StreamMemoryClass::SourceCpu, sourceBytes, error );
                if( !sourceBudget )
                {
                    continue;
                }
                auto payload = static_cast<const CellPayload *>( ready->second.payload.get() );
                // Reserve retirement slots for every live cell before creating actors.
                s.retired.reserve( s.retired.size() + s.residents.size() + 1 );
                // Admit both publication and failure bookkeeping before actor creation.
                SourceBudgetMap<u64, Impl::Resident> admittedResident;
                auto &resident = admittedResident.try_emplace( cell.key ).first->second;
                SourceBudgetSet<u64> admittedFailure;
                admittedFailure.insert( cell.key );
                resident.source = overlay == s.overlays.end() ? payload->data : overlay->second.data;
                if( instanceOwner && overlay == s.overlays.end() )
                {
                    resident.source = remapInstanceData( *resident.source, s.instanceIdentities );
                }
                resident.sourceBudget = std::move( sourceBudget );
                resident.count = cell.actors;
                resident.installed = Clock::now();
                resident.nextGraphicsRetry = resident.installed + std::chrono::milliseconds( 250 );
                const auto installBegin = Clock::now();
                const auto residentSlot =
                    s.residents.insert( admittedResident.extract( cell.key ) ).position;
                const auto admissionEpoch = sourceBudgetDenialCount();
                try
                {
                    // Include scene membership publication in the same render pause.
                    SourceBudgetUnique<TaskLock> pause;
                    if( auto tasks = app->getTaskManager() )
                    {
                        if( auto renderTask = tasks->getTask( TaskId::Render );
                            renderTask && renderTask->isLoaded() )
                        {
                            pause = makeSourceBudgetUnique<TaskLock>( renderTask );
                        }
                    }
                    resident.actors = GameActorUtil::loadSceneActors(
                        resident.source->getChildrenByName( ApplicationUtil::actorsStr ), s.scene );
                    if( s.closeRequested )
                    {
                        throw std::runtime_error(
                            "Streaming world was closed during cell installation" );
                    }
                    if( instanceOwner &&
                        ( !instanceOwner->isLoaded() || instanceOwner->getScene() != s.scene ) )
                    {
                        throw std::runtime_error(
                            "Streaming prefab owner was removed during cell installation" );
                    }
                    if( cell.detailKey || cell.proxyKey )
                    {
                        resident.visible = false;
                        for( const auto &actor : resident.actors )
                        {
                            s.setRepresentationVisible( actor, false );
                        }
                    }
                    for( auto actor : resident.actors )
                    {
                        if( s.closeRequested )
                        {
                            throw std::runtime_error(
                                "Streaming world was closed during cell publication" );
                        }
                        if( instanceOwner &&
                            ( !instanceOwner->isLoaded() || instanceOwner->getScene() != s.scene ) )
                        {
                            throw std::runtime_error(
                                "Streaming prefab owner was removed during cell publication" );
                        }
                        // Save the world reference, not a duplicate of whichever
                        // neighborhood happened to be resident in the Editor.
                        actor->setFlag( IGameActor::ActorFlagDontSave, true );
                        if( instanceOwner )
                        {
                            instanceOwner->addChild( actor );
                            s.registerInstanceUpdates( actor );
                        }
                        else
                        {
                            s.scene->addActor( actor );
                        }
                    }
                    if( instanceOwner &&
                        ( !instanceOwner->isLoaded() || instanceOwner->getScene() != s.scene ) )
                    {
                        throw std::runtime_error(
                            "Streaming prefab owner was removed during cell publication" );
                    }
                    // Whole-scene loads activate actors through sceneLoaded.
                    // A cell joins an already active scene, so adopt its mode
                    // after publishing every root and before capturing edits.
                    const auto mode = s.scene->getState();
                    if( mode == IGameScene::State::Edit || mode == IGameScene::State::Play )
                    {
                        const auto actorMode = mode == IGameScene::State::Edit ? IGameActor::State::Edit
                                                                               : IGameActor::State::Play;
                        for( auto actor : resident.actors )
                        {
                            actor->setState( actorMode, true );
                        }
                    }
                    if( s.closeRequested )
                    {
                        throw std::runtime_error( "Streaming world was closed during cell activation" );
                    }
                    resident.baseline = fingerprint( *s.snapshot( resident ) );
                    if( sourceBudgetDenialCount() != admissionEpoch ) throw std::bad_alloc();
                    ++s.stats.installedCells;
                    ++installed;
                }
                catch( ... )
                {
                    const bool denied = sourceBudgetDenialCount() != admissionEpoch;
                    resident.rollback = true;
                    try
                    {
                        s.retire( resident );
                        s.residents.erase( residentSlot );
                    }
                    catch( const std::bad_alloc & ) {}
                    if( denied ) throw std::bad_alloc();
                    s.ready.erase( ready );
                    s.failed.insert( admittedFailure.extract( cell.key ) );
                    ++s.stats.failedCells;
                    throw;
                }
                s.stats.lastInstallMilliseconds =
                    std::chrono::duration<double, std::milli>( Clock::now() - installBegin ).count();
                if( s.stats.lastInstallMilliseconds > s.options.installMilliseconds )
                {
                    ++s.stats.installBudgetOvershoots;
                }
                s.ready.erase( ready );
            }
            s.publishRepresentations();
            if( s.closeRequested )
            {
                throw std::runtime_error( "Streaming world was closed during its update" );
            }
            // Once a bounded request batch drains, request the remainder without
            // cancelling active work or rereading cells which are already ready.
            auto state = s.producer->stats();
            if( !state.pending && !state.active && !state.ready )
            {
                for( auto key : s.demand )
                {
                    if( !s.residents.count( key ) && !s.ready.count( key ) && !s.failed.count( key ) )
                    {
                        s.refreshRequests = true;
                        break;
                    }
                }
            }
            if( !s.failure.empty() )
            {
                error = s.failure;
            }
            return error.empty();
        }
        catch( const std::bad_alloc & )
        {
            error = "Out of memory";
            return false;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    void SceneWorldStreamer::close()
    {
        auto &s = *m_impl;
        // Scene and component callbacks may ask to close the world from inside
        // an install/retirement. Finish unwinding that transaction before
        // invalidating its manifest, candidate references or resident iterator.
        if( s.updating )
        {
            s.closeRequested = true;
            return;
        }
        if( s.closing )
        {
            return;
        }
        Impl::PublishOnExit publish{ s };
        s.closing = true;

        struct Closing
        {
            bool &value;

            ~Closing()
            {
                value = false;
            }
        } closing{ s.closing };

        s.retryClosingCleanup();

        if( s.producer )
        {
            s.producer->drain();
        }
        s.producer.reset();
        s.ready.clear();
        s.pendingResult = {};
        s.hasPendingResult = false;
        s.demand.clear();
        s.failed.clear();
        s.representationTargets.clear();
        s.invalidProxies.clear();
        s.lastGraphicsRetryCell = 0;
        s.refreshRequests = false;
        s.failure.clear();
        s.overlays.clear();
        // Transfer already-admitted ownership before attempting dynamic resource
        // capture. No new Impl, map sentinels, job body or outer slots are needed.
        auto registration = std::exchange( s.retirementRegistration, nullptr );
        auto cleanup = registration ? registration->state : nullptr;
        if( cleanup )
        {
            cleanup->residents = std::move( s.residents );
            cleanup->retired = std::move( s.retired );
            cleanup->meshCandidates = std::move( s.meshCandidates );
            cleanup->lastMeshCandidate = std::move( s.lastMeshCandidate );
            cleanup->retirementManager = s.retirementManager;
            cleanup->scene = std::exchange( s.scene, nullptr );
            cleanup->closingCleanup = std::move( s.closingCleanup );
            cleanup->markClosingGraph();
        }
        // Demand is closed. Release unused index/document storage before capture
        // can need headroom; deferred residents keep their own documents/leases.
        s.manifest = {};
        s.index.clear();
        s.manifestBudget.reset();
        s.instanceIdentities.clear();
        if( cleanup )
        {
            cleanup->reclaim();
            s.stats.evictedCells += cleanup->stats.evictedCells;
            s.stats.releasedMeshResources += cleanup->stats.releasedMeshResources;
            s.stats.releasedMaterialResources += cleanup->stats.releasedMaterialResources;
            s.stats.releasedTextureResources += cleanup->stats.releasedTextureResources;
            if( cleanup->hasRetirementWork() )
            {
                auto app = core::IApplicationManager::instancePtr();
                auto queue = app ? app->getJobQueue() : nullptr;
                bool deferred = false;
                if( queue && queue->isRunning() )
                {
                    try { queue->addJob( registration ); }
                    catch( const std::bad_alloc & ) {}
                    deferred = registration->getState() != IJob::State::Ready;
                }
                if( !deferred )
                {
                    const auto passes = ( cleanup->meshCandidates.size() + 31 ) / 32 + 1;
                    for( size_t i = 0; i < passes; ++i ) cleanup->reclaim();
                    if( cleanup->hasRetirementWork() ) s.closingCleanup = cleanup;
                }
            }
        }
        s.retirementManager = nullptr;
        s.scene = nullptr;
        s.manifest = {};
        s.index.clear();
        s.manifestBudget.reset();
        s.path.clear();
        s.instanceOwner = nullptr;
        s.instanceOwnerId.clear();
        s.instanceIdentities.clear();
        s.observedSceneMode = IGameScene::State::None;
    }

    bool SceneWorldStreamer::pinActor( const String &uuid )
    {
        auto &s = *m_impl;
        auto contains = [&]( auto &&self, const SmartPtr<IGameActor> &actor ) -> bool {
            if( actor->getHandle()->getUUIDAsString() == uuid )
            {
                return true;
            }
            for( const auto &child : actor->getChildren() )
            {
                if( self( self, child ) )
                {
                    return true;
                }
            }
            return false;
        };
        for( auto &resident : s.residents )
        {
            if( !s.manifest.cells[s.index.byKey.at( resident.first )].detailKey )
            {
                for( const auto &actor : resident.second.actors )
                {
                    if( contains( contains, actor ) )
                    {
                        resident.second.pinned = true;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    SceneWorldStreamStatistics SceneWorldStreamer::statistics() const
    {
        auto &s = *m_impl;
        std::lock_guard<std::mutex> lock( s.statisticsMutex );
        return s.publishedStatistics;
    }
}  // namespace workphone::scene
