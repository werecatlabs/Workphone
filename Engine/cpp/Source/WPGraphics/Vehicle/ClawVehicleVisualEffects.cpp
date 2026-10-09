#include <WPGraphics/WPClawHammerPCH.hpp>
#include <Workphone/Vehicle/SkidDecalPool.h>

#include <WPGraphics/Vehicle/ClawVehicleVisualEffects.hpp>
#include <Workphone/Workphone.hpp>
#include <cmath>
#include <random>
#if WP_GRAPHICS_SYSTEM_CLAW
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/Particle/CParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#endif

namespace workphone::advanced
{
    WP_CLASS_REGISTER_DERIVED( workphone::advanced, ClawVehicleVisualEffects,
                               IVehicleVisualEffects );
    struct ClawVehicleVisualEffects::Impl
    {
        SkidDecalPool marks;
        std::array<float, 4> smokeClock{}, dustClock{};
        std::mt19937 random;
        Vector3F previousVelocity;
        size_t emitted = 0;
        float uploadClock = 0;
        bool uploaded = true, hadVelocity = false;
#if WP_GRAPHICS_SYSTEM_CLAW
        SmartPtr<render::IGraphicsScene> scene;
        SmartPtr<render::IGraphicsSceneNode> root;
        std::array<SmartPtr<render::CParticleSystem>, 3> systems;
        std::array<SmartPtr<render::IMaterial>, 4> materials;
        std::array<SmartPtr<render::ITexture>, 2> textures;
        SmartPtr<render::ClawMesh> decalMesh;
        Array<wp_graphics_mesh_vertex_pntc> vertices;
        Array<u32> indices;
#endif
        Impl( size_t capacity, unsigned seed ) : marks( capacity ), random( seed ) {}
    };

    ClawVehicleVisualEffects::ClawVehicleVisualEffects() = default;
    ClawVehicleVisualEffects::~ClawVehicleVisualEffects() { unload( nullptr ); }

    bool ClawVehicleVisualEffects::configure( unsigned quality, unsigned seed )
    {
        unload( nullptr );
#if WP_GRAPHICS_SYSTEM_CLAW
        m_impl = std::make_unique<Impl>( quality == 0 ? 256 : quality == 1 ? 1024 : 2048, seed );
        auto& fx = *m_impl;
        auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
        fx.scene = graphics->getGraphicsScene();
        fx.root = fx.scene->getRootSceneNode()->addChildSceneNode();
        // Identity world anchor: moving the car never moves old smoke or decals.
        for ( size_t i = 0; i < fx.textures.size(); ++i )
        {
            const auto name = "VehicleFX/texture/" + StringUtil::generateUUID().to_string();
            auto texture =
                graphics->getTextureManager()->createManual( name, "General", 2, 64, 64, 1, 0, 0 );
            if ( !texture )
            {
                unload( nullptr );
                return false;
            }
            fx.textures[i] = texture;
            std::array<unsigned char, 64 * 64 * 4> pixels{};
            for ( int y = 0; y < 64; ++y )
                for ( int x = 0; x < 64; ++x )
                {
                    const auto u = ( x + .5f ) / 64.f, v = ( y + .5f ) / 64.f;
                    float alpha;
                    if ( i == 0 )
                    {
                        const auto radius = ( u - .5f ) * ( u - .5f ) + ( v - .5f ) * ( v - .5f );
                        alpha = std::pow( std::max( 0.f, 1.f - radius * 4.f ), 3.f );
                    }
                    else
                    {
                        const auto edge = std::clamp( std::min( u, 1 - u ) * 12.f, 0.f, 1.f );
                        const auto groove = .65f + .35f * std::pow( std::sin( u * 31.4159f ), 2.f );
                        alpha = edge * groove;
                    }
                    const auto offset = ( y * 64 + x ) * 4;
                    pixels[offset] = pixels[offset + 1] = pixels[offset + 2] = 255;
                    pixels[offset + 3] = static_cast<unsigned char>( alpha * 255 );
                }
            texture->copyData( pixels.data(), Vector2I( 64, 64 ) );
        }
        for ( size_t i = 0; i < fx.materials.size(); ++i )
        {
            auto material = ApplicationUtil::createDefaultMaterial();
            material->load( nullptr );
            material->setLightingEnabled( false );
            if ( i == 3 ) material->setMaterialType( MaterialType::UI );
            material->setDiffuse( ColourF::White );
            material->setCullMode( 1 );
            material->setBlendMode( i == 2 ? 3 : 1 );
            material->setDepthWrite( false );
            material->setDepthTest( 2 );
            material->setTexture( fx.textures[i == 3 ? 1 : 0], 0 );
            fx.materials[i] = material;
        }
        for ( size_t i = 0; i < fx.systems.size(); ++i )
        {
            auto system = dynamic_pointer_cast<render::CParticleSystem>(
                fx.scene->addGraphicsObjectByType<render::IParticleSystem>() );
            if ( !system )
            {
                unload( nullptr );
                return false;
            }
            fx.systems[i] = system;
            // Creation is queued by the scene; configure capacity before loading.
            system->unload( nullptr );
            system->setPoolSize( quality == 0 ? 64 : quality == 1 ? 192 : 384 );
            system->setSeed( seed + unsigned( i ) );
            wp_particle_simulation_settings settings;
            wp_particle_simulation_default_settings( &settings );
            settings.rate = 0;
            settings.gravity.y = i == 2 ? -7.f : .15f;
            const float colors[3][3] = {
                { .55f, .57f, .6f }, { .48f, .34f, .20f }, { 1.f, .45f, .06f } };
            for ( int c = 0; c < 3; ++c )
                settings.color_start[c] = settings.color_end[c] = colors[i][c];
            settings.color_start[3] = i == 2 ? 1.f : .32f;
            settings.color_end[3] = 0;
            system->setSimulationSettings( settings );
            system->setMaterialName( fx.materials[i]->getName() );
            system->load( nullptr );
            system->setState( render::ParticleSystemState::Started );
            system->setCastShadows( false );
            fx.root->attachObject( system );
            if ( !system->isLoaded() )
            {
                unload( nullptr );
                return false;
            }
        }
        fx.decalMesh = dynamic_pointer_cast<render::ClawMesh>(
            fx.scene->addGraphicsObjectByType<render::IGraphicsMesh>() );
        if ( !fx.decalMesh )
        {
            unload( nullptr );
            return false;
        }
        fx.decalMesh->load( nullptr );
        fx.decalMesh->setMaterial( fx.materials[3] );
        fx.decalMesh->setCastShadows( false );
        fx.decalMesh->setVisibilityFlags( render::IGraphicsObject::SceneFlag );
        fx.decalMesh->setVisible( false );
        fx.root->attachObject( fx.decalMesh );
        fx.vertices.reserve( 2048 * 4 );
        fx.indices.reserve( 2048 * 6 );
        WP_LOG( "Vehicle FX: smoke, dust, impact sparks and skid decals ready." );
        setLoadingState( LoadingState::Loaded );
        return true;
#else
        return false;
#endif
    }

