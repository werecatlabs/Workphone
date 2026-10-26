#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/FoliageData.hpp>
#include "FoliageJsonBudget.hpp"
#include <cJSON.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <unordered_set>

namespace workphone::render
{
    namespace
    {
        bool reject( String &error, const char *message )
        {
            error = message;
            return false;
        }

        bool range( f64 n, f64 low, f64 high )
        {
            return std::isfinite( n ) && n >= low && n <= high;
        }

        bool finite( const Vector3F &v )
        {
            return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z );
        }

        bool textValid( const String &s, size_t maximum )
        {
            return s.size() <= maximum && s.find( '\0' ) == String::npos;
        }

        bool colourValid( const ColourF &c )
        {
            return range( c.r, 0, 64 ) && range( c.g, 0, 64 ) && range( c.b, 0, 64 ) &&
                   range( c.a, 0, 1 );
        }

        u64 speciesBytes( const FoliageSpecies &s )
        {
            u64 bytes =
                sizeof( s ) + s.id.capacity() + 1 + s.materials.capacity() * sizeof( FoliageMaterial );
            for( const auto &m : s.materials )
            {
                bytes += m.albedo.capacity() + m.normal.capacity() + m.orm.capacity() + 3;
            }
            for( const auto &m : s.lods )
            {
                bytes +=
                    m.positions.capacity() * sizeof( Vector3F ) +
                    m.normals.capacity() * sizeof( Vector3F ) + m.uvs.capacity() * sizeof( Vector2F ) +
                    m.colours.capacity() * sizeof( ColourF ) + m.indices.capacity() * sizeof( u32 ) +
                    m.sections.capacity() * sizeof( FoliageMeshSection );
            }
            bytes += s.impostor.views.capacity() * sizeof( FoliageImpostorView ) +
                     s.impostor.albedo.capacity() + s.impostor.normal.capacity() +
                     s.impostor.orm.capacity() + s.impostor.depth.capacity() + 4;
            return bytes;
        }

        void estimateSpeciesJson( const FoliageSpecies &s, detail::FoliageJsonEstimate &budget )
        {
            budget.add( 256, s.id.size() + s.impostor.albedo.size() + s.impostor.normal.size() +
                                 s.impostor.orm.size() + s.impostor.depth.size() );
            for( const auto &m : s.materials )
            {
                budget.add( 32, m.albedo.size() + m.normal.size() + m.orm.size() );
            }
            budget.add( s.impostor.views.size() * 32 );
            for( const auto &m : s.lods )
            {
                budget.add( 32 + m.positions.size() * 4 + m.normals.size() * 4 + m.uvs.size() * 3 +
                            m.colours.size() * 5 + m.indices.size() + m.sections.size() * 8 );
            }
        }

        using Json = std::unique_ptr<cJSON, decltype( &cJSON_Delete )>;

        Json own( cJSON *value )
        {
            if( !value )
            {
                throw std::bad_alloc();
            }
            return Json( value, cJSON_Delete );
        }

        Json object()
        {
            return own( cJSON_CreateObject() );
        }

        Json array()
        {
            return own( cJSON_CreateArray() );
        }

        void add( cJSON *parent, const char *key, Json child )
        {
            if( !cJSON_AddItemToObject( parent, key, child.get() ) )
            {
                throw std::bad_alloc();
            }
            child.release();
        }

        void append( cJSON *parent, Json child )
        {
            if( !cJSON_AddItemToArray( parent, child.get() ) )
            {
                throw std::bad_alloc();
            }
            child.release();
        }

        void number( cJSON *parent, const char *key, double n )
        {
            add( parent, key, own( cJSON_CreateNumber( n ) ) );
        }

        void string( cJSON *parent, const char *key, const String &s )
        {
            add( parent, key, own( cJSON_CreateString( s.c_str() ) ) );
        }

        void boolean( cJSON *parent, const char *key, bool b )
        {
            add( parent, key, own( cJSON_CreateBool( b ) ) );
        }

        Json floats( std::initializer_list<f32> values )
        {
            auto a = array();
            for( auto n : values )
            {
                append( a.get(), own( cJSON_CreateNumber( n ) ) );
            }
            return a;
        }

        Json vec( const Vector3F &v )
        {
            return floats( { v.x, v.y, v.z } );
        }

        Json colour( const ColourF &c )
        {
            return floats( { c.r, c.g, c.b, c.a } );
        }

        const cJSON *field( const cJSON *o, const char *key )
        {
            if( !cJSON_IsObject( o ) )
            {
                throw std::invalid_argument( "Expected foliage JSON object." );
            }
            auto v = cJSON_GetObjectItemCaseSensitive( o, key );
            if( !v )
            {
                throw std::invalid_argument( String( "Missing foliage field: " ) + key );
            }
            return v;
        }

