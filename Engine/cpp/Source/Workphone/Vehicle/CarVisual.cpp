#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Vehicle/CarVisual.hpp>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <stdexcept>

namespace workphone::vehiclevisual
{
    namespace
    {
        constexpr float pi = 3.14159265358979323846f;

        Vec3 plus( Vec3 a, Vec3 b )
        {
            return { a.x + b.x, a.y + b.y, a.z + b.z };
        }

        Vec3 sub( Vec3 a, Vec3 b )
        {
            return { a.x - b.x, a.y - b.y, a.z - b.z };
        }

        Vec3 mul( Vec3 a, float s )
        {
            return { a.x * s, a.y * s, a.z * s };
        }

        bool finite( Vec3 p )
        {
            return std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z );
        }

        Vec3 yaw( Vec3 p, float a )
        {
            return { p.x * std::cos( a ) + p.z * std::sin( a ), p.y,
                     -p.x * std::sin( a ) + p.z * std::cos( a ) };
        }

        float unit( float f )
        {
            return std::isfinite( f ) ? std::clamp( f, 0.f, 1.f ) : 0.f;
        }

        bool named( std::string name, const char *words )
        {
            std::transform( name.begin(), name.end(), name.begin(),
                            []( unsigned char c ) { return char( std::tolower( c ) ); } );
            std::string list( words );
            for( std::size_t a = 0, b; a < list.size(); a = b + 1 )
            {
                b = list.find( '|', a );
                if( b == std::string::npos )
                {
                    b = list.size();
                }
                if( name.find( list.substr( a, b - a ) ) != std::string::npos )
                {
                    return true;
                }
            }
            return false;
        }

        Bounds bounds( const Mesh &m )
        {
            Bounds b;
            for( const auto &v : m.vertices )
            {
                b.include( v.position );
            }
            return b;
        }

        void validate( const Mesh &m )
        {
            if( m.vertices.empty() || m.indices.empty() || m.indices.size() % 3 )
            {
                throw std::invalid_argument( "CarVisual requires indexed triangle meshes" );
            }
            for( const auto &v : m.vertices )
            {
                if( !finite( v.position ) || !finite( v.normal ) || !finite( v.tangent ) )
                {
                    throw std::invalid_argument( "CarVisual mesh contains non-finite vertices" );
                }
            }
            for( auto i : m.indices )
            {
                if( i >= m.vertices.size() )
                {
                    throw std::invalid_argument( "CarVisual index outside vertex buffer" );
                }
            }
        }

        void transform( Mesh &mesh, float angle, float scale, Vec3 offset, bool mirror = false )
        {
            for( auto &v : mesh.vertices )
            {
                v.position = plus( mul( yaw( v.position, angle ), scale ), offset );
                v.normal = yaw( v.normal, angle );
                v.tangent = yaw( v.tangent, angle );
                if( mirror )
                {
                    v.position.x = -v.position.x;
                    v.normal.x = -v.normal.x;
                    v.tangent.x = -v.tangent.x;
                    v.tangentSign = -v.tangentSign;
                }
            }
            if( mirror )
            {
                for( std::size_t i = 0; i < mesh.indices.size(); i += 3 )
                {
                    std::swap( mesh.indices[i + 1], mesh.indices[i + 2] );
                }
            }
        }

        std::shared_ptr<Mesh> plane( float w, float h )
        {
            auto m = std::make_shared<Mesh>();
            // Native nose -Z. UV0 is preserved separately from livery UV1.
            m->vertices = { { { -w / 2, -h / 2, 0 }, { 0, 0, -1 }, { 1, 0, 0 }, 1, { 0, 1 } },
                            { { -w / 2, h / 2, 0 }, { 0, 0, -1 }, { 1, 0, 0 }, 1, { 0, 0 } },
                            { { w / 2, h / 2, 0 }, { 0, 0, -1 }, { 1, 0, 0 }, 1, { 1, 0 } },
                            { { w / 2, -h / 2, 0 }, { 0, 0, -1 }, { 1, 0, 0 }, 1, { 1, 1 } } };
            m->indices = { 0, 1, 2, 0, 2, 3 };
            return m;
        }

