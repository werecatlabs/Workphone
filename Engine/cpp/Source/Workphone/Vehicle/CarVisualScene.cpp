#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Vehicle/CarVisualScene.hpp>
#include <Workphone/Vehicle/CarVisualAssets.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshResource.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Scene/Components/Light.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>
#include <Workphone/Interface/Mesh/IVertexElement.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <unordered_map>
#include <stdexcept>

namespace workphone::vehiclevisual
{
    namespace
    {
        Vector3F vector( Vec3 v )
        {
            return { v.x, v.y, v.z };
        }

        ColourF colour( Colour c )
        {
            return { c.r, c.g, c.b, c.a };
        }

        std::vector<Mesh> readModel( SmartPtr<IMesh> source )
        {
            if( !source )
            {
                throw std::runtime_error( "CarVisual could not load the model" );
            }
            std::vector<Mesh> result;
            auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
            auto materials = graphics ? graphics->getMaterialManager() : nullptr;
            for( auto sub : source->getSubMeshes() )
            {
                if( !sub )
                {
                    continue;
                }
                auto buffer = sub->getUseSharedVertices() ? source->getSharedVertexBuffer()
                                                          : sub->getVertexBuffer();
                if( !buffer || !buffer->getVertexDeclaration() )
                {
                    continue;
                }
                auto declaration = buffer->getVertexDeclaration();
                auto position =
                    declaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
                auto normal = declaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
                auto tangent = declaration->findElementBySemantic( VertexElementSemantic::VES_TANGENT );
                auto uv = declaration->findElementBySemantic(
                    VertexElementSemantic::VES_TEXTURE_COORDINATES, 0 );
                if( !position )
                {
                    throw std::runtime_error( "CarVisual model has no positions" );
                }
                Mesh mesh;
                const auto materialName = sub->getMaterialName();
                mesh.name = materialName.c_str();
                mesh.sourceMaterial = mesh.name;
                if( materials )
                {
                    if( auto m = dynamic_pointer_cast<render::IMaterial>(
                            materials->getByName( materialName ) ) )
                    {
                        auto c = m->getDiffuse();
                        mesh.colour = { c.r, c.g, c.b, c.a };
                        mesh.opacity = m->getOpacity();
                        mesh.transparent = m->isTransparent();
                        mesh.roughness = m->getRoughness();
                        mesh.metalness = m->getMetalness();
                    }
                }
                auto indices = MeshUtil::getIndices( sub );
                mesh.indices.assign( indices.begin(), indices.end() );
                auto element = [&]( SmartPtr<IVertexElement> e, u32 vertex,
                                    unsigned required ) -> const float * {
                    if( !e )
                    {
                        return nullptr;
                    }
                    if( e->getType() != VertexElementType::VET_FLOAT2 &&
                        e->getType() != VertexElementType::VET_FLOAT3 &&
                        e->getType() != VertexElementType::VET_FLOAT4 )
                    {
                        throw std::runtime_error(
                            "CarVisual requires float vertex attributes; convert the model through the "
                            "mesh importer" );
                    }
                    if( e->getSize() < required * sizeof( float ) )
                    {
                        throw std::runtime_error( "CarVisual vertex attribute is too small" );
                    }
                    auto data = static_cast<u8 *>( buffer->getVertexData( e->getSource() ) );
                    if( !data )
                    {
                        throw std::runtime_error(
                            "CarVisual requires CPU-readable imported vertex buffers" );
                    }
                    float *value = nullptr;
                    e->getElementData( data + vertex * declaration->getSize( e->getSource() ), &value );
                    return value;
                };
                mesh.vertices.reserve( buffer->getNumVertices() );
                for( u32 i = 0; i < buffer->getNumVertices(); ++i )
                {
                    Vertex v;
                    auto p = element( position, i, 3 );
                    v.position = { p[0], p[1], p[2] };
                    if( auto n = element( normal, i, 3 ) )
                    {
                        v.normal = { n[0], n[1], n[2] };
                    }
                    if( auto t = element( tangent, i, 3 ) )
                    {
                        v.tangent = { t[0], t[1], t[2] };
                        if( tangent->getSize() >= 16 )
                        {
                            v.tangentSign = t[3];
                        }
                    }
                    if( auto t = element( uv, i, 2 ) )
                    {
                        v.uv0 = { t[0], t[1] };
                    }
                    mesh.vertices.push_back( v );
                }
                if( !mesh.vertices.empty() && !mesh.indices.empty() )
                {
                    result.push_back( std::move( mesh ) );
                }
            }
            if( result.empty() )
            {
                throw std::runtime_error( "CarVisual model contains no drawable triangle meshes" );
            }
            return result;
        }
    }  // namespace