    void ClawVehicleVisualEffects::unload( SmartPtr<ISharedObject> data )
    {
        if ( !m_impl ) return;
#if WP_GRAPHICS_SYSTEM_CLAW
        auto& fx = *m_impl;
        for ( auto& system : fx.systems )
            if ( system )
            {
                if ( fx.root ) fx.root->detachObject( system );
                fx.scene->removeGraphicsObject( system );
                system->unload( nullptr );
            }
        if ( fx.decalMesh )
        {
            if ( fx.root ) fx.root->detachObject( fx.decalMesh );
            fx.scene->removeGraphicsObject( fx.decalMesh );
            fx.decalMesh->unload( nullptr );
        }
        if ( fx.root ) fx.scene->removeSceneNode( fx.root );
        if ( auto app = core::IApplicationManager::instance() )
            if ( auto graphics = app->getGraphicsSystem() )
            {
                for ( auto& material : fx.materials )
                    if ( material )
                    {
                        material->unload( nullptr );
                        graphics->getMaterialManager()->destroyResource( material );
                    }
                for ( auto& texture : fx.textures )
                    if ( texture )
                    {
                        texture->unload( nullptr );
                        graphics->getTextureManager()->destroyResource( texture );
                    }
            }
#endif
        m_impl.reset();
        setLoadingState( LoadingState::Unloaded );
    }

    void ClawVehicleVisualEffects::reset()
    {
        if ( !m_impl ) return;
        auto& fx = *m_impl;
        fx.marks.clear();
        fx.smokeClock.fill( 0 );
        fx.dustClock.fill( 0 );
        fx.hadVelocity = false;
#if WP_GRAPHICS_SYSTEM_CLAW
        for ( auto& system : fx.systems )
            if ( system )
            {
                system->setState( render::ParticleSystemState::Stopped );
                system->setState( render::ParticleSystemState::Started );
            }
        if ( fx.decalMesh ) fx.decalMesh->setVisible( false );
#endif
    }

