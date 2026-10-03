#ifndef __UIImage_h__
#define __UIImage_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @brief UI element that renders an image or sprite.
         *
         * UIImage is a concrete UI element responsible for displaying a texture
         * or material. It supports sprite sizing, configurable borders for
         * 9-slice (scale9) rendering and optional tiling of the texture.
         *
         * This class derives from `UIElement<IUIImage>` and implements the
         * `IUIImage` interface.
         */
        class WPCore_API UIImage : public UIElement<IUIImage>
        {
        public:
            /**
             * @brief Construct a new UIImage.
             *
             * Initializes the image element to default settings.
             */
            UIImage();

            UIImage( u32 poolTypeId );

            /**
             * @brief Destroy the UIImage.
             */
            ~UIImage() override;

            /**
             * @brief Unloads resources used by this object.
             *
             * This method should free or detach any runtime resources held by the
             * image (such as textures, materials or GPU resources) and prepare the
             * object to be safely destroyed or reloaded.
             *
             * @param data Optional data passed to the unload call (context specific).
             *
             * @copydoc IObject::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Set the texture displayed by this image.
             *
             * Setting the texture will update the visual content used by the UI
             * element. The provided `SmartPtr` may be null to clear the current texture.
             *
             * @param texture Smart pointer to a `render::ITexture` to use for rendering.
             *
             * @copydoc IUIImage::setTexture
             */
            void setTexture( SmartPtr<render::ITexture> texture ) override;

            /**
             * @brief Get the texture currently used by this image.
             *
             * @return SmartPtr<render::ITexture> Smart pointer to the current texture
             *         or a null pointer if no texture is set.
             *
             * @copydoc IUIImage::getTexture
             */
            SmartPtr<render::ITexture> getTexture() const override;

            /**
             * @brief Get the sprite size in pixels.
             *
             * The sprite size is used when rendering sprite sheets or when an image
             * represents a sub-region of a larger texture.
             *
             * @return Vector2I Width and height of the sprite in pixels.
             */
            Vector2I getSpriteSize() const override;

            /**
             * @brief Set the sprite size in pixels.
             *
             * Use this when the texture contains a sprite sheet or when explicit
             * sizing of the image region is required.
             *
             * @param spriteSize Width and height of the sprite in pixels.
             */
            void setSpriteSize( const Vector2I &spriteSize ) override;

            /**
             * @brief Get the left border used for 9-slice scaling.
             *
             * The border values are typically expressed in pixels and control the
             * non-scaled margins when the image is stretched.
             *
             * @return f32 Left border thickness.
             */
            f32 getBorderLeft() const override;

            /**
             * @brief Set the left border used for 9-slice scaling.
             *
             * @param borderLeft Left border thickness in pixels.
             */
            void setBorderLeft( f32 borderLeft ) override;

            /**
             * @brief Get the right border used for 9-slice scaling.
             *
             * @return f32 Right border thickness.
             */
            f32 getBorderRight() const override;

            /**
             * @brief Set the right border used for 9-slice scaling.
             *
             * @param borderRight Right border thickness in pixels.
             */
            void setBorderRight( f32 borderRight ) override;

            /**
             * @brief Get the top border used for 9-slice scaling.
             *
             * @return f32 Top border thickness.
             */
            f32 getBorderTop() const override;

            /**
             * @brief Set the top border used for 9-slice scaling.
             *
             * @param borderTop Top border thickness in pixels.
             */
            void setBorderTop( f32 borderTop ) override;

            /**
             * @brief Get the bottom border used for 9-slice scaling.
             *
             * @return f32 Bottom border thickness.
             */
            f32 getBorderBottom() const override;

            /**
             * @brief Set the bottom border used for 9-slice scaling.
             *
             * @param borderBottom Bottom border thickness in pixels.
             */
            void setBorderBottom( f32 borderBottom ) override;

            /**
             * @brief Returns whether the image texture is tiled when rendered.
             *
             * When tiling is enabled, the texture repeats across the element area
             * instead of being stretched.
             *
             * @return true if tiling is enabled, false otherwise.
             *
             * @copydoc IUIImage::getUseTiling
             */
            bool getUseTiling() const override;

            /**
             * @brief Enable or disable texture tiling for this image.
             *
             * @param useTiling true to tile the texture; false to stretch/scale it.
             *
             * @copydoc IUIImage::setUseTiling
             */
            void setUseTiling( bool useTiling ) override;

            /**
             * @brief Query whether nine-slice rendering is active.
             *
             * When enabled and all four border values are non-zero the image is
             * split into a 3×3 grid of tiles so the corners are never stretched.
             *
             * @return True if nine-slice mode is on.
             */
            bool getUseNineSlice() const;

            /**
             * @brief Enable or disable nine-slice rendering.
             *
             * @param useNineSlice True to use nine-slice, false for normal scaling.
             */
            void setUseNineSlice( bool useNineSlice );

            // ---------------------------------------------------------------
            // Tiling scale
            // ---------------------------------------------------------------

            /**
             * @brief Get the horizontal tiling scale.
             *
             * Tile step = bounds.w / tileScaleX.  A value of 1.0 tiles once per
             * element width.  Only used when `getUseTiling()` is true.
             *
             * @return Horizontal tile scale factor.
             */
            f32 getTileScaleX() const;

            /**
             * @brief Set the horizontal tiling scale.
             *
             * @param scale Horizontal tile scale (must be > 0).
             */
            void setTileScaleX( f32 scale );

            /**
             * @brief Get the vertical tiling scale.
             *
             * Tile step = bounds.h / tileScaleY.
             *
             * @return Vertical tile scale factor.
             */
            f32 getTileScaleY() const;

            /**
             * @brief Set the vertical tiling scale.
             *
             * @param scale Vertical tile scale (must be > 0).
             */
            void setTileScaleY( f32 scale );

            /**
             * @brief Set the material used by this image for rendering.
             *
             * A material may provide shaders, blending modes and additional
             * parameters which can override or augment the basic texture-based rendering.
             *
             * @param material Smart pointer to a `render::IMaterial` to use.
             */
            virtual void setMaterial( SmartPtr<render::IMaterial> material );

            /**
             * @brief Get the material currently used by this image.
             *
             * @return SmartPtr<render::IMaterial> Smart pointer to the current material
             *         or a null pointer if no material is assigned.
             */
            virtual SmartPtr<render::IMaterial> getMaterial() const;

            /**
             * @brief Retrieve the serialized properties for this component.
             *
             * Properties returned may include material, texture, border and sprite
             * configuration used for saving, inspection or editor integration.
             *
             * @return SmartPtr<Properties> Smart pointer to the properties object.
             *
             * @copydoc IComponent::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply serialized properties to this component.
             *
             * This method restores the component state from the provided `Properties`
             * object (used for loading, editor editing, or runtime property changes).
             *
             * @param properties Properties object containing settings to apply.
             *
             * @copydoc IComponent::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Return child shared objects owned by this UI element.
             *
             * Child objects can include resources or subordinate components that
             * are considered part of this object's lifecycle.
             *
             * @return Array<SmartPtr<ISharedObject>> Array of child shared object pointers.
             *
             * @copydoc ISharedObject::getChildObjects
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Called when the UI element changes state.
             *
             * Override of `UIElement::onChangedState`. This is invoked when the UI
             * element transitions between states (for example focused/hovered/pressed)
             * and should update visuals accordingly.
             *
             * @copydoc UIElement::onChangedState
             */
            void onChangedState() override;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // UIImage_h__
