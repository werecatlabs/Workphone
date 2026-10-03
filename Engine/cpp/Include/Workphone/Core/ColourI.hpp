#ifndef ColourI_h__
#define ColourI_h__

#include <Workphone/WorkphoneTypes.hpp>

namespace workphone
{
    /** @class ColourI
     *  @brief A class representing a 32-bit integer color value in ARGB format.
     *  @details This class provides functionality for working with colors in A8R8G8B8 format,
     *           where each component (alpha, red, green, blue) is represented by 8 bits.
     *           The class supports color manipulation, conversion, and interpolation operations.
     */
    class WPCore_API ColourI
    {
    public:
        /** @brief Default constructor.
         *  @details Initializes the color to black (0x00000000).
         */
        ColourI();

        /** @brief Constructor with individual color components.
         *  @param a Alpha component (0-255)
         *  @param r Red component (0-255)
         *  @param g Green component (0-255)
         *  @param b Blue component (0-255)
         */
        ColourI( u32 a, u32 r, u32 g, u32 b );

        /** @brief Constructor with a single color value.
         *  @param clr Color value in A8R8G8B8 format
         */
        explicit ColourI( u32 clr );

        /** @brief Gets the alpha component of the color.
         *  @return The alpha value (0-255) representing the color's transparency
         */
        u32 getAlpha() const;

        /** @brief Gets the red component of the color.
         *  @return The red value (0-255) representing the color's red intensity
         */
        u32 getRed() const;

        /** @brief Gets the green component of the color.
         *  @return The green value (0-255) representing the color's green intensity
         */
        u32 getGreen() const;

        /** @brief Gets the blue component of the color.
         *  @return The blue value (0-255) representing the color's blue intensity
         */
        u32 getBlue() const;

        /** @brief Calculates the luminance of the color.
         *  @return The luminance value as a floating-point number
         *  @note Luminance is calculated using the standard formula: 0.299R + 0.587G + 0.114B
         */
        f32 getLuminance() const;

        /** @brief Calculates the average intensity of the color components.
         *  @return The average of the red, green, and blue components
         */
        u32 getAverage() const;

        /** @brief Sets the alpha component of the color.
         *  @param a The alpha value (0-255) to set
         */
        void setAlpha( u32 a );

        /** @brief Sets the red component of the color.
         *  @param r The red value (0-255) to set
         */
        void setRed( u32 r );

        /** @brief Sets the green component of the color.
         *  @param g The green value (0-255) to set
         */
        void setGreen( u32 g );

        /** @brief Sets the blue component of the color.
         *  @param b The blue value (0-255) to set
         */
        void setBlue( u32 b );

        /** @brief Converts the color to A1R5G5B5 format.
         *  @return A 16-bit color value in A1R5G5B5 format
         *  @note This format uses 1 bit for alpha and 5 bits each for red, green, and blue
         */
        u16 toA1R5G5B5() const;

        /** @brief Converts the color to OpenGL color format.
         *  @param dest Pointer to the destination buffer where the RGBA color will be stored
         *  @note Converts from ARGB to RGBA format for OpenGL compatibility
         */
        void toOpenGLColor( u8 *dest ) const;

        /** @brief Sets all color components at once.
         *  @param a Alpha component (0-255)
         *  @param r Red component (0-255)
         *  @param g Green component (0-255)
         *  @param b Blue component (0-255)
         */
        void set( u32 a, u32 r, u32 g, u32 b );

        /** @brief Sets the color using a single value.
         *  @param col Color value in A8R8G8B8 format
         */
        void set( u32 col );

        /** @brief Equality comparison operator.
         *  @param other The color to compare with
         *  @return true if the colors are identical, false otherwise
         */
        bool operator==( const ColourI &other ) const;

        /** @brief Inequality comparison operator.
         *  @param other The color to compare with
         *  @return true if the colors are different, false otherwise
         */
        bool operator!=( const ColourI &other ) const;

        /** @brief Addition operator for color blending.
         *  @param other The color to add
         *  @return A new color representing the sum of both colors
         *  @note Components are clamped to their maximum values
         */
        ColourI operator+( const ColourI &other ) const;

        /** @brief Linear interpolation between two colors.
         *  @param other The target color
         *  @param d Interpolation factor (0.0 to 1.0)
         *  @return The interpolated color
         */
        ColourI getInterpolated( const ColourI &other, f32 d ) const;

        /** @brief Quadratic interpolation between three colors.
         *  @param c1 First color to interpolate with
         *  @param c2 Second color to interpolate with
         *  @param d Interpolation factor (0.0 to 1.0)
         *  @return The quadratically interpolated color
         */
        ColourI getInterpolated_quadratic( const ColourI &c1, const ColourI &c2, f32 d ) const;

    private:
        /** @brief The color value stored in A8R8G8B8 format */
        u32 m_color = 0;
    };
}  // namespace workphone

#endif  // ColourI_h__
