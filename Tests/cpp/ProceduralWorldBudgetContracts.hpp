#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/CProceduralWorld.hpp>
#include <WPProcedural/CProceduralScene.hpp>
#include <WPProcedural/CWorldGenerator.hpp>

namespace procedural_world_budget_contracts
{
    using namespace city_inventory_budget_contracts;
    class WorldInventory : public CProceduralWorld
    {
    public:
        void reserveScenes( size_t count ) { m_scenes.reserve( count ); }
        size_t capacity() const { return m_scenes.capacity(); }
        void importForeign( const Array<SmartPtr<IProceduralScene>> &input ) { m_scenes = input; }
    };
    class GeneratorInventory : public CWorldGenerator
    {
    public:
        size_t capacity() const { return m_scenes.capacity(); }
    };
    class RejectingWorld : public CProceduralWorld
    {
    public:
        bool reject = false;
        void setScenes( const Array<SmartPtr<IProceduralScene>> &scenes ) override
        {
            if( reject )
            {
                Budget cap;
                cap.set( cap.used() );
                CProceduralWorld::setScenes( scenes );
                return;
            }
            CProceduralWorld::setScenes( scenes );
        }
    };
    class CancellingScene : public CProceduralScene
    {
    public:
        CWorldGenerator *builder = nullptr;
        int additions = 0;
        void addTerrain( SmartPtr<IProceduralTerrain> terrain ) override
        {
            CProceduralScene::addTerrain( terrain );
            ++additions;
            if( builder ) builder->clear();
        }
    };
    class ObservingScene : public CProceduralScene
    {
    public:
        CProceduralWorld *world = nullptr;
        CWorldGenerator *builder = nullptr;
        bool *detached = nullptr;
        bool *failed = nullptr;
        int *destructions = nullptr;
        int *unloads = nullptr;
        ~ObservingScene() override
        {
            if( destructions ) ++*destructions;
            if( world || builder )
            {
                try
                {
                    const bool worldEmpty = !world || world->getScenes().empty();
                    const bool builderEmpty = !builder || ( builder->getScenes().empty() &&
                        !builder->getProceduralWorld() && !builder->isFinished() );
                    if( detached ) *detached = worldEmpty && builderEmpty;
                }
                catch( ... ) { if( failed ) *failed = true; }
            }
        }
        void unload( SmartPtr<ISharedObject> data ) override
        {
            if( unloads ) ++*unloads;
            CProceduralScene::unload( data );
        }
    };
    inline void run()
    {
        Budget budget;
        {
            WorldInventory world;
            auto scene = make_ptr<CProceduralScene>();
            auto empty = world.getScenes();
            check( empty.empty() && empty.get_allocator().isSourceBudgeted(),
                   "Empty world snapshots retain SourceCpu admission policy" );
            const auto baseline = budget.used();
            const auto bytes = sourceBudgetStorageBytes( 257 * sizeof( SmartPtr<IProceduralScene> ),
                                                         alignof( SmartPtr<IProceduralScene> ) );
            budget.set( baseline );
            denied( [&]{ world.addScene( scene ); } );
            denied( [&]{ empty.push_back( scene ); } );
            budget.set( baseline + bytes - 1 );
            denied( [&]{ world.reserveScenes( 257 ); } );
            check( world.capacity() == 0 && world.getScenes().empty() && budget.used() == baseline,
                   "World scene inventory rejects one-byte-short spare capacity without publication" );
            budget.set( baseline + bytes );
            world.reserveScenes( 257 ); world.addScene( scene );
            check( budget.used() == baseline + bytes && world.capacity() == 257,
                   "World scene inventory charges exact physical spare capacity" );
            denied( [&]{ world.getScenes(); } );
            budget.set( budget.original );
            auto snapshot = world.getScenes();
            check( snapshot.size() == 1 && snapshot[0] == scene && snapshot.capacity() == 257 &&
                   snapshot.get_allocator().isSourceBudgeted(),
                   "World caller snapshots retain scene owners and admitted spare capacity" );
            const auto live = budget.used();
            budget.set( live ); world.removeScene( scene );
            check( world.capacity() == 257 && budget.used() == live,
                   "Removing a scene retains charged inventory capacity until physical retirement" );
            world.unload( nullptr );
            check( world.capacity() == 0 && world.getScenes().empty() && snapshot[0] == scene &&
                   budget.used() == live - bytes,
                   "Full-cap world unload retires inventory storage while caller scene snapshots survive" );
            { auto retired = std::move( snapshot ); }
            check( budget.used() == live - 2 * bytes, "Final world snapshot physically retires its spare slots" );
            budget.set( budget.original );
        }
        {
            WorldInventory world;
            auto scene = make_ptr<CProceduralScene>();
            Array<SmartPtr<IProceduralScene>> foreign;
            foreign.reserve( 43 ); foreign.push_back( scene );
            world.importForeign( foreign );
            const auto baseline = budget.used();
            const auto bytes = sourceBudgetStorageBytes( 43 * sizeof( SmartPtr<IProceduralScene> ),
                                                         alignof( SmartPtr<IProceduralScene> ) );
            budget.set( baseline + bytes - 1 );
            denied( [&]{ world.getScenes(); } );
            budget.set( baseline + bytes );
            {
                auto snapshot = world.getScenes();
                check( snapshot.size() == 1 && snapshot.capacity() == 43 &&
                       snapshot.get_allocator().isSourceBudgeted() && budget.used() == baseline + bytes,
                       "World snapshots restore admission after explicit foreign allocator substitution" );
            }
            budget.set( budget.original );
        }
        {
            CProceduralWorld world;
            bool detached = false, failed = false;
            int destructions = 0, unloads = 0;
            SmartPtr<ObservingScene> scene( new ObservingScene );
            // Raw construction starts with one creator reference; SmartPtr adds
            // another. Drop the creator's reference so the inventory can be last owner.
            scene->removeReference();
            scene->world = &world; scene->detached = &detached; scene->failed = &failed;
            scene->destructions = &destructions; scene->unloads = &unloads;
            world.addScene( scene ); scene = nullptr;
            budget.set( budget.used() );
            world.unload( nullptr );
            check( destructions == 1 && unloads == 0 && detached && !failed,
                   "Last-owner scene destruction reenters a detached world without admission or child unload" );
            world.unload( nullptr );
            budget.set( budget.original );
        }
        {
            auto generator = make_ptr<GeneratorInventory>();
            auto world = make_ptr<RejectingWorld>();
            auto previous = make_ptr<CProceduralScene>();
            auto next = make_ptr<CProceduralScene>();
            Array<SmartPtr<IProceduralScene>> initial;
            initial.push_back( previous ); generator->setScenes( initial );
            world->addScene( previous ); generator->setProceduralWorld( world );
            Array<SmartPtr<IProceduralScene>> replacement;
            replacement.reserve( 67 ); replacement.push_back( next );
            world->reject = true;
            denied( [&]{ generator->setScenes( replacement ); } );
            denied( [&]{ generator->addScene( next ); } );
            check( generator->getScenes().size() == 1 && generator->getScenes()[0] == previous &&
                   world->getScenes().size() == 1 && world->getScenes()[0] == previous,
                   "Final world inventory denial preserves both world and builder scene lists" );
            world->reject = false; generator->setScenes( replacement );
            auto snapshot = generator->getScenes();
            check( snapshot.capacity() == 67 && snapshot[0] == next &&
                   snapshot.get_allocator().isSourceBudgeted() && world->getScenes().size() == 2,
                   "World builder replacement retries while preserving legacy append-to-world order" );
            budget.set( budget.used() ); generator->unload( nullptr );
            check( generator->capacity() == 0 && generator->getScenes().empty() &&
                   !generator->getProceduralWorld() && snapshot[0] == next,
                   "Full-cap builder unload detaches its inventory and world while snapshots survive" );
            budget.set( budget.original );
            check( world->getScenes().size() == 2, "Builder unload does not unload externally retained world scenes" );
        }
        {
            auto generator = make_ptr<GeneratorInventory>();
            auto options = generator->getOptions();
            options.worldWidth = options.worldHeight = options.chunkSize = 2;
            options.generateTerrain = false; options.randomizeSeed = false; options.seed = 19;
            generator->setOptions( options );
            budget.set( budget.used() );
            denied( []{ auto options = std::make_unique<SWorldGeneratorOptions>(); } );
            denied( [&]{ generator->generate(); } );
            check( !generator->getProceduralWorld() && generator->getScenes().empty() &&
                   !generator->isFinished() && generator->getSeed() == 19,
                   "Denied default world construction publishes no world, scene or completed status" );
            budget.set( budget.original ); generator->generate();
            auto world = generator->getProceduralWorld();
            auto snapshot = generator->getScenes();
            check( world && snapshot.size() == 1 && world->getScenes().size() == 1 && generator->isFinished(),
                   "Default world generation retries and publishes complete admitted inventories" );
            budget.set( budget.used() );
            denied( [&]{ generator->generate(); } );
            check( generator->getProceduralWorld() == world && generator->isFinished(),
                   "Denied world regeneration retains the completed world and status" );
            generator->clear();
            check( generator->capacity() == 0 && !generator->getProceduralWorld() && snapshot.size() == 1,
                   "Full-cap clear retires builder capacity without invalidating retained scenes" );
            budget.set( budget.original );
            check( world->getScenes().size() == 1, "Retained default world remains intact after builder clear" );
            generator->setOptions( options );
            auto rejecting = make_ptr<RejectingWorld>();
            generator->setProceduralWorld( rejecting ); rejecting->reject = true;
            denied( [&]{ generator->generate(); } );
            check( generator->getProceduralWorld() == rejecting && generator->getScenes().empty() &&
                   rejecting->getScenes().empty() && !generator->isFinished(),
                   "Final default-scene publication denial retains the prior empty world and builder" );
            rejecting->reject = false; generator->generate();
            check( generator->isFinished() && generator->getScenes().size() == 1 && rejecting->getScenes().size() == 1,
                   "Default-scene final publication retries exactly once" );
        }
        {
            GeneratorInventory generator;
            bool detached = false, failed = false;
            int destructions = 0, unloads = 0;
            SmartPtr<ObservingScene> scene( new ObservingScene );
            // Raw construction starts with one creator reference; SmartPtr adds
            // another. Drop the creator's reference so the inventory can be last owner.
            scene->removeReference();
            scene->builder = &generator; scene->detached = &detached; scene->failed = &failed;
            scene->destructions = &destructions; scene->unloads = &unloads;
            generator.addScene( scene ); scene = nullptr;
            budget.set( budget.used() ); generator.unload( nullptr );
            check( destructions == 1 && unloads == 0 && detached && !failed,
                   "Last-owner scene destruction observes fully detached builder state at the cap" );
            budget.set( budget.original );
        }
        {
            bool detached = false, failed = false;
            int destructions = 0, unloads = 0;
            auto generator = new CWorldGenerator;
            SmartPtr<ObservingScene> scene( new ObservingScene );
            // Raw construction starts with one creator reference; SmartPtr adds
            // another. Drop the creator's reference so the inventory can be last owner.
            scene->removeReference();
            scene->builder = generator; scene->detached = &detached; scene->failed = &failed;
            scene->destructions = &destructions; scene->unloads = &unloads;
            generator->addScene( scene ); scene = nullptr;
            budget.set( budget.used() ); delete generator;
            check( destructions == 1 && unloads == 0 && detached && !failed,
                   "Builder body destruction detaches scene ownership before reentrant final-owner destruction" );
            budget.set( budget.original );
        }
        {
            auto generator = make_ptr<CWorldGenerator>();
            auto scene = make_ptr<CancellingScene>(); scene->builder = generator.get();
            auto world = make_ptr<CProceduralWorld>();
            Array<SmartPtr<IProceduralScene>> scenes;
            scenes.push_back( scene ); generator->setScenes( scenes );
            generator->setProceduralWorld( world );
            auto options = generator->getOptions();
            options.worldWidth = options.worldHeight = 4; options.chunkSize = 2;
            options.worldDepth = 2; options.octaves = 1; options.randomizeSeed = false;
            generator->setOptions( options ); generator->generate();
            check( scene->additions == 1 && scene->getTerrains().size() == 1 &&
                   !generator->getProceduralWorld() && generator->getScenes().empty() && !generator->isFinished(),
                   "Reentrant clear stops remaining world chunks and cannot republish cancelled defaults" );
            // The callback's already-committed external terrain remains its scene owner's responsibility.
            scene->unload( nullptr );
        }
    }
}
