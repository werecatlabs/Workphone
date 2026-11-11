#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/ProceduralFoliage.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>

using namespace workphone;

namespace
{
    void require( bool condition, const char *message )
    {
        if( !condition )
        {
            throw std::runtime_error( message );
        }
    }

    // Exercise the component's real CPU generation and mesh conversion without
    // requiring an Editor, graphics device, or a procedural plugin installation.
    class FoliageProbe : public scene::ProceduralFoliage
    {
    public:
        using ProceduralFoliage::buildMesh;
    };

    template <class T>
    T propertyValue( SmartPtr<Properties> properties, const String &name )
    {
        T value{};
        require( properties->getPropertyValue( name, value ),
                 ( "Missing foliage property: " + name ).c_str() );
        return value;
    }

    render::FoliageSpecies readMesh( SmartPtr<IMesh> mesh, bool grass )
    {
        require( mesh != nullptr, "Foliage preset generated no mesh" );
        const auto subMeshes = mesh->getSubMeshes();
        require( subMeshes.size() == ( grass ? 1u : 2u ),
                 "Foliage mesh lost its bark/leaf material partition" );
        for( auto subMesh : subMeshes )
        {
            require( subMesh && !subMesh->getMaterialName().empty(),
                     "Foliage submesh has no material binding" );
        }
        if( !grass )
        {
            require( subMeshes[0]->getMaterialName() != subMeshes[1]->getMaterialName(),
                     "Bark and leaves must have distinct material bindings" );
        }

        render::FoliageSpecies result;
        Array<render::FoliageMaterial> materials( subMeshes.size() );
        String error;
        const auto imported = scene::importFoliageMesh( mesh, "test/component", materials, result, error );
        require( imported, error.c_str() );
        const auto &geometry = result.lods[0];
        require( !geometry.positions.empty() && !geometry.indices.empty(),
                 "Foliage preset generated empty geometry" );
        for( size_t vertex = 0; vertex < geometry.positions.size(); ++vertex )
        {
            const auto &position = geometry.positions[vertex];
            const auto &normal = geometry.normals[vertex];
            const auto &uv = geometry.uvs[vertex];
            require( std::isfinite( position.x ) && std::isfinite( position.y ) &&
                         std::isfinite( position.z ) && std::isfinite( uv.x ) &&
                         std::isfinite( uv.y ) &&
                         std::abs( normal.lengthSquared() - 1.f ) < .001f,
                     "Foliage conversion produced invalid positions, normals, or UVs" );
        }
        return result;
    }

    void requireSameGeometry( const render::FoliageMeshData &a,
                              const render::FoliageMeshData &b, s32 preset, u32 lod,
                              bool serialized = false )
    {
        require( a.positions.size() == b.positions.size() && a.indices.size() == b.indices.size() &&
                     a.sections.size() == b.sections.size(),
                 "Foliage component reload changed mesh topology" );
        for( size_t vertex = 0; vertex < a.positions.size(); ++vertex )
        {
            const auto positionDelta = ( a.positions[vertex] - b.positions[vertex] ).length();
            const auto normalDelta = ( a.normals[vertex] - b.normals[vertex] ).length();
            // Float properties serialize with six significant digits. This
            // permits ten micrometres of position drift after JSON roundtrip;
            // repeated generation without serialization remains bit-for-bit.
            const bool same = ( serialized ? positionDelta < .00001f && normalDelta < .00001f :
                                a.positions[vertex] == b.positions[vertex] &&
                                a.normals[vertex] == b.normals[vertex] ) &&
                              a.uvs[vertex] == b.uvs[vertex];
            if( !same )
            {
                const auto &p = a.positions[vertex], &q = b.positions[vertex];
                const auto &n = a.normals[vertex], &m = b.normals[vertex];
                const auto &u = a.uvs[vertex], &v = b.uvs[vertex];
                std::fprintf( stderr,
                    "preset=%d lod=%u vertex=%zu position=(%.9g,%.9g,%.9g) -> (%.9g,%.9g,%.9g), "
                    "normal=(%.9g,%.9g,%.9g) -> (%.9g,%.9g,%.9g), uv=(%.9g,%.9g) -> (%.9g,%.9g)\n",
                    preset, lod, vertex, p.x, p.y, p.z, q.x, q.y, q.z,
                    n.x, n.y, n.z, m.x, m.y, m.z, u.x, u.y, v.x, v.y );
            }
            require( same, "Foliage component changed seeded vertex attributes" );
        }
        for( size_t index = 0; index < a.indices.size(); ++index )
        {
            require( a.indices[index] == b.indices[index],
                     "Foliage component reload changed seeded triangle indices" );
        }
        for( size_t section = 0; section < a.sections.size(); ++section )
        {
            require( a.sections[section].firstIndex == b.sections[section].firstIndex &&
                         a.sections[section].indexCount == b.sections[section].indexCount &&
                         a.sections[section].material == b.sections[section].material,
                     "Foliage component reload changed material sections" );
        }
    }

