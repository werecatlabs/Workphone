#pragma once

        namespace cityBuildingImposters
        {
            constexpr float minimumFarMeshCoverage = .025f;
            constexpr size_t maximumSourceTriangles = 32768;

            struct Baked
            {
                MeshImposterAtlas atlas;
                Vector3F centre, minimum, maximum;
                size_t sourceTriangles;
            };

            // Keep one accepted custom-module placement in both LOD meshes.
            // Copying its emitted range avoids a second kit placement or budget charge.
            void appendPlacedModule( Geometry &target, const Geometry &source,
                                     size_t firstVertex, size_t firstIndex )
            {
                const auto base = u32( target.positions.size() );
                for( size_t i = firstVertex; i < source.positions.size(); ++i )
                {
                    target.positions.push_back( source.positions[i] );
                    target.normals.push_back( source.normals[i] );
                    target.uv.push_back( source.uv[i] );
                    target.tangents.push_back( source.tangents[i] );
                }
                for( size_t i = firstIndex; i < source.indices.size(); ++i )
                    target.indices.push_back( base + source.indices[i] - u32( firstVertex ) );
            }

            template <class Patch>
            std::optional<Baked> bake( const Patch &patch, VehicleAppearanceQuality quality )
            {
                // Median linear reflectance of the shipped scans. Plaster and
                // concrete include the tint used on the corresponding LOD1 mesh.
                const std::array<std::pair<const Geometry *, ColourF>, 8> surfaces{{
                    { &patch.farWalls[0], { .122f, .063f, .041f, 1.f } },
                    { &patch.farWalls[1], { .247f, .214f, .156f, 1.f } },
                    { &patch.farWalls[2], { .306f, .295f, .266f, 1.f } },
                    { &patch.farRoofs, { .18f, .21f, .23f, 1.f } },
                    { &patch.farPitchedRoofs.slate, { .108f, .123f, .130f, 1.f } },
                    { &patch.farPitchedRoofs.clay, { .375f, .175f, .100f, 1.f } },
                    { &patch.farTiles, { .435f, .20f, .108f, 1.f } },
                    { &patch.farGlass, { .120f, .146f, .161f, 1.f } }
                }};
                size_t sourceTriangles = 0;
                for( const auto &surface : surfaces )
                {
                    const auto indices = surface.first->indices.size();
                    const auto count = indices / 3;
                    if( indices % 3 || count > maximumSourceTriangles - sourceTriangles )
                        return {};
                    sourceTriangles += count;
                }
                if( sourceTriangles == 0 ) return {};

                auto triangles = makeSourceBudgetSnapshot<MeshImposterTriangle>();
                triangles.reserve( sourceTriangles );
                const auto bound = std::numeric_limits<float>::max();
                Vector3F minimum( bound, bound, bound ), maximum( -bound, -bound, -bound );
                for( const auto &[geometry, colour] : surfaces )
                {
                    for( size_t i = 0; i < geometry->indices.size(); i += 3 )
                    {
                        MeshImposterTriangle triangle{ {}, colour };
                        for( size_t corner = 0; corner < triangle.positions.size(); ++corner )
                        {
                            const auto index = geometry->indices[i + corner];
                            if( index >= geometry->positions.size() ) return {};
                            const auto p = geometry->positions[index];
                            if( !MathUtil<f32>::isFinite( p ) ) return {};
                            minimum.x = std::min( minimum.x, p.x );
                            minimum.y = std::min( minimum.y, p.y );
                            minimum.z = std::min( minimum.z, p.z );
                            maximum.x = std::max( maximum.x, p.x );
                            maximum.y = std::max( maximum.y, p.y );
                            maximum.z = std::max( maximum.z, p.z );
                            triangle.positions[corner] = p;
                        }
                        triangles.push_back( triangle );
                    }
                }

                const auto extent = maximum - minimum;
                if( !MathUtil<f32>::isFinite( extent ) || std::min( { extent.x, extent.y, extent.z } ) < .01f )
                    return {};
                // These are true referenced bounds, including pitched roofs and
                // the accepted kit geometry. The baker supplies transparent texel gutters.
                const auto centre = minimum + extent * .5f;
                const Vector3F margin( .025f, .025f, .025f );
                minimum -= margin;
                maximum += margin;
                for( auto &triangle : triangles )
                    for( auto &point : triangle.positions ) point -= centre;

                const u32 resolution = quality == VehicleAppearanceQuality::Preview ? 64u : 128u;
                auto atlas = generateMeshImposters( triangles, minimum - centre, maximum - centre,
                                                    resolution, resolution );
                for( size_t view = 0; view < atlas.views.size(); ++view )
                {
                    bool occupied = false;
                    for( u32 y = 1; y + 1 < atlas.height && !occupied; ++y )
                    {
                        for( u32 x = 1; x + 1 < atlas.tileWidth; ++x )
                        {
                            const auto pixel = ( size_t( y ) * atlas.width + view * atlas.tileWidth + x ) * 4;
                            if( !atlas.rgba[pixel + 3] ) continue;
                            occupied = true;
                            break;
                        }
                    }
                    if( !occupied ) return {};
                }
                return Baked{ std::move( atlas ), centre, minimum, maximum, sourceTriangles };
            }

            Geometry cards( const Baked &baked )
            {
                Geometry geometry;
                const auto &atlas = baked.atlas;
                for( size_t i = 0; i < atlas.views.size(); ++i )
                {
                    const auto &view = atlas.views[i];
                    // Side cards cross at the patch centre. The overhead chart sits
                    // in the upper storeys, within the same fitted local bounds.
                    const auto centre = baked.centre + ( i == 2 ?
                        Vector3F( 0, ( baked.maximum.y - baked.minimum.y ) * .28f, 0 ) : Vector3F() );
                    const auto point = [&]( float x, float y ) { return centre + view.right * x + view.up * y; };
                    const auto base = geometry.positions.size();
                    geometry.quad( point( view.minimum.x, view.minimum.y ), point( view.maximum.x, view.minimum.y ),
                                   point( view.maximum.x, view.maximum.y ), point( view.minimum.x, view.maximum.y ),
                                   view.towardCamera );
                    const auto u0 = float( i * atlas.tileWidth + 1 ) / atlas.width;
                    const auto u1 = float( ( i + 1 ) * atlas.tileWidth - 2 ) / atlas.width;
                    const auto top = 1.f / atlas.height, bottom = float( atlas.height - 2 ) / atlas.height;
                    geometry.uv[base] = { u0, bottom, 0 };
                    geometry.uv[base + 1] = { u1, bottom, 0 };
                    geometry.uv[base + 2] = { u1, top, 0 };
                    geometry.uv[base + 3] = { u0, top, 0 };
                    for( size_t vertex = base; vertex < base + 4; ++vertex )
                        geometry.tangents[vertex] = { view.right.x, view.right.y, view.right.z, -1.f };
                }
                return geometry;
            }

            SmartPtr<scene::MeshRenderer> publish( SceneAssets &assets, const Baked &baked,
                                                  SmartPtr<IGameActor> root )
            {
                const auto &atlas = baked.atlas;
                auto texture = uploadPixels( assets, atlas.rgba.data(), atlas.rgba.size(), atlas.width, atlas.height,
                                             render::TextureMipFilter::Cutout, u32( atlas.views.size() ) );
                auto surface = material( assets, ColourF::White, 1.f );
                surface->setTexture( texture, 0 );
                surface->setTexture( texture, 13 );
                surface->setDiffuse( ColourF::Black );
                surface->setEmissive( ColourF::White );
                surface->setEmissionEnabled( true );
                surface->setCutout( true );
                surface->setAlphaClip( .5f );
                surface->setTransparent( false );
                surface->setDepthWrite( true );
                surface->setEditorUInt( "uvFilter", 3 );
                surface->setEditorFloat( "uvAniso", 8.f );
                if( auto actor = mesh( assets, "LOD2 city building imposters", cards( baked ), surface, root ) )
                {
                    auto renderer = actor->getComponent<scene::MeshRenderer>();
                    renderer->setCastShadows( scene::Renderer::CastShadows::Off );
                    WP_LOG( String( "City building imposter atlas " ) + StringUtil::toString( atlas.width ) +
                        "x" + StringUtil::toString( atlas.height ) + " sourceTriangles=" +
                        StringUtil::toString( baked.sourceTriangles ) );
                    return renderer;
                }
                return nullptr;
            }

            template <class Patch>
            SmartPtr<scene::MeshRenderer> build( SceneAssets &assets, const Patch &patch,
                                                SmartPtr<IGameActor> root, VehicleAppearanceQuality quality )
            {
                const auto baked = bake( patch, quality );
                return baked ? publish( assets, *baked, root ) : nullptr;
            }
        }
