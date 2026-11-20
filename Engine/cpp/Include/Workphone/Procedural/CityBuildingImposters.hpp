#pragma once

namespace cityBuildingImposters
{
    constexpr float minimumFarMeshCoverage = .025f;
    constexpr size_t maximumSourceTriangles = 32768;

    struct Baked
    {
        MeshImposterSpriteSheet atlas;
        Vector3F centre, minimum, maximum;
        size_t sourceTriangles;
    };

    // Keep one accepted custom-module placement in both LOD meshes.
    // Copying its emitted range avoids a second kit placement or budget charge.
    void appendPlacedModule( Geometry &target, const Geometry &source, size_t firstVertex,
                             size_t firstIndex )
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
        {
            target.indices.push_back( base + source.indices[i] - u32( firstVertex ) );
        }
    }

    template <class Patch>
    auto surfaces( const Patch &patch )
    {
        // Median linear reflectance of the shipped scans. Plaster and
        // concrete include the tint used on the corresponding LOD1 mesh.
        return std::array<std::pair<const Geometry *, ColourF>, 10>{
            { { &patch.farWalls[0], { .122f, .063f, .041f, 1.f } },
              { &patch.farWalls[1], { .247f, .214f, .156f, 1.f } },
              { &patch.farWalls[2], { .306f, .295f, .266f, 1.f } },
              { &patch.farRoofs, { .18f, .21f, .23f, 1.f } },
              { &patch.farPitchedRoofs.slate, { .108f, .123f, .130f, 1.f } },
              { &patch.farMansardZinc, { .216f, .239f, .250f, 1.f } },
              { &patch.farPitchedRoofs.clay, { .375f, .175f, .100f, 1.f } },
              { &patch.farTiles, { .435f, .20f, .108f, 1.f } },
              { &patch.farGlass[0], { .120f, .146f, .161f, 1.f } },
              { &patch.farGlass[1], { .222f, .227f, .217f, 1.f } } }
        };
    }

    template <class Patch>
    std::optional<Baked> bake( const Patch &patch, VehicleAppearanceQuality quality,
                               u32 requestedResolution )
    {
        const auto inputs = surfaces( patch );
        size_t sourceTriangles = 0;
        for( const auto &surface : inputs )
        {
            const auto indices = surface.first->indices.size();
            const auto count = indices / 3;
            if( indices % 3 || count > maximumSourceTriangles - sourceTriangles )
            {
                return {};
            }
            sourceTriangles += count;
        }
        if( sourceTriangles == 0 )
        {
            return {};
        }

        auto triangles = makeSourceBudgetSnapshot<MeshImposterTriangle>();
        triangles.reserve( sourceTriangles );
        const auto bound = std::numeric_limits<float>::max();
        Vector3F minimum( bound, bound, bound ), maximum( -bound, -bound, -bound );
        for( const auto &[geometry, colour] : inputs )
        {
            for( size_t i = 0; i < geometry->indices.size(); i += 3 )
            {
                MeshImposterTriangle triangle{ {}, colour };
                for( size_t corner = 0; corner < triangle.positions.size(); ++corner )
                {
                    const auto index = geometry->indices[i + corner];
                    if( index >= geometry->positions.size() )
                    {
                        return {};
                    }
                    const auto p = geometry->positions[index];
                    if( !MathUtil<f32>::isFinite( p ) )
                    {
                        return {};
                    }
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
        {
            return {};
        }
        // These are true referenced bounds, including pitched roofs and
        // the accepted kit geometry. The baker supplies transparent texel gutters.
        const auto centre = minimum + extent * .5f;
        const Vector3F margin( .025f, .025f, .025f );
        minimum -= margin;
        maximum += margin;
        for( auto &triangle : triangles )
        {
            for( auto &point : triangle.positions )
            {
                point -= centre;
            }
        }

        const u32 resolution = requestedResolution
                                   ? requestedResolution
                                   : ( quality == VehicleAppearanceQuality::Preview ? 64u : 128u );
        MeshImposterSpriteSheetOptions options;
        options.tileWidth = options.tileHeight = resolution;
        // A strip keeps every view's mip filtering and alpha coverage
        // independent through the existing native atlas upload path.
        options.columns = 49;
        options.supersampling = quality == VehicleAppearanceQuality::Preview ? 1u : 2u;
        auto atlas =
            bakeMeshImposterSpriteSheet( triangles, minimum - centre, maximum - centre, options );
        for( size_t view = 0; view < atlas.views.size(); ++view )
        {
            bool occupied = false;
            for( u32 y = 0; y < atlas.tileHeight && !occupied; ++y )
            {
                for( u32 x = 0; x < atlas.tileWidth; ++x )
                {
                    const auto pixel = ( size_t( y ) * atlas.width + view * atlas.tileWidth + x ) * 4;
                    if( !atlas.rgba[pixel + 3] )
                    {
                        continue;
                    }
                    occupied = true;
                    break;
                }
            }
            if( !occupied )
            {
                return {};
            }
        }
        return Baked{ std::move( atlas ), centre, minimum, maximum, sourceTriangles };
    }

    Geometry cards( const Baked &baked )
    {
        Geometry geometry;
        // One persistent unit quad. MeshImposter publishes the selected
        // baked projection through its transform and material UV rectangle.
        geometry.quad( { -.5f, -.5f, 0 }, { .5f, -.5f, 0 }, { .5f, .5f, 0 }, { -.5f, .5f, 0 },
                       Vector3F::unitZ() );
        geometry.uv[0] = { 0, 1, 0 };
        geometry.uv[1] = { 1, 1, 0 };
        geometry.uv[2] = { 1, 0, 0 };
        geometry.uv[3] = { 0, 0, 0 };
        for( auto &tangent : geometry.tangents )
        {
            tangent = { 1, 0, 0, -1 };
        }
        return geometry;
    }

    SmartPtr<scene::MeshRenderer> publish( SceneAssets &assets, const Baked &baked,
                                           SmartPtr<IGameActor> root )
    {
        const auto &atlas = baked.atlas;
        auto texture =
            uploadPixels( assets, atlas.rgba.data(), atlas.rgba.size(), atlas.width, atlas.height,
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
            actor->setStatic( false );
            actor->addComponent<scene::MeshImposter>()->configure( atlas.views, baked.centre, surface );
            auto renderer = actor->getComponent<scene::MeshRenderer>();
            renderer->setCastShadows( scene::Renderer::CastShadows::Off );
            WP_LOG( String( "City building imposter atlas " ) + StringUtil::toString( atlas.width ) +
                    "x" + StringUtil::toString( atlas.height ) +
                    " sourceTriangles=" + StringUtil::toString( baked.sourceTriangles ) +
                    " views=" + StringUtil::toString( atlas.views.size() ) );
            return renderer;
        }
        return nullptr;
    }

    template <class Patch>
    SmartPtr<scene::MeshRenderer> build( SceneAssets &assets, const Patch &patch,
                                         SmartPtr<IGameActor> root, VehicleAppearanceQuality quality )
    {
        const auto baked = bake( patch, quality, assets.cityBuildingImpostorResolution );
        return baked ? publish( assets, *baked, root ) : nullptr;
    }
}  // namespace cityBuildingImposters
