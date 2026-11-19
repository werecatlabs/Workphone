#pragma once

namespace cityRoadLaneWear
{
    using cityCarriagewayUtilities::Patch;
    using cityCarriagewayUtilities::Projected;
    using cityCarriagewayUtilities::Triangle;

    float smooth( float low, float high, float value )
    {
        const float t = std::clamp( ( value - low ) / ( high - low ), 0.f, 1.f );
        return t * t * ( 3.f - 2.f * t );
    }

    TextureBuffer atlas( u32 seed )
    {
        constexpr u32 width = 192, height = 96;
        TextureBuffer result( 4 * width, height );
        const auto byte = []( float value ) {
            return u8( std::clamp( std::lround( value ), 0l, 255l ) );
        };
        for( u32 tile = 0; tile < 4; ++tile )
        {
            const auto key = ( u64( seed ) << 32 ) ^ ( 0x736b696c6c786d65ull + tile * 0x9e3779b9u );
            const float separation = .60f + .08f * cityHillside::unit( key + 1 );
            const float phase = 2.f * pi * cityHillside::unit( key + 2 );
            for( u32 y = 0; y < height; ++y )
            {
                const float across = 2.f * ( y + .5f ) / height - 1.f;
                for( u32 x = 0; x < width; ++x )
                {
                    const float station = ( x + .5f ) / width;
                    const float grain = cityHillside::unit( key + y * width + x + 41 ) - .5f;
                    const float edge = smooth( .049f, .29f, station ) *
                                       smooth( .051f, .27f, 1.f - station ) *
                                       smooth( .053f, .23f, 1.f - std::abs( across ) );
                    const float wander = .045f * std::sin( phase + station * 8.9f );
                    float wheels = 0;
                    for( const float side : { -1.f, 1.f } )
                    {
                        const float alignment = std::abs( across - wander - side * separation );
                        const float interruption =
                            .26f * std::sin( phase + station * 16.f + side * 1.3f ) +
                            .11f * std::cos( phase + station * 31.f - side );
                        const float bandWidth = .18f + .052f * std::sin( phase + station * 11.f + side );
                        wheels += ( 1.f - smooth( bandWidth * .33f, bandWidth * 1.64f, alignment ) ) *
                                  ( .61f + interruption );
                    }
                    const float centre = ( 1.f - smooth( .09f, .38f, std::abs( across + wander ) ) ) *
                                         ( .55f + .24f * std::cos( station * 17.f + phase ) );
                    // The scan supplies fine aggregate. This decal adds
                    // metre-scale binder wear and two irregular traffic
                    // tracks without painting an opaque rectangle.
                    const float binder = .5f +
                                         .26f * std::sin( phase + station * 11.f + across * 4.8f ) +
                                         .17f * std::cos( phase * 1.7f + station * 23.f - across * 9.f );
                    const float aged = smooth( .46f, .82f, binder );
                    const float edgeDust = smooth( .68f, .96f, across );
                    const float a =
                        std::min( .58f, .09f * aged + .37f * wheels + .09f * centre + .10f * edgeDust ) *
                        edge * ( .94f + .12f * grain );
                    const float tone =
                        68.f + 11.f * aged - 16.f * wheels + 14.f * edgeDust + 6.f * grain;
                    auto *pixel = result.pixel( x + tile * width, y );
                    pixel[0] = byte( tone + 1.f );
                    pixel[1] = byte( tone );
                    pixel[2] = byte( tone - 1.f );
                    pixel[3] = byte( a * 255.f );
                }
            }
        }
        return result;
    }

    template <class Samples>
    bool pointAt( const Samples &samples, float station, Vector3F &point )
    {
        const auto end = std::lower_bound(
            samples.begin(), samples.end(), station,
            []( const auto &sample, float value ) { return sample.distance < value; } );
        if( end == samples.begin() || end == samples.end() )
        {
            return false;
        }
        const auto &start = *( end - 1 );
        const float interval = end->distance - start.distance;
        if( interval < .001f )
        {
            return false;
        }
        point = start.position +
                ( end->position - start.position ) * ( ( station - start.distance ) / interval );
        return point.isFinite();
    }

    template <class Samples>
    bool chord( const Samples &samples, float position, float halfLength, Vector3F &centre,
                Vector3F &along )
    {
        Vector3F a, b;
        if( !pointAt( samples, position - halfLength, a ) ||
            !pointAt( samples, position + halfLength, b ) )
        {
            return false;
        }
        along = b - a;
        along.y = 0;
        if( along.lengthSquared() < 4.f * halfLength * halfLength * .962f )
        {
            return false;
        }
        along.normalise();
        centre = ( a + b ) * .5f;
        const Vector3F across( -along.z, 0, along.x );
        for( const float fraction : { -.58f, -.29f, 0.f, .29f, .58f } )
        {
            Vector3F p;
            if( !pointAt( samples, position + fraction * halfLength, p ) ||
                std::abs( ( p - centre ).dotProduct( across ) ) > .19f )
            {
                return false;
            }
        }
        return true;
    }

