#ifndef ColourUtil_h__
#define ColourUtil_h__

#include <Workphone/WorkphoneTypes.hpp>

namespace workphone
{
    /**
     * @class ColourUtil
     * @brief Utility class for color format conversions and component extraction.
     *
     * Provides static methods to convert between various color formats (16-bit, 32-bit)
     * and to extract or manipulate color components (alpha, red, green, blue).
     */
    class WPCore_API ColourUtil
    {
    public:
        /**
         * @brief Creates a 16-bit A1R5G5B5 color from 8-bit RGBA components.
         * @param r Red component (0-255)
         * @param g Green component (0-255)
         * @param b Blue component (0-255)
         * @param a Alpha component (0-255)
         * @return 16-bit color in A1R5G5B5 format
         */
        static u16 RGBA16( u32 r, u32 g, u32 b, u32 a );

        /**
         * @brief Creates a 16-bit A1R5G5B5 color from 8-bit RGB components (alpha set to 1).
         * @param r Red component (0-255)
         * @param g Green component (0-255)
         * @param b Blue component (0-255)
         * @return 16-bit color in A1R5G5B5 format
         */
        static u16 RGB16( u32 r, u32 g, u32 b );

        /**
         * @brief Creates a 16-bit A1R5G5B5 color from 16-bit RGB components.
         * @param r Red component (0-65535)
         * @param g Green component (0-65535)
         * @param b Blue component (0-65535)
         * @return 16-bit color in A1R5G5B5 format
         */
        static u16 RGB16from16( u16 r, u16 g, u16 b );

        /**
         * @brief Converts a 32-bit X8R8G8B8 color to a 16-bit A1R5G5B5 color.
         * @param color 32-bit color in X8R8G8B8 format
         * @return 16-bit color in A1R5G5B5 format
         */
        static u16 X8R8G8B8toA1R5G5B5( u32 color );

        /**
         * @brief Converts a 32-bit A8R8G8B8 color to a 16-bit A1R5G5B5 color.
         * @param color 32-bit color in A8R8G8B8 format
         * @return 16-bit color in A1R5G5B5 format
         */
        static u16 A8R8G8B8toA1R5G5B5( u32 color );

        /**
         * @brief Converts a 32-bit A8R8G8B8 color to a 16-bit R5G6B5 color.
         * @param color 32-bit color in A8R8G8B8 format
         * @return 16-bit color in R5G6B5 format
         */
        static u16 A8R8G8B8toR5G6B5( u32 color );

        /**
         * @brief Converts a 16-bit A1R5G5B5 color to a 32-bit A8R8G8B8 color.
         *
         * Builds a higher quality 32-bit color by extending the lower bits of each channel
         * with the high bits from the source color.
         *
         * @param color 16-bit color in A1R5G5B5 format
         * @return 32-bit color in A8R8G8B8 format
         */
        static u32 A1R5G5B5toA8R8G8B8( u32 color );

        /**
         * @brief Converts a 16-bit R5G6B5 color to a 32-bit A8R8G8B8 color.
         * @param color 16-bit color in R5G6B5 format
         * @return 32-bit color in A8R8G8B8 format
         */
        static u32 R5G6B5toA8R8G8B8( u16 color );

        /**
         * @brief Converts a 16-bit R5G6B5 color to a 16-bit A1R5G5B5 color.
         * @param color 16-bit color in R5G6B5 format
         * @return 16-bit color in A1R5G5B5 format
         */
        static u16 R5G6B5toA1R5G5B5( u16 color );

        /**
         * @brief Converts a 16-bit A1R5G5B5 color to a 16-bit R5G6B5 color.
         * @param color 16-bit color in A1R5G5B5 format
         * @return 16-bit color in R5G6B5 format
         */
        static u16 A1R5G5B5toR5G6B5( u16 color );

        /**
         * @brief Extracts the alpha component from a 16-bit A1R5G5B5 color.
         * @param color 16-bit color in A1R5G5B5 format
         * @return Alpha component (0 or 1)
         */
        static u32 getAlpha( u16 color );

        /**
         * @brief Extracts the red component from a 16-bit A1R5G5B5 color.
         * @param color 16-bit color in A1R5G5B5 format
         * @return Red component (shifted left by 3 to get 8-bit value)
         */
        static u32 getRed( u16 color );

        /**
         * @brief Extracts the green component from a 16-bit A1R5G5B5 color.
         * @param color 16-bit color in A1R5G5B5 format
         * @return Green component (shifted left by 3 to get 8-bit value)
         */
        static u32 getGreen( u16 color );

        /**
         * @brief Extracts the blue component from a 16-bit A1R5G5B5 color.
         * @param color 16-bit color in A1R5G5B5 format
         * @return Blue component (shifted left by 3 to get 8-bit value)
         */
        static u32 getBlue( u16 color );

        /**
         * @brief Extracts the red component as a signed value from a 16-bit A1R5G5B5 color.
         * @param color 16-bit color in A1R5G5B5 format
         * @return Signed red component (shifted left by 3 to get 8-bit value)
         */
        static s32 getRedSigned( u16 color );

        /**
         * @brief Extracts the green component as a signed value from a 16-bit A1R5G5B5 color.
         * @param color 16-bit color in A1R5G5B5 format
         * @return Signed green component (shifted left by 3 to get 8-bit value)
         */
        static s32 getGreenSigned( u16 color );

        /**
         * @brief Extracts the blue component as a signed value from a 16-bit A1R5G5B5 color.
         * @param color 16-bit color in A1R5G5B5 format
         * @return Signed blue component (shifted left by 3 to get 8-bit value)
         */
        static s32 getBlueSigned( u16 color );

        /**
         * @brief Computes the average value of the color components from a 16-bit A1R5G5B5 color.
         * @param color 16-bit color in A1R5G5B5 format (signed)
         * @return Average value of the color components
         */
        static s32 getAverage( s16 color );
    };
}  // namespace workphone

#endif  // ColourUtil_h__