        f64 numeric( const cJSON *v )
        {
            if( !cJSON_IsNumber( v ) || !std::isfinite( v->valuedouble ) )
            {
                throw std::invalid_argument( "Expected finite foliage number." );
            }
            return v->valuedouble;
        }

        f32 scalar( const cJSON *o, const char *key )
        {
            const auto n = numeric( field( o, key ) );
            if( std::abs( n ) > std::numeric_limits<f32>::max() )
            {
                throw std::invalid_argument( "Foliage float overflows." );
            }
            return f32( n );
        }

        u32 unsignedValue( const cJSON *v )
        {
            const auto n = numeric( v );
            if( n < 0 || n > std::numeric_limits<u32>::max() || std::floor( n ) != n )
            {
                throw std::invalid_argument( "Expected foliage uint32." );
            }
            return u32( n );
        }

        u32 integer( const cJSON *o, const char *key )
        {
            return unsignedValue( field( o, key ) );
        }

        String stringValue( const cJSON *v, size_t maximum = 4096 )
        {
            if( !cJSON_IsString( v ) || !v->valuestring || std::strlen( v->valuestring ) > maximum )
            {
                throw std::invalid_argument( "Invalid foliage string." );
            }
            return v->valuestring;
        }

        String textField( const cJSON *o, const char *key, size_t maximum = 4096 )
        {
            return stringValue( field( o, key ), maximum );
        }

        bool flag( const cJSON *o, const char *key )
        {
            auto v = field( o, key );
            if( !cJSON_IsBool( v ) )
            {
                throw std::invalid_argument( "Expected foliage boolean." );
            }
            return cJSON_IsTrue( v ) != 0;
        }

        size_t count( const cJSON *a, size_t maximum )
        {
            if( !cJSON_IsArray( a ) )
            {
                throw std::invalid_argument( "Expected foliage array." );
            }
            const auto n = cJSON_GetArraySize( a );
            if( n < 0 || size_t( n ) > maximum )
            {
                throw std::invalid_argument( "Foliage array exceeds its bound." );
            }
            return size_t( n );
        }

        std::array<f32, 4> components( const cJSON *v, size_t expected )
        {
            if( count( v, expected ) != expected )
            {
                throw std::invalid_argument( "Wrong foliage vector width." );
            }
            std::array<f32, 4> out{};
            size_t i = 0;
            for( auto p = v->child; p; p = p->next )
            {
                auto n = numeric( p );
                if( std::abs( n ) > std::numeric_limits<f32>::max() )
                {
                    throw std::invalid_argument( "Foliage vector overflows." );
                }
                out[i++] = f32( n );
            }
            return out;
        }

        Vector3F readVec( const cJSON *o, const char *key )
        {
            auto v = components( field( o, key ), 3 );
            return { v[0], v[1], v[2] };
        }

        ColourF readColour( const cJSON *o, const char *key )
        {
            auto v = components( field( o, key ), 4 );
            return { v[0], v[1], v[2], v[3] };
        }

        Json meshJson( const FoliageMeshData &m )
        {
            auto o = object(), p = array(), n = array(), uv = array(), c = array(), indices = array(),
                 sections = array();
            for( const auto &v : m.positions )
            {
                append( p.get(), vec( v ) );
            }
            for( const auto &v : m.normals )
            {
                append( n.get(), vec( v ) );
            }
            for( const auto &v : m.uvs )
            {
                append( uv.get(), floats( { v.x, v.y } ) );
            }
            for( const auto &v : m.colours )
            {
                append( c.get(), colour( v ) );
            }
            for( auto v : m.indices )
            {
                append( indices.get(), own( cJSON_CreateNumber( v ) ) );
            }
            for( const auto &v : m.sections )
            {
                auto s = object();
                number( s.get(), "first", v.firstIndex );
                number( s.get(), "count", v.indexCount );
                number( s.get(), "material", v.material );
                append( sections.get(), std::move( s ) );
            }
            add( o.get(), "positions", std::move( p ) );
            add( o.get(), "normals", std::move( n ) );
            add( o.get(), "uvs", std::move( uv ) );
            add( o.get(), "colours", std::move( c ) );
            add( o.get(), "indices", std::move( indices ) );
            add( o.get(), "sections", std::move( sections ) );
            return o;
        }

