#include <WPLuabind/WPLuabindPCH.hpp>
#include <luabind/luabind.hpp>
#include <WPLuabind/StringConverter.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <Workphone/Workphone.hpp>

#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

#include <cmath>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <limits>

namespace workphone
{
    String terrainFoliageAuthoring( scene::TerrainSystem *terrain, const String &action,
                                    luabind::object options )
    {
        try
        {
            auto author = terrain->getFoliageAuthoring();
            String error;
            const auto number = [&]( const char *key, double fallback ) {
                luabind::object v = options[key];
                const auto value =
                    luabind::type( v ) == LUA_TNIL ? fallback : luabind::object_cast<double>( v );
                if( !std::isfinite( value ) || std::abs( value ) > std::numeric_limits<f32>::max() )
                {
                    throw std::runtime_error( "Foliage numbers must be finite." );
                }
                return value;
            };
            const auto integer = [&]( const char *key, u32 fallback, u32 maximum ) {
                const auto v = number( key, fallback );
                if( v < 0 || v > maximum || std::floor( v ) != v )
                {
                    throw std::runtime_error( "Foliage integer outside bounds." );
                }
                return u32( v );
            };
            const auto boolean = [&]( const char *key, bool fallback ) {
                luabind::object v = options[key];
                return luabind::type( v ) == LUA_TNIL ? fallback : luabind::object_cast<bool>( v );
            };
            const auto text = [&]( const char *key, const String &fallback = String() ) {
                luabind::object v = options[key];
                return luabind::type( v ) == LUA_TNIL ? fallback : luabind::object_cast<String>( v );
            };
            const auto points = [&]( const char *key ) {
                Array<Vector2F> result;
                luabind::object list = options[key];
                if( luabind::type( list ) != LUA_TTABLE )
                {
                    return result;
                }
                for( u32 i = 1; i <= 257; ++i )
                {
                    luabind::object p = list[i];
                    if( luabind::type( p ) == LUA_TNIL )
                    {
                        break;
                    }
                    if( i > 256 )
                    {
                        throw std::runtime_error( "Foliage polygon exceeds 256 points." );
                    }
                    const auto x = luabind::object_cast<double>( p[1] ),
                               z = luabind::object_cast<double>( p[2] );
                    if( !std::isfinite( x ) || !std::isfinite( z ) || std::abs( x ) > 1e12 ||
                        std::abs( z ) > 1e12 )
                    {
                        throw std::runtime_error( "Invalid foliage polygon coordinate." );
                    }
                    result.push_back( { f32( x ), f32( z ) } );
                }
                return result;
            };
            const auto ids = [&] {
                Array<u64> result;
                luabind::object list = options["ids"];
                if( luabind::type( list ) != LUA_TTABLE )
                {
                    return result;
                }
                for( u32 i = 1; i <= 65537; ++i )
                {
                    luabind::object v = list[i];
                    if( luabind::type( v ) == LUA_TNIL )
                    {
                        break;
                    }
                    if( i > 65536 )
                    {
                        throw std::runtime_error( "Foliage selection exceeds 65536 IDs." );
                    }
                    auto value = luabind::object_cast<std::string>( v );
                    if( value.empty() || value.size() > 20 ||
                        value.find_first_not_of( "0123456789" ) != std::string::npos )
                    {
                        throw std::runtime_error( "Foliage IDs must be decimal strings." );
                    }
                    result.push_back( std::stoull( value ) );
                }
                return result;
            };
            if( action == "foliageForceLod" )
            {
                auto renderer = dynamic_pointer_cast<render::Terrain>( terrain->getTerrain() );
                if( !renderer )
                {
                    return "Foliage LOD forcing requires an attached native terrain renderer.";
                }
                const auto level = number( "level", -1 );
                if( level < -1 || level > 2 || std::floor( level ) != level )
                {
                    return "Foliage forced LOD must be -1 or 0..2.";
                }
                return renderer->setFoliageForcedLod( s32( level ), error ) ? String() : error;
            }
            if( action == "foliageStatus" )
            {
                const auto snapshot = author->getSnapshot();
                options["instanceCount"] = double( snapshot->instances.size() );
                options["speciesCount"] = double( snapshot->species.size() );
                options["revision"] = double( snapshot->revision );
                options["strokeActive"] = author->isStrokeActive();
                const auto recipe = author->getRecipe();
                options["seed"] = recipe.seed;
                options["candidateSpacing"] = recipe.candidateSpacing;
                options["pageSize"] = recipe.pageSize;
                auto rules = luabind::newtable( options.interpreter() );
                for( u32 i = 0; i < recipe.rules.size(); ++i )
                {
                    const auto &r = recipe.rules[i];
                    auto o = luabind::newtable( options.interpreter() );
#define FIELD( name ) o[#name] = r.name
                    FIELD( species );
                    FIELD( enabled );
                    FIELD( density );
                    FIELD( weight );
                    FIELD( minimumSpacing );
                    FIELD( minimumHeight );
                    FIELD( maximumHeight );
                    FIELD( minimumSlope );
                    FIELD( maximumSlope );
                    FIELD( minimumScale );
                    FIELD( maximumScale );
                    FIELD( randomYaw );
                    FIELD( alignToNormal );
                    FIELD( allowedLayers );
                    FIELD( minimumLayerWeight );
                    FIELD( minimumMoisture );
                    FIELD( maximumMoisture );
#undef FIELD
                    const auto &species = *snapshot->species[r.species];
                    o["id"] = species.id;
                    o["kind"] = u32( species.kind );
                    o["castShadows"] = species.castShadows;
                    o["wind"] = species.wind.amplitude > 0;
                    o["windAmplitude"] = species.wind.amplitude;
                    o["collisionRadius"] = species.collisionRadius;
                    o["collision"] = species.collisionRadius > 0;
                    rules[i + 1] = o;
                }
                options["rules"] = rules;
                const auto active = terrain->getActiveTerrainSnapshot();
                options["pending"] = active != terrain->getTerrainSnapshot();
                if( auto renderer = dynamic_pointer_cast<render::Terrain>( terrain->getTerrain() ) )
                {
                    const auto d = renderer->getFoliageDiagnostics();
                    options["rendererAvailable"] = true;
                    options["ready"] = d.currentGenerationReady;
                    options["error"] = d.error;
                    options["forcedLod"] = renderer->getFoliageForcedLod();
#define STAT( name ) options[#name] = double( d.name )
                    STAT( residentPages );
                    STAT( pendingPages );
                    STAT( failedPages );
                    STAT( residentInstances );
                    STAT( testedInstances );
                    STAT( culledInstances );
                    STAT( groups );
                    STAT( predictedDraws );
                    STAT( actualDraws );
                    STAT( submittedInstances );
                    STAT( geometryBytes );
                    STAT( uploadBytes );
#undef STAT
                    options["nearInstances"] = double( d.lodInstances[0] );
                    options["midInstances"] = double( d.lodInstances[1] );
                    options["farInstances"] = double( d.lodInstances[2] );
                }
                else
                {
                    options["rendererAvailable"] = false;
                    options["error"] = "Native foliage renderer unavailable.";
                }
                return {};
            }
            if( action == "foliageSave" )
            {
                return author->save( text( "path" ), error ) ? String() : error;
            }
            if( action == "foliageExport" )
            {
                String source;
                if( !author->serialize( source, error ) )
                {
                    return error;
                }
                options["source"] = source;
                return {};
            }
            if( action == "foliageSelect" )
            {
                auto selection = author->select( points( "points" ) );
                auto values = luabind::newtable( options.interpreter() );
                for( u32 i = 0; i < selection.size(); ++i )
                {
                    values[i + 1] = std::to_string( selection[i] );
                }
                options["ids"] = values;
                return {};
            }
            if( action == "foliageBegin" )
            {
                return author->beginStroke( error ) ? String() : error;
            }
            bool success = false;
            if( action == "foliageCommit" )
            {
                success = author->commitStroke( error );
            }
            else if( action == "foliageCancel" )
            {
                success = author->cancelStroke( error );
            }
            else if( action == "foliageUndo" )
            {
                success = author->undo( error );
            }
            else if( action == "foliageRedo" )
            {
                success = author->redo( error );
            }
            else if( action == "foliageLoad" )
            {
                success = author->load( text( "path" ), error );
            }
            else if( action == "foliageImport" )
            {
                success = author->deserialize( text( "source" ), error );
            }
            else if( action == "foliageRegenerate" )
            {
                auto recipe = author->getRecipe();
                recipe.seed = integer( "seed", recipe.seed, 0xffffffffu );
                const bool ownStroke = !author->isStrokeActive();
                if( ownStroke && !author->beginStroke( error ) )
                {
                    return error;
                }
                success = author->configure( recipe, error ) &&
                          author->regenerateAll( terrain->getTerrainSnapshot(), error );
                if( ownStroke )
                {
                    String ignored;
                    if( success )
                    {
                        success = author->commitStroke( error );
                    }
                    else
                    {
                        author->cancelStroke( ignored );
                    }
                }
            }
            else if( action == "foliageBakeImpostors" )
            {
                const auto index = integer( "species", 0, 63 );
                auto species = author->getSnapshot()->species;
                if( index >= species.size() )
                {
                    return "Select an existing foliage species to bake.";
                }
                if( species[index]->kind == render::FoliageKind::Grass ||
                    species[index]->kind == render::FoliageKind::GroundCover )
                {
                    return "Grass uses clump LODs and density; select a tree or bush for impostor "
                           "baking.";
                }
                render::FoliageImpostorBakeOptions settings;
                settings.tileSize = integer( "tileSize", 128, 512 );
                settings.gutter = integer( "gutter", 4, 16 );
                settings.resourcePrefix = text( "resourcePrefix" );
                resource::ResourceID atlasId;
                if( !atlasId.set( settings.resourcePrefix + "_albedo.texres", &error ) ||
                    atlasId.isSubResource() ||
                    atlasId.str() != settings.resourcePrefix + "_albedo.texres" )
                {
                    return "Impostor resource prefix must be a canonical data:// file prefix: " + error;
                }
                auto app = core::IApplicationManager::instancePtr();
                String projectRoot;
                if( !app || app->getProjectPath().empty() ||
                    !canonicalAssetCatalogRoot( app->getProjectPath(), projectRoot, error ) )
                {
                    return "Impostor source export requires an open project with a valid source root.";
                }
                const auto expected =
                    std::filesystem::u8path( projectRoot.c_str() ) /
                    std::filesystem::u8path( atlasId.sourceRelativePath().c_str() ).parent_path();
                const auto directory = text( "directory", expected.u8string().c_str() );
                const String destination =
                    directory.empty() ? String( expected.u8string().c_str() ) : directory;
                AssetCatalogPath actualPath, expectedPath;
                if( !canonicalAssetCatalogPath( projectRoot, destination, actualPath, error ) ||
                    !canonicalAssetCatalogPath( projectRoot, expected.u8string().c_str(), expectedPath,
                                                error ) ||
                    actualPath.key != expectedPath.key )
                {
                    return "Impostor export directory must match the resource prefix under the project "
                           "root: " +
                           String( expected.u8string().c_str() );
                }
                if( std::filesystem::exists( expected ) )
                {
                    return "Impostor export requires a fresh resource-prefix directory: " +
                           String( expected.u8string().c_str() );
                }
                options["directory"] = expected.u8string().c_str();
                const auto views = integer( "views", 8, 32 );
                if( !views )
                {
                    return "Impostor baking requires one or more views.";
                }
                settings.directions.clear();
                for( u32 i = 0; i < views; ++i )
                {
                    const auto angle = double( i ) * 6.283185307179586 / views;
                    settings.directions.push_back(
                        { f32( std::sin( angle ) ), 0, f32( std::cos( angle ) ) } );
                }
                render::FoliageImpostorBake baked;
                if( !render::bakeFoliageImpostor( *species[index],
                                                  render::getFoliageImpostorTextureResolver(), settings,
                                                  baked, error ) )
                {
                    return error;
                }
                String path;
                if( !render::saveFoliageImpostorSource( *species[index], baked,
                                                        expected.u8string().c_str(), path, error ) )
                {
                    return error;
                }
                auto updated = std::make_shared<render::FoliageSpecies>( *species[index] );
                updated->impostor = baked.metadata;
                updated->farBillboard = true;
                species[index] = updated;
                success = author->setSpecies( std::move( species ), error );
                options["speciesPath"] = path;
            }
            else if( action == "foliagePaint" )
            {
                success = author->paint( terrain->getTerrainSnapshot(), integer( "species", 0, 63 ),
                                         { f32( number( "x", 0 ) ), f32( number( "z", 0 ) ) },
                                         f32( number( "radius", 5 ) ), integer( "count", 1, 4096 ),
                                         integer( "seed", 1, 0xffffffffu ), error );
            }
            else if( action == "foliageErase" )
            {
                success = author->erase( { f32( number( "x", 0 ) ), f32( number( "z", 0 ) ) },
                                         f32( number( "radius", 5 ) ), error );
            }
            else if( action == "foliageTransform" )
            {
                success = author->transformSelection(
                    ids(),
                    { f32( number( "dx", 0 ) ), f32( number( "dy", 0 ) ), f32( number( "dz", 0 ) ) },
                    f32( number( "yaw", 0 ) ), f32( number( "scale", 1 ) ), error );
            }
            else if( action == "foliageReplace" )
            {
                success = author->replaceSpecies( ids(), integer( "species", 0, 63 ), error );
            }
            else if( action == "foliageFlags" )
            {
                success = author->setInstanceFlags( ids(), integer( "flags", render::FoliageLocked, 15 ),
                                                    boolean( "enabled", true ), error );
            }
            else if( action == "foliagePlant" || action == "foliageImportMesh" )
            {
                scene::FoliagePlantRecipe recipe;
                recipe.speciesId = text( "id", "generated-tree" );
                recipe.kind = render::FoliageKind( integer( "kind", 0, 3 ) );
                recipe.seed = integer( "seed", 1, 0xffffffffu );
                recipe.height =
                    f32( number( "height", recipe.kind == render::FoliageKind::Grass ? .7 : 5 ) );
                recipe.radius =
                    f32( number( "radius", recipe.kind == render::FoliageKind::Grass ? .4 : 1.5 ) );
                recipe.trunkRadius = f32( number( "trunkRadius", .05 ) );
                recipe.branches = integer( "branches", 12, 128 );
                recipe.bark.albedo = text( "bark" );
                recipe.leaves.albedo = text( "albedo" );
                recipe.leaves.normal = text( "normal" );
                recipe.leaves.orm = text( "orm" );
                recipe.wind.amplitude = f32( number( "wind", .05 ) );
                recipe.wind.bendEnd = recipe.height;
                recipe.bark.tint = { .3f, .18f, .07f, 1 };
                recipe.leaves.tint = { .2f, .45f, .15f, 1 };
                render::FoliageSpecies species;
                if( action == "foliagePlant" )
                {
                    success = scene::generateFoliagePlant( recipe, species, error );
                }
                else
                {
                    SmartPtr<IMesh> mesh;
                    luabind::object supplied = options["mesh"];
                    if( luabind::type( supplied ) != LUA_TNIL )
                    {
                        mesh = luabind::object_cast<SmartPtr<IMesh>>( supplied );
                    }
                    else
                    {
                        auto app = core::IApplicationManager::instancePtr();
                        auto manager = app ? app->getMeshManager() : nullptr;
                        auto resource = manager ? dynamic_pointer_cast<IMeshResource>(
                                                      manager->loadResource( text( "path" ) ) )
                                                : nullptr;
                        if( resource )
                        {
                            mesh = resource->getMesh();
                        }
                    }
                    success = scene::importFoliageMesh( mesh, recipe.speciesId, { recipe.leaves },
                                                        species, error );
                }
                if( success )
                {
                    auto all = author->getSnapshot()->species;
                    auto index = integer( "species", u32( all.size() ), 64 );
                    if( index > all.size() )
                    {
                        return "Species slots must be appended or replaced contiguously.";
                    }
                    if( index == all.size() )
                    {
                        all.push_back(
                            std::make_shared<const render::FoliageSpecies>( std::move( species ) ) );
                    }
                    else
                    {
                        all[index] =
                            std::make_shared<const render::FoliageSpecies>( std::move( species ) );
                    }
                    const bool ownStroke = !author->isStrokeActive();
                    if( ownStroke && !author->beginStroke( error ) )
                    {
                        return error;
                    }
                    success = author->setSpecies( std::move( all ), error );
                    auto placement = author->getRecipe();
                    bool found = false;
                    for( const auto &r : placement.rules )
                    {
                        found |= r.species == index;
                    }
                    if( success && !found )
                    {
                        scene::FoliagePlacementRule rule;
                        rule.species = index;
                        placement.rules.push_back( rule );
                        success = author->configure( placement, error );
                    }
                    if( ownStroke )
                    {
                        String ignored;
                        if( success )
                        {
                            success = author->commitStroke( error );
                        }
                        else
                        {
                            author->cancelStroke( ignored );
                        }
                    }
                    options["species"] = index;
                }
            }
            else if( action == "foliageRule" || action == "foliageExclusion" ||
                     action == "foliageMasks" )
            {
                auto recipe = author->getRecipe();
                recipe.seed = integer( "seed", recipe.seed, 0xffffffffu );
                recipe.candidateSpacing = f32( number( "candidateSpacing", recipe.candidateSpacing ) );
                recipe.pageSize = f32( number( "pageSize", recipe.pageSize ) );
                if( action == "foliageRule" )
                {
                    auto index = integer( "rule", 0, 63 );
                    if( index >= recipe.rules.size() )
                    {
                        return "Generate or import a species before editing its placement rule.";
                    }
                    auto &r = recipe.rules[index];
#define FIELD( name ) r.name = f32( number( #name, r.name ) )
                    FIELD( density );
                    FIELD( weight );
                    FIELD( minimumSpacing );
                    FIELD( minimumHeight );
                    FIELD( maximumHeight );
                    FIELD( minimumSlope );
                    FIELD( maximumSlope );
                    FIELD( minimumScale );
                    FIELD( maximumScale );
                    FIELD( heightOffset );
                    FIELD( minimumLayerWeight );
                    FIELD( minimumMoisture );
                    FIELD( maximumMoisture );
#undef FIELD
                    r.enabled = boolean( "enabled", r.enabled );
                    r.randomYaw = boolean( "randomYaw", r.randomYaw );
                    r.alignToNormal = boolean( "alignToNormal", r.alignToNormal );
                    r.allowedLayers = integer( "allowedLayers", r.allowedLayers, 15 );
                    auto all = author->getSnapshot()->species;
                    auto species = std::make_shared<render::FoliageSpecies>( *all[r.species] );
                    species->castShadows = boolean( "castShadows", species->castShadows );
                    if( luabind::type( options["wind"] ) != LUA_TNIL )
                    {
                        species->wind.amplitude = boolean( "wind", false ) ? .1f : 0;
                    }
                    if( luabind::type( options["collision"] ) != LUA_TNIL )
                    {
                        species->collisionRadius = boolean( "collision", false ) ? .2f : 0;
                        species->collisionHeight =
                            species->collisionRadius > 0
                                ? species->boundsMaximum.y - species->boundsMinimum.y
                                : 0;
                    }
                    luabind::object albedo = options["albedo"];
                    if( luabind::type( albedo ) != LUA_TNIL )
                    {
                        for( auto &material : species->materials )
                        {
                            material.albedo = text( "albedo" );
                        }
                    }
                    all[r.species] = species;
                    const bool ownStroke = !author->isStrokeActive();
                    if( ownStroke && !author->beginStroke( error ) )
                    {
                        return error;
                    }
                    success = author->setSpecies( std::move( all ), error ) &&
                              author->configure( recipe, error ) &&
                              author->regenerateAll( terrain->getTerrainSnapshot(), error );
                    if( ownStroke )
                    {
                        String ignored;
                        if( success )
                        {
                            success = author->commitStroke( error );
                        }
                        else
                        {
                            author->cancelStroke( ignored );
                        }
                    }
                }
                else
                {
                    if( action == "foliageExclusion" )
                    {
                        if( boolean( "clear", false ) )
                        {
                            recipe.exclusions.clear();
                        }
                        else
                        {
                            recipe.exclusions.push_back(
                                { points( "points" ), f32( number( "radius", 0 ) ) } );
                        }
                    }
                    else
                    {
                        recipe.maskDimensions = { s32( integer( "width", 0, 65536 ) ),
                                                  s32( integer( "height", 0, 65536 ) ) };
                        recipe.densityMask.clear();
                        recipe.biomeMask.clear();
                        const u64 count = u64( recipe.maskDimensions.x ) * recipe.maskDimensions.y;
                        if( count > 65536 )
                        {
                            return "Foliage masks exceed 65536 samples.";
                        }
                        luabind::object density = options["density"], biome = options["biome"];
                        for( u32 i = 1; i <= count; ++i )
                        {
                            if( luabind::type( density ) == LUA_TTABLE )
                            {
                                recipe.densityMask.push_back( luabind::object_cast<f32>( density[i] ) );
                            }
                            if( luabind::type( biome ) == LUA_TTABLE )
                            {
                                const auto value = luabind::object_cast<double>( biome[i] );
                                if( value < 0 || value > 63 || std::floor( value ) != value )
                                {
                                    return "Invalid foliage biome index.";
                                }
                                recipe.biomeMask.push_back( u32( value ) );
                            }
                        }
                    }
                    const bool ownStroke = !author->isStrokeActive();
                    if( ownStroke && !author->beginStroke( error ) )
                    {
                        return error;
                    }
                    success = author->configure( recipe, error ) &&
                              author->regenerateAll( terrain->getTerrainSnapshot(), error );
                    if( ownStroke )
                    {
                        String ignored;
                        if( success )
                        {
                            success = author->commitStroke( error );
                        }
                        else
                        {
                            author->cancelStroke( ignored );
                        }
                    }
                }
            }
            else
            {
                return "Unknown foliage operation.";
            }
            if( !success )
            {
                return error;
            }
            // Authored state is retained if rendering is temporarily unavailable; report it
            // explicitly and retry at the next coherent terrain publication boundary.
            if( !terrain->refreshFoliage( error ) )
            {
                options["pending"] = true;
                options["error"] = error;
            }
            return {};
        }
        catch( const std::exception &e )
        {
            return e.what();
        }
    }

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
        {
            return "No terrain selected.";
        }
        if( action.find( "foliage" ) == 0 )
        {
            return terrainFoliageAuthoring( terrain, action, options );
        }
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
                {
                    throw std::runtime_error(
                        "Terrain integer setting is outside its supported range." );
                }
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
                {
                    for( auto collision : actor->getComponentsByType<scene::CollisionTerrain>() )
                    {
                        if( !collision->getTerrainError().empty() )
                        {
                            options["error"] = collision->getTerrainError();
                        }
                    }
                }
                return {};
            }
            if( action == "collision" )
            {
                auto actor = terrain->getActor();
                if( !actor )
                {
                    return "Collision publication requires a terrain actor with CollisionTerrain.";
                }
                auto collisions = actor->getComponentsByType<scene::CollisionTerrain>();
                if( collisions.empty() )
                {
                    return "Attach CollisionTerrain and a supported physics body before building "
                           "collision.";
                }
                terrain->updateTransform();
                auto active = terrain->getActiveTerrainSnapshot();
                for( auto collision : collisions )
                {
                    if( !collision->getTerrainError().empty() )
                    {
                        return collision->getTerrainError();
                    }
                    if( !collision->isValid() || collision->getTerrainSnapshot() != active )
                    {
                        return "Collision is waiting for the matching render/source revision to "
                               "publish.";
                    }
                }
                return {};
            }
            if( action == "rebuildLod" )
            {
                if( !terrain->getTerrain() )
                {
                    return "Terrain LOD requires an attached renderer.";
                }
                terrain->rebuild();
                return {};
            }
            SmartPtr<scene::TerrainRecipeStack> recipeStack;
            luabind::object recipeValue = options["recipe"];
            if( luabind::type( recipeValue ) != LUA_TNIL )
            {
                recipeStack = luabind::object_cast<SmartPtr<scene::TerrainRecipeStack>>( recipeValue );
                if( !recipeStack )
                {
                    return "Terrain recipe reference is invalid.";
                }
            }
            if( action.find( "recipe" ) == 0 )
            {
                if( !recipeStack )
                {
                    return "Create a bounded terrain recipe first.";
                }
                if( action == "recipePreview" )
                {
                    recipeStack->requestPreview( error );
                }
                else if( action == "recipePoll" )
                {
                    recipeStack->poll( error );
                }
                else if( action == "recipeCommit" )
                {
                    recipeStack->commit( terrain, commands, error );
                }
                else if( action == "recipeCancel" )
                {
                    recipeStack->cancel();
                }
                else if( action == "recipeReset" )
                {
                    recipeStack->setBase( terrain, error );
                }
                else if( action == "recipeSave" )
                {
                    recipeStack->save( text( "path" ), error );
                }
                else if( action == "recipeLoad" )
                {
                    recipeStack->load( text( "path" ), terrain, error );
                }
                else if( action == "recipeRemove" )
                {
                    recipeStack->remove( boundedInteger( "index", 0, 31 ), error );
                }
                else if( action == "recipeMove" )
                {
                    recipeStack->move( boundedInteger( "index", 0, 31 ), boundedInteger( "to", 0, 31 ),
                                       error );
                }
                else if( action == "recipeEnable" )
                {
                    recipeStack->setEnabled( boundedInteger( "index", 0, 31 ),
                                             number( "enabled", 1 ) != 0, error );
                }
                else
                {
                    error = "Unknown terrain recipe operation.";
                }
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
                {
                    format.encoding = scene::TerrainHeightEncoding::RawU16LE;
                }
                else if( encoding == "raw32" )
                {
                    format.encoding = scene::TerrainHeightEncoding::RawF32LE;
                }
                else if( encoding != "pgm16" )
                {
                    return "Height encoding must be pgm16, raw16 or raw32.";
                }
                format.dimensions =
                    Vector2I( boundedInteger( "width", 0, render::terrainMaximumDimension ),
                              boundedInteger( "depth", 0, render::terrainMaximumDimension ) );
                format.minimumHeight = f32( number( "minimum", 0 ) );
                format.maximumHeight = f32( number( "maximum", 1 ) );
                if( action == "importHeight" )
                {
                    scene::importTerrainHeightFile( terrain, text( "path" ), format, error );
                }
                else
                {
                    scene::exportTerrainHeightFile( terrain, text( "path" ), format, error );
                }
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
                {
                    scene::applyTerrainErosion( terrain, recipe, commands, error );
                }
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
                {
                    return "Set a centreline file containing local x y z rows.";
                }
                std::ifstream file( std::filesystem::u8path( path.c_str() ),
                                    std::ios::binary | std::ios::ate );
                if( !file || file.tellg() > std::streamoff( 128 * 1024 ) )
                {
                    return "Centreline file is missing or exceeds 128 KiB.";
                }
                file.seekg( 0 );
                std::string line;
                while( std::getline( file, line ) )
                {
                    if( line.empty() || line[0] == '#' )
                    {
                        continue;
                    }
                    std::istringstream row( line );
                    row.imbue( std::locale::classic() );
                    Vector3F point;
                    std::string extra;
                    if( !( row >> point.x >> point.y >> point.z ) || row >> extra )
                    {
                        return "Centreline rows require exactly three finite coordinates.";
                    }
                    if( recipe.points.size() >= 1024 )
                    {
                        return "Centreline exceeds 1024 points.";
                    }
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
                {
                    scene::applyTerrainGrade( terrain, recipe, commands, error );
                }
                return error;
            }
            if( action == "pick" || action == "pickHeightfield" )
            {
                Transform3<real_Num> placement;
                if( auto actor = terrain->getActor() )
                {
                    if( auto transform = actor->getTransform() )
                    {
                        placement = transform->getWorldTransform();
                    }
                }
                const Vector3F origin( f32( number( "rayX", 0 ) ), f32( number( "rayY", 0 ) ),
                                       f32( number( "rayZ", 0 ) ) );
                const Vector3F direction( f32( number( "dirX", 0 ) ), f32( number( "dirY", -1 ) ),
                                          f32( number( "dirZ", 0 ) ) );
                Vector3<real_Num> hit;
                const auto source = terrain->getTerrainSnapshot();
                const bool picked = action == "pickHeightfield"
                    ? render::intersectTerrainHeightfieldForAuthoring(
                          *source, placement, Ray3F( origin, direction ), hit )
                    : render::intersectTerrain( *source, placement, Ray3F( origin, direction ), hit );
                if( !picked )
                {
                    return action == "pickHeightfield" ? "Brush ray does not hit the authored heightfield."
                                                       : "Brush ray does not hit solid terrain.";
                }
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
            {
                brush.mode = scene::TerrainBrushMode::Raise;
            }
            else if( mode == "lower" )
            {
                brush.mode = scene::TerrainBrushMode::Lower;
            }
            else if( mode == "smooth" )
            {
                brush.mode = scene::TerrainBrushMode::Smooth;
            }
            else if( mode == "flatten" )
            {
                brush.mode = scene::TerrainBrushMode::Flatten;
            }
            else if( mode == "stamp" )
            {
                brush.mode = scene::TerrainBrushMode::Stamp;
            }
            else if( mode == "paint" )
            {
                brush.mode = scene::TerrainBrushMode::Paint;
            }
            else if( mode == "clearPaint" )
            {
                brush.mode = scene::TerrainBrushMode::ClearPaint;
            }
            else if( mode == "normalize" )
            {
                brush.mode = scene::TerrainBrushMode::Normalize;
            }
            else if( mode == "hole" )
            {
                brush.mode = scene::TerrainBrushMode::Hole;
            }
            else if( mode == "fillHole" )
            {
                brush.mode = scene::TerrainBrushMode::FillHole;
            }
            else if( mode == "terrace" )
            {
                brush.mode = scene::TerrainBrushMode::Terrace;
            }
            else if( mode == "noise" )
            {
                brush.mode = scene::TerrainBrushMode::Noise;
            }
            else if( mode == "paintWetness" )
            {
                brush.mode = scene::TerrainBrushMode::PaintWetness;
            }
            else if( mode == "clearWetness" )
            {
                brush.mode = scene::TerrainBrushMode::ClearWetness;
            }
            else if( mode == "paintSnow" )
            {
                brush.mode = scene::TerrainBrushMode::PaintSnow;
            }
            else if( mode == "clearSnow" )
            {
                brush.mode = scene::TerrainBrushMode::ClearSnow;
            }
            else
            {
                return "Terrain operation is unavailable.";
            }
            const auto stampPath = text( "stampPath" );
            if( mode == "stamp" && !stampPath.empty() )
            {
                auto stamp = make_ptr<scene::TerrainSystem>();
                scene::TerrainHeightFileOptions format;
                if( !scene::importTerrainHeightFile( stamp, stampPath, format, error ) )
                {
                    return error;
                }
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
            {
                terrain->updateEditStroke( brush, error );
            }
            else
            {
                scene::applyTerrainBrush( terrain, brush, commands, error );
            }
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
