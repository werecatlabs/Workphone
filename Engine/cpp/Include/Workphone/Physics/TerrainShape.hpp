#ifndef __WP_TerrainShape_h__
#define __WP_TerrainShape_h__

#include <Workphone/Interface/Physics/ITerrainShape.hpp>
#include <Workphone/Physics/PhysicsShape3.hpp>
#include <Workphone/Graphics/TerrainData.hpp>

namespace workphone
{
    namespace physics
    {
        /** Immutable authoritative source passed through the existing shape load API.
         * Actor translation/yaw belong to the rigid body; positive actor scale is baked once.
         */
        class WPCore_API TerrainCollisionData : public ISharedObject
        {
        public:
            render::TerrainSnapshot terrain;
            Vector3<real_Num> scale = Vector3<real_Num>::unit();
            // Full-resolution collision is rebuilt synchronously. Callers may
            // lower this budget; raising it above the engine ceiling is rejected.
            static constexpr u32 maximumSupportedTriangles = 524288;
            u32 maximumTriangles = maximumSupportedTriangles;
            WP_CLASS_REGISTER_DECL;
        };

        /**
         * @class TerrainShape
         * @brief Represents a terrain-based collision shape in 3D space.
         *
         * This class implements the ITerrainShape interface and is used to define
         * a heightmap-based terrain for physics collisions. It allows for a
         * complex, irregular surface to be represented efficiently in the
         * physics engine.
         */
        class WPCore_API TerrainShape : public PhysicsShape3<ITerrainShape>
        {
        public:
            /**
             * @brief Constructs a new TerrainShape instance.
             */
            TerrainShape();

            /**
             * @brief Destroys the TerrainShape instance.
             */
            ~TerrainShape() override;

            render::TerrainSnapshot getTerrainSnapshot() const;
            u64 getTerrainRevision() const;
            Vector3<real_Num> getTerrainScale() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            render::TerrainSnapshot m_terrainSnapshot;
            Vector3<real_Num> m_terrainScale = Vector3<real_Num>::unit();
        };

    }  // namespace physics
}  // namespace workphone

#endif  // TerrainShape_h__
