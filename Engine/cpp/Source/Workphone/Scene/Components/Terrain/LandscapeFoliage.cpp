#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Terrain/LandscapeFoliage.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePresets.hpp>
#include <Workphone/Scene/Components/Terrain/FoliageAuthoring.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <cJSON.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        const char *speciesIds[] = { "landscape/palm", "landscape/bush", "landscape/grass" };
        constexpr FoliagePreset presets[] = { FoliagePreset::Palm, FoliagePreset::Bush,
                                              FoliagePreset::Grass };

        void require( bool condition, const char *message )
        {
            if( !condition )
            {
                throw std::runtime_error( message );
            }
        }

        int landscapeKind( const String &id )
        {
            for( int index = 0; index < 3; ++index )
            {
                if( id == speciesIds[index] )
                {
                    return index;
                }
            }
            return -1;
        }

        bool managed( const render::FoliageWorldData &world, u32 species )
        {
            return species < world.species.size() && world.species[species] &&
                   landscapeKind( world.species[species]->id ) >= 0;
        }

        void validateSettings( const LandscapeFoliageSettings &settings )
        {
            require( std::isfinite( settings.seaLevel ) && std::abs( settings.seaLevel ) <= 100000 &&
                         std::isfinite( settings.candidateSpacing ) &&
                         settings.candidateSpacing >= .05f && settings.candidateSpacing <= 256,
                     "Landscape foliage needs finite sea level and candidate spacing .05..256 metres." );
            for( auto density : { settings.palmDensity, settings.bushDensity, settings.grassDensity } )
            {
                require( std::isfinite( density ) && density >= 0 && density <= 10000,
                         "Landscape foliage densities must be finite and between zero and 10000." );
            }
        }

        void checkCandidateBudget( const render::TerrainData &terrain,
                                   const FoliagePlacementRecipe &recipe )
        {
            render::FoliageCellKey low, high;
            String error;
            const auto end =
                terrain.origin + Vector2F( ( terrain.dimensions.x - 1 ) * terrain.spacing.x,
                                           ( terrain.dimensions.y - 1 ) * terrain.spacing.y );
            require(
                render::foliageCellForPosition( { terrain.origin.x, 0, terrain.origin.y },
                                                recipe.pageSize, low, error ) &&
                    render::foliageCellForPosition( { end.x, 0, end.y }, recipe.pageSize, high, error ),
                "Landscape foliage bounds exceed the page grid." );
            const double pagesX = double( high.x ) - low.x + 1;
            const double pagesZ = double( high.z ) - low.z + 1;
            require( pagesX * pagesZ <= render::foliageMaximumPages,
                     "Landscape foliage exceeds 2048 pages; use a smaller terrain." );
            const auto spacing = double( recipe.candidateSpacing );
            const auto width = std::ceil( ( double( high.x ) + 1 ) * recipe.pageSize / spacing ) -
                               std::floor( double( low.x ) * recipe.pageSize / spacing ) + 10;
            const auto depth = std::ceil( ( double( high.z ) + 1 ) * recipe.pageSize / spacing ) -
                               std::floor( double( low.z ) * recipe.pageSize / spacing ) + 10;
            require( width * depth <= FoliageAuthoring::maximumCandidates,
                     "Landscape foliage exceeds 262144 candidates; increase candidate spacing (8m suits "
                     "a 2048m island)." );
        }

        // Disabling generated landscape plants must not reroll unrelated species.
        // Existing manual, locked, removed and promoted instances remain authored.
        bool withoutGeneratedLandscape( const String &authored, String &output, String &error )
        {
            auto candidate = make_ptr<FoliageAuthoring>();
            if( !candidate->deserialize( authored, error ) )
            {
                return false;
            }
            auto world = *candidate->getSnapshot();
            world.instances.erase( std::remove_if( world.instances.begin(), world.instances.end(),
                                                   [&]( const auto &instance ) {
                                                       return instance.flags == 0 &&
                                                              managed( world, instance.species );
                                                   } ),
                                   world.instances.end() );
            String encoded;
            if( !render::serializeFoliageWorld( world, encoded, error ) )
            {
                return false;
            }
            std::unique_ptr<cJSON, decltype( &cJSON_Delete )> root( cJSON_Parse( authored.c_str() ),
                                                                    cJSON_Delete );
            std::unique_ptr<cJSON, decltype( &cJSON_Delete )> replacement(
                cJSON_Parse( encoded.c_str() ), cJSON_Delete );
            require( root && replacement, "Cannot stage disabled landscape foliage." );
            require( cJSON_ReplaceItemInObjectCaseSensitive( root.get(), "world", replacement.get() ),
                     "Cannot replace landscape foliage population." );
            replacement.release();
            std::unique_ptr<char, decltype( &cJSON_free )> text( cJSON_PrintUnformatted( root.get() ),
                                                                 cJSON_free );
            require( bool( text ), "Cannot serialize disabled landscape foliage." );
            output = text.get();
            return true;
        }
    }  // namespace

    bool configureLandscapeFoliage( TerrainSystem *terrain, const LandscapeFoliageSettings &settings,
                                    String &error )
    {
        try
        {
            require( terrain != nullptr, "Select a terrain before configuring landscape foliage." );
            validateSettings( settings );
            const auto ground = terrain->getTerrainSnapshot();
            require( ground && render::validateTerrainData( *ground, error ),
                     "Landscape foliage requires valid terrain." );
            auto current = terrain->getFoliageAuthoring();
            require( !current->isStrokeActive(), "Commit or cancel the current foliage stroke first." );
            const auto original = current->getSnapshot();
            String originalText;
            if( !current->serialize( originalText, error ) )
            {
                return false;
            }
            auto candidate = make_ptr<FoliageAuthoring>();
            if( !candidate->deserialize( originalText, error ) )
            {
                return false;
            }
            auto recipe = candidate->getRecipe();
            auto species = candidate->getSnapshot()->species;
            if( settings.enabled )
            {
                recipe.seed = settings.seed;
                recipe.candidateSpacing = settings.candidateSpacing;
                recipe.pageSize = std::max( recipe.pageSize, 128.f );
                checkCandidateBudget( *ground, recipe );
                const auto app = core::IApplicationManager::instancePtr();
                require( app && !app->getProjectPath().empty(),
                         "Landscape foliage needs an open project for its generated texture assets." );
                for( u32 kind = 0; kind < 3; ++kind )
                {
                    auto plant = foliagePresetRecipe( presets[kind] );
                    plant.speciesId = speciesIds[kind];
                    plant.seed = settings.seed ^ ( 7919u + kind * 104729u );
                    plant.wind.amplitude = kind == 0 ? .15f : .05f;
                    plant.wind.frequency = .65f;
                    plant.wind.flutter = kind == 0 ? .025f : .015f;
                    plant.wind.bendStart = kind == 0 ? 2.f : 0.f;
                    plant.wind.bendEnd = plant.height;
                    if( kind == 2 )
                    {
                        // Scatter grass patches on the bounded landscape grid,
                        // retaining the shared grass-card generator and cutout.
                        plant.radius = 1.6f;
                        plant.branches = 40;
                    }
                    if( !saveFoliagePresetCutout( presets[kind], app->getProjectPath(),
                                                  settings.assetDirectory, plant.leaves.albedo, error ) )
                    {
                        return false;
                    }
                    auto value = std::make_shared<render::FoliageSpecies>();
                    if( !generateFoliagePreset( presets[kind], plant, *value, error ) )
                    {
                        return false;
                    }
                    value->maximumDistance = kind == 0 ? 700.f : ( kind == 1 ? 250.f : 100.f );
                    value->shadowDistance = kind == 0 ? 120.f : ( kind == 1 ? 60.f : 20.f );
                    auto found = std::find_if( species.begin(), species.end(), [&]( const auto &entry ) {
                        return entry && entry->id == speciesIds[kind];
                    } );
                    if( found == species.end() )
                    {
                        require( species.size() < render::foliageMaximumSpecies,
                                 "Landscape foliage exceeds the 64-species budget." );
                        species.push_back( value );
                    }
                    else
                    {
                        *found = value;
                    }
                }
                if( !candidate->setSpecies( species, error ) )
                {
                    return false;
                }
            }
            const float densities[] = { settings.palmDensity, settings.bushDensity,
                                        settings.grassDensity };
            for( u32 index = 0; index < species.size(); ++index )
            {
                const auto kind = species[index] ? landscapeKind( species[index]->id ) : -1;
                if( kind < 0 )
                {
                    continue;
                }
                auto found =
                    std::find_if( recipe.rules.begin(), recipe.rules.end(),
                                  [index]( const auto &rule ) { return rule.species == index; } );
                FoliagePlacementRule rule;
                if( found != recipe.rules.end() )
                {
                    rule = *found;
                }
                rule.species = index;
                rule.enabled = settings.enabled && densities[kind] > 0;
                rule.density = densities[kind];
                rule.minimumHeight = settings.seaLevel + .75f;
                rule.maximumHeight = settings.seaLevel + ( kind == 0 ? 40.f : 180.f );
                rule.maximumSlope = kind == 0 ? 28.f : ( kind == 1 ? 35.f : 40.f );
                rule.minimumSpacing = std::min( recipe.candidateSpacing * 4,
                                                kind == 0 ? 7.f : ( kind == 1 ? 3.f : 1.5f ) );
                rule.weight = kind == 0 ? 1.f : ( kind == 1 ? 2.f : 5.f );
                rule.allowedLayers = kind == 0 ? 1u : ( kind == 1 ? 10u : 2u );
                rule.minimumLayerWeight = kind == 0 ? .25f : .45f;
                rule.randomYaw = true;
                rule.alignToNormal = kind != 0;
                rule.minimumScale = .8f;
                rule.maximumScale = 1.25f;
                if( found == recipe.rules.end() )
                {
                    recipe.rules.push_back( rule );
                }
                else
                {
                    *found = rule;
                }
            }
            if( !candidate->configure( recipe, error ) )
            {
                return false;
            }
            if( settings.enabled && !candidate->regenerateAll( ground, error ) )
            {
                return false;
            }
            String serialized;
            if( !candidate->serialize( serialized, error ) )
            {
                return false;
            }
            if( !settings.enabled )
            {
                String filtered;
                if( !withoutGeneratedLandscape( serialized, filtered, error ) )
                {
                    return false;
                }
                serialized = std::move( filtered );
            }
            require( terrain->getTerrainSnapshot() == ground &&
                         terrain->getFoliageAuthoring() == current &&
                         current->getSnapshot() == original && !current->isStrokeActive(),
                     "Terrain or foliage changed while landscape generation was preparing; retry the "
                     "operation." );
            return terrain->importFoliageData( serialized, error );
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }

    bool getLandscapeFoliageSettings( TerrainSystem *terrain, LandscapeFoliageSettings &settings )
    {
        if( !terrain )
        {
            return false;
        }
        try
        {
            const auto authoring = terrain->getFoliageAuthoring();
            const auto world = authoring->getSnapshot();
            const auto recipe = authoring->getRecipe();
            auto result = settings;
            result.enabled = false;
            result.seed = recipe.seed;
            result.candidateSpacing = recipe.candidateSpacing;
            bool found = false;
            for( const auto &rule : recipe.rules )
            {
                if( !managed( *world, rule.species ) )
                {
                    continue;
                }
                const auto &species = *world->species[rule.species];
                const auto kind = landscapeKind( species.id );
                if( kind == 0 )
                {
                    result.palmDensity = rule.density;
                }
                if( kind == 1 )
                {
                    result.bushDensity = rule.density;
                }
                if( kind == 2 )
                {
                    result.grassDensity = rule.density;
                }
                result.enabled |= rule.enabled;
                result.seaLevel = rule.minimumHeight - .75f;
                if( species.materials.size() > 1 )
                {
                    const auto &id = species.materials[1].albedo;
                    const auto slash = id.find_last_of( '/' );
                    if( id.substr( 0, 7 ) == "data://" && slash != String::npos && slash > 6 )
                    {
                        result.assetDirectory = id.substr( 7, slash - 7 );
                    }
                }
                found = true;
            }
            if( found )
            {
                settings = std::move( result );
            }
            return found;
        }
        catch( const std::exception & )
        {
            return false;
        }
    }
}  // namespace workphone::scene
