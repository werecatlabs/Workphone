#ifndef ITerrainShape_h__
#define ITerrainShape_h__

#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a terrain collision shape.
         *
         * The `ITerrainShape` class is a subtype of `IPhysicsShape3` that defines the
         * interface for a collision shape representing a terrain. Terrain shapes can be
         * used with physics engines to enable collision detection and response between
         * rigid bodies and terrain features in a 3D environment.
         *
         * This class provides the necessary functions to describe the shape of a terrain
         * collision object, such as its dimensions, orientation, and position, as well as
         * to query properties of the shape, such as its bounding box or volume. These
         * functions can be overridden by subclasses to implement specific terrain shapes.
         *
         * In addition to the functions inherited from `IPhysicsShape3`, this class also
         * defines a number of specific member functions for terrain shapes, including
         * functions to set and retrieve the heightfield data for the terrain, and to
         * adjust the scaling and offset of the heightfield data.
         *
         * The terrain shape is typically used to represent a heightfield-based terrain,
         * where the height of the terrain at each point is defined by a heightfield data
         * array. This allows for efficient collision detection and response with the terrain,
         * as the heightfield data can be used to quickly determine the height of the terrain
         * at any given point.
         *
         * @see IPhysicsShape3
         * @see IPhysicsManager
         * @see IPhysicsScene3
         */
        class WPCore_API ITerrainShape : public IPhysicsShape3
        {
        public:
            /** Destructor */
            ~ITerrainShape() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // ITerrainShape_h__