        FoliageMeshData readMesh( const cJSON *o )
        {
            FoliageMeshData m;
            const auto read3 = [&]( const char *key, Array<Vector3F> &out ) {
                auto a = field( o, key );
                out.reserve( count( a, foliageMaximumMeshVertices ) );
                for( auto p = a->child; p; p = p->next )
                {
                    auto v = components( p, 3 );
                    out.emplace_back( v[0], v[1], v[2] );
                }
            };
            read3( "positions", m.positions );
            read3( "normals", m.normals );
            auto a = field( o, "uvs" );
            m.uvs.reserve( count( a, foliageMaximumMeshVertices ) );
            for( auto p = a->child; p; p = p->next )
            {
                auto v = components( p, 2 );
                m.uvs.emplace_back( v[0], v[1] );
            }
            a = field( o, "colours" );
            m.colours.reserve( count( a, foliageMaximumMeshVertices ) );
            for( auto p = a->child; p; p = p->next )
            {
                auto v = components( p, 4 );
                m.colours.emplace_back( v[0], v[1], v[2], v[3] );
            }
            a = field( o, "indices" );
            m.indices.reserve( count( a, foliageMaximumMeshIndices ) );
            for( auto p = a->child; p; p = p->next )
            {
                m.indices.push_back( unsignedValue( p ) );
            }
            a = field( o, "sections" );
            m.sections.reserve( count( a, 64 ) );
            for( auto p = a->child; p; p = p->next )
            {
                m.sections.push_back(
                    { integer( p, "first" ), integer( p, "count" ), integer( p, "material" ) } );
            }
            return m;
        }

        Json speciesJson( const FoliageSpecies &s )
        {
            auto o = object();
            number( o.get(), "version", foliageDataFormatVersion );
            string( o.get(), "id", s.id );
            number( o.get(), "kind", u32( s.kind ) );
            add( o.get(), "minimum", vec( s.boundsMinimum ) );
            add( o.get(), "maximum", vec( s.boundsMaximum ) );
            add( o.get(), "pivot", vec( s.pivot ) );
            auto lods = array(), mats = array();
            for( const auto &m : s.lods )
            {
                append( lods.get(), meshJson( m ) );
            }
            for( const auto &m : s.materials )
            {
                auto j = object();
                string( j.get(), "albedo", m.albedo );
                string( j.get(), "normal", m.normal );
                string( j.get(), "orm", m.orm );
                add( j.get(), "tint", colour( m.tint ) );
                number( j.get(), "roughness", m.roughness );
                number( j.get(), "metalness", m.metalness );
                number( j.get(), "alphaCutoff", m.alphaCutoff );
                boolean( j.get(), "twoSided", m.twoSided );
                append( mats.get(), std::move( j ) );
            }
            add( o.get(), "lods", std::move( lods ) );
            add( o.get(), "materials", std::move( mats ) );
            add( o.get(), "thresholds",
                 floats( { s.screenHeightThresholds[0], s.screenHeightThresholds[1],
                           s.screenHeightThresholds[2] } ) );
            add( o.get(), "density",
                 floats( { s.densityFractions[0], s.densityFractions[1], s.densityFractions[2] } ) );
            number( o.get(), "maximumDistance", s.maximumDistance );
            number( o.get(), "shadowDistance", s.shadowDistance );
            number( o.get(), "hysteresis", s.hysteresis );
            number( o.get(), "shadowLodBias", s.shadowLodBias );
            boolean( o.get(), "castShadows", s.castShadows );
            boolean( o.get(), "farBillboard", s.farBillboard );
            number( o.get(), "collisionRadius", s.collisionRadius );
            number( o.get(), "collisionHeight", s.collisionHeight );
            number( o.get(), "collisionDistance", s.collisionDistance );
            auto wind = object();
            number( wind.get(), "amplitude", s.wind.amplitude );
            number( wind.get(), "frequency", s.wind.frequency );
            number( wind.get(), "flutter", s.wind.flutter );
            number( wind.get(), "bendStart", s.wind.bendStart );
            number( wind.get(), "bendEnd", s.wind.bendEnd );
            number( wind.get(), "maximumInteractionBend", s.wind.maximumInteractionBend );
            add( o.get(), "wind", std::move( wind ) );
            auto imp = object(), views = array();
            string( imp.get(), "albedo", s.impostor.albedo );
            string( imp.get(), "normal", s.impostor.normal );
            string( imp.get(), "orm", s.impostor.orm );
            string( imp.get(), "depth", s.impostor.depth );
            number( imp.get(), "width", s.impostor.width );
            number( imp.get(), "height", s.impostor.height );
            number( imp.get(), "gutter", s.impostor.gutter );
            boolean( imp.get(), "relightable", s.impostor.relightable );
            for( const auto &v : s.impostor.views )
            {
                auto j = object();
                add( j.get(), "direction", vec( v.direction ) );
                add( j.get(), "minimum", floats( { v.uvMinimum.x, v.uvMinimum.y } ) );
                add( j.get(), "maximum", floats( { v.uvMaximum.x, v.uvMaximum.y } ) );
                append( views.get(), std::move( j ) );
            }
            add( imp.get(), "views", std::move( views ) );
            add( o.get(), "impostor", std::move( imp ) );
            return o;
        }