    struct CarVisualScene::Impl
    {
        CarVisual visual;
        std::vector<SmartPtr<scene::IGameActor>> actors;
        std::vector<SmartPtr<scene::MeshRenderer>> renderers;
        std::vector<SmartPtr<render::IMaterial>> materials;
        std::vector<SmartPtr<IMeshResource>> meshes;
        std::array<SmartPtr<render::ITexture>, 5> textures;
        std::array<SmartPtr<scene::Light>, 2> lights;
        std::array<SmartPtr<scene::IGameActor>, 2> lightActors;
        scene::IComponent::State state = scene::IComponent::State::Edit;

        ~Impl()
        {
            auto app = core::IApplicationManager::instancePtr();
            auto game = app ? app->getGameManager() : nullptr;
            if( game )
            {
                for( auto it = actors.rbegin(); it != actors.rend(); ++it )
                {
                    if( *it )
                    {
                        game->destroyActor( *it );
                    }
                }
            }
            auto meshManager =
                app ? dynamic_pointer_cast<MeshManager>( app->getMeshManager() ) : nullptr;
            if( meshManager )
            {
                for( auto mesh : meshes )
                {
                    meshManager->removeMeshResource( mesh );
                }
            }
            auto graphics = app ? app->getGraphicsSystem() : nullptr;
            if( graphics && graphics->getMaterialManager() )
            {
                for( auto m : materials )
                {
                    if( m )
                    {
                        graphics->getMaterialManager()->destroyResource( m );
                    }
                }
            }
            if( graphics && graphics->getTextureManager() )
            {
                for( auto t : textures )
                {
                    if( t )
                    {
                        graphics->getTextureManager()->destroyResource( t );
                    }
                }
            }
        }

        SmartPtr<render::ITexture> texture( Texture kind )
        {
            if( kind == Texture::None )
            {
                return nullptr;
            }
            auto &entry = textures[static_cast<size_t>( kind )];
            if( entry )
            {
                return entry;
            }
            auto image = CarVisual::texture( kind );
            auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
            entry = graphics->getTextureManager()->createManual( "CarVisual/" + StringUtil::getUUID(),
                                                                 "General", 2, image->width,
                                                                 image->height, 1, 0, 0 );
            if( !entry )
            {
                throw std::runtime_error( "Cannot create CarVisual effect texture" );
            }
            auto bgra = image->rgba;
            for( std::size_t i = 0; i < bgra.size(); i += 4 )
            {
                std::swap( bgra[i], bgra[i + 2] );
            }
            entry->copyData( bgra.data(), Vector2I( image->width, image->height ) );
            return entry;
        }

