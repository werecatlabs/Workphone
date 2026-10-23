#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/SceneWorldStreamer.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/Components/Renderer.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <chrono>
#include <filesystem>
#include <set>
#include <map>
#include <cmath>
#include <limits>

namespace workphone::scene
{
    namespace
    {
        using Clock = std::chrono::steady_clock;
        struct CellPayload { SmartPtr<Properties> data; };
        bool finite( const Vector3F &v )
        { return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z ); }
        u64 fingerprint( const Properties &data )
        {
            const auto bytes = PropertiesBinarySerializer::serialize( data );
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
            u64 baseline = 0;
        };
        struct Overlay { SmartPtr<Properties> data; StreamBudget budget; };
        struct Retired
        {
            Array<SmartPtr<render::IGraphicsObject>> objects;
            StreamBudget sourceBudget;
        };
        std::shared_ptr<WorldStreamingScheduler> scheduler;
        std::shared_ptr<WorldStreamProducer> producer;
        SmartPtr<IGameScene> scene;
        u64 sceneGeneration = 0;
        String path;
        WorldManifest manifest;
        SceneWorldStreamOptions options;
        SceneWorldStreamStatistics stats;
        StreamBudget manifestBudget;
        std::map<u64, Resident> residents;
        std::map<u64, WorldStreamResult> ready;
        std::set<u64> demand, failed;
        String failure;
        Array<Retired> retired;
        std::map<u64, Overlay> overlays;
        bool refreshRequests = false;

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
            for( auto renderer : actor->getComponentsByType<Renderer>() )
                if( auto object = renderer->getGraphicsObject() ) retirement.objects.push_back( object );
            for( const auto &child : actor->getChildren() ) collectObjects( child, retirement );
        }
        void retire( Resident &resident )
        {
            Retired retirement;
            retirement.sourceBudget = std::move( resident.sourceBudget );
            auto app = core::IApplicationManager::instancePtr();
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
            retired.erase( std::remove_if( retired.begin(), retired.end(), []( const Retired &r ) {
                for( const auto &object : r.objects )
                    if( object && object->getLoadingState() != LoadingState::Unloaded ) return false;
                return true;
            } ), retired.end() );
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
        try
        {
            if( !s.scheduler || !scene || !scene->isLoaded() ||
                !std::isfinite( options.loadRadius ) || options.loadRadius < 0 ||
                !std::isfinite( options.retainRadius ) || options.retainRadius < options.loadRadius ||
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
            WorldStreamProducerLimits limits;
            limits.maximumPending = options.maximumPendingCells;
            limits.maximumActive = std::min<u32>( 2, s.scheduler->stats().workers ); limits.maximumReady = 4;
            limits.maximumReadyBytes = options.maximumDecodedCellBytes * 4;
            auto producer = s.scheduler->createProducer( limits, error );
            if( !producer ) return false;
            auto concrete = dynamic_cast<GameScene *>( scene.get() );
            const auto generation = concrete ? concrete->getLoadGeneration() : 0;
            if( s.producer && s.path == path && s.scene == scene && s.sceneGeneration == generation &&
                s.manifest.generation == manifest.generation )
            {
                // Editing radii/budgets is not a new world session. Retain live
                // actors, promoted cells and state overlays across configuration.
                s.producer->drain(); s.producer = std::move( producer );
                s.ready.clear(); s.demand.clear(); s.failed.clear(); s.failure.clear();
                s.refreshRequests = false; s.options = options;
                s.manifest = std::move( manifest ); s.manifestBudget = std::move( budget );
                s.stats.manifestBytes = size;
                return true;
            }
            close();
            s.path = path; s.manifest = std::move( manifest ); s.options = options;
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
                for( const auto &actor : resident.second.actors )
                    if( actor && s.dynamic( actor ) ) resident.second.pinned = true;
            struct Candidate { const WorldCell *cell; double distance; bool pinned; };
            Array<Candidate> candidates;
            const auto now = Clock::now();
            for( const auto &cell : s.manifest.cells )
            {
                double distance = std::numeric_limits<double>::max();
                for( const auto &observer : observers ) distance = std::min( distance, cell.distanceSquared( observer ) );
                auto resident = s.residents.find( cell.key );
                const bool present = resident != s.residents.end();
                const bool pinned = cell.id == "persistent" || ( present && ( resident->second.pinned ||
                    ( std::chrono::duration<double>( now - resident->second.installed ).count() <
                        s.options.minimumResidencySeconds && distance < double( s.options.retainRadius ) * s.options.retainRadius * 16 ) ) );
                const auto radius = present ? s.options.retainRadius : s.options.loadRadius;
                if( pinned || distance <= double( radius ) * radius ) candidates.push_back( { &cell, distance, pinned } );
            }
            std::stable_sort( candidates.begin(), candidates.end(), []( const auto &a, const auto &b ) {
                return a.pinned != b.pinned ? a.pinned > b.pinned :
                    a.distance != b.distance ? a.distance < b.distance : a.cell->key < b.cell->key;
            } );
            std::set<u64> desired;
            u32 actors = 0;
            for( const auto &candidate : candidates )
            {
                if( desired.size() >= s.options.maximumResidentCells ||
                    candidate.cell->actors > s.options.maximumResidentActors - actors )
                {
                    if( candidate.pinned ) { error = "Pinned world demand exceeds resident capacity"; return false; }
                    continue;
                }
                desired.insert( candidate.cell->key ); actors += candidate.cell->actors;
            }
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
                    if( requests.size() >= s.options.maximumPendingCells ) break;
                    WorldStreamRequest request;
                    request.key = cell.key; request.priority = cell.id == "persistent" ? 100 : 0;
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
                    if( !s.preserve( it->first, it->second, error ) ) return false;
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
                    const auto persistent = std::find_if( s.manifest.cells.begin(), s.manifest.cells.end(),
                        []( const auto &c ) { return c.id == "persistent"; } );
                    if( persistent != s.manifest.cells.end() && !s.residents.count( persistent->key ) ) continue;
                }
                u32 liveActors = 0;
                for( const auto &resident : s.residents ) liveActors += resident.second.count;
                if( cell.actors > s.options.maximumResidentActors - std::min( liveActors, s.options.maximumResidentActors ) ) continue;
                const auto capacity = manager->getActors().size();
                if( manager->getNumActors() > capacity || cell.actors > capacity - manager->getNumActors() ) continue;
                auto sourceBudget = s.producer->reserve( StreamMemoryClass::SourceCpu, cell.decodedBytes, error );
                if( !sourceBudget ) continue;
                auto payload = static_cast<const CellPayload *>( ready->second.payload.get() );
                auto overlay = s.overlays.find( cell.key );
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
                    for( auto actor : resident.actors )
                    {
                        // Save the world reference, not a duplicate of whichever
                        // neighborhood happened to be resident in the Editor.
                        actor->setFlag( IGameActor::ActorFlagDontSave, true );
                        s.scene->addActor( actor );
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
        if( s.producer ) s.producer->drain();
        s.producer.reset(); s.ready.clear(); s.demand.clear(); s.failed.clear();
        s.refreshRequests = false; s.failure.clear(); s.overlays.clear();
        for( auto &resident : s.residents ) s.retire( resident.second );
        s.residents.clear(); s.reclaim();
        s.scene = nullptr; s.manifest = {}; s.manifestBudget.reset(); s.path.clear();
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
            for( const auto &actor : resident.second.actors )
                if( contains( actor ) ) { resident.second.pinned = true; return true; }
        return false;
    }

    SceneWorldStreamStatistics SceneWorldStreamer::statistics() const
    {
        auto &s = *m_impl;
        auto stats = s.stats;
        stats.residentCells = static_cast<u32>( s.residents.size() );
        stats.residentActors = 0; stats.residentSourceBytes = 0; stats.retiringObjects = 0;
        for( const auto &resident : s.residents )
        { stats.residentActors += resident.second.count; stats.residentSourceBytes += resident.second.sourceBudget->bytes(); }
        for( const auto &retired : s.retired )
        {
            stats.retiringObjects += static_cast<u32>( retired.objects.size() );
            if( retired.sourceBudget ) stats.residentSourceBytes += retired.sourceBudget->bytes();
        }
        if( s.producer )
        { auto p = s.producer->stats(); stats.pendingCells = p.pending + p.active + p.ready + static_cast<u32>( s.ready.size() ); }
        return stats;
    }
}
