#include <WPLuabind/WPLuabindPCH.hpp>
#include <luabind/luabind.hpp>
#include <WPLuabind/StringConverter.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/Terrain/LandscapeFoliage.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainLandscapeOcean.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainLandscapeMaterials.hpp>
#include <Workphone/Scene/Components/ProceduralOcean.hpp>
#include <cmath>
#include <limits>

namespace workphone
{
    String terrainLandscapeAuthoring( scene::TerrainSystem *terrain, const String &action,
                                      luabind::object options )
    {
        try
        {
            const auto number = [&]( const char *key, double fallback ) {
                luabind::object value = options[key];
                const auto result = luabind::type( value ) == LUA_TNIL ? fallback :
                                     luabind::object_cast<double>( value );
                if( !std::isfinite( result ) || std::abs( result ) > std::numeric_limits<f32>::max() )
                    throw std::runtime_error( "Landscape values must be finite." );
                return result;
            };
            const auto boolean = [&]( const char *key, bool fallback ) {
                luabind::object value = options[key];
                return luabind::type( value ) == LUA_TNIL ? fallback :
                       luabind::object_cast<bool>( value );
            };
            const auto source = terrain ? terrain->getTerrainSnapshot() : nullptr;
            if( !source ) return "Select a terrain first.";
            auto app = core::IApplicationManager::instancePtr();
            const auto commands = app ? app->getCommandManager() : nullptr;
            String error;
            if( action == "landscapeGet" )
            {
                const auto &settings = source->autoMaterial;
                options["enabled"] = settings.enabled;
                options["seaLevel"] = settings.seaLevel;
                options["beachHeight"] = settings.beachHeight;
                options["beachTransition"] = settings.beachTransition;
                options["rockSlope"] = settings.rockSlope;
                options["rockTransition"] = settings.rockTransition;
                options["soilHeight"] = settings.soilHeight;
                options["soilTransition"] = settings.soilTransition;
                scene::LandscapeFoliageSettings foliage;
                const bool configured = scene::getLandscapeFoliageSettings( terrain, foliage );
                options["foliageEnabled"] = configured && foliage.enabled;
                options["foliageSeaLevel"] = configured ? foliage.seaLevel : settings.seaLevel;
                options["seed"] = foliage.seed;
                options["candidateSpacing"] = foliage.candidateSpacing;
                options["palmDensity"] = foliage.palmDensity;
                options["bushDensity"] = foliage.bushDensity;
                options["grassDensity"] = foliage.grassDensity;
                options["oceanEnabled"] = false;
                options["oceanSeaLevel"] = settings.seaLevel;
                options["oceanMargin"] = 512.f;
                if( auto actor = terrain->getActor() )
                    for( auto child : actor->getChildren() )
                        if( auto ocean = child->getComponent<scene::ProceduralOcean>() )
                            if( ocean->isLandscapeOwned() )
                            {
                                options["oceanEnabled"] = child->isEnabled();
                                options["oceanSeaLevel"] = child->getLocalPosition().y;
                                f32 width = 0;
                                ocean->getProperties()->getPropertyValue( "Width", width );
                                options["oceanMargin"] = std::max( 0.f,
                                    ( width - ( source->dimensions.x - 1 ) * source->spacing.x ) * .5f );
                                break;
                            }
                return {};
            }
            if( action == "landscapeMaterial" )
            {
                auto settings = source->autoMaterial;
                settings.enabled = boolean( "enabled", settings.enabled );
                settings.seaLevel = f32( number( "seaLevel", settings.seaLevel ) );
                settings.beachHeight = f32( number( "beachHeight", settings.beachHeight ) );
                settings.beachTransition = f32( number( "beachTransition", settings.beachTransition ) );
                settings.rockSlope = f32( number( "rockSlope", settings.rockSlope ) );
                settings.rockTransition = f32( number( "rockTransition", settings.rockTransition ) );
                settings.soilHeight = f32( number( "soilHeight", settings.soilHeight ) );
                settings.soilTransition = f32( number( "soilTransition", settings.soilTransition ) );
                scene::applyTerrainAutoMaterial( terrain, settings, commands, error );
                return error;
            }
            if( action == "landscapePalette" )
            {
                scene::applyTerrainIslandMaterials( terrain, commands, error );
                return error;
            }
            if( action == "landscapeTextures" )
            {
                scene::createLandscapeMaterials( terrain, commands, error );
                return error;
            }
            if( action == "landscapeFoliage" )
            {
                scene::LandscapeFoliageSettings settings;
                settings.seaLevel = source->autoMaterial.seaLevel;
                scene::getLandscapeFoliageSettings( terrain, settings );
                settings.enabled = boolean( "enabled", true );
                settings.seaLevel = f32( number( "seaLevel", settings.seaLevel ) );
                const auto seed = number( "seed", settings.seed );
                if( seed < 0 || seed > std::numeric_limits<u32>::max() || std::floor( seed ) != seed )
                    return "Landscape seed must be an unsigned integer.";
                settings.seed = u32( seed );
                settings.candidateSpacing = f32( number( "candidateSpacing", settings.candidateSpacing ) );
                settings.palmDensity = f32( number( "palmDensity", settings.palmDensity ) );
                settings.bushDensity = f32( number( "bushDensity", settings.bushDensity ) );
                settings.grassDensity = f32( number( "grassDensity", settings.grassDensity ) );
                scene::configureLandscapeFoliage( terrain, settings, error );
                return error;
            }
            if( action == "landscapeOcean" )
            {
                scene::configureLandscapeOcean( terrain, boolean( "enabled", true ),
                    f32( number( "seaLevel", source->autoMaterial.seaLevel ) ),
                    f32( number( "margin", 512 ) ), error );
                return error;
            }
            return "Unknown landscape operation.";
        }
        catch( const std::exception &exception )
        {
            return String( "Landscape: " ) + exception.what();
        }
    }
}
