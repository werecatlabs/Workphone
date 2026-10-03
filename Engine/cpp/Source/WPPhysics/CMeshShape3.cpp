#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/CMeshShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    CMeshShape3::CMeshShape3() : CPhysicsShape3Adapter( WORKPHONE_COLLISION_SHAPE_MESH )
    {
    }

    void CMeshShape3::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        if( data )
        {
            if( data->isDerived<IMeshResource>() )
            {
                setMeshResource( workphone::static_pointer_cast<IMeshResource>( data ) );
            }
            else if( data->isDerived<IMesh>() )
            {
                setCleanMesh( workphone::static_pointer_cast<IMesh>( data ) );
            }
        }

        rebuildMeshData();

        setLoadingState( LoadingState::Loaded );
    }

    SmartPtr<IMesh> CMeshShape3::getMesh() const
    {
        return m_meshResource ? m_meshResource->getMesh() : nullptr;
    }

    SmartPtr<IMeshResource> CMeshShape3::getMeshResource() const
    {
        return m_meshResource;
    }

    void CMeshShape3::setMeshResource( SmartPtr<IMeshResource> meshResource )
    {
        m_meshResource = meshResource;
        rebuildMeshData();
    }

    SmartPtr<IMesh> CMeshShape3::getCleanMesh() const
    {
        return m_cleanMesh;
    }

    void CMeshShape3::setCleanMesh( SmartPtr<IMesh> cleanMesh )
    {
        m_cleanMesh = cleanMesh;
        rebuildMeshData();
    }

    bool CMeshShape3::isConvex() const
    {
        return m_convex;
    }

    void CMeshShape3::setConvex( bool convex )
    {
        m_convex = convex;
    }

    SmartPtr<IPhysicsShape3> CMeshShape3::clone()
    {
        ScopedLock lock( this );
        auto       shape = workphone::make_ptr<CMeshShape3>();
        shape->setMeshResource( m_meshResource );
        shape->setCleanMesh( m_cleanMesh );
        shape->setConvex( m_convex );
        shape->setLocalPose( getLocalPose() );
        shape->setSimulationFilterData( getSimulationFilterData() );
        shape->setMaterial( getMaterial() );
        shape->setEnabled( isEnabled() );
        shape->setTrigger( isTrigger() );
        return shape;
    }

    void CMeshShape3::rebuildMeshData()
    {
        ScopedLock lock( this );

        auto mesh = m_cleanMesh ? m_cleanMesh : getMesh();
        if( !mesh )
        {
            m_vertices.clear();
            m_indices.clear();
            wp_collision_shape_set_mesh_data( getShape(), nullptr );
            return;
        }

        const auto points = MeshUtil::getPoints( mesh );
        const auto indices = MeshUtil::getIndices( mesh );
        if( points.empty() || indices.empty() || indices.size() % 3 != 0 )
        {
            WP_LOG_WARNING( "CMeshShape3::rebuildMeshData: mesh has no valid triangles." );
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
