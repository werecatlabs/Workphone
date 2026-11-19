#pragma once

namespace cityCarriagewayUtilities
{
    struct Layers
    {
        SmartPtr<IMesh> road, paint, island, kerb;
        SmartPtr<render::IMaterial> roadMaterial;
    };

    Layers activeLayers( const SceneAssets &assets )
    {
        Layers result;
        if( !assets.roadAuthoring )
        {
            return result;
        }
        const auto revision = assets.roadAuthoring->getActiveRevision();
        const auto owner = assets.roadAuthoring->getActor();
        if( !owner || !revision || revision != assets.roadAuthoring->getSourceRevision() ||
            !detail::runtimeActorPublished( owner ) )
        {
            return result;
        }
        const auto name = "City road generation " + StringUtil::toString( revision );
        for( const auto &generation : owner->getChildren() )
        {
            if( !generation || generation->getName() != name ||
                generation->getScene() != owner->getScene() ||
                !detail::runtimeActorPublished( generation ) )
            {
                continue;
            }
            for( const auto &actor : generation->getChildren() )
            {
                if( !actor || actor->getScene() != owner->getScene() ||
                    !detail::runtimeActorPublished( actor ) )
                {
                    continue;
                }
                const auto component = actor->getComponent<scene::Mesh>();
                const auto resource = component ? component->getMeshResource() : nullptr;
                if( !resource )
                {
                    continue;
                }
                const auto actorName = actor->getName();
                if( actorName == "Authored road carriageway" )
                {
                    result.road = resource->getMesh();
                    if( const auto binding = actor->getComponent<scene::Material>() )
                    {
                        result.roadMaterial = binding->getMaterial();
                    }
                }
                else if( actorName == "Road markings" )
                {
                    result.paint = resource->getMesh();
                }
                else if( actorName == "Roundabout island" )
                {
                    result.island = resource->getMesh();
                }
                else if( actorName == "Roundabout kerb" )
                {
                    result.kerb = resource->getMesh();
                }
            }
            break;
        }
        return result;
    }

    template <class Visit>
    bool visitTriangles( SmartPtr<IMesh> mesh, const Visit &visit )
    {
        if( !mesh )
        {
            return false;
        }
        // Keep the packed CPU buffers owned by their meshes during the walk.
        // Full point/index copies would duplicate the terrain-sized road mesh.
        for( const auto &section : mesh->getSubMeshes() )
        {
            if( !section )
            {
                return false;
            }
            const auto vertices = section->getVertexBuffer();
            const auto indices = section->getIndexBuffer();
            auto declaration = vertices ? vertices->getVertexDeclaration() : nullptr;
            const auto position =
                declaration ? declaration->findElementBySemantic( VertexElementSemantic::VES_POSITION )
                            : nullptr;
            if( !position || position->getType() != VertexElementType::VET_FLOAT3 || !indices ||
                !indices->getIndexData() || indices->getNumIndices() % 3 != 0 )
            {
                return false;
            }
            const auto stride = declaration->getSize( position->getSource() );
            const auto offset = position->getOffset();
            const auto stream =
                static_cast<const u8 *>( vertices->getVertexData( position->getSource() ) );
            if( !stream || size_t( offset ) + sizeof( float ) * 3 > stride )
            {
                return false;
            }
            const auto vertex = [&]( u32 index ) {
                std::array<float, 3> data{};
                std::memcpy( data.data(), stream + index * size_t( stride ) + offset,
                             3 * sizeof( float ) );
                return Vector3F( data[0], data[1], data[2] );
            };
            const auto index = [&]( u32 i ) -> u32 {
                return indices->getIndexType() == IIndexBuffer::Type::IT_16BIT
                           ? static_cast<const u16 *>( indices->getIndexData() )[i]
                           : static_cast<const u32 *>( indices->getIndexData() )[i];
            };
            for( u32 i = 0; i < indices->getNumIndices(); i += 3 )
            {
                const auto a = index( i ), b = index( i + 1 ), c = index( i + 2 );
                if( std::max( { a, b, c } ) >= vertices->getNumVertices() )
                {
                    return false;
                }
                const auto p = vertex( a ), q = vertex( b ), r = vertex( c );
                if( !p.isFinite() || !q.isFinite() || !r.isFinite() )
                {
                    return false;
                }
                visit( p, q, r );
            }
        }
        return true;
    }

    struct Triangle
    {
        Vector3F a, b, c;
    };

