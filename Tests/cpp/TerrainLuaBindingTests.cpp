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
                -- Unsupported material controls must neither report success nor change saved settings/data.
                local unsupported = {
                    {TerrainEditorTypes.LayerBlendMode, 'layerBlendMode'},
                    {TerrainEditorTypes.LayerUVSet, 'layerUVSet'},
                    {TerrainEditorTypes.LayerProjection, 'layerProjection'},
                    {TerrainEditorTypes.LayerOffsetX, 'layerOffsetX'},
                    {TerrainEditorTypes.LayerOffsetY, 'layerOffsetY'},
                    {TerrainEditorTypes.LayerRotation, 'layerRotation'},
                    {TerrainEditorTypes.LayerTriplanar, 'layerTriplanar'},
                    {TerrainEditorTypes.LayerTriplanarScale, 'layerTriplanarScale'},
                    {TerrainEditorTypes.LayerAO, 'layerAO'},
                    {TerrainEditorTypes.LayerHeightBlend, 'layerHeightBlend'},
                    {TerrainEditorTypes.LayerHeightContrast, 'layerHeightContrast'},
                    {TerrainEditorTypes.LayerPaintHardness, 'layerPaintHardness'},
                }
                local materialRevision = terrainFixture:getTerrainRevision()
                for _, setting in ipairs(unsupported) do
                    local previous = editor.settings[setting[2]]
                    editor.controlBindings[setting[1]] = {key=setting[2], valueType='value'}
                    assert(not editor:updateBoundSetting(setting[1], {getValue=function() return 2 end}))
                    assert(string.find(editor._currentStatus, 'Unavailable:', 1, true))
                    assert(editor.settings[setting[2]] == previous)
                    assert(terrainFixture:getTerrainRevision() == materialRevision)
                end
                for _, setting in ipairs({
                    {TerrainEditorTypes.LayerSlopeMin, 'layerSlopeMin'},
                    {TerrainEditorTypes.LayerSlopeMax, 'layerSlopeMax'},
                    {TerrainEditorTypes.LayerHeightMin, 'layerHeightMin'},
                    {TerrainEditorTypes.LayerHeightMax, 'layerHeightMax'},
                }) do
                    local previous = editor.settings[setting[2]]
                    editor.controlBindings[setting[1]] = {key=setting[2], valueType='value'}
                    assert(editor:updateBoundSetting(setting[1], {getValue=function() return 12 end}))
                    assert(editor.settings[setting[2]] == 12)
                    assert(string.find(editor._currentStatus, 'auto-paint', 1, true))
                    assert(terrainFixture:getTerrainRevision() == materialRevision)
                    editor.settings[setting[2]] = previous
                end
                local layer = {layer=0}
                assert(terrainFixture:editTerrain('getLayer', layer) == '')
                local priorRoughness = layer.roughness
                editor.controlBindings[TerrainEditorTypes.LayerRoughness] = {key='layerRoughness', valueType='value'}
                assert(editor:updateBoundSetting(TerrainEditorTypes.LayerRoughness, {getValue=function() return 0.4 end}))
                assert(terrainFixture:editTerrain('getLayer', layer) == '' and math.abs(layer.roughness - 0.4) < 0.0001)
                materialRevision = terrainFixture:getTerrainRevision()
                assert(not editor:updateBoundSetting(TerrainEditorTypes.LayerRoughness, {getValue=function() return 2 end}))
                assert(terrainFixture:getTerrainRevision() == materialRevision)
                assert(editor:updateBoundSetting(TerrainEditorTypes.LayerRoughness, {getValue=function() return priorRoughness end}))
                -- Global appearance controls use persistent native values, with rejected edits reported.
                editor.controlBindings[TerrainEditorTypes.Wetness] = {key='wetness', valueType='value'}
                editor.controlBindings[TerrainEditorTypes.AppearanceProjection] = {key='appearanceProjection', valueType='selected'}
                editor.controlBindings[TerrainEditorTypes.HeightBlendEnabled] = {key='heightBlendEnabled', valueType='toggle'}
                assert(editor:updateBoundSetting(TerrainEditorTypes.Wetness, {getValue=function() return 0.7 end}))
                assert(editor:updateBoundSetting(TerrainEditorTypes.AppearanceProjection, {getSelectedOption=function() return 1 end}))
                assert(editor:updateBoundSetting(TerrainEditorTypes.HeightBlendEnabled, {getValue=function() return true end}))
                local appearance = {}
                assert(terrainFixture:editTerrain('getAppearance', appearance) == '')
                assert(math.abs(appearance.wetness - 0.7) < 0.0001 and appearance.projection == 1 and appearance.heightBlend == 1)
                materialRevision = terrainFixture:getTerrainRevision()
                assert(not editor:updateBoundSetting(TerrainEditorTypes.Wetness, {getValue=function() return 2 end}))
                assert(terrainFixture:getTerrainRevision() == materialRevision and math.abs(editor.settings.wetness - 0.7) < 0.0001)
                editor.controlBindings[TerrainEditorTypes.LayerHeightBias] = {key='layerHeightBias', valueType='value'}
                assert(editor:updateBoundSetting(TerrainEditorTypes.LayerHeightBias, {getValue=function() return -0.3 end}))
                assert(terrainFixture:editTerrain('getLayer', layer) == '' and math.abs(layer.heightBias + 0.3) < 0.0001)
                -- Undo modifies native state before any UI refresh. Editing a different field must not replay stale values.
                local history = IApplicationManager.instance():getCommandManager()
                editor.controlBindings[TerrainEditorTypes.Snow] = {key='snow', valueType='value'}
                assert(editor:updateBoundSetting(TerrainEditorTypes.Snow, {getValue=function() return 0.5 end}))
                history:getPreviousCommand():undo()
                assert(editor.settings.snow == 0.5, 'Fixture must exercise an actually stale cached field')
                assert(editor:updateBoundSetting(TerrainEditorTypes.Wetness, {getValue=function() return 0.8 end}))
                assert(terrainFixture:editTerrain('getAppearance', appearance) == '' and appearance.snow == 0 and math.abs(appearance.wetness - 0.8) < 0.0001)
                assert(editor.settings.snow == 0, 'Native values must resynchronize after a partial material edit')
                local priorBias = layer.heightBias
                assert(editor:updateBoundSetting(TerrainEditorTypes.LayerHeightBias, {getValue=function() return 0.4 end}))
                history:getPreviousCommand():undo()
                assert(math.abs(editor.settings.layerHeightBias - 0.4) < 0.0001)
                assert(editor:updateBoundSetting(TerrainEditorTypes.LayerRoughness, {getValue=function() return 0.6 end}))
                assert(terrainFixture:editTerrain('getLayer', layer) == '' and math.abs(layer.heightBias - priorBias) < 0.0001)
                -- External changes are reflected by the editor update without replaying authoring commands.
                assert(terrainFixture:editTerrain('appearance', {snow=0.25}) == '')
                local externalRevision = terrainFixture:getTerrainRevision()
                editor:update()
                assert(editor.settings.snow == 0.25 and terrainFixture:getTerrainRevision() == externalRevision)
                editor.settings.layerPaintSize = 0.6
                editor.settings.layerPaintStrength = 1
                assert(editor:performAction(TerrainEditorTypes.ClearWetness), editor._currentStatus)
                local mask = {index=7}
                assert(terrainFixture:editTerrain('getSurfaceMask', mask) == '' and mask.wetness == 0 and mask.snow == 1)
                assert(editor:performAction(TerrainEditorTypes.PaintWetness), editor._currentStatus)
                assert(terrainFixture:editTerrain('getSurfaceMask', mask) == '' and mask.wetness == 1 and mask.snow == 1)
                -- A real viewport snow stroke uses the mask channel and restores it on cancellation.
                editor.settings.brushMode = 13
                editor.settings.viewportBrushEnabled = true
                assert(editor:handleViewportBrush('begin', 0, 20, 0, 0, -1, 0))
                assert(terrainFixture:editTerrain('getSurfaceMask', mask) == '' and mask.snow == 0)
                assert(editor:handleViewportBrush('cancel', 0, 0, 0, 0, 0, 0))
                assert(terrainFixture:editTerrain('getSurfaceMask', mask) == '' and mask.snow == 1)
                editor.settings.brushMode = 0
                editor.settings.viewportBrushEnabled = false
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
                editor.settings.outputFile = terrainFixturePath
                assert(editor:performAction(TerrainEditorTypes.SaveTerrain), editor._currentStatus)
                assert(editor:performAction(TerrainEditorTypes.ExportTerrainJson), editor._currentStatus)
                assert(editor:performAction(TerrainEditorTypes.ExportTerrainRecipe), editor._currentStatus)
                assert(editor.settings.outputFile == terrainFixturePath)
                -- The actual Lua viewport adapter groups drag updates and restores cancelled edits.
                editor.settings.viewportBrushEnabled = true
                assert(editor:handleViewportBrush('begin', 0, 20, 0, 0, -1, 0))
                assert(editor:handleViewportBrush('update', 1, 20, 1, 0, -1, 0))
                assert(editor:handleViewportBrush('cancel', 0, 0, 0, 0, -1, 0))
                assert(editor:handleViewportBrush('begin', 0, 20, 0, 0, -1, 0))
                editor.controlBindings[TerrainEditorTypes.ViewportBrushEnabled] = {key='viewportBrushEnabled', valueType='toggle'}
                assert(editor:updateBoundSetting(TerrainEditorTypes.ViewportBrushEnabled, {getValue=function() return false end}))
                assert(editor._strokeTerrain == nil and not editor:getViewportBrushEnabled())
                -- A transient native publication lock must retain the original stroke owner for retry.
                local attempts = 0
                local busyOwner = {editTerrain=function(self, action, options)
                    assert(action == 'cancel')
                    attempts = attempts + 1
                    return attempts == 1 and 'Physics publication busy' or ''
                end}
                editor._strokeTerrain = busyOwner
                assert(not editor:handleViewportBrush('cancel', 0, 0, 0, 0, 0, 0))
                assert(editor._strokeTerrain == busyOwner and editor._cancelStrokePending)
                editor:update()
                assert(attempts == 2 and editor._strokeTerrain == nil and not editor._cancelStrokePending)
                assert(editor:performAction(TerrainEditorTypes.ExportMesh), editor._currentStatus)
                editor.settings.outputFile = terrainFixturePath
                assert(editor:performAction(TerrainEditorTypes.ExportHeightData), editor._currentStatus)
                editor.settings.outputFile = terrainFixturePath
                -- Recipe actions only prepare data; cancel leaves the saved sculpt and undo history intact.
                editor.settings.recipeMode = true
                editor.settings.brushStrength = 0.2
                assert(editor:ensureRecipe(), editor._currentStatus)
                collectgarbage('collect')
                assert(editor.recipe:getOperationCount() == 0)
                assert(editor:performAction(TerrainEditorTypes.SculptApplyNoise), editor._currentStatus)
                collectgarbage('collect')
                assert(editor.recipe:getOperationCount() == 1, 'Queued recipe must retain one owned operation after Lua collection')
                assert(editor:performAction(TerrainEditorTypes.RecipePreview), editor._currentStatus)
                assert(editor._recipePending and editor._showRecipePreview)
                assert(editor:performAction(TerrainEditorTypes.RecipeSave), editor._currentStatus)
                assert(editor._recipePending and editor._showRecipePreview, 'Saving an in-flight recipe must retain its polling and visible-preview intent')
                collectgarbage('collect')
                assert(editor.recipe:getOperationCount() == 1)
                assert(editor:getTerrainRecipePreview() == nil, 'Pending preview must not be drawn')
                for attempt=1,100000 do
                    editor:update()
                    if not editor._recipePending then break end
                end
                assert(not editor._recipePending, 'Recipe worker must finish')
                local previewRecipe, previewOwner = editor:getTerrainRecipePreview()
                assert(previewRecipe == editor.recipe and previewOwner == editor.terrain)
                assert(editor:performAction(TerrainEditorTypes.RecipeCancel), editor._currentStatus)
                assert(not editor.recipe:isBusy())
                assert(editor:getTerrainRecipePreview() == nil, 'Cancellation hides the prepared overlay')
                assert(editor:performAction(TerrainEditorTypes.RecipeSave), editor._currentStatus)
                collectgarbage('collect')
                assert(editor.recipe:getOperationCount() == 1)
                editor.recipe = nil
                collectgarbage('collect')
                assert(editor:performAction(TerrainEditorTypes.RecipeLoad), editor._currentStatus)
                collectgarbage('collect')
                assert(editor.recipe:getOperationCount() == 1)
                assert(editor:performAction(TerrainEditorTypes.RecipePreview), editor._currentStatus)
                collectgarbage('collect')
                assert(editor:performAction(TerrainEditorTypes.RecipeCancel), editor._currentStatus)
                editor.settings.recipeMode = false
                editor.settings.outputFile = terrainFixturePath
            )" );
            if( !fs::exists( folder / "edited.obj" ) || !fs::exists( folder / "edited.r32" ) ||
                !fs::exists( folder / "edited.terrain.recipe.json" ) )
                throw std::runtime_error(
                    "Lua exports must write actual mesh, float heights and ordered recipe files" );
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
