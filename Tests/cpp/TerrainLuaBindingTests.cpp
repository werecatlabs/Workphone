#include <WPLua/LuaManager.hpp>
#include <WPLua/LuaObjectData.hpp>
#include <WPImGui/SceneViewportToolInput.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/StringConverter.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/CommandManagerMT.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <Workphone/Interface/Script/IScriptClass.hpp>
#include <luabind/luabind.hpp>
#include <filesystem>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <limits>
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
            application->setProjectPath( folder.u8string().c_str() );
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
                const auto top = lua_gettop( state );
                lua_pushcfunction( state, []( lua_State *s ) {
                    luaL_traceback( s, s, lua_tostring( s, 1 ), 1 ); return 1;
                } );
                const auto handler = lua_gettop( state );
                if( luaL_loadstring( state, script ) != LUA_OK || lua_pcall( state, 0, 0, handler ) != LUA_OK )
                {
                    const std::string message = lua_tostring( state, -1 );
                    lua_settop( state, top ); throw std::runtime_error( message );
                }
                lua_settop( state, top );
            };

            // Exercise the same Parameters/results ABI used by TerrainWindow,
            // rather than calling the public Lua helpers directly.
            auto bridgeTerrain = make_ptr<scene::TerrainSystem>();
            if( !bridgeTerrain->applyTerrainData( data, error ) )
                throw std::runtime_error( error.c_str() );
            luabind::globals( state )["bridgeTerrain"] = bridgeTerrain;
            auto bridgeOwner = make_ptr<Properties>();
            if( !manager.createObject( "TerrainEditor", bridgeOwner ) )
                throw std::runtime_error( "Native terrain viewport bridge instance must initialize" );
            auto bridgeData = dynamic_pointer_cast<LuaObjectData>( bridgeOwner->getScriptData() );
            if( !bridgeData ) throw std::runtime_error( "Native terrain bridge must retain Lua object data" );
            luabind::globals( state )["bridgeEditor"] = bridgeData->getObject();
            run( R"(
                bridgeEditor.terrain = bridgeTerrain
                bridgeEditor.settings.viewportBrushEnabled = true
                bridgeEditor.settings.brushMode = 0
                bridgeEditor.settings.brushSize = 0.6
                bridgeEditor.settings.brushStrength = 1
                bridgeEditor.settings.brushOpacity = 1
            )" );
            const auto invokeBridge = [&]( const String &member, const Parameters &parameters ) {
                Parameters values;
                manager.callObjectMember( bridgeOwner, member, parameters, values );
                if( manager.getError() ) throw std::runtime_error( manager.getLastDiagnostic().c_str() );
                return values;
            };
            auto viewport = invokeBridge( "getViewportScriptState", {} );
            if( viewport.size() != 3 || viewport[0].type != ParameterType::PARAM_TYPE_BOOL || !viewport[0].getBool() ||
                viewport[1].type != ParameterType::PARAM_TYPE_F64 || viewport[1].getF64() != 0.6 ||
                viewport[2].type != ParameterType::PARAM_TYPE_BOOL || viewport[2].getBool() )
                throw std::runtime_error( "Native viewport bridge must return enabled, radius and solid-only policy" );
            if( !invokeBridge( "getViewportBrushEnabled", {} ).empty() )
                throw std::runtime_error( "Viewport bridge must preserve the global results-container ABI" );
            run( R"(
                bridgeEditor.recipe = bridgeTerrain:createTerrainRecipe()
                assert(bridgeEditor.recipe)
                bridgeEditor._showRecipePreview = true
                bridgeEditor._recipePending = false
            )" );
            viewport = invokeBridge( "getViewportScriptState", {} );
            if( viewport.size() != 5 || viewport[2].getBool() || !viewport[3].object || viewport[4].object != bridgeTerrain )
                throw std::runtime_error( "Native viewport bridge must retain recipe then terrain object identity" );
            run( "bridgeEditor._showRecipePreview = false; bridgeEditor.recipe = nil" );
            const auto original = bridgeTerrain->getTerrainSnapshot()->heights;
            const auto brush = [&]( const char *phase ) {
                return invokeBridge( "handleViewportBrushEvent",
                    { Parameter( phase ), Parameter( 0.0 ), Parameter( 20.0 ), Parameter( 0.0 ),
                      Parameter( 0.0 ), Parameter( -1.0 ), Parameter( 0.0 ) } );
            };
            const auto requireApplied = [&]( const char *phase ) {
                const auto values = brush( phase );
                if( values.size() != 1 || !values[0].getBool() )
                {
                    const auto status = luabind::object_cast<String>( bridgeData->getObject()["_currentStatus"] );
                    throw std::runtime_error( std::string( "Native viewport brush phase '" ) + phase +
                        "' failed; results=" + std::to_string( values.size() ) + "; status=" + status.c_str() );
                }
            };
            requireApplied( "begin" );
            if( bridgeTerrain->getTerrainSnapshot()->heights == original )
                throw std::runtime_error( "Native viewport begin must apply actual terrain samples" );
            struct ViewportBridgeCamera
            {
                bool validProjection = false;
                Matrix4<real_Num> getViewMatrix() const { return Matrix4<real_Num>::identity(); }
                Matrix4<real_Num> getProjectionMatrix() const
                {
                    auto projection = Matrix4<real_Num>::identity();
                    if( !validProjection ) projection[0][0] = std::numeric_limits<real_Num>::quiet_NaN();
                    return projection;
                }
            };
            Parameters invalidRayCancel;
            if( ui::makeSceneViewportToolArguments( ViewportBridgeCamera{}, { 8, 80 }, { 384, 300 },
                    { 195, 244 }, "update", invalidRayCancel ) )
                throw std::runtime_error( "Invalid native viewport projection must not emit a brush update" );
            const auto cancelledRay = invokeBridge( "handleViewportBrushEvent", invalidRayCancel );
            if( cancelledRay.size() != 1 || !cancelledRay[0].getBool() ||
                bridgeTerrain->getTerrainSnapshot()->heights != original )
                throw std::runtime_error( "Invalid viewport ray must cancel the active native stroke and restore samples" );
            requireApplied( "begin" );
            Parameters outsideRelease;
            if( !ui::makeSceneViewportToolArguments( ViewportBridgeCamera{ true }, { 8, 86 }, { 384, 300 },
                    { 230, 70 }, ui::terrainViewportBrushPhase( false, false, true, false, false ), outsideRelease ) ||
                outsideRelease[0].getStr() != "cancel" )
                throw std::runtime_error( "Release-outside viewport input must produce the actual cancel protocol" );
            const auto outsideResult = invokeBridge( "handleViewportBrushEvent", outsideRelease );
            if( outsideResult.size() != 1 || !outsideResult[0].getBool() ||
                bridgeTerrain->getTerrainSnapshot()->heights != original ||
                luabind::object_cast<String>( bridgeData->getObject()["_currentStatus"] ) != "Cancelled terrain stroke." )
                throw std::runtime_error( "Release outside must restore native samples and report cancellation explicitly" );
            requireApplied( "begin" );
            requireApplied( "cancel" );
            if( bridgeTerrain->getTerrainSnapshot()->heights != original )
                throw std::runtime_error( "Native viewport cancel must restore actual terrain samples" );
            requireApplied( "begin" );
            requireApplied( "commit" );
            const auto committed = bridgeTerrain->getTerrainSnapshot()->heights;
            auto viewportUndo = commands->getPreviousCommand();
            if( committed == original || !viewportUndo )
                throw std::runtime_error( "Native viewport commit must retain an undoable terrain edit" );
            viewportUndo->undo();
            if( bridgeTerrain->getTerrainSnapshot()->heights != original )
                throw std::runtime_error( "Native viewport commit undo must restore the original samples" );
            auto viewportRedo = commands->getNextCommand();
            if( !viewportRedo ) throw std::runtime_error( "Native viewport commit must retain redo history" );
            viewportRedo->redo();
            if( bridgeTerrain->getTerrainSnapshot()->heights != committed )
                throw std::runtime_error( "Native viewport commit redo must restore the committed samples" );
            requireApplied( "cancel" );
            if( luabind::object_cast<String>( bridgeData->getObject()["_currentStatus"] ) !=
                    "Committed terrain stroke. Use Edit > Undo/Redo." )
                throw std::runtime_error( "Idle outside cancellation must not replace a completed stroke's status" );

            // Target an interior hole with a radius too small to reach any solid cell.
            // Exercise the real Lua results-container adapter, native picking, edit and history.
            commands->clearAll();
            render::TerrainData holeData;
            holeData.dimensions = { 9, 9 };
            holeData.origin = { -4, -4 };
            holeData.heights.resize( 81 );
            holeData.holes.assign( 64, 0 );
            for( s32 z = 0; z < 9; ++z )
                for( s32 x = 0; x < 9; ++x )
                    holeData.heights[size_t( z ) * 9 + x] = 3 + .2f * x + .1f * z;
            holeData.heights[4 * 9 + 4] += 1;
            for( s32 z = 2; z < 6; ++z )
                for( s32 x = 2; x < 6; ++x ) holeData.holes[size_t( z ) * 8 + x] = 1;
            if( !bridgeTerrain->applyTerrainData( holeData, error ) )
                throw std::runtime_error( error.c_str() );
            const Ray3F repairRay( { .25f, 20, .5f }, { 0, -1, 0 } );
            Vector3<real_Num> repairHit;
            f32 repairHeight = 0;
            if( render::intersectTerrain( *bridgeTerrain->getTerrainSnapshot(), {}, repairRay, repairHit ) ||
                render::sampleTerrainHeight( *bridgeTerrain->getTerrainSnapshot(), {}, { .25, 0, .5 }, repairHeight ) )
                throw std::runtime_error( "Interior hole must remain empty for gameplay rays and support queries" );
            run( "bridgeEditor.settings.brushMode = 0; bridgeEditor.settings.brushSize = 0.4" );
            const auto repairBrush = [&]( const char *phase ) {
                return invokeBridge( "handleViewportBrushEvent",
                    { Parameter( phase ), Parameter( .25 ), Parameter( 20.0 ), Parameter( .5 ),
                      Parameter( 0.0 ), Parameter( -1.0 ), Parameter( 0.0 ) } );
            };
            const auto requireRepair = [&]( const char *phase, bool expected ) {
                const auto values = repairBrush( phase );
                if( values.size() != 1 || values[0].getBool() != expected )
                {
                    const auto status = luabind::object_cast<String>( bridgeData->getObject()["_currentStatus"] );
                    throw std::runtime_error( std::string( "Hole repair viewport phase '" ) + phase +
                        "' had unexpected result; status=" + status.c_str() );
                }
            };
            requireRepair( "hover", false );
            requireRepair( "begin", false );
            if( bridgeTerrain->getTerrainSnapshot()->holes != holeData.holes )
                throw std::runtime_error( "Ordinary viewport tools must not edit through a hole" );
            run( "bridgeEditor.settings.brushMode = 9" );
            viewport = invokeBridge( "getViewportScriptState", {} );
            if( viewport.size() != 3 || !viewport[0].getBool() || !viewport[2].getBool() ||
                viewport[1].getF64() != 0.4 )
                throw std::runtime_error( "Fill Hole must publish its explicit authoring pick policy with the cached radius" );
            run( "bridgeEditor.settings.brushMode = 8" );
            viewport = invokeBridge( "getViewportScriptState", {} );
            if( viewport.size() != 3 || viewport[2].getBool() )
                throw std::runtime_error( "Switching to Hole must remove the cached authoring-only picking opt-in" );
            run( "bridgeEditor.settings.brushMode = 9" );
            requireRepair( "hover", true );
            {
                const luabind::object preview = bridgeData->getObject()["brushPreview"];
                if( std::abs( luabind::object_cast<double>( preview["x"] ) - .25 ) > .0001 ||
                    std::abs( luabind::object_cast<double>( preview["y"] ) - 4.55 ) > .0001 ||
                    std::abs( luabind::object_cast<double>( preview["z"] ) - .5 ) > .0001 )
                    throw std::runtime_error( "Fill Hole preview must hit the authored nonplanar surface inside the void" );
            }
            auto repairedHoles = holeData.holes;
            repairedHoles[4 * 8 + 4] = 0;
            const auto requireHoleState = [&]( const Array<u8> &expected, const char *message ) {
                const auto source = bridgeTerrain->getTerrainSnapshot();
                if( source->holes != expected || source->heights != holeData.heights )
                    throw std::runtime_error( message );
            };
            requireRepair( "begin", true );
            requireHoleState( repairedHoles, "Fill Hole must repair only the targeted native cell without changing heights" );
            requireRepair( "cancel", true );
            requireHoleState( holeData.holes, "Cancelled Fill Hole must restore the original native hole patch" );
            requireRepair( "begin", true );
            requireRepair( "commit", true );
            requireHoleState( repairedHoles, "Committed Fill Hole must preserve its native cell repair" );
            auto repairUndo = commands->getPreviousCommand();
            if( !repairUndo ) throw std::runtime_error( "Fill Hole must create an undoable edit" );
            repairUndo->undo();
            requireHoleState( holeData.holes, "Fill Hole undo must reopen the exact original hole patch" );
            if( render::intersectTerrain( *bridgeTerrain->getTerrainSnapshot(), {}, repairRay, repairHit ) )
                throw std::runtime_error( "Undo must remove gameplay ray support at the repaired cell" );
            auto repairRedo = commands->getNextCommand();
            if( !repairRedo ) throw std::runtime_error( "Fill Hole must retain redo history" );
            repairRedo->redo();
            requireHoleState( repairedHoles, "Fill Hole redo must restore the exact committed hole repair" );
            if( !render::intersectTerrain( *bridgeTerrain->getTerrainSnapshot(), {}, repairRay, repairHit ) ||
                std::abs( repairHit.y - 4.55 ) > .0001 ||
                render::intersectTerrain( *bridgeTerrain->getTerrainSnapshot(), {},
                    Ray3F( { -.75f, 20, .5f }, { 0, -1, 0 } ), repairHit ) )
                throw std::runtime_error( "Only the filled cell may regain gameplay ray support after redo" );
            run( "bridgeEditor.settings.viewportBrushEnabled = false" );
            viewport = invokeBridge( "getViewportScriptState", {} );
            if( viewport.size() != 3 || viewport[0].getBool() || viewport[2].getBool() )
                throw std::runtime_error( "Native viewport bridge must publish disabled state" );
            const auto rejected = invokeBridge( "handleViewportBrushEvent", {} );
            if( rejected.size() != 1 || rejected[0].getBool() )
                throw std::runtime_error( "Malformed native viewport events must be rejected" );
            manager.destroyObject( bridgeOwner );
            run( "bridgeEditor = nil; bridgeTerrain = nil; collectgarbage('collect')" );
            commands->clearAll();
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
                -- Real foliage authoring dispatch through the established Lua editor.
                editor.settings.foliageSpecies=0
                editor.settings.foliageId='lua/grass'
                editor.settings.foliageKind=2
                editor.settings.foliageHeight=.7
                editor.settings.foliageRadius=.3
                editor.settings.foliageBranches=12
                editor.settings.foliageAlbedo='fixture/grass-cutout'
                assert(editor:performAction(TerrainEditorTypes.FoliagePlant), editor._currentStatus)
                assert(string.find(editor._currentStatus, 'Preview pending', 1, true), 'Headless fixture must report the unavailable render preview explicitly')
                local status={}
                assert(restoredTerrainFixture:editTerrain('foliageStatus',status) == '')
                assert(status.speciesCount == 1 and status.rules[1].kind == 2)
                local authoredWind=status.rules[1].windAmplitude
                assert(editor:foliageOperation('foliageRule',{rule=0,density=.3,candidateSpacing=2,minimumSpacing=0}))
                status={}; assert(restoredTerrainFixture:editTerrain('foliageStatus',status) == '')
                assert(status.rules[1].windAmplitude == authoredWind, 'Changing density must preserve the authored continuous wind amplitude')
                local scattered=status.instanceCount
                assert(scattered > 0, 'Density controls must create real compact instances')
                editor.settings.brushMode=14
                editor.settings.layerPaintSize=.2
                editor.settings.foliageCount=3
                editor.settings.viewportBrushEnabled=true
                assert(editor:handleViewportBrush('begin',0,30,0,0,-1,0),editor._currentStatus)
                assert(editor:handleViewportBrush('commit',0,30,0,0,-1,0),editor._currentStatus)
                status={}; assert(restoredTerrainFixture:editTerrain('foliageStatus',status) == '')
                assert(status.instanceCount == scattered+3 and not status.strokeActive)
                assert(editor:performAction(TerrainEditorTypes.FoliageUndo))
                status={}; assert(restoredTerrainFixture:editTerrain('foliageStatus',status) == '')
                assert(status.instanceCount == scattered)
                assert(editor:performAction(TerrainEditorTypes.FoliageRedo))
                assert(editor:handleViewportBrush('begin',0,30,0,0,-1,0),editor._currentStatus)
                assert(editor:handleViewportBrush('cancel',0,0,0,0,0,0),editor._currentStatus)
                status={}; assert(restoredTerrainFixture:editTerrain('foliageStatus',status) == '')
                assert(status.instanceCount == scattered+3 and not status.strokeActive)
                local exported={}; assert(restoredTerrainFixture:editTerrain('foliageExport',exported) == '')
                assert(terrainFixture:editTerrain('foliageImport',{source=exported.source}) == '')
                local reopened={}; assert(terrainFixture:editTerrain('foliageStatus',reopened) == '')
                assert(reopened.instanceCount == status.instanceCount and reopened.rules[1].density == status.rules[1].density)
                assert(editor:performAction(TerrainEditorTypes.FoliageSave),editor._currentStatus)
                assert(editor:performAction(TerrainEditorTypes.FoliageLoad),editor._currentStatus)
                editor.controlBindings[TerrainEditorTypes.GrassWind]={key='grassWind',valueType='toggle'}
                assert(editor:updateBoundSetting(TerrainEditorTypes.GrassWind,{getValue=function() return false end}))
                assert(editor:performAction(TerrainEditorTypes.FoliageUndo))
                assert(editor:applyFoliageRule('grass','density',.2))
                status={}; assert(restoredTerrainFixture:editTerrain('foliageStatus',status) == '')
                assert(status.rules[1].wind, 'Changing density after undo must preserve the restored wind field')
                assert(restoredTerrainFixture:editTerrain('foliagePlant',{id='lua/tree',kind=0,branches=3}) == '')
                local beforeBake={}; assert(restoredTerrainFixture:editTerrain('foliageExport',beforeBake) == '')
                local rejectedBake=restoredTerrainFixture:editTerrain('foliageBakeImpostors',{species=1,resourcePrefix='data://baked/tree',directory=terrainFixturePath,tileSize=32,views=2})
                assert(string.find(rejectedBake,'must match',1,true),rejectedBake)
                local afterBake={}; assert(restoredTerrainFixture:editTerrain('foliageExport',afterBake) == '')
                assert(afterBake.source == beforeBake.source,'Invalid bake path must preserve complete authoring state')
                local baked={species=1,resourcePrefix='data://baked/tree',directory='',tileSize=32,views=2}
                assert(restoredTerrainFixture:editTerrain('foliageBakeImpostors',baked) == '')
                assert(baked.pending and baked.speciesPath and baked.directory,'Real source bake reports the renderer handoff and derived directory')
                local bakedSource={}; assert(restoredTerrainFixture:editTerrain('foliageExport',bakedSource) == '')
                assert(string.find(bakedSource.source,'data://baked/tree_albedo.texres',1,true),'Baked authoring stores stable typed resource IDs')
                assert(terrainFixture:editTerrain('foliageImport',{source=bakedSource.source}) == '')
                editor.terrain = nil
                assert(not editor:performAction(TerrainEditorTypes.SculptApplyLower))
                editor = nil
                terrainFixture = nil
                restoredTerrainFixture = nil
                collectgarbage('collect')
            )" );
            if( !fs::exists( folder / "baked/tree_albedo.tga" ) || !fs::exists( folder / "baked/tree_orm.texres" ) ||
                !fs::exists( folder / "baked/species.folspec" ) )
                throw std::runtime_error( "Real Lua bake must export a complete reopenable typed atlas under the project root" );
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
