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
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/System/Job.hpp>
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
        struct CellPayload { SmartPtr<Properties> data; };
        bool finite( const Vector3F &v )
        { return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z ); }
        SmartPtr<Properties> fingerprintData( const Properties &data )
        {
            auto copy = make_ptr<Properties>();
            copy->setName( data.getName() );
            copy->setPropertiesAsArray( data.getPropertiesAsArray() );
            // Component FSM modes follow the destination scene. A deferred
            // transition or play/edit toggle is not an authored cell edit.
            // Normalize only this hash copy; overlays retain complete payloads.
            if( data.hasProperty( GameActorUtil::componentTypeStr ) )
                copy->removeProperty( IComponent::stateStr );
            for( const auto &child : data.getChildren() )
                if( child ) copy->addChild( fingerprintData( *child ) );
            return copy;
        }
        u64 fingerprint( const Properties &data )
        {
            const auto bytes = PropertiesBinarySerializer::serialize( *fingerprintData( data ) );
            u64 hash = 14695981039346656037ull;
            for( auto byte : bytes ) { hash ^= byte; hash *= 1099511628211ull; }
            return hash;
        }
    }
    struct SceneWorldStreamer::Impl
    {
        struct Resident
        {
            Array<SmartPtr<IGameActor>> actors;
            SmartPtr<Properties> source;
            StreamBudget sourceBudget;
            Clock::time_point installed;
            u32 count = 0;
            bool pinned = false;
            bool visible = true;
            u64 baseline = 0;
        };
        struct Overlay { SmartPtr<Properties> data; StreamBudget budget; };
        struct Retired
        {
            Array<SmartPtr<render::IGraphicsObject>> objects;
            Array<SmartPtr<IMeshResource>> meshes;
            StreamBudget sourceBudget;
        };
        std::shared_ptr<WorldStreamingScheduler> scheduler;
        std::shared_ptr<WorldStreamProducer> producer;
        SmartPtr<IGameScene> scene;
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
            ~PublishOnExit() { owner.publishStatistics(); }
        };
        StreamBudget manifestBudget;
        std::map<u64, Resident> residents;
        std::map<u64, WorldStreamResult> ready;
        std::set<u64> demand, failed;
        std::map<u64, u64> representationTargets;
        String failure;
        Array<Retired> retired;
        std::map<u64, Overlay> overlays;
        std::map<String, IMeshResource *> meshCandidates;
        String lastMeshCandidate;
        WeakPtr<MeshManager> retirementManager;
        bool refreshRequests = false;

        // Registration runs on Primary. Coroutine registration itself is not
        // thread-safe in every queue implementation, so it never occurs directly
        // from an Application-task scene close.
        struct RetirementRegistration : Job
        {
            explicit RetirementRegistration( std::shared_ptr<Impl> state ) : state( std::move( state ) )
            { setPrimary( true ); }
            void execute() override
            {
                auto app = core::IApplicationManager::instancePtr();
                if( auto queue = app ? app->getJobQueue() : nullptr; queue && queue->isRunning() )
                    queue->startCoroutine( [cleanup = state]( ICoroutineData::PullType &yield ) {
                        for( ;; )
                        {
                            cleanup->reclaim();
                            if( cleanup->retired.empty() && cleanup->meshCandidates.empty() ) return;
                            yield();
                        }
                    } );
            }
            std::shared_ptr<Impl> state;
        };

        void publishStatistics()
        {
            auto snapshot = stats;
            snapshot.residentCells = static_cast<u32>( residents.size() );
            snapshot.residentActors = 0; snapshot.residentSourceBytes = 0; snapshot.retiringObjects = 0;
            snapshot.residentProxyCells = 0; snapshot.visibleProxyCells = 0;
            for( const auto &resident : residents )
            {
                snapshot.residentActors += resident.second.count;
                snapshot.residentSourceBytes += resident.second.sourceBudget->bytes();
                if( manifest.cells[index.byKey.at( resident.first )].detailKey )
                {
                    ++snapshot.residentProxyCells;
                    if( resident.second.visible ) ++snapshot.visibleProxyCells;
                }
            }
            for( const auto &r : retired )
            {
                snapshot.retiringObjects += static_cast<u32>( r.objects.size() );
                if( r.sourceBudget ) snapshot.residentSourceBytes += r.sourceBudget->bytes();
            }
            snapshot.pendingCells = 0;
            if( producer )
            {
                auto p = producer->stats();
                snapshot.pendingCells = p.pending + p.active + p.ready + static_cast<u32>( ready.size() );
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
                if( actor && actor->isLoaded() )
                {
                    auto root = GameActorUtil::toData( actor );
                    root->setName( ApplicationUtil::actorsStr ); data->addChild( root );
                }
            return data;
        }
        bool dynamic( const SmartPtr<IGameActor> &actor )
        {
            if( !actor->isStatic() ) return true;
            for( const auto &child : actor->getChildren() ) if( dynamic( child ) ) return true;
            return false;
        }
        bool graphicsReady( const SmartPtr<IGameActor> &actor ) const
        {
            for( const auto &renderer : actor->getComponentsByType<Renderer>() )
                if( renderer->isEnabled() && actor->isEnabledInScene() )
                {
                    const auto object = renderer->getGraphicsObject();
                    if( !object || !object->isLoaded() ) return false;
                }
            for( const auto &child : actor->getChildren() ) if( !graphicsReady( child ) ) return false;
            return true;
        }
        bool representationReady( u64 key ) const
        {
            const auto found = residents.find( key );
            if( found == residents.end() ) return false;
            const auto app = core::IApplicationManager::instancePtr();
            // Actor-only/headless scenes have no asynchronous renderer boundary.
            if( !app || !app->getGraphicsSystem() ) return true;
            for( const auto &actor : found->second.actors ) if( !graphicsReady( actor ) ) return false;
            return true;
        }
        void setRepresentationVisible( const SmartPtr<IGameActor> &actor, bool visible )
        {
            // Use the existing LOD visibility channel; authored flags and the
            // serialized edit baseline must not change during a handoff.
            for( const auto &renderer : actor->getComponentsByType<Renderer>() ) renderer->setLODVisible( visible );
            for( const auto &child : actor->getChildren() ) setRepresentationVisible( child, visible );
        }
        void publishRepresentations()
        {
            if( representationTargets.empty() ) return;
            auto app = core::IApplicationManager::instancePtr();
            std::unique_ptr<TaskLock> pause;
            if( auto tasks = app ? app->getTaskManager() : nullptr )
                if( auto renderTask = tasks->getTask( TaskId::Render ); renderTask && renderTask->isLoaded() )
                    pause = std::make_unique<TaskLock>( renderTask );
            for( const auto &target : representationTargets )
            {
                const auto &detail = manifest.cells[index.byKey.at( target.first )];
                const auto other = target.second == detail.key ? detail.proxyKey : detail.key;
                const auto shown = representationReady( target.second ) ? target.second : other;
                for( auto key : { detail.key, detail.proxyKey } )
                {
                    auto found = residents.find( key );
                    if( found == residents.end() ) continue;
                    const bool visible = key == shown;
                    if( found->second.visible == visible ) continue;
                    for( const auto &actor : found->second.actors ) setRepresentationVisible( actor, visible );
                    found->second.visible = visible; ++stats.representationSwitches;
                }
            }
        }
        bool preserve( u64 key, Resident &resident, String &error )
        {
            auto working = scheduler->reserve( StreamMemoryClass::WorkingCpu, resident.sourceBudget->bytes(), error );
            if( !working ) return false;
            auto data = snapshot( resident );
            if( fingerprint( *data ) == resident.baseline ) return true;
            const auto bytes = PropertiesBinarySerializer::serialize( *data ).size() * 8 + u64( resident.count ) * 4096;
            if( bytes > options.maximumDecodedCellBytes )
            { error = "Edited cell exceeds decoded capacity; retain or save it before eviction"; return false; }
            auto budget = scheduler->reserve( StreamMemoryClass::SourceCpu, bytes, error );
            if( !budget ) return false;
            overlays[key] = { data, std::move( budget ) };
            return true;
        }

        void collectObjects( SmartPtr<IGameActor> actor, Retired &retirement )
        {
            for( auto mesh : actor->getComponentsByType<scene::Mesh>() )
                if( auto resource = mesh->getMeshResource() )
                    if( std::find( retirement.meshes.begin(), retirement.meshes.end(), resource ) == retirement.meshes.end() )
                        retirement.meshes.push_back( resource );
            for( auto renderer : actor->getComponentsByType<Renderer>() )
                if( auto object = renderer->getGraphicsObject() ) retirement.objects.push_back( object );
            for( const auto &child : actor->getChildren() ) collectObjects( child, retirement );
        }
        void retire( Resident &resident )
        {
            Retired retirement;
            retirement.sourceBudget = std::move( resident.sourceBudget );
            auto app = core::IApplicationManager::instancePtr();
            if( !retirementManager.lock() && app )
                retirementManager = dynamic_pointer_cast<MeshManager>( app->getMeshManager() );
            auto manager = app ? app->getGameManager() : nullptr;
            for( const auto &actor : resident.actors )
            {
                if( actor && actor->getLoadingState() != LoadingState::Unloaded )
                {
                    collectObjects( actor, retirement );
                    if( manager ) manager->destroyActor( actor );
                }
            }
            resident.actors.clear(); resident.source = nullptr;
            retired.push_back( std::move( retirement ) );
            ++stats.evictedCells;
        }
        void reclaim()
        {
            retired.erase( std::remove_if( retired.begin(), retired.end(), [&]( const Retired &r ) {
                for( const auto &object : r.objects )
                    if( object && object->getLoadingState() != LoadingState::Unloaded ) return false;
                for( const auto &mesh : r.meshes )
                    meshCandidates[mesh->getHandle()->getUUIDAsString()] = mesh.get();
                return true;
            } ), retired.end() );
            auto manager = retirementManager.lock();
            if( !manager ) { meshCandidates.clear(); return; }
            // Identity tickets add neither strong nor weak residency pins. This
            // also lets independent streamers retire the same shared asset.
            auto it = meshCandidates.upper_bound( lastMeshCandidate );
            const auto attempts = std::min<size_t>( 32, meshCandidates.size() );
            for( size_t i = 0; i < attempts; ++i )
            {
                if( it == meshCandidates.end() ) it = meshCandidates.begin();
                auto current = it++;
                lastMeshCandidate = current->first;
                auto mesh = dynamic_pointer_cast<IMeshResource>( manager->getById( current->first ) );
                if( mesh.get() != current->second ) mesh = nullptr; // A replaced cache entry owns its own lifecycle.
                const bool loaded = mesh && mesh->isLoaded();
                if( !mesh || manager->releaseUnusedResource( mesh ) )
                {
                    if( loaded ) ++stats.releasedMeshResources;
                    meshCandidates.erase( current );
                }
            }
        }
    };

    SceneWorldStreamer::SceneWorldStreamer( std::shared_ptr<WorldStreamingScheduler> scheduler ) :
        m_impl( std::make_unique<Impl>() ) { m_impl->scheduler = std::move( scheduler ); }
    SceneWorldStreamer::~SceneWorldStreamer() { close(); }
    bool SceneWorldStreamer::isOpen() const { return m_impl->producer != nullptr; }

    bool SceneWorldStreamer::open( const String &path, SmartPtr<IGameScene> scene,
                                    const SceneWorldStreamOptions &options, String &error )
    {
        error.clear();
        auto &s = *m_impl;
        Impl::PublishOnExit publish{ s };
        try
        {
            if( !s.scheduler || !scene || !scene->isLoaded() ||
                !std::isfinite( options.loadRadius ) || options.loadRadius < 0 ||
                !std::isfinite( options.retainRadius ) || options.retainRadius < options.loadRadius ||
                !std::isfinite( options.proxyLoadRadius ) || !std::isfinite( options.proxyRetainRadius ) ||
                options.proxyLoadRadius < 0 || options.proxyRetainRadius < options.proxyLoadRadius ||
                ( options.enableHlod && options.proxyLoadRadius < options.retainRadius ) ||
                !std::isfinite( options.installMilliseconds ) || options.installMilliseconds <= 0 ||
                !std::isfinite( options.minimumResidencySeconds ) || options.minimumResidencySeconds < 0 ||
                !options.maximumResidentActors || options.maximumResidentActors > WP_MAX_ACTORS ||
                !options.maximumResidentCells || !options.maximumPendingCells || options.maximumPendingCells > 256 ||
                !options.maximumInstallsPerUpdate || !options.maximumDecodedCellBytes )
                throw std::runtime_error( "Invalid scene world streaming configuration" );
            const auto size = std::filesystem::file_size( std::filesystem::path( path.c_str() ) );
            if( size > ( 8ull << 20 ) ) throw std::runtime_error( "World index exceeds limit" );
            auto budget = s.scheduler->reserve( StreamMemoryClass::SourceCpu, size * 8 + 4096, error );
            if( !budget ) return false;
            WorldManifest manifest;
            if( !SceneWorld::readManifest( path, manifest, error ) ) return false;
            for( const auto &cell : manifest.cells )
                if( cell.decodedBytes > options.maximumDecodedCellBytes || cell.actors > options.maximumResidentActors )
                    throw std::runtime_error( "World cell exceeds configured decoded or actor capacity" );
            auto concrete = dynamic_cast<GameScene *>( scene.get() );
            const auto generation = concrete ? concrete->getLoadGeneration() : 0;
            const bool sameSession = s.producer && s.path == path && s.scene == scene &&
                s.sceneGeneration == generation && s.manifest.generation == manifest.generation;
            if( sameSession )
                for( const auto &overlay : s.overlays )
                    if( overlay.second.budget->bytes() > options.maximumDecodedCellBytes )
                        throw std::runtime_error( "Existing edited cell overlay exceeds configured decoded capacity" );
            WorldStreamProducerLimits limits;
            const auto capacity = s.scheduler->stats();
            limits.maximumPending = std::min( options.maximumPendingCells, capacity.maximumPending );
            limits.maximumActive = std::min<u32>( 2, capacity.workers );
            limits.maximumReady = std::min<u32>( 4, capacity.maximumReady );
            if( options.maximumDecodedCellBytes > std::numeric_limits<u64>::max() / limits.maximumReady )
                throw std::runtime_error( "Decoded cell capacity overflows ready budget" );
            limits.maximumReadyBytes = options.maximumDecodedCellBytes * limits.maximumReady;
            auto producer = s.scheduler->createProducer( limits, error );
            if( !producer ) return false;
            SceneWorldIndex index; index.build( manifest );
            if( sameSession )
            {
                // Editing radii/budgets is not a new world session. Retain live
                // actors, promoted cells and state overlays across configuration.
                s.producer->drain(); s.producer = std::move( producer );
                s.ready.clear(); s.demand.clear(); s.failed.clear(); s.failure.clear(); s.representationTargets.clear();
                s.refreshRequests = false; s.options = options;
                s.manifest = std::move( manifest ); s.manifestBudget = std::move( budget );
                s.index = std::move( index ); s.requestLimit = limits.maximumPending;
                s.stats.manifestBytes = size;
                return true;
            }
            close();
            s.path = path; s.manifest = std::move( manifest ); s.options = options;
            s.index = std::move( index ); s.requestLimit = limits.maximumPending;
            s.manifestBudget = std::move( budget ); s.producer = std::move( producer ); s.scene = scene;
            s.sceneGeneration = generation;
            s.stats = {}; s.stats.manifestBytes = size;
            return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    bool SceneWorldStreamer::update( const Array<Vector3F> &observers, String &error )
    {
        error.clear();
        auto &s = *m_impl;
        Impl::PublishOnExit publish{ s };
        if( !isOpen() ) { error = "No streaming world is open"; return false; }
        auto concrete = dynamic_cast<GameScene *>( s.scene.get() );
        if( !s.scene->isLoaded() || ( concrete && concrete->getLoadGeneration() != s.sceneGeneration ) )
        { close(); error = "Scene world session was replaced"; return false; }
        if( observers.empty() || observers.size() > 8 ||
            std::any_of( observers.begin(), observers.end(), []( auto p ) { return !finite( p ); } ) )
        { error = "World streaming requires one to eight finite observers"; return false; }
        try
        {
            s.reclaim();
            // Authored static content can become gameplay content at runtime.
            // Promote its ownership before distance eviction can destroy it.
            for( auto &resident : s.residents )
                if( !s.manifest.cells[s.index.byKey.at( resident.first )].detailKey )
                for( const auto &actor : resident.second.actors )
                    if( actor && s.dynamic( actor ) ) resident.second.pinned = true;
            struct Candidate { const WorldCell *cell; double distance; bool pinned; const WorldCell *fallback = nullptr; };
            Array<Candidate> candidates;
            const auto now = Clock::now();
            Array<u32> nearby;
            s.stats.indexNodesVisited = 0; s.stats.indexCellsTested = 0;
            const auto queryRadius = s.options.enableHlod ? s.options.proxyRetainRadius : s.options.retainRadius;
            for( const auto &observer : observers )
                s.index.query( s.manifest, observer, double( queryRadius ) * queryRadius,
                               nearby, s.stats.indexNodesVisited, s.stats.indexCellsTested );
            // Existing residents must also be considered outside the query for
            // explicit pins and the minimum-residency/teleport policy.
            for( const auto &resident : s.residents )
            {
                const auto &cell = s.manifest.cells[s.index.byKey.at( resident.first )];
                nearby.push_back( s.index.byKey.at( cell.detailKey ? cell.detailKey : cell.key ) );
            }
            if( s.index.persistent != ~u32( 0 ) ) nearby.push_back( s.index.persistent );
            std::sort( nearby.begin(), nearby.end() );
            nearby.erase( std::unique( nearby.begin(), nearby.end() ), nearby.end() );
            for( auto i : nearby )
            {
                const auto &cell = s.manifest.cells[i];
                double distance = std::numeric_limits<double>::max();
                for( const auto &observer : observers ) distance = std::min( distance, cell.distanceSquared( observer ) );
                auto resident = s.residents.find( cell.key );
                const bool present = resident != s.residents.end();
                const bool pinned = cell.id == "persistent" || ( present && ( resident->second.pinned ||
                    ( std::chrono::duration<double>( now - resident->second.installed ).count() <
                        s.options.minimumResidencySeconds && distance < double( s.options.retainRadius ) * s.options.retainRadius * 16 ) ) );
                const auto radius = present ? s.options.retainRadius : s.options.loadRadius;
                const WorldCell *target = nullptr, *fallback = nullptr;
                const WorldCell *proxy = cell.proxyKey ? &s.manifest.cells[s.index.byKey.at( cell.proxyKey )] : nullptr;
                if( pinned || distance <= double( radius ) * radius ) target = &cell;
                else if( s.options.enableHlod && proxy )
                {
                    const auto proxyRadius = s.residents.count( proxy->key ) ? s.options.proxyRetainRadius : s.options.proxyLoadRadius;
                    if( distance <= double( proxyRadius ) * proxyRadius ) target = proxy;
                }
                if( !target ) continue;
                if( proxy && !s.representationReady( target->key ) )
                {
                    // Keep a usable old representation through decoding and GPU
                    // creation. A cold near-cell request also asks for its cheap
                    // skyline proxy while the detailed graph is installed.
                    const auto other = target == proxy ? &cell : proxy;
                    if( s.residents.count( other->key ) || ( target == &cell && s.options.enableHlod ) ) fallback = other;
                }
                candidates.push_back( { target, distance, pinned, fallback } );
            }
            std::stable_sort( candidates.begin(), candidates.end(), []( const auto &a, const auto &b ) {
                return a.pinned != b.pinned ? a.pinned > b.pinned :
                    a.distance != b.distance ? a.distance < b.distance : a.cell->key < b.cell->key;
            } );
            std::set<u64> desired;
            std::map<u64, u64> targets;
            Array<Candidate> admitted;
            u32 actors = 0;
            for( const auto &candidate : candidates )
            {
                const auto *target = candidate.cell;
                auto fallback = candidate.fallback;
                const auto requiredCells = fallback ? 2u : 1u;
                const auto requiredActors = u64( target->actors ) + ( fallback ? fallback->actors : 0 );
                if( desired.size() + requiredCells > s.options.maximumResidentCells ||
                    requiredActors > s.options.maximumResidentActors - actors )
                {
                    if( candidate.pinned ) { error = "Pinned world demand exceeds resident capacity"; return false; }
                    if( !fallback ) continue;
                    // The handoff's temporary pair must fit the same live actor
                    // cap. Keep coarse/old coverage when there is no headroom.
                    ++s.stats.hlodAdmissionFailures;
                    if( !fallback->detailKey && !s.residents.count( fallback->key ) ) continue;
                    target = fallback; fallback = nullptr;
                    if( desired.size() >= s.options.maximumResidentCells ||
                        target->actors > s.options.maximumResidentActors - actors ) continue;
                }
                desired.insert( target->key ); actors += target->actors;
                admitted.push_back( { target, candidate.distance, candidate.pinned } );
                const auto &detail = target->detailKey ? s.manifest.cells[s.index.byKey.at( target->detailKey )] : *target;
                if( detail.proxyKey ) targets[detail.key] = target->key;
                if( fallback )
                {
                    desired.insert( fallback->key ); actors += fallback->actors;
                    admitted.push_back( { fallback, candidate.distance, false } );
                }
            }
            candidates = std::move( admitted );
            // Publish the replacement before the obsolete representation can be
            // retired. This runs even when demand itself has not changed.
            s.representationTargets = std::move( targets );
            s.publishRepresentations();
            // Compare full demand, not the set of unfinished jobs. A stationary
            // observer must not cancel/restart its reads on every update.
            if( desired != s.demand || s.refreshRequests )
            {
                const bool changed = desired != s.demand;
                if( changed )
                {
                    for( auto it = s.ready.begin(); it != s.ready.end(); )
                        if( !desired.count( it->first ) ) it = s.ready.erase( it ); else ++it;
                    s.failed.clear(); s.failure.clear();
                }
                Array<WorldStreamRequest> requests;
                auto reserve = s.producer->budgetAllocator();
                for( const auto &candidate : candidates )
                {
                    const auto cell = *candidate.cell;
                    if( !desired.count( cell.key ) || s.residents.count( cell.key ) ||
                        s.ready.count( cell.key ) || s.failed.count( cell.key ) ) continue;
                    if( requests.size() >= s.requestLimit ) break;
                    WorldStreamRequest request;
                    request.key = cell.key; request.priority = cell.id == "persistent" ? 100 : cell.detailKey ? 10 : 0;
                    request.workingBytes = cell.bytes;
                    request.maximumResultBytes = cell.decodedBytes;
                    const auto path = s.path;
                    request.execute = [cell, path, reserve]( const StreamCancellation &cancel ) {
                        WorldStreamResult result;
                        if( cancel.cancelled() ) return result;
                        String error;
                        auto source = reserve( StreamMemoryClass::SourceCpu, cell.bytes, error );
                        if( !source ) throw std::runtime_error( error.c_str() );
                        auto data = SceneWorld::readCell( path, cell, error );
                        if( !data ) throw std::runtime_error( error.c_str() );
                        if( cancel.cancelled() ) return result;
                        auto payload = std::make_shared<CellPayload>(); payload->data = data;
                        result.payload = payload; result.bytes = cell.decodedBytes;
                        return result;
                    };
                    requests.push_back( std::move( request ) );
                }
                if( !s.producer->replace( requests, error ) ) return false;
                s.refreshRequests = false; s.demand = std::move( desired );
            }
            u32 retiredThisUpdate = 0;
            for( auto it = s.residents.begin(); it != s.residents.end(); )
            {
                if( !s.demand.count( it->first ) && retiredThisUpdate < s.options.maximumInstallsPerUpdate )
                {
                    if( !s.manifest.cells[s.index.byKey.at( it->first )].detailKey &&
                        !s.preserve( it->first, it->second, error ) ) return false;
                    s.retire( it->second ); it = s.residents.erase( it ); ++retiredThisUpdate;
                }
                else ++it;
            }
            WorldStreamResult result;
            while( s.producer->take( result ) )
            {
                ++s.stats.cellReads;
                if( !s.demand.count( result.key ) ) continue;
                if( !result.error.empty() || !result.payload )
                {
                    ++s.stats.failedCells; s.failed.insert( result.key );
                    s.failure = result.error.empty() ? String( "Cell decode returned no payload" ) : result.error;
                }
                else s.ready[result.key] = std::move( result );
            }
            const auto begin = Clock::now();
            u32 installed = 0;
            auto app = core::IApplicationManager::instancePtr();
            auto manager = app->getGameManager();
            for( const auto &candidate : candidates )
            {
                const auto &cell = *candidate.cell;
                auto ready = s.ready.find( cell.key );
                if( ready == s.ready.end() || !s.demand.count( cell.key ) || s.residents.count( cell.key ) ) continue;
                if( installed >= s.options.maximumInstallsPerUpdate ||
                    std::chrono::duration<double,std::milli>( Clock::now() - begin ).count() >= s.options.installMilliseconds ) break;
                if( cell.id != "persistent" )
                {
                    if( s.index.persistent != ~u32( 0 ) &&
                        !s.residents.count( s.manifest.cells[s.index.persistent].key ) ) continue;
                }
                if( s.residents.size() >= s.options.maximumResidentCells ) continue;
                u32 liveActors = 0;
                for( const auto &resident : s.residents ) liveActors += resident.second.count;
                if( cell.actors > s.options.maximumResidentActors - std::min( liveActors, s.options.maximumResidentActors ) ) continue;
                const auto capacity = manager->getActors().size();
                if( manager->getNumActors() > capacity || cell.actors > capacity - manager->getNumActors() ) continue;
                auto overlay = s.overlays.find( cell.key );
                const auto sourceBytes = overlay == s.overlays.end() ? cell.decodedBytes : overlay->second.budget->bytes();
                auto sourceBudget = s.producer->reserve( StreamMemoryClass::SourceCpu, sourceBytes, error );
                if( !sourceBudget ) continue;
                auto payload = static_cast<const CellPayload *>( ready->second.payload.get() );
                Impl::Resident resident;
                resident.source = overlay == s.overlays.end() ? payload->data : overlay->second.data;
                resident.sourceBudget = std::move( sourceBudget );
                resident.count = cell.actors; resident.installed = Clock::now();
                const auto installBegin = Clock::now();
                try
                {
                    // Include scene membership publication in the same render pause.
                    std::unique_ptr<TaskLock> pause;
                    if( auto tasks = app->getTaskManager() )
                        if( auto renderTask = tasks->getTask( TaskId::Render ); renderTask && renderTask->isLoaded() )
                            pause = std::make_unique<TaskLock>( renderTask );
                    resident.actors = GameActorUtil::loadSceneActors(
                        resident.source->getChildrenByName( ApplicationUtil::actorsStr ), s.scene );
                    if( cell.detailKey || cell.proxyKey )
                    {
                        resident.visible = false;
                        for( const auto &actor : resident.actors ) s.setRepresentationVisible( actor, false );
                    }
                    for( auto actor : resident.actors )
                    {
                        // Save the world reference, not a duplicate of whichever
                        // neighborhood happened to be resident in the Editor.
                        actor->setFlag( IGameActor::ActorFlagDontSave, true );
                        s.scene->addActor( actor );
                    }
                    // Whole-scene loads activate actors through sceneLoaded.
                    // A cell joins an already active scene, so adopt its mode
                    // after publishing every root and before capturing edits.
                    const auto mode = s.scene->getState();
                    if( mode == IGameScene::State::Edit || mode == IGameScene::State::Play )
                    {
                        const auto actorMode = mode == IGameScene::State::Edit ?
                            IGameActor::State::Edit : IGameActor::State::Play;
                        for( auto actor : resident.actors ) actor->setState( actorMode, true );
                    }
                    resident.baseline = fingerprint( *s.snapshot( resident ) );
                    s.residents.emplace( cell.key, std::move( resident ) );
                    ++s.stats.installedCells; ++installed;
                }
                catch( ... )
                {
                    s.retire( resident ); s.ready.erase( ready ); s.failed.insert( cell.key );
                    ++s.stats.failedCells; throw;
                }
                s.stats.lastInstallMilliseconds = std::chrono::duration<double,std::milli>( Clock::now() - installBegin ).count();
                if( s.stats.lastInstallMilliseconds > s.options.installMilliseconds ) ++s.stats.installBudgetOvershoots;
                s.ready.erase( ready );
            }
            s.publishRepresentations();
            // Once a bounded request batch drains, request the remainder without
            // cancelling active work or rereading cells which are already ready.
            auto state = s.producer->stats();
            if( !state.pending && !state.active && !state.ready )
                for( auto key : s.demand )
                    if( !s.residents.count( key ) && !s.ready.count( key ) && !s.failed.count( key ) )
                    { s.refreshRequests = true; break; }
            if( !s.failure.empty() ) error = s.failure;
            return error.empty();
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    void SceneWorldStreamer::close()
    {
        auto &s = *m_impl;
        Impl::PublishOnExit publish{ s };
        if( s.producer ) s.producer->drain();
        s.producer.reset(); s.ready.clear(); s.demand.clear(); s.failed.clear();
        s.representationTargets.clear();
        s.refreshRequests = false; s.failure.clear(); s.overlays.clear();
        for( auto &resident : s.residents ) s.retire( resident.second );
        s.residents.clear(); s.reclaim();
        if( !s.retired.empty() || !s.meshCandidates.empty() )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto queue = app ? app->getJobQueue() : nullptr;
            bool deferred = false;
            if( queue && queue->isRunning() )
            {
                // Cleanup outlives the world/coordinator but owns no actor graph,
                // scene, producer or manifest. Native unloads can finish later.
                auto cleanup = std::make_shared<Impl>();
                cleanup->retired = std::move( s.retired );
                cleanup->meshCandidates = std::move( s.meshCandidates );
                cleanup->lastMeshCandidate = s.lastMeshCandidate;
                cleanup->retirementManager = s.retirementManager;
                auto registration = make_ptr<Impl::RetirementRegistration>( cleanup );
                queue->addJob( registration );
                deferred = registration->getState() != IJob::State::Ready;
                if( !deferred )
                {
                    // Queue implementations may reject a paused/quitting app or
                    // a concurrent shutdown. Do not drop work on rejection.
                    s.retired = std::move( cleanup->retired );
                    s.meshCandidates = std::move( cleanup->meshCandidates );
                }
            }
            if( !deferred )
            {
                // Headless callers without an update service can still exhaust
                // currently reclaimable CPU work. External pins remain resident.
                const auto passes = ( s.meshCandidates.size() + 31 ) / 32;
                for( size_t i = 0; i < passes; ++i ) s.reclaim();
            }
        }
        s.retirementManager = nullptr;
        s.scene = nullptr; s.manifest = {}; s.index = {}; s.manifestBudget.reset(); s.path.clear();
    }

    bool SceneWorldStreamer::pinActor( const String &uuid )
    {
        auto &s = *m_impl;
        std::function<bool( const SmartPtr<IGameActor> & )> contains = [&]( const auto &actor ) {
            if( actor->getHandle()->getUUIDAsString() == uuid ) return true;
            for( const auto &child : actor->getChildren() ) if( contains( child ) ) return true;
            return false;
        };
        for( auto &resident : s.residents )
            if( !s.manifest.cells[s.index.byKey.at( resident.first )].detailKey )
            for( const auto &actor : resident.second.actors )
                if( contains( actor ) ) { resident.second.pinned = true; return true; }
        return false;
    }

    SceneWorldStreamStatistics SceneWorldStreamer::statistics() const
    {
        auto &s = *m_impl;
        std::lock_guard<std::mutex> lock( s.statisticsMutex );
        return s.publishedStatistics;
    }
}
