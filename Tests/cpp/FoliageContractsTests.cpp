#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/FoliagePageStream.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePhysics.hpp>
#include <Workphone/Scene/Components/Terrain/FoliageAuthoring.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/StateManager.hpp>
#include <Workphone/Physics/PhysicsManager.hpp>
#include <Workphone/Physics/PhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <WPPhysics/WPPhysics.hpp>
#include <workphone_physics_scene.h>
#include "FoliageAuthoringContracts.hpp"
#include "FoliagePageStreamContracts.hpp"
#include "FoliageImpostorContracts.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>

using namespace workphone;
namespace
{
    void require(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
    std::shared_ptr<render::FoliageWorldData> fixture()
    {
        String error; render::FoliageSpecies species;
        require(scene::generateFoliagePlant({},species,error),error.c_str());
        species.collisionRadius=.5f; species.collisionHeight=4; species.collisionDistance=10;
        auto world=std::make_shared<render::FoliageWorldData>(); world->pageSize=32;
        world->species.push_back(std::make_shared<const render::FoliageSpecies>(std::move(species)));
        for(u32 n=0;n<4;++n)
        { render::FoliageInstance i; i.id=std::numeric_limits<u64>::max()-n; i.position={float(n)*32-2,0,0}; world->instances.push_back(i); }
        return world;
    }
    void dataContracts()
    {
        auto world=fixture(); String error,text;
        require(render::validateFoliageWorld(*world,error),error.c_str());
        require(render::serializeFoliageWorld(*world,text,error),error.c_str());
        render::FoliageWorldData restored;
        require(render::deserializeFoliageWorld(text,restored,error),error.c_str());
        require(restored.instances[0].id==std::numeric_limits<u64>::max(),"64-bit identities round-trip without float precision loss");
        String textAgain;
        require(render::serializeFoliageWorld(restored,textAgain,error)&&text==textAgain,"World serialization is deterministic");
        render::FoliageCellKey cell;
        require(render::foliageCellForPosition({-.001f,0,-32},32,cell,error)&&cell==render::FoliageCellKey{-1,-1},"Negative cell ownership uses floor");
        const auto old=restored.instances;
        require(!render::deserializeFoliageWorld(text+"garbage",restored,error)&&restored.instances.size()==old.size(),"Malformed JSON retains previous output");
        restored.instances[1].id=restored.instances[0].id;
        require(!render::validateFoliageWorld(restored,error),"Duplicate identity rejected");
        restored=*world; restored.instances[0].scale.x=-1;
        require(!render::validateFoliageWorld(restored,error),"Mirrored instance rejected");
        restored=*world; restored.instances[0].position.x=std::numeric_limits<float>::infinity();
        require(!render::validateFoliageWorld(restored,error),"Nonfinite instance rejected");
        render::FoliagePageData page; page.cell={-1,0}; page.instances={world->instances[0]};
        require(render::validateFoliagePage(page,*world,error),error.c_str());
        page.cell={0,0}; require(!render::validateFoliagePage(page,*world,error),"Cross-page instance rejected");
        auto invalid=*world->species[0]; invalid.lods[2].indices[0]=render::foliageMaximumMeshVertices;
        require(!render::validateFoliageSpecies(invalid,error),"Invalid LOD geometry rejected");
        invalid=*world->species[0]; invalid.wind.bendEnd=invalid.wind.bendStart;
        require(!render::validateFoliageSpecies(invalid,error),"Degenerate wind interval rejected");
        std::puts("Foliage data/identity/validation contracts: PASS");
    }
    void publicationContracts()
    {
        auto terrain = make_ptr<render::Terrain>(); auto first = fixture(), second = fixture();
        String error; u32 commits = 0; Transform3<real_Num> placement;
        require( terrain->setFoliageSnapshot( first, error ) &&
            terrain->publishFoliageSnapshot( first, placement, [&] { ++commits; }, error ), error.c_str() );
        require( terrain->setFoliageSnapshot( second, error ), error.c_str() );
        require( !terrain->publishFoliageSnapshot( first, placement, [&] { ++commits; }, error ) && commits == 1 &&
            terrain->getActiveFoliage().snapshot == first, "Superseded foliage cannot replace the active generation." );
        render::Terrain::FoliageDiagnostics diagnostics; diagnostics.actualDraws = 7; terrain->setFoliageDiagnostics( diagnostics );
        terrain->setFoliagePublicationGate( []( render::FoliageSnapshot, const Transform3<real_Num> &,
            const std::function<void()> &, String &failure ) { failure = "fixture consumer busy"; return false; } );
        require( !terrain->setFoliageSnapshot( {}, error ) && terrain->getFoliageSnapshot() == second &&
            terrain->getActiveFoliage().snapshot == first && terrain->getFoliageDiagnostics().actualDraws == 7,
            "Failed clear retains requested/active foliage and the last successful diagnostics." );
        terrain->setFoliagePublicationGate( {} );
        require( terrain->publishFoliageSnapshot( second, placement, [&] { ++commits; }, error ) && commits == 2,
            "Current foliage can retry publication after consumer failure." );
        // Exercise recursive preparation: a gate discovers a newer authored request.
        terrain->setFoliagePublicationGate( [&]( render::FoliageSnapshot, const Transform3<real_Num> &,
            const std::function<void()> &publish, String &failure ) {
                require( terrain->setFoliageSnapshot( first, failure ), failure.c_str() ); publish(); return true;
            } );
        require( !terrain->publishFoliageSnapshot( second, placement, [&] { ++commits; }, error ) && commits == 2 &&
            terrain->getFoliageSnapshot() == first && terrain->getActiveFoliage().snapshot == second,
            "A request superseded inside the gate never commits renderer or active consumers." );
        terrain->setFoliagePublicationGate( {} );
        require( terrain->setFoliageSnapshot( {}, error ) && !terrain->getFoliageSnapshot() &&
            !terrain->getActiveFoliage().snapshot && terrain->getFoliageDiagnostics().actualDraws == 0,
            "Successful clear retires active foliage and diagnostics together." );
        auto invalid = *first->species[0]; invalid.farBillboard = true;
        require( !render::validateFoliageSpecies( invalid, error ), "Billboard mode requires real baked views and texture metadata." );
        std::puts( "Foliage requested/active publication, stale rejection and failure retention: PASS" );
    }

    void streamContracts()
    {
        String error; auto world=fixture();
        auto scheduler=WorldStreamingScheduler::shared(); const auto before=scheduler->stats();
        std::shared_ptr<const render::FoliagePageData> retained;
        {
            render::FoliagePageStream stream;
            require(stream.setSource(world,error),error.c_str());
            require(stream.setDemand({{-1,0},{0,0}},error),error.c_str());
            std::set<u64> ids;
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            while(ids.size()<2 && std::chrono::steady_clock::now()<deadline)
            {
                render::FoliagePageResult result;
                while(stream.take(result))
                { require(result.page!=nullptr,result.error.c_str()); ids.insert(result.page->instances[0].id); retained=result.page; }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            require(ids.size()==2,"Async signed-cell pages become ready");
            require(stream.updateViews({{{-2,0,0},40},{{65,0,0},40}},error),error.c_str());
            const auto unionDemand=stream.demand();
            require(stream.updateViews({{{65,0,0},40},{{-2,0,0},40}},error)&&stream.demand()==unionDemand,"View order preserves canonical residency union");
            const auto generation=stream.stats().generation;
            require(!stream.updateViews({{{0,0,0},std::numeric_limits<float>::infinity()}},error)&&stream.stats().generation==generation,"Invalid demand retains prior generation");
            stream.cancel(); render::FoliagePageResult result;
            require(!stream.take(result),"Cancelled generation cannot publish");
            stream.drain(); require(retained&&!retained->instances.empty(),"Retained immutable page outlives stream drain");
        }
        require(scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)]>before.used[size_t(StreamMemoryClass::ResultCpu)],"Retained page retains result reservation");
        retained.reset();
        require(scheduler->stats().used==before.used,"Source/worker/page reservations balance after destruction");
        std::puts("Foliage streaming readiness/multiview/cancellation/budget contracts: PASS");
    }
    void physicsContracts(SmartPtr<physics::IPhysicsManager> manager)
    {
        auto world=fixture(); String error;
        auto scene=manager->addScene(); require(scene!=nullptr,"Native foliage physics scene");
        void *handle=nullptr; scene->_getObject(&handle); auto native=static_cast<wp_physics_scene *>(handle);
        require(native!=nullptr,"WPPhysics scene exposes native handle");
        const auto before=WorldStreamingScheduler::shared()->stats();
        {
            scene::FoliagePhysics solids(manager,scene,1);
            require(solids.setSource(world,{},error)&&solids.update({{-2,0,0}},error),error.c_str());
            require(solids.getStats().active==1&&solids.getBody(world->instances[0].id)!=nullptr,"Only nearby trunk owns a physics body");
            wp_vec3f hit{},normal{};
            require(wp_physics_scene_intersects(native,{-8,2,0},{8,2,0},&hit,&normal,0,0)!=0,"Real native ray hits foliage trunk");
            require(std::abs(hit.x+2.5f)<.01f,"Native foliage trunk uses authored radius and height");
            require(solids.update({{30,0,0}},error),error.c_str());
            require(!solids.getBody(world->instances[0].id)&&solids.getBody(world->instances[1].id),"Teleport retires old collision independently of renderer residency");
            auto removed=std::make_shared<render::FoliageWorldData>(*world); removed->instances[1].flags|=render::FoliageRemoved;
            require(solids.setSource(removed,{},error)&&solids.update({{30,0,0}},error)&&solids.getStats().active==0,"Persistent tombstones suppress nearby collision");
            Transform3<real_Num> placement; placement.setPosition({10,3,-5}); placement.setScale({2,3,4});
            require(solids.setSource(world,placement,error)&&solids.update({{6,3,-5}},error),error.c_str());
            require(wp_physics_scene_intersects(native,{0,5,-5},{12,5,-5},&hit,&normal,0,0)!=0&&std::abs(hit.x-5)<.01f,"Terrain placement scales and translates native foliage solids");
            solids.clear(); require(solids.getStats().active==0,"Clear retires all foliage bodies");
        }
        {
            // A native scene wrapper injects capacity rejection after one candidate
            // actor has registered, exercising rollback beyond the first allocation.
            class LimitedScene final : public physics::PhysicsScene3
            {
            public:
                SmartPtr<physics::IPhysicsScene3> backend;
                int accepts=-1;
                void addActor(SmartPtr<physics::IPhysicsBody3> body) override
                { if(accepts==0)return;if(accepts>0)--accepts;backend->addActor(body); }
                void removeActor(SmartPtr<physics::IPhysicsBody3> body) override {backend->removeActor(body);}
                bool hasActor(SmartPtr<physics::IPhysicsBody3> body) const override {return backend->hasActor(body);}
                void lock() override {backend->lock();}
                bool try_lock() override {return backend->try_lock();}
                void unlock() override {backend->unlock();}
            };
            auto limited=make_ptr<LimitedScene>();limited->backend=scene;
            scene::FoliagePhysics solids(manager,limited,2);
            auto original=std::make_shared<render::FoliageWorldData>(*world);original->instances[1].position.x=1;
            require(solids.setSource(original,{},error)&&solids.update({{0,0,0}},error),error.c_str());
            const auto oldFirst=solids.getBody(original->instances[0].id),oldSecond=solids.getBody(original->instances[1].id);
            require(oldFirst&&oldSecond&&wp_physics_scene_get_actor_count(native)==2,"Two native actors form initial collision generation");
            auto changed=std::make_shared<render::FoliageWorldData>(*original);changed->instances[0].position.x=-3;changed->instances[1].position.x=2;
            require(solids.setSource(changed,{},error)&&solids.getBody(original->instances[0].id)==oldFirst,"Source admission retains old bodies until update commits");
            auto scheduler=WorldStreamingScheduler::shared();const auto budget=scheduler->stats();const auto index=size_t(StreamMemoryClass::Collision);
            require(scheduler->setBudget(StreamMemoryClass::Collision,budget.used[index],error),error.c_str());
            struct Restore{std::shared_ptr<WorldStreamingScheduler> scheduler;u64 bytes;~Restore(){String error;scheduler->setBudget(StreamMemoryClass::Collision,bytes,error);}}restore{scheduler,budget.limits[index]};
            require(!solids.update({{0,0,0}},error)&&solids.getBody(original->instances[0].id)==oldFirst&&wp_physics_scene_get_actor_count(native)==2,"Candidate budget failure retains exact old native generation");
            require(scheduler->setBudget(StreamMemoryClass::Collision,restore.bytes,error),error.c_str());
            limited->accepts=1;
            require(!solids.update({{0,0,0}},error)&&solids.getBody(original->instances[0].id)==oldFirst&&solids.getBody(original->instances[1].id)==oldSecond&&wp_physics_scene_get_actor_count(native)==2,"Partial native capacity rejection rolls back every new actor and retains both old bodies");
            wp_vec3f hit{},normal{};require(wp_physics_scene_intersects(native,{-8,2,0},{8,2,0},&hit,&normal,0,0)&&std::abs(hit.x+2.5f)<.01f,"Failed replacement preserves actual old ray geometry");
            limited->accepts=-1;require(solids.update({{0,0,0}},error),error.c_str());
            require(solids.getBody(original->instances[0].id)!=oldFirst&&wp_physics_scene_get_actor_count(native)==2,"Retry atomically replaces the accepted generation");
            require(wp_physics_scene_intersects(native,{-8,2,0},{8,2,0},&hit,&normal,0,0)&&std::abs(hit.x+3.5f)<.01f,"Successful retry publishes actual new ray geometry");
            solids.clear();
        }
        require(WorldStreamingScheduler::shared()->stats().used==before.used,"Collision reservations balance after unload");
        manager->removeScene(scene);
        auto authoring=make_ptr<scene::FoliageAuthoring>();
        require(authoring->setSpecies(world->species,error),error.c_str());
        auto ground=std::make_shared<render::TerrainData>(); ground->dimensions={2,2}; ground->spacing={16,16}; ground->heights={0,0,0,0};
        require(authoring->paint(ground,0,{8,8},1,1,7,error),error.c_str());
        const auto id=authoring->getSnapshot()->instances.front().id;
        u32 acquired=0,released=0;
        {
            scene::FoliageGameplay gameplay(authoring,[&](const render::FoliageInstance &i,String &){ ++acquired; return SmartPtr<ISharedObject>(make_ptr<Properties>()); },[&](SmartPtr<ISharedObject>){ ++released; });
            require(gameplay.promote(id,error)&&gameplay.getActor(id),error.c_str());
            require(acquired==1&&(authoring->getSnapshot()->instances[0].flags&render::FoliagePromoted),"Promotion hands one stable instance to gameplay");
            require(gameplay.promote(id,error)&&acquired==1,"Repeated promotion is idempotent");
            require(gameplay.demote(id,error)&&released==1&&!(authoring->getSnapshot()->instances[0].flags&render::FoliagePromoted),"Demotion returns the instance to compact rendering");
            require(gameplay.promote(id,error)&&gameplay.remove(id,error)&&released==2,"Destruction releases the actor and saves a removal delta");
            require((authoring->getSnapshot()->instances[0].flags&render::FoliageRemoved)!=0,"Removal tombstone remains saved");
        }
        std::puts("Foliage native collision/proximity/placement/promotion contracts: PASS");
    }

