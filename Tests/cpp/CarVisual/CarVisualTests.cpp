#include <Workphone/Vehicle/CarVisual.hpp>
#include <Workphone/Vehicle/CarVisualAssets.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <filesystem>
#include <chrono>
#include <meshoptimizer.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace workphone::vehiclevisual;

namespace
{
    void require( bool condition, const char *message )
    {
        if( !condition )
        {
            throw std::runtime_error( message );
        }
    }

    bool close( float a, float b )
    {
        return std::abs( a - b ) < .001f;
    }

    Mesh box( std::string name, Vec3 lo, Vec3 hi, unsigned repeats = 1 )
    {
        Mesh m;
        m.name = m.sourceMaterial = std::move( name );
        for( unsigned repeat = 0; repeat < repeats; ++repeat )
        {
            for( unsigned i = 0; i < 8; ++i )
            {
                Vertex v;
                v.position = { i & 1 ? hi.x : lo.x, i & 2 ? hi.y : lo.y, i & 4 ? hi.z : lo.z };
                v.uv0 = { .125f, .875f };
                m.vertices.push_back( v );
            }
        }
        m.indices = { 0, 2, 3, 0, 3, 1, 4, 5, 7, 4, 7, 6, 0, 4, 6, 0, 6, 2,
                      1, 3, 7, 1, 7, 5, 0, 1, 5, 0, 5, 4, 2, 6, 7, 2, 7, 3 };
        return m;
    }

    Models fixture()
    {
        Models models;
        models.chassis.push_back( box( "body_paint", { -.9f, 0, -2 }, { .9f, 1.4f, 2 }, 32 ) );
        auto glass = box( "windows", { -.5f, .8f, -.6f }, { .5f, 1.4f, .6f } );
        glass.transparent = true;
        glass.opacity = .7f;
        models.chassis.push_back( glass );
        models.chassis.push_back( box( "rubber_trim", { -.8f, .1f, -1.8f }, { .8f, .2f, 1.8f } ) );
        models.wheel.push_back( box( "wheel_rim", { 1.9f, 2.2f, -1.8f }, { 2.1f, 3.8f, -.2f } ) );
        return models;
    }

    const Node &find( const CarVisual &visual, Kind kind )
    {
        for( const auto &n : visual.nodes() )
        {
            if( n.kind == kind )
            {
                return n;
            }
        }
        throw std::runtime_error( "Missing presentation node" );
    }

