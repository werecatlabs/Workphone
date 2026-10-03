#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/CollisionMesh.hpp>
#include <Workphone/Mesh/CollisionSubMesh.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/Mesh.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, CollisionMesh, ISharedObject );

    CollisionMesh::CollisionMesh() = default;

    CollisionMesh::~CollisionMesh() = default;

    void CollisionMesh::load( SmartPtr<ISharedObject> data )
    {
    }

    void CollisionMesh::unload( SmartPtr<ISharedObject> data )
    {
    }

    CollisionMesh::CollisionMesh( const SmartPtr<IMesh> &mesh, const Matrix4<real_Num> &transform )
    {
        auto subMeshes = mesh->getSubMeshes();
        for( auto subMesh : subMeshes )
        {
            SmartPtr<CollisionSubMesh> collisionSubMesh(
                new CollisionSubMesh( mesh, subMesh, transform ) );
            m_subMeshes.push_back( collisionSubMesh );
        }
    }

    void CollisionMesh::HitData::setHitDistance( f32 hitDistance )
    {
        m_hitDistance = hitDistance;
    }

    auto CollisionMesh::HitData::getHitDistance() const -> f32
    {
        return m_hitDistance;
    }

    CollisionMesh::HitData::HitData( float distance ) : m_hitDistance( distance )
    {
    }

    CollisionMesh::HitData::HitData()
    {
    }

    auto CollisionMesh::rayCast( const Vector3<real_Num> &origin, const Vector3<real_Num> &dir,
                                 Array<HitData> &hits ) -> bool
    {
        bool retValue = false;

        for( auto &subMesh : m_subMeshes )
        {
            Array<float> distances;
            retValue = subMesh->rayCast( origin, dir, distances );

            if( retValue )
            {
                hits.emplace_back( distances[0] );
                return true;
            }
        }

        return retValue;
    }

    auto CollisionMesh::rayCast( const Vector3<real_Num> &origin, const Vector3<real_Num> &dir,
                                 Array<f32> &hits ) -> bool
    {
        bool retValue = false;

        for( auto &subMesh : m_subMeshes )
        {
            retValue = subMesh->rayCast( origin, dir, hits );
            if( retValue )
            {
                return true;
            }
        }

        return retValue;
    }

}  // namespace workphone