        std::shared_ptr<Mesh> disc( float radius )
        {
            auto m = std::make_shared<Mesh>();
            m->vertices.push_back( { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, 1, { .5f, .5f } } );
            for( unsigned i = 0; i <= 24; ++i )
            {
                float a = 2 * pi * i / 24;
                m->vertices.push_back( { { 0, radius * std::cos( a ), radius * std::sin( a ) },
                                         { 1, 0, 0 },
                                         { 0, 0, 1 },
                                         1,
                                         { .5f + .5f * std::cos( a ), .5f + .5f * std::sin( a ) } } );
            }
            for( unsigned i = 1; i <= 24; ++i )
            {
                m->indices.insert( m->indices.end(), { 0, i, i + 1 } );
            }
            return m;
        }

        std::shared_ptr<Mesh> cone()
        {
            auto m = std::make_shared<Mesh>();
            for( unsigned i = 0; i <= 14; ++i )
            {
                float a = 2 * pi * i / 14;
                m->vertices.push_back(
                    { { 0, 0, 0 }, { 0, 1, 0 }, { 1, 0, 0 }, 1, { float( i ) / 14, 0 } } );
                m->vertices.push_back( { { 1.15f * std::cos( a ), 1.15f * std::sin( a ), -13 },
                                         { 0, 1, 0 },
                                         { 1, 0, 0 },
                                         1,
                                         { float( i ) / 14, 1 } } );
            }
            for( unsigned i = 0; i < 14; ++i )
            {
                unsigned a = i * 2;
                m->indices.insert( m->indices.end(), { a, a + 1, a + 3 } );
            }
            return m;
        }

        float noise( std::uint32_t seed, double time, unsigned channel )
        {
            std::uint32_t x = seed ^ std::uint32_t( std::fmod( std::abs( time ) * 120, 4294967295.0 ) ) ^
                              ( channel * 0x9e3779b9u );
            x ^= x >> 16;
            x *= 0x7feb352du;
            x ^= x >> 15;
            return float( x & 65535 ) / 65535;
        }

