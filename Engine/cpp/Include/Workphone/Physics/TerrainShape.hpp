#ifndef __WP_TerrainShape_h__
#define __WP_TerrainShape_h__

#include <Workphone/Interface/Physics/ITerrainShape.hpp>
#include <Workphone/Physics/PhysicsShape3.hpp>

namespace workphone
{
    namespace physics
    {

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

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace physics
}  // namespace workphone

#endif  // TerrainShape_h__
