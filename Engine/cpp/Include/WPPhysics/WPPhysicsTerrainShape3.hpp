#ifndef WPPHYSICSTERRAINSHAPE3_HPP
#define WPPHYSICSTERRAINSHAPE3_HPP

#include <WPPhysics/WPPhysicsShape3T.hpp>
#include <Workphone/Physics/TerrainShape.hpp>

namespace workphone::physics
{

    class WPPhysicsTerrainShape3 : public WPPhysicsShape3T<TerrainShape>
    {
    public:
        WPPhysicsTerrainShape3();

        void load( SmartPtr<ISharedObject> data ) override;
        SmartPtr<IPhysicsShape3> clone() override;

    private:
        void rebuildMeshData();

        SmartPtr<IMeshResource> m_meshResource;
        SmartPtr<IMesh> m_mesh;
        Array<wp_f32> m_vertices;
        Array<wp_u32> m_indices;
    };

}  // namespace workphone::physics

#endif