        void apply()
        {
            const auto &nodes = visual.nodes();
            for( std::size_t i = 0; i < nodes.size(); ++i )
            {
                const auto &n = nodes[i];
                auto actor = actors[i];
                // Keep world translation/yaw with the owning physics actor.
                if( i == 0 )
                {
                    actor->setLocalPosition( { 0, n.position.y, 0 } );
                }
                if( i != 0 )
                {
                    actor->setLocalPosition( vector( n.position ) );
                    QuaternionF q;
                    q.fromRadians( vector( n.euler ) );
                    actor->setLocalOrientation( q );
                }
                actor->setLocalScale( vector( n.scale ) );
                actor->setVisible( nodes[0].visible && n.visible && ( !n.mesh || n.opacity > .001f ),
                                   false );
                if( auto m = materials[i] )
                {
                    auto c = colour( n.colour );
                    c.a = n.opacity;
                    m->setDiffuse( c );
                    m->setOpacity( n.opacity );
                    if( n.additive )
                    {
                        m->setEmissive( c );
                        m->setEmissionEnabled( true );
                    }
                }
                if( renderers[i] )
                {
                    if( auto object = renderers[i]->getGraphicsObjectByType<render::IGraphicsMesh>() )
                    {
                        object->setCastShadows( n.castShadow );
                    }
                }
            }
            for( unsigned i = 0; i < lights.size(); ++i )
            {
                if( lights[i] )
                {
                    const auto &spot = visual.spots()[i];
                    lights[i]->setIntensity( nodes[0].visible ? spot.intensity : 0 );
                    if( auto light = lights[i]->getLight() )
                    {
                        light->setSpotlightRange( spot.angle * ( 1 - spot.penumbra ), spot.angle,
                                                  1.25f );
                        light->setAttenuation( spot.range, 1, 0, .002f );
                        light->setCastShadows( spot.shadows );
                    }
                }
            }
        }
    };

    CarVisualScene::CarVisualScene() = default;
    CarVisualScene::~CarVisualScene() = default;

    bool CarVisualScene::ready() const
    {
        return bool( m_impl );
    }

    void CarVisualScene::unload()
    {
        m_impl.reset();
    }

