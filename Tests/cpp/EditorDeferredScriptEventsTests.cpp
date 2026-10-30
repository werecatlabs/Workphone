#include "ui/DeferredEditorScriptEvents.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

using workphone::editor::DeferredEditorScriptEvents;
using workphone::editor::invokeEditorScriptCall;
using workphone::editor::shouldDeferEditorScriptCall;
using workphone::editor::isCurrentEditorScriptEventGeneration;

struct ScriptAbiProbe
{
    int argumentCount = -1, resultWrites = 0;
    void callObjectMember( const char * ) { argumentCount = 0; }
    void callObjectMember( const char *, const std::vector<int> & ) { argumentCount = 1; }
    void callObjectMember( const char *, const std::vector<int> &parameters, std::vector<int> &results )
    {
        argumentCount = 2;
        results.push_back( parameters.at( 0 ) );
        ++resultWrites;
    }
};

static void require( bool condition, const char *message )
{
    if( !condition ) throw std::runtime_error( message );
}

int main()
{
    try
    {
        DeferredEditorScriptEvents events( 4, 2 );
        enum class Task { Render, Application };
        Task task = Task::Render;
        bool renderFrameActive = true;
        std::vector<int> applied;
        require( events.enqueue( [&] {
            require( task == Task::Application && !renderFrameActive, "Lua entered an active render frame" );
            applied.push_back( 1 );
            // A paused callback must be able to drain debugger actions without
            // a queue lock deadlock or running unrelated authoring actions.
            require( events.enqueue( [&] { applied.push_back( 3 ); }, false ), "Authoring enqueue failed" );
            require( events.enqueue( [&] { applied.push_back( 2 ); }, true ), "Debugger enqueue failed" );
            events.dispatch( true );
            require( applied == std::vector<int>( { 1, 2 } ), "Paused dispatch ran authoring work" );
        }, false ), "Render event enqueue failed" );
        require( applied.empty(), "Render event executed synchronously" );
        renderFrameActive = false;
        task = Task::Application;
        events.dispatch( false );
        require( applied == std::vector<int>( { 1, 2 } ) && events.size() == 1,
                 "Dispatch drained newly queued authoring work in the same pass" );
        events.dispatch( false );
        require( applied == std::vector<int>( { 1, 2, 3 } ), "Retained authoring action was lost" );

        auto window = std::make_shared<int>( 1 );
        std::weak_ptr<int> owner = window;
        bool expiredOwnerCalled = false;
        events.enqueue( [owner, &expiredOwnerCalled] { if( owner.lock() ) expiredOwnerCalled = true; }, false );
        window.reset();
        events.dispatch( false );
        require( !expiredOwnerCalled, "An expired window received an event" );

        int refresh = 0;
        for( int i = 1; i <= 100; ++i )
            require( events.enqueue( [&, i] { refresh = i; }, false, 17, "update" ), "Refresh coalescing failed" );
        require( events.size() == 1, "Refresh events filled the bounded queue" );
        events.dispatch( true );
        require( refresh == 0 && events.size() == 1, "Paused refresh mutated authoring state" );
        events.dispatch( false );
        require( refresh == 100, "Refresh did not retain the latest state" );

        applied.clear();
        events.enqueue( [&] { applied.push_back( 1 ); }, false, 17, "cancel" );
        events.enqueue( [&] { applied.push_back( 2 ); }, false, 17 );
        events.enqueue( [&] { applied.push_back( 3 ); }, false, 17, "cancel" );
        events.dispatch( false );
        require( applied == std::vector<int>( { 1, 2, 3 } ), "Coalescing moved a cancel across a begin command" );

        for( int i = 0; i < 4; ++i ) require( events.enqueue( [] {}, false ), "Queue filled too early" );
        require( !events.enqueue( [] {}, false ), "Queue bound was not enforced" );
        bool continued = false;
        require( events.enqueue( [&] { continued = true; }, true ), "Authoring flood rejected Continue" );
        events.dispatch( true );
        require( continued && events.size() == 4, "Continue failed or drained retained authoring work" );
        require( events.enqueue( [] {}, true ) && events.enqueue( [] {}, true ), "Debugger lane filled too early" );
        require( !events.enqueue( [] {}, true ), "Debugger lane bound was not enforced" );
        events.clear();
        require( events.size() == 0, "Clearing did not retire queued captures" );
        for( int i = 0; i < 4; ++i ) require( events.enqueue( [] {}, false ), "Clear did not reset authoring capacity" );
        require( events.enqueue( [] {}, true ) && events.enqueue( [] {}, true ), "Clear did not reset debugger capacity" );
        events.clear();

        ScriptAbiProbe abi;
        const std::vector<int> parameters{ 42 };
        invokeEditorScriptCall( abi, "handleEvent", parameters, true, true );
        require( abi.argumentCount == 2 && abi.resultWrites == 1, "Synchronous event lost its mutable results argument" );
        events.enqueue( [&] { invokeEditorScriptCall( abi, "handleEvent", parameters, true, true ); }, true );
        require( abi.resultWrites == 1, "Queued results callback ran before dispatch" );
        events.dispatch( true );
        require( abi.argumentCount == 2 && abi.resultWrites == 2, "Deferred event lost its mutable results argument" );
        invokeEditorScriptCall( abi, "show", parameters, false, false );
        require( abi.argumentCount == 0, "Zero-argument callback acquired parameters" );
        invokeEditorScriptCall( abi, "setProperties", parameters, true, false );
        require( abi.argumentCount == 1 && abi.resultWrites == 2, "Parameters-only callback acquired results" );

        require( shouldDeferEditorScriptCall( true, false, false, false, true ),
                 "Normal terrain Render callbacks must defer without a debugger" );
        require( !shouldDeferEditorScriptCall( false, false, false, false, true ),
                 "Normal terrain Application callbacks must retain their synchronous ABI" );
        require( !shouldDeferEditorScriptCall( true, false, false, false, false ),
                 "Terrain dispatch must not change other normal Editor panels" );
        require( shouldDeferEditorScriptCall( true, true, false, true, false ) &&
                 shouldDeferEditorScriptCall( false, true, true, false, true ) &&
                 !shouldDeferEditorScriptCall( false, true, true, true, false ),
                 "Terrain dispatch must preserve debugger Render and paused-control rules" );

        task = Task::Render;
        renderFrameActive = true;
        applied.clear();
        for( int phase : { 1, 2, 3 } )
        {
            auto terrainCall = [&, phase] {
                require( task == Task::Application && !renderFrameActive,
                         "Normal terrain Lua entered a frame that can own texture-manager locks" );
                applied.push_back( phase );
            };
            if( shouldDeferEditorScriptCall( task == Task::Render, false, false, false, true ) )
                require( events.enqueue( terrainCall, false ), "Normal terrain command enqueue failed" );
            else terrainCall();
        }
        require( applied.empty(), "Normal terrain commands entered Lua synchronously on Render" );
        events.dispatch( true );
        require( applied.empty(), "Debugger-only drain executed ordinary terrain authoring" );
        task = Task::Application;
        renderFrameActive = false;
        events.dispatch( false );
        require( applied == std::vector<int>( { 1, 2, 3 } ),
                 "Normal terrain dispatch lost begin/sample/commit ordering" );

        std::uint64_t selectionGeneration = 1;
        applied.clear();
        const auto queueBrush = [&]( std::uint64_t capturedGeneration, int phase ) {
            require( events.enqueue( [&, capturedGeneration, phase] {
                if( isCurrentEditorScriptEventGeneration( capturedGeneration, selectionGeneration ) )
                    applied.push_back( phase );
            }, false ), "Generation-scoped terrain event enqueue failed" );
        };
        const auto firstTerrain = selectionGeneration;
        queueBrush( firstTerrain, 1 );
        ++selectionGeneration;
        // An old render snapshot may enqueue even after selection changes. Its
        // original generation, not the generation at enqueue time, must travel.
        queueBrush( firstTerrain, 3 );
        queueBrush( selectionGeneration, 11 );
        queueBrush( selectionGeneration, 13 );
        events.dispatch( false );
        require( applied == std::vector<int>( { 11, 13 } ),
                 "An old terrain stroke edited the new selection or suppressed its current commands" );
        queueBrush( selectionGeneration, 14 );
        ++selectionGeneration; // Hiding/unloading must also retire pending brush work.
        events.dispatch( false );
        require( applied == std::vector<int>( { 11, 13 } ),
                 "Retired terrain viewport work executed after lifecycle invalidation" );
        std::cout << "Editor deferred script events: PASS\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
