#ifndef WPTerrainEditingContracts_h__
#define WPTerrainEditingContracts_h__

#include <Workphone/Scene/TerrainEditing.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/CommandManager.hpp>
#include <Workphone/System/CommandManagerMT.hpp>
#include <Workphone/System/JobQueue.hpp>
#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <future>
#include <thread>
#include <chrono>

namespace terrain_editing_contracts
{
    using namespace workphone;
    using namespace workphone::scene;
    namespace fs = std::filesystem;

    inline void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }
    inline void require( bool value, const String &message )
    {
        if( !value )
            throw std::runtime_error( message.c_str() );
    }

    class CounterCommand : public ICommand
    {
    public:
        State state = State::Allocated;
        int executions = 0;
        bool primary = true;
        int unloads = 0;
        std::function<void()> action;
        void execute() override
        {
            if( action )
                action();
            ++executions;
            state = State::Finished;
        }
        void undo() override
        {
        }
        void redo() override
        {
        }
        void unload( SmartPtr<ISharedObject> ) override
        {
            ++unloads;
        }
        State getState() const override
        {
            return state;
        }
        void setState( State value ) override
        {
            state = value;
        }
        bool isPrimary() const override
        {
            return primary;
        }
        void setPrimary( bool value ) override
        {
            primary = value;
        }
    };

    class HeldJobQueue : public JobQueue
    {
    public:
        Array<SmartPtr<IJob>> held;
        void addJob( SmartPtr<IJob> job ) override
        {
            held.push_back( job );
        }
        void drain()
        {
            auto jobs = std::move( held );
            held.clear();
            for( auto job : jobs )
                job->execute();
        }
    };

    inline void queuedHistory( SmartPtr<core::ApplicationManager> application )
    {
        auto queue = SmartPtr<HeldJobQueue>( new HeldJobQueue );
        application->setJobQueue( queue );
        auto manager = make_ptr<CommandManagerMT>();
        const auto makeCommand = [] {
            auto result = SmartPtr<CounterCommand>( new CounterCommand );
            result->primary = false;
            return result;
        };
        auto command = makeCommand();
        manager->addCommand( command );
        require( queue->held.size() == 1 && command->executions == 0 &&
                     command->getState() == ICommand::State::Queued &&
                     manager->isCommandQueued( command ) && !manager->getPreviousCommand() &&
                     !manager->getNextCommand(),
                 "Pending asynchronous work must block undo/redo without moving history" );
        queue->drain();
        require( command->executions == 1 && command->getState() == ICommand::State::Finished &&
                     manager->getPreviousCommand() == command && manager->getNextCommand() == command,
                 "Completed queued work must enter exact undo/redo history" );
        auto removed = makeCommand();
        manager->addCommand( removed );
        manager->removeCommand( removed );
        queue->drain();
        require( removed->executions == 0, "Removing queued work must invalidate its job" );
        auto reused = makeCommand();
        manager->addCommand( reused );
        auto stale = queue->held.front();
        queue->held.clear();
        manager->clearAll();
        manager->addCommand( reused );
        stale->execute();
        require( reused->executions == 0 && !manager->getPreviousCommand(),
                 "Cleared job identity must remain invalid after the same command is re-added" );
        queue->drain();
        require( reused->executions == 1, "Replacement job must execute exactly once" );
        manager->clearAll();
        Array<SmartPtr<CounterCommand>> bounded;
        for( unsigned i = 0; i < 105; ++i )
        {
            auto item = makeCommand();
            bounded.push_back( item );
            manager->addCommand( item );
        }
        queue->drain();
        for( auto item : bounded )
            require( item->executions == 1, "History eviction must not cancel queued work" );
        require( !manager->hasCommand( bounded[4] ) && manager->hasCommand( bounded[5] ),
                 "All queued work must execute while undo history remains bounded" );
        auto unloaded = makeCommand();
        manager->addCommand( unloaded );
        manager->unload( nullptr );
        queue->drain();
        require( unloaded->executions == 0 && !manager->getPreviousCommand(),
                 "Unloading history must cancel outstanding queued work" );
        std::promise<void> started, release;
        auto startedSignal = started.get_future();
        auto releaseSignal = release.get_future();
        auto active = makeCommand();
        active->action = [&] {
            started.set_value();
            releaseSignal.wait();
        };
        manager->addCommand( active );
        std::thread worker( [&] { queue->drain(); } );
        startedSignal.wait();
        auto probe = std::async( std::launch::async, [manager, active]() mutable {
            const bool blockedUndo = !manager->getPreviousCommand();
            manager->unload( nullptr );
            return blockedUndo && active->unloads == 0;
        } );
        const bool responsive = probe.wait_for( std::chrono::seconds( 2 ) ) == std::future_status::ready;
        release.set_value();
        worker.join();
        require( responsive && probe.get() && active->executions == 1 && active->unloads == 1,
                 "Running commands must release the history lock and defer unload until completion" );
        auto orphaned = makeCommand();
        manager->addCommand( orphaned );
        manager = nullptr;
        queue->drain();
        require( orphaned->executions == 0, "Retired manager must invalidate its remaining jobs" );
        application->setJobQueue( nullptr );
    }

    inline void history( SmartPtr<ICommandManager> manager )
    {
        require( !manager->getPreviousCommand() && !manager->getNextCommand(),
                 "Empty history must be safe" );
        auto first = SmartPtr<CounterCommand>( new CounterCommand );
        auto second = SmartPtr<CounterCommand>( new CounterCommand );
        manager->addCommand( first );
        manager->addCommand( second );
        require(
            first->executions == 1 && second->executions == 1 && !manager->isCommandQueued( second ),
            "Commands must execute once and enter applied history" );
        require( manager->getPreviousCommand() == second && manager->isCommandQueued( second ) &&
                     manager->getPreviousCommand() == first && !manager->getPreviousCommand(),
                 "Undo must visit each command once and stop at the beginning" );
        require( manager->getNextCommand() == first && !manager->isCommandQueued( first ),
                 "Redo must start at the oldest undone command" );
        auto branch = SmartPtr<CounterCommand>( new CounterCommand );
        manager->addCommand( branch );
        require( !manager->hasCommand( second ) && !manager->getNextCommand() &&
                     manager->getPreviousCommand() == branch && manager->getNextCommand() == branch &&
                     !manager->getNextCommand(),
                 "New history branch must discard only redo work" );
        manager->removeCommand( first );
        require( manager->getPreviousCommand() == branch && !manager->getPreviousCommand(),
                 "Removing an applied command must preserve the cursor" );
        manager->clearAll();
        require( !manager->getNextCommand() && !manager->getPreviousCommand(),
                 "Clear must reset history" );
        auto finished = SmartPtr<CounterCommand>( new CounterCommand );
        finished->state = ICommand::State::Finished;
        manager->addCommand( finished );
        require( finished->executions == 0 && manager->getPreviousCommand() == finished,
                 "Completed owner-thread command must register without a second application" );
        manager->clearAll();
        Array<SmartPtr<CounterCommand>> commands;
        for( unsigned i = 0; i < 105; ++i )
        {
            auto command = SmartPtr<CounterCommand>( new CounterCommand );
            commands.push_back( command );
            manager->addCommand( command );
        }
        require( !manager->hasCommand( commands[4] ) && manager->hasCommand( commands[5] ),
                 "History must evict the oldest commands at its 100-entry bound" );
        for( unsigned i = 0; i < 100; ++i )
            require( manager->getPreviousCommand() == commands[104 - i],
                     "Bounded undo order must remain exact" );
        require( !manager->getPreviousCommand(), "Evicted commands must not remain undoable" );
        for( unsigned i = 0; i < 100; ++i )
            require( manager->getNextCommand() == commands[5 + i],
                     "Bounded redo order must remain exact" );
        require( !manager->getNextCommand(), "Redo exhaustion must be safe" );
        manager->clearAll();
    }

    struct Fixture
    {
        SmartPtr<core::IApplicationManager> previous = core::IApplicationManager::instance();
        SmartPtr<core::ApplicationManager> application = make_ptr<core::ApplicationManager>();
        fs::path temporary = fs::temp_directory_path();
        fs::path folder =
            temporary / ( std::string( "workphone_terrain_edits_" ) + StringUtil::getUUID().c_str() );
        Fixture()
        {
            core::IApplicationManager::setInstance( application );
            fs::create_directories( folder );
        }
        ~Fixture()
        {
            core::IApplicationManager::setInstance( previous );
            if( folder.parent_path() == temporary &&
                folder.filename().string().find( "workphone_terrain_edits_" ) == 0 )
            {
                std::error_code ignored;
                fs::remove_all( folder, ignored );
            }
        }
    };

    inline void run()
    {
        Fixture fixture;
        history( make_ptr<CommandManager>() );
        history( make_ptr<CommandManagerMT>() );
        queuedHistory( fixture.application );
        auto manager = make_ptr<CommandManager>();
        auto terrain = make_ptr<TerrainSystem>();
        render::TerrainData original;
        original.dimensions = Vector2I( 3, 5 );
        original.origin = Vector2F( -1, -2 );
        original.heightScale = 2;
        original.heights.assign( 15, 0 );
        String error;
        require( terrain->applyTerrainData( original, error ), error );
        TerrainBrush brush;
        brush.radius = .6f;
        brush.strength = 4;
        require( applyTerrainBrush( terrain, brush, manager, error ), error );
        require( terrain->getTerrainSnapshot()->heights[7] == 2 &&
                     terrain->getTerrainSnapshot()->heights[6] == 0,
                 "Raise must apply local height units and preserve samples outside its footprint" );
        brush.mode = TerrainBrushMode::Lower;
        brush.strength = 2;
        require( applyTerrainBrush( terrain, brush, manager, error ), error );
        require( terrain->getTerrainSnapshot()->heights[7] == 1,
                 "Lower must apply the opposite height delta" );
        manager->getPreviousCommand()->undo();
        manager->getPreviousCommand()->undo();
        require( terrain->getTerrainSnapshot()->heights == original.heights,
                 "Multiple undo must restore exact samples" );
        manager->getNextCommand()->redo();
        manager->getNextCommand()->redo();
        require( terrain->getTerrainSnapshot()->heights[7] == 1,
                 "Multiple redo must replay exact patches" );
        const auto saved = terrain->getTerrainSnapshot();
        const auto path = ( fixture.folder / "edited.terrain.json" ).u8string();
        require( saveTerrainDataFile( terrain, path.c_str(), error ), error );
        auto restored = make_ptr<TerrainSystem>();
        require( loadTerrainDataFile( restored, path.c_str(), error ), error );
        require( restored->getTerrainSnapshot()->heights == saved->heights &&
                     restored->getTerrainSnapshot()->dimensions == saved->dimensions &&
                     restored->getTerrainSnapshot()->heightScale == 2,
                 "Saved samples must survive a fresh component" );
        require( saveTerrainDataFile( terrain, path.c_str(), error ),
                 "Existing terrain files must replace atomically" );
        {
            std::ofstream invalid( fixture.folder / "bad.terrain.json" );
            invalid << "{broken";
        }
        require( !loadTerrainDataFile(
                     restored, ( fixture.folder / "bad.terrain.json" ).u8string().c_str(), error ) &&
                     restored->getTerrainSnapshot()->heights == saved->heights,
                 "Malformed file load must retain previous samples" );
        require( !saveTerrainDataFile(
                     terrain, ( fixture.folder / "missing" / "out.json" ).u8string().c_str(), error ),
                 "Unwritable output must report failure" );
        auto changed = *terrain->getTerrainSnapshot();
        changed.heights[7] = 8;
        require( terrain->applyTerrainData( changed, error ), error );
        manager->getPreviousCommand()->undo();
        require( terrain->getTerrainSnapshot()->heights[7] == 8,
                 "Conflicting undo must retain externally changed samples" );
        manager->clearAll();
        for( size_t i = 0; i < original.heights.size(); ++i )
            original.heights[i] = static_cast<f32>( i * i );
        require( terrain->applyTerrainData( original, error ), error );
        brush.mode = TerrainBrushMode::Smooth;
        brush.strength = 1;
        require( applyTerrainBrush( terrain, brush, manager, error ), error );
        require( std::abs( terrain->getTerrainSnapshot()->heights[7] - 501.f / 9 ) < .0001f,
                 "Smooth must read one immutable neighbourhood, without traversal feedback" );
        brush.mode = TerrainBrushMode::Flatten;
        brush.targetHeight = 8;
        require( applyTerrainBrush( terrain, brush, manager, error ), error );
        require( terrain->getTerrainSnapshot()->heights[7] == 4,
                 "Flatten target is in local height metres" );
        const auto revision = terrain->getTerrainRevision();
        brush.radius = std::numeric_limits<f32>::quiet_NaN();
        require( !applyTerrainBrush( terrain, brush, manager, error ) &&
                     terrain->getTerrainRevision() == revision,
                 "Invalid brush must not publish or add history" );
        manager->clearAll();
        original.dimensions = Vector2I( 257, 257 );
        original.origin = Vector2F( -128, -128 );
        original.heights.assign( 257 * 257, 0 );
        require( terrain->applyTerrainData( original, error ), error );
        brush.mode = TerrainBrushMode::Raise;
        brush.radius = 1000;
        brush.strength = 1;
        const auto largeRevision = terrain->getTerrainRevision();
        require( !applyTerrainBrush( terrain, brush, manager, error ) &&
                     terrain->getTerrainRevision() == largeRevision && !manager->getPreviousCommand(),
                 "An oversized patch must fail before publishing any samples or history" );
    }
}  // namespace terrain_editing_contracts

#endif
