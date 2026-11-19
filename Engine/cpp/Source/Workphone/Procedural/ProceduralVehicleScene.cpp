#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    namespace sceneBuild
    {
        void buildCar( SceneAssets &assets, SmartPtr<scene::IGameActor> actor, u32 seed,
                       VehicleAppearanceQuality quality )
        {
            auto app = core::IApplicationManager::instance();
            auto generator =
                app->getFactoryManager()->createObjectFromType<IVehicleGenerator>( "IVehicleGenerator" );
            if( !generator )
            {
                throw std::runtime_error( "VehicleAdvanced requires WPProcedural IVehicleGenerator." );
            }
            VehicleGenerationConfig config;
            config.seed = seed;
            config.appearance.quality = quality;
            config.appearance.customMaxResolution = quality == VehicleAppearanceQuality::Preview ? 256
                                                    : quality == VehicleAppearanceQuality::Standard
                                                        ? 512
                                                        : 1024;
            assets.vehicle = generator->generate( config );
            if( !assets.vehicle.isValid() )
            {
                throw std::runtime_error( "Procedural vehicle generation failed." );
            }
            const auto &appearance = assets.vehicle.appearance;
            std::array<SmartPtr<render::IMaterial>, size_t( VehicleMaterialSlot::Count )> materials;
            for( size_t i = 0; i < materials.size(); ++i )
            {
                reserveOwnerSlot( assets.vehicleMaterials );
                const auto &d = appearance.materials[i];
                auto m = material( assets, { d.baseColor.r, d.baseColor.g, d.baseColor.b, d.opacity },
                                   d.roughness, d.metallic );
                m->setEmissive( { d.emissiveColor.r, d.emissiveColor.g, d.emissiveColor.b, 1 } );
                m->setEditorFloat( "emissionIntensity", d.emissiveIntensity );
                m->setEmissionEnabled( d.emissiveIntensity > 0 );
                m->setEditorFloat( "clearCoat", d.clearcoat );
                m->setEditorFloat( "clearCoatRoughness", d.clearcoatRoughness );
                m->setNormalStrength( d.normalScale );
                m->setSpecular( ColourF( .04f, .04f, .04f, 1 ) );
                m->setDiffuse( { d.baseColor.r, d.baseColor.g, d.baseColor.b, d.opacity } );
                m->setRoughness( d.roughness );
                m->setMetalness( d.metallic );
                m->setEmissive( { d.emissiveColor.r, d.emissiveColor.g, d.emissiveColor.b, 1 } );
                m->setEmissionEnabled( d.emissiveIntensity > 0 );
                if( d.doubleSided )
                {
                    m->setCullMode( 1 );
                }
                materials[i] = m;
                assets.vehicleMaterials.push_back( m );
            }
            const auto &t = appearance.textures;
            maps( assets, materials[size_t( VehicleMaterialSlot::BodyPaint )], t.bodyLivery,
                  t.bodyNormal, t.bodyORM );
            maps( assets, materials[size_t( VehicleMaterialSlot::CarbonGloss )], t.carbonAlbedo,
                  t.carbonNormal, t.carbonORM );
            // Share the carbon texture objects rather than uploading a second copy.
            auto carbon = materials[size_t( VehicleMaterialSlot::CarbonGloss )];
            auto matte = materials[size_t( VehicleMaterialSlot::CarbonMatte )];
            matte->setTextures( carbon->getTextures() );
            matte->setDiffuse( ColourF::White );
            matte->setRoughness( 1 );
            matte->setMetalness( 1 );
            maps( assets, materials[size_t( VehicleMaterialSlot::Rubber )], t.rubberAlbedo,
                  t.rubberNormal, t.rubberORM );
            TextureBuffer empty;
            maps( assets, materials[size_t( VehicleMaterialSlot::BareMetal )], empty,
                  t.brushedMetalNormal, t.brushedMetalORM );
            auto com = vector( assets.vehicle.physics.massProperties.centreOfMass );
            std::array<Vector3F, 4> hubs;
            for( size_t i = 0; i < 4; ++i )
            {
                hubs[i] = vector( assets.vehicle.physics.wheels[i].hubPosition );
                reserveOwnerSlot( assets.actors );
                auto wheel = app->getGameManager()->createActor();
                if( !wheel )
                {
                    throw std::runtime_error( "Cannot create procedural actor" );
                }
                assets.actors.push_back( wheel );
                wheel->setName( String( "Generated wheel " ) + StringUtil::toString( i ) );
                actor->addChild( wheel );
                wheel->addComponent<scene::WheelController>();
                assets.wheels[i] = wheel;
            }
            auto vehicleLOD = actor->addComponent<scene::LODGroup>();
            assets.vehicleLOD = vehicleLOD;
            vehicleLOD->setSize( 4.5f );
            vehicleLOD->setHysteresis( .15f );
            vehicleLOD->setCullBelowLastLOD( false );
            const size_t firstLOD = quality == VehicleAppearanceQuality::Preview ? 1 : 0;
            for( size_t lod = firstLOD; lod < assets.vehicle.geometry.lods.size(); ++lod )
            {
                auto renderers = makeSourceBudgetSnapshot<SmartPtr<scene::Renderer>>();
                for( const auto &section : assets.vehicle.geometry.lods[lod].sections )
                {
                    auto m = materials[size_t( generator->materialSlotFor( section.material ) )];
                    bool rotating = section.material == VehicleMaterial::Tyre ||
                                    section.material == VehicleMaterial::Rim ||
                                    section.material == VehicleMaterial::Brake;
                    // Wheel sections contain four disconnected assemblies. Assign whole triangles
                    // to their closest authored axle; preserve winding, UVs and normals unchanged.
                    std::array<Geometry, 4> parts;
                    for( size_t index = 0; index < section.indices.size(); index += 3 )
                    {
                        size_t corner = 0;
                        if( rotating )
                        {
                            auto centre = ( section.vertices[section.indices[index]].position +
                                            section.vertices[section.indices[index + 1]].position +
                                            section.vertices[section.indices[index + 2]].position ) /
                                          3.0f;
                            float closest = std::numeric_limits<float>::max();
                            for( size_t i = 0; i < 4; ++i )
                            {
                                auto distance = ( centre - hubs[i] ).length();
                                if( distance < closest )
                                {
                                    closest = distance;
                                    corner = i;
                                }
                            }
                        }
                        auto &g = parts[corner];
                        for( size_t j = 0; j < 3; ++j )
                        {
                            const auto &v = section.vertices[section.indices[index + j]];
                            g.indices.push_back( static_cast<u32>( g.positions.size() ) );
                            g.positions.push_back( v.position - ( rotating ? hubs[corner] : com ) );
                            g.normals.push_back( v.normal );
                            g.uv.push_back( { v.uv.x, v.uv.y, 0 } );
                            g.tangents.push_back(
                                { v.tangent.x, v.tangent.y, v.tangent.z, v.tangentSign } );
                        }
                    }
                    for( size_t i = 0; i < ( rotating ? 4u : 1u ); ++i )
                    {
                        auto part = mesh(
                            assets,
                            "LOD" + StringUtil::toString( lod ) + " " + String( section.name.c_str() ),
                            parts[i], m, rotating ? assets.wheels[i] : actor );
                        if( !assets.body && !rotating )
                        {
                            assets.body = part;
                        }
                        if( part )
                        {
                            renderers.push_back( part->getComponent<scene::MeshRenderer>() );
                        }
                    }
                }
                vehicleLOD->addLevel(
                    lod + 1 == assets.vehicle.geometry.lods.size() ? 0.f : ( lod == 0 ? .32f : .085f ),
                    renderers );
            }
            generator = nullptr;
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