    void requirePalmSilhouette( const render::FoliageMeshData &geometry, real_Num height,
                                real_Num radius )
    {
        real_Num leafMinimumY = std::numeric_limits<real_Num>::max();
        real_Num leafMaximumY = -leafMinimumY;
        real_Num leafMinimumX = leafMinimumY, leafMinimumZ = leafMinimumY;
        real_Num leafMaximumX = leafMaximumY, leafMaximumZ = leafMaximumY;
        real_Num trunkMinimumY = leafMinimumY, lowerTrunkRadius = 0;
        size_t lowerTrunkVertices = 0;
        for( const auto &section : geometry.sections )
        {
            for( u32 index = section.firstIndex; index < section.firstIndex + section.indexCount;
                 ++index )
            {
                const auto &position = geometry.positions[geometry.indices[index]];
                if( section.material == 1 )
                {
                    leafMinimumY = std::min( leafMinimumY, real_Num( position.y ) );
                    leafMaximumY = std::max( leafMaximumY, real_Num( position.y ) );
                    leafMinimumX = std::min( leafMinimumX, real_Num( position.x ) );
                    leafMaximumX = std::max( leafMaximumX, real_Num( position.x ) );
                    leafMinimumZ = std::min( leafMinimumZ, real_Num( position.z ) );
                    leafMaximumZ = std::max( leafMaximumZ, real_Num( position.z ) );
                }
                else
                {
                    trunkMinimumY = std::min( trunkMinimumY, real_Num( position.y ) );
                    if( position.y < height * .5f )
                    {
                        ++lowerTrunkVertices;
                        lowerTrunkRadius = std::max(
                            lowerTrunkRadius,
                            real_Num( std::hypot( position.x, position.z ) ) );
                    }
                }
            }
        }

        // A palm must keep a tall exposed trunk and a spreading crown even in its
        // coarsest LOD; a generic broadleaf crown would fail the bare-trunk check.
        require( lowerTrunkVertices > 0 && std::abs( trunkMinimumY ) < height * .05f &&
                     lowerTrunkRadius < radius * .5f,
                 "Palm must have a slender trunk extending to ground level" );
        require( leafMinimumY > height * .5f && leafMaximumY > height * .85f,
                 "Palm fronds must remain in the crown above the exposed lower trunk" );
        require( leafMaximumX - leafMinimumX > radius * 1.3f &&
                     leafMaximumZ - leafMinimumZ > radius * 1.3f,
                 "Palm crown must retain broad radial fronds along both horizontal axes" );
    }

