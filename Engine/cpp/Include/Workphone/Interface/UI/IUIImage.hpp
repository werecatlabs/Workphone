#ifndef _IUIImage_H
#define _IUIImage_H

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @brief Interface for a UI image element.
         *
         * This interface represents an image element that can be used in the
         * Workphone UI system. Implementations provide accessors and mutators
         * for the image's texture, sprite size, nine-slice borders and
         * tiling behaviour.
         */
        class WPCore_API IUIImage : public IUIElement
        {
        public:
            /**
             * @name Property keys
             * Strings used as property keys by implementations. They are
             * declared here and defined in the corresponding .cpp file.
             */
            //@{
            static const String borderLeftStr;   /**< Key for left border value. */
            static const String borderRightStr;  /**< Key for right border value. */
            static const String borderTopStr;    /**< Key for top border value. */
            static const String borderBottomStr; /**< Key for bottom border value. */
            static const String useTilingStr;    /**< Key for use-tiling flag. */
            static const String spriteSizeStr;   /**< Key for sprite size value. */
            static const String materialStr;     /**< Key for material identifier. */
            static const String textureStr;      /**< Key for texture reference. */
            static const String useNineSliceStr;
            static const String tileScaleXStr;
            static const String tileScaleYStr;
            static const String referenceWidthStr;
            static const String referenceHeightStr;
            static const String colourStr; /**< Key for the element tint colour. */
            //@}

            IUIImage();

            IUIImage( u32 poolTypeId );

            /**
             * @brief Virtual destructor.
             *
             * Ensure derived classes are destructed correctly when deleted
             * through a pointer to IUIImage.
             */
            ~IUIImage() override;

            /**
             * @brief Set the texture used by this image.
             *
             * @param texture Smart pointer to the texture to assign. A null
             * pointer indicates no texture.
             */
            virtual void setTexture( SmartPtr<render::ITexture> texture ) = 0;

            /**
             * @brief Get the texture currently assigned to the image.
             *
             * @return Smart pointer to the current texture (may be null).
             */
            virtual SmartPtr<render::ITexture> getTexture() const = 0;

            /**
             * @brief Get the sprite size in pixels.
             *
             * Sprite size is used by renderers to determine the base size of
             * the sprite within the texture atlas or source image.
             *
             * @return The sprite size as a Vector2I (width, height).
             */
            virtual Vector2I getSpriteSize() const = 0;

            /**
             * @brief Set the sprite size in pixels.
             *
             * @param spriteSize Width and height of the sprite in pixels.
             */
            virtual void setSpriteSize( const Vector2I &spriteSize ) = 0;

            /**
             * @brief Get the left border used for nine-slice scaling.
             *
             * @return The left border size in pixels or normalized units as
             * defined by the implementation.
             */
            virtual f32 getBorderLeft() const = 0;

            /**
             * @brief Set the left border used for nine-slice scaling.
             *
             * @param borderLeft The left border size.
             */
            virtual void setBorderLeft( f32 borderLeft ) = 0;

            /**
             * @brief Get the right border used for nine-slice scaling.
             *
             * @return The right border size.
             */
            virtual f32 getBorderRight() const = 0;

            /**
             * @brief Set the right border used for nine-slice scaling.
             *
             * @param borderRight The right border size.
             */
            virtual void setBorderRight( f32 borderRight ) = 0;

            /**
             * @brief Get the top border used for nine-slice scaling.
             *
             * @return The top border size.
             */
            virtual f32 getBorderTop() const = 0;

            /**
             * @brief Set the top border used for nine-slice scaling.
             *
             * @param borderTop The top border size.
             */
            virtual void setBorderTop( f32 borderTop ) = 0;

            /**
             * @brief Get the bottom border used for nine-slice scaling.
             *
             * @return The bottom border size.
             */
            virtual f32 getBorderBottom() const = 0;

            /**
             * @brief Set the bottom border used for nine-slice scaling.
             *
             * @param borderBottom The bottom border size.
             */
            virtual void setBorderBottom( f32 borderBottom ) = 0;

            /**
             * @brief Query whether the image uses texture tiling.
             *
             * When tiling is enabled, the image's texture will be repeated to
             * fill the element area instead of being stretched.
             *
             * @return True if tiling is enabled; false otherwise.
             */
            virtual bool getUseTiling() const = 0;

            /**
             * @brief Enable or disable texture tiling for the image.
             *
             * @param useTiling True to enable tiling, false to disable.
             */
            virtual void setUseTiling( bool useTiling ) = 0;

            /**
             * @brief Query whether nine-slice rendering is active.
             *
             * When enabled and all four border values are non-zero the image is
             * split into a 3×3 grid of tiles so the corners are never stretched.
             *
             * @return True if nine-slice mode is on.
             */
            virtual bool getUseNineSlice() const = 0;

            /**
             * @brief Enable or disable nine-slice rendering.
             *
             * @param useNineSlice True to use nine-slice, false for normal scaling.
             */
            virtual void setUseNineSlice( bool useNineSlice ) = 0;

            /**
             * @brief Get the horizontal tiling scale.
             *
             * Tile step = bounds.w / tileScaleX.  A value of 1.0 tiles once per
             * element width.  Only used when `getUseTiling()` is true.
             *
             * @return Horizontal tile scale factor.
             */
            virtual f32 getTileScaleX() const = 0;

            /**
             * @brief Set the horizontal tiling scale.
             *
             * @param scale Horizontal tile scale (must be > 0).
             */
            virtual void setTileScaleX( f32 scale ) = 0;

            /**
             * @brief Get the vertical tiling scale.
             *
             * Tile step = bounds.h / tileScaleY.
             *
             * @return Vertical tile scale factor.
             */
            virtual f32 getTileScaleY() const = 0;

            /**
             * @brief Set the vertical tiling scale.
             *
             * @param scale Vertical tile scale (must be > 0).
             */
            virtual void setTileScaleY( f32 scale ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