    struct Patch
    {
        Vector3F centre, along, across;
        float halfLength, halfWidth, cover, range;
        size_t slot, family;
        bool square, blocked = false;
        double supportedArea = 0;
        SourceBudgetVector<Triangle> triangles;
    };

    struct Projected
    {
        Vector3F position;
        double along, across;
    };

    size_t clip( std::array<Projected, 8> &polygon, const Triangle &face, const Patch &patch,
                 double halfLength, double halfWidth )
    {
        size_t count = 0;
        for( const auto p : { face.a, face.b, face.c } )
        {
            const auto delta = p - patch.centre;
            polygon[count++] = { p,
                                 double( delta.x ) * patch.along.x + double( delta.z ) * patch.along.z,
                                 double( delta.x ) * patch.across.x +
                                     double( delta.z ) * patch.across.z };
        }
        std::array<Projected, 8> scratch{};
        for( int plane = 0; plane < 4 && count >= 3; ++plane )
        {
            const auto distance = [&]( const Projected &p ) {
                const double z = plane < 2 ? p.along : p.across;
                return ( plane < 2 ? halfLength : halfWidth ) + ( plane % 2 ? z : -z );
            };
            size_t written = 0;
            auto previous = polygon[count - 1];
            double d0 = distance( previous );
            for( size_t i = 0; i < count; ++i )
            {
                const auto &current = polygon[i];
                const double d1 = distance( current );
                if( ( d0 < 0 && d1 > 0 ) || ( d0 > 0 && d1 < 0 ) )
                {
                    const double t = d0 / ( d0 - d1 );
                    scratch[written++] = { previous.position +
                                               ( current.position - previous.position ) * float( t ),
                                           previous.along + ( current.along - previous.along ) * t,
                                           previous.across + ( current.across - previous.across ) * t };
                }
                if( d1 >= 0 )
                {
                    scratch[written++] = current;
                }
                previous = current;
                d0 = d1;
            }
            polygon.swap( scratch );
            count = written;
        }
        return count;
    }

    double projectedArea( const std::array<Projected, 8> &polygon, size_t count )
    {
        double area = 0;
        for( size_t i = 1; i + 1 < count; ++i )
        {
            area +=
                ( polygon[i].along - polygon[0].along ) * ( polygon[i + 1].across - polygon[0].across ) -
                ( polygon[i + 1].along - polygon[0].along ) * ( polygon[i].across - polygon[0].across );
        }
        return std::abs( area ) * .5;
    }