    void pagedPhysicsContracts( SmartPtr<physics::IPhysicsManager> manager )
    {
        auto scene = manager->addScene();
        require( scene != nullptr, "Paged foliage native physics scene" );
        void *handle = nullptr;
        scene->_getObject( &handle );
        auto native = static_cast<wp_physics_scene *>( handle );
        require( native != nullptr, "Paged foliage native scene handle" );
        const auto before = WorldStreamingScheduler::shared()->stats().used;
        const auto world = fixture();
        String error;
        const auto makeSource = [&]( const std::shared_ptr<const render::FoliageWorldData> &population,
                                     const std::shared_ptr<std::atomic<bool>> &allowFar,
                                     const std::shared_ptr<std::atomic<u32>> &farReads ) {
            auto metadata = std::make_shared<render::FoliageWorldData>();
            metadata->revision = population->revision;
            metadata->species = population->species;
            metadata->pageSize = population->pageSize;
            auto source = std::make_shared<render::FoliageStreamSource>();
            source->snapshot = metadata;
            require( render::foliageInstanceRadii( *metadata, population->instances,
                                                   source->maximumGeometryRadius,
                                                   source->maximumBendRadius, error ),
                     error.c_str() );
            source->cells = { { -1, 0 }, { 0, 0 }, { 1, 0 }, { 2, 0 } };
            source->pageInstanceCounts = { 1, 1, 1, 1 };
            source->load = [population, allowFar, farReads]( render::FoliageCellKey cell,
                                                            const StreamCancellation &cancel,
                                                            String &failure )
                -> std::shared_ptr<const render::FoliagePageData> {
                if( cell.x == 2 )
                {
                    ++*farReads;
                    while( !*allowFar && !cancel.cancelled() )
                    {
                        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                    }
                    if( cancel.cancelled() )
                    {
                        return {};
                    }
                }
                auto page = std::make_shared<render::FoliagePageData>();
                page->cell = cell;
                page->revision = population->revision;
                page->instances = { population->instances[size_t( cell.x + 1 )] };
                failure.clear();
                return page;
            };
            return source;
        };
        {
            auto allowFar = std::make_shared<std::atomic<bool>>( false );
            auto farReads = std::make_shared<std::atomic<u32>>( 0 );
            const auto source = makeSource( world, allowFar, farReads );
            scene::FoliagePhysics solids( manager, scene, 2 );
            const auto finish = [&]( const Vector3F &observer ) {
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
                while( !solids.update( { observer }, error ) )
                {
                    require( error.empty(), error.c_str() );
                    require( std::chrono::steady_clock::now() < deadline, "Paged collision timed out" );
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
            };
            require( solids.setPagedSource( source, {}, error ), error.c_str() );
            finish( { -2, 0, 0 } );
            const auto first = world->instances.front().id;
            const auto last = world->instances.back().id;
            require( solids.getBody( first ) && *farReads == 0, "Remote cooked page stays cold" );
            require( !solids.update( { { 94, 0, 0 } }, error ) && error.empty() &&
                         solids.getBody( first ) && !solids.getBody( last ),
                     "Pending teleport preserves the previous complete native collision set" );
            *allowFar = true;
            finish( { 94, 0, 0 } );
            require( !solids.getBody( first ) && solids.getBody( last ) && *farReads == 1,
                     "Ready teleport pages replace old collision and reuse the loaded page" );
            wp_vec3f hit{}, normal{};
            require( wp_physics_scene_intersects( native, { 90, 2, 0 }, { 98, 2, 0 }, &hit,
                                                  &normal, 0, 0 ) &&
                         std::abs( hit.x - 93.5f ) < .01f,
                     "Lazy page placement reaches native ray queries" );

            auto edited = std::make_shared<render::FoliageWorldData>( *world );
            ++edited->revision;
            edited->instances.back().flags = render::FoliagePromoted;
            const auto replacement = makeSource( edited, allowFar, farReads );
            require( solids.setPagedSource( replacement, {}, error ) &&
                         solids.updateActive( { { 94, 0, 0 } }, error ) && solids.getBody( last ),
                     "Physics task updates cannot consume an unpublished replacement" );
            finish( { 94, 0, 0 } );
            require( !solids.getBody( last ) && wp_physics_scene_get_actor_count( native ) == 0,
                     "Cooked promotion suppresses the compact native trunk" );
        }
        require( WorldStreamingScheduler::shared()->stats().used == before,
                 "Paged collision leases retire with the owner" );
        manager->removeScene( scene );
        std::puts( "Foliage paged collision demand, teleport, promotion and lease contracts: PASS" );
    }
    void lifecycleContracts()
    {
        const auto scheduler = WorldStreamingScheduler::shared();
        const auto before = scheduler->stats();
        String error;
        auto ground = std::make_shared<render::TerrainData>();
        ground->dimensions = { 17, 9 };
        ground->origin = { -8, 0 };
        ground->heights.assign( 17 * 9, 0 );
        const Array<render::FoliageCellKey> cells{ { -1, 0 }, { 0, 0 } };
        {
            auto author = make_ptr<scene::FoliageAuthoring>();
            require( author->setSpecies( foliage_page_stream_contracts::source()->species, error ), error.c_str() );
            scene::FoliagePlacementRecipe recipe;
            recipe.pageSize = 8;
            recipe.candidateSpacing = 2;
            scene::FoliagePlacementRule rule;
            rule.density = 1;
            rule.minimumSpacing = 0;
            recipe.rules = { rule };
            render::FoliagePageStream stream;
            for( u32 cycle = 0; cycle < 1000; ++cycle )
            {
                recipe.seed = cycle + 1;
                const auto previous = author->getSnapshot();
                require( author->configure( recipe, error ) && author->regenerate( ground, cells, error ),
                         error.c_str() );
                const auto source = author->getSnapshot();
                require( source != previous && !source->instances.empty(), "Regeneration publishes a new population" );
                const auto cell = cells[cycle % cells.size()];
                require( stream.setSource( source, error ) && stream.setDemand( { cell }, error ), error.c_str() );
                const auto result = foliage_page_stream_contracts::take( stream );
                require( result.page && result.source == source && result.cell == cell && !result.page->instances.empty(),
                         "Regenerate/load publishes only the current foliage generation" );
                const auto count = result.page->instances.size();
                stream.drain();
                require( result.page->instances.size() == count, "Retained result survives each unload" );
            }
        }
        require( scheduler->stats().used == before.used, "1000 regeneration/page/load/unload cycles release every reservation" );
        std::puts( "Foliage 1000 regeneration/page/load/unload cycles: PASS" );
    }
}
int main()
{
    TypeManager types; types.load(); TypeManager::setInstance(&types);
    auto app=make_ptr<core::ApplicationManager>(); core::IApplicationManager::setInstance(app);
    auto factories=make_ptr<FactoryManager>(); factories->load(nullptr); app->setFactoryManager(factories);
    app->setStateManager(make_ptr<StateManager>());
    auto plugin=make_ptr<physics::WPPhysics>(); plugin->load(nullptr);
    auto manager=factories->make_object<physics::IPhysicsManager>(physics::PhysicsManager::typeInfo());
    app->setPhysicsManager(manager); int result=0;
    try
    {
        require( manager != nullptr, "Native physics plugin factory" );
        dataContracts();
        publicationContracts();
        runFoliageAuthoringContracts();
        runFoliageImpostorContracts();
        foliage_page_stream_contracts::run();
        streamContracts();
        physicsContracts( manager );
        pagedPhysicsContracts( manager );
        lifecycleContracts();
    }
    catch(const std::exception &e) { std::fprintf(stderr,"Foliage contracts failed: %s\n",e.what()); result=1; }
    if(manager)manager->unload(nullptr); app->setPhysicsManager(nullptr); manager=nullptr;
    plugin->unload(nullptr); plugin=nullptr; app->setStateManager(nullptr); app->setFactoryManager(nullptr);
    factories->unload(nullptr); factories=nullptr; core::IApplicationManager::setInstance(nullptr); app=nullptr;
    TypeManager::setInstance(nullptr); types.unload(); return result;
}
