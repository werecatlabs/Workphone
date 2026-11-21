#pragma once

#include <EditorApplication.hpp>
#include <editor/McpAuthoring.hpp>
#include <editor/McpPlaytest.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

namespace workphone::editor
{
    inline boost::json::object mcpEvaluateLua( const boost::json::object &args )
    {
        auto scripts = core::IApplicationManager::instance()->getScriptManager();
        if( !scripts || !scripts->isLoaded() )
        {
            throw std::runtime_error( "The Editor Lua plugin is unavailable" );
        }
        // Pass source as an argument, including syntax errors, through the ordinary
        // protected Lua invocation and mutable Parameters result boundary.
        scripts->loadScriptFromString( R"lua(
function WorkphoneMcpEvaluate(parameters, result)
    local ok, value = pcall(function()
        local compile = loadstring or load
        local chunk, message = compile(parameters:getAsString(0), "=MCP")
        if not chunk then error(message, 0) end
        local value = chunk()
        if value == nil then return "null" end
        return require("cjson").encode(value)
    end)
    result:addAsBool(ok)
    result:addAsString(tostring(value))
end
)lua" );
        Parameters parameters, results;
        const auto &source = args.at( "source" ).as_string();
        if( source.find( '\0' ) != boost::json::string::npos )
        {
            throw std::runtime_error( "Lua source contains a null character" );
        }
        parameters.emplace_back( String( source.c_str() ) );
        scripts->callFunction( "WorkphoneMcpEvaluate", parameters, results );
        if( results.size() != 2 )
        {
            throw std::runtime_error( "Lua execution failed before returning a result" );
        }
        const auto value = results[1].getStr();
        if( !results[0].getBool() )
        {
            throw std::runtime_error( value.c_str() );
        }
        return { { "value", boost::json::parse( value.c_str() ) } };
    }

    inline boost::json::object mcpCapture( EditorApplication &app, const std::filesystem::path &session )
    {
        auto manager = core::IApplicationManager::instance();
        if( !manager->isPlaying() )
        {
            // Authoring may enqueue component and render work. Let the editor
            // finish an application tick and present the resulting view first.
            auto timer = manager->getTimer();
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
            for( auto task : { TaskId::Application, TaskId::Render } )
            {
                const auto tick = timer->getTickCount( task );
                while( timer->getTickCount( task ) == tick )
                {
                    if( std::chrono::steady_clock::now() >= deadline || manager->getQuit() )
                    {
                        throw std::runtime_error( "The editor did not render a capture frame" );
                    }
                    app.iterate();
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
            }
        }
        auto graphics = manager->getGraphicsSystem();
        auto window = graphics ? graphics->getDefaultWindow() : nullptr;
        if( !window )
        {
            throw std::runtime_error( "Editor render window unavailable" );
        }
        const auto size = window->getSize();
        if( size.x <= 0 || size.y <= 0 || size.x > 16384 || size.y > 16384 ||
            uint64_t( size.x ) * size.y > 32ull * 1024 * 1024 )
        {
            throw std::runtime_error( "Invalid Editor back-buffer dimensions" );
        }
        std::vector<uint8_t> pixels( size_t( size.x ) * size.y * 4 );
        if( !window->tryCopyContentsToMemory( pixels.data(), u32( pixels.size() ) ) )
        {
            throw std::runtime_error( "The renderer could not read the Editor back buffer" );
        }
        std::ofstream image( session / "frame.rgba", std::ios::binary );
        image.exceptions( std::ios::badbit | std::ios::failbit );
        image.write( reinterpret_cast<const char *>( pixels.data() ), pixels.size() );
        image.close();
        return { { "width", size.x }, { "height", size.y } };
    }

    inline void runMcpSession( EditorApplication &app, const std::filesystem::path &projects,
                               const std::filesystem::path &session )
    {
        // This private reply file keeps engine/plugin stdout out of the MCP stream.
        // The launcher consumes each complete reply before sending the next request.
        auto publish = [&]( const boost::json::object &reply ) {
            const auto staging = session / "reply.tmp";
            std::ofstream output( staging, std::ios::binary );
            output.exceptions( std::ios::badbit | std::ios::failbit );
            output << boost::json::serialize( reply );
            output.close();
            std::filesystem::rename( staging, session / "reply.json" );
        };
        auto manager = core::IApplicationManager::instance();
        auto timer = dynamic_pointer_cast<TimerMT>( manager->getTimer() );
        app.iterate();
        publish( { { "ready", true } } );
        std::string line;
        while( true )
        {
            if( timer )
            {
                timer->beginSuspension();
            }
            // A client may spend minutes inspecting a result. Keep the native
            // window responsive while gameplay waits for the next explicit step.
            auto input = GetStdHandle( STD_INPUT_HANDLE );
            DWORD available = 0;
            auto graphics = manager->getGraphicsSystem();
            bool open = true;
            if( GetFileType( input ) == FILE_TYPE_PIPE )
            {
                while( PeekNamedPipe( input, nullptr, 0, nullptr, &available, nullptr ) && !available )
                {
                    if( graphics )
                    {
                        graphics->messagePump();
                    }
                    if( manager->getQuit() )
                    {
                        open = false;
                        break;
                    }
                    MsgWaitForMultipleObjects( 0, nullptr, FALSE, 20, QS_ALLINPUT );
                }
            }
            const bool received = open && bool( std::getline( std::cin, line ) );
            if( timer )
            {
                timer->endSuspension();
            }
            if( !received )
            {
                break;
            }
            boost::json::object reply;
            try
            {
                const auto command = boost::json::parse( line );
                const auto &args = command.at( "arguments" ).as_object();
                const std::string name( command.at( "name" ).as_string() );
                if( name == "editor_lua" )
                {
                    reply["result"] = mcpEvaluateLua( args );
                }
                else if( name == "editor_capture" )
                {
                    reply["result"] = mcpCapture( app, session );
                }
                else if( name.rfind( "play_", 0 ) == 0 )
                {
                    reply["result"] = mcpPlaytest( name, args, app );
                }
                else
                {
                    reply["result"] = mcpAuthor( name, args, projects );
                }
            }
            catch( const std::exception &error )
            {
                reply["error"] = error.what();
            }
            publish( reply );
        }
    }
}  // namespace workphone::editor