    void ClawVehicleVisualEffects::update( const VehicleEffectsFrame& frame, float dt )
    {
        if ( !m_impl || !std::isfinite( dt ) || dt <= 0 ) return;
        auto& fx = *m_impl;
#if WP_GRAPHICS_SYSTEM_CLAW
        for ( auto& system : fx.systems )
            system->setState( frame.playing ? render::ParticleSystemState::Started
                                            : render::ParticleSystemState::Paused );
        if ( !frame.playing )
        {
            for ( size_t i = 0; i < 4; ++i ) fx.marks.breakTrail( i );
            fx.hadVelocity = false;
            return;
        }
        fx.marks.advance( dt );
        dt = std::min( dt, .05f );
        std::uniform_real_distribution<float> jitter( -.5f, .5f );
        auto emit = [&]( size_t type, Vector3F p, Vector3F velocity, float size, float lifetime )
        {
            velocity += Vector3F( jitter( fx.random ), std::abs( jitter( fx.random ) ),
                                  jitter( fx.random ) );
            if ( fx.systems[type]->emitParticle( p, velocity, size, lifetime ) ) ++fx.emitted;
        };
        const auto speed = frame.velocity.length();
        for ( size_t i = 0; i < 4; ++i )
        {
            const auto slip = std::isfinite( frame.slip[i] )
                                  ? std::clamp( ( frame.slip[i] - 1.5f ) / 6.f, 0.f, 1.f )
                                  : 0.f;
            const auto smoke = frame.grounded[i] && frame.onRoad[i] ? slip : 0.f;
            const auto dust = frame.grounded[i] && !frame.onRoad[i]
                                  ? std::clamp( ( speed - 4.f ) / 20.f, 0.f, 1.f )
                                  : 0.f;
            fx.smokeClock[i] += smoke * 18.f * dt;
            fx.dustClock[i] += dust * 14.f * dt;
            while ( fx.smokeClock[i] >= 1 )
            {
                fx.smokeClock[i] -= 1;
                emit( 0, frame.contact[i] + Vector3F( 0, .08f, 0 ),
                      frame.velocity * .1f + Vector3F( 0, .8f, 0 ), .65f, 1.1f );
            }
            while ( fx.dustClock[i] >= 1 )
            {
                fx.dustClock[i] -= 1;
                emit( 1, frame.contact[i] + Vector3F( 0, .06f, 0 ),
                      frame.velocity * .08f + Vector3F( 0, .55f, 0 ), .45f, .7f );
            }
            if ( frame.grounded[i] && frame.onRoad[i] && slip > .05f && speed > 4 )
            {
                const auto p = frame.contact[i], n = frame.normal[i];
                fx.marks.sample( i, { p.x, p.y, p.z }, { n.x, n.y, n.z }, frame.width[i],
                                 slip * .72f );
            }
            else
                fx.marks.breakTrail( i );
            if ( smoke == 0 ) fx.smokeClock[i] = 0;
            if ( dust == 0 ) fx.dustClock[i] = 0;
        }
        const auto previousSpeed = fx.previousVelocity.length();
        if ( fx.hadVelocity && previousSpeed > 8.f )
        {
            const auto direction = fx.previousVelocity / previousSpeed;
            if ( previousSpeed - frame.velocity.dotProduct( direction ) > 6.f )
                for ( int i = 0; i < 12; ++i )
                    emit( 2, frame.position + direction * 1.7f,
                          Vector3F( 0, 1.5f, 0 ) - direction * 2.f, .035f, .3f );
        }
        fx.previousVelocity = frame.velocity;
        fx.hadVelocity = true;
        fx.uploadClock += dt;
        if ( fx.uploadClock < 1.f / 30.f ) return;
        fx.uploadClock = 0;
        fx.vertices.clear();
        fx.indices.clear();
        for ( const auto& mark : fx.marks.marks() )
        {
            Vector3F a( mark.start.x, mark.start.y, mark.start.z ),
                b( mark.end.x, mark.end.y, mark.end.z );
            Vector3F normal( mark.normal.x, mark.normal.y, mark.normal.z );
            normal.normalise();
            auto side = ( b - a ).crossProduct( normal ).normaliseCopy() * mark.width * .5f;
            // The procedural road is drawn above the collision plane.
            a += normal * .018f;
            b += normal * .018f;
            const Vector3F corners[] = { a - side, a + side, b + side, b - side };
            const float uv[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
            const auto base = static_cast<u32>( fx.vertices.size() );
            const auto alpha = static_cast<u32>( SkidDecalPool::opacity( mark ) * 255 );
            const u32 color = 0x10101200u | alpha;
            for ( int i = 0; i < 4; ++i )
                fx.vertices.push_back( { { corners[i].x, corners[i].y, corners[i].z },
                                         { normal.x, normal.y, normal.z },
                                         { uv[i][0], uv[i][1] },
                                         color } );
            for ( auto index : { 0u, 1u, 2u, 0u, 2u, 3u } ) fx.indices.push_back( base + index );
        }
        if ( !fx.vertices.empty() )
        {
            const bool uploaded = fx.decalMesh->updateGeometry( fx.vertices, fx.indices );
            fx.uploaded = uploaded && fx.uploaded;
            fx.decalMesh->setVisible( uploaded );
        }
        else
            fx.decalMesh->setVisible( false );
#endif
    }
    size_t ClawVehicleVisualEffects::particles() const
    {
        size_t count = 0;
#if WP_GRAPHICS_SYSTEM_CLAW
        if ( m_impl )
            for ( const auto& system : m_impl->systems )
                if ( system ) count += system->getNumParticles();
#endif
        return count;
    }
    size_t ClawVehicleVisualEffects::decals() const
    {
        return m_impl ? m_impl->marks.marks().size() : 0;
    }
    size_t ClawVehicleVisualEffects::emitted() const { return m_impl ? m_impl->emitted : 0; }
    bool ClawVehicleVisualEffects::uploaded() const { return m_impl && m_impl->uploaded; }
}  // namespace workphone::advanced