        float stop( float p, float a, float b, float va, float vb )
        {
            return va + ( vb - va ) * std::clamp( ( p - a ) / ( b - a ), 0.f, 1.f );
        }
    }  // namespace

    void Bounds::include( Vec3 p )
    {
        if( !finite( p ) )
        {
            return;
        }
        if( !valid )
        {
            min = max = p;
            valid = true;
            return;
        }
        min = { std::min( min.x, p.x ), std::min( min.y, p.y ), std::min( min.z, p.z ) };
        max = { std::max( max.x, p.x ), std::max( max.y, p.y ), std::max( max.z, p.z ) };
    }

    Vec3 Bounds::size() const
    {
        return valid ? sub( max, min ) : Vec3{};
    }

    Vec3 Bounds::centre() const
    {
        return mul( plus( min, max ), .5f );
    }

    bool Bounds::contains( const Bounds &b ) const
    {
        return valid && b.valid && b.min.x >= min.x && b.max.x <= max.x && b.min.y >= min.y &&
               b.max.y <= max.y && b.min.z >= min.z && b.max.z <= max.z;
    }

    Bounds CarVisual::robustBox( const std::vector<Mesh> &meshes )
    {
        std::vector<std::size_t> order;
        for( std::size_t i = 0; i < meshes.size(); ++i )
        {
            if( bounds( meshes[i] ).valid )
            {
                order.push_back( i );
            }
        }
        if( order.empty() )
        {
            return {};
        }
        std::sort( order.begin(), order.end(), [&]( auto a, auto b ) {
            return meshes[a].vertices.size() > meshes[b].vertices.size();
        } );
        auto shell = order.front();
        float best = -1;
        for( std::size_t i = 0; i < std::min<std::size_t>( 6, order.size() ); ++i )
        {
            auto s = bounds( meshes[order[i]] ).size();
            if( s.x * s.z > best )
            {
                best = s.x * s.z;
                shell = order[i];
            }
        }
        auto result = bounds( meshes[shell] );
        auto room = result;
        auto pad = mul( room.size(), .12f );
        room.min = sub( room.min, pad );
        room.max = plus( room.max, pad );
        for( const auto &m : meshes )
        {
            auto b = bounds( m );
            if( b.valid && ( order.size() < 3 || room.contains( b ) ) )
            {
                result.include( b.min );
                result.include( b.max );
            }
        }
        return result;
    }

    Bounds CarVisual::percentileBox( const std::vector<Mesh> &meshes, float low, float high )
    {
        if( !std::isfinite( low ) || !std::isfinite( high ) || low < 0 || high > 1 || low >= high )
        {
            throw std::invalid_argument( "Invalid CarVisual percentile range" );
        }
        std::vector<Vec3> points;
        std::array<std::vector<float>, 3> axes;
        for( const auto &m : meshes )
        {
            auto stride = std::max<std::size_t>( 1, m.vertices.size() / 900 );
            for( std::size_t i = 0; i < m.vertices.size(); i += stride )
            {
                auto p = m.vertices[i].position;
                if( !finite( p ) )
                {
                    continue;
                }
                points.push_back( p );
                axes[0].push_back( p.x );
                axes[1].push_back( p.y );
                axes[2].push_back( p.z );
            }
        }
        if( points.empty() )
        {
            return {};
        }
        auto quantile = [&]( unsigned a, float q ) {
            auto &v = axes[a];
            std::sort( v.begin(), v.end() );
            return v[std::size_t( std::round( q * ( v.size() - 1 ) ) )];
        };
        Bounds core;
        core.include( { quantile( 0, low ), quantile( 1, low ), quantile( 2, low ) } );
        core.include( { quantile( 0, high ), quantile( 1, high ), quantile( 2, high ) } );
        auto c = core.centre(), s = core.size();
        Bounds room;
        room.include( sub( c, mul( s, .85f ) ) );
        room.include( plus( c, mul( s, .85f ) ) );
        Bounds result;
        for( auto p : points )
        {
            Bounds b;
            b.include( p );
            if( room.contains( b ) )
            {
                result.include( p );
            }
        }
        return result.valid ? result : core;
    }

    int CarVisual::add( Node node )
    {
        m_nodes.push_back( std::move( node ) );
        return int( m_nodes.size() - 1 );
    }

    void CarVisual::configure( const Spec &spec, const Models &models, Colour paint )
    {
        for( float v : { spec.length, spec.wheelbase, spec.frontTrack, spec.rearTrack } )
        {
            if( !std::isfinite( v ) || v <= 0 )
            {
                throw std::invalid_argument( "Invalid CarVisual dimensions" );
            }
        }
        for( float r : spec.wheelRadius )
        {
            if( !std::isfinite( r ) || r <= 0 )
            {
                throw std::invalid_argument( "Invalid wheel radius" );
            }
        }
        if( models.chassis.empty() || !std::isfinite( spec.yawFix ) ||
            !std::isfinite( spec.axleOffsetZ ) )
        {
            throw std::invalid_argument( "CarVisual requires a chassis and finite alignment" );
        }
        for( const auto &m : models.chassis )
        {
            validate( m );
        }
        for( const auto &m : models.wheel )
        {
            validate( m );
        }
        CarVisual next;
        next.m_spec = spec;
        next.m_paint = paint;
        Node group;
        group.name = "CarVisual";
        next.add( group );
        group.parent = 0;
        group.name = "Car body";
        next.add( group );
        auto chassis = models.chassis;
        auto size = percentileBox( chassis ).size();
        float angle =
            ( size.x > size.z ? pi / 2 : 0 ) + spec.yawFix + ( spec.sourceForwardPlusZ ? pi : 0 );
        for( auto &m : chassis )
        {
            transform( m, angle, 1, {} );
        }
        auto box = percentileBox( chassis );
        size = box.size();
        if( size.z < 1e-5f )
        {
            throw std::invalid_argument( "Degenerate chassis length" );
        }
        float scale = spec.length / size.z;
        auto centre = box.centre();
        Vec3 offset{ -centre.x * scale, -box.min.y * scale, -centre.z * scale + spec.axleOffsetZ };
        std::vector<std::pair<float, std::size_t>> candidates;
        for( auto &m : chassis )
        {
            transform( m, 0, scale, offset );
            auto s = bounds( m ).size();
            Node n;
            n.parent = 1;
            n.kind = Kind::Body;
            n.name = m.name;
            n.mesh = std::make_shared<Mesh>( std::move( m ) );
            const auto &mesh = *n.mesh;
            n.colour = mesh.colour;
            n.roughness = std::min( mesh.roughness, .55f );
            n.metalness = mesh.metalness;
            n.opacity = mesh.opacity;
            n.transparent = mesh.transparent;
            n.castShadow = spec.player || spec.rivalShadows;
            n.glass = named( mesh.sourceMaterial + " " + mesh.name, "glass|window" ) ||
                      ( mesh.transparent && mesh.opacity < .9f );
            if( n.glass )
            {
                n.colour = { .007f, .009f, .013f, 1 };
                n.opacity = std::min( n.opacity, .42f );
                n.roughness = .06f;
                n.metalness = .2f;
                n.transparent = true;
                n.depthWrite = false;
            }
            else
            {
                float score = s.x * s.z * std::sqrt( float( mesh.vertices.size() ) + 1 );
                auto name = mesh.sourceMaterial + " " + mesh.name;
                if( named( name, "paint|body|carrosser|lack|shell|exterior|primary" ) )
                {
                    score *= 25;
                }
                if( named(
                        name,
                        "interior|engine|brake|tyre|tire|rubber|chrome|carbon|light|badge|logo|plate" ) )
                {
                    score *= .05f;
                }
                candidates.push_back( { score, next.m_nodes.size() } );
            }
            next.add( std::move( n ) );
        }
        std::stable_sort( candidates.begin(), candidates.end(),
                          []( auto a, auto b ) { return a.first > b.first; } );
        for( std::size_t i = 0; i < std::min<std::size_t>( 2, candidates.size() ); ++i )
        {
            auto &n = next.m_nodes[candidates[i].second];
            n.paint = true;
            n.colour = paint;
            n.roughness = .24f;
            n.metalness = .55f;
            n.clearcoat = 1;
        }
        // Project channel 1 after normalisation; original UV0 and tangent frames survive.
        for( auto &n : next.m_nodes )
        {
            if( n.kind == Kind::Body )
            {
                auto m = std::make_shared<Mesh>( *n.mesh );
                for( auto &v : m->vertices )
                {
                    v.uv1 = { ( v.position.z + spec.length * .5f - spec.axleOffsetZ ) / spec.length,
                              v.position.y / std::max( size.y * scale, .001f ) };
                }
                n.mesh = std::move( m );
            }
        }
        for( unsigned i = 0; i < 4; ++i )
        {
            bool front = i < 2, left = ( i % 2 ) == 0;
            Node pivot;
            pivot.parent = 1;
            pivot.name = "Wheel steering";
            pivot.corner = int( i );
            pivot.position = { ( left ? -.5f : .5f ) * ( front ? spec.frontTrack : spec.rearTrack ),
                               spec.wheelRadius[i],
                               ( front ? -.5f : .5f ) * spec.wheelbase + spec.axleOffsetZ };
            next.m_pivots[i] = next.add( pivot );
            Node spin;
            spin.parent = next.m_pivots[i];
            spin.name = "Wheel spin";
            next.m_spins[i] = next.add( spin );
            auto wheel = models.wheel;
            auto wb = robustBox( wheel );
            auto ws = wb.size();
            float dia = std::max( { ws.x, ws.y, ws.z } );
            if( wb.valid && dia > 1e-5f )
            {
                for( auto &m : wheel )
                {
                    float sc = spec.wheelRadius[i] * 2 / dia;
                    transform( m, 0, sc, mul( wb.centre(), -sc ), left );
                    Node n;
                    n.parent = next.m_spins[i];
                    n.kind = Kind::Wheel;
                    n.corner = int( i );
                    n.name = m.name;
                    n.colour = m.colour;
                    n.roughness = m.roughness;
                    n.metalness = m.metalness;
                    n.mesh = std::make_shared<Mesh>( std::move( m ) );
                    n.castShadow = spec.player || spec.rivalShadows;
                    next.add( n );
                }
            }
            if( !wb.valid )
            {
                continue;
            }
            Node blur;
            blur.parent = next.m_pivots[i];
            blur.kind = Kind::WheelBlur;
            blur.corner = int( i );
            blur.name = "Wheel rotational blur";
            blur.mesh = disc( spec.wheelRadius[i] * .92f );
            blur.position.x = ( left ? -.06f : .06f );
            blur.colour = { .003f, .004f, .006f, 1 };
            blur.opacity = 0;
            blur.transparent = true;
            blur.depthWrite = false;
            next.add( blur );
        }
        auto fx = [&]( Kind kind, std::string name, Vec3 pos, float w, float h, Colour col, float alpha,
                       Texture tex ) {
            Node n;
            n.parent = 1;
            n.kind = kind;
            n.name = std::move( name );
            n.position = pos;
            n.mesh = plane( w, h );
            n.colour = col;
            n.opacity = alpha;
            n.transparent = true;
            n.depthWrite = false;
            n.additive = kind != Kind::ContactShadow;
            n.texture = tex;
            return next.add( n );
        };
        float front = -spec.length * .47f + spec.axleOffsetZ,
              rear = spec.length * .47f + spec.axleOffsetZ, hw = spec.frontTrack * .5f + .1f;
        for( int side : { -1, 1 } )
        {
            fx( Kind::Headlight, "Headlight emitter", { side * hw * .72f, .62f, front - .02f }, .34f,
                .20f, { 1, .91f, .72f, 1 }, .38f, Texture::Glow );
            Node beam;
            beam.parent = 1;
            beam.kind = Kind::Beam;
            beam.name = "Headlight beam";
            beam.position = { side * hw * .62f, .58f, front - .9f };
            beam.mesh = cone();
            beam.opacity = .028f;
            beam.transparent = true;
            beam.additive = true;
            beam.depthWrite = false;
            beam.texture = Texture::Cone;
            next.add( beam );
            int t = fx( Kind::Tail, "Tail light", { side * hw * .60f, .72f, rear + .02f }, .30f, .10f,
                        { 1, .009f, .03f, 1 }, .85f, Texture::None );
            next.m_nodes[t].euler.y = pi;
            fx( Kind::Flame, "Exhaust flame", { side * .34f, .42f, rear + .35f }, 1.5f, 1.5f,
                { 1, .35f, .055f, 1 }, 0, Texture::Flare );
        }
        int pool = fx( Kind::Pool, "Headlight road pool", { 0, .035f, front - 8.5f }, 11, 22,
                       { 1, .84f, .65f, 1 }, .16f, Texture::Glow );
        next.m_nodes[pool].euler.x = pi / 2;
        next.m_nodes[pool].visible = false;
        int tail = fx( Kind::TailGlow, "Tail glow", { 0, .7f, rear + .30f }, 1.7f, .75f,
                       { 1, .014f, .03f, 1 }, .3f, Texture::Glow );
        next.m_nodes[tail].euler.y = pi;
        int under = fx( Kind::Underglow, "Nitrous underglow", { 0, .055f, 0 }, spec.length * 1.25f,
                        spec.length * .72f, { .04f, .79f, 1, 1 }, 0, Texture::Glow );
        next.m_nodes[under].euler.x = pi / 2;
        int shadow = fx( Kind::ContactShadow, "Contact shadow", { 0, .02f, 0 }, spec.length * .44f,
                         spec.length * .82f, { 0, 0, 0, 1 }, 1, Texture::Shadow );
        next.m_nodes[shadow].parent = 0;
        next.m_nodes[shadow].euler.x = pi / 2;
        if( spec.police )
        {
            for( int side : { -1, 1 } )
            {
                Colour c = side < 0 ? Colour{ .016f, .058f, 1, 1 } : Colour{ 1, .016f, .033f, 1 };
                int lamp = fx( Kind::PoliceLamp, "Police lamp", { side * .30f, 1.42f, .1f }, .46f, .14f,
                               c, 1, Texture::None );
                next.m_nodes[lamp].corner = side < 0 ? 0 : 1;
                int halo = fx( Kind::PoliceHalo, "Police halo", { side * .30f, 1.42f, .1f }, 4.6f, 4.6f,
                               c, .5f, Texture::Glow );
                next.m_nodes[halo].corner = side < 0 ? 0 : 1;
            }
        }
        if( spec.player )
        {
            next.m_spots[0] = { { 0, .62f, front },
                                { 0, -.2f, front - 26 },
                                { 1, .87f, .69f, 1 },
                                170,
                                95,
                                .44f,
                                .60f,
                                true };
            next.m_spots[1] = { { 0, .7f, front },
                                { 0, -.4f, front - 12 },
                                { 1, .81f, .60f, 1 },
                                70,
                                42,
                                .92f,
                                .92f,
                                false };
        }
        *this = std::move( next );
        sync( {}, 0 );
    }

    void CarVisual::setPaint( Colour paint )
    {
        m_paint = paint;
        for( auto &n : m_nodes )
        {
            if( n.paint )
            {
                n.colour = m_livery.empty() ? paint : Colour{};
            }
        }
    }

    void CarVisual::setLivery( std::string path )
    {
        m_livery = std::move( path );
        setPaint( m_paint );
    }

    void CarVisual::setVisible( bool v )
    {
        m_visible = v;
        if( !m_nodes.empty() )
        {
            m_nodes[0].visible = v;
        }
    }

    void CarVisual::sync( const Frame &frame, double seconds, const Vec3 *camera )
    {
        if( m_nodes.empty() )
        {
            return;
        }
        if( !finite( frame.position ) || !std::isfinite( seconds ) )
        {
            throw std::invalid_argument( "Non-finite CarVisual frame" );
        }
        for( float v : { frame.yaw, frame.pitch, frame.roll, frame.rideY, frame.speed, frame.brake,
                         frame.backfire, frame.nitrous } )
        {
            if( !std::isfinite( v ) )
            {
                throw std::invalid_argument( "Non-finite CarVisual controls" );
            }
        }
        for( unsigned i = 0; i < 4; ++i )
        {
            if( !std::isfinite( frame.steering[i] ) || !std::isfinite( frame.rotation[i] ) ||
                !std::isfinite( frame.suspension[i] ) )
            {
                throw std::invalid_argument( "Non-finite wheel pose" );
            }
        }
        if( camera && !finite( *camera ) )
        {
            throw std::invalid_argument( "Non-finite camera" );
        }
        m_nodes[0].position = plus( frame.position, { 0, frame.rideY, 0 } );
        m_nodes[0].euler.y = frame.yaw;
        m_nodes[0].visible = m_visible;
        m_nodes[1].euler = { frame.pitch, 0, frame.roll };
        for( unsigned i = 0; i < 4; ++i )
        {
            m_nodes[m_pivots[i]].euler.y = frame.steering[i];
            m_nodes[m_pivots[i]].position.y = m_spec.wheelRadius[i] + frame.suspension[i];
            m_nodes[m_spins[i]].euler.x = std::remainder( frame.rotation[i], 2 * pi );
        }
        float b = unit( frame.brake ), nos = unit( frame.nitrous ),
              fl = std::max( nos * ( .7f + noise( m_spec.seed, seconds, 1 ) * .3f ),
                             unit( frame.backfire ) );
        for( auto &n : m_nodes )
        {
            switch( n.kind )
            {
            case Kind::WheelBlur:
                n.opacity = std::clamp( ( std::abs( frame.speed ) - 11 ) / 26, 0.f, 1.f ) * .72f;
                break;
            case Kind::Headlight:
                n.opacity = frame.headlights ? .38f : 0;
                break;
            case Kind::Beam:
                n.opacity = frame.headlights ? .028f : 0;
                n.scale.x = n.scale.y = 1 + std::sin( float( seconds ) * 3.1f ) * .02f;
                break;
            case Kind::Pool:
                n.opacity =
                    frame.headlights ? .14f + std::min( .10f, std::abs( frame.speed ) * .0024f ) : 0;
                break;
            case Kind::Tail:
                n.opacity = .42f + b * .58f;
                n.colour = b > .05f ? Colour{ 1, .003f, .016f, 1 } : Colour{ 1, .009f, .03f, 1 };
                break;
            case Kind::TailGlow:
                n.opacity = .10f + b * .34f;
                n.scale = { 1 + b * .35f, 1 + b * .35f, 1 };
                break;
            case Kind::Underglow:
                n.opacity = nos * .34f;
                n.colour = { .04f, .79f, 1, 1 };
                break;
            case Kind::Flame:
                n.opacity = fl * .95f;
                n.colour = nos > 0 ? Colour{ .12f, .58f, 1, 1 } : Colour{ 1, .25f, .03f, 1 };
                n.scale = { .7f + fl * ( .9f + noise( m_spec.seed, seconds, 2 ) * .5f ),
                            .7f + fl * ( .9f + noise( m_spec.seed, seconds, 2 ) * .5f ), 1 };
                break;
            case Kind::PoliceLamp:
            case Kind::PoliceHalo:
            {
                float ph = float( std::fmod( std::max( 0.0, seconds ) * 7.5, 2 ) );
                float on = ( n.corner == 0 ? ph < .55f : ph > 1 && ph < 1.55f ) ? 1.f : .06f;
                n.opacity = on * ( n.kind == Kind::PoliceHalo ? .65f : 1.f );
                break;
            }
            default:
                break;
            }
            if( camera && ( n.kind == Kind::Flame || n.kind == Kind::PoliceHalo ) )
            {
                auto local = yaw( sub( *camera, m_nodes[0].position ), -frame.yaw );
                auto d = sub( local, n.position );
                n.euler = { std::atan2( d.y, std::sqrt( d.x * d.x + d.z * d.z ) ),
                            std::atan2( -d.x, -d.z ), 0 };
            }
        }
        for( auto &spot : m_spots )
        {
            spot.intensity = frame.headlights ? ( spot.shadows ? 170.f : 70.f ) : 0;
        }
    }

    std::shared_ptr<const Image> CarVisual::texture( Texture type )
    {
        auto make = []( Texture t ) {
            auto image = std::make_shared<Image>();
            image->width = t == Texture::Shadow ? 64 : t == Texture::Cone ? 64 : 128;
            image->height = t == Texture::Cone ? 128 : image->width;
            image->rgba.resize( image->width * image->height * 4 );
            for( unsigned y = 0; y < image->height; ++y )
            {
                for( unsigned x = 0; x < image->width; ++x )
                {
                    float dx = ( float( x ) + .5f ) / image->width * 2 - 1,
                          dy = ( float( y ) + .5f ) / image->height * 2 - 1,
                          r = std::sqrt( dx * dx + dy * dy ), a = 0;
                    Colour c;
                    if( t == Texture::Cone )
                    {
                        float v = float( y ) / std::max( 1u, image->height - 1 );
                        a = v < .35f ? stop( v, 0, .35f, .85f, .30f ) : stop( v, .35f, 1, .30f, 0 );
                        c = { 1, .94f, .80f, 1 };
                    }
                    else if( t == Texture::Shadow )
                    {
                        a = r < .55f ? stop( r, 0, .55f, 1, .55f ) : stop( r, .55f, 1, .55f, 0 );
                        c = { 0, 0, 0, 1 };
                    }
                    else if( t == Texture::Flare )
                    {
                        a = r < .15f  ? stop( r, 0, .15f, 1, .8f )
                            : r < .5f ? stop( r, .15f, .5f, .8f, .2f )
                                      : stop( r, .5f, 1, .2f, 0 );
                        if( std::abs( dy ) < .024f )
                        {
                            a = std::max( a, .5f );
                        }
                    }
                    else
                    {
                        a = r < .3f   ? stop( r, 0, .3f, 1, .5f )
                            : r < .7f ? stop( r, .3f, .7f, .5f, .1f )
                                      : stop( r, .7f, 1, .1f, 0 );
                    }
                    auto i = ( y * image->width + x ) * 4;
                    image->rgba[i] = std::uint8_t( unit( c.r ) * 255 );
                    image->rgba[i + 1] = std::uint8_t( unit( c.g ) * 255 );
                    image->rgba[i + 2] = std::uint8_t( unit( c.b ) * 255 );
                    image->rgba[i + 3] = std::uint8_t( unit( a ) * 255 );
                }
            }
            return std::shared_ptr<const Image>( image );
        };
        static const auto glow = make( Texture::Glow ), coneImage = make( Texture::Cone ),
                          flare = make( Texture::Flare ), shadow = make( Texture::Shadow );
        switch( type )
        {
        case Texture::Glow:
            return glow;
        case Texture::Cone:
            return coneImage;
        case Texture::Flare:
            return flare;
        case Texture::Shadow:
            return shadow;
        default:
            return {};
        }
    }
}  // namespace workphone::vehiclevisual
