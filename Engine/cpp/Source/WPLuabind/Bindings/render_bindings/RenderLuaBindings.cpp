#include <WPLuabind/WPLuabindPCH.hpp>
#include <luabind/luabind.hpp>
#include <WPLuabind/StringConverter.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <Workphone/Scene/TerrainEditing.hpp>
#include <Workphone/Scene/TerrainRecipes.hpp>
#include <Workphone/Scene/TerrainMasks.hpp>
#include <Workphone/Scene/TerrainIntegration.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Scene/Components/CollisionTerrain.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <cmath>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace workphone
{
    SmartPtr<scene::TerrainRecipeStack> terrainCreateRecipe( scene::TerrainSystem *terrain )
    {
        auto result = make_ptr<scene::TerrainRecipeStack>();
        String error;
        return result->setBase( terrain, error ) ? result : nullptr;
    }
    // Only conversion/dispatch lives here; terrain math and transaction ownership stay native.
    String terrainAuthoring( scene::TerrainSystem *terrain, const String &action,
                             luabind::object options )
    {
        if( !terrain )
            return "No terrain selected.";
        try
        {
            const auto number = [&]( const char *key, double fallback ) {
                luabind::object value = options[key];
                return luabind::type( value ) == LUA_TNIL ? fallback
                                                          : luabind::object_cast<double>( value );
            };
            const auto text = [&]( const char *key, const String &fallback = String() ) {
                luabind::object value = options[key];
                return luabind::type( value ) == LUA_TNIL ? fallback
                                                          : luabind::object_cast<String>( value );
            };
            const auto boundedInteger = [&]( const char *key, u32 fallback, u32 maximum ) {
                const auto value = number( key, fallback );
                if( !std::isfinite( value ) || value < 0 || value > maximum ||
                    std::floor( value ) != value )
                    throw std::runtime_error(
                        "Terrain integer setting is outside its supported range." );
                return u32( value );
            };
            auto application = core::IApplicationManager::instancePtr();
            auto commands = application ? application->getCommandManager() : nullptr;
            String error;
            if( action == "publicationStatus" )
            {
                auto requested = terrain->getTerrainSnapshot();
                auto active = terrain->getActiveTerrainSnapshot();
                options["pending"] = !active || requested != active;
                options["requestedRevision"] = double( requested ? requested->revision : 0 );
                options["activeRevision"] = double( active ? active->revision : 0 );
                if( auto actor = terrain->getActor() )
                    for( auto collision : actor->getComponentsByType<scene::CollisionTerrain>() )
                        if( !collision->getTerrainError().empty() )
                            options["error"] = collision->getTerrainError();
                return {};
            }
            if( action == "collision" )
            {
                auto actor = terrain->getActor();
                if( !actor )
                    return "Collision publication requires a terrain actor with CollisionTerrain.";
                auto collisions = actor->getComponentsByType<scene::CollisionTerrain>();
                if( collisions.empty() )
                    return "Attach CollisionTerrain and a supported physics body before building "
                           "collision.";
                terrain->updateTransform();
                auto active = terrain->getActiveTerrainSnapshot();
                for( auto collision : collisions )
                {
                    if( !collision->getTerrainError().empty() )
                        return collision->getTerrainError();
                    if( !collision->isValid() || collision->getTerrainSnapshot() != active )
                        return "Collision is waiting for the matching render/source revision to "
                               "publish.";
                }
                return {};
            }
            if( action == "rebuildLod" )
            {
                if( !terrain->getTerrain() )
                    return "Terrain LOD requires an attached renderer.";
                terrain->rebuild();
                return {};
            }
            SmartPtr<scene::TerrainRecipeStack> recipeStack;
            luabind::object recipeValue = options["recipe"];
            if( luabind::type( recipeValue ) != LUA_TNIL )
            {
                recipeStack = luabind::object_cast<SmartPtr<scene::TerrainRecipeStack>>( recipeValue );
                if( !recipeStack )
                    return "Terrain recipe reference is invalid.";
            }
            if( action.find( "recipe" ) == 0 )
            {
                if( !recipeStack )
                    return "Create a bounded terrain recipe first.";
                if( action == "recipePreview" )
                    recipeStack->requestPreview( error );
                else if( action == "recipePoll" )
                    recipeStack->poll( error );
                else if( action == "recipeCommit" )
                    recipeStack->commit( terrain, commands, error );
                else if( action == "recipeCancel" )
                    recipeStack->cancel();
                else if( action == "recipeReset" )
                    recipeStack->setBase( terrain, error );
                else if( action == "recipeSave" )
                    recipeStack->save( text( "path" ), error );
                else if( action == "recipeLoad" )
                    recipeStack->load( text( "path" ), terrain, error );
                else if( action == "recipeRemove" )
                    recipeStack->remove( boundedInteger( "index", 0, 31 ), error );
                else if( action == "recipeMove" )
                    recipeStack->move( boundedInteger( "index", 0, 31 ), boundedInteger( "to", 0, 31 ),
                                       error );
                else if( action == "recipeEnable" )
                    recipeStack->setEnabled( boundedInteger( "index", 0, 31 ),
                                             number( "enabled", 1 ) != 0, error );
                else
                    error = "Unknown terrain recipe operation.";
                return error;
            }
            if( action == "begin" )
            {
                terrain->beginEditStroke( commands, error );
                return error;
            }
            if( action == "commit" )
            {
                terrain->commitEditStroke( error );
                return error;
            }
            if( action == "cancel" )
            {
                terrain->cancelEditStroke( error );
                return error;
            }
            if( action == "exportMesh" )
            {
                scene::exportTerrainMeshFile( terrain, text( "path" ), error );
                return error;
            }
            if( action == "navigation" )
            {
                scene::exportTerrainNavigationFile( terrain, text( "path" ), error );
                return error;
            }
            if( action == "shoreline" )
            {
                scene::exportTerrainShorelineFile( terrain, text( "path" ),
                                                   f32( number( "waterHeight", 0 ) ), error );
                return error;
            }
            if( action == "importHeight" || action == "exportHeight" )
            {
                scene::TerrainHeightFileOptions format;
                const auto encoding = text( "encoding", "pgm16" );
                if( encoding == "raw16" )
                    format.encoding = scene::TerrainHeightEncoding::RawU16LE;
                else if( encoding == "raw32" )
                    format.encoding = scene::TerrainHeightEncoding::RawF32LE;
                else if( encoding != "pgm16" )
                    return "Height encoding must be pgm16, raw16 or raw32.";
                format.dimensions =
                    Vector2I( boundedInteger( "width", 0, render::terrainMaximumDimension ),
                              boundedInteger( "depth", 0, render::terrainMaximumDimension ) );
                format.minimumHeight = f32( number( "minimum", 0 ) );
                format.maximumHeight = f32( number( "maximum", 1 ) );
                if( action == "importHeight" )
                    scene::importTerrainHeightFile( terrain, text( "path" ), format, error );
                else
                    scene::exportTerrainHeightFile( terrain, text( "path" ), format, error );
                return error;
            }
            if( action == "getSurfaceMask" )
            {
                const auto data = terrain->getTerrainSnapshot();
                const auto index = boundedInteger( "index", 0, u32( data->heights.size() - 1 ) );
                const auto mask =
                    data->surfaceMasks.empty() ? Vector2F( 1, 1 ) : data->surfaceMasks[index];
                options["wetness"] = mask.x;
                options["snow"] = mask.y;
                return {};
            }
            if( action == "getAppearance" )
            {
                const auto data = terrain->getTerrainSnapshot();
                const auto &appearance = data->appearance;
                options["projection"] = u32( appearance.projection );
                options["heightBlend"] = appearance.heightBlend ? 1 : 0;
                options["detailNormal"] = appearance.detailNormal;
                options["macroSeed"] = appearance.macroSeed;
                options["triplanarSharpness"] = appearance.triplanarSharpness;
                options["heightTransition"] = appearance.heightTransition;
                options["detailStrength"] = appearance.detailStrength;
                options["macroScale"] = appearance.macroScale;
                options["macroStrength"] = appearance.macroStrength;
                options["wetness"] = appearance.wetness;
                options["wetDarkening"] = appearance.wetDarkening;
                options["wetRoughness"] = appearance.wetRoughness;
                options["snow"] = appearance.snow;
                options["snowRoughness"] = appearance.snowRoughness;
                options["detailUvX"] = appearance.detailUvScale.x;
                options["detailUvZ"] = appearance.detailUvScale.y;
                options["macroRed"] = appearance.macroTint.x;
                options["macroGreen"] = appearance.macroTint.y;
                options["macroBlue"] = appearance.macroTint.z;
                options["snowRed"] = appearance.snowTint.x;
                options["snowGreen"] = appearance.snowTint.y;
                options["snowBlue"] = appearance.snowTint.z;
                return {};
            }
            if( action == "appearance" )
            {
                auto appearance = terrain->getTerrainSnapshot()->appearance;
                appearance.projection = render::TerrainProjection(
                    boundedInteger( "projection", u32( appearance.projection ), 1 ) );
                appearance.heightBlend =
                    boundedInteger( "heightBlend", appearance.heightBlend ? 1 : 0, 1 ) != 0;
                appearance.detailNormal = text( "detailNormal", appearance.detailNormal );
                appearance.macroSeed =
                    boundedInteger( "macroSeed", appearance.macroSeed, std::numeric_limits<u32>::max() );
                appearance.triplanarSharpness =
                    f32( number( "triplanarSharpness", appearance.triplanarSharpness ) );
                appearance.heightTransition =
                    f32( number( "heightTransition", appearance.heightTransition ) );
                appearance.detailStrength = f32( number( "detailStrength", appearance.detailStrength ) );
                appearance.macroScale = f32( number( "macroScale", appearance.macroScale ) );
                appearance.macroStrength = f32( number( "macroStrength", appearance.macroStrength ) );
                appearance.wetness = f32( number( "wetness", appearance.wetness ) );
                appearance.wetDarkening = f32( number( "wetDarkening", appearance.wetDarkening ) );
                appearance.wetRoughness = f32( number( "wetRoughness", appearance.wetRoughness ) );
                appearance.snow = f32( number( "snow", appearance.snow ) );
                appearance.snowRoughness = f32( number( "snowRoughness", appearance.snowRoughness ) );
                appearance.detailUvScale.x = f32( number( "detailUvX", appearance.detailUvScale.x ) );
                appearance.detailUvScale.y = f32( number( "detailUvZ", appearance.detailUvScale.y ) );
                appearance.macroTint.x = f32( number( "macroRed", appearance.macroTint.x ) );
                appearance.macroTint.y = f32( number( "macroGreen", appearance.macroTint.y ) );
                appearance.macroTint.z = f32( number( "macroBlue", appearance.macroTint.z ) );
                appearance.snowTint.x = f32( number( "snowRed", appearance.snowTint.x ) );
                appearance.snowTint.y = f32( number( "snowGreen", appearance.snowTint.y ) );
                appearance.snowTint.z = f32( number( "snowBlue", appearance.snowTint.z ) );
                scene::applyTerrainAppearance( terrain, appearance, commands, error );
                return error;
            }
            if( action == "getLayer" )
            {
                const auto data = terrain->getTerrainSnapshot();
                const auto &layer = data->layers[boundedInteger( "layer", 0, 3 )];
                options["albedo"] = layer.albedo;
                options["normal"] = layer.normal;
                options["orm"] = layer.orm;
                options["tilingX"] = layer.uvScale.x;
                options["tilingZ"] = layer.uvScale.y;
                options["roughness"] = layer.roughness;
                options["metallic"] = layer.metallic;
                options["normalStrength"] = layer.normalStrength;
                options["heightScale"] = layer.heightScale;
                options["heightBias"] = layer.heightBias;
                return {};
            }
            if( action == "layer" )
            {
                auto source = terrain->getTerrainSnapshot();
                auto data = *source;
                const auto layerIndex = boundedInteger( "layer", 0, 3 );
                auto &layer = data.layers[layerIndex];
                layer.albedo = text( "albedo", layer.albedo );
                layer.normal = text( "normal", layer.normal );
                layer.orm = text( "orm", layer.orm );
                layer.uvScale = Vector2F( f32( number( "tilingX", layer.uvScale.x ) ),
                                          f32( number( "tilingZ", layer.uvScale.y ) ) );
                layer.roughness = f32( number( "roughness", layer.roughness ) );
                layer.metallic = f32( number( "metallic", layer.metallic ) );
                layer.normalStrength = f32( number( "normalStrength", layer.normalStrength ) );
                layer.heightScale = f32( number( "heightScale", layer.heightScale ) );
                layer.heightBias = f32( number( "heightBias", layer.heightBias ) );
                scene::applyTerrainLayer( terrain, layerIndex, layer, commands, error );
                return error;
            }
            scene::TerrainBrush brush;
            brush.centre = Vector2F( f32( number( "x", 0 ) ), f32( number( "z", 0 ) ) );
            brush.radius = f32( number( "radius", 10 ) );
            brush.strength = f32( number( "strength", 1 ) );
            brush.targetHeight = f32( number( "target", 0 ) );
            brush.layer = boundedInteger( "layer", 0, 3 );
            brush.seed = boundedInteger( "seed", 0, std::numeric_limits<u32>::max() );
            brush.terraceStep = f32( number( "terraceStep", 1 ) );
            brush.minHeight = f32( number( "minHeight", brush.minHeight ) );
            brush.maxHeight = f32( number( "maxHeight", brush.maxHeight ) );
            brush.minSlope = f32( number( "minSlope", 0 ) );
            brush.maxSlope = f32( number( "maxSlope", 90 ) );
            if( action == "thermal" || action == "hydraulic" || action == "erosionMask" )
            {
                scene::TerrainErosionRecipe recipe;
                recipe.region = brush;
                recipe.seed = brush.seed;
                recipe.mode = action == "thermal" ? scene::TerrainErosionMode::Thermal
                                                  : scene::TerrainErosionMode::Hydraulic;
                recipe.iterations = boundedInteger( "iterations", 16, 1024 );
                recipe.rain = f32( number( "rain", 0.01 ) );
                recipe.sedimentCapacity = f32( number( "capacity", 0.25 ) );
                recipe.talusAngle = f32( number( "talus", 30 ) );
                if( action == "erosionMask" )
                {
                    scene::exportTerrainErosionMask( terrain, text( "path" ), recipe, error );
                    return error;
                }
                if( recipeStack )
                {
                    scene::TerrainRecipeOperation operation;
                    operation.kind = scene::TerrainRecipeOperation::Kind::Erosion;
                    operation.erosion = recipe;
                    recipeStack->append( operation, error );
                }
                else
                    scene::applyTerrainErosion( terrain, recipe, commands, error );
                return error;
            }
            if( action == "ruleMask" )
            {
                scene::exportTerrainRuleMask( terrain, text( "path" ), brush, error );
                return error;
            }
            if( action == "road" || action == "river" )
            {
                scene::TerrainGradeRecipe recipe;
                recipe.width = f32( number( "width", 4 ) );
                recipe.shoulder = f32( number( "shoulder", 2 ) );
                recipe.strength = brush.strength;
                recipe.river = action == "river";
                recipe.depth = f32( number( "riverDepth", 1 ) );
                // Authored centreline file is plain local-metre x y z rows, not an opaque recipe.
                const auto path = text( "path" );
                if( path.empty() || path.find( '\0' ) != String::npos )
                    return "Set a centreline file containing local x y z rows.";
                std::ifstream file( std::filesystem::u8path( path.c_str() ),
                                    std::ios::binary | std::ios::ate );
                if( !file || file.tellg() > std::streamoff( 128 * 1024 ) )
                    return "Centreline file is missing or exceeds 128 KiB.";
                file.seekg( 0 );
                std::string line;
                while( std::getline( file, line ) )
                {
                    if( line.empty() || line[0] == '#' )
                        continue;
                    std::istringstream row( line );
                    row.imbue( std::locale::classic() );
                    Vector3F point;
                    std::string extra;
                    if( !( row >> point.x >> point.y >> point.z ) || row >> extra )
                        return "Centreline rows require exactly three finite coordinates.";
                    if( recipe.points.size() >= 1024 )
                        return "Centreline exceeds 1024 points.";
                    recipe.points.push_back( point );
                }
                if( recipeStack )
                {
                    scene::TerrainRecipeOperation operation;
                    operation.kind = scene::TerrainRecipeOperation::Kind::Grade;
                    operation.grade = recipe;
                    recipeStack->append( operation, error );
                }
                else
                    scene::applyTerrainGrade( terrain, recipe, commands, error );
                return error;
            }
            if( action == "pick" )
            {
                Transform3<real_Num> placement;
                if( auto actor = terrain->getActor() )
                    if( auto transform = actor->getTransform() )
                        placement = transform->getWorldTransform();
                const Vector3F origin( f32( number( "rayX", 0 ) ), f32( number( "rayY", 0 ) ),
                                       f32( number( "rayZ", 0 ) ) );
                const Vector3F direction( f32( number( "dirX", 0 ) ), f32( number( "dirY", -1 ) ),
                                          f32( number( "dirZ", 0 ) ) );
                Vector3<real_Num> hit;
                if( !render::intersectTerrain( *terrain->getTerrainSnapshot(), placement,
                                               Ray3F( origin, direction ), hit ) )
                    return "Brush ray does not hit solid terrain.";
                auto local = placement.getOrientation().inverse() * ( hit - placement.getPosition() );
                const auto scale = placement.getScale();
                options["x"] = f32( local.x / scale.x );
                options["z"] = f32( local.z / scale.z );
                options["hitX"] = f32( hit.x );
                options["hitY"] = f32( hit.y );
                options["hitZ"] = f32( hit.z );
                return {};
            }
            const auto mode = action == "update" ? text( "mode", "raise" ) : action;
            if( mode == "raise" )
                brush.mode = scene::TerrainBrushMode::Raise;
            else if( mode == "lower" )
                brush.mode = scene::TerrainBrushMode::Lower;
            else if( mode == "smooth" )
                brush.mode = scene::TerrainBrushMode::Smooth;
            else if( mode == "flatten" )
                brush.mode = scene::TerrainBrushMode::Flatten;
            else if( mode == "stamp" )
                brush.mode = scene::TerrainBrushMode::Stamp;
            else if( mode == "paint" )
                brush.mode = scene::TerrainBrushMode::Paint;
            else if( mode == "clearPaint" )
                brush.mode = scene::TerrainBrushMode::ClearPaint;
            else if( mode == "normalize" )
                brush.mode = scene::TerrainBrushMode::Normalize;
            else if( mode == "hole" )
                brush.mode = scene::TerrainBrushMode::Hole;
            else if( mode == "fillHole" )
                brush.mode = scene::TerrainBrushMode::FillHole;
            else if( mode == "terrace" )
                brush.mode = scene::TerrainBrushMode::Terrace;
            else if( mode == "noise" )
                brush.mode = scene::TerrainBrushMode::Noise;
            else if( mode == "paintWetness" )
                brush.mode = scene::TerrainBrushMode::PaintWetness;
            else if( mode == "clearWetness" )
                brush.mode = scene::TerrainBrushMode::ClearWetness;
            else if( mode == "paintSnow" )
                brush.mode = scene::TerrainBrushMode::PaintSnow;
            else if( mode == "clearSnow" )
                brush.mode = scene::TerrainBrushMode::ClearSnow;
            else
                return "Terrain operation is unavailable.";
            const auto stampPath = text( "stampPath" );
            if( mode == "stamp" && !stampPath.empty() )
            {
                auto stamp = make_ptr<scene::TerrainSystem>();
                scene::TerrainHeightFileOptions format;
                if( !scene::importTerrainHeightFile( stamp, stampPath, format, error ) )
                    return error;
                const auto data = stamp->getTerrainSnapshot();
                brush.stampDimensions = data->dimensions;
                brush.stamp = data->heights;
            }
            if( recipeStack )
            {
                scene::TerrainRecipeOperation operation;
                operation.brush = brush;
                recipeStack->append( operation, error );
            }
            else if( action == "update" )
                terrain->updateEditStroke( brush, error );
            else
                scene::applyTerrainBrush( terrain, brush, commands, error );
            return error;
        }
        catch( const std::exception &e )
        {
            return String( "Invalid terrain settings: " ) + e.what();
        }
        catch( ... )
        {
            return "Invalid terrain settings table.";
        }
    }
}  // namespace workphone
