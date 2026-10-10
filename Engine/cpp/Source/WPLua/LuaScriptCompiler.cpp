#include <WPLua/LuaScriptCompiler.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>
extern "C" {
#include <lauxlib.h>
}

namespace workphone
{
    String LuaScriptCompiler::name() const { return "WorkphoneLuaSource"; }

    Array<resource::CompilerOutput> LuaScriptCompiler::outputs() const
    {
        return { { resource::ResourceTypeID( "lua" ), version } };
    }

    bool LuaScriptCompiler::getDependencies( const resource::CompileContext &context,
                                             resource::DependencySet &dependencies,
                                             String &error ) const
    {
        std::filesystem::path path( std::string( context.sourcePath.data(), context.sourcePath.size() ) );
        path += ".deps";
        std::error_code status;
        if( !std::filesystem::exists( path, status ) )
        {
            if( status ) { error = "Cannot inspect Lua dependency manifest"; return false; }
            return true;
        }
        std::ifstream input( path, std::ios::binary );
        if( !input ) { error = "Cannot read Lua dependency manifest"; return false; }
        dependencies.compileDependencies.push_back(
            resource::CompileDependency::data( context.resourceId.str() + ".deps" ) );
        std::set<String> seen;
        std::string line;
        size_t count = 0;
        while( std::getline( input, line ) )
        {
            if( ++count > 4096 || line.size() > 4096 )
            { error = "Lua dependency manifest exceeds limits"; return false; }
            line = line.substr( 0, line.find( '#' ) );
            const auto first = line.find_first_not_of( " \t\r" );
            if( first == String::npos ) continue;
            line = line.substr( first, line.find_last_not_of( " \t\r" ) - first + 1 );
            resource::ResourceID id;
            if( !id.set( String( line.c_str() ), &error ) ) return false;
            if( seen.insert( id.str() ).second )
            {
                dependencies.compileDependencies.push_back( resource::CompileDependency::resource( id ) );
                dependencies.installDependencies.push_back( id );
            }
        }
        if( input.bad() ) { error = "Lua dependency manifest read failed"; return false; }
        return true;
    }

    resource::CompilationStatus LuaScriptCompiler::compile(
        const resource::CompileContext &context, std::ostream &output, Array<String> &messages ) const
    {
        using resource::CompilationStatus;
        std::ifstream input( std::filesystem::path( std::string( context.sourcePath.data(),
                                                                 context.sourcePath.size() ) ),
                             std::ios::binary | std::ios::ate );
        if( !input ) { messages.push_back( "Cannot open Lua source" ); return CompilationStatus::Failure; }
        const auto length = input.tellg();
        if( length < 0 || static_cast<u64>( length ) > maxSourceBytes )
        { messages.push_back( "Lua source exceeds 8 MiB limit" ); return CompilationStatus::Failure; }
        String source( static_cast<size_t>( length ), '\0' );
        input.seekg( 0 );
        input.read( source.data(), static_cast<std::streamsize>( source.size() ) );
        if( !input ) { messages.push_back( "Lua source read failed" ); return CompilationStatus::Failure; }
        // Match Lua file-loader conventions without losing source line numbering.
        if( source.compare( 0, 3, "\xef\xbb\xbf" ) == 0 ) source.erase( 0, 3 );
        if( !source.empty() && source[0] == '#' )
        {
            auto end = source.find( '\n' );
            source.replace( 0, end == String::npos ? source.size() : end, "" );
        }
        std::unique_ptr<lua_State, decltype( &lua_close )> state( luaL_newstate(), lua_close );
        if( !state ) { messages.push_back( "Cannot allocate Lua compiler state" ); return CompilationStatus::Failure; }
        const String name = "@" + context.resourceId.sourceRelativePath();
        if( luaL_loadbufferx( state.get(), source.data(), source.size(), name.c_str(), "t" ) != LUA_OK )
        {
            const auto text = lua_tostring( state.get(), -1 );
            messages.push_back( text ? text : "Lua compilation failed" );
            return CompilationStatus::Failure;
        }
        output.write( source.data(), static_cast<std::streamsize>( source.size() ) );
        if( !output ) { messages.push_back( "Lua resource write failed" ); return CompilationStatus::Failure; }
        return CompilationStatus::Success;
    }
}