    std::array<Vector3F, 4> footprint( const Patch &patch, float margin = 0 )
    {
        const auto u = patch.along * ( patch.halfLength + margin );
        const auto v = patch.across * ( patch.halfWidth + margin );
        return { patch.centre - u - v, patch.centre + u - v, patch.centre + u + v,
                 patch.centre - u + v };
    }

    bool supported( const Patch &patch )
    {
        const double area = 4.0 * patch.halfLength * patch.halfWidth;
        return area > 0 && std::abs( patch.supportedArea - area ) < std::max( .005, .0016 * area );
    }

    struct Site
    {
        explicit Site( Patch value, unsigned pattern ) :
            patch( std::move( value ) ),
            bounds( footprint( patch, .1f ) ),
            tile( pattern )
        {
        }

        Patch patch;
        cityRoadEdges::Bounds bounds;
        unsigned tile;
        size_t lastFace = 0;
    };

    void build( SceneAssets &assets, const CityRoadTerrainMesh &walks,
                const cityCarriagewayUtilities::Layers &layers, u32 seed,
                VehicleAppearanceQuality quality )
    {
        const auto detail = std::clamp( assets.cityStreetDetail, 0, 100 );
        if( !detail || !layers.road || !assets.roadAuthoring ||
            walks.sourceRevision != assets.roadAuthoring->getActiveRevision() )
        {
            return;
        }
        const auto &source = assets.roadAuthoring->getSource();
        const auto &bake = assets.roadAuthoring->getBake();
        if( source.roads.size() != bake.roads.size() )
        {
            return;
        }

        constexpr size_t candidateLimit = 192;
        SourceBudgetVector<Site> candidates;
        for( size_t roadIndex = 0; roadIndex < source.roads.size() && candidates.size() < candidateLimit;
             ++roadIndex )
        {
            const auto &samples = bake.roads[roadIndex].points;
            const float roadHalf = source.roads[roadIndex].width * .5f;
            if( samples.size() < 2 || roadHalf < 2.70f )
            {
                continue;
            }
            const float length = samples.back().distance - samples.front().distance;
            if( length < 16.f )
            {
                continue;
            }
            const auto groups = u32( std::clamp( std::floor( length / 23.f ), 1.f, 8.f ) );
            const float spacing = length / groups;
            for( u32 i = 0; i < groups && candidates.size() + 1 < candidateLimit; ++i )
            {
                const auto key = render::foliageStableHash(
                    ( u64( seed ) << 32 ) ^ 0x726f616477656172ull ^ ( u64( roadIndex ) << 8 ) ^ i );
                const float halfLength =
                    std::min( spacing * .37f, 4.9f + 2.8f * cityHillside::unit( key + 1 ) );
                const float station = samples.front().distance +
                                      spacing * ( i + .37f + .24f * cityHillside::unit( key + 2 ) );
                const float firstSide = ( key & 1u ) ? 1.f : -1.f;
                for( const float side : { firstSide, -firstSide } )
                {
                    const float offset = side * ( .55f + .63f * cityHillside::unit( key + 3 ) );
                    Vector3F centre, along;
                    if( !chord( samples, station + offset, halfLength, centre, along ) )
                    {
                        continue;
                    }
                    const Vector3F across( -along.z, 0, along.x );
                    const float halfWidth = std::clamp( roadHalf * .34f, 1.02f, 1.36f );
                    // Positive atlas V always points toward the kerb, so
                    // the faint edge grit appears on the outside of both lanes.
                    Patch patch{ centre + across * ( side * roadHalf * .51f ),
                                 along,
                                 across * side,
                                 halfLength,
                                 halfWidth,
                                 0,
                                 0,
                                 0,
                                 0,
                                 false };
                    if( std::any_of(
                            walks.crossings.begin(), walks.crossings.end(), [&]( const auto &crossing ) {
                                return cityCarriagewayUtilities::overlapsCrossing( patch, crossing );
                            } ) )
                    {
                        continue;
                    }
                    candidates.emplace_back( std::move( patch ),
                                             unsigned( ( key >> ( side > 0 ? 5 : 11 ) ) & 3u ) );
                }
            }
        }
        if( candidates.empty() )
        {
            return;
        }

        using Cell = std::pair<s32, s32>;
        SourceBudgetMap<Cell, SourceBudgetVector<size_t>> index;
        for( size_t i = 0; i < candidates.size(); ++i )
        {
            candidates[i].bounds.cells( [&]( auto cell ) {
                index[cell].push_back( i );
                return false;
            } );
        }
        size_t serial = 0;
        const auto visitCandidates = [&]( const Triangle &face, const auto &accept ) {
            ++serial;
            const cityRoadEdges::Bounds bounds( std::array{ face.a, face.b, face.c } );
            bounds.cells( [&]( auto cell ) {
                const auto found = index.find( cell );
                if( found == index.end() )
                {
                    return false;
                }
                for( const auto id : found->second )
                {
                    auto &site = candidates[id];
                    if( site.lastFace == serial || site.patch.blocked ||
                        !site.bounds.intersects( bounds ) )
                    {
                        continue;
                    }
                    site.lastFace = serial;
                    accept( site.patch );
                }
                return false;
            } );
        };
        const auto avoid = [&]( SmartPtr<IMesh> layer ) {
            return !layer || cityCarriagewayUtilities::visitTriangles( layer, [&]( Vector3F a,
                                                                                   Vector3F b,
                                                                                   Vector3F c ) {
                visitCandidates( { a, b, c }, [&]( Patch &patch ) {
                    std::array<Projected, 8> cut{};
                    const auto count = cityCarriagewayUtilities::clip(
                        cut, { a, b, c }, patch, patch.halfLength + .09f, patch.halfWidth + .09f );
                    patch.blocked = cityCarriagewayUtilities::projectedArea( cut, count ) > 1e-7;
                } );
            } );
        };
        for( const auto &layer : { layers.paint, layers.island, layers.kerb } )
        {
            if( !avoid( layer ) )
            {
                WP_LOG_WARNING( "City wheel wear skipped: an exclusion mesh could not be read." );
                return;
            }
        }
        for( const auto &actor : assets.actors )
        {
            if( !actor )
            {
                continue;
            }
            const auto name = actor->getName();
            if( name != "Terrain city carriageway asphalt repairs" )
            {
                continue;
            }
            const auto component = actor->getComponent<scene::Mesh>();
            const auto resource = component ? component->getMeshResource() : nullptr;
            if( !resource || !resource->getMesh() || !avoid( resource->getMesh() ) )
            {
                WP_LOG_WARNING( "City wheel wear skipped: the resurfacing mesh could not be read." );
                return;
            }
        }
        if( !cityCarriagewayUtilities::visitTriangles(
                layers.road, [&]( Vector3F a, Vector3F b, Vector3F c ) {
                    const auto normal = ( b - a ).crossProduct( c - a );
                    if( normal.lengthSquared() < 1e-12f || std::abs( normal.normaliseCopy().y ) < .94f )
                    {
                        return;
                    }
                    visitCandidates( { a, b, c }, [&]( Patch &patch ) {
                        std::array<Projected, 8> cut{};
                        const auto count = cityCarriagewayUtilities::clip(
                            cut, { a, b, c }, patch, patch.halfLength, patch.halfWidth );
                        const auto area = cityCarriagewayUtilities::projectedArea( cut, count );
                        if( area <= 1e-8 )
                        {
                            return;
                        }
                        if( patch.triangles.size() == 224 )
                        {
                            patch.blocked = true;
                            return;
                        }
                        patch.triangles.push_back( { a, b, c } );
                        patch.supportedArea += area;
                    } );
                } ) )
        {
            WP_LOG_WARNING(
                "City wheel wear skipped: the published carriageway mesh could not be read." );
            return;
        }

        Geometry wear;
        size_t installed = 0, rejected = 0;
        const size_t limit =
            ( ( quality == VehicleAppearanceQuality::Preview ? 24u : 64u ) * detail + 99 ) / 100;
        for( const auto &site : candidates )
        {
            const auto &patch = site.patch;
            if( patch.blocked || !supported( patch ) )
            {
                ++rejected;
                continue;
            }
            const auto before = wear.indices.size();
            cityCarriagewayUtilities::project( wear, patch, patch.halfLength, patch.halfWidth, site.tile,
                                               .008f );
            if( wear.indices.size() > before && ++installed == limit )
            {
                break;
            }
        }
        if( wear.indices.empty() )
        {
            return;
        }
        auto surface = material( assets, ColourF::White, .94f );
        surface->setTexture( upload( assets, atlas( seed ), render::TextureMipFilter::Colour, 4 ), 0 );
        surface->setTransparent( true );
        surface->setBlendMode( 1 );
        surface->setDepthWrite( false );
        surface->setCullMode( 2 );
        surface->setEditorUInt( "uvFilter", 3 );
        surface->setEditorFloat( "uvAniso", 8.f );
        if( auto actor = mesh( assets, "Terrain city dry wheel wear", wear, surface ) )
        {
            actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                scene::Renderer::CastShadows::Off );
            WP_LOG( String( "Terrain city dry wheel wear spans=" ) + StringUtil::toString( installed ) +
                    ", triangles=" + StringUtil::toString( wear.indices.size() / 3 ) +
                    ", rejected=" + StringUtil::toString( rejected ) );
        }
    }
}  // namespace cityRoadLaneWear