        FoliageSpecies readSpecies( const cJSON *o )
        {
            if( integer( o, "version" ) != foliageDataFormatVersion )
            {
                throw std::invalid_argument( "Unsupported foliage species version." );
            }
            FoliageSpecies s;
            s.id = textField( o, "id", 512 );
            s.kind = FoliageKind( integer( o, "kind" ) );
            s.boundsMinimum = readVec( o, "minimum" );
            s.boundsMaximum = readVec( o, "maximum" );
            s.pivot = readVec( o, "pivot" );
            auto a = field( o, "lods" );
            if( count( a, 3 ) != 3 )
            {
                throw std::invalid_argument( "A species requires three LODs." );
            }
            size_t level = 0;
            for( auto p = a->child; p; p = p->next )
            {
                s.lods[level++] = readMesh( p );
            }
            a = field( o, "materials" );
            s.materials.reserve( count( a, 16 ) );
            for( auto p = a->child; p; p = p->next )
            {
                FoliageMaterial m;
                m.albedo = textField( p, "albedo" );
                m.normal = textField( p, "normal" );
                m.orm = textField( p, "orm" );
                m.tint = readColour( p, "tint" );
                m.roughness = scalar( p, "roughness" );
                m.metalness = scalar( p, "metalness" );
                m.alphaCutoff = scalar( p, "alphaCutoff" );
                m.twoSided = flag( p, "twoSided" );
                s.materials.push_back( std::move( m ) );
            }
            auto thresholds = components( field( o, "thresholds" ), 3 ),
                 density = components( field( o, "density" ), 3 );
            std::copy_n( thresholds.begin(), 3, s.screenHeightThresholds.begin() );
            std::copy_n( density.begin(), 3, s.densityFractions.begin() );
            s.maximumDistance = scalar( o, "maximumDistance" );
            s.shadowDistance = scalar( o, "shadowDistance" );
            s.hysteresis = scalar( o, "hysteresis" );
            s.shadowLodBias = integer( o, "shadowLodBias" );
            s.castShadows = flag( o, "castShadows" );
            s.farBillboard = flag( o, "farBillboard" );
            s.collisionRadius = scalar( o, "collisionRadius" );
            s.collisionHeight = scalar( o, "collisionHeight" );
            s.collisionDistance = scalar( o, "collisionDistance" );
            auto w = field( o, "wind" );
            s.wind.amplitude = scalar( w, "amplitude" );
            s.wind.frequency = scalar( w, "frequency" );
            s.wind.flutter = scalar( w, "flutter" );
            s.wind.bendStart = scalar( w, "bendStart" );
            s.wind.bendEnd = scalar( w, "bendEnd" );
            s.wind.maximumInteractionBend = scalar( w, "maximumInteractionBend" );
            auto i = field( o, "impostor" );
            s.impostor.albedo = textField( i, "albedo" );
            s.impostor.normal = textField( i, "normal" );
            s.impostor.orm = textField( i, "orm" );
            s.impostor.depth = textField( i, "depth" );
            s.impostor.width = integer( i, "width" );
            s.impostor.height = integer( i, "height" );
            s.impostor.gutter = integer( i, "gutter" );
            s.impostor.relightable = flag( i, "relightable" );
            a = field( i, "views" );
            s.impostor.views.reserve( count( a, 64 ) );
            for( auto p = a->child; p; p = p->next )
            {
                FoliageImpostorView v;
                v.direction = readVec( p, "direction" );
                auto mn = components( field( p, "minimum" ), 2 ),
                     mx = components( field( p, "maximum" ), 2 );
                v.uvMinimum = { mn[0], mn[1] };
                v.uvMaximum = { mx[0], mx[1] };
                s.impostor.views.push_back( v );
            }
            return s;
        }

        Json parse( const String &text )
        {
            if( text.empty() || text.size() > foliageMaximumSourceBytes ||
                text.find( '\0' ) != String::npos )
            {
                throw std::invalid_argument(
                    "Foliage JSON is empty, contains NUL, or exceeds 128 MiB." );
            }
            detail::checkFoliageJsonBudget( { text.data(), text.size() } );
            auto p = cJSON_ParseWithLengthOpts( text.c_str(), text.size() + 1, nullptr, 1 );
            if( !p )
            {
                throw std::invalid_argument( "Invalid foliage JSON." );
            }
            return own( p );
        }

