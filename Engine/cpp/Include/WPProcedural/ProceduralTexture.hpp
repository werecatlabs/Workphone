// ProceduralTexture.hpp
#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/Properties.hpp>
#include <vector>
#include <memory>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        // ---------------------------------------------------------------

        /**
         * @class Color4
         * @brief Floating-point RGBA colour with channels nominally in [0, 1].
         */
        struct WPProcedural_API Color4
        {
            float r = 1.0f;  ///< Red channel.
            float g = 1.0f;  ///< Green channel.
            float b = 1.0f;  ///< Blue channel.
            float a = 1.0f;  ///< Alpha channel (1 = fully opaque).

            /** Clamp all channels to [0, 1] and return the result. */
            Color4 clamped() const;

            /** Linear interpolation towards @p other by factor @p t (unclamped). */
            Color4 lerp( const Color4 &other, float t ) const;

            bool operator==( const Color4 &rhs ) const;
            bool operator!=( const Color4 &rhs ) const;
        };

        // ---------------------------------------------------------------

        /**
         * @class Texture2D
         * @brief CPU-side flat RGBA texture filled by TextureBaker.
         *
         * Pixels are stored in row-major order: pixel(x, y) lives at
         * index y * width + x.  The at() accessor validates bounds and
         * returns a safe reference to a static black pixel on failure.
         */
        struct WPProcedural_API Texture2D
        {
            int width = 0;               ///< Horizontal resolution in pixels.
            int height = 0;              ///< Vertical resolution in pixels.
            std::vector<Color4> pixels;  ///< Flat row-major pixel store.

            /**
             * @brief Bounds-checked pixel accessor.
             * @return Reference to pixel(x, y), or a static fallback on out-of-range.
             */
            Color4 &at( int x, int y );

            /** @overload Const version. */
            const Color4 &at( int x, int y ) const;

            /** @return true when the texture has a valid non-zero size. */
            bool isValid() const;

            SmartPtr<Properties> getProperties() const;
        };

        // ---------------------------------------------------------------

        /**
         * @class TextureRule
         * @brief Abstract base for all procedural texture sampling rules.
         *
         * Each concrete subclass samples a colour at normalised UV coordinates
         * and exposes its parameters through getProperties() / setProperties()
         * for game-editor access.
         */
        class WPProcedural_API TextureRule
        {
        public:
            virtual ~TextureRule() = default;

            /**
             * @brief Sample the rule at normalised texture coordinates.
             * @param u Horizontal coordinate in [0, 1].
             * @param v Vertical coordinate in [0, 1].
             * @return Sampled RGBA colour.
             */
            virtual Color4 sample( float u, float v ) const = 0;

            /** Return all editable parameters as a Properties object. */
            virtual SmartPtr<Properties> getProperties() const;

            /** Apply values from @p properties to this rule's parameters. */
            virtual void setProperties( SmartPtr<Properties> properties );
        };

        // ---------------------------------------------------------------

        /**
         * @class SolidColorRule
         * @brief Returns the same colour for every UV coordinate.
         */
        class WPProcedural_API SolidColorRule final : public TextureRule
        {
        public:
            Color4 color;  ///< The constant output colour.

            explicit SolidColorRule( Color4 c = {} );

            Color4 sample( float u, float v ) const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
        };

        // ---------------------------------------------------------------

        /**
         * @class CheckerRule
         * @brief Alternating two-colour checkerboard pattern.
         */
        class WPProcedural_API CheckerRule final : public TextureRule
        {
        public:
            Color4 a;            ///< First tile colour.
            Color4 b;            ///< Second tile colour.
            float scale = 8.0f;  ///< Number of checker tiles across [0, 1].  Must be > 0.

            CheckerRule( Color4 a_, Color4 b_, float scale_ );
            CheckerRule() = default;

            Color4 sample( float u, float v ) const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
        };

        // ---------------------------------------------------------------

        /**
         * @class StripeRule
         * @brief Horizontal stripe pattern repeating at a given frequency.
         */
        class WPProcedural_API StripeRule final : public TextureRule
        {
        public:
            Color4 base;              ///< Background colour.
            Color4 stripe;            ///< Stripe colour.
            float frequency = 10.0f;  ///< Number of stripe periods across [0, 1].  Must be > 0.
            float thickness = 0.1f;   ///< Fractional width of the stripe within one period (0–1).

            StripeRule( Color4 base_, Color4 stripe_, float frequency_, float thickness_ );
            StripeRule() = default;

            Color4 sample( float u, float v ) const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
        };

        // ---------------------------------------------------------------

        /**
         * @class BlendRule
         * @brief Linearly blends two child rules by a constant factor.
         */
        class WPProcedural_API BlendRule final : public TextureRule
        {
        public:
            std::shared_ptr<TextureRule> a;  ///< First input rule (must not be null at sample time).
            std::shared_ptr<TextureRule> b;  ///< Second input rule (must not be null at sample time).
            float amount = 0.5f;             ///< Blend weight towards b (clamped to [0, 1]).

            BlendRule( std::shared_ptr<TextureRule> a_, std::shared_ptr<TextureRule> b_, float amount_ );
            BlendRule() = default;

            Color4 sample( float u, float v ) const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
        };

        // ---------------------------------------------------------------

        /**
         * @class GradientRule
         * @brief Produces a linear gradient between two colours along the U or V axis.
         */
        class WPProcedural_API GradientRule final : public TextureRule
        {
        public:
            Color4 colorStart;       ///< Colour at u/v = 0.
            Color4 colorEnd;         ///< Colour at u/v = 1.
            bool horizontal = true;  ///< true = gradient along U, false = along V.

            GradientRule( Color4 start, Color4 end, bool horizontal_ = true );
            GradientRule() = default;

            Color4 sample( float u, float v ) const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
        };

        // ---------------------------------------------------------------

        /**
         * @class NoiseRule
         * @brief Value-noise greyscale pattern useful for roughness or height maps.
         *
         * Implements a simple, deterministic hash-based value noise.
         * The output range is [0, 1] mapped onto all four RGBA channels
         * (with alpha fixed at 1).
         */
        class WPProcedural_API NoiseRule final : public TextureRule
        {
        public:
            float scale = 4.0f;      ///< Frequency of the noise pattern.  Must be > 0.
            float amplitude = 1.0f;  ///< Output amplitude multiplier clamped to [0, 1].

            NoiseRule( float scale_, float amplitude_ = 1.0f );
            NoiseRule() = default;

            Color4 sample( float u, float v ) const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
        };

        // ---------------------------------------------------------------

        /**
         * @class TextureBaker
         * @brief Rasterises a TextureRule tree into a Texture2D.
         */
        class WPProcedural_API TextureBaker
        {
        public:
            /**
             * @brief Bake @p rule into a new Texture2D of the given resolution.
             * @param rule    The root sampling rule.
             * @param width   Horizontal resolution (must be >= 1).
             * @param height  Vertical resolution (must be >= 1).
             * @return A fully populated Texture2D, or an empty one on failure.
             */
            static Texture2D bake( const TextureRule &rule, int width, int height );
        };

    }  // namespace procedural
}  // namespace workphone
