#ifndef CollisionSubMesh_h__
#define CollisionSubMesh_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /** @brief CollisionSubMesh
     * @details
     *  CollisionSubMesh is a class that represents a submesh that can be used for collision detection.
     *  It uses the Opcode library to perform ray casting.
     */
    class WPCore_API CollisionSubMesh : public ISharedObject
    {
    public:
        CollisionSubMesh();
        CollisionSubMesh( SmartPtr<IMesh> mesh, SmartPtr<ISubMesh> subMesh,
                          Matrix4<real_Num> transform );
        ~CollisionSubMesh() override;

        bool rayCast( const Vector3<real_Num> &origin, const Vector3<real_Num> &dir,
                      Array<float> &hits );

        void *getUserData() const override;
        void setUserData( void *userData ) override;

        void build( float *vertices, int vertexStide, int vertexCount, const void *indices,
                    int indexCount );

        WP_CLASS_REGISTER_DECL;

        Opcode::RayCollider *m_rayCollider;
        Opcode::Model *m_tree;
        Opcode::MeshInterface *m_mesh;

        u32 *m_indices;
        IceMaths::Point *m_points;

        Matrix4F m_transformation;

        u32 m_vertexCount;
        u32 m_indexCount;
        u32 m_triangleCount;

        void *m_userData;
    };
}  // namespace workphone

#endif  // CollisionSubMesh_h__
