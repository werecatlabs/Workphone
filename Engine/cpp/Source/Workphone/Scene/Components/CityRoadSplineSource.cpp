#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/CityRoadSplineSource.hpp>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <stdexcept>

namespace workphone::procedural
{
    namespace
    {
        constexpr size_t maxBytes = 1024 * 1024, maxKnots = 4096, maxRoads = 128;
        constexpr char schema[] = "workphone.city-road-source";

        void require( bool ok, const char *message )
        {
            if( !ok )
            {
                throw std::runtime_error( message );
            }
        }

        bool finite( Vector3F p )
        {
            return std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z ) &&
                   std::abs( p.x ) <= 1000000 && std::abs( p.y ) <= 1000000 &&
                   std::abs( p.z ) <= 1000000;
        }

        bool guid( const SourceBudgetText &s )
        {
            if( s.size() != 36 )
            {
                return false;
            }
            for( size_t i = 0; i < s.size(); ++i )
            {
                if( i == 8 || i == 13 || i == 18 || i == 23 )
                {
                    if( s[i] != '-' )
                    {
                        return false;
                    }
                }
                else if( !( ( s[i] >= '0' && s[i] <= '9' ) || ( s[i] >= 'a' && s[i] <= 'f' ) ) )
                {
                    return false;
                }
            }
            return s != "00000000-0000-0000-0000-000000000000";
        }

        // RapidJSON DOM, parse stack and output buffer are charged to SourceCpu too.
        struct JsonAllocator
        {
            static constexpr bool kNeedFree = true;
            struct Block;

            struct State
            {
                size_t bytes = 0;
                Block *head = nullptr;
                ~State();
            };

            struct alignas( std::max_align_t ) Block
            {
                State *state;
                size_t size;
                Block *previous, *next;
            };

            State *state;

            explicit JsonAllocator( State *value = nullptr ) : state( value )
            {
            }

            void *Malloc( size_t size )
            {
                if( !size )
                {
                    return nullptr;
                }
                if( !state || size > 8 * maxBytes - state->bytes )
                {
                    throw std::bad_alloc();
                }
                auto block = static_cast<Block *>(
                    allocateSourceBudgetStorage( sizeof( Block ) + size, alignof( Block ) ) );
                block->state = state;
                block->size = size;
                block->previous = nullptr;
                block->next = state->head;
                if( state->head )
                {
                    state->head->previous = block;
                }
                state->head = block;
                state->bytes += size;
                return block + 1;
            }

            static void Free( void *p )
            {
                if( p )
                {
                    auto b = static_cast<Block *>( p ) - 1;
                    if( b->previous )
                    {
                        b->previous->next = b->next;
                    }
                    else
                    {
                        b->state->head = b->next;
                    }
                    if( b->next )
                    {
                        b->next->previous = b->previous;
                    }
                    b->state->bytes -= b->size;
                    freeSourceBudgetStorage( b, alignof( Block ) );
                }
            }

            void *Realloc( void *p, size_t oldSize, size_t size )
            {
                if( !size )
                {
                    Free( p );
                    return nullptr;
                }
                auto next = Malloc( size );
                if( p )
                {
                    std::memcpy( next, p, std::min( oldSize, size ) );
                }
                Free( p );
                return next;
            }
        };

        // Some bundled RapidJSON Pop-before-allocate paths abandon children when
        // an allocator throws. Retire those blocks after DOM/stack destruction.
        JsonAllocator::State::~State()
        {
            while( head )
            {
                JsonAllocator::Free( head + 1 );
            }
        }

        using Json = rapidjson::GenericValue<rapidjson::UTF8<>, JsonAllocator>;
        using Document = rapidjson::GenericDocument<rapidjson::UTF8<>, JsonAllocator, JsonAllocator>;
        using Buffer = rapidjson::GenericStringBuffer<rapidjson::UTF8<>, JsonAllocator>;

        void fields( const Json &v, std::initializer_list<const char *> names )
        {
            require( v.IsObject() && v.MemberCount() == names.size(),
                     "Unknown, missing or duplicate city source field." );
            for( auto name : names )
            {
                require( v.HasMember( name ), "Missing city source field." );
            }
        }

