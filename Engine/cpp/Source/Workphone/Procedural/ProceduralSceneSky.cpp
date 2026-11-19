#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    namespace sceneBuild
    {
#include "Workphone/Scene/Components/CityDaylightSky.hpp"

        void buildSky( SceneAssets &assets, SmartPtr<scene::IGameActor> vehicle, u32 seed )
        {
            auto app = core::IApplicationManager::instance();
            auto sky =
                app->getFactoryManager()->createObjectFromType<ISkyAtmosphere>( "ISkyAtmosphere" );
            if( !sky )
            {
                throw std::runtime_error( "WPProcedural atmosphere is unavailable." );
            }
            SkyState state;
            state.timeOfDay = 15;
            state.turbidity = 2.4f;
            if( assets.openCity )
            {
                // Morning sun reaches the south and west street elevations before
                // the cooler sky fill lifts the opposite faces.
                state.timeOfDay = 9.65f;
                state.turbidity = 2.7f;
                state.latitude = 48.f;
                state.dayOfYear = 117.f;
            }
            sky->setState( state );
            sky->update();
            const auto result = sky->getResults();
            auto graphicsScene = app->getGraphicsSystem()->getGraphicsScene();

            auto faces = makeSourceBudgetSnapshot<SmartPtr<render::ITexture>>();
            auto displayFaces = makeSourceBudgetSnapshot<SmartPtr<render::ITexture>>();
            auto reflectionFaces = makeSourceBudgetSnapshot<SmartPtr<render::ITexture>>();
            const auto probePosition = vehicle->getPosition() + Vector3F( 0, 2, 0 );
            const Vector3F corners[8] = { { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, -1 },
                                          { -1, -1, 1 },  { 1, -1, 1 },  { 1, 1, 1 },  { -1, 1, 1 } };
            const int faceCorners[6][4] = { { 0, 1, 2, 3 }, { 5, 4, 7, 6 }, { 4, 0, 3, 7 },
                                            { 1, 5, 6, 2 }, { 3, 2, 6, 7 }, { 4, 5, 1, 0 } };
            constexpr u32 reflectionSize = 256;
            const u32 samplesPerSide = assets.openCity ? 2 : 1;
            const u32 visibleSize = reflectionSize * samplesPerSide;
            // Downsample the same angular field for the HDR probe in linear space.
            // Keep the probe and its prefilter at their existing allocation size.
            const float reflectionWeight = 1.f / float( samplesPerSide * samplesPerSide );
            for( int face = 0; face < 6; ++face )
            {
                TextureBuffer texture( visibleSize, visibleSize );
                auto visibleRadiance = makeSourceBudgetSnapshot<f32>();
                if( assets.openCity )
                {
                    visibleRadiance.resize( size_t( visibleSize ) * visibleSize * 4, 0.f );
                }
                auto reflectionFace = makeSourceBudgetSnapshot<f32>();
                reflectionFace.resize( reflectionSize * reflectionSize * 4, 0.f );
                for( u32 y = 0; y < texture.height; ++y )
                {
                    for( u32 x = 0; x < texture.width; ++x )
                    {
                        float u = ( x + .5f ) / texture.width, v = ( y + .5f ) / texture.height;
                        // Image rows run top to bottom; the sky's bottom corners use UV.y=1.
                        auto left = corners[faceCorners[face][0]] * v +
                                    corners[faceCorners[face][3]] * ( 1 - v );
                        auto right = corners[faceCorners[face][1]] * v +
                                     corners[faceCorners[face][2]] * ( 1 - v );
                        auto dir = left * ( 1 - u ) + right * u;
                        dir.normalise();
                        const auto sunAlignment =
                            std::clamp( dir.dotProduct( result.sunDir ), 0.f, 1.f );
                        ColourF c;
                        float sunTransmission = 1.f;
                        if( assets.openCity )
                        {
                            const auto city =
                                cityDaylight::sample( dir, result.sunDir, sunAlignment, seed );
                            c = city.radiance;
                            sunTransmission = city.sunTransmission;
                        }
                        else
                        {
                            c = sky->computeSkyColour( dir, result.sunDir, state.turbidity,
                                                       result.sunAltitude );
                            const auto haze = 1.f - std::max( dir.y, 0.f );
                            const auto glow = std::pow( sunAlignment, 128.f );
                            c.r = .06f + .35f * haze + c.r * .15f + glow * .6f;
                            c.g = .22f + .35f * haze + c.g * .15f + glow * .55f;
                            c.b = .58f + .24f * haze + c.b * .15f + glow * .4f;
                        }
                        // Keep the solar core as radiance in the reflection source.
                        // HDR views retain the same radiance as the reflection source.
                        // The byte face is retained for legacy display-space viewports.
                        const float solarCore = std::pow( sunAlignment, 2048.f ) * sunTransmission;
                        c.r += solarCore * 16.f;
                        c.g += solarCore * 14.f;
                        c.b += solarCore * 10.f;
                        auto reflectionColour = c;
                        if( assets.openCity && dir.y < 0 )
                        {
                            const auto ground = std::clamp( -dir.y * 5.5f, 0.f, 1.f );
                            reflectionColour.r = c.r * ( 1 - ground ) + .115f * ground;
                            reflectionColour.g = c.g * ( 1 - ground ) + .139f * ground;
                            reflectionColour.b = c.b * ( 1 - ground ) + .106f * ground;
                        }
                        else if( dir.y < 0 )
                        {
                            // Bake the start straight and surrounding grass into the lower
                            // hemisphere. This is a custom environment, not a realtime capture.
                            const auto hit = probePosition + dir * ( -probePosition.y / dir.y );
                            const bool road = std::abs( hit.x ) < 6 && hit.z > -90 && hit.z < 95;
                            const bool edge = std::abs( std::abs( hit.x ) - 5.7f ) < 0.12f && road;
                            reflectionColour.r = edge ? .7f : road ? .045f : .07f;
                            reflectionColour.g = edge ? .7f : road ? .05f : .13f;
                            reflectionColour.b = edge ? .7f : road ? .06f : .045f;
                            const float horizon = std::clamp( -dir.y * 20.f, 0.f, 1.f );
                            reflectionColour.r = c.r * ( 1 - horizon ) + reflectionColour.r * horizon;
                            reflectionColour.g = c.g * ( 1 - horizon ) + reflectionColour.g * horizon;
                            reflectionColour.b = c.b * ( 1 - horizon ) + reflectionColour.b * horizon;
                        }
                        auto pixel = texture.pixel( x, y );
                        auto reflectedPixel =
                            reflectionFace.data() +
                            ( size_t( y / samplesPerSide ) * reflectionSize + x / samplesPerSide ) * 4;
                        for( size_t channel = 0; channel < 3; ++channel )
                        {
                            float value = channel == 0 ? c.r : channel == 1 ? c.g : c.b;
                            if( assets.openCity )
                            {
                                visibleRadiance[( size_t( y ) * visibleSize + x ) * 4 + channel] =
                                    std::max( value, 0.f );
                            }
                            pixel[channel] =
                                assets.openCity
                                    ? cityDaylight::encodeDisplay( value )
                                    : u8( std::clamp(
                                          std::pow( std::max( value, 0.f ), 1.f / 2.2f ) * 255.f, 0.f,
                                          255.f ) );
                            const float reflectedValue = channel == 0   ? reflectionColour.r
                                                         : channel == 1 ? reflectionColour.g
                                                                        : reflectionColour.b;
                            reflectedPixel[channel] +=
                                std::max( reflectedValue, 0.f ) * reflectionWeight;
                        }
                        pixel[3] = 255;
                        if( assets.openCity )
                        {
                            visibleRadiance[( size_t( y ) * visibleSize + x ) * 4 + 3] = 1.f;
                        }
                        reflectedPixel[3] = 1;
                    }
                }
                auto skyTexture = upload( assets, texture );
                if( assets.openCity )
                {
                    displayFaces.push_back( skyTexture );
                    faces.push_back( uploadLinearRadiance(
                        assets, visibleRadiance, { int( visibleSize ), int( visibleSize ) } ) );
                }
                else
                {
                    faces.push_back( skyTexture );
                }
                reflectionFaces.push_back(
                    uploadLinearRadiance( assets, reflectionFace, { reflectionSize, reflectionSize } ) );
            }
            auto skyObject = graphicsScene->addGraphicsObjectByType<render::ISky>();
            assets.sky = skyObject;
            skyObject->setTextures( faces );
            if( assets.openCity )
            {
                // The existing material cube slot supplies display faces for direct
                // legacy viewports. HDR draws and environment sampling use owned faces.
                auto displaySky = material( assets, ColourF::White, 1.f );
                displaySky->setCubicTextures( displayFaces );
                displaySky->setName( assets.resourcePrefix + "/sky-display-fallback" );
                skyObject->setMaterial( displaySky );
            }
            skyObject->setDistance( 500 );
            skyObject->setVisible( true );

            reserveOwnerSlot( assets.textures );
            auto reflection =
                app->getGraphicsSystem()->getTextureManager()->createCubeMap( reflectionFaces );
            if( reflection )
            {
                assets.reflectionTexture = reflection;
                assets.textures.push_back( reflection );
                reflection->setName( assets.resourcePrefix + "/reflection" );
                // Eight RGBA32F levels at 128px per face, generated by ClawCubemap.
                assets.textureBytes += 6u * 21845u * 16u;
                auto probeResource = make_ptr<render::GraphicsCubemap>();
                probeResource->setTexture( reflection );
                probeResource->setTextureName( reflection->getName() );
                probeResource->setSceneManager( graphicsScene );
                reserveOwnerSlot( assets.actors );
                auto actor = app->getGameManager()->createActor();
                if( !actor )
                {
                    throw std::runtime_error( "Cannot create procedural actor" );
                }
                assets.actors.push_back( actor );
                actor->setName( "Vehicle Reflection Cubemap" );
                actor->setPosition( probePosition );
                auto component = actor->addComponent<scene::Cubemap>();
                component->setProbeName( "Vehicle Outdoor Environment" );
                component->setSourceType( scene::Cubemap::SourceType::Custom );
                component->setCubemapPath( reflection->getName() );
                component->setResolution( 128 );
                component->setProjectionMode( scene::Cubemap::ProjectionMode::Infinite );
                component->setAutoEnableByDistance( false );
                component->setRenderCubemap( probeResource );
                component->setEnabled( true );
                const auto reflectionSlot = static_cast<u32>( PbsTextureTypes::PBSM_REFLECTION );
                for( auto &vehicleMaterial : assets.vehicleMaterials )
                {
                    vehicleMaterial->setTexture( reflection, reflectionSlot );
                }
                if( assets.openCity )
                {
                    // The outdoor HDR sky also lights architectural specular response.
                    // Recessed opaque glazing still has no transmission/interior model.
                    for( auto &cityMaterial : assets.materials )
                    {
                        cityMaterial->setTexture( reflection, reflectionSlot );
                    }
                }
                // The actor's material also lets Cubemap manage the representative body material.
                actor->addComponent<scene::Material>()->setMaterial( assets.vehicleMaterials.front() );
                assets.reflectionActor = actor;
                WP_LOG(
                    "VehicleAdvanced: custom cubemap actor created; reflection applied to all vehicle "
                    "materials." );
            }

            graphicsScene->setAmbientLight( assets.openCity ? ColourF( 1.16f, 1.29f, 1.43f, 1 )
                                                            : ColourF( .28f, .32f, .38f, 1 ) );
            if( assets.openCity )
            {
                graphicsScene->setFog(
                    render::IGraphicsScene::FOG_EXP2,
                    ColourF( citySkyHorizon.x, citySkyHorizon.y, citySkyHorizon.z, 1 ), .00145f, 0,
                    950 );
            }
            else
            {
                const auto horizon = sky->computeSkyColour( Vector3F( 1, 0, 0 ), result.sunDir,
                                                            state.turbidity, result.sunAltitude );
                const auto fogRadiance = []( float displayedLinear ) {
                    const float colour = std::clamp( displayedLinear, 0.f, .95f );
                    return colour / ( 1 - colour );
                };
                graphicsScene->setFog( render::IGraphicsScene::FOG_LINEAR,
                                       ColourF( fogRadiance( .41f + horizon.r * .15f ),
                                                fogRadiance( .57f + horizon.g * .15f ),
                                                fogRadiance( .82f + horizon.b * .15f ), 1 ),
                                       .0007f, 180, 800 );
            }
            // Own the sky's matching sun. The showcase explicitly selects this light;
            // existing host lighting remains available to the scene's fallback policy.
            auto sun = graphicsScene->addGraphicsObjectByType<render::IGraphicsLight>();
            if( !sun )
            {
                throw std::runtime_error( "Cannot create procedural sunlight." );
            }
            assets.sun = sun;
            sun->setType( LightTypes::LT_DIRECTIONAL );
            sun->setDirection( -result.sunDir );
            sun->setDiffuseColour( assets.openCity ? ColourF( 1, .955f, .885f, 1 )
                                                   : ColourF( 1, .96f, .88f, 1 ) );
            sun->setPowerScale( assets.openCity ? 6.6f : 1.8f );
            sun->setCastShadows( true );
            sun->setVisible( true );
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
