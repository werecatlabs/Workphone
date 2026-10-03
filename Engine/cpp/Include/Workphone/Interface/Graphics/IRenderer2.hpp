#ifndef __IRenderer2_h__
#define __IRenderer2_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IRenderer.hpp>
#include <Workphone/Interface/Graphics/ISprite.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Math/AABB2.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Interface for 2D rendering.
         *
         * Extends IRenderer with 2D-specific capabilities including scissor rectangles,
         * sprite rendering with UV source regions, and 2D primitive drawing.
         */
        class WPCore_API IRenderer2 : public IRenderer
        {
        public:
            /** Virtual destructor. */
            ~IRenderer2() override;

            /**
             * Renders a texture as a sprite using a UV source rectangle.
             *
             * @param renderData    A shared object containing additional rendering data.
             * @param texture       The texture to use for rendering.
             * @param transform     The transformation matrix for the sprite.
             * @param colour        The colour to apply to the sprite.
             * @param srcRect       The source rectangle (UV region) within the texture, in
             *                      normalised [0,1] coordinates.
             */
            virtual void render( const SmartPtr<ISharedObject> &renderData,
                                 const SmartPtr<ITexture> &texture, const Matrix4F &transform,
                                 const ColourF &colour, const AABB2<real_Num> &srcRect ) = 0;

            /**
             * Renders a sprite object.
             *
             * @param renderData    A shared object containing additional rendering data.
             * @param sprite        The sprite to render.
             * @param colour        The colour to apply to the sprite.
             */
            virtual void render( const SmartPtr<ISharedObject> &renderData,
                                 const SmartPtr<ISprite> &sprite, const ColourF &colour ) = 0;

            /**
             * Sets the scissor rectangle, restricting rendering to the given screen-space region.
             *
             * @param rect  The scissor rectangle in screen-space pixel coordinates.
             */
            virtual void setScissorRect( const AABB2<real_Num> &rect ) = 0;

            /**
             * Clears the active scissor rectangle, restoring full render target coverage.
             */
            virtual void clearScissorRect() = 0;

            /**
             * Draws a 2D line segment.
             *
             * @param start     The start position in screen space.
             * @param end       The end position in screen space.
             * @param colour    The colour of the line.
             */
            virtual void drawLine( const Vector2<real_Num> &start, const Vector2<real_Num> &end,
                                   const ColourF &colour ) = 0;

            /**
             * Draws the outline of a 2D rectangle.
             *
             * @param rect      The rectangle in screen space.
             * @param colour    The outline colour.
             */
            virtual void drawRect( const AABB2<real_Num> &rect, const ColourF &colour ) = 0;

            /**
             * Draws a filled 2D rectangle.
             *
             * @param rect      The rectangle in screen space.
             * @param colour    The fill colour.
             */
            virtual void drawFilledRect( const AABB2<real_Num> &rect, const ColourF &colour ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // __IRenderer2_h__