        SourceBudgetText text( const Json &v, bool optional = false )
        {
            require( v.IsString() &&
                         ( v.GetStringLength() == 36 || ( optional && v.GetStringLength() == 0 ) ),
                     "City source requires canonical GUID strings." );
            SourceBudgetText result( v.GetString(), v.GetStringLength() );
            require( ( optional && result.empty() ) || guid( result ),
                     "City source requires lowercase GUIDs." );
            return result;
        }

        f32 number( const Json &v )
        {
            require(
                v.IsNumber() && std::isfinite( v.GetDouble() ) && std::abs( v.GetDouble() ) <= 1000000,
                "Invalid city spline number." );
            return f32( v.GetDouble() );
        }

        Vector3F vector( const Json &v )
        {
            require( v.IsArray() && v.Size() == 3, "City spline vector must have three values." );
            return { number( v[0] ), number( v[1] ), number( v[2] ) };
        }

        template <class Writer>
        void writeVector( Writer &w, Vector3F p )
        {
            w.StartArray();
            w.Double( p.x );
            w.Double( p.y );
            w.Double( p.z );
            w.EndArray();
        }

        void validate( const CityRoadSplineSource &s )
        {
            require( ( s.version == 1 || s.version == 2 ) && guid( s.id ),
                     "Unsupported city source version or invalid source GUID." );
            require( s.terrainResourceUuid.empty() || guid( s.terrainResourceUuid ),
                     "Invalid terrain resource GUID." );
            require( ( s.version == 2 || !s.roads.empty() ) && s.roads.size() <= maxRoads,
                     "City source road count exceeds its version/budget." );
            require( s.roundabouts.size() <= 8 && ( s.version == 2 || s.roundabouts.empty() ),
                     "Roundabouts require source version 2 and at most eight junctions." );
            SourceBudgetSet<SourceBudgetText> ids, roadIds;
            ids.insert( s.id );
            size_t count = 0;
            for( const auto &r : s.roads )
            {
                require( guid( r.id ) && ids.insert( r.id ).second, "Invalid or duplicate road GUID." );
                roadIds.insert( r.id );
                require( std::isfinite( r.width ) && r.width >= .5f && r.width <= 60 &&
                             std::isfinite( r.maxGrade ) && r.maxGrade > 0 && r.maxGrade <= .35f &&
                             std::isfinite( r.heightOffset ) && std::abs( r.heightOffset ) <= 1000,
                         "Invalid road width, grade or offset." );
                require(
                    r.heightMode == CityRoadHeightMode::Drape ||
                        r.heightMode == CityRoadHeightMode::Fixed,
                    "Bridge height mode is unavailable; deck/support/collision policy is required." );
                require( r.heightMode != CityRoadHeightMode::Drape || !s.terrainResourceUuid.empty(),
                         "Draped roads require a terrain resource GUID." );
                require( r.knots.size() >= ( r.closed ? 3u : 2u ) && r.knots.size() <= 256 &&
                             ( count += r.knots.size() ) <= maxKnots,
                         "City spline knot budget exceeded." );
                for( size_t i = 0; i < r.knots.size(); ++i )
                {
                    const auto &k = r.knots[i];
                    require( guid( k.id ) && ids.insert( k.id ).second,
                             "Invalid or duplicate control-point GUID." );
                    require( finite( k.position ) && finite( k.inHandle ) && finite( k.outHandle ) &&
                                 finite( k.position + k.inHandle ) && finite( k.position + k.outHandle ),
                             "Nonfinite or excessive Bezier control." );
                    if( i || r.closed )
                    {
                        require( ( k.position -
                                   r.knots[( i + r.knots.size() - 1 ) % r.knots.size()].position )
                                         .lengthSquared() > .0001f,
                                 "Adjacent spline knots must be distinct." );
                    }
                }
            }
            for( const auto &j : s.roundabouts )
            {
                require( guid( j.junctionId ) && ids.insert( j.junctionId ).second,
                         "Invalid or duplicate junction GUID." );
                require( finite( j.center ) && std::isfinite( j.outerRadius ) &&
                             std::isfinite( j.islandRadius ) && j.outerRadius > j.islandRadius &&
                             j.islandRadius > 0 && j.ports.size() >= 3 && j.ports.size() <= 6,
                         "Invalid roundabout dimensions or approach count." );
                require( std::isfinite( j.gradeX ) && std::isfinite( j.gradeZ ) &&
                             std::isfinite( j.maxGrade ) && std::isfinite( j.maxTerrainDeviation ) &&
                             std::isfinite( j.islandHeight ) && std::isfinite( j.kerbWidth ) &&
                             std::isfinite( j.terrainSampleStep ),
                         "Nonfinite roundabout profile." );
                require( j.outerRadius >= 6 && j.outerRadius <= 128 && j.islandRadius >= 1 &&
                             j.outerRadius - j.islandRadius >= 3 &&
                             j.outerRadius - j.islandRadius <= 7 && j.islandHeight >= .05f &&
                             j.islandHeight <= .4f && j.kerbWidth >= .1f && j.kerbWidth <= .5f &&
                             j.kerbWidth < j.islandRadius * .5f && j.ringSamples >= 32 &&
                             j.ringSamples <= 256 && j.maxGrade > 0 && j.maxGrade <= .15f &&
                             std::hypot( j.gradeX, j.gradeZ ) <= j.maxGrade &&
                             j.maxTerrainDeviation >= 0 && j.maxTerrainDeviation <= 2 &&
                             j.terrainSampleStep >= .25f && j.terrainSampleStep <= 4 &&
                             j.maximumTerrainSamples >= 32 && j.maximumTerrainSamples <= 65536,
                         "Unsupported roundabout dimensions, grade or sampling budget." );
                SourceBudgetSet<SourceBudgetText> ports;
                for( const auto &p : j.ports )
                {
                    require(
                        guid( p.approachId ) && ports.insert( p.approachId ).second &&
                            roadIds.count( p.approachId ) && finite( p.outward ) &&
                            std::isfinite( p.width ) && std::isfinite( p.length ),
                        "Roundabout port requires a unique existing road GUID and finite profile." );
                }
                for( size_t i = 0; i < j.ports.size(); ++i )
                {
                    const auto &p = j.ports[i];
                    const float run = std::hypot( p.outward.x, p.outward.z );
                    require( std::abs( p.outward.y ) < .0001f && run > .001f && p.width >= 4 &&
                                 p.width <= 12 && p.width < j.outerRadius && p.length >= 4 &&
                                 p.length <= 256 &&
                                 std::sqrt( j.outerRadius * j.outerRadius - p.width * p.width * .25f ) -
                                         j.islandRadius >=
                                     3,
                             "Unsupported roundabout approach profile." );
                    for( size_t k = 0; k < i; ++k )
                    {
                        const auto &q = j.ports[k];
                        const float qr = std::hypot( q.outward.x, q.outward.z );
                        const float cosine = std::clamp(
                            ( p.outward.x * q.outward.x + p.outward.z * q.outward.z ) / ( run * qr ),
                            -1.f, 1.f );
                        require( std::acos( cosine ) >= std::asin( p.width * .5f / j.outerRadius ) +
                                                            std::asin( q.width * .5f / j.outerRadius ) +
                                                            .12f,
                                 "Roundabout approach mouths overlap." );
                    }
                }
            }
        }