    void portContracts()
    {
        auto models = fixture();
        auto original = models;
        // Tall outlier as a separate mesh and outlying vertices in the body itself.
        auto dirty = models.chassis;
        dirty.push_back( box( "helper", { -.05f, 9, -.05f }, { .05f, 20, .05f } ) );
        auto robust = CarVisual::robustBox( dirty );
        require( robust.max.y < 2, "Tall helper corrupted robust bounds" );
        auto offsetWheel = models.wheel;
        offsetWheel.emplace_back();
        require( CarVisual::robustBox( offsetWheel ).min.x > 1,
                 "Empty mesh incorrectly expanded wheel bounds to the origin" );
        auto sparse = models.chassis;
        sparse[0].vertices.push_back( { { 0, 200, 0 } } );
        require( CarVisual::percentileBox( sparse ).max.y < 2,
                 "Embedded helper corrupted percentile bounds" );
        Spec spec;
        spec.police = true;
        CarVisual visual;
        visual.configure( spec, models, { .7f, .1f, .03f, 1 } );
        require( models.wheel[0].vertices[0].position.x == original.wheel[0].vertices[0].position.x,
                 "Shared wheel prototype mutated" );
        require( models.chassis[0].vertices[0].position.z == original.chassis[0].vertices[0].position.z,
                 "Shared chassis mutated" );
        unsigned wheels = 0, painted = 0, glass = 0;
        for( const auto &node : visual.nodes() )
        {
            if( node.paint )
            {
                ++painted;
                require( close( node.roughness, .24f ), "Paint response not ported" );
            }
            if( node.glass )
            {
                ++glass;
                require( node.opacity <= .42f && !node.depthWrite, "Glass response not ported" );
            }
            if( node.kind == Kind::Wheel )
            {
                ++wheels;
                Bounds b;
                for( const auto &v : node.mesh->vertices )
                {
                    b.include( v.position );
                }
                require( close( b.centre().x, 0 ) && close( b.centre().y, 0 ),
                         "Wheel was not centred on its own axle" );
                require( close( b.size().y, spec.wheelRadius[node.corner] * 2 ),
                         "Wheel scaling compounded across clones" );
                const auto &spin = visual.nodes()[node.parent], &pivot = visual.nodes()[spin.parent];
                require( node.corner < 2 ? pivot.position.z < 0 : pivot.position.z > 0,
                         "Steering axle reversed" );
            }
            if( node.kind == Kind::Body )
            {
                for( const auto &v : node.mesh->vertices )
                {
                    require( v.uv0.x == .125f, "UV0 overwritten by livery projection" );
                }
            }
        }
        require( wheels == 4 && painted == 2 && glass == 1, "Missing wheel/material assembly" );
        require(
            find( visual, Kind::Headlight ).position.z < 0 && find( visual, Kind::Tail ).position.z > 0,
            "Lights reversed" );
        visual.setLivery( "livery.png" );
        require( close( find( visual, Kind::Body ).colour.r, 1 ), "Livery double-tinted" );
        visual.setPaint( { .2f, .3f, .4f, 1 } );
        visual.setLivery( {} );
        require( close( find( visual, Kind::Body ).colour.r, .2f ), "Removing livery lost paint" );
        Frame f;
        f.speed = 40;
        f.brake = 1;
        f.nitrous = 1;
        f.steering[0] = .25f;
        f.rotation[2] = 5;
        visual.sync( f, 1 );
        require( find( visual, Kind::WheelBlur ).opacity > .7f, "Wheel blur not ported" );
        require( close( find( visual, Kind::Tail ).opacity, 1 ), "Brake lights not ported" );
        require(
            find( visual, Kind::Underglow ).opacity > .3f && find( visual, Kind::Flame ).opacity > .6f,
            "Nitrous FX not ported" );
        auto flameOpacity = find( visual, Kind::Flame ).opacity;
        visual.sync( f, 1 );
        require( close( find( visual, Kind::Flame ).opacity, flameOpacity ), "FX not reproducible" );
        f.nitrous = 0;
        f.headlights = false;
        visual.sync( f, 2 );
        require(
            find( visual, Kind::Flame ).opacity == 0 && find( visual, Kind::Underglow ).opacity == 0,
            "Permanent idle nitrous leak" );
        require( visual.spots()[0].intensity == 0 && find( visual, Kind::Beam ).opacity == 0,
                 "Headlight switch ignored" );
        visual.setVisible( false );
        require( !visual.nodes()[0].visible, "Visibility not ported" );
        auto image = CarVisual::texture( Texture::Glow );
        require( image == CarVisual::texture( Texture::Glow ), "Effect texture not shared" );
        require( image->rgba[3] == 0 && image->rgba[( 64 * 128 + 64 ) * 4 + 3] > 240,
                 "Glow texture gradient broken" );
        auto before = visual.nodes().size();
        auto invalid = spec;
        invalid.length = 0;
        try
        {
            visual.configure( invalid, models, {} );
            require( false, "Invalid spec accepted" );
        }
        catch( const std::invalid_argument & )
        {
        }
        require( visual.nodes().size() == before, "Failed rebuild destroyed valid visual" );
        f.speed = std::numeric_limits<float>::quiet_NaN();
        try
        {
            visual.sync( f, 3 );
            require( false, "Invalid frame accepted" );
        }
        catch( const std::invalid_argument & )
        {
        }
    }

