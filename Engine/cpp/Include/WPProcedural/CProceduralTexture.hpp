#ifndef CProceduralTexture_h__
#define CProceduralTexture_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralTexture.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    namespace procedural
    {
        struct Color4
        {
            float r = 1, g = 1, b = 1, a = 1;
        };

        struct Texture2D
        {
            int width = 0;
            int height = 0;
            std::vector<Color4> pixels;

            Color4 &at( int x, int y )
            {
                return pixels[y * width + x];
            }
        };

        class TextureRule
        {
        public:
            virtual ~TextureRule() = default;

            virtual Color4 sample( float u, float v ) const = 0;
        };

        class SolidColorRule final : public TextureRule
        {
        public:
            Color4 color;

            explicit SolidColorRule( Color4 c ) : color( c )
            {
            }

            Color4 sample( float, float ) const override
            {
                return color;
            }
        };

        class CheckerRule final : public TextureRule
        {
        public:
            Color4 a;
            Color4 b;
            float scale = 8.0f;

            CheckerRule( Color4 a_, Color4 b_, float scale_ ) : a( a_ ), b( b_ ), scale( scale_ )
            {
            }

            Color4 sample( float u, float v ) const override
            {
                const int x = static_cast<int>( std::floor( u * scale ) );
                const int y = static_cast<int>( std::floor( v * scale ) );

                return ( ( x + y ) & 1 ) ? a : b;
            }
        };

        class StripeRule final : public TextureRule
        {
        public:
            Color4 base;
            Color4 stripe;
            float frequency = 10.0f;
            float thickness = 0.1f;

            StripeRule( Color4 base_, Color4 stripe_, float frequency_, float thickness_ ) :
                base( base_ ),
                stripe( stripe_ ),
                frequency( frequency_ ),
                thickness( thickness_ )
            {
            }

            Color4 sample( float u, float ) const override
            {
                const float x = std::fmod( u * frequency, 1.0f );
                return x < thickness ? stripe : base;
            }
        };

        class BlendRule final : public TextureRule
        {
        public:
            std::shared_ptr<TextureRule> a;
            std::shared_ptr<TextureRule> b;
            float amount = 0.5f;

            BlendRule( std::shared_ptr<TextureRule> a_, std::shared_ptr<TextureRule> b_,
                       float amount_ ) :
                a( std::move( a_ ) ),
                b( std::move( b_ ) ),
                amount( amount_ )
            {
            }

            Color4 sample( float u, float v ) const override
            {
                const Color4 ca = a->sample( u, v );
                const Color4 cb = b->sample( u, v );

                const float t = std::clamp( amount, 0.0f, 1.0f );

                return { ca.r * ( 1.0f - t ) + cb.r * t, ca.g * ( 1.0f - t ) + cb.g * t,
                         ca.b * ( 1.0f - t ) + cb.b * t, ca.a * ( 1.0f - t ) + cb.a * t };
            }
        };

        class TextureBaker
        {
        public:
            static Texture2D bake( const TextureRule &rule, int width, int height )
            {
                Texture2D texture;
                texture.width = width;
                texture.height = height;
                texture.pixels.resize( width * height );

                for( int y = 0; y < height; ++y )
                {
                    for( int x = 0; x < width; ++x )
                    {
                        const float u = static_cast<float>( x ) / static_cast<float>( width - 1 );
                        const float v = static_cast<float>( y ) / static_cast<float>( height - 1 );

                        texture.at( x, y ) = rule.sample( u, v );
                    }
                }

                return texture;
            }
        };

        class WPProcedural_API CProceduralTexture : public IProceduralTexture
        {
        public:
            CProceduralTexture();

            ~CProceduralTexture();

            void generate() override;

            void setSize( int width, int height ) override;

            int getWidth() const override;

            int getHeight() const override;

            void setData( const Array<ColourF> &data ) override;

            Array<ColourF> getData() const override;

        protected:
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // CProceduralTexture_h__
