#ifndef Resolution_h__
#define Resolution_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    /**
     * @class Resolution
     * @brief Represents a display resolution with width, height, and color depth.
     *
     * This class encapsulates the properties of a display resolution, including its dimensions
     * and color depth. It inherits from ISharedObject to support shared memory management.
     */
    class Resolution : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Creates a new Resolution object with default values.
         */
        Resolution();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of resources when the object is destroyed.
         */
        ~Resolution() override;

        /**
         * @brief The dimensions of the resolution.
         *
         * Represents the width and height of the resolution in pixels.
         */
        Vector2I size;

        /**
         * @brief The color depth of the resolution.
         *
         * Specifies the number of bits used to represent each pixel's color.
         * Default value is 32 bits (8 bits per channel for RGBA).
         */
        s32 depth = 32;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // Resolution_h__
