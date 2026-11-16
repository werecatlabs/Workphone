#pragma once

        namespace cityFoliageLOD
        {
            // The generator's leaf image is still available here, so baking does
            // not require GPU readback, catalog files or an optional backend resolver.
            struct LeafSource
            {
                StreamBudget budget;
                std::shared_ptr<render::TextureMipLevel> pixels;
                String name;

                LeafSource( const TextureBuffer &source, const String &identity ) : name( identity )
                {
                    String error;
                    budget = WorldStreamingScheduler::shared()->reserve(
                        StreamMemoryClass::WorkingCpu, source.pixels.size() + 4096, error );
                    if( !budget ) throw std::runtime_error( error.c_str() );
                    pixels = std::make_shared<render::TextureMipLevel>();
                    pixels->width = source.width;
                    pixels->height = source.height;
                    pixels->bgra.assign( source.pixels.begin(), source.pixels.end() );
                    for( size_t i = 0; i < pixels->bgra.size(); i += 4 )
                        std::swap( pixels->bgra[i], pixels->bgra[i + 2] );
                }
            };

            void configure( SceneAssets &assets, render::FoliageSpecies &species,
                            const LeafSource &leaf, VehicleAppearanceQuality quality )
            {
                if( species.kind != render::FoliageKind::Tree && species.kind != render::FoliageKind::Bush )
                    return;
                render::FoliageImpostorBakeOptions options;
                options.tileSize = assets.cityFoliageImpostorResolution ?
                    assets.cityFoliageImpostorResolution :
                    ( quality == VehicleAppearanceQuality::Preview ? 64u : 128u );
                options.gutter = 4;
                options.resourcePrefix = species.id + "/lod2";
                options.directions.clear();
                constexpr float pi = 3.14159265358979323846f;
                for( u32 elevation = 0; elevation < 3; ++elevation )
                    for( u32 azimuth = 0; azimuth < 8; ++azimuth )
                    {
                        const float pitch = float( elevation ) * pi / 6;
                        const float yaw = float( azimuth ) * pi / 4;
                        options.directions.push_back( { std::sin( yaw ) * std::cos( pitch ),
                            std::sin( pitch ), std::cos( yaw ) * std::cos( pitch ) } );
                    }
                options.directions.push_back( { 0, 1, 0 } );

                // Bound the existing baker's ordinary-vector working storage:
                // four base maps, four mip chains, roughness resolve, tile depth
                // and dilation scratch. Retained texture copies are admitted by
                // uploadPixels/ClawTexture independently in SourceCpu and Gpu.
                const u64 baseBytes = u64( options.tileSize ) * options.tileSize *
                                      options.directions.size() * 4;
                String error;
                auto working = WorldStreamingScheduler::shared()->reserve(
                    StreamMemoryClass::WorkingCpu, baseBytes * 16 + ( 1ull << 20 ), error );
                if( !working ) throw std::runtime_error( error.c_str() );
                render::FoliageImpostorBake baked;
                const render::FoliageBakeTextureResolver resolver =
                    [&]( const String &id, String &failure ) -> std::shared_ptr<const render::TextureMipLevel> {
                        if( id == leaf.name ) return leaf.pixels;
                        failure = "Generated foliage LOD cannot resolve texture: " + id;
                        return {};
                    };
                if( !render::bakeFoliageImpostor( species, resolver, options, baked, error ) )
                    throw std::runtime_error( ( "Cannot bake generated foliage LOD: " + error ).c_str() );

                const auto previousCount = assets.textures.size();
                const auto previousBytes = assets.textureBytes;
                auto manager = core::IApplicationManager::instance()->getGraphicsSystem()->getTextureManager();
                const auto publish = [&]( const render::TextureMipLevel &mip, render::TextureMipFilter filter ) {
                    const auto name = assets.resourcePrefix + "/texture/" + StringUtil::toString( assets.textures.size() );
                    auto texture = uploadPixels( assets, mip.bgra.data(), mip.bgra.size(), mip.width,
                        mip.height, filter, u32( options.directions.size() ), nullptr, true );
                    texture->setName( name );
                    if( manager->getByName( name ).get() != texture.get() )
                        throw std::runtime_error( "Generated foliage LOD lost its registered texture identity." );
                    return name;
                };
                try
                {
                    auto metadata = baked.metadata;
                    metadata.albedo = publish( baked.albedo.front(), render::TextureMipFilter::Cutout );
                    metadata.normal = publish( baked.normal.front(), render::TextureMipFilter::Normal );
                    // Generated bark/leaves use constant roughness/AO/metalness,
                    // so Data mips retain the baker's material factors exactly.
                    metadata.orm = publish( baked.orm.front(), render::TextureMipFilter::Data );
                    // The current instanced renderer uses a conservative plane;
                    // do not retain an unused depth texture or claim depth reconstruction.
                    metadata.depth.clear();
                    species.impostor = std::move( metadata );
                    species.farBillboard = true;
                    // Keep the actual 3-D crown until it occupies under ~2% of
                    // viewport height. Both near and middle geometry stay available.
                    species.screenHeightThresholds = { .12f, .02f, 0.f };
                    if( assets.openCity )
                    {
                        const size_t offset = species.kind == render::FoliageKind::Tree ? 0 : 2;
                        species.lodDistances = { assets.cityFoliageLODDistances[offset],
                                                 assets.cityFoliageLODDistances[offset + 1] };
                    }
                }
                catch( ... )
                {
                    while( assets.textures.size() > previousCount )
                    {
                        auto texture = assets.textures.back();
                        assets.textures.pop_back();
                        try { manager->destroyResource( texture ); } catch( ... ) {}
                    }
                    assets.textureBytes = previousBytes;
                    throw;
                }
                WP_LOG( "Generated foliage LOD species=" + species.id + " views=25 atlas=" +
                    StringUtil::toString( baked.metadata.width ) + "x" +
                    StringUtil::toString( baked.metadata.height ) + " rasterSamples=" +
                    StringUtil::toString( baked.rasterSamples ) );
            }
        }