    void project( Geometry &geometry, const Patch &patch, float halfLength, float halfWidth,
                  unsigned tile, float lift )
    {
        for( const auto &face : patch.triangles )
        {
            std::array<Projected, 8> polygon{};
            const auto count = clip( polygon, face, patch, halfLength, halfWidth );
            auto normal = ( face.b - face.a ).crossProduct( face.c - face.a ).normaliseCopy();
            if( normal.y < 0 )
            {
                normal = -normal;
            }
            const auto tangent =
                ( patch.along - normal * normal.dotProduct( patch.along ) ).normaliseCopy();
            const float handedness =
                normal.crossProduct( tangent ).dotProduct( patch.across ) >= 0 ? 1.f : -1.f;
            for( size_t i = 1; i + 1 < count; ++i )
            {
                const auto triple = ( polygon[i].position - polygon[0].position )
                                        .crossProduct( polygon[i + 1].position - polygon[0].position )
                                        .dotProduct( normal );
                if( std::abs( triple ) < 1e-8f )
                {
                    continue;
                }
                for( const auto index : { size_t( 0 ), triple > 0 ? i : i + 1, triple > 0 ? i + 1 : i } )
                {
                    const auto &p = polygon[index];
                    const float u = .028f + .944f * float( .5 + p.along / ( 2 * halfLength ) );
                    const float v = .028f + .944f * float( .5 + p.across / ( 2 * halfWidth ) );
                    geometry.indices.push_back( u32( geometry.positions.size() ) );
                    geometry.positions.push_back( p.position + Vector3F( 0, lift, 0 ) );
                    geometry.normals.push_back( normal );
                    geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, handedness } );
                    geometry.uv.push_back( { ( tile + u ) * .25f, v, 0 } );
                }
            }
        }
    }

    std::pair<TextureBuffer, TextureBuffer> utilityAtlas( u64 seed )
    {
        constexpr u32 cell = 96;
        TextureBuffer colourAtlas( 4 * cell, cell ), normalAtlas( 4 * cell, cell );
        const auto frac = []( float x ) { return x - std::floor( x ); };
        const auto byte = []( float x ) { return u8( std::clamp( x, 0.f, 255.f ) ); };
        const auto smooth = []( float low, float high, float value ) {
            const float t = std::clamp( ( value - low ) / ( high - low ), 0.f, 1.f );
            return t * t * ( 3.f - 2.f * t );
        };
        const auto relief = [&]( u32 tile, s32 x, s32 y ) {
            x = std::clamp( x, 0, s32( cell ) - 1 );
            y = std::clamp( y, 0, s32( cell ) - 1 );
            const float u = ( x + .5f ) / cell * 2.f - 1.f;
            const float v = ( y + .5f ) / cell * 2.f - 1.f;
            const float round = std::hypot( u, v );
            const float square = std::max( std::abs( u ), std::abs( v ) ) +
                                 .17f * std::min( std::abs( u ), std::abs( v ) );
            if( tile < 2 )
            {
                const float edge = tile == 0 ? round : square;
                const float diamond = std::abs( frac( ( u + v ) * 6.2f ) - .5f ) +
                                      std::abs( frac( ( u - v ) * 6.2f ) - .5f );
                const float tread = 1.f - smooth( .27f, .38f, diamond );
                const float recess = smooth( .73f, .79f, edge ) * ( 1.f - smooth( .80f, .85f, edge ) );
                const float rim = smooth( .80f, .84f, edge ) * ( 1.f - smooth( .87f, .91f, edge ) );
                return .0007f + .0011f * tread - .0017f * recess + .0023f * rim;
            }
            const float jagged =
                .049f * std::sin( 12.f * u + 7.f * v ) + .031f * std::sin( 31.f * u - 13.f * v );
            const float edge = ( tile == 2 ? round : square ) + jagged;
            const float seam = smooth( .72f, .79f, edge ) * ( 1.f - smooth( .86f, .92f, edge ) );
            return .0005f * ( 1.f - smooth( .77f, .88f, edge ) ) - .0008f * seam;
        };
        for( u32 tile = 0; tile < 4; ++tile )
        {
            for( u32 y = 0; y < cell; ++y )
            {
                for( u32 x = 0; x < cell; ++x )
                {
                    const float u = ( x + .5f ) / cell * 2.f - 1.f;
                    const float v = ( y + .5f ) / cell * 2.f - 1.f;
                    const float round = std::hypot( u, v );
                    const float square = std::max( std::abs( u ), std::abs( v ) ) +
                                         .17f * std::min( std::abs( u ), std::abs( v ) );
                    const float grain =
                        cityHillside::unit( seed ^ ( tile * 19721u + y * cell + x ) ) - .5f;
                    float colour = 0, alpha = 0;
                    if( tile < 2 )
                    {
                        const float edge = tile == 0 ? round : square;
                        alpha = std::clamp( ( .913f - edge ) * 47.f, 0.f, 1.f );
                        const float diamond = std::abs( frac( ( u + v ) * 6.2f ) - .5f ) +
                                              std::abs( frac( ( u - v ) * 6.2f ) - .5f );
                        const float rib = diamond < .29f ? 15.f : diamond < .37f ? -11.f : 0.f;
                        const bool slot =
                            std::abs( std::abs( u ) - .53f ) < .075f && std::abs( v ) < .057f;
                        colour = 69.f + 15.f * grain + rib + ( slot ? -27.f : 0.f );
                        if( edge > .755f )
                        {
                            colour = 47.f + 9.f * grain + ( edge > .804f && edge < .854f ? 24.f : 0.f );
                        }
                    }
                    else
                    {
                        const float jagged = .049f * std::sin( 12.f * u + 7.f * v ) +
                                             .031f * std::sin( 31.f * u - 13.f * v );
                        const float edge = ( tile == 2 ? round : square ) + jagged;
                        const float fade = std::clamp( ( .92f - edge ) * 5.8f, 0.f, 1.f );
                        const float sealedEdge =
                            smooth( .72f, .79f, edge ) * ( 1.f - smooth( .86f, .92f, edge ) );
                        alpha = std::min( 1.f, fade * ( tile == 2 ? .64f : .56f ) + .08f * sealedEdge );
                        colour = 69.f + 28.f * grain + ( edge > .76f ? -7.f : 5.f ) - 13.f * sealedEdge;
                    }
                    auto *pixel = colourAtlas.pixel( tile * cell + x, y );
                    pixel[0] = byte( colour * 1.04f );
                    pixel[1] = byte( colour );
                    pixel[2] = byte( colour * .93f );
                    pixel[3] = byte( alpha * 255.f );

                    // A small tangent-space bevel makes the inset iron
                    // rim and the sealed repair edge catch grazing light.
                    const float metresPerPixel = tile < 2 ? .0085f : .018f;
                    const float dx = ( relief( tile, s32( x ) + 1, s32( y ) ) -
                                       relief( tile, s32( x ) - 1, s32( y ) ) ) /
                                     ( 2.f * metresPerPixel );
                    const float dy = ( relief( tile, s32( x ), s32( y ) + 1 ) -
                                       relief( tile, s32( x ), s32( y ) - 1 ) ) /
                                     ( 2.f * metresPerPixel );
                    const auto n = Vector3F( -dx, -dy, 1.f ).normaliseCopy();
                    auto *normal = normalAtlas.pixel( tile * cell + x, y );
                    normal[0] = byte( 127.5f * ( 1.f + n.x ) );
                    normal[1] = byte( 127.5f * ( 1.f + n.y ) );
                    normal[2] = byte( 127.5f * ( 1.f + n.z ) );
                    normal[3] = 255;
                }
            }
        }
        return { std::move( colourAtlas ), std::move( normalAtlas ) };
    }

    bool overlapsCrossing( const Patch &patch, const CityRoadCrossing &crossing )
    {
        const auto offset = patch.centre - crossing.centre;
        for( const auto axis : { patch.along, patch.across, crossing.outward, crossing.across } )
        {
            const float a = std::abs( axis.dotProduct( patch.along ) ) * patch.halfLength +
                            std::abs( axis.dotProduct( patch.across ) ) * patch.halfWidth;
            const float b =
                std::abs( axis.dotProduct( crossing.outward ) ) * ( crossing.depth * .5f + .16f ) +
                std::abs( axis.dotProduct( crossing.across ) ) * ( crossing.width * .5f + .05f );
            if( std::abs( offset.dotProduct( axis ) ) >= a + b )
            {
                return false;
            }
        }
        return true;
    }

    void build( SceneAssets &assets, const CityRoadTerrainMesh &walks, u32 seed,
                VehicleAppearanceQuality quality )
    {
        const int detail = std::clamp( assets.cityStreetDetail, 0, 100 );
        if( !detail )
        {
            return;
        }
        const auto layers = activeLayers( assets );
        if( !layers.road )
        {
            return;
        }
        const auto &source = assets.roadAuthoring->getSource();
        const auto &bake = assets.roadAuthoring->getBake();
        if( source.roads.size() != bake.roads.size() )
        {
            return;
        }
        SourceBudgetVector<Patch> patches;
        size_t slots = 0;
        constexpr size_t maximumCandidates = 512;
        const auto offer = [&]( Vector3F centre, Vector3F along, float lane, size_t family, bool iron,
                                bool square, u64 key ) {
            if( patches.size() >= maximumCandidates )
            {
                return;
            }
            const Vector3F across( -along.z, 0, along.x );
            const float cover = iron ? .34f + .097f * cityHillside::unit( key + 4 ) : 0.f;
            const float halfLength = iron ? cover + .27f + .15f * cityHillside::unit( key + 21 )
                                          : .78f + .52f * cityHillside::unit( key + 21 );
            const float halfWidth = iron ? cover + .24f : .36f + .20f * cityHillside::unit( key + 19 );
            const float range = std::hypot( halfLength + .105f, halfWidth + .105f );
            Patch patch{ centre + across * lane,
                         along,
                         across,
                         halfLength,
                         halfWidth,
                         cover,
                         range,
                         slots,
                         family,
                         square };
            if( std::any_of(
                    walks.crossings.begin(), walks.crossings.end(),
                    [&]( const auto &crossing ) { return overlapsCrossing( patch, crossing ); } ) )
            {
                return;
            }
            patches.push_back( std::move( patch ) );
        };

        // A service cover on each eligible approach gives the junction room
        // some scale. Try inside the pedestrian markings before the next block.
        for( size_t index = 0; index < walks.crossings.size(); ++index )
        {
            const auto &crossing = walks.crossings[index];
            const float half = crossing.width * .5f;
            if( half < 2.75f )
            {
                continue;
            }
            const auto key = render::foliageStableHash( ( u64( seed ) << 32 ) ^ 0x7614542bu ^ index );
            for( float side : { -1.f, 1.f } )
            {
                for( const float offset : { -1.44f, -2.72f, 1.70f, 3.05f } )
                {
                    const float station = offset + std::copysign( crossing.depth * .5f, offset );
                    offer( crossing.centre + crossing.outward * station, crossing.outward,
                           side * ( half * .53f + .17f * cityHillside::unit( key + 37 ) ),
                           crossing.roundabout ? 0 : 1, true, cityHillside::unit( key + 19 ) < .24f,
                           key ^ ( side < 0 ? 0x214be316u : 0xebb6be5fu ) );
                }
                ++slots;
            }
        }
        for( size_t roadIndex = 0; roadIndex < source.roads.size() && patches.size() < maximumCandidates;
             ++roadIndex )
        {
            const auto &samples = bake.roads[roadIndex].points;
            const float half = source.roads[roadIndex].width * .5f;
            if( samples.size() < 2 || half < 2.75f )
            {
                continue;
            }
            const float length = samples.back().distance - samples.front().distance;
            if( length < 21.f )
            {
                continue;
            }
            const auto stations = std::clamp( u32( std::ceil( length / 54.f ) ), 1u, 8u );
            for( u32 i = 0; i < stations; ++i )
            {
                const auto key = render::foliageStableHash( ( u64( seed ) << 32 ) ^ 0x9b71b6e1u ^
                                                            ( u64( roadIndex ) << 8 ) ^ i );
                const float t = ( i + .29f + .43f * cityHillside::unit( key ) ) / stations;
                const float distance = samples.front().distance + length * t;
                const auto b =
                    std::lower_bound( samples.begin(), samples.end(), distance,
                                      []( const auto &p, float value ) { return p.distance < value; } );
                if( b == samples.begin() || b == samples.end() )
                {
                    continue;
                }
                const auto &a = *( b - 1 );
                const float span = b->distance - a.distance;
                auto along = b->position - a.position;
                along.y = 0;
                if( span <= 0 || along.lengthSquared() < .001f )
                {
                    continue;
                }
                along.normalise();
                const auto centre =
                    a.position + ( b->position - a.position ) * ( ( distance - a.distance ) / span );
                const float lane = ( key & 1 ? 1.f : -1.f ) * half * .54f;
                const bool iron = cityHillside::unit( key + 3 ) > .51f;
                const bool square = cityHillside::unit( key + 5 ) > .65f;
                offer( centre, along, lane, 2, iron, square, key );
                offer( centre + along * 3.1f, along, -lane, 2, iron, square, key ^ 0x19c5ef72u );
                ++slots;
            }
        }
        if( patches.empty() )
        {
            return;
        }

        SourceBudgetMap<cityHillside::Cell, SourceBudgetVector<size_t>> candidates;
        for( size_t i = 0; i < patches.size(); ++i )
        {
            candidates[cityHillside::cellFor( patches[i].centre )].push_back( i );
        }
        const auto visitOverlappingPatches = [&]( const Triangle &face, const auto &accept ) {
            constexpr float search = 1.7f;
            const Vector3F low( std::min( { face.a.x, face.b.x, face.c.x } ) - search, 0,
                                std::min( { face.a.z, face.b.z, face.c.z } ) - search );
            const Vector3F high( std::max( { face.a.x, face.b.x, face.c.x } ) + search, 0,
                                 std::max( { face.a.z, face.b.z, face.c.z } ) + search );
            cityHillside::visitCells( low, high, [&]( auto cell ) {
                const auto found = candidates.find( cell );
                if( found == candidates.end() )
                {
                    return true;
                }
                for( const auto index : found->second )
                {
                    auto &patch = patches[index];
                    if( patch.blocked )
                    {
                        continue;
                    }
                    const auto p = patch.centre;
                    const float r = patch.range;
                    if( p.x + r < low.x + search || p.x - r > high.x - search ||
                        p.z + r < low.z + search || p.z - r > high.z - search )
                    {
                        continue;
                    }
                    accept( patch );
                }
                return true;
            } );
        };
        for( const auto &layer : { layers.paint, layers.island, layers.kerb } )
        {
            if( layer && !visitTriangles( layer, [&]( Vector3F a, Vector3F b, Vector3F c ) {
                    if( std::abs( cityHillside::cross( b - a, c - a ) ) < 1e-6f )
                    {
                        return;
                    }
                    const Triangle face{ a, b, c };
                    visitOverlappingPatches( face, [&]( Patch &patch ) {
                        std::array<Projected, 8> polygon{};
                        const auto count = clip( polygon, face, patch, patch.halfLength + .10f,
                                                 patch.halfWidth + .10f );
                        if( projectedArea( polygon, count ) > 1e-7 )
                        {
                            patch.blocked = true;
                        }
                    } );
                } ) )
            {
                return;
            }
        }
        if( !visitTriangles( layers.road, [&]( Vector3F a, Vector3F b, Vector3F c ) {
                const auto normal = ( b - a ).crossProduct( c - a ).normaliseCopy();
                if( std::abs( normal.y ) < .94f )
                {
                    return;
                }
                const Triangle face{ a, b, c };
                visitOverlappingPatches( face, [&]( Patch &patch ) {
                    std::array<Projected, 8> polygon{};
                    const auto count = clip( polygon, face, patch, patch.halfLength, patch.halfWidth );
                    const double area = projectedArea( polygon, count );
                    if( area <= 1e-10 )
                    {
                        return;
                    }
                    if( patch.triangles.size() == 96 )
                    {
                        patch.blocked = true;
                        return;
                    }
                    patch.triangles.push_back( face );
                    patch.supportedArea += area;
                } );
            } ) )
        {
            return;
        }

        Geometry asphalt, iron;
        SourceBudgetVector<u8> used( slots, 0 );
        SourceBudgetVector<Vector3F> installed;
        const bool preview = quality == VehicleAppearanceQuality::Preview;
        const size_t limits[] = { preview ? 7u : 18u, preview ? 9u : 23u, preview ? 8u : 21u };
        size_t counts[3] = {}, lids = 0;
        for( const auto &site : patches )
        {
            const size_t budget = ( limits[site.family] * detail + 99 ) / 100;
            const double footprint = 4. * site.halfLength * site.halfWidth;
            if( counts[site.family] >= budget || used[site.slot] || site.blocked ||
                std::abs( site.supportedArea - footprint ) > .00018 ||
                std::any_of( installed.begin(), installed.end(), [&]( Vector3F p ) {
                    return cityHillside::segmentDistance( p, site.centre, site.centre ) < 3.3f * 3.3f;
                } ) )
            {
                continue;
            }
            const auto begin = asphalt.indices.size();
            project( asphalt, site, site.halfLength, site.halfWidth,
                     site.cover > 0 && !site.square ? 2 : 3, .008f );
            if( asphalt.indices.size() == begin )
            {
                continue;
            }
            if( site.cover > 0 )
            {
                project( iron, site, site.cover, site.cover * ( site.square ? .86f : 1.f ),
                         site.square ? 1 : 0, .014f );
                ++lids;
            }
            used[site.slot] = true;
            installed.push_back( site.centre );
            ++counts[site.family];
        }
        if( asphalt.indices.empty() )
        {
            return;
        }
        const auto atlases = utilityAtlas( seed );
        const auto atlas = upload( assets, atlases.first, render::TextureMipFilter::Colour, 4 );
        const auto normalAtlas = upload( assets, atlases.second, render::TextureMipFilter::Normal, 4 );
        const auto publish = [&]( const char *name, const Geometry &geometry, float roughness,
                                  float metal, bool cutout ) {
            if( geometry.indices.empty() )
            {
                return;
            }
            auto surface = material( assets, ColourF::White, roughness, metal );
            surface->setTexture( atlas, 0 );
            surface->setTexture( normalAtlas, 1 );
            surface->setNormalStrength( cutout ? .65f : .38f );
            if( cutout )
            {
                surface->setCutout( true );
                surface->setAlphaClip( .5f );
                surface->setDepthWrite( true );
            }
            else
            {
                surface->setTransparent( true );
                surface->setBlendMode( 1 );
                surface->setDepthWrite( false );
            }
            surface->setCullMode( 2 );
            surface->setEditorUInt( "uvFilter", 3 );
            surface->setEditorFloat( "uvAniso", 8.f );
            if( auto actor = mesh( assets, name, geometry, surface ) )
            {
                actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                    scene::Renderer::CastShadows::Off );
            }
        };
        publish( "Terrain city carriageway asphalt repairs", asphalt, .97f, 0, false );
        publish( "Terrain city carriageway cast iron covers", iron, .68f, .26f, true );
        WP_LOG( String( "Terrain city carriageway utility covers=" ) + StringUtil::toString( lids ) +
                " repairs=" + StringUtil::toString( installed.size() ) + " roundabout=" +
                StringUtil::toString( counts[0] ) + " ordinary=" + StringUtil::toString( counts[1] ) );
    }
}  // namespace cityCarriagewayUtilities
