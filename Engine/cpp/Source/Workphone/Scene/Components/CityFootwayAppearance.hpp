#pragma once

        namespace cityFootwayAppearance
        {
            constexpr float curbTileMetres = 5.76f;
            // A larger repeat footprint prevents the same few slabs from
            // reappearing every couple of footsteps without enlarging the atlas.
            constexpr u32 pavingPairs = 8, pavingCourses = 14;
            static_assert( pavingCourses % 2u == 0u ); // Preserve alternating stagger across V repeats.
            constexpr float pavingPairMetres = .80f, pavingCourseMetres = .40f;
            constexpr float pavingTileU = pavingPairs * pavingPairMetres;
            constexpr float pavingTileV = pavingCourses * pavingCourseMetres;

            float unit( u64 key )
            {
                return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
            }

            float ramp( float low, float high, float value )
            {
                const float t = std::clamp( ( value - low ) / ( high - low ), 0.f, 1.f );
                return t * t * ( 3.f - 2.f * t );
            }

            u8 asByte( float value )
            {
                return u8( std::clamp( value + .5f, 0.f, 255.f ) );
            }

            float aggregateNoise( u32 seed, float u, float v, u32 columns, u32 rows )
            {
                const float x = u * columns, y = v * rows;
                const auto column = u32( x ), row = u32( y );
                const auto noise = [&]( u32 ix, u32 iy ) {
                    return unit( ( u64( seed ) << 32 ) | ( iy % rows * columns + ix % columns ) );
                };
                const float fx = ramp( 0, 1, x - column ), fy = ramp( 0, 1, y - row );
                const float upper = noise( column, row ) * ( 1.f - fx ) + noise( column + 1, row ) * fx;
                const float lower = noise( column, row + 1 ) * ( 1.f - fx ) + noise( column + 1, row + 1 ) * fx;
                return upper * ( 1.f - fy ) + lower * fy;
            }

            void stonePixels( TextureBuffer &colour, TextureBuffer &roughness, u32 x, u32 y,
                              float tone, float jointTone, float face, float wear, float aggregate, float warmth = 0.f )
            {
                auto pixel = colour.pixel( x, y );
                pixel[0] = asByte( jointTone + 3.f + ( tone - jointTone + warmth ) * face );
                pixel[1] = asByte( jointTone + 1.f + ( tone - jointTone + 1.f + .5f * warmth ) * face );
                pixel[2] = asByte( jointTone - 3.f + ( tone - jointTone + 1.f ) * face );
                pixel[3] = 255;
                auto rough = roughness.pixel( x, y );
                rough[0] = rough[1] = rough[2] = asByte( 245.f - face * ( 17.f + 13.f * wear - 7.f * aggregate ) );
                rough[3] = 255;
            }

            SmartPtr<render::IMaterial> stoneMaterial( SceneAssets &assets, const TextureBuffer &colour,
                                                       const TextureBuffer &roughness,
                                                       const SourceBudgetVector<float> &level,
                                                       float tileU, float tileV, bool wrapV, float strength,
                                                       const TextureBuffer *microNormal = nullptr )
            {
                const auto width = colour.width, height = colour.height;
                TextureBuffer normal( width, height );
                const auto point = [&]( u32 x, u32 y ) { return level[size_t( y ) * width + x]; };
                for( u32 y = 0; y < height; ++y )
                {
                    const auto next = wrapV ? ( y + 1 ) % height : std::min( y + 1, height - 1 );
                    const auto previous = y ? y - 1 : ( wrapV ? height - 1 : 0 );
                    for( u32 x = 0; x < width; ++x )
                    {
                        const float dx = ( point( ( x + 1 ) % width, y ) - point( ( x + width - 1 ) % width, y ) ) *
                                         ( width / ( 2.f * tileU ) );
                        const float dy = ( point( x, next ) - point( x, previous ) ) * ( height / ( 2.f * tileV ) );
                        auto n = Vector3F( -dx, -dy, 1 ).normaliseCopy();
                        if( microNormal )
                        {
                            const auto *detail = microNormal->pixel( x, y );
                            const float weight = .82f * detail[3] / 255.f;
                            const Vector3F grain( detail[0] / 127.5f - 1.f,
                                                  detail[1] / 127.5f - 1.f,
                                                  detail[2] / 127.5f - 1.f );
                            n = Vector3F( n.x + weight * grain.x, n.y + weight * grain.y,
                                          n.z * ( 1.f - weight + weight * grain.z ) ).normaliseCopy();
                        }
                        auto pixel = normal.pixel( x, y );
                        pixel[0] = asByte( 127.5f * ( 1.f + n.x ) );
                        pixel[1] = asByte( 127.5f * ( 1.f + n.y ) );
                        pixel[2] = asByte( 127.5f * ( 1.f + n.z ) );
                        pixel[3] = 255;
                    }
                }
                auto result = material( assets, ColourF::White, 1.f );
                result->setTexture( upload( assets, colour ), 0 );
                result->setTexture( upload( assets, normal, render::TextureMipFilter::Normal ), 1 );
                result->setTexture( upload( assets, roughness, render::TextureMipFilter::Roughness ), 3 );
                result->setUVProjection( 0 );
                result->setNormalStrength( strength );
                result->setEditorUInt( "uvFilter", 3 );
                result->setEditorFloat( "uvAniso", 8.f );
                return result;
            }

            SmartPtr<render::IMaterial> curbMaterial( SceneAssets &assets, u32 seed )
            {
                constexpr u32 width = 512, height = 64;
                constexpr std::array<float, 9> joints{ 0.f, .70f, 1.43f, 2.09f, 2.87f, 3.57f, 4.33f, 5.02f,
                                                       curbTileMetres };
                TextureBuffer colour( width, height ), roughness( width, height );
                SourceBudgetVector<float> level( size_t( width ) * height );
                for( u32 y = 0; y < height; ++y )
                {
                    const float v = ( y + .5f ) / height;
                    for( u32 x = 0; x < width; ++x )
                    {
                        const float u = ( x + .5f ) / width;
                        const float station = u * curbTileMetres;
                        const auto next = std::upper_bound( joints.begin(), joints.end(), station );
                        const auto course = u32( next - joints.begin() - 1 );
                        const auto block = unit( seed + 1991u + course );
                        const float fine = aggregateNoise( seed + 71u, u, v, 196, 13 );
                        const float mineral = aggregateNoise( seed + 101u, u, v, 66, 5 );
                        const float weather = aggregateNoise( seed + 181u, u, v, 23, 7 );
                        const float streak = aggregateNoise( seed + 187u, u, v, 72, 3 );
                        const float jointDistance = std::min( station - joints[course], *next - station );
                        const float face = ramp( .004f, .020f, jointDistance - .004f * mineral );
                        const float wornEdge = 1.f - ramp( .05f, .37f, v );
                        const float dirt = ramp( .40f, .96f, v ) * ( .25f + .75f * mineral );
                        // Road spray and runoff darken the lower kerb irregularly;
                        // the cap remains lighter where feet and tyres polish it.
                        const float waterline = ramp( .52f, .90f, v ) *
                                                ( .35f + .65f * ramp( .55f, .84f, weather ) * streak );
                        const float capChips = ( 1.f - ramp( .03f, .12f, v ) ) * ramp( .78f, .93f, fine );
                        const float granite = 140.f + 16.f * block + 13.f * ( fine - .5f ) +
                                              10.f * wornEdge - 18.f * dirt - 11.f * waterline + 5.f * capChips;
                        stonePixels( colour, roughness, x, y, granite, 108.f + 9.f * mineral, face, wornEdge, fine );
                        level[size_t( y ) * width + x] = face *
                            ( .0034f + .0012f * fine - .0011f * wornEdge - .0003f * waterline );
                    }
                }
                return stoneMaterial( assets, colour, roughness, level, curbTileMetres, .15f, false, .57f );
            }

            SmartPtr<render::IMaterial> pavingMaterial( SceneAssets &assets, u32 seed )
            {
                // Keep nominal slab dimensions and narrow joints, but vary the
                // stagger and cut slightly so runs do not form a mechanical grid.
                constexpr u32 width = 1024, height = 1024;
                TextureBuffer colour( width, height ), roughness( width, height );
                SourceBudgetVector<float> level( size_t( width ) * height );
                // Use only the interiors of the bundled photographed pavers.
                // Sampling the whole scan would stamp its mortar joints across
                // the separately authored slab joints below.
                constexpr u32 photoWidth = 100, photoHeight = 60;
                constexpr std::array<std::array<u32, 2>, 15> photoPatches{{
                    { 100, 12 }, { 270, 12 },
                    { 15, 97 }, { 185, 97 }, { 355, 97 },
                    { 100, 182 }, { 270, 182 },
                    { 15, 267 }, { 185, 267 }, { 355, 267 },
                    { 100, 352 }, { 270, 352 },
                    { 15, 437 }, { 185, 437 }, { 355, 437 }
                }};
                procedural::SurfaceBakeResult photo;
                String photoDiagnostic;
                const bool hasPhoto = loadCitySurface( CitySurface::Paving, 512, photo, photoDiagnostic );
                std::unique_ptr<TextureBuffer> microNormal;
                std::array<float, 256> toLinear{};
                std::array<u8, 4097> toSrgb{};
                std::array<std::array<float, 3>, photoPatches.size()> patchMeans{};
                if( hasPhoto )
                {
                    microNormal = std::make_unique<TextureBuffer>( width, height );
                    for( u32 i = 0; i < toLinear.size(); ++i )
                    {
                        const float value = i / 255.f;
                        toLinear[i] = value <= .04045f ? value / 12.92f :
                                      std::pow( ( value + .055f ) / 1.055f, 2.4f );
                    }
                    for( u32 i = 0; i < toSrgb.size(); ++i )
                    {
                        const float value = i / 4096.f;
                        toSrgb[i] = asByte( 255.f * ( value <= .0031308f ? value * 12.92f :
                                             1.055f * std::pow( value, 1.f / 2.4f ) - .055f ) );
                    }
                    for( size_t i = 0; i < photoPatches.size(); ++i )
                    {
                        const auto &patch = photoPatches[i];
                        auto &mean = patchMeans[i];
                        for( u32 y = 0; y < photoHeight; ++y )
                            for( u32 x = 0; x < photoWidth; ++x )
                            {
                                const auto *pixel = photo.albedo.pixel( patch[0] + x, patch[1] + y );
                                for( u32 c = 0; c < 3; ++c ) mean[c] += toLinear[pixel[c]];
                            }
                        for( auto &channel : mean ) channel /= photoWidth * photoHeight;
                    }
                }
                const auto encodeLinear = [&]( float value ) {
                    return toSrgb[u32( std::clamp( std::lround( value * 4096.f ), 0l, 4096l ) )];
                };
                const auto photoSample = [&]( const TextureBuffer &map, float px, float py, bool albedo ) {
                    const u32 x0 = u32( px ), y0 = u32( py );
                    const u32 x1 = std::min( x0 + 1, map.width - 1 ), y1 = std::min( y0 + 1, map.height - 1 );
                    const float u = px - x0, v = py - y0;
                    const auto *a = map.pixel( x0, y0 ), *b = map.pixel( x1, y0 );
                    const auto *c = map.pixel( x0, y1 ), *d = map.pixel( x1, y1 );
                    std::array<float, 4> result{};
                    for( u32 channel = 0; channel < 4; ++channel )
                    {
                        const auto value = [&]( const u8 *pixel ) {
                            return albedo && channel < 3 ? toLinear[pixel[channel]] : pixel[channel] / 255.f;
                        };
                        result[channel] = ( value( a ) * ( 1.f - u ) + value( b ) * u ) * ( 1.f - v ) +
                                          ( value( c ) * ( 1.f - u ) + value( d ) * u ) * v;
                    }
                    return result;
                };
                for( u32 y = 0; y < height; ++y )
                {
                    const float v = ( y + .5f ) / height;
                    const float across = v * pavingCourses;
                    const auto row = u32( across );
                    const float withinRow = ( across - row ) * pavingCourseMetres;
                    const float rowDistance = std::min( withinRow, pavingCourseMetres - withinRow );
                    const float stagger = ( row % 2 ? .40f : .13f ) +
                                          .09f * ( unit( ( u64( seed + 829u ) << 32 ) | row ) - .5f );
                    for( u32 x = 0; x < width; ++x )
                    {
                        const float u = ( x + .5f ) / width;
                        const float along = u * pavingTileU + stagger;
                        const auto pair = u32( along / pavingPairMetres );
                        // Both sides of the U seam use the same cut and slab identity.
                        const auto wrappedPair = pair % pavingPairs;
                        const float withinPair = along - pair * pavingPairMetres;
                        const float longSlab = .52f + .05f *
                            ( unit( ( u64( seed + 839u ) << 32 ) | ( row * pavingPairs + wrappedPair ) ) - .5f );
                        const bool shortSlab = withinPair > longSlab;
                        const float slabDistance = shortSlab ? std::min( withinPair - longSlab,
                                                                       pavingPairMetres - withinPair ) :
                                                              std::min( withinPair, longSlab - withinPair );
                        const float fine = aggregateNoise( seed + 221u, u, v, 768, 640 );
                        const float mineral = aggregateNoise( seed + 467u, u, v, 167, 139 );
                        const float bloom = aggregateNoise( seed + 463u, u, v, 49, 39 );
                        const float weather = aggregateNoise( seed + 727u, u, v, 12, 10 );
                        const float runoff = aggregateNoise( seed + 733u, u, v, 52, 26 );
                        const float broadWeather = aggregateNoise( seed + 743u, u, v, 4, 4 );
                        const auto slab = ( row * pavingPairs + wrappedPair ) * 2u + u32( shortSlab );
                        const u64 slabKey = ( u64( seed ) << 32 ) | slab;
                        const float variation = unit( slabKey );
                        const float wear = unit( slabKey ^ 0x9829a1u );
                        const float replacement = unit( slabKey ^ 0x61ab293du ) > .962f ? 1.f : 0.f;
                        const float distance = std::min( rowDistance, slabDistance );
                        // Aggregate catches at some joints and knocks back a few
                        // arrises without turning the mortar into a crack grid.
                        const float edgeChips = ( 1.f - ramp( .005f, .028f, distance ) ) *
                                                ramp( .71f, .91f, mineral ) * ( .38f + .62f * ( 1.f - wear ) );
                        const float face = ramp( .0025f, .0105f,
                                                 distance - .0015f * fine - .0022f * edgeChips );
                        const float patina = ramp( .55f, .88f, bloom ) * ( 1.f - wear );
                        const float rainStain = ramp( .61f, .83f, weather ) *
                                                ( .35f + .65f * runoff ) * ( .65f + .35f * ( 1.f - wear ) );
                        const float damp = ramp( .58f, .82f, broadWeather ) * ( .35f + .65f * runoff );
                        const float grain = unit( ( u64( seed + 529u ) << 32 ) | ( y * width + x ) );
                        const float lightMineral = ramp( .67f, .86f, fine );
                        const float pore = 1.f - ramp( .045f, .17f, fine );
                        const float edgeDirt = ( 1.f - ramp( .010f, .048f, distance ) ) *
                                               ( .35f + .65f * mineral ) * ( .30f + .70f * ( 1.f - wear ) );
                        const float concrete = 139.f + 22.f * variation + 11.f * ( fine - .5f ) +
                                               5.f * ( mineral - .5f ) + 4.f * ( grain - .5f ) +
                                               5.f * lightMineral - 8.f * pore - 9.f * patina -
                                               12.f * edgeDirt - 12.f * rainStain -
                                               8.f * damp - 4.f * edgeChips - 11.f * replacement;
                        stonePixels( colour, roughness, x, y, concrete,
                                     108.f + 7.f * bloom - 3.f * damp,
                                     face, wear, fine, -2.f + 8.f * wear + 4.f * variation -
                                     2.f * replacement );
                        auto rough = roughness.pixel( x, y );
                        rough[0] = rough[1] = rough[2] = asByte(
                            233.f - face * ( 8.f + 11.f * wear ) + 4.f * pore -
                            7.f * lightMineral + 5.f * rainStain + 5.f * damp );
                        if( hasPhoto )
                        {
                            const u64 sampleKey = render::foliageStableHash( slabKey ^ 0x706176657273636eull );
                            const size_t patchIndex = size_t( ( sampleKey >> 16 ) % photoPatches.size() );
                            const auto &patch = photoPatches[patchIndex];
                            const float slabStart = shortSlab ? longSlab : 0.f;
                            const float slabLength = shortSlab ? pavingPairMetres - longSlab : longSlab;
                            const float slabU = std::clamp( ( withinPair - slabStart ) / slabLength, 0.f, 1.f );
                            const float slabV = std::clamp( withinRow / pavingCourseMetres, 0.f, 1.f );
                            const bool flipU = ( sampleKey & 1u ) != 0, flipV = ( sampleKey & 2u ) != 0;
                            const float px = patch[0] + ( flipU ? 1.f - slabU : slabU ) * ( photoWidth - 1 );
                            const float py = patch[1] + ( flipV ? 1.f - slabV : slabV ) * ( photoHeight - 1 );
                            const auto photographed = photoSample( photo.albedo, px, py, true );
                            const auto &mean = patchMeans[patchIndex];
                            const float luminance = .22f * photographed[0] + .70f * photographed[1] +
                                                    .08f * photographed[2];
                            const float meanLuminance = .22f * mean[0] + .70f * mean[1] + .08f * mean[2];
                            // A dark source seam must never become a second joint
                            // inside one of our generated concrete slabs.
                            const float photoFace = face * ramp( meanLuminance - .16f,
                                                                 meanLuminance - .09f, luminance );
                            auto *paint = colour.pixel( x, y );
                            for( u32 c = 0; c < 3; ++c )
                            {
                                const float base = toLinear[paint[c]];
                                paint[c] = encodeLinear( std::clamp( base + photoFace *
                                    ( .95f * ( photographed[c] - mean[c] ) + .16f * ( mean[c] - base ) ),
                                    0.f, 1.f ) );
                            }
                            const auto scannedRoughness = photoSample( photo.orm, px, py, false )[1];
                            const float response = ( 1.f - .30f * photoFace ) * rough[0] / 255.f +
                                                   .30f * photoFace * scannedRoughness;
                            rough[0] = rough[1] = rough[2] = asByte( response * 255.f );
                            const auto sampledNormal = photoSample( photo.normal, px, py, false );
                            auto detail = Vector3F( sampledNormal[0] * 2.f - 1.f,
                                                    sampledNormal[1] * 2.f - 1.f,
                                                    sampledNormal[2] * 2.f - 1.f );
                            if( flipU ) detail.x = -detail.x;
                            if( flipV ) detail.y = -detail.y;
                            detail.normalise();
                            auto *normal = microNormal->pixel( x, y );
                            normal[0] = asByte( 127.5f * ( detail.x + 1.f ) );
                            normal[1] = asByte( 127.5f * ( detail.y + 1.f ) );
                            normal[2] = asByte( 127.5f * ( detail.z + 1.f ) );
                            normal[3] = asByte( photoFace * 255.f );
                        }
                        level[size_t( y ) * width + x] =
                            face * ( .0026f + .0011f * variation + .00042f * fine + .00018f * grain -
                                     .00040f * pore + .00012f * lightMineral - .0003f * rainStain -
                                     .00022f * edgeChips );
                    }
                }
                return stoneMaterial( assets, colour, roughness, level, pavingTileU, pavingTileV,
                                      true, .64f, microNormal.get() );
            }

            SmartPtr<render::IMaterial> sharedTopMaterial( SceneAssets &assets, SmartPtr<render::IMaterial> shared )
            {
                auto result = material( assets, shared->getDiffuse(), shared->getRoughness(), shared->getMetalness() );
                for( const u32 layer : { 0u, 1u, 2u, 3u, 22u } )
                    if( auto texture = shared->getTexture( layer ) ) result->setTexture( texture, layer );
                result->setUVProjection( shared->getUVProjection() );
                result->setTriplanarScale( shared->getTriplanarScale() );
                result->setNormalStrength( .59f );
                result->setEditorFloat( "aoStrength", .82f );
                result->setEditorUInt( "uvFilter", 3 );
                result->setEditorFloat( "uvAniso", 8.f );
                return result;
            }

            void addVertex( Geometry &geometry, const CityRoadTerrainVertex &vertex,
                            Vector2F uv, Vector3F tangent, float handedness = 1.f )
            {
                tangent -= vertex.normal * tangent.dotProduct( vertex.normal );
                tangent.normalise();
                geometry.positions.push_back( vertex.position );
                geometry.normals.push_back( vertex.normal );
                geometry.uv.push_back( { uv.x, uv.y, 0 } );
                geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, handedness } );
            }

            SmartPtr<IMesh> pack( const Geometry &geometry )
            {
                return MeshUtil::createMesh( geometry.positions, geometry.normals, geometry.tangents,
                                             geometry.uv, geometry.indices );
            }

            constexpr u32 missing = std::numeric_limits<u32>::max();
            using Node = std::pair<s64, s64>;
            Node node( Vector3F point )
            {
                return { std::llround( double( point.x ) * 512 ), std::llround( double( point.z ) * 512 ) };
            }

            struct Edge
            {
                std::array<u32, 4> vertices;
                Vector3F direction;
                float length, u0 = 0, u1 = 0;
                u32 next = missing, previous = missing;
                bool assigned = false;
            };

            void assignStations( SourceBudgetVector<Edge> &edges, const CityRoadTerrainMesh &fitted, u32 seed )
            {
                SourceBudgetMap<Node, SourceBudgetVector<u32>> outgoing;
                for( u32 i = 0; i < edges.size(); ++i )
                    outgoing[node( fitted.vertices[edges[i].vertices[0]].position )].push_back( i );
                for( u32 i = 0; i < edges.size(); ++i )
                {
                    const auto end = fitted.vertices[edges[i].vertices[3]].position;
                    const auto found = outgoing.find( node( end ) );
                    if( found == outgoing.end() ) continue;
                    u32 best = missing;
                    float alignment = -.55f;
                    for( const auto next : found->second )
                    {
                        if( next == i ) continue;
                        const auto start = fitted.vertices[edges[next].vertices[0]].position;
                        const float score = edges[i].direction.dotProduct( edges[next].direction );
                        if( ( start - end ).lengthSquared() > .000016f || score <= alignment ) continue;
                        if( edges[next].previous != missing &&
                            edges[edges[next].previous].direction.dotProduct( edges[next].direction ) >= score ) continue;
                        alignment = score;
                        best = next;
                    }
                    if( best == missing ) continue;
                    auto &next = edges[best];
                    if( next.previous != missing ) edges[next.previous].next = missing;
                    next.previous = i;
                    edges[i].next = best;
                }

                SourceBudgetVector<u32> contour;
                for( const bool open : { true, false } )
                {
                    for( u32 first = 0; first < edges.size(); ++first )
                    {
                        if( edges[first].assigned || ( edges[first].previous == missing ) != open ) continue;
                        contour.clear();
                        float length = 0;
                        u32 index = first;
                        while( index != missing && !edges[index].assigned )
                        {
                            auto &edge = edges[index];
                            edge.assigned = true;
                            length += edge.length;
                            contour.push_back( index );
                            index = edge.next;
                        }
                        const bool closed = index == first && length >= 2.8f;
                        const float tile = closed ? length / std::max( 1.f, std::round( length / curbTileMetres ) ) :
                                                   curbTileMetres;
                        const auto start = node( fitted.vertices[edges[first].vertices[0]].position );
                        const float phase = unit( ( u64( u32( start.first ) ) << 32 ) | ( u32( start.second ) ^ seed ) );
                        float station = 0;
                        for( const auto member : contour )
                        {
                            auto &edge = edges[member];
                            edge.u0 = phase + station / tile;
                            station += edge.length;
                            edge.u1 = phase + station / tile;
                        }
                    }
                }
            }

            struct Appearance
            {
                SmartPtr<IMesh> mesh;
                SmartPtr<render::IMaterial> slab, curb;
            };

            Appearance prepare( SceneAssets &assets, const CityRoadTerrainMesh &fitted,
                                SmartPtr<render::IMaterial> paving, u32 seed )
            {
                if( fitted.indices.empty() ) return {};
                auto edgeStart = fitted.indices.size();
                for( size_t i = 0; i + 2 < fitted.indices.size(); i += 3 )
                {
                    const auto &top = fitted.vertices[fitted.indices[i]];
                    const auto &bottom = fitted.vertices[fitted.indices[i + 1]];
                    if( std::abs( top.normal.y ) < .01f && top.position.y - bottom.position.y > .012f &&
                        std::abs( top.position.x - bottom.position.x ) < .001f &&
                        std::abs( top.position.z - bottom.position.z ) < .001f )
                    {
                        edgeStart = i;
                        break;
                    }
                }

                SourceBudgetVector<Edge> edges;
                size_t vertexCount = fitted.vertices.size();
                for( size_t i = edgeStart; i + 5 < fitted.indices.size(); i += 6 )
                {
                    const auto *j = fitted.indices.data() + i;
                    const auto &a = fitted.vertices[j[0]], &b = fitted.vertices[j[1]];
                    const auto &c = fitted.vertices[j[2]], &d = fitted.vertices[j[5]];
                    auto delta = d.position - a.position;
                    delta.y = 0;
                    const float length = delta.length();
                    if( j[0] != j[3] || j[2] != j[4] || length < .00001f ||
                        std::abs( a.normal.y ) > .01f || a.position.y - b.position.y < .012f ||
                        std::abs( a.position.x - b.position.x ) > .001f ||
                        std::abs( a.position.z - b.position.z ) > .001f ||
                        std::abs( d.position.x - c.position.x ) > .001f ||
                        std::abs( d.position.z - c.position.z ) > .001f ) break;
                    edges.push_back( { { j[0], j[1], j[2], j[5] }, delta / length, length } );
                    vertexCount = std::min( vertexCount, size_t( std::min( { j[0], j[1], j[2], j[5] } ) ) );
                }
                const auto topEnd = fitted.indices.begin() + edgeStart;
                const bool sectioned = !edges.empty() && edgeStart > 0 && edgeStart + edges.size() * 6 == fitted.indices.size() &&
                    std::all_of( fitted.indices.begin(), topEnd, [vertexCount]( u32 index ) { return index < vertexCount; } );

                Geometry slab;
                for( size_t i = 0; i < ( sectioned ? vertexCount : fitted.vertices.size() ); ++i )
                {
                    const auto &vertex = fitted.vertices[i];
                    if( sectioned )
                    {
                        const auto tangent = std::abs( vertex.normal.y ) > .001f ?
                            Vector3F( vertex.normal.y, -vertex.normal.x, 0.f ) :
                            Vector3F( -vertex.normal.z, 0.f, vertex.normal.x );
                        addVertex( slab, vertex, { vertex.position.x / pavingTileU, vertex.position.z / pavingTileV },
                                   tangent, -1.f );
                    }
                    else
                    {
                        const auto tangent = std::abs( vertex.normal.y ) > .9f ? Vector3F( 1, 0, 0 ) :
                                             Vector3F( -vertex.normal.z, 0, vertex.normal.x );
                        addVertex( slab, vertex, vertex.uv, tangent );
                    }
                }
                const auto slabEnd = sectioned ? topEnd : fitted.indices.end();
                slab.indices.reserve( slabEnd - fitted.indices.begin() );
                for( auto index = fitted.indices.begin(); index != slabEnd; ++index )
                    slab.indices.push_back( *index );
                auto mesh = pack( slab );
                if( !sectioned ) return { mesh, sharedTopMaterial( assets, paving ), nullptr };

                assignStations( edges, fitted, seed );
                Geometry curb;
                for( const auto &edge : edges )
                {
                    const auto base = u32( curb.positions.size() );
                    for( u32 i = 0; i < edge.vertices.size(); ++i )
                        addVertex( curb, fitted.vertices[edge.vertices[i]],
                                   { i < 2 ? edge.u0 : edge.u1, i == 0 || i == 3 ? .045f : .955f },
                                   edge.direction );
                    // The original (top A, bottom A, bottom B), (top A, bottom B,
                    // top B) faces are retained once, with their exact XYZ and winding.
                    for( const auto index : { 0u, 1u, 2u, 0u, 2u, 3u } ) curb.indices.push_back( base + index );
                }
                for( auto section : pack( curb )->getSubMeshes() ) mesh->addSubMesh( section );
                return { mesh, pavingMaterial( assets, seed ), curbMaterial( assets, seed ) };
            }
        }  // namespace cityFootwayAppearance