        void jsonDepth( std::string_view s )
        {
            int depth = 0;
            bool quoted = false, escaped = false;
            for( char c : s )
            {
                require( c != '\0', "Embedded NUL in city source." );
                if( quoted )
                {
                    if( escaped )
                    {
                        escaped = false;
                    }
                    else if( c == '\\' )
                    {
                        escaped = true;
                    }
                    else if( c == '\"' )
                    {
                        quoted = false;
                    }
                }
                else if( c == '\"' )
                {
                    quoted = true;
                }
                else if( c == '{' || c == '[' )
                {
                    require( ++depth <= 24, "City source nesting budget exceeded." );
                }
                else if( c == '}' || c == ']' )
                {
                    --depth;
                }
            }
        }

        float distanceToChord( Vector3F p, Vector3F a, Vector3F b )
        {
            auto d = b - a;
            const auto n = d.lengthSquared();
            return ( p - ( a + d * ( n > 1e-12f ? std::clamp( ( p - a ).dotProduct( d ) / n, 0.f, 1.f )
                                                : 0.f ) ) )
                .length();
        }

        void flatten( Vector3F a, Vector3F b, Vector3F c, Vector3F d, unsigned depth,
                      const CityRoadSplineSampling &o, SourceBudgetVector<CityRoadSplineSample> &out,
                      size_t &total )
        {
            const bool fit =
                std::max( distanceToChord( b, a, d ), distanceToChord( c, a, d ) ) <= o.tolerance &&
                ( b - a ).length() + ( c - b ).length() + ( d - c ).length() <= o.maxStep;
            if( fit )
            {
                require( ++total <= o.maximumSamples, "City spline sample budget exceeded." );
                CityRoadSplineSample p;
                p.position = d;
                out.push_back( p );
                return;
            }
            require( depth < 20,
                     "Bezier tolerance could not be reached within the subdivision budget." );
            const auto ab = ( a + b ) * .5f, bc = ( b + c ) * .5f, cd = ( c + d ) * .5f,
                       abc = ( ab + bc ) * .5f, bcd = ( bc + cd ) * .5f, m = ( abc + bcd ) * .5f;
            flatten( a, ab, abc, m, depth + 1, o, out, total );
            flatten( m, bcd, cd, d, depth + 1, o, out, total );
        }

