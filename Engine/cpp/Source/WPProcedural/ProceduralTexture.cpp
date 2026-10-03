#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/ProceduralTexture.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace workphone
{
    namespace procedural
    {
        // ================================================================
        // Internal helpers
        // ================================================================

        namespace
        {
            /// Safe fmod that always returns a value in [0, 1) even for
            /// negative inputs, and guards against zero divisor.
            inline float safeFrac( float value, float divisor )
            {
                if( divisor <= 0.0f )
                {
                    return 0.0f;
                }

                float r = std::fmod( value, divisor );
                if( r < 0.0f )
                {
                    r += divisor;
                }

                return r / divisor;
            }

            /// Write a Color4's RGBA channels into a Properties object under a
            /// named prefix (e.g. prefix="color" ? "colorR", "colorG" …).
            void colorToProperties( SmartPtr<Properties> &props, const char *prefix, const Color4 &c )
            {
                props->setProperty( std::string( prefix ) + "R", c.r );
                props->setProperty( std::string( prefix ) + "G", c.g );
                props->setProperty( std::string( prefix ) + "B", c.b );
                props->setProperty( std::string( prefix ) + "A", c.a );
            }

            /// Read a Color4's channels back from a Properties object.
            void colorFromProperties( const SmartPtr<Properties> &props, const char *prefix, Color4 &c )
            {
                props->getPropertyValue( std::string( prefix ) + "R", c.r );
                props->getPropertyValue( std::string( prefix ) + "G", c.g );
                props->getPropertyValue( std::string( prefix ) + "B", c.b );
                props->getPropertyValue( std::string( prefix ) + "A", c.a );
            }
        }  // anonymous namespace

        // ================================================================
        // Color4
        // ================================================================

        Color4 Color4::clamped() const
        {
            return { std::clamp( r, 0.0f, 1.0f ), std::clamp( g, 0.0f, 1.0f ),
                     std::clamp( b, 0.0f, 1.0f ), std::clamp( a, 0.0f, 1.0f ) };
        }

        Color4 Color4::lerp( const Color4 &other, float t ) const
        {
            const float s = 1.0f - t;
            return { r * s + other.r * t, g * s + other.g * t, b * s + other.b * t,
                     a * s + other.a * t };
        }

        bool Color4::operator==( const Color4 &rhs ) const
        {
            return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
        }

        bool Color4::operator!=( const Color4 &rhs ) const
        {
            return !( *this == rhs );
        }

        // ================================================================
        // Texture2D
        // ================================================================

        namespace
        {
            /// A safe fallback pixel returned when an out-of-range access occurs.
            static Color4 s_fallbackPixel{};
        }  // namespace

        Color4 &Texture2D::at( int x, int y )
        {
            if( width <= 0 || height <= 0 || x < 0 || x >= width || y < 0 || y >= height )
            {
                WP_LOG_ERROR( "Texture2D::at - coordinates (" + std::to_string( x ) + ", " +
                              std::to_string( y ) + ") out of range for " + std::to_string( width ) +
                              "x" + std::to_string( height ) + " texture." );
                s_fallbackPixel = {};
                return s_fallbackPixel;
            }

            return pixels[static_cast<size_t>( y ) * static_cast<size_t>( width ) +
                          static_cast<size_t>( x )];
        }

        const Color4 &Texture2D::at( int x, int y ) const
        {
            if( width <= 0 || height <= 0 || x < 0 || x >= width || y < 0 || y >= height )
            {
                WP_LOG_ERROR( "Texture2D::at (const) - coordinates (" + std::to_string( x ) + ", " +
                              std::to_string( y ) + ") out of range for " + std::to_string( width ) +
                              "x" + std::to_string( height ) + " texture." );
                return s_fallbackPixel;
            }

            return pixels[static_cast<size_t>( y ) * static_cast<size_t>( width ) +
                          static_cast<size_t>( x )];
        }

        bool Texture2D::isValid() const
        {
            return width > 0 && height > 0 &&
                   pixels.size() == static_cast<size_t>( width ) * static_cast<size_t>( height );
        }

        SmartPtr<Properties> Texture2D::getProperties() const
        {
            try
            {
                auto props = workphone::make_ptr<Properties>();
                if( !props )
                {
                    WP_LOG_ERROR( "Texture2D::getProperties - allocation failed." );
                    return nullptr;
                }

                props->setProperty( "width", width );
                props->setProperty( "height", height );
                props->setProperty( "pixelCount", static_cast<int>( pixels.size() ) );
                props->setProperty( "isValid", isValid() );

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        // ================================================================
        // TextureRule base
        // ================================================================

        SmartPtr<Properties> TextureRule::getProperties() const
        {
            return workphone::make_ptr<Properties>();
        }

        void TextureRule::setProperties( SmartPtr<Properties> /*properties*/ )
        {
            // Default no-op; derived classes override to apply values.
        }

        // ================================================================
        // SolidColorRule
        // ================================================================

        SolidColorRule::SolidColorRule( Color4 c ) : color( c )
        {
        }

        Color4 SolidColorRule::sample( float /*u*/, float /*v*/ ) const
        {
            return color;
        }

        SmartPtr<Properties> SolidColorRule::getProperties() const
        {
            try
            {
                auto props = TextureRule::getProperties();
                if( !props )
                {
                    WP_LOG_ERROR( "SolidColorRule::getProperties - allocation failed." );
                    return nullptr;
                }

                colorToProperties( props, "color", color );
                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void SolidColorRule::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "SolidColorRule::setProperties - null properties provided." );
                return;
            }

            try
            {
                colorFromProperties( properties, "color", color );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ================================================================
        // CheckerRule
        // ================================================================

        CheckerRule::CheckerRule( Color4 a_, Color4 b_, float scale_ ) :
            a( a_ ),
            b( b_ ),
            scale( scale_ )
        {
        }

        Color4 CheckerRule::sample( float u, float v ) const
        {
            if( scale <= 0.0f )
            {
                WP_LOG_ERROR( "CheckerRule::sample - scale must be > 0; returning color a." );
                return a;
            }

            // Use floor-based parity; cast to unsigned to avoid negative-index
            // ambiguity when u or v are negative.
            const auto xi = static_cast<int>( std::floor( u * scale ) );
            const auto yi = static_cast<int>( std::floor( v * scale ) );

            // XOR the lowest bits for checkerboard parity.
            return ( ( xi ^ yi ) & 1 ) ? a : b;
        }

        SmartPtr<Properties> CheckerRule::getProperties() const
        {
            try
            {
                auto props = TextureRule::getProperties();
                if( !props )
                {
                    WP_LOG_ERROR( "CheckerRule::getProperties - allocation failed." );
                    return nullptr;
                }

                colorToProperties( props, "colorA", a );
                colorToProperties( props, "colorB", b );
                props->setProperty( "scale", scale );

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CheckerRule::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "CheckerRule::setProperties - null properties provided." );
                return;
            }

            try
            {
                auto s = scale;
                colorFromProperties( properties, "colorA", a );
                colorFromProperties( properties, "colorB", b );
                properties->getPropertyValue( "scale", s );
                scale = std::max( 1e-6f, s );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ================================================================
        // StripeRule
        // ================================================================

        StripeRule::StripeRule( Color4 base_, Color4 stripe_, float frequency_, float thickness_ ) :
            base( base_ ),
            stripe( stripe_ ),
            frequency( frequency_ ),
            thickness( thickness_ )
        {
        }

        Color4 StripeRule::sample( float u, float /*v*/ ) const
        {
            if( frequency <= 0.0f )
            {
                WP_LOG_ERROR( "StripeRule::sample - frequency must be > 0; returning base." );
                return base;
            }

            const float t = std::clamp( thickness, 0.0f, 1.0f );
            const float phase = safeFrac( u * frequency, 1.0f );

            return phase < t ? stripe : base;
        }

        SmartPtr<Properties> StripeRule::getProperties() const
        {
            try
            {
                auto props = TextureRule::getProperties();
                if( !props )
                {
                    WP_LOG_ERROR( "StripeRule::getProperties - allocation failed." );
                    return nullptr;
                }

                colorToProperties( props, "base", base );
                colorToProperties( props, "stripe", stripe );
                props->setProperty( "frequency", frequency );
                props->setProperty( "thickness", thickness );

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void StripeRule::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "StripeRule::setProperties - null properties provided." );
                return;
            }

            try
            {
                auto f = frequency;
                auto t = thickness;

                colorFromProperties( properties, "base", base );
                colorFromProperties( properties, "stripe", stripe );
                properties->getPropertyValue( "frequency", f );
                properties->getPropertyValue( "thickness", t );

                frequency = std::max( 1e-6f, f );
                thickness = std::clamp( t, 0.0f, 1.0f );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ================================================================
        // BlendRule
        // ================================================================

        BlendRule::BlendRule( std::shared_ptr<TextureRule> a_, std::shared_ptr<TextureRule> b_,
                              float amount_ ) :
            a( std::move( a_ ) ),
            b( std::move( b_ ) ),
            amount( amount_ )
        {
            if( !a )
            {
                WP_LOG_ERROR( "BlendRule - input 'a' is null; blend results will be wrong." );
            }

            if( !b )
            {
                WP_LOG_ERROR( "BlendRule - input 'b' is null; blend results will be wrong." );
            }
        }

        Color4 BlendRule::sample( float u, float v ) const
        {
            if( !a && !b )
            {
                WP_LOG_ERROR( "BlendRule::sample - both inputs are null; returning black." );
                return {};
            }

            const float t = std::clamp( amount, 0.0f, 1.0f );

            if( !a )
            {
                WP_LOG_ERROR( "BlendRule::sample - input 'a' is null; returning b." );
                return b->sample( u, v );
            }

            if( !b )
            {
                WP_LOG_ERROR( "BlendRule::sample - input 'b' is null; returning a." );
                return a->sample( u, v );
            }

            return a->sample( u, v ).lerp( b->sample( u, v ), t );
        }

        SmartPtr<Properties> BlendRule::getProperties() const
        {
            try
            {
                auto props = TextureRule::getProperties();
                if( !props )
                {
                    WP_LOG_ERROR( "BlendRule::getProperties - allocation failed." );
                    return nullptr;
                }

                props->setProperty( "amount", amount );
                props->setProperty( "hasA", ( a != nullptr ) );
                props->setProperty( "hasB", ( b != nullptr ) );

                if( a )
                {
                    auto aProps = a->getProperties();
                    if( aProps )
                    {
                        aProps->setName( "inputA" );
                        props->addChild( aProps );
                    }
                }

                if( b )
                {
                    auto bProps = b->getProperties();
                    if( bProps )
                    {
                        bProps->setName( "inputB" );
                        props->addChild( bProps );
                    }
                }

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void BlendRule::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "BlendRule::setProperties - null properties provided." );
                return;
            }

            try
            {
                auto amt = amount;
                properties->getPropertyValue( "amount", amt );
                amount = std::clamp( amt, 0.0f, 1.0f );

                if( a )
                {
                    auto aProps = properties->getChild( "inputA" );
                    if( aProps )
                    {
                        a->setProperties( aProps );
                    }
                }

                if( b )
                {
                    auto bProps = properties->getChild( "inputB" );
                    if( bProps )
                    {
                        b->setProperties( bProps );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ================================================================
        // GradientRule
        // ================================================================

        GradientRule::GradientRule( Color4 start, Color4 end, bool horizontal_ ) :
            colorStart( start ),
            colorEnd( end ),
            horizontal( horizontal_ )
        {
        }

        Color4 GradientRule::sample( float u, float v ) const
        {
            const float t = std::clamp( horizontal ? u : v, 0.0f, 1.0f );
            return colorStart.lerp( colorEnd, t );
        }

        SmartPtr<Properties> GradientRule::getProperties() const
        {
            try
            {
                auto props = TextureRule::getProperties();
                if( !props )
                {
                    WP_LOG_ERROR( "GradientRule::getProperties - allocation failed." );
                    return nullptr;
                }

                colorToProperties( props, "colorStart", colorStart );
                colorToProperties( props, "colorEnd", colorEnd );
                props->setProperty( "horizontal", horizontal );

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void GradientRule::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "GradientRule::setProperties - null properties provided." );
                return;
            }

            try
            {
                colorFromProperties( properties, "colorStart", colorStart );
                colorFromProperties( properties, "colorEnd", colorEnd );
                properties->getPropertyValue( "horizontal", horizontal );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ================================================================
        // NoiseRule  (deterministic value noise)
        // ================================================================

        NoiseRule::NoiseRule( float scale_, float amplitude_ ) : scale( scale_ ), amplitude( amplitude_ )
        {
        }

        Color4 NoiseRule::sample( float u, float v ) const
        {
            if( scale <= 0.0f )
            {
                WP_LOG_ERROR( "NoiseRule::sample - scale must be > 0; returning black." );
                return {};
            }

            // A fast, portable integer hash for 2D grid positions.
            auto hashGrid = []( int ix, int iy ) -> float {
                // MurmurHash-inspired mixing.
                uint32_t h = static_cast<uint32_t>( ix ) * 1664525u +
                             static_cast<uint32_t>( iy ) * 22695477u + 1013904223u;
                h ^= ( h >> 16 );
                h *= 0x45d9f3bu;
                h ^= ( h >> 16 );
                return static_cast<float>( h & 0xFFFFu ) / 65535.0f;
            };

            const float su = u * scale;
            const float sv = v * scale;

            const int x0 = static_cast<int>( std::floor( su ) );
            const int y0 = static_cast<int>( std::floor( sv ) );
            const int x1 = x0 + 1;
            const int y1 = y0 + 1;

            const float fx = su - static_cast<float>( x0 );
            const float fy = sv - static_cast<float>( y0 );

            // Smooth-step interpolation (Hermite curve).
            const float sx = fx * fx * ( 3.0f - 2.0f * fx );
            const float sy = fy * fy * ( 3.0f - 2.0f * fy );

            const float v00 = hashGrid( x0, y0 );
            const float v10 = hashGrid( x1, y0 );
            const float v01 = hashGrid( x0, y1 );
            const float v11 = hashGrid( x1, y1 );

            const float top = v00 + ( v10 - v00 ) * sx;
            const float bottom = v01 + ( v11 - v01 ) * sx;
            const float n = std::clamp(
                ( top + ( bottom - top ) * sy ) * std::clamp( amplitude, 0.0f, 1.0f ), 0.0f, 1.0f );

            return { n, n, n, 1.0f };
        }

        SmartPtr<Properties> NoiseRule::getProperties() const
        {
            try
            {
                auto props = TextureRule::getProperties();
                if( !props )
                {
                    WP_LOG_ERROR( "NoiseRule::getProperties - allocation failed." );
                    return nullptr;
                }

                props->setProperty( "scale", scale );
                props->setProperty( "amplitude", amplitude );

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void NoiseRule::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "NoiseRule::setProperties - null properties provided." );
                return;
            }

            try
            {
                auto s = scale;
                auto amp = amplitude;

                properties->getPropertyValue( "scale", s );
                properties->getPropertyValue( "amplitude", amp );

                scale = std::max( 1e-6f, s );
                amplitude = std::clamp( amp, 0.0f, 1.0f );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ================================================================
        // TextureBaker
        // ================================================================

        Texture2D TextureBaker::bake( const TextureRule &rule, int width, int height )
        {
            Texture2D texture;

            if( width <= 0 || height <= 0 )
            {
                WP_LOG_ERROR( "TextureBaker::bake - invalid dimensions " + std::to_string( width ) +
                              "x" + std::to_string( height ) + "; must both be >= 1." );
                return texture;
            }

            try
            {
                texture.width = width;
                texture.height = height;
                texture.pixels.resize( static_cast<size_t>( width ) * static_cast<size_t>( height ) );

                // When width == 1 or height == 1 we cannot divide by (dim - 1)
                // without a zero divisor; clamp to [0, 1] directly in that case.
                const float uScale = ( width > 1 ) ? 1.0f / static_cast<float>( width - 1 ) : 0.0f;
                const float vScale = ( height > 1 ) ? 1.0f / static_cast<float>( height - 1 ) : 0.0f;

                for( int y = 0; y < height; ++y )
                {
                    const float v = ( height > 1 ) ? static_cast<float>( y ) * vScale : 0.5f;

                    for( int x = 0; x < width; ++x )
                    {
                        const float u = ( width > 1 ) ? static_cast<float>( x ) * uScale : 0.5f;

                        try
                        {
                            texture.at( x, y ) = rule.sample( u, v );
                        }
                        catch( std::exception &e )
                        {
                            WP_LOG_EXCEPTION( e );
                            // Leave this pixel at its default (white) and continue.
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                // Return whatever was built so far rather than an uninitialised object.
            }

            return texture;
        }

    }  // namespace procedural
}  // namespace workphone