        void print( const Json &json, String &output )
        {
            std::unique_ptr<char, decltype( &cJSON_free )> raw( cJSON_PrintUnformatted( json.get() ),
                                                                cJSON_free );
            if( !raw )
            {
                throw std::bad_alloc();
            }
            if( std::strlen( raw.get() ) > foliageMaximumSourceBytes )
            {
                throw std::invalid_argument( "Foliage JSON exceeds 128 MiB." );
            }
            detail::checkFoliageJsonBudget( raw.get() );
            output = raw.get();
        }
    }  // namespace

    bool validateFoliageSpecies( const FoliageSpecies &s, String &error )
    {
        error.clear();
        if( s.id.empty() || !textValid( s.id, 512 ) || u32( s.kind ) > u32( FoliageKind::GroundCover ) ||
            s.materials.empty() || s.materials.size() > 16 )
        {
            return reject(
                error,
                "Foliage species requires a bounded stable ID, supported kind and 1..16 materials." );
        }
        if( !finite( s.boundsMinimum ) || !finite( s.boundsMaximum ) || !finite( s.pivot ) )
        {
            return reject( error, "Foliage bounds/pivot must be finite." );
        }
        for( size_t k = 0; k < 3; ++k )
        {
            if( s.boundsMinimum[k] > s.boundsMaximum[k] ||
                !range( f64( s.boundsMaximum[k] ) - s.boundsMinimum[k], 0, 100000 ) )
            {
                return reject( error, "Foliage bounds must be ordered and at most 100 km per axis." );
            }
        }
        if( s.boundsMaximum == s.boundsMinimum )
        {
            return reject( error, "Foliage bounds cannot collapse to one point." );
        }
        for( const auto &m : s.materials )
        {
            if( !textValid( m.albedo, 4096 ) || !textValid( m.normal, 4096 ) ||
                !textValid( m.orm, 4096 ) || !colourValid( m.tint ) || !range( m.roughness, 0, 1 ) ||
                !range( m.metalness, 0, 1 ) || !range( m.alphaCutoff, 0, 1 ) )
            {
                return reject( error, "Invalid foliage material maps, tint or surface values." );
            }
        }
        for( const auto &m : s.lods )
        {
            if( m.positions.empty() || m.positions.size() > foliageMaximumMeshVertices ||
                m.normals.size() != m.positions.size() || m.uvs.size() != m.positions.size() ||
                ( !m.colours.empty() && m.colours.size() != m.positions.size() ) || m.indices.empty() ||
                m.indices.size() > foliageMaximumMeshIndices || m.indices.size() % 3 ||
                m.sections.size() > 64 )
            {
                return reject( error,
                               "Each foliage LOD needs bounded matching vertex arrays and complete "
                               "indexed triangles." );
            }
            for( size_t n = 0; n < m.positions.size(); ++n )
            {
                const auto &p = m.positions[n], &normal = m.normals[n];
                if( !finite( p ) || !finite( normal ) || !std::isfinite( m.uvs[n].x ) ||
                    !std::isfinite( m.uvs[n].y ) ||
                    ( !m.colours.empty() && !colourValid( m.colours[n] ) ) )
                {
                    return reject( error, "Foliage vertices must be finite." );
                }
                const f64 length =
                    f64( normal.x ) * normal.x + f64( normal.y ) * normal.y + f64( normal.z ) * normal.z;
                if( !range( length, .9, 1.1 ) )
                {
                    return reject( error, "Foliage vertex normals must be normalized." );
                }
                for( size_t k = 0; k < 3; ++k )
                {
                    const f64 tolerance =
                        1.e-4 * std::max( 1.0, f64( s.boundsMaximum[k] ) - s.boundsMinimum[k] );
                    if( p[k] < s.boundsMinimum[k] - tolerance || p[k] > s.boundsMaximum[k] + tolerance )
                    {
                        return reject( error, "Species bounds must contain every LOD vertex." );
                    }
                }
            }
            for( auto index : m.indices )
            {
                if( index >= m.positions.size() )
                {
                    return reject( error, "Foliage mesh index is outside its vertex buffer." );
                }
            }
            u64 end = 0;
            for( const auto &section : m.sections )
            {
                if( section.firstIndex != end || !section.indexCount || section.indexCount % 3 ||
                    section.indexCount > m.indices.size() - end ||
                    section.material >= s.materials.size() )
                {
                    return reject(
                        error,
                        "Foliage sections must cover complete nonoverlapping triangles in order." );
                }
                end += section.indexCount;
            }
            if( !m.sections.empty() && end != m.indices.size() )
            {
                return reject( error, "Foliage sections must cover the complete index buffer." );
            }
        }
        if( !range( s.screenHeightThresholds[0], .000001, 10 ) ||
            !range( s.screenHeightThresholds[1], 0, s.screenHeightThresholds[0] ) ||
            s.screenHeightThresholds[2] != 0 || !range( s.hysteresis, 0, .49 ) ||
            !range( s.maximumDistance, .01, 100000 ) ||
            !range( s.shadowDistance, 0, s.maximumDistance ) || s.shadowLodBias > 2 )
        {
            return reject( error, "Invalid foliage LOD thresholds, hysteresis or range policy." );
        }
        for( size_t n = 0; n < 3; ++n )
        {
            if( !range( s.densityFractions[n], 0, n ? s.densityFractions[n - 1] : 1 ) )
            {
                return reject( error,
                               "Foliage density fractions must decrease monotonically within [0,1]." );
            }
        }
        const auto &w = s.wind;
        if( !range( w.amplitude, 0, 100 ) || !range( w.frequency, 0, 100 ) ||
            !range( w.flutter, 0, 100 ) || !range( w.maximumInteractionBend, 0, 100 ) ||
            !range( w.bendStart, -1.e6, 1.e6 ) || !range( w.bendEnd, -1.e6, 1.e6 ) ||
            f64( w.bendEnd ) - w.bendStart < 1.e-4 )
        {
            return reject( error, "Invalid bounded foliage wind profile." );
        }
        if( !range( s.collisionRadius, 0, 1000 ) || !range( s.collisionHeight, 0, 10000 ) ||
            !range( s.collisionDistance, 0, 100000 ) ||
            ( ( s.collisionRadius == 0 ) != ( s.collisionHeight == 0 ) ) )
        {
            return reject( error, "Foliage collision needs both radius and height, or both zero." );
        }
        const auto &i = s.impostor;
        if( ( s.kind == FoliageKind::Grass || s.kind == FoliageKind::GroundCover ) && s.farBillboard )
        {
            return reject( error,
                           "Grass and ground cover use clump LOD/density, not per-blade impostors." );
        }
        if( s.farBillboard && ( i.views.empty() || i.albedo.empty() ) )
        {
            return reject( error, "Far foliage billboards require baked views and an albedo texture." );
        }
        if( !textValid( i.albedo, 4096 ) || !textValid( i.normal, 4096 ) || !textValid( i.orm, 4096 ) ||
            !textValid( i.depth, 4096 ) || i.views.size() > 64 )
        {
            return reject( error, "Invalid foliage impostor references/views." );
        }
        if( !i.views.empty() )
        {
            if( i.albedo.empty() || ( i.relightable && i.normal.empty() ) || i.width < 4 ||
                i.height < 4 || i.width > 8192 || i.height > 8192 || i.gutter > 64 )
            {
                return reject(
                    error,
                    "Foliage atlas requires bounded dimensions and its declared texture channels." );
            }
            for( const auto &v : i.views )
            {
                const f64 length = f64( v.direction.x ) * v.direction.x +
                                   f64( v.direction.y ) * v.direction.y +
                                   f64( v.direction.z ) * v.direction.z;
                if( !range( length, .99, 1.01 ) || !range( v.uvMinimum.x, 0, 1 ) ||
                    !range( v.uvMinimum.y, 0, 1 ) || !range( v.uvMaximum.x, 0, 1 ) ||
                    !range( v.uvMaximum.y, 0, 1 ) || v.uvMaximum.x <= v.uvMinimum.x ||
                    v.uvMaximum.y <= v.uvMinimum.y )
                {
                    return reject( error, "Invalid foliage atlas view direction or UV rectangle." );
                }
            }
        }
        if( speciesBytes( s ) > foliageMaximumSourceBytes )
        {
            return reject( error, "Foliage species exceeds the 128 MiB source budget." );
        }
        return true;
    }

    bool foliageCellForPosition( const Vector3F &p, f32 pageSize, FoliageCellKey &out, String &error )
    {
        error.clear();
        if( !finite( p ) || !range( pageSize, 1, 4096 ) )
        {
            return reject( error,
                           "Foliage requires finite positions and page size in [1,4096] metres." );
        }
        const f64 x = std::floor( f64( p.x ) / pageSize ), z = std::floor( f64( p.z ) / pageSize );
        if( !range( x, std::numeric_limits<s32>::min(), std::numeric_limits<s32>::max() ) ||
            !range( z, std::numeric_limits<s32>::min(), std::numeric_limits<s32>::max() ) )
        {
            return reject( error, "Foliage cell coordinate exceeds signed 32-bit range." );
        }
        out = { s32( x ), s32( z ) };
        return true;
    }

    u64 foliageCellIdentity( FoliageCellKey c )
    {
        return ( u64( u32( c.x ) ) << 32 ) | u32( c.z );
    }

    u64 foliageStableHash( u64 x )
    {
        x += 0x9e3779b97f4a7c15ull;
        x = ( x ^ ( x >> 30 ) ) * 0xbf58476d1ce4e5b9ull;
        x = ( x ^ ( x >> 27 ) ) * 0x94d049bb133111ebull;
        return x ^ ( x >> 31 );
    }

    bool foliageInstanceRadii( const FoliageWorldData &definition,
                               const Array<FoliageInstance> &instances, f32 &geometryRadius,
                               f32 &bendRadius, String &error )
    {
        double geometry = 0, bend = 0;
        for( const auto &instance : instances )
        {
            if( instance.species >= definition.species.size() || !definition.species[instance.species] )
            {
                return reject( error, "Invalid foliage radius species." );
            }
            const auto &s = *definition.species[instance.species];
            const double largestScale =
                std::max( { instance.scale.x, instance.scale.y, instance.scale.z } );
            const double smallestScale =
                std::min( { instance.scale.x, instance.scale.y, instance.scale.z } );
            if( !std::isfinite( largestScale ) || !std::isfinite( smallestScale ) || smallestScale <= 0 )
            {
                return reject( error, "Invalid foliage radius scale." );
            }
            const auto centre = ( s.boundsMinimum + s.boundsMaximum ) * .5f;
            geometry =
                std::max( geometry, ( double( centre.length() ) +
                                      double( ( s.boundsMaximum - s.boundsMinimum ).length() ) * .5 +
                                      s.wind.flutter * 1.415 ) *
                                        largestScale * 1.7321 );
            bend =
                std::max( bend, ( double( s.wind.amplitude ) + s.wind.maximumInteractionBend * 1.415 ) *
                                    largestScale / smallestScale );
        }
        if( !std::isfinite( geometry ) || !std::isfinite( bend ) ||
            geometry >= std::numeric_limits<f32>::max() || bend >= std::numeric_limits<f32>::max() )
        {
            return reject( error, "Foliage paging extents exceed the numeric range." );
        }
        geometryRadius =
            geometry ? std::nextafter( f32( geometry ), std::numeric_limits<f32>::infinity() ) : 0;
        bendRadius = bend ? std::nextafter( f32( bend ), std::numeric_limits<f32>::infinity() ) : 0;
        error.clear();
        return true;
    }

    u64 foliageSourceBytes( const FoliageWorldData &data )
    {
        u64 bytes = sizeof( data ) + data.instances.capacity() * sizeof( FoliageInstance ) +
                    data.species.capacity() * sizeof( std::shared_ptr<const FoliageSpecies> );
        for( const auto &s : data.species )
        {
            if( s )
            {
                bytes += speciesBytes( *s );
            }
        }
        return bytes;
    }

    namespace
    {
        bool validateInstance( const FoliageInstance &i, const FoliageWorldData &data,
                               FoliageCellKey &cell, String &error )
        {
            if( !i.id || i.species >= data.species.size() || !data.species[i.species] ||
                ( i.flags & ~u32( 15 ) ) || !colourValid( i.tint ) )
            {
                return reject( error,
                               "Foliage instances require nonzero IDs, valid species, flags and tint." );
            }
            if( !foliageCellForPosition( i.position, data.pageSize, cell, error ) )
            {
                return false;
            }
            const auto &q = i.rotation;
            const f64 norm = f64( q.w ) * q.w + f64( q.x ) * q.x + f64( q.y ) * q.y + f64( q.z ) * q.z;
            if( !range( norm, .999, 1.001 ) || !finite( i.scale ) || i.scale.x <= 0 || i.scale.y <= 0 ||
                i.scale.z <= 0 )
            {
                return reject( error,
                               "Foliage rotations must be normalized and scales finite and positive." );
            }
            const auto &s = *data.species[i.species];
            const f64 radius =
                std::max( { std::abs( f64( s.boundsMinimum.x ) ), std::abs( f64( s.boundsMinimum.y ) ),
                            std::abs( f64( s.boundsMinimum.z ) ), std::abs( f64( s.boundsMaximum.x ) ),
                            std::abs( f64( s.boundsMaximum.y ) ),
                            std::abs( f64( s.boundsMaximum.z ) ) } ) *
                    2 +
                s.wind.amplitude + s.wind.flutter + s.wind.maximumInteractionBend;
            const f64 extent =
                radius * std::max( { f64( i.scale.x ), f64( i.scale.y ), f64( i.scale.z ) } );
            if( !range( extent +
                            std::max( { std::abs( f64( i.position.x ) ), std::abs( f64( i.position.y ) ),
                                        std::abs( f64( i.position.z ) ) } ),
                        0, std::numeric_limits<f32>::max() / 2 ) )
            {
                return reject( error, "Foliage transformed bounds overflow renderer coordinates." );
            }
            return true;
        }
    }  // namespace

    bool validateFoliagePage( const FoliagePageData &page, const FoliageWorldData &definition,
                              String &error )
    {
        error.clear();
        if( page.instances.size() > foliageMaximumPageInstances ||
            definition.species.size() > foliageMaximumSpecies || !range( definition.pageSize, 1, 4096 ) )
        {
            return reject( error, "Invalid foliage page/definition bounds." );
        }
        try
        {
            std::unordered_set<u64> ids;
            ids.reserve( page.instances.size() );
            for( const auto &i : page.instances )
            {
                FoliageCellKey cell;
                if( !validateInstance( i, definition, cell, error ) )
                {
                    return false;
                }
                if( cell != page.cell || !ids.insert( i.id ).second )
                {
                    return reject( error,
                                   "Foliage page instances must have unique identities and belong to "
                                   "the page cell." );
                }
            }
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool validateFoliageWorld( const FoliageWorldData &data, String &error )
    {
        error.clear();
        if( !range( data.pageSize, 1, 4096 ) || data.species.size() > foliageMaximumSpecies ||
            data.instances.size() > foliageMaximumInstances ||
            foliageSourceBytes( data ) > foliageMaximumSourceBytes )
        {
            return reject(
                error, "Foliage world exceeds page-size, species, population or source-memory bounds." );
        }
        try
        {
            std::set<String> speciesIds;
            std::unordered_set<u64> ids;
            std::map<FoliageCellKey, u32> cells;
            for( const auto &s : data.species )
            {
                if( !s || !validateFoliageSpecies( *s, error ) || !speciesIds.insert( s->id ).second )
                {
                    if( error.empty() )
                    {
                        error = "Foliage species are null or their stable IDs are duplicated.";
                    }
                    return false;
                }
            }
            ids.reserve( data.instances.size() );
            for( const auto &i : data.instances )
            {
                FoliageCellKey cell;
                if( !validateInstance( i, data, cell, error ) )
                {
                    return false;
                }
                if( !ids.insert( i.id ).second )
                {
                    return reject( error, "Foliage instance identities must be unique." );
                }
                if( ++cells[cell] > foliageMaximumPageInstances || cells.size() > foliageMaximumPages )
                {
                    return reject( error, "Foliage world exceeds per-page population or cell bounds." );
                }
            }
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool serializeFoliageSpecies( const FoliageSpecies &s, String &out, String &error )
    {
        try
        {
            if( !validateFoliageSpecies( s, error ) )
            {
                return false;
            }
            detail::FoliageJsonEstimate budget;
            estimateSpeciesJson( s, budget );
            auto root = speciesJson( s );
            print( root, out );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool deserializeFoliageSpecies( const String &text, FoliageSpecies &out, String &error )
    {
        try
        {
            auto root = parse( text );
            auto s = readSpecies( root.get() );
            if( !validateFoliageSpecies( s, error ) )
            {
                return false;
            }
            out = std::move( s );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool serializeFoliageWorld( const FoliageWorldData &data, String &out, String &error )
    {
        try
        {
            if( !validateFoliageWorld( data, error ) )
            {
                return false;
            }
            detail::FoliageJsonEstimate budget;
            budget.add( 16 + data.instances.size() * 32 );
            for( const auto &s : data.species )
            {
                estimateSpeciesJson( *s, budget );
            }
            auto root = object(), species = array(), instances = array();
            number( root.get(), "version", foliageDataFormatVersion );
            number( root.get(), "pageSize", data.pageSize );
            for( const auto &s : data.species )
            {
                append( species.get(), speciesJson( *s ) );
            }
            for( const auto &i : data.instances )
            {
                auto j = object();
                string( j.get(), "id", std::to_string( i.id ) );
                number( j.get(), "species", i.species );
                add( j.get(), "position", vec( i.position ) );
                add( j.get(), "rotation",
                     floats( { i.rotation.w, i.rotation.x, i.rotation.y, i.rotation.z } ) );
                add( j.get(), "scale", vec( i.scale ) );
                add( j.get(), "tint", colour( i.tint ) );
                number( j.get(), "seed", i.seed );
                number( j.get(), "flags", i.flags );
                append( instances.get(), std::move( j ) );
            }
            add( root.get(), "species", std::move( species ) );
            add( root.get(), "instances", std::move( instances ) );
            print( root, out );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool deserializeFoliageWorld( const String &text, FoliageWorldData &out, String &error )
    {
        try
        {
            auto root = parse( text );
            if( integer( root.get(), "version" ) != foliageDataFormatVersion )
            {
                return reject( error, "Unsupported foliage world version." );
            }
            FoliageWorldData data;
            data.pageSize = scalar( root.get(), "pageSize" );
            auto a = field( root.get(), "species" );
            data.species.reserve( count( a, foliageMaximumSpecies ) );
            for( auto p = a->child; p; p = p->next )
            {
                data.species.push_back( std::make_shared<const FoliageSpecies>( readSpecies( p ) ) );
            }
            a = field( root.get(), "instances" );
            data.instances.reserve( count( a, foliageMaximumInstances ) );
            for( auto p = a->child; p; p = p->next )
            {
                FoliageInstance i;
                auto id = textField( p, "id", 20 );
                const auto parsed = std::from_chars( id.data(), id.data() + id.size(), i.id );
                if( parsed.ec != std::errc{} || parsed.ptr != id.data() + id.size() )
                {
                    return reject( error, "Invalid foliage instance uint64 identity." );
                }
                i.species = integer( p, "species" );
                i.position = readVec( p, "position" );
                auto q = components( field( p, "rotation" ), 4 );
                i.rotation = { q[0], q[1], q[2], q[3] };
                i.scale = readVec( p, "scale" );
                i.tint = readColour( p, "tint" );
                i.seed = integer( p, "seed" );
                i.flags = integer( p, "flags" );
                data.instances.push_back( i );
            }
            if( !validateFoliageWorld( data, error ) )
            {
                return false;
            }
            out = std::move( data );
            return true;  // A saved revision never becomes a live publication token.
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
}  // namespace workphone::render