        void grade( Vector3F a, Vector3F b, float limit )
        {
            const float run = std::hypot( b.x - a.x, b.z - a.z );
            require( run > 1e-5f && std::abs( b.y - a.y ) <= limit * run + 1e-5f,
                     "Road profile exceeds grade or has zero horizontal span." );
        }
    }  // namespace

    bool validateCityRoadSplineSource( const CityRoadSplineSource &s, String &error )
    {
        try
        {
            validate( s );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool deserializeCityRoadSplineSource( std::string_view bytes, CityRoadSplineSource &output,
                                          String &error )
    {
        try
        {
            require( !bytes.empty() && bytes.size() <= maxBytes,
                     "City source exceeds the 1 MiB input budget." );
            jsonDepth( bytes );
            JsonAllocator::State scratch;
            JsonAllocator allocator( &scratch );
            Document d( &allocator, 1024, &allocator );
            d.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseIterativeFlag>(
                bytes.data(), bytes.size() );
            require( !d.HasParseError(), "Invalid UTF-8 city source JSON." );
            require( d.IsObject() && d.HasMember( "version" ) && d["version"].IsUint(),
                     "Missing city source version." );
            const auto version = d["version"].GetUint();
            if( version == 1 )
            {
                fields( d, { "schema", "version", "id", "revision", "terrain", "roads" } );
            }
            else if( version == 2 )
            {
                fields( d,
                        { "schema", "version", "id", "revision", "terrain", "roads", "roundabouts" } );
            }
            else
            {
                require( false, "Unsupported city source version." );
            }
            require( d["schema"].IsString() && d["schema"].GetStringLength() == sizeof( schema ) - 1 &&
                         std::memcmp( d["schema"].GetString(), schema, sizeof( schema ) - 1 ) == 0 &&
                         d["version"].IsUint(),
                     "Unsupported city source schema/version." );
            require( d["revision"].IsUint64(), "Invalid city source revision." );
            CityRoadSplineSource next;
            next.version = version;
            next.id = text( d["id"] );
            next.revision = d["revision"].GetUint64();
            const auto &t = d["terrain"];
            fields( t, { "resourceUuid", "observedRevision" } );
            next.terrainResourceUuid = text( t["resourceUuid"], true );
            require( t["observedRevision"].IsUint64(), "Invalid terrain revision." );
            next.terrainRevision = t["observedRevision"].GetUint64();
            require( d["roads"].IsArray() && d["roads"].Size() <= maxRoads, "Invalid city road array." );
            size_t knots = 0;
            for( const auto &v : d["roads"].GetArray() )
            {
                fields( v,
                        { "id", "width", "closed", "heightMode", "maxGrade", "heightOffset", "knots" } );
                CityRoadSpline r;
                r.id = text( v["id"] );
                r.width = number( v["width"] );
                r.maxGrade = number( v["maxGrade"] );
                r.heightOffset = number( v["heightOffset"] );
                require( v["closed"].IsBool() && v["heightMode"].IsString(),
                         "Invalid road closure or height mode." );
                r.closed = v["closed"].GetBool();
                const std::string_view mode( v["heightMode"].GetString(),
                                             v["heightMode"].GetStringLength() );
                require( mode == "Drape" || mode == "Fixed",
                         "Bridge/unknown height mode is unavailable." );
                r.heightMode = mode == "Drape" ? CityRoadHeightMode::Drape : CityRoadHeightMode::Fixed;
                require( v["knots"].IsArray() && v["knots"].Size() <= 256 &&
                             ( knots += v["knots"].Size() ) <= maxKnots,
                         "City spline knot budget exceeded." );
                for( const auto &p : v["knots"].GetArray() )
                {
                    fields( p, { "id", "position", "inHandle", "outHandle" } );
                    r.knots.push_back( { text( p["id"] ), vector( p["position"] ),
                                         vector( p["inHandle"] ), vector( p["outHandle"] ) } );
                }
                next.roads.push_back( std::move( r ) );
            }
            if( version == 2 )
            {
                const auto &junctions = d["roundabouts"];
                require( junctions.IsArray() && junctions.Size() <= 8, "Roundabout budget exceeded." );
                for( const auto &v : junctions.GetArray() )
                {
                    fields( v, { "id", "center", "outerRadius", "islandRadius", "islandHeight",
                                 "kerbWidth", "clockwise", "ringSamples", "gradeX", "gradeZ", "maxGrade",
                                 "maxTerrainDeviation", "terrainSampleStep", "maximumTerrainSamples",
                                 "ports" } );
                    CityRoadRoundaboutSpec j;
                    j.junctionId = text( v["id"] );
                    j.center = vector( v["center"] );
                    j.outerRadius = number( v["outerRadius"] );
                    j.islandRadius = number( v["islandRadius"] );
                    j.islandHeight = number( v["islandHeight"] );
                    j.kerbWidth = number( v["kerbWidth"] );
                    j.gradeX = number( v["gradeX"] );
                    j.gradeZ = number( v["gradeZ"] );
                    j.maxGrade = number( v["maxGrade"] );
                    j.maxTerrainDeviation = number( v["maxTerrainDeviation"] );
                    j.terrainSampleStep = number( v["terrainSampleStep"] );
                    require( v["clockwise"].IsBool() && v["ringSamples"].IsUint() &&
                                 v["maximumTerrainSamples"].IsUint(),
                             "Invalid roundabout sampling or circulation." );
                    j.clockwise = v["clockwise"].GetBool();
                    j.ringSamples = v["ringSamples"].GetUint();
                    j.maximumTerrainSamples = v["maximumTerrainSamples"].GetUint();
                    require( v["ports"].IsArray() && v["ports"].Size() <= 6,
                             "Invalid roundabout port array." );
                    for( const auto &p : v["ports"].GetArray() )
                    {
                        fields( p, { "roadId", "outward", "width", "length" } );
                        j.ports.push_back( { text( p["roadId"] ), vector( p["outward"] ),
                                             number( p["width"] ), number( p["length"] ) } );
                    }
                    next.roundabouts.push_back( std::move( j ) );
                }
            }
            validate( next );
            output = std::move( next );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool serializeCityRoadSplineSource( const CityRoadSplineSource &s, SourceBudgetText &output,
                                        String &error )
    {
        try
        {
            validate( s );
            JsonAllocator::State scratch;
            JsonAllocator allocator( &scratch );
            Buffer buffer( &allocator );
            rapidjson::Writer<Buffer, rapidjson::UTF8<>, rapidjson::UTF8<>, JsonAllocator> w(
                buffer, &allocator );
            w.StartObject();
            w.Key( "schema" );
            w.String( schema );
            w.Key( "version" );
            w.Uint( s.version );
            w.Key( "id" );
            w.String( s.id.c_str() );
            w.Key( "revision" );
            w.Uint64( s.revision );
            w.Key( "terrain" );
            w.StartObject();
            w.Key( "resourceUuid" );
            w.String( s.terrainResourceUuid.c_str() );
            w.Key( "observedRevision" );
            w.Uint64( s.terrainRevision );
            w.EndObject();
            w.Key( "roads" );
            w.StartArray();
            for( const auto &r : s.roads )
            {
                w.StartObject();
                w.Key( "id" );
                w.String( r.id.c_str() );
                w.Key( "width" );
                w.Double( r.width );
                w.Key( "closed" );
                w.Bool( r.closed );
                w.Key( "heightMode" );
                w.String( r.heightMode == CityRoadHeightMode::Drape ? "Drape" : "Fixed" );
                w.Key( "maxGrade" );
                w.Double( r.maxGrade );
                w.Key( "heightOffset" );
                w.Double( r.heightOffset );
                w.Key( "knots" );
                w.StartArray();
                for( const auto &p : r.knots )
                {
                    w.StartObject();
                    w.Key( "id" );
                    w.String( p.id.c_str() );
                    w.Key( "position" );
                    writeVector( w, p.position );
                    w.Key( "inHandle" );
                    writeVector( w, p.inHandle );
                    w.Key( "outHandle" );
                    writeVector( w, p.outHandle );
                    w.EndObject();
                }
                w.EndArray();
                w.EndObject();
            }
            w.EndArray();
            if( s.version == 2 )
            {
                w.Key( "roundabouts" );
                w.StartArray();
                for( const auto &j : s.roundabouts )
                {
                    w.StartObject();
                    w.Key( "id" );
                    w.String( j.junctionId.c_str() );
                    w.Key( "center" );
                    writeVector( w, j.center );
                    w.Key( "outerRadius" );
                    w.Double( j.outerRadius );
                    w.Key( "islandRadius" );
                    w.Double( j.islandRadius );
                    w.Key( "islandHeight" );
                    w.Double( j.islandHeight );
                    w.Key( "kerbWidth" );
                    w.Double( j.kerbWidth );
                    w.Key( "clockwise" );
                    w.Bool( j.clockwise );
                    w.Key( "ringSamples" );
                    w.Uint( j.ringSamples );
                    w.Key( "gradeX" );
                    w.Double( j.gradeX );
                    w.Key( "gradeZ" );
                    w.Double( j.gradeZ );
                    w.Key( "maxGrade" );
                    w.Double( j.maxGrade );
                    w.Key( "maxTerrainDeviation" );
                    w.Double( j.maxTerrainDeviation );
                    w.Key( "terrainSampleStep" );
                    w.Double( j.terrainSampleStep );
                    w.Key( "maximumTerrainSamples" );
                    w.Uint( j.maximumTerrainSamples );
                    w.Key( "ports" );
                    w.StartArray();
                    for( const auto &p : j.ports )
                    {
                        w.StartObject();
                        w.Key( "roadId" );
                        w.String( p.approachId.c_str() );
                        w.Key( "outward" );
                        writeVector( w, p.outward );
                        w.Key( "width" );
                        w.Double( p.width );
                        w.Key( "length" );
                        w.Double( p.length );
                        w.EndObject();
                    }
                    w.EndArray();
                    w.EndObject();
                }
                w.EndArray();
            }
            w.EndObject();
            require( buffer.GetSize() <= maxBytes, "City source exceeds the 1 MiB output budget." );
            SourceBudgetText next( buffer.GetString(), buffer.GetSize() );
            output.swap( next );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool sampleCityRoadSplines( const CityRoadSplineSource &s, const CityRoadSplineSampling &o,
                                const CityRoadHeightSampler &height, CityRoadSplineBake &output,
                                String &error )
    {
        try
        {
            validate( s );
            require( std::isfinite( o.tolerance ) && o.tolerance >= .001f && o.tolerance <= 2 &&
                         std::isfinite( o.maxStep ) && o.maxStep >= .05f && o.maxStep <= 50 &&
                         o.maximumSamples >= 2 && o.maximumSamples <= 65536,
                     "Invalid city spline sampling limits." );
            CityRoadSplineBake next;
            next.sourceRevision = s.revision;
            size_t total = 0;
            for( const auto &r : s.roads )
            {
                CityRoadSplineSamples result;
                result.roadId = r.id;
                CityRoadSplineSample first;
                first.position = r.knots.front().position;
                require( ++total <= o.maximumSamples, "City spline sample budget exceeded." );
                result.points.push_back( first );
                for( size_t i = 0; i < ( r.closed ? r.knots.size() : r.knots.size() - 1 ); ++i )
                {
                    const auto &a = r.knots[i], &b = r.knots[( i + 1 ) % r.knots.size()];
                    flatten( a.position, a.position + a.outHandle, b.position + b.inHandle, b.position,
                             0, o, result.points, total );
                }
                const auto fit = [&]( Vector3F &p ) {
                    if( r.heightMode == CityRoadHeightMode::Drape )
                    {
                        f32 y = 0;
                        require( height && height( p, y ) && std::isfinite( y ),
                                 "Terrain sample is outside, a hole or unavailable." );
                        p.y = y;
                    }
                    p.y += r.heightOffset;
                    require( finite( p ), "Fitted road sample is nonfinite or excessive." );
                };
                for( auto &p : result.points )
                {
                    fit( p.position );
                }
                for( size_t i = 0; i < result.points.size(); ++i )
                {
                    auto &p = result.points[i];
                    const size_t previous = i ? i - 1 : r.closed ? result.points.size() - 2 : 0;
                    const size_t after = i + 1 < result.points.size() ? i + 1 : r.closed ? 1 : i;
                    auto tangent = result.points[after].position - result.points[previous].position;
                    if( !r.closed && ( i == 0 || i + 1 == result.points.size() ) )
                    {
                        const auto authored =
                            i == 0 ? r.knots.front().outHandle : -r.knots.back().inHandle;
                        const float authoredRun = std::hypot( authored.x, authored.z ),
                                    sampledRun = std::hypot( tangent.x, tangent.z );
                        if( authoredRun > 1e-5f && sampledRun > 1e-5f )
                        {
                            const float rise = r.heightMode == CityRoadHeightMode::Drape
                                                   ? tangent.y / sampledRun * authoredRun
                                                   : authored.y;
                            tangent = { authored.x, rise, authored.z };
                        }
                    }
                    const float run = std::hypot( tangent.x, tangent.z );
                    require( run > 1e-5f, "Spline tangent has zero horizontal span." );
                    p.tangent = tangent.normaliseCopy();
                    const Vector3F side( tangent.z / run, 0, -tangent.x / run );
                    p.left = p.position + side * ( r.width * .5f );
                    p.right = p.position - side * ( r.width * .5f );
                    // Fixed cross-sections retain authored centre height; Drape samples both edges.
                    if( r.heightMode == CityRoadHeightMode::Drape )
                    {
                        p.left.y = p.right.y = 0;
                        fit( p.left );
                        fit( p.right );
                        grade( p.left, p.position, r.maxGrade );
                        grade( p.position, p.right, r.maxGrade );
                    }
                    if( i )
                    {
                        const auto &a = result.points[i - 1];
                        const auto step = p.position - a.position;
                        const auto advances = [&]( Vector3F from, Vector3F to ) {
                            return ( to.x - from.x ) * step.x + ( to.z - from.z ) * step.z >
                                   1e-5f * ( step.x * step.x + step.z * step.z );
                        };
                        require(
                            advances( a.left, p.left ) && advances( a.right, p.right ),
                            "Road width folds a local offset edge; reduce width or loosen the curve." );
                        grade( a.position, p.position, r.maxGrade );
                        grade( a.left, p.left, r.maxGrade );
                        grade( a.right, p.right, r.maxGrade );
                        p.distance = a.distance + step.length();
                    }
                    require( finite( p.left ) && finite( p.right ) && std::isfinite( p.distance ),
                             "Invalid sampled road cross-section." );
                }
                next.roads.push_back( std::move( result ) );
            }
            output = std::move( next );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
}  // namespace workphone::procedural
