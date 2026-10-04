#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsTerrainShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    WPPhysicsTerrainShape3::WPPhysicsTerrainShape3() : WPPhysicsShape3Adapter( WORKPHONE_COLLISION_SHAPE_MESH )
    {
    }

    void WPPhysicsTerrainShape3::load( SmartPtr<ISharedObject> data )
    {
        if( data )
        {
            if( data->isDerived<IMeshResource>() )
            {
                m_meshResource = workphone::static_pointer_cast<IMeshResource>( data );
                m_mesh = m_meshResource->getMesh();
            }
            else if( data->isDerived<IMesh>() )
            {
                m_mesh = workphone::static_pointer_cast<IMesh>( data );
            }
        }
        rebuildMeshData();
    }

    SmartPtr<IPhysicsShape3> WPPhysicsTerrainShape3::clone()
    {
        auto shape = workphone::make_ptr<WPPhysicsTerrainShape3>();
        shape->m_meshResource = m_meshResource;
        shape->m_mesh = m_mesh;
        shape->rebuildMeshData();
        shape->setLocalPose( getLocalPose() );
        shape->setSimulationFilterData( getSimulationFilterData() );
        shape->setMaterial( getMaterial() );
        shape->setEnabled( isEnabled() );
        shape->setTrigger( isTrigger() );
        return shape;
    }

    void WPPhysicsTerrainShape3::rebuildMeshData()
    {
        if( !m_mesh )
        {
            m_vertices.clear();
            m_indices.clear();
            wp_collision_shape_set_mesh_data( getShape(), nullptr );
            return;
        }

        const auto points = MeshUtil::getPoints( m_mesh );
        const auto indices = MeshUtil::getIndices( m_mesh );
        if( points.empty() || indices.empty() || indices.size() % 3 != 0 )
        {
            WP_LOG_WARNING( "WPPhysicsTerrainShape3::rebuildMeshData: terrain has no valid triangles." );
            m_vertices.clear();
            m_indices.clear();
            wp_collision_shape_set_mesh_data( getShape(), nullptr );
            return;
        }

        m_vertices.resize( points.size() * 3 );
        for( size_t i = 0; i < points.size(); ++i )
        {
            m_vertices[i * 3] = static_cast<wp_f32>( points[i].X() );
            m_vertices[i * 3 + 1] = static_cast<wp_f32>( points[i].Y() );
            m_vertices[i * 3 + 2] = static_cast<wp_f32>( points[i].Z() );
        }
        m_indices.assign( indices.begin(), indices.end() );

        const wp_collision_mesh_data meshData = { m_vertices.data(),
                                                  static_cast<wp_u32>( points.size() ), m_indices.data(),
                                                  static_cast<wp_u32>( m_indices.size() / 3 ) };
        wp_collision_shape_set_mesh_data( getShape(), &meshData );
    }
} // namespace workphone::physics
