#ifndef WPPHYSICSTERRAINSHAPE3_HPP
#define WPPHYSICSTERRAINSHAPE3_HPP

#include <WPPhysics/WPPhysicsShape3T.hpp>
#include <Workphone/Physics/TerrainShape.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>

namespace workphone::physics
{
    class WPPhysicsTerrainShape3 : public WPPhysicsShape3T<TerrainShape>
    {
    public:
        WPPhysicsTerrainShape3();
        ~WPPhysicsTerrainShape3() override;

        void load(SmartPtr<ISharedObject> data) override;
        void unload(SmartPtr<ISharedObject> data) override;
        SmartPtr<IPhysicsShape3> clone() override;
        bool isValid() const override;

    private:
        void installMesh(const Array<Vector3F> &points, const Array<u32> &indices);

        SmartPtr<IMeshResource> m_meshResource;
        SmartPtr<IMesh> m_mesh;
        StreamBudget m_collisionBudget, m_sourceBudget;
        Array<wp_f32> m_vertices;
        Array<wp_u32> m_indices;
    };
} // namespace workphone::physics

#endif
