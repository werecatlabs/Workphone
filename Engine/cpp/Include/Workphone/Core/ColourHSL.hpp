#ifndef ColourHSL_h__
#define ColourHSL_h__

#include <Workphone/Core/ColourI.hpp>
#include <Workphone/Math/Math.hpp>

namespace workphone
{
    /**
     * @brief Class representing a color in the HSL color space.
     * @tparam T Type of the HSL components. Usually a floating-point type.
     */
    template <class T>
    class ColourHSL
    {
    public:
        /**
         * @brief Default constructor. Initializes all components to zero.
         */
        ColourHSL();

        /**
         * @brief Constructs a color from hue, saturation and luminance.
         * @param h Hue component of the color, in the range [0, 360).
         * @param s Saturation component of the color, in the range [0, 1].
         * @param l Luminance component of the color, in the range [0, 1].
         */
        ColourHSL( T h, T s, T l );

        /**
         * @brief Default destructor.
         */
        ~ColourHSL();

        /**
         * @brief Sets the color from an RGB color.
         * @param color The RGB color to set the HSL color from.
         */
        void setfromRGB( const ColourI &color );

        /**
         * @brief Sets an RGB color from this HSL color.
         * @param color The RGB color to set from this HSL color.
         */
        void settoRGB( ColourI &color ) const;

        /**
         * @brief Gets the hue component of the color.
         * @return The hue component of the color, in the range [0, 360).
         */
        T getHue() const;

        /**
         * @brief Sets the hue component of the color.
         * @param hue The hue component of the color, in the range [0, 360).
         */
        void setHue( T hue );

        /**
         * @brief Gets the saturation component of the color.
         * @return The saturation component of the color, in the range [0, 1].
         */
        T getSaturation() const;

        /**
         * @brief Sets the saturation component of the color.
         * @param saturation The saturation component of the color, in the range [0, 1].
         */
        void setSaturation( T saturation );

        /**
         * @brief Gets the luminance component of the color.
         * @return The luminance component of the color, in the range [0, 1].
         */
        T getLuminance() const;

        /**
         * @brief Sets the luminance component of the color.
         * @param luminance The luminance component of the color, in the range [0, 1].
         */
        void setLuminance( T luminance );

        /** Returns true as there are no known conditions that would make the ColourHSL object invalid.
         */
        bool isValid() const;

    private:
        /**
         * @brief Converts a value from the HSL color space to the RGB color space.
         * @param rm1 The first component of the RGB color.
         * @param rm2 The second component of the RGB color.
         * @param rh The hue component of the color.
         * @return The value converted to the RGB color space.
         */
        u32 toRGB1( T rm1, T rm2, T rh ) const;

        T m_hue = T( 0.0 );         //!< Hue component of the color.
        T m_saturation = T( 0.0 );  //!< Saturation component of the color.
        T m_luminance = T( 0.0 );   //!< Luminance component of the color.
    };

    template <class T>
    ColourHSL<T>::ColourHSL() = default;

    template <class T>
    ColourHSL<T>::ColourHSL( T h, T s, T l ) : m_hue( h ), m_saturation( s ), m_luminance( l )
    {
    }

    template <class T>
    ColourHSL<T>::~ColourHSL() = default;

    template <class T>
    u32 ColourHSL<T>::toRGB1( T rm1, T rm2, T rh ) const
    {
        while( rh > T( 2.0 ) * Math<T>::pi() )
        {
            rh -= T( 2.0 ) * Math<T>::pi();
        }

        while( rh < T( 0.0 ) )
        {
            rh += T( 2.0 ) * Math<T>::pi();
        }

        if( rh < T( 60.0 ) * Math<T>::deg_to_rad() )
        {
            rm1 = rm1 + ( rm2 - rm1 ) * rh / ( T( 60.0 ) * Math<T>::deg_to_rad() );
        }
        else if( rh < T( 180.0 ) * Math<T>::deg_to_rad() )
        {
            rm1 = rm2;
        }
        else if( rh < T( 240.0 ) * Math<T>::deg_to_rad() )
        {
            rm1 = rm1 + ( rm2 - rm1 ) * ( ( T( 240.0 ) * Math<T>::deg_to_rad() ) - rh ) /
                            ( T( 60.0 ) * Math<T>::deg_to_rad() );
        }

        return static_cast<u32>( rm1 * T( 255.0 ) );
    }

    template <class T>
    void ColourHSL<T>::setfromRGB( const ColourI &color )
    {
        T r = static_cast<T>( color.getRed() ) / T( 255.0 );
        T g = static_cast<T>( color.getGreen() ) / T( 255.0 );
        T b = static_cast<T>( color.getBlue() ) / T( 255.0 );

        T maxC = Math<T>::max( r, Math<T>::max( g, b ) );
        T minC = Math<T>::min( r, Math<T>::min( g, b ) );
        T delta = maxC - minC;

        m_luminance = ( maxC + minC ) / T( 2.0 );

        if( delta == T( 0.0 ) )
        {
            m_hue = T( 0.0 );
            m_saturation = T( 0.0 );
        }
        else
        {
            if( m_luminance < T( 0.5 ) )
            {
                m_saturation = delta / ( T( 1.0 ) - 2.0 * m_luminance );
            }
            else
            {
                m_saturation = delta / ( T( 2.0 ) * m_luminance - T( 1.0 ) );
            }

            if( maxC == r )
            {
                m_hue = ( g - b ) / delta;
                if( g < b )
                {
                    m_hue += T( 6.0 );
                }
            }
            else if( maxC == g )
            {
                m_hue = ( b - r ) / delta + T( 2.0 );
            }
            else
            {
                m_hue = ( r - g ) / delta + T( 4.0 );
            }

            m_hue *= T( 60.0 );
            m_hue *= Math<T>::deg_to_rad();
        }
    }

    template <class T>
    void ColourHSL<T>::settoRGB( ColourI &color ) const
    {
        if( m_saturation == T( 0.0 ) )  // grey
        {
            auto c = static_cast<u8>( m_luminance * T( 255.0 ) );
            color.setRed( c );
            color.setGreen( c );
            color.setBlue( c );
            return;
        }

        T rm1, rm2;

        if( m_luminance <= T( 0.5 ) )
        {
            rm2 = m_luminance + m_luminance * m_saturation;
        }
        else
        {
            rm2 = m_luminance + m_saturation - m_luminance * m_saturation;
        }

        rm1 = T( 2.0 ) * m_luminance - rm2;

        color.setRed( toRGB1( rm1, rm2, m_hue + ( T( 120.0 ) * Math<T>::deg_to_rad() ) ) );
        color.setGreen( toRGB1( rm1, rm2, m_hue ) );
        color.setBlue( toRGB1( rm1, rm2, m_hue - ( T( 120.0 ) * Math<T>::deg_to_rad() ) ) );
    }

    template <class T>
    T ColourHSL<T>::getHue() const
    {
        return m_hue;
    }

    template <class T>
    void ColourHSL<T>::setHue( T hue )
    {
        m_hue = hue;
    }

    template <class T>
    T ColourHSL<T>::getSaturation() const
    {
        return m_saturation;
    }

    template <class T>
    void ColourHSL<T>::setSaturation( T saturation )
    {
        m_saturation = saturation;
    }

    template <class T>
    T ColourHSL<T>::getLuminance() const
    {
        return m_luminance;
    }

    template <class T>
    void ColourHSL<T>::setLuminance( T luminance )
    {
        m_luminance = luminance;
    }

    template <class T>
    bool ColourHSL<T>::isValid() const
    {
        return true;
    }

}  // namespace workphone

#endif  // ColourHSL_h__
