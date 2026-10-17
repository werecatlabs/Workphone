#ifndef WPTerrainAuthoringContracts_h__
#define WPTerrainAuthoringContracts_h__

#include <Workphone/Scene/Components/Terrain/TerrainEditing.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainRecipes.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainMasks.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/System/CommandManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstdio>

namespace terrain_authoring_contracts
{
    using namespace workphone;
    using namespace workphone::scene;
    inline void require( bool value, const String &message )
    {
        if( !value )
            throw std::runtime_error( message.c_str() );
    }
    inline bool ready( SmartPtr<TerrainRecipeStack> stack, String &error )
    {
        for( int i = 0; i < 500; ++i )
        {
            if( stack->poll( error ) )
                return true;
            if( !error.empty() || !stack->isBusy() )
                return false;
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }
        error = "Terrain recipe worker exceeded the fixture deadline.";
        return false;
    }
    inline void appearanceContracts()
    {
        auto terrain = make_ptr<TerrainSystem>();
        auto commands = make_ptr<CommandManager>();
        render::TerrainData base;
        base.dimensions = { 3, 5 };
        base.origin = { -1, -2 };
        base.heights.assign( 15, 0 );
        base.heightScale = 0;  // Weather painting must work on a flat zero-scale terrain.
        String error;
        require( terrain->applyTerrainData( base, error ), error );
        TerrainBrush brush;
        brush.radius = .6f;
        brush.strength = .5f;
        brush.mode = TerrainBrushMode::ClearWetness;
        {
            TerrainEditStroke stroke( terrain, commands );
            require( stroke.update( brush, error ), error );
            require( terrain->getTerrainSnapshot()->surfaceMasks[7] == Vector2F( .5f, 1 ),
                     "Wetness erasing changes only its channel with bounded blend strength." );
            brush.mode = TerrainBrushMode::ClearSnow;
            require( stroke.update( brush, error ), error );
            const auto dirty = stroke.getDirtyRect();
            require( dirty.minX == 1 && dirty.maxX == 1 && dirty.minZ == 2 && dirty.maxZ == 2,
                     "Weather strokes report exactly their edited sample rectangle." );
            require( stroke.cancel( error ), error );
            require(
                terrain->getTerrainSnapshot()->surfaceMasks.empty() && !commands->getPreviousCommand(),
                "Cancelled weather strokes restore implicit masks and leave no history." );
        }
        require( applyTerrainBrush( terrain, brush, commands, error ), error );
        auto changed = *terrain->getTerrainSnapshot();
        changed.appearance.wetness = .4f;
        changed.surfaceMasks[0] = Vector2F( .2f, .3f );
        require( terrain->applyTerrainData( changed, error ), error );
        auto command = commands->getPreviousCommand();
        command->undo();
        require( terrain->getTerrainSnapshot()->surfaceMasks[7] == Vector2F( 1, 1 ) &&
                     terrain->getTerrainSnapshot()->surfaceMasks[0] == Vector2F( .2f, .3f ) &&
                     terrain->getTerrainSnapshot()->appearance.wetness == .4f,
                 "Mask undo preserves independent material and outside-sample edits." );
        command->redo();
        require( terrain->getTerrainSnapshot()->surfaceMasks[7] == Vector2F( 1, .5f ),
                 "Mask redo restores the authored snow channel." );
        commands->clearAll();
        auto appearance = terrain->getTerrainSnapshot()->appearance;
        appearance.projection = render::TerrainProjection::Triplanar;
        appearance.heightBlend = true;
        appearance.macroSeed = 773;
        appearance.macroStrength = .25f;
        appearance.snow = .6f;
        require( applyTerrainAppearance( terrain, appearance, commands, error ), error );
        auto materialUndo = commands->getPreviousCommand();
        materialUndo->undo();
        require( terrain->getTerrainSnapshot()->appearance == changed.appearance,
                 "Global material parameters form one undoable edit." );
        materialUndo->redo();
        render::TerrainData reopened;
        require( render::deserializeTerrainData( terrain->exportTerrainData(), reopened, error ),
                 error );
        require( reopened.appearance == appearance &&
                     reopened.surfaceMasks == terrain->getTerrainSnapshot()->surfaceMasks,
                 "Appearance and both weather masks survive source reopening." );
        auto invalid = appearance;
        invalid.wetness = 2;
        const auto beforeInvalid = terrain->getTerrainSnapshot();
        require( !applyTerrainAppearance( terrain, invalid, commands, error ) &&
                     terrain->getTerrainSnapshot() == beforeInvalid,
                 "Invalid appearance edits preserve snapshot identity." );
        auto layer = terrain->getTerrainSnapshot()->layers[1];
        layer.heightScale = 2;
        layer.heightBias = -.2f;
        require( applyTerrainLayer( terrain, 1, layer, commands, error ), error );
        commands->getPreviousCommand()->undo();
        require( terrain->getTerrainSnapshot()->layers[1] == base.layers[1],
                 "Per-layer ORM height parameters participate in undo." );
        commands->clearAll();
        require( terrain->applyTerrainData( base, error ), error );
        auto recipe = make_ptr<TerrainRecipeStack>();
        require( recipe->setBase( terrain, error ), error );
        TerrainRecipeOperation operation;
        operation.brush = brush;
        operation.brush.mode = TerrainBrushMode::ClearWetness;
        require( recipe->append( operation, error ), error );
        const auto bytes = recipe->serialize();
        auto current = *terrain->getTerrainSnapshot();
        current.appearance = appearance;
        current.surfaceMasks.assign( 15, Vector2F( 1, .25f ) );
        require( terrain->applyTerrainData( current, error ), error );
        require( recipe->deserialize( bytes, terrain, error ) && recipe->requestPreview( error ) &&
                     ready( recipe, error ),
                 error );
        require( recipe->commit( terrain, commands, error ), error );
        require( terrain->getTerrainSnapshot()->surfaceMasks[7] == Vector2F( .5f, .25f ) &&
                     terrain->getTerrainSnapshot()->surfaceMasks[0] == Vector2F( 1, .25f ) &&
                     terrain->getTerrainSnapshot()->appearance == appearance,
                 "Reloaded mask recipes preserve current appearance and untouched mask channels." );
        commands->getPreviousCommand()->undo();
        require( terrain->getTerrainSnapshot()->surfaceMasks == current.surfaceMasks &&
                     terrain->getTerrainSnapshot()->appearance == appearance,
                 "Recipe mask commit has one complete undo without changing material settings." );
        commands->clearAll();
    }
    inline void run( const std::filesystem::path &folder )
    {
        appearanceContracts();
        auto terrain = make_ptr<TerrainSystem>();
        auto commands = make_ptr<CommandManager>();
        String error;
        render::TerrainData base;
        base.dimensions = { 3, 5 };
        base.origin = { -1, -2 };
        base.heights.assign( 15, 0 );
        require( terrain->applyTerrainData( base, error ), error );
        const auto implicitSource = terrain->exportTerrainData();
        render::TerrainData implicitRoundTrip;
        require( !implicitSource.empty() &&
                     render::deserializeTerrainData( implicitSource, implicitRoundTrip, error ) &&
                     implicitRoundTrip.layerWeights.empty() && implicitRoundTrip.holes.empty() &&
                     implicitRoundTrip.heights == base.heights,
                 "Implicit default weights and solid cells serialize as empty arrays and reopen." );
        TerrainBrush brush;
        brush.radius = 0.6f;
        brush.strength = 2;
        TerrainEditStroke stroke( terrain, commands );
        require( stroke.update( brush, error ), error );
        brush.centre = { 1, 1 };
        require( stroke.update( brush, error ), error );
        require( !commands->getPreviousCommand(), "Preview updates must not each add history." );
        const auto dirty = stroke.getDirtyRect();
        require( dirty.minX == 1 && dirty.maxX == 2 && dirty.minZ == 2 && dirty.maxZ == 3,
                 "Stroke dirty rectangle must contain exact changed samples." );
        require( stroke.commit( error ), error );
        auto committed = terrain->getTerrainSnapshot();
        auto undo = commands->getPreviousCommand();
        require( undo != nullptr, "Stroke adds one undo command." );
        undo->undo();
        require(
            terrain->getTerrainSnapshot()->heights == base.heights && !commands->getPreviousCommand(),
            "One undo restores both stroke dabs." );
        auto redo = commands->getNextCommand();
        require( redo != nullptr, "Stroke redo exists." );
        redo->redo();
        require( terrain->getTerrainSnapshot()->heights == committed->heights,
                 "Stroke redo restores both dabs exactly." );
        require( terrain->applyTerrainData( base, error ), error );
        commands->clearAll();
        TerrainEditStroke cancelled( terrain, commands );
        brush.centre = { 0, 0 };
        brush.mode = TerrainBrushMode::Paint;
        brush.layer = 2;
        brush.strength = 0.75f;
        require( cancelled.update( brush, error ), error );
        auto painted = terrain->getTerrainSnapshot();
        require( painted->layerWeights[7] == Vector4F( 0.25f, 0, 0.75f, 0 ),
                 "Paint increases chosen layer and proportionally normalizes other channels." );
        brush.mode = TerrainBrushMode::Raise;
        brush.strength = 3;
        require( cancelled.update( brush, error ), error );
        require( cancelled.cancel( error ), error );
        require( terrain->exportTerrainData() == render::serializeTerrainData( base ) &&
                     !commands->getPreviousCommand(),
                 "Cancelled mixed stroke preserves exact samples/weights and adds no history." );
        brush.mode = TerrainBrushMode::Hole;
        brush.centre = { -0.5f, -1.5f };
        brush.radius = 0.4f;
        require( applyTerrainBrush( terrain, brush, commands, error ), error );
        auto holes = terrain->getTerrainSnapshot();
        require( holes->holes[0] == 1, "Hole paint addresses cell centre." );
        Transform3<real_Num> placement;
        f32 height;
        Vector3<real_Num> hit;
        require( !render::sampleTerrainHeight( *holes, placement, { -0.5, 0, -1.5 }, height ) &&
                     !render::intersectTerrain( *holes, placement,
                                                Ray3F( { -0.5f, 10, -1.5f }, { 0, -1, 0 } ), hit ),
                 "Height and ray queries cannot hit hole cells." );
        render::TerrainMeshData mesh;
        require( render::buildTerrainMeshData( *holes, mesh, error ), error );
        require( mesh.indices.size() == 42, "Hole omits exactly two mesh triangles." );
        auto restored = make_ptr<TerrainSystem>();
        require( restored->importTerrainData( terrain->exportTerrainData(), error ), error );
        require( restored->getTerrainSnapshot()->holes == holes->holes,
                 "Holes survive fresh component reopening." );
        const auto obj = ( folder / "terrain.obj" ).u8string();
        require( exportTerrainMeshFile( terrain, obj.c_str(), error ), error );
        std::ifstream objFile( folder / "terrain.obj" );
        std::string line;
        unsigned vertices = 0, faces = 0;
        while( std::getline( objFile, line ) )
        {
            if( line.rfind( "v ", 0 ) == 0 )
                ++vertices;
            if( line.rfind( "f ", 0 ) == 0 )
                ++faces;
        }
        require( vertices == 15 && faces == 14, "OBJ contains actual positions and hole-aware faces." );
        require(
            !exportTerrainMeshFile( terrain, ( folder / "fake.mesh.json" ).u8string().c_str(), error ),
            "Mesh export refuses recipe/image file extensions." );
        for( size_t i = 0; i < base.heights.size(); ++i )
            base.heights[i] = f32( i ) / 14;
        base.layers[2].albedo = "terrain/rock";
        base.layerWeights.assign( 15, Vector4F( 0.25f, 0, 0.75f, 0 ) );
        require( terrain->applyTerrainData( base, error ), error );
        require( restored->importTerrainData( terrain->exportTerrainData(), error ), error );
        require( restored->getTerrainSnapshot()->layers == base.layers &&
                     restored->getTerrainSnapshot()->layerWeights == base.layerWeights,
                 "All four material descriptors and painted weights roundtrip." );
        TerrainHeightFileOptions encoding;
        for( const auto mode : { TerrainHeightEncoding::Pgm16, TerrainHeightEncoding::RawU16LE,
                                 TerrainHeightEncoding::RawF32LE } )
        {
            encoding.encoding = mode;
            encoding.dimensions = { 3, 5 };
            const auto path = folder / ( mode == TerrainHeightEncoding::Pgm16      ? "height.pgm"
                                         : mode == TerrainHeightEncoding::RawU16LE ? "height.r16"
                                                                                   : "height.r32" );
            require( exportTerrainHeightFile( terrain, path.u8string().c_str(), encoding, error ),
                     error );
            auto imported = make_ptr<TerrainSystem>();
            require( importTerrainHeightFile( imported, path.u8string().c_str(), encoding, error ),
                     error );
            const auto data = imported->getTerrainSnapshot();
            require( data->dimensions == base.dimensions,
                     "Height image/raw preserves asymmetric dimensions." );
            for( size_t i = 0; i < base.heights.size(); ++i )
                require( std::abs( data->heights[i] - base.heights[i] ) <=
                             ( mode == TerrainHeightEncoding::RawF32LE ? 0 : 1.f / 65535 ),
                         "Height codec obeys float exactness or declared U16 quantization." );
        }
        encoding.encoding = TerrainHeightEncoding::Pgm16;
        encoding.maximumHeight = 0.5f;
        require( !exportTerrainHeightFile( terrain, ( folder / "clipped.pgm" ).u8string().c_str(),
                                           encoding, error ),
                 "U16 export rejects clipping instead of silently losing heights." );
        const auto beforeBad = terrain->getTerrainSnapshot();
        std::ofstream bad( folder / "bad.r32", std::ios::binary );
        bad << "bad";
        bad.close();
        encoding.encoding = TerrainHeightEncoding::RawF32LE;
        require( !importTerrainHeightFile( terrain, ( folder / "bad.r32" ).u8string().c_str(), encoding,
                                           error ) &&
                     terrain->getTerrainSnapshot() == beforeBad,
                 "Malformed raw import retains prior snapshot identity." );
        render::TerrainData legacy;
        require( render::deserializeTerrainData(
                     "{\"format\":\"workphone.terrain\",\"version\":1,\"width\":2,\"depth\":2,"
                     "\"heightScale\":1,\"spacing\":[1,1],\"origin\":[0,0],\"heights\":[0,1,2,3]}",
                     legacy, error ),
                 error );
        require( legacy.layerWeights.empty() && legacy.holes.empty(),
                 "Version-one migration defaults to solid layer-zero terrain." );

        base.dimensions = { 9, 7 };
        base.origin = { -4, -3 };
        base.heights.assign( 63, 0 );
        base.layerWeights.clear();
        base.holes.clear();
        base.heights[31] = 10;
        require( terrain->applyTerrainData( base, error ), error );
        commands->clearAll();
        TerrainErosionRecipe erosion;
        erosion.region.radius = 3;
        erosion.iterations = 8;
        render::TerrainData thermal, repeat, hydraulic;
        require( prepareTerrainErosion( base, erosion, thermal, nullptr, error ), error );
        require( prepareTerrainErosion( base, erosion, repeat, nullptr, error ), error );
        require(
            thermal.heights == repeat.heights && thermal.heights[31] < 10 && thermal.heights[0] == 0,
            "Thermal erosion is deterministic, smooths excess talus and preserves unselected samples." );
        double mass = 0;
        auto wholeDomain = erosion;
        wholeDomain.region.radius = 100;
        render::TerrainData conserved;
        require( prepareTerrainErosion( base, wholeDomain, conserved, nullptr, error ), error );
        for( auto value : conserved.heights )
            mass += value;
        require( std::abs( mass - 10 ) < 1.e-4, "Whole-domain thermal flux conserves total material." );
        erosion.mode = TerrainErosionMode::Hydraulic;
        erosion.rain = 0.3f;
        erosion.seed = 97;
        require( prepareTerrainErosion( base, erosion, hydraulic, nullptr, error ), error );
        require( hydraulic.heights != base.heights && hydraulic.heights[0] == 0,
                 "Hydraulic flow transports sediment within the selected region." );
        // Independently prepared adjacent write regions read their authoritative
        // halo, so both must match a whole-domain reference, including shared edges.
        render::TerrainData haloSource;
        haloSource.dimensions = { 33, 17 };
        haloSource.origin = { 0, 0 };
        haloSource.heights.resize( 33 * 17 );
        for( s32 z = 0; z < 17; ++z )
            for( s32 x = 0; x < 33; ++x )
                haloSource.heights[z * 33 + x] = f32( ( x * 13 + z * 7 ) % 11 ) + 0.2f * x;
        for( auto mode : { TerrainErosionMode::Thermal, TerrainErosionMode::Hydraulic } )
        {
            auto region = erosion;
            region.mode = mode;
            region.iterations = 3;
            region.region.centre = { 16, 8 };
            region.region.radius = 100;
            render::TerrainData reference;
            require( prepareTerrainErosion( haloSource, region, reference, nullptr, error ), error );
            for( f32 centreX : { 14.0f, 20.0f } )
            {
                region.region.centre.x = centreX;
                region.region.radius = 3.5f;
                render::TerrainData part;
                require( prepareTerrainErosion( haloSource, region, part, nullptr, error ), error );
                for( s32 z = 0; z < 17; ++z )
                    for( s32 x = 0; x < 33; ++x )
                    {
                        const auto selected = std::hypot( double( x ) - centreX, double( z ) - 8 ) < 3.5;
                        const auto index = size_t( z ) * 33 + x;
                        require( part.heights[index] ==
                                     ( selected ? reference.heights[index] : haloSource.heights[index] ),
                                 "Adjacent erosion regions match global simulation and preserve every "
                                 "unselected sample." );
                    }
            }
        }
        render::TerrainData erosionField;
        erosionField.dimensions = { 129, 129 };
        erosionField.origin = { 0, 0 };
        erosionField.heights.resize( 129 * 129 );
        for( s32 z = 0; z < 129; ++z )
            for( s32 x = 0; x < 129; ++x )
                erosionField.heights[z * 129 + x] = f32( 8 * std::sin( x * .13 ) * std::cos( z * .17 ) );
        for( auto mode : { TerrainErosionMode::Thermal, TerrainErosionMode::Hydraulic } )
        {
            auto measured = erosion;
            measured.mode = mode;
            measured.region.centre = { 64, 64 };
            measured.region.radius = 100;
            measured.iterations = 16;
            render::TerrainData result;
            const auto start = std::chrono::steady_clock::now();
            require( prepareTerrainErosion( erosionField, measured, result, nullptr, error ), error );
            const auto milliseconds =
                std::chrono::duration<double, std::milli>( std::chrono::steady_clock::now() - start )
                    .count();
            require(
                result.heights != erosionField.heights && render::validateTerrainData( result, error ),
                "Multi-tile erosion produces changed finite terrain at representative authoring "
                "scale." );
            std::printf(
                "Terrain %s erosion: 129x129, 16 iterations, seed 97, %.3f ms; "
                "simulation sample cap 262144 (13 MiB simulation-array ceiling)\n",
                mode == TerrainErosionMode::Thermal ? "thermal" : "hydraulic", milliseconds );
        }
        const auto maskSource = terrain->getTerrainSnapshot();
        const auto erosionMaskPath = ( folder / "erosion-mask.pgm" ).u8string();
        require( exportTerrainErosionMask( terrain, erosionMaskPath.c_str(), erosion, error ), error );
        auto maskTerrain = make_ptr<TerrainSystem>();
        TerrainHeightFileOptions maskFormat;
        require( importTerrainHeightFile( maskTerrain, erosionMaskPath.c_str(), maskFormat, error ),
                 error );
        require( *std::max_element( maskTerrain->getTerrainSnapshot()->heights.begin(),
                                    maskTerrain->getTerrainSnapshot()->heights.end() ) == 1 &&
                     terrain->getTerrainSnapshot() == maskSource,
                 "Erosion displacement mask decodes as normalized image without applying erosion." );
        auto rule = erosion.region;
        rule.minHeight = 9;
        rule.maxHeight = 11;
        const auto ruleMaskPath = ( folder / "rule-mask.pgm" ).u8string();
        require( exportTerrainRuleMask( terrain, ruleMaskPath.c_str(), rule, error ), error );
        require( importTerrainHeightFile( maskTerrain, ruleMaskPath.c_str(), maskFormat, error ),
                 error );
        require( maskTerrain->getTerrainSnapshot()->heights[31] == 1 &&
                     maskTerrain->getTerrainSnapshot()->heights[30] == 0,
                 "Height/slope rule mask encodes the selected terrain region." );
        std::atomic_bool stop{ true };
        auto rejected = base;
        require( !prepareTerrainErosion( base, erosion, rejected, &stop, error ) &&
                     rejected.heights == base.heights,
                 "Cancelled erosion never publishes partial candidate output." );
        auto oversizedHalo = base;
        oversizedHalo.dimensions = { 513, 513 };
        oversizedHalo.origin = { 0, 0 };
        oversizedHalo.heights.assign( 513 * 513, 0 );
        auto longErosion = erosion;
        longErosion.region.centre = { 256, 256 };
        longErosion.region.radius = 0.6f;
        longErosion.iterations = 1024;
        require( !prepareTerrainErosion( oversizedHalo, longErosion, rejected, nullptr, error ) &&
                     rejected.heights == base.heights,
                 "Oversized erosion read halos fail before replacing caller output." );
        TerrainGradeRecipe road;
        road.points = { { -3, 2, 0 }, { 3, 4, 0 } };
        road.width = 2;
        road.shoulder = 1;
        require( applyTerrainGrade( terrain, road, commands, error ), error );
        require( terrain->getTerrainSnapshot()->heights[31] == 3 &&
                     terrain->getTerrainSnapshot()->heights[0] == 0,
                 "Road grading interpolates profile elevation and preserves outside manual data." );
        commands->getPreviousCommand()->undo();
        require( terrain->getTerrainSnapshot()->heights == base.heights,
                 "Grading undo restores exact prior sculpt." );
        require( terrain->applyTerrainData( base, error ), error );
        commands->clearAll();
        auto recipe = make_ptr<TerrainRecipeStack>();
        require( recipe->setBase( terrain, error ), error );
        TerrainRecipeOperation raise;
        raise.brush.radius = 0.6f;
        raise.brush.strength = 2;
        TerrainRecipeOperation flatten = raise;
        flatten.brush.mode = TerrainBrushMode::Flatten;
        flatten.brush.strength = 1;
        flatten.brush.targetHeight = 4;
        require( recipe->append( raise, error ) && recipe->append( flatten, error ), error );
        require( recipe->serialize().find( "\"stamp\":[]" ) != String::npos,
                 "Non-stamp recipe operations retain an explicit empty stamp array." );
        require( recipe->requestPreview( error ) && ready( recipe, error ), error );
        require(
            recipe->getPreview()->heights[31] == 4 && terrain->getTerrainSnapshot()->heights[31] == 10,
            "Recipe preparation is ordered and non-destructive until commit." );
        require(
            recipe->move( 1, 0, error ) && recipe->requestPreview( error ) && ready( recipe, error ),
            error );
        require( recipe->getPreview()->heights[31] == 6,
                 "Changing operation order changes deterministic result." );
        const auto recipePath = ( folder / "ordered.terrain.recipe.json" ).u8string();
        require( recipe->save( recipePath.c_str(), error ), error );
        auto reopened = make_ptr<TerrainRecipeStack>();
        require( reopened->load( recipePath.c_str(), terrain, error ), error );
        require( reopened->requestPreview( error ) && ready( reopened, error ), error );
        require( reopened->getPreview()->heights == recipe->getPreview()->heights,
                 "Ordered recipe/base survive save and fresh runtime reopening." );
        require( reopened->getPreviewFor( terrain ) == reopened->getPreview() &&
                     !reopened->getPreviewFor( restored ),
                 "Visual recipe preview is only available for its captured owner." );
        require( reopened->commit( terrain, commands, error ), error );
        require( terrain->getTerrainSnapshot()->heights[31] == 6,
                 "Prepared recipe commits through terrain command service." );
        commands->getPreviousCommand()->undo();
        require( terrain->getTerrainSnapshot()->heights == base.heights,
                 "Recipe commit is one complete undo item." );
        require( !reopened->commit( terrain, commands, error ),
                 "Manual/history revision changes reject stale recipe publication." );
        require( !reopened->getPreviewFor( terrain ),
                 "Manual/history revision changes hide stale viewport recipe previews." );
        auto generation = recipe->getGeneration();
        require( recipe->requestPreview( error ), error );
        recipe->cancel();
        require( !recipe->isBusy() && !recipe->getPreview() && recipe->getGeneration() > generation,
                 "Recipe cancellation drains workers and invalidates prepared generation." );
        auto materials = *terrain->getTerrainSnapshot();
        materials.layers[0].roughness = 0.2f;
        require( terrain->applyTerrainData( materials, error ), error );
        require( reopened->load( recipePath.c_str(), terrain, error ) &&
                     reopened->requestPreview( error ) && ready( reopened, error ),
                 error );
        require( reopened->commit( terrain, commands, error ), error );
        require( terrain->getTerrainSnapshot()->layers[0].roughness == 0.2f,
                 "Loading geometry recipes cannot replace current material descriptors." );
        const auto serialized = reopened->serialize();
        auto malformed = serialized;
        const auto kind = malformed.find( "\"kind\":0" );
        require( kind != String::npos, "Recipe fixture contains a brush operation." );
        malformed.replace( kind, 8, "\"kind\":9" );
        require(
            !reopened->deserialize( malformed, terrain, error ) && reopened->serialize() == serialized,
            "Invalid imported operations preserve the complete previous ordered recipe." );
    }
}  // namespace terrain_authoring_contracts
#endif
