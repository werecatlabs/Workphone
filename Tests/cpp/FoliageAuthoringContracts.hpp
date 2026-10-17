#ifndef WPFoliageAuthoringContracts_h__
#define WPFoliageAuthoringContracts_h__
#include <Workphone/Scene/Components/Terrain/FoliageAuthoring.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <cmath>
#include <set>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <limits>

inline void runFoliageAuthoringContracts( bool plantGeometryOnly = false )
{
    using namespace workphone;
    using namespace workphone::scene;
    const auto require = []( bool result, const char *message ) { if( !result ) throw std::runtime_error( message ); };
    String error;
    FoliagePlantRecipe plant; plant.speciesId = "fixture/tree"; plant.leaves.albedo = "fixture/leaf-cutout"; plant.branches = 16;
    render::FoliageSpecies tree, grass;
    require( generateFoliagePlant( plant, tree, error ), error.c_str() );
    require( tree.lods[0].indices.size() > tree.lods[1].indices.size() && tree.lods[1].indices.size() > tree.lods[2].indices.size(), "Plant LODs independently reduce topology." );
    require( tree.materials[1].albedo == "fixture/leaf-cutout" && !tree.lods[2].uvs.empty(), "Generated leaf cards retain the texture resource and UVs." );
    const auto checkTreeGeometry = [&]( const render::FoliageSpecies &value ) {
        size_t previous = std::numeric_limits<size_t>::max();
        for( const auto &mesh : value.lods )
        {
            require( mesh.indices.size() < previous && mesh.indices.size() % 3 == 0,
                     "Each tree LOD must strictly reduce complete triangle topology." );
            previous = mesh.indices.size();
            require( mesh.positions.size() <= render::foliageMaximumMeshVertices &&
                     mesh.indices.size() <= render::foliageMaximumMeshIndices,
                     "Tree recipe must stay within species mesh budgets." );
            require( mesh.sections.size() == 2 && mesh.sections[0].material == 0 &&
                     mesh.sections[1].material == 1 && mesh.sections[0].firstIndex == 0 &&
                     mesh.sections[0].indexCount == mesh.sections[1].firstIndex &&
                     size_t( mesh.sections[1].firstIndex ) + mesh.sections[1].indexCount == mesh.indices.size(),
                     "Tree bark and leaf material sections must partition every LOD." );
            require( mesh.positions.size() == mesh.normals.size() && mesh.positions.size() == mesh.uvs.size(),
                     "Tree LOD vertex attributes must have equal extents." );
            bool crownNormalUp = false, crownNormalSide = false;
            for( size_t vertex = 0; vertex < mesh.positions.size(); ++vertex )
            {
                const auto p = mesh.positions[vertex], n = mesh.normals[vertex];
                const auto uv = mesh.uvs[vertex];
                require( std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z ) &&
                         std::isfinite( n.x ) && std::isfinite( n.y ) && std::isfinite( n.z ) &&
                         std::isfinite( uv.x ) && std::isfinite( uv.y ) && std::abs( n.lengthSquared() - 1.f ) < .0001f,
                         "Every tree position, UV and unit lighting normal must remain finite." );
                require( p.x >= value.boundsMinimum.x && p.x <= value.boundsMaximum.x &&
                         p.y >= value.boundsMinimum.y && p.y <= value.boundsMaximum.y &&
                         p.z >= value.boundsMinimum.z && p.z <= value.boundsMaximum.z,
                         "Tree bounds must contain every LOD vertex." );
            }
            for( size_t index = 0; index < mesh.indices.size(); index += 3 )
            {
                const auto a = mesh.indices[index], b = mesh.indices[index + 1], c = mesh.indices[index + 2];
                require( a < mesh.positions.size() && b < mesh.positions.size() && c < mesh.positions.size(),
                         "Tree triangle indices must reference valid vertices." );
                require( ( mesh.positions[b] - mesh.positions[a] ).crossProduct( mesh.positions[c] - mesh.positions[a] ).lengthSquared() > 1.e-16f,
                         "Tree taper and leaf cards must not generate degenerate triangles." );
                if( index >= mesh.sections[1].firstIndex )
                {
                    const auto n = mesh.normals[a];
                    crownNormalUp |= n.y > .5f;
                    crownNormalSide |= std::abs( n.x ) > .5f || std::abs( n.z ) > .5f;
                }
            }
            require( crownNormalUp && crownNormalSide && mesh.sections[1].indexCount >= 64 * 6,
                     "Reduced tree crowns must retain dense foliage with rounded lighting normals." );
        }
    };
    checkTreeGeometry( tree );
    render::FoliageSpecies repeated;
    require( generateFoliagePlant( plant, repeated, error ), error.c_str() );
    for( size_t lod = 0; lod < 3; ++lod )
    {
        const auto &a = tree.lods[lod], &b = repeated.lods[lod];
        require( a.positions.size() == b.positions.size() && a.indices.size() == b.indices.size(),
                 "Seeded tree generation must repeat topology exactly." );
        for( size_t vertex = 0; vertex < a.positions.size(); ++vertex )
            require( a.positions[vertex] == b.positions[vertex] && a.normals[vertex] == b.normals[vertex] && a.uvs[vertex] == b.uvs[vertex],
                     "Seeded tree generation must repeat positions, normals and UVs exactly." );
        for( size_t index = 0; index < a.indices.size(); ++index )
            require( a.indices[index] == b.indices[index], "Seeded tree generation must repeat indices exactly." );
    }
    auto denseRecipe = plant;
    denseRecipe.branches = 128;
    denseRecipe.seed = 0xffffffffu;
    denseRecipe.wind.amplitude = .15f;
    denseRecipe.wind.bendStart = 1;
    denseRecipe.wind.bendEnd = 4;
    render::FoliageSpecies dense;
    require( generateFoliagePlant( denseRecipe, dense, error ), error.c_str() );
    checkTreeGeometry( dense );
    require( dense.lods[0].positions.size() <= 28000 && dense.lods[0].indices.size() / 3 <= 14000 &&
             dense.lods[0].indices.size() > tree.lods[0].indices.size(),
             "Maximum branch recipe must retain the reviewed near-tree geometry budget." );
    require( dense.wind.amplitude == denseRecipe.wind.amplitude && dense.wind.bendStart == 1 && dense.wind.bendEnd == 4,
             "Generated crowns must retain authored height-derived wind weights." );
    plant.speciesId = "fixture/grass"; plant.kind = render::FoliageKind::Grass; plant.height = .8f; plant.radius = .4f;
    require( generateFoliagePlant( plant, grass, error ), error.c_str() );
    for( size_t lod = 0; lod < 3; ++lod )
        require( grass.lods[lod].positions.size() == ( 16u >> lod ) * 4 &&
                 grass.lods[lod].indices.size() == ( 16u >> lod ) * 6 && grass.lods[lod].sections.size() == 1 &&
                 grass.lods[lod].sections[0].material == 1,
                 "Broadleaf improvements must preserve existing grass card topology." );
    if( plantGeometryOnly ) return;
    // Population persistence/concurrency tests need species identity and bounds, not
    // thousands of repeated hero-tree vertices in each JSON round trip.
    auto placementTree = tree;
    for( auto &mesh : placementTree.lods )
    {
        const auto source = mesh;
        mesh = {};
        for( size_t i = 0; i < 3; ++i )
        {
            const auto vertex = source.indices[i];
            mesh.positions.push_back( source.positions[vertex] );
            mesh.normals.push_back( source.normals[vertex] );
            mesh.uvs.push_back( source.uvs[vertex] );
        }
        mesh.indices = { 0, 1, 2 };
        mesh.sections = { { 0, 3, 0 } };
    }
    const Array<std::shared_ptr<const render::FoliageSpecies>> species = { std::make_shared<const render::FoliageSpecies>( placementTree ), std::make_shared<const render::FoliageSpecies>( grass ) };
    auto data = std::make_shared<render::TerrainData>(); data->dimensions = { 17, 17 }; data->origin = { -8, -8 }; data->heights.assign( 289, 0 );
    FoliagePlacementRecipe recipe; recipe.pageSize = 8; recipe.candidateSpacing = 2; recipe.seed = 0xf1234567;
    FoliagePlacementRule rule; rule.density = 1; rule.minimumSpacing = 2.5f; recipe.rules = { rule };
    auto all = make_ptr<FoliageAuthoring>(), ordered = make_ptr<FoliageAuthoring>();
    for( auto author : { all, ordered } ) require( author->setSpecies( species, error ) && author->configure( recipe, error ), error.c_str() );
    require( all->regenerateAll( data, error ), error.c_str() );
    const auto population = all->getSnapshot(); require( !population->instances.empty(), "Deterministic scatter emits non-origin terrain instances." );
    // Generate the same signed cells in reverse order, with full neighbour halos.
    for( s32 z = 1; z >= -1; --z ) for( s32 x = 1; x >= -1; --x ) require( ordered->regenerate( data, { { x, z } }, error ), error.c_str() );
    require( ordered->getSnapshot()->instances.size() == population->instances.size(), "Cell order must not change population size." );
    for( size_t i = 0; i < population->instances.size(); ++i )
    {
        const auto &a = population->instances[i], &b = ordered->getSnapshot()->instances[i];
        require( a.id == b.id && a.position == b.position && a.scale == b.scale, "Stable cell identity is independent of generation order." );
        for( size_t j = 0; j < i; ++j ) require( ( a.position - population->instances[j].position ).length() >= 2.499f, "Cross-cell candidates respect the neighbour spacing halo." );
    }
    // Density-only changes form a stable subset when no spacing thinning applies.
    recipe.rules[0].minimumSpacing = 0; recipe.rules[0].density = .1f;
    require( all->configure( recipe, error ) && all->regenerateAll( data, error ), error.c_str() );
    std::set<u64> low; for( const auto &i : all->getSnapshot()->instances ) low.insert( i.id );
    recipe.rules[0].density = .2f; require( all->configure( recipe, error ) && all->regenerateAll( data, error ), error.c_str() );
    std::set<u64> high; for( const auto &i : all->getSnapshot()->instances ) high.insert( i.id );
    require( high.size() > low.size(), "Changing density visibly changes the native population." );
    for( auto id : low ) require( high.count( id ) != 0, "Density selection retains lower-density stable IDs." );
    const auto beforeStroke = all->getSnapshot();
    require( all->beginStroke( error ) && all->paint( data, 1, { 0, 0 }, 2, 12, 71, error ), error.c_str() );
    require( all->getSnapshot()->instances.size() == beforeStroke->instances.size() + 12, "Painting creates compact manual instances." );
    require( all->cancelStroke( error ) && all->getSnapshot()->instances.size() == beforeStroke->instances.size(), "Cancel restores the complete pre-stroke population." );
    require( all->beginStroke( error ) && all->paint( data, 1, { 0, 0 }, 2, 12, 71, error ) && all->commitStroke( error ), error.c_str() );
    auto painted = all->getSnapshot(); require( all->undo( error ) && all->getSnapshot()->instances.size() == beforeStroke->instances.size(), "Undo removes one whole paint stroke." );
    require( all->redo( error ) && all->getSnapshot()->instances.size() == painted->instances.size(), "Redo restores one whole paint stroke." );
    Array<u64> manual; for( const auto &i : all->getSnapshot()->instances ) if( i.flags & render::FoliageManual ) manual.push_back( i.id );
    require( manual.size() == 12, "Manual instances retain explicit identity and locking." );
    require( all->transformSelection( manual, { 0, 2, 0 }, 45, 1.5f, error ), error.c_str() );
    auto raised = std::make_shared<render::TerrainData>( *data ); raised->heights.assign( 289, 3 );
    auto grounded = all->getGroundedSnapshot( raised, error ); require( bool( grounded ), error.c_str() );
    for( const auto &i : grounded->instances ) if( i.flags & render::FoliageManual ) require( std::abs( i.position.y - 5 ) < .0001f, "Manual offsets follow changed terrain without losing authored move-Y." );
    require( all->setInstanceFlags( { manual[0] }, render::FoliageRemoved, true, error ) && all->setInstanceFlags( { manual[1] }, render::FoliagePromoted, true, error ), error.c_str() );
    require( all->regenerateAll( data, error ), error.c_str() );
    u32 removed = 0, promoted = 0; for( const auto &i : all->getSnapshot()->instances ) { removed += ( i.flags & render::FoliageRemoved ) != 0; promoted += ( i.flags & render::FoliagePromoted ) != 0; }
    require( removed == 1 && promoted == 1, "Regeneration preserves removal and promotion overrides." );
    String saved; require( all->serialize( saved, error ), error.c_str() );
    auto restored = make_ptr<FoliageAuthoring>(); require( restored->deserialize( saved, error ), error.c_str() );
    require( restored->getSnapshot()->instances.size() == all->getSnapshot()->instances.size(), "Save/reopen restores the exact compact population." );
    grounded = restored->getGroundedSnapshot( raised, error ); require( bool( grounded ), error.c_str() );
    for( const auto &i : grounded->instances ) if( i.flags & render::FoliageManual ) require( std::abs( i.position.y - 5 ) < .0001f, "Manual terrain offsets survive persistence." );
    raised->holes.assign( 256, 1 );
    require( restored->getGroundedSnapshot( raised, error )->instances.empty() && !restored->getSnapshot()->instances.empty(), "Holes suppress runtime placement without deleting manual source data." );
    std::atomic<bool> cancelled{ true }; auto retained = restored->getSnapshot();
    require( !restored->regenerateAll( data, error, &cancelled ) && restored->getSnapshot() == retained, "Cancelled generation preserves the last complete population." );
    require( !restored->deserialize( "{}", error ) && restored->getSnapshot() == retained, "Malformed persistence preserves the last complete population." );
    // Wire length alone cannot bound cJSON: millions of one-byte values have a
    // small file representation but an excessive DOM/allocator footprint.
    String crowded = "{\"padding\":[";
    crowded.reserve( 2400016 );
    for( size_t n = 0; n < 1200000; ++n ) crowded += n ? ",0" : "0";
    crowded += "]}";
    require( !restored->deserialize( crowded, error ) && error.find( "DOM/scratch budget" ) != String::npos &&
        restored->getSnapshot() == retained, "DOM-heavy authoring input is rejected before allocation and retains publication." );
    render::FoliageWorldData decoded = *retained;
    require( !render::deserializeFoliageWorld( crowded, decoded, error ) && error.find( "DOM/scratch budget" ) != String::npos &&
        decoded.instances.size() == retained->instances.size(), "Direct world parsing applies the same DOM budget and retains output." );
    render::FoliageSpecies decodedSpecies = tree;
    require( !render::deserializeFoliageSpecies( crowded, decodedSpecies, error ) && error.find( "DOM/scratch budget" ) != String::npos &&
        decodedSpecies.id == tree.id, "Direct species parsing applies the same DOM budget and retains output." );
    String nested( 65, '[' ); nested += '0'; nested.append( 65, ']' );
    require( !restored->deserialize( nested, error ) && error.find( "nesting" ) != String::npos &&
        restored->getSnapshot() == retained, "Deep foliage JSON is rejected before recursive DOM parsing." );
    render::FoliageWorldData oversized; oversized.species = species;
    oversized.instances.reserve( 70000 );
    for( u32 n = 0; n < 70000; ++n ) { render::FoliageInstance i; i.id = n + 1; i.position.x = f32( n / 60000 ) * 64; oversized.instances.push_back( i ); }
    require( render::validateFoliageWorld( oversized, error ), error.c_str() );
    String retainedText = "last good source";
    require( !render::serializeFoliageWorld( oversized, retainedText, error ) && error.find( "DOM/scratch budget" ) != String::npos &&
        retainedText == "last good source", "Serialization rejects excessive DOM construction before changing output." );
    // Structural-looking characters inside escaped strings are not JSON nodes.
    auto quotedSpecies = tree; quotedSpecies.id = "fixture/[{\"quoted\"}]";
    require( render::serializeFoliageSpecies( quotedSpecies, retainedText, error ) &&
        render::deserializeFoliageSpecies( retainedText, decodedSpecies, error ) && decodedSpecies.id == quotedSpecies.id,
        "JSON budget scanning respects quoted and escaped species IDs." );
    // Explicit density/biome masks and road exclusion operate on the same terrain sampler.
    auto biomes = make_ptr<FoliageAuthoring>(); require( biomes->setSpecies( species, error ), error.c_str() );
    recipe.rules[0].density = 1; rule = recipe.rules[0]; rule.species = 1; recipe.rules.push_back( rule );
    recipe.maskDimensions = { 2, 1 }; recipe.densityMask = { 1, 1 }; recipe.biomeMask = { 0, 1 };
    recipe.exclusions = { { { { 0, -8 }, { 0, 8 } }, 1 } };
    require( biomes->configure( recipe, error ) && biomes->regenerateAll( data, error ), error.c_str() );
    require( !biomes->getSnapshot()->instances.empty(), "Biomes retain placements outside exclusions." );
    for( const auto &i : biomes->getSnapshot()->instances ) require( std::abs( i.position.x ) > 1 && i.species == ( i.position.x < 0 ? 0u : 1u ), "Biome species masks and road corridor exclusions are honoured." );
    auto selected = biomes->select( { { -8, -8 }, { 0, -8 }, { 0, 8 }, { -8, 8 } } );
    require( !selected.empty() && biomes->replaceSpecies( selected, 1, error ), "Lasso selection and species replacement are native edits." );

    // Terrain refresh and Editor writes use different tasks. Exercise retained
    // grounding/serialization while edits replace worlds and their offset maps.
    std::atomic<bool> start{false}; std::exception_ptr readerFailure;
    std::thread reader([&] {
        try {
            while(!start.load()) std::this_thread::yield();
            for(u32 n=0;n<80;++n) {
                String failure,text; auto world=biomes->getGroundedSnapshot(data,failure);
                if(!world||!render::validateFoliageWorld(*world,failure)||!biomes->serialize(text,failure))
                    throw std::runtime_error(failure.c_str());
                auto copy=make_ptr<FoliageAuthoring>();
                if(!copy->deserialize(text,failure))throw std::runtime_error(failure.c_str());
            }
        }catch(...){readerFailure=std::current_exception();}
    });
    start=true; std::exception_ptr writerFailure;
    try {
        for(u32 n=0;n<80;++n) {
            require(biomes->transformSelection(selected,{0,.01f,0},0,1,error),error.c_str());
            auto next=biomes->getRecipe();next.seed=n+1;
            require(biomes->configure(next,error),error.c_str());
        }
    }catch(...){writerFailure=std::current_exception();}
    reader.join();if(readerFailure)std::rethrow_exception(readerFailure);if(writerFailure)std::rethrow_exception(writerFailure);
}
#endif
