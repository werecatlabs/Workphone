#include <WPLua/LuaManager.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/StringConverter.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/CommandManagerMT.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <luabind/luabind.hpp>
#include <filesystem>
#include <cstdio>
#include <stdexcept>
extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

int main()
{
    using namespace workphone;
    namespace fs = std::filesystem;
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto application = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( application );
    auto commands = make_ptr<CommandManagerMT>();
    application->setCommandManager( commands );
    const auto temporary = fs::temp_directory_path();
    const auto folder =
        temporary / ( std::string( "workphone_terrain_lua_" ) + StringUtil::getUUID().c_str() );
    int result = 0;
    {
        LuaManager manager;
        try
        {
            fs::create_directories( folder );
            manager.load( nullptr );
            auto state = manager.getLuaState();
            if( !state )
                throw std::runtime_error( "Lua state must initialize" );
            auto terrain = make_ptr<scene::TerrainSystem>();
            auto restored = make_ptr<scene::TerrainSystem>();
            render::TerrainData data;
            data.dimensions = Vector2I( 3, 5 );
            data.origin = Vector2F( -1, -2 );
            data.heights.assign( 15, 0 );
            String error;
            if( !terrain->applyTerrainData( data, error ) )
                throw std::runtime_error( error.c_str() );
            luabind::globals( state )["terrainFixture"] = terrain;
            luabind::globals( state )["restoredTerrainFixture"] = restored;
            luabind::globals( state )["terrainFixturePath"] =
                String( ( folder / "edited.terrain.json" ).u8string().c_str() );
            const auto scripts = fs::path( WP_TEST_SOURCE_ROOT ) / "bin/Media/Scripts/Lua/Editor";
            for( const auto file : { "BaseEditor.lua", "TerrainEditor.lua" } )
                if( luaL_dofile( state, ( scripts / file ).string().c_str() ) != LUA_OK )
                    throw std::runtime_error( lua_tostring( state, -1 ) );
            const auto run = [&]( const char *script ) {
                if( luaL_dostring( state, script ) != LUA_OK )
                    throw std::runtime_error( lua_tostring( state, -1 ) );
            };
            run( R"(
                editor = TerrainEditor(nil)
                editor.terrain = terrainFixture
                editor:syncTerrainDataSettings()
                assert(editor._terrainWidth == 3 and editor._terrainDepth == 5)
                assert(editor.settings.heightScale == 1)
                editor.controlBindings[TerrainEditorTypes.HeightScale] = {key='heightScale', valueType='value'}
                editor.controlBindings[TerrainEditorTypes.ChunkSize] = {key='chunkSize', valueType='value'}
                editor.controlBindings[TerrainEditorTypes.HeightmapResolution] = {key='heightmapResolution', valueType='value'}
                assert(editor:updateBoundSetting(TerrainEditorTypes.HeightScale, {getValue=function() return 2 end}))
                assert(editor:updateBoundSetting(TerrainEditorTypes.ChunkSize, {getValue=function() return 64 end}))
                assert(terrainFixture:getHeightMapSize():X() == 3 and terrainFixture:getHeightMapSize():Y() == 5)
                -- An unsupported generation size must preserve the authored rectangular grid and scale.
                local revision = terrainFixture:getTerrainRevision()
                editor.settings.heightmapResolution = 8192
                assert(not editor:performAction(TerrainEditorTypes.GenerateHeight))
                assert(terrainFixture:getTerrainRevision() == revision)
                assert(terrainFixture:getHeightScale() == 2)
                assert(not editor:updateBoundSetting(TerrainEditorTypes.HeightmapResolution, {getValue=function() return 8192 end}))
                assert(string.find(editor._currentStatus, 'rejected', 1, true))
                assert(terrainFixture:getTerrainRevision() == revision)
                editor.settings.brushSize = 0.6
                editor.settings.brushStrength = 4
                editor.settings.brushOpacity = 1
                assert(editor:performAction(TerrainEditorTypes.SculptApplyRaise), editor._currentStatus)
                assert(not editor:performAction(TerrainEditorTypes.ExportMesh))
                assert(string.find(editor._currentStatus, 'Unavailable', 1, true))
                assert(not editor:performAction(TerrainEditorTypes.RunHydraulicErosion))
                editor.settings.outputFile = terrainFixturePath
                assert(editor:performAction(TerrainEditorTypes.SaveTerrain), editor._currentStatus)
                assert(editor:performAction(TerrainEditorTypes.ExportTerrainJson), editor._currentStatus)
                assert(editor:performAction(TerrainEditorTypes.ExportTerrainRecipe), editor._currentStatus)
                assert(editor.settings.outputFile == terrainFixturePath)
            )" );
            if( terrain->getTerrainSnapshot()->heights[7] != 2 )
                throw std::runtime_error( "Real Lua brush must change native terrain samples" );
            auto undo = commands->getPreviousCommand();
            if( !undo )
                throw std::runtime_error( "Lua action must enter editor command history" );
            undo->undo();
            if( terrain->getTerrainSnapshot()->heights[7] != 0 )
                throw std::runtime_error( "Editor undo must restore Lua brush samples" );
            auto redo = commands->getNextCommand();
            if( !redo )
                throw std::runtime_error( "Lua edit must be redoable" );
            redo->redo();
            run( R"(
                editor.terrain = restoredTerrainFixture
                assert(editor:performAction(TerrainEditorTypes.ReloadTerrain), editor._currentStatus)
                assert(editor._terrainWidth == 3 and editor._terrainDepth == 5 and editor.settings.heightScale == 2)
                editor.settings.brushSize = -1
                assert(not editor:performAction(TerrainEditorTypes.SculptApplyRaise))
                assert(string.find(editor._currentStatus, 'failed', 1, true))
                assert(restoredTerrainFixture:importTerrainData('{invalid') ~= '')
                editor.terrain = nil
                assert(not editor:performAction(TerrainEditorTypes.SculptApplyLower))
                editor = nil
                terrainFixture = nil
                restoredTerrainFixture = nil
                collectgarbage('collect')
            )" );
            if( restored->getTerrainSnapshot()->heights != terrain->getTerrainSnapshot()->heights )
                throw std::runtime_error(
                    "Lua Save/Reload must preserve actual samples across components" );
            commands->clearAll();
            manager.unload( nullptr );
            std::puts(
                "Terrain Lua actions, native undo/redo, sample save/reload and capability failures: "
                "PASS" );
        }
        catch( const std::exception &e )
        {
            std::fprintf( stderr, "Terrain Lua workflow: FAIL: %s\n", e.what() );
            result = 1;
            commands->clearAll();
            manager.unload( nullptr );
        }
    }
    application->setCommandManager( nullptr );
    commands = nullptr;
    core::IApplicationManager::setInstance( nullptr );
    application = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    if( folder.parent_path() == temporary &&
        folder.filename().string().find( "workphone_terrain_lua_" ) == 0 )
    {
        std::error_code ignored;
        fs::remove_all( folder, ignored );
    }
    return result;
}
