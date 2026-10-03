#ifndef ColourF_h__
#define ColourF_h__

#include <Workphone/Core/ColourI.hpp>

namespace workphone
{
    /** @brief Type aliases for different color formats */
    using RGBA = u32;  ///< Red-Green-Blue-Alpha format
    using ARGB = u32;  ///< Alpha-Red-Green-Blue format
    using ABGR = u32;  ///< Alpha-Blue-Green-Red format
    using BGRA = u32;  ///< Blue-Green-Red-Alpha format

    /** @brief A class representing a color with four floating-point components (RGBA).
     *
     * This class provides functionality for color manipulation, conversion, and interpolation.
     * All color components (r, g, b, a) are stored as floating-point values in the range [0.0, 1.0].
     * The class supports various color format conversions and provides common color constants.
     */
    class WPCore_API ColourF
    {
    public:
        /** @brief Default constructor.
         *
         * Initializes all color components to 0.0f, resulting in a black color.
         */
        ColourF();

        /** @brief Constructs a color from RGB components.
         *
         * @param r Red component in range [0.0f, 1.0f]
         * @param g Green component in range [0.0f, 1.0f]
         * @param b Blue component in range [0.0f, 1.0f]
         * @note Alpha is set to 1.0f (fully opaque)
         */
        ColourF( f32 r, f32 g, f32 b );

        /** @brief Constructs a color from RGBA components.
         *
         * @param r Red component in range [0.0f, 1.0f]
         * @param g Green component in range [0.0f, 1.0f]
         * @param b Blue component in range [0.0f, 1.0f]
         * @param a Alpha component in range [0.0f, 1.0f] (0.0f = opaque, 1.0f = transparent)
         */
        ColourF( f32 r, f32 g, f32 b, f32 a );

        /** @brief Constructs a color from a 32-bit integer color.
         *
         * @param c The source color in ColourI format
         */
        explicit ColourF( ColourI c );

        /** @brief Destructor */
        ~ColourF();

        /** @brief Converts this color to a 32-bit integer color format.
         *
         * @return The color converted to ColourI format
         */
        ColourI toSColor() const;

        /** @brief Sets the RGB components of the color.
         *
         * @param rr Red component in range [0.0f, 1.0f]
         * @param gg Green component in range [0.0f, 1.0f]
         * @param bb Blue component in range [0.0f, 1.0f]
         * @note Alpha component remains unchanged
         */
        void set( f32 rr, f32 gg, f32 bb );

        /** @brief Sets all RGBA components of the color.
         *
         * @param aa Alpha component in range [0.0f, 1.0f]
         * @param rr Red component in range [0.0f, 1.0f]
         * @param gg Green component in range [0.0f, 1.0f]
         * @param bb Blue component in range [0.0f, 1.0f]
         */
        void set( f32 aa, f32 rr, f32 gg, f32 bb );

        /** @brief Performs linear interpolation between this color and another color.
         *
         * @param other The target color to interpolate to
         * @param d Interpolation factor in range [0.0f, 1.0f]
         * @return The interpolated color
         */
        ColourF getInterpolated( const ColourF &other, f32 d ) const;

        /** @brief Performs quadratic interpolation between this color and two other colors.
         *
         * @param c1 First color to interpolate with
         * @param c2 Second color to interpolate with
         * @param d Interpolation factor in range [0.0f, 1.0f]
         * @return The quadratically interpolated color
         */
        ColourF getInterpolated_quadratic( const ColourF &c1, const ColourF &c2, f32 d ) const;

        /** @brief Sets a specific color component by index.
         *
         * @param index Component index (0=R, 1=G, 2=B, 3=A)
         * @param value New value for the component in range [0.0f, 1.0f]
         */
        void setColorComponentValue( s32 index, f32 value );

        /** @brief Gets the color as a 32-bit RGBA value.
         * @return Color packed as RGBA
         */
        RGBA getAsRGBA() const;

        /** @brief Gets the color as a 32-bit ARGB value.
         * @return Color packed as ARGB
         */
        ARGB getAsARGB() const;

        /** @brief Gets the color as a 32-bit BGRA value.
         * @return Color packed as BGRA
         */
        BGRA getAsBGRA() const;

        /** @brief Gets the color as a 32-bit ABGR value.
         * @return Color packed as ABGR
         */
        ABGR getAsABGR() const;

        /** @brief Gets the color as a 32-bit RGBA value (byte format).
         * @return Color packed as RGBA bytes
         */
        RGBA getAsBYTE() const;

        /** @brief Sets the color from a 32-bit RGBA value.
         * @param value Color value in RGBA format
         */
        void setAsRGBA( RGBA rgba );

        /** @brief Sets the color from a 32-bit ARGB value.
         * @param value Color value in ARGB format
         */
        void setAsARGB( ARGB argb );

        /** @brief Sets the color from a 32-bit BGRA value.
         * @param value Color value in BGRA format
         */
        void setAsBGRA( BGRA bgra );

        /** @brief Sets the color from a 32-bit ABGR value.
         * @param value Color value in ABGR format
         */
        void setAsABGR( ABGR abgr );

        /** @brief Validates the color components.
         *
         * Checks if all color components are within valid range [0.0f, 1.0f].
         * @return true if all components are valid, false otherwise
         */
        bool isValid() const;

        /** @brief Multiplies all color components by a scalar value.
         * @param fScalar The scalar multiplier
         * @return The resulting color
         */
        ColourF operator*( f32 fScalar ) const;

        /** @brief Multiplies all color components by a scalar value in-place.
         * @param fScalar The scalar multiplier
         * @return Reference to this color
         */
        ColourF &operator*=( f32 fScalar );

        /** @brief Equality comparison operator.
         * @param other The color to compare with
         * @return true if colors are equal, false otherwise
         */
        bool operator==( const ColourF &other ) const;

        /** @brief Inequality comparison operator.
         * @param other The color to compare with
         * @return true if colors are not equal, false otherwise
         */
        bool operator!=( const ColourF &other ) const;

        /** @brief Predefined color constants */
        static const ColourF ZERO;   ///< Color with all components set to 0.0f
        static const ColourF Black;  ///< Black color (0,0,0,1)
        static const ColourF White;  ///< White color (1,1,1,1)
        static const ColourF Red;    ///< Red color (1,0,0,1)
        static const ColourF Green;  ///< Green color (0,1,0,1)
        static const ColourF Blue;   ///< Blue color (0,0,1,1)

        /** @brief Red component of the color [0.0f, 1.0f] */
        f32 r = 0.0f;

        /** @brief Green component of the color [0.0f, 1.0f] */
        f32 g = 0.0f;

        /** @brief Blue component of the color [0.0f, 1.0f] */
        f32 b = 0.0f;

        /** @brief Alpha component of the color [0.0f, 1.0f] (0.0f = opaque, 1.0f = transparent) */
        f32 a = 0.0f;
    };
}  // namespace workphone

#endif  // ColourF_h__
