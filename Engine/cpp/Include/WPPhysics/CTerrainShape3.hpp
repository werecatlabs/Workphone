#ifndef WP_CTERRAINSHAPE3_HPP
#define WP_CTERRAINSHAPE3_HPP

#include <WPPhysics/CPhysicsShape3Adapter.hpp>
#include <Workphone/Interface/Physics/ITerrainShape.hpp>

namespace workphone::physics
{
    class CTerrainShape3 : public CPhysicsShape3Adapter<ITerrainShape>
    {
    public:
        CTerrainShape3();

        void                     load( SmartPtr<ISharedObject> data ) override;
        SmartPtr<IPhysicsShape3> clone() override;

    private:
        void rebuildMeshData();

        SmartPtr<IMeshResource> m_meshResource;
        SmartPtr<IMesh>         m_mesh;
        Array<wp_f32>           m_vertices;
        Array<wp_u32>           m_indices;
    };
} // namespace workphone::physics

#endif
