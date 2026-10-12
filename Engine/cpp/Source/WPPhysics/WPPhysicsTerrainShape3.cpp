#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsTerrainShape3.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/TerrainTiles.hpp>
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace workphone::physics
{
    WPPhysicsTerrainShape3::WPPhysicsTerrainShape3() :
        WPPhysicsShape3T<TerrainShape>( WORKPHONE_COLLISION_SHAPE_MESH )
    {
    }

    WPPhysicsTerrainShape3::~WPPhysicsTerrainShape3()
    {
        unload( nullptr );
    }

    void WPPhysicsTerrainShape3::load( SmartPtr<ISharedObject> data )
    {
        WPPhysicsShape3T<TerrainShape>::load( nullptr );
        if( !data )
        {
            return;  // A missing replacement never erases last-good collision.
        }
        if( auto terrain = dynamic_cast<TerrainCollisionData *>( data.get() ) )
        {
            String error;
            Transform3<real_Num> placement;
            placement.setScale( terrain->scale );
            if( !terrain->terrain || !render::validateTerrainData( *terrain->terrain, error ) ||
                !render::validateTerrainPlacement( *terrain->terrain, placement, error ) )
            {
                throw std::runtime_error( "Invalid terrain collision source: " + error );
            }
            if( terrain->maximumTriangles > TerrainCollisionData::maximumSupportedTriangles )
            {
                throw std::runtime_error(
                    "Terrain collision triangle budget exceeds the supported ceiling of 524288." );
            }
            const auto &source = *terrain->terrain;
            const auto cells = size_t( source.dimensions.x - 1 ) * ( source.dimensions.y - 1 );
            const auto solidCells = source.holes.empty()
                                        ? cells
                                        : static_cast<size_t>( std::count(
                                              source.holes.begin(), source.holes.end(), u8( 0 ) ) );
            if( solidCells * 2 > terrain->maximumTriangles )
            {
                throw std::runtime_error( "Terrain collision exceeds its solid-triangle budget of " +
                                          std::to_string( terrain->maximumTriangles ) +
                                          "; tiled collision is required for larger surfaces." );
            }
            auto scheduler = WorldStreamingScheduler::shared();
            auto sourceBudget = scheduler->reserve( StreamMemoryClass::SourceCpu,
                                                    render::terrainSourceBytes( source ), error );
            if( !sourceBudget )
            {
                throw std::runtime_error( error );
            }
            // Conservative reservation includes full-grid mesh preparation (even
            // holes retain vertex/index capacity), shallow backing arrays, native
            // cooked triangle bounds and the 2*triangle BVH allocation ceiling.
            const u64 collisionBytes = u64( source.heights.size() ) * 64 + u64( cells ) * 24 +
                                       u64( solidCells ) * 2 * 160 + 4096;
            auto collisionBudget =
                scheduler->reserve( StreamMemoryClass::Collision, collisionBytes, error );
            if( !collisionBudget )
            {
                throw std::runtime_error( error );
            }
            render::TerrainMeshData mesh;
            if( !render::buildTerrainMeshData( *terrain->terrain, mesh, error ) )
            {
                throw std::runtime_error( error );
            }
            for( auto &point : mesh.positions )
            {
                point.x = static_cast<f32>( point.x * terrain->scale.x );
                point.y = static_cast<f32>( point.y * terrain->scale.y );
                point.z = static_cast<f32>( point.z * terrain->scale.z );
            }
            installMesh( mesh.positions, mesh.indices );
            // Keep retiring reservations until the corresponding retained source
            // and any old backing storage have actually been released.
            auto oldCollisionBudget = std::move( m_collisionBudget );
            auto oldSourceBudget = std::move( m_sourceBudget );
            auto oldSource = std::move( m_terrainSnapshot );
            m_collisionBudget = std::move( collisionBudget );
            m_sourceBudget = std::move( sourceBudget );
            m_terrainSnapshot = terrain->terrain;
            m_terrainScale = terrain->scale;
            m_meshResource = nullptr;
            m_mesh = nullptr;
            return;
        }

        SmartPtr<IMeshResource> resource;
        SmartPtr<IMesh> mesh;
        if( data->isDerived<IMeshResource>() )
        {
            resource = static_pointer_cast<IMeshResource>( data );
            mesh = resource->getMesh();
        }
        else if( data->isDerived<IMesh>() )
        {
            mesh = static_pointer_cast<IMesh>( data );
        }
        if( !mesh )
        {
            throw std::runtime_error( "Terrain collision requires a terrain snapshot or mesh." );
        }
        const auto meshPoints = MeshUtil::getPoints( mesh );
        Array<Vector3F> points;
        points.reserve( meshPoints.size() );
        for( const auto &point : meshPoints )
        {
            points.emplace_back( static_cast<f32>( point.x ), static_cast<f32>( point.y ),
                                 static_cast<f32>( point.z ) );
        }
        const auto indices = MeshUtil::getIndices( mesh );
        if( points.empty() || indices.empty() )
        {
            throw std::runtime_error( "Legacy terrain collision mesh contains no triangles." );
        }
        installMesh( points, indices );
        m_meshResource = resource;
        m_mesh = mesh;
        m_terrainSnapshot.reset();
        m_terrainScale = Vector3<real_Num>::unit();
        m_collisionBudget.reset();
        m_sourceBudget.reset();
    }

    void WPPhysicsTerrainShape3::installMesh( const Array<Vector3F> &points, const Array<u32> &indices )
    {
        if( points.size() > std::numeric_limits<wp_u32>::max() || indices.size() % 3 ||
            indices.size() / 3 > std::numeric_limits<wp_u32>::max() )
        {
            throw std::runtime_error( "Terrain collision mesh exceeds supported bounds." );
        }
        Array<wp_f32> vertices( points.size() * 3 );
        Array<wp_u32> candidateIndices( indices.begin(), indices.end() );
        for( size_t i = 0; i < points.size(); ++i )
        {
            vertices[i * 3] = points[i].x;
            vertices[i * 3 + 1] = points[i].y;
            vertices[i * 3 + 2] = points[i].z;
        }
        const wp_collision_mesh_data meshData{ vertices.data(), static_cast<wp_u32>( points.size() ),
                                               candidateIndices.data(),
                                               static_cast<wp_u32>( candidateIndices.size() / 3 ) };
        if( !wp_collision_shape_try_set_mesh_data( getShape(), &meshData ) )
        {
            throw std::runtime_error( "Unable to prepare validated terrain collision mesh." );
        }
        // Native shallow references point into these candidate allocations. Swapping
        // transfers ownership without moving the referenced elements.
        m_vertices.swap( vertices );
        m_indices.swap( candidateIndices );
    }

    void WPPhysicsTerrainShape3::unload( SmartPtr<ISharedObject> data )
    {
        WPPhysicsShape3T<TerrainShape>::unload( data );
        Array<wp_f32>().swap( m_vertices );
        Array<wp_u32>().swap( m_indices );
        m_terrainSnapshot.reset();
        m_meshResource = nullptr;
        m_mesh = nullptr;
        m_collisionBudget.reset();
        m_sourceBudget.reset();
    }

    bool WPPhysicsTerrainShape3::isValid() const
    {
        return WPPhysicsShape3T<TerrainShape>::isValid() &&
               ( m_terrainSnapshot || ( !m_vertices.empty() && !m_indices.empty() ) );
    }

    SmartPtr<IPhysicsShape3> WPPhysicsTerrainShape3::clone()
    {
        auto shape = make_ptr<WPPhysicsTerrainShape3>();
        if( m_terrainSnapshot )
        {
            auto source = make_ptr<TerrainCollisionData>();
            source->terrain = m_terrainSnapshot;
            source->scale = m_terrainScale;
            shape->load( source );
        }
        else if( m_meshResource )
        {
            shape->load( m_meshResource );
        }
        else if( m_mesh )
        {
            shape->load( m_mesh );
        }
        shape->setLocalPose( getLocalPose() );
        shape->setSimulationFilterData( getSimulationFilterData() );
        shape->setMaterial( getMaterial() );
        shape->setEnabled( isEnabled() );
        shape->setTrigger( isTrigger() );
        return shape;
    }
}  // namespace workphone::physics