    void compressedAssetContracts()
    {
        namespace fs = std::filesystem;
        const auto directory =
            fs::current_path() /
            ( "carvisual-fixture-" +
              std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
        fs::create_directories( directory );
        const float vertices[9] = { -1, 0, 0, 1, 0, 0, 0, 1, 0 };
        std::vector<unsigned char> compressed( meshopt_encodeVertexBufferBound( 3, 12 ) );
        compressed.resize(
            meshopt_encodeVertexBuffer( compressed.data(), compressed.size(), vertices, 3, 12 ) );
        using Json = nlohmann::json;
        Json extension = {
            { "buffer", 0 },      { "byteOffset", 0 }, { "byteLength", compressed.size() },
            { "byteStride", 12 }, { "count", 3 },      { "mode", "ATTRIBUTES" }
        };
        Json view = { { "buffer", 1 }, { "byteLength", 36 }, { "byteStride", 12 } };
        view["extensions"]["EXT_meshopt_compression"] = extension;
        Json document;
        document["asset"]["version"] = "2.0";
        document["buffers"] =
            Json::array( { Json{ { "byteLength", compressed.size() } }, Json{ { "byteLength", 36 } } } );
        document["bufferViews"] = Json::array( { view } );
        // Encoded image bytes stay identical while the importer receives a path.
        const auto imageOffset = compressed.size();
        compressed.insert( compressed.end(), { 'R', 'I', 'F', 'F', 'W', 'E', 'B', 'P' } );
        document["buffers"][0]["byteLength"] = compressed.size();
        document["bufferViews"].push_back(
            { { "buffer", 0 }, { "byteOffset", imageOffset }, { "byteLength", 8 } } );
        document["images"] =
            Json::array( { Json{ { "bufferView", 1 }, { "mimeType", "image/webp" } } } );
        document["accessors"] = Json::array( { Json{
            { "bufferView", 0 }, { "componentType", 5126 }, { "count", 3 }, { "type", "VEC3" } } } );
        Json primitive;
        primitive["attributes"]["POSITION"] = 0;
        Json mesh;
        mesh["primitives"] = Json::array( { primitive } );
        document["meshes"] = Json::array( { mesh } );
        auto json = document.dump();
        while( json.size() % 4 )
        {
            json.push_back( ' ' );
        }
        while( compressed.size() % 4 )
        {
            compressed.push_back( 0 );
        }
        const auto input = directory / "source.glb";
        auto write32 = []( std::ostream &out, std::uint32_t value ) {
            for( unsigned i = 0; i < 4; ++i )
            {
                out.put( char( value >> ( i * 8 ) ) );
            }
        };
        {
            std::ofstream out( input, std::ios::binary );
            write32( out, 0x46546c67 );
            write32( out, 2 );
            write32( out, std::uint32_t( 28 + json.size() + compressed.size() ) );
            write32( out, std::uint32_t( json.size() ) );
            write32( out, 0x4e4f534a );
            out.write( json.data(), json.size() );
            write32( out, std::uint32_t( compressed.size() ) );
            write32( out, 0x004e4942 );
            out.write( reinterpret_cast<const char *>( compressed.data() ), compressed.size() );
        }
        const auto sourceSize = fs::file_size( input );
        const auto decoded = prepareCarVisualGLB( input.u8string(), ( directory / "cache" ).u8string() );
        require( decoded != input.u8string() && fs::is_regular_file( decoded ),
                 "Compressed GLB was not prepared" );
        require( fs::file_size( input ) == sourceSize, "Source GLB was overwritten" );
        const auto sidecar = fs::path( decoded ).parent_path() /
                             ( fs::path( decoded ).stem().u8string() + "-image-0.webp" );
        require( fs::is_regular_file( sidecar ) && fs::file_size( sidecar ) == 8,
                 "Embedded texture was not exposed to the native importer" );
        {
            std::ifstream image( sidecar, std::ios::binary );
            std::string bytes( ( std::istreambuf_iterator<char>( image ) ), {} );
            require( bytes == "RIFFWEBP", "Embedded texture bytes changed" );
        }
        require( prepareCarVisualGLB( input.u8string(), ( directory / "cache" ).u8string() ) == decoded,
                 "GLB cache identity changed" );
        {
            std::ofstream bad( directory / "bad.glb", std::ios::binary );
            bad << "bad";
        }
        try
        {
            prepareCarVisualGLB( ( directory / "bad.glb" ).u8string(),
                                 ( directory / "cache" ).u8string() );
            require( false, "Truncated GLB accepted" );
        }
        catch( const std::runtime_error &e )
        {
            require( std::string( e.what() ) != "Truncated GLB accepted", "Truncated GLB accepted" );
        }
        fs::remove( decoded );
        fs::remove( sidecar );
        fs::remove( input );
        fs::remove( directory / "bad.glb" );
        fs::remove( directory / "cache" );
        fs::remove( directory );
    }

    std::vector<Mesh> readFixture( const char *path )
    {
        std::ifstream in( path );
        std::size_t count = 0;
        in >> count;
        require( bool( in ) && count > 0, "Cannot read imported model fixture" );
        std::vector<Mesh> meshes;
        for( std::size_t i = 0; i < count; ++i )
        {
            Mesh m;
            std::size_t vertices = 0, indices = 0;
            in >> std::quoted( m.name ) >> std::quoted( m.sourceMaterial ) >> vertices >> indices >>
                m.colour.r >> m.colour.g >> m.colour.b >> m.colour.a >> m.roughness >> m.metalness >>
                m.opacity >> m.transparent;
            require( vertices < 10000000 && indices < 30000000, "Fixture size invalid" );
            m.vertices.resize( vertices );
            m.indices.resize( indices );
            for( auto &v : m.vertices )
            {
                in >> v.position.x >> v.position.y >> v.position.z >> v.normal.x >> v.normal.y >>
                    v.normal.z >> v.tangent.x >> v.tangent.y >> v.tangent.z >> v.tangentSign >>
                    v.uv0.x >> v.uv0.y;
            }
            for( auto &index : m.indices )
            {
                in >> index;
            }
            require( bool( in ), "Truncated imported model fixture" );
            meshes.push_back( std::move( m ) );
        }
        return meshes;
    }

    void importedContracts( const char *chassis, const char *wheel, const char *output )
    {
        Models models;
        models.chassis = readFixture( chassis );
        models.wheel = readFixture( wheel );
        Spec spec;
        spec.length = 4.67f;
        spec.wheelbase = 2.81f;
        spec.frontTrack = spec.rearTrack = 1.62f;
        CarVisual visual;
        visual.configure( spec, models, { .36f, .018f, .030f, 1 } );
        Bounds body;
        unsigned wheels = 0, paint = 0;
        std::ofstream out( output );
        for( auto &n : visual.nodes() )
        {
            if( n.kind == Kind::Body || n.kind == Kind::Wheel )
            {
                if( n.kind == Kind::Wheel )
                {
                    ++wheels;
                }
                if( n.paint )
                {
                    ++paint;
                }
                Vec3 offset = n.position;
                for( int p = n.parent; p >= 0; p = visual.nodes()[p].parent )
                {
                    auto v = visual.nodes()[p].position;
                    offset.x += v.x;
                    offset.y += v.y;
                    offset.z += v.z;
                }
                for( std::size_t i = 0; i < n.mesh->indices.size(); i += 3 )
                {
                    out << n.colour.r << " " << n.colour.g << " " << n.colour.b << " " << n.opacity;
                    for( int j = 0; j < 3; ++j )
                    {
                        auto p = n.mesh->vertices[n.mesh->indices[i + j]].position;
                        p.x += offset.x;
                        p.y += offset.y;
                        p.z += offset.z;
                        out << " " << p.x << " " << p.y << " " << p.z;
                        if( n.kind == Kind::Body )
                        {
                            body.include( p );
                        }
                    }
                    for( int j = 0; j < 3; ++j )
                    {
                        auto p = n.mesh->vertices[n.mesh->indices[i + j]].normal;
                        out << " " << p.x << " " << p.y << " " << p.z;
                    }
                    out << "\n";
                }
            }
        }
        require( paint > 0 && wheels >= 4 && body.valid, "Real imported model assembly failed" );
        std::cout << "Imported model: " << models.chassis.size() << " chassis parts, " << wheels
                  << " wheel parts, " << paint << " paint parts; raw bounds " << body.size().x << ", "
                  << body.size().y << ", " << body.size().z << "\n";
    }

}  // namespace

int main( int argc, char **argv )
{
    try
    {
        if( argc == 4 && std::string( argv[1] ) == "--prepare" )
        {
            std::cout << prepareCarVisualGLB( argv[2], argv[3] ) << "\n";
            return 0;
        }
        portContracts();
        compressedAssetContracts();
        if( argc == 4 )
        {
            importedContracts( argv[1], argv[2], argv[3] );
        }
        std::cout << "CarVisual C++17 port contracts passed\n";
        return 0;
    }
    catch( const std::exception &e )
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