    void presetContracts()
    {
        using Preset = scene::ProceduralFoliage::Preset;
        for( const auto preset : { Preset::Oak, Preset::Birch, Preset::Pine, Preset::Bush,
                                   Preset::Grass, Preset::TallGrass, Preset::Palm } )
        {
            const bool grass = preset == Preset::Grass || preset == Preset::TallGrass;
            FoliageProbe source;
            source.setPreset( preset );
            require( source.getPreset() == preset, "Foliage preset selection was not retained" );
            require( !source.regenerate(), "Detached foliage must defer actor regeneration" );

            auto input = source.getProperties();
            input->setProperty( "Seed", u32( 7821 ) );
            input->setProperty( "Height", propertyValue<real_Num>( input, "Height" ) * real_Num( 1.25 ) );
            input->setProperty( "Radius", propertyValue<real_Num>( input, "Radius" ) * real_Num( 1.125 ) );
            input->setProperty( "Bark Colour", ColourF( .31f, .17f, .08f, 1 ) );
            input->setProperty( "Leaf Colour", ColourF( .12f, .41f, .09f, 1 ) );
            source.setProperties( input );
            source.setEnabled( false );

            size_t previousIndices = std::numeric_limits<size_t>::max();
            for( u32 lod = 0; lod < 3; ++lod )
            {
                input = source.getProperties();
                input->setProperty( "LOD", lod );
                source.setProperties( input );
                const auto before = readMesh( source.buildMesh(), grass );
                if( preset == Preset::Palm )
                {
                    requirePalmSilhouette( before.lods[0], propertyValue<real_Num>( input, "Height" ),
                                           propertyValue<real_Num>( input, "Radius" ) );
                }
                const auto repeated = readMesh( source.buildMesh(), grass );
                requireSameGeometry( before.lods[0], repeated.lods[0], s32( preset ), lod );
                require( before.lods[0].indices.size() < previousIndices,
                         "Foliage component LOD selection must reduce triangle topology" );
                previousIndices = before.lods[0].indices.size();

                const auto serialized = DataUtil::toString( source.toData().get(), true );
                auto parsed = make_ptr<Properties>();
                DataUtil::parse( serialized, parsed.get() );
                FoliageProbe restored;
                restored.fromData( parsed );
                require( restored.getPreset() == preset && !restored.isEnabled(),
                         "Foliage JSON roundtrip lost preset or enabled state" );
                const auto output = restored.getProperties();
                require( propertyValue<u32>( output, "Seed" ) == 7821u &&
                             propertyValue<u32>( output, "LOD" ) == lod,
                         "Foliage JSON roundtrip lost seed or LOD" );
                for( const auto *property : { "Height", "Radius", "Trunk Radius" } )
                {
                    require( std::abs( propertyValue<real_Num>( output, property ) -
                                       propertyValue<real_Num>( input, property ) ) < real_Num( .00001 ),
                             "Foliage JSON roundtrip lost authored dimensions" );
                }
                require( propertyValue<u32>( output, "Branches" ) ==
                             propertyValue<u32>( input, "Branches" ),
                         "Foliage JSON roundtrip lost branch count" );
                const auto bark = propertyValue<ColourF>( output, "Bark Colour" );
                const auto leaves = propertyValue<ColourF>( output, "Leaf Colour" );
                require( std::abs( bark.r - .31f ) < .00001f &&
                             std::abs( leaves.g - .41f ) < .00001f,
                         "Foliage JSON roundtrip lost authored material colours" );
                const auto after = readMesh( restored.buildMesh(), grass );
                requireSameGeometry( before.lods[0], after.lods[0], s32( preset ), lod, true );
            }
        }
    }
}  // namespace

int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto app = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( app );
    app->setFactoryManager( make_ptr<FactoryManager>() );
    app->setGameManager( make_ptr<scene::GameManager>() );
    int result = 0;
    try
    {
        presetContracts();
        std::puts( "Procedural foliage: seven presets, palm silhouette, LOD geometry, materials and JSON persistence PASS" );
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "Procedural foliage failed: %s\n", error.what() );
        result = 1;
    }
    app->setGameManager( nullptr );
    app->setFactoryManager( nullptr );
    core::IApplicationManager::setInstance( nullptr );
    app = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    return result;
}