    void CarVisualScene::build( SmartPtr<scene::IGameActor> parent, scene::IComponent::State state,
                                const Spec &spec, const String &chassisPath, const String &wheelPath,
                                Colour paint, const String &liveryPath )
    {
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? dynamic_pointer_cast<MeshManager>( app->getMeshManager() ) : nullptr;
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        if( !parent || !manager || !graphics || !graphics->getMaterialManager() ||
            !graphics->getTextureManager() || !app->getGameManager() )
        {
            throw std::runtime_error( "CarVisual needs the scene, mesh, material and texture systems" );
        }
        Models models;
        models.chassis = readModel( manager->loadMesh(
            prepareCarVisualGLB( chassisPath.c_str(), ".FBCache/CarVisual" ).c_str() ) );
        if( !wheelPath.empty() )
        {
            models.wheel = readModel( manager->loadMesh(
                prepareCarVisualGLB( wheelPath.c_str(), ".FBCache/CarVisual" ).c_str() ) );
        }
        auto next = std::make_unique<Impl>();
        next->state = state;
        next->visual.configure( spec, models, paint );
        next->visual.setLivery( liveryPath.c_str() );
        auto materialManager = graphics->getMaterialManager();
        std::unordered_map<const Mesh *, SmartPtr<IMeshResource>> cache;
        for( const auto &node : next->visual.nodes() )
        {
            auto actor = app->getGameManager()->createActor();
            actor->setName( "CarVisual: " + String( node.name.c_str() ) );
            actor->setFlag( scene::IGameActor::ActorFlagDontSave, true );
            next->actors.push_back( actor );
            next->materials.push_back( nullptr );
            next->renderers.push_back( nullptr );
            if( node.parent < 0 )
            {
                parent->addChild( actor );
            }
            else
            {
                next->actors[node.parent]->addChild( actor );
            }
            if( !node.mesh )
            {
                continue;
            }
            auto &resource = cache[node.mesh.get()];
            if( !resource )
            {
                Array<Vector3F> positions, normals, uv0, uv1;
                Array<Vector4F> tangents;
                for( const auto &v : node.mesh->vertices )
                {
                    positions.push_back( vector( v.position ) );
                    normals.push_back( vector( v.normal ) );
                    tangents.emplace_back( v.tangent.x, v.tangent.y, v.tangent.z, v.tangentSign );
                    uv0.emplace_back( v.uv0.x, v.uv0.y, 0 );
                    uv1.emplace_back( v.uv1.x, v.uv1.y, 0 );
                }
                Array<u32> indices( node.mesh->indices.begin(), node.mesh->indices.end() );
                auto mesh = MeshUtil::createMesh( positions, normals, tangents, uv0, uv1, indices );
                mesh->updateAABB( true );
                auto r = make_ptr<MeshResource>();
                const auto path = "__carvisual/" + StringUtil::getUUID() + ".fbmeshbin";
                r->setName( path );
                r->setFilePath( path );
                r->setMesh( mesh );
                r->setLoadingState( LoadingState::Loaded );
                manager->addMeshResource( r );
                resource = r;
                next->meshes.push_back( r );
            }
            const auto name = "CarVisual/" + StringUtil::getUUID();
            auto original = materialManager->getByName( node.mesh->sourceMaterial.c_str() );
            auto material = dynamic_pointer_cast<render::IMaterial>(
                original ? materialManager->cloneResource( original, name )
                         : materialManager->create( name ) );
            if( !material )
            {
                throw std::runtime_error( "Cannot create CarVisual material" );
            }
            next->materials.back() = material;
            material->load( nullptr );
            material->setDiffuse( colour( node.colour ) );
            material->setMetalness( node.metalness );
            material->setRoughness( node.roughness );
            material->setEditorFloat( "clearCoat", node.clearcoat );
            material->setEditorFloat( "clearCoatRoughness", node.clearcoatRoughness );
            material->setTransparent( node.transparent );
            material->setDepthWrite( node.depthWrite );
            material->setCullMode( node.transparent ? 1 : 0 );
            material->setBlendMode( node.additive ? 3 : ( node.transparent ? 1 : 0 ) );
            if( node.paint )
            {
                if( liveryPath.empty() )
                {
                    material->setTexture( SmartPtr<render::ITexture>(), 0 );
                }
                else
                {
                    material->setTexture( liveryPath, 0 );
                    material->setUVSet( 1 );
                }
            }
            if( node.texture != Texture::None )
            {
                material->setTexture( next->texture( node.texture ), 0 );
            }
            actor->addComponent<scene::Mesh>()->setMeshResource( resource );
            auto renderer = actor->addComponent<scene::MeshRenderer>();
            renderer->setState( state );
            next->renderers.back() = renderer;
            auto component = actor->addComponent<scene::Material>();
            component->setMaterial( material );
            component->setState( state );
        }
        if( spec.player )
        {
            for( unsigned i = 0; i < 2; ++i )
            {
                const auto &spot = next->visual.spots()[i];
                auto actor = app->getGameManager()->createActor();
                actor->setName( "CarVisual headlight" );
                actor->setFlag( scene::IGameActor::ActorFlagDontSave, true );
                next->actors[1]->addChild( actor );
                next->lightActors[i] = actor;
                next->actors.push_back( actor );
                actor->setLocalPosition( vector( spot.position ) );
                const auto d = vector( spot.target ) - vector( spot.position );
                QuaternionF q;
                q.fromRadians( { std::atan2( d.y, std::sqrt( d.x * d.x + d.z * d.z ) ),
                                 std::atan2( -d.x, -d.z ), 0 } );
                actor->setLocalOrientation( q );
                auto light = actor->addComponent<scene::Light>();
                light->setLightType( LightTypes::LT_SPOTLIGHT );
                light->setDiffuseColour( colour( spot.colour ) );
                light->setState( state );
                next->lights[i] = light;
            }
        }
        next->apply();
        m_impl = std::move( next );
    }

    void CarVisualScene::sync( const Frame &frame, double seconds, const Vec3 *camera )
    {
        if( m_impl )
        {
            m_impl->visual.sync( frame, seconds, camera );
            m_impl->apply();
        }
    }

    void CarVisualScene::setState( scene::IComponent::State state )
    {
        if( !m_impl || m_impl->state == state )
        {
            return;
        }
        m_impl->state = state;
        for( auto actor : m_impl->actors )
        {
            for( auto component : actor->getComponents() )
            {
                component->setState( state );
            }
        }
    }
}  // namespace workphone::vehiclevisual
