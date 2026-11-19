#include <Workphone/WorkphonePCH.hpp>
#include "ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    namespace sceneBuild
    {
        SmartPtr<scene::IGameActor> publishMesh( SceneAssets &assets, const String &name,
                                                 SmartPtr<IMesh> generated, size_t triangles,
                                                 SmartPtr<render::IMaterial> m,
                                                 SmartPtr<scene::IGameActor> parent,
                                                 bool deferNative )
        {
            if( !generated )
            {
                return nullptr;
            }
            auto app = core::IApplicationManager::instance();
            auto manager = dynamic_pointer_cast<MeshManager>( app->getMeshManager() );
            if( !manager )
            {
                manager = make_ptr<MeshManager>();
                app->setMeshManager( manager );
            }
            generated->updateAABB( true );
            auto path = assets.resourcePrefix + "/mesh/" + StringUtil::toString( assets.meshes.size() ) +
                        ".meshbin";
            reserveOwnerSlot( assets.meshes );
            auto resource =
                dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve( path ).first );
            if( !resource )
            {
                throw std::runtime_error( "Cannot create procedural mesh resource" );
            }
            assets.meshes.push_back( resource );
            resource->setName( path );
            resource->setFilePath( path );
            resource->setMesh( generated );
            resource->setLoadingState( LoadingState::Loaded );
            assets.triangles += triangles;
            reserveOwnerSlot( assets.actors );
            auto actor = app->getGameManager()->createActor();
            if( !actor )
            {
                throw std::runtime_error( "Cannot create procedural actor" );
            }
            assets.actors.push_back( actor );
            actor->setName( name );
            if( parent )
            {
                parent->addChild( actor );
            }
            actor->addComponent<scene::Mesh>()->setMeshResource( resource );
            if( deferNative )
            {
                // addComponent<T>() loads immediately. Disable the renderer
                // before that load so no detailed native mesh enters the scene.
                auto renderer = app->getFactoryManagerPtr()->make_ptr<scene::MeshRenderer>();
                actor->addComponentInstance( renderer );
                renderer->setEnabled( false );
                renderer->load( nullptr );
            }
            else
            {
                actor->addComponent<scene::MeshRenderer>();
            }
            actor->addComponent<scene::Material>()->setMaterial( m );
            return actor;
        }

        SmartPtr<scene::IGameActor> mesh( SceneAssets &assets, const String &name, const Geometry &g,
                                          SmartPtr<render::IMaterial> m,
                                          SmartPtr<scene::IGameActor> parent,
                                          bool deferNative )
        {
            if( g.indices.empty() )
            {
                return nullptr;
            }
            // DX11's material rasterizer uses counterclockwise front faces, so keep the
            // generated winding that matches the supplied lighting normals.
            return publishMesh(
                assets, name,
                MeshUtil::createMesh( g.positions, g.normals, g.tangents, g.uv, g.indices ),
                g.indices.size() / 3, m, parent, deferNative );
        }

        Vector3F vector( const VehiclePhysicsVector3 &p )
        {
            return { float( p.x ), float( p.y ), float( p.z ) };
        }

        void attachWorldActor( SmartPtr<IGameScene> scene, SmartPtr<IGameActor> owner,
                               SmartPtr<IGameActor> actor )
        {
            actor->setFlag( IGameActor::ActorFlagWorldSpaceTransform, true );
            owner->addChild( actor );
            actor->setScene( scene );
            scene->registerAllUpdates( actor );
        }

        void staticBox( SceneAssets &assets, const String &name, Vector3F centre, Vector3F size,
                        QuaternionF orientation )
        {
            auto app = core::IApplicationManager::instance();
            reserveOwnerSlot( assets.actors );
            auto actor = app->getGameManager()->createActor();
            if( !actor )
            {
                throw std::runtime_error( "Cannot create procedural actor" );
            }
            assets.actors.push_back( actor );
            // Own these independently of batched render meshes and tree LODs.
            // The normal generated-scene cleanup also removes their physics bodies.
            actor->setName( "Trackside collision: " + name );
            actor->setStatic( true );
            actor->setPosition( centre );
            actor->setOrientation( orientation );
            // CollisionBox takes full dimensions, not half extents.
            actor->addComponent<scene::CollisionBox>()->setExtents( size );
            actor->addComponent<scene::Rigidbody>();
        }

        void railCollision( SceneAssets &assets, Vector3F start, Vector3F end )
        {
            auto direction = end - start;
            direction.y = 0;
            const auto length = direction.length();
            if( length <= 1e-4f )
            {
                return;
            }
            const auto yaw = std::atan2( direction.x, direction.z ) * 180.f / pi;
            staticBox( assets, "Guardrail beam", ( start + end ) * .5f + Vector3F( 0, .5f, 0 ),
                       { .2f, .5f, length }, QuaternionF::eulerDegrees( 0, yaw, 0 ) );
        }

        float cityValueNoise( u32 seed, float x, float z )
        {
            const auto xi = static_cast<s32>( std::floor( x ) );
            const auto zi = static_cast<s32>( std::floor( z ) );
            auto u = x - xi, v = z - zi;
            u = u * u * ( 3 - 2 * u );
            v = v * v * ( 3 - 2 * v );
            const auto sample = [=]( s32 dx, s32 dz ) {
                const auto key = ( u64( u32( xi + dx ) ) << 32 ) | u32( zi + dz );
                return float(
                           render::foliageStableHash( key ^ ( u64( seed ) * 0x9e3779b97f4a7c15ULL ) ) >>
                           40 ) /
                       16777216.f;
            };
            const float a = sample( 0, 0 ) * ( 1 - u ) + sample( 1, 0 ) * u;
            const float b = sample( 0, 1 ) * ( 1 - u ) + sample( 1, 1 ) * u;
            return a * ( 1 - v ) + b * v;
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
