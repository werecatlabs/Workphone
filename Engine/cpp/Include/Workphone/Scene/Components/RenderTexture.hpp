#ifndef RenderTexture_h__
#define RenderTexture_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Graphics/IRenderTexture.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class RenderTexture
         * @brief Component that owns and manages a render-target texture.
         *
         * The RenderTexture component encapsulates a GPU render texture and an
         * associated CPU-visible texture object. It exposes properties for
         * width, height, pixel format and a name for the texture, and provides
         * control over automatic updates of the render target.
         */
        class WPCore_API RenderTexture : public Component
        {
        public:
            /** @name Static property names
             *  Property name constants used for serialization and editor UI.
             */
            //@{
            static const String widthStr;       /**< Property name for width. */
            static const String heightStr;      /**< Property name for height. */
            static const String formatStr;      /**< Property name for pixel format. */
            static const String textureNameStr; /**< Property name for texture name. */
            static const String autoUpdateStr;  /**< Property name for auto-update flag. */
            static const String updateStr;      /**< Property name used to trigger an update. */
            //@}

            /**
             * @brief Constructs a new RenderTexture component with default
             *        width/height/format values.
             */
            RenderTexture();

            /**
             * @brief Destructor — releases GPU resources if still held.
             */
            ~RenderTexture() override;

            /**
             * @brief Loads component state from serialized data.
             *
             * This restores properties such as width, height, format and
             * texture name. If necessary it will recreate the underlying
             * render texture resource.
             *
             * @param data Serialized object containing component properties.
             *
             * @copydoc Component::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads component state and releases runtime resources.
             *
             * Typically called when the component is removed or the scene is
             * unloaded. Will destroy GPU resources associated with this
             * component.
             *
             * @param data Optional serialized object for unloading.
             *
             * @copydoc Component::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Returns nested shared objects owned by this component.
             *
             * The returned array may include the texture object if the
             * component exposes it as a child for serialization or editor
             * inspection.
             *
             * @return Array of child shared objects.
             * @copydoc Component::getChildObjects
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Retrieves a Properties object that describes this
             *        component's current state.
             *
             * Used by serialization and editor code to display and edit
             * component fields.
             *
             * @return SmartPtr to a Properties instance representing state.
             * @copydoc Component::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Updates the component state from a Properties object.
             *
             * This will apply any changes to width, height, format,
             * texture name or auto-update flags and recreate the render
             * texture if necessary.
             *
             * @param properties Properties object containing new values.
             * @copydoc Component::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Forces an update of the render texture contents.
             *
             * When auto-update is disabled this function can be invoked to
             * manually refresh the render target.
             */
            void update() override;

            /**
             * @brief Returns the GPU render texture object managed by this
             *        component.
             *
             * @return SmartPtr to the render::IRenderTexture instance or
             *         null if not created.
             */
            SmartPtr<render::IRenderTexture> getRenderTexture() const;

            /**
             * @brief Assigns an existing render texture to this component.
             *
             * Setting an external render texture will replace any internal
             * resource previously created by this component.
             *
             * @param renderTexture SmartPtr to the render texture to use.
             */
            void setRenderTexture( SmartPtr<render::IRenderTexture> renderTexture );

            /**
             * @brief Returns the texture view associated with the render
             *        target suitable for sampling in shaders or CPU access.
             *
             * @return SmartPtr to the render::ITexture instance or null.
             */
            SmartPtr<render::ITexture> getTexture() const;

            /**
             * @brief Sets the texture view associated with the render
             *        target.
             *
             * This does not necessarily change the underlying GPU render
             * target; it associates a texture interface for sampling or
             * external use.
             *
             * @param texture SmartPtr to the texture to associate.
             */
            void setTexture( SmartPtr<render::ITexture> texture );

            /**
             * @brief Returns the width (in pixels) of the render texture.
             *
             * @return Width in pixels.
             */
            u32 getWidth() const;

            /**
             * @brief Sets the width (in pixels) of the render texture.
             *
             * Changing the width may recreate the underlying GPU resource.
             *
             * @param width New width in pixels.
             */
            void setWidth( u32 width );

            /**
             * @brief Returns the height (in pixels) of the render texture.
             *
             * @return Height in pixels.
             */
            u32 getHeight() const;

            /**
             * @brief Sets the height (in pixels) of the render texture.
             *
             * Changing the height may recreate the underlying GPU resource.
             *
             * @param height New height in pixels.
             */
            void setHeight( u32 height );

            /**
             * @brief Returns the pixel format used by the render texture.
             *
             * @return PixelFormat enum value.
             */
            PixelFormat getFormat() const;

            /**
             * @brief Sets the pixel format for the render texture.
             *
             * Changing the format will recreate the GPU resource.
             *
             * @param format PixelFormat value to use.
             */
            void setFormat( PixelFormat format );

            /**
             * @brief Returns the user-facing name of the texture.
             *
             * This name may be used by editor tools or resource managers.
             *
             * @return Texture name string.
             */
            String getTextureName() const;

            /**
             * @brief Sets a user-facing name for the texture.
             *
             * @param textureName Name to assign to the texture.
             */
            void setTextureName( const String &textureName );

            /**
             * @brief Returns whether the render texture is updated
             *        automatically each frame.
             *
             * @return True if auto-update is enabled.
             */
            bool getAutoUpdate() const;

            /**
             * @brief Enables or disables automatic per-frame updates of the
             *        render texture.
             *
             * @param autoUpdate True to enable automatic updates.
             */
            void setAutoUpdate( bool autoUpdate );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Allocates and initializes the underlying GPU
             *        render texture using the current width/height/format.
             */
            void createRenderTexture();

            /**
             * @brief Releases GPU resources owned by this component and
             *        clears internal pointers.
             */
            void destroyRenderTexture();

            SmartPtr<render::IRenderTexture> m_renderTexture;
            SmartPtr<render::ITexture> m_texture;

            u32 m_width = 512;
            u32 m_height = 512;
            PixelFormat m_format = PixelFormat::PF_R8G8B8A8;
            String m_textureName;
            bool m_autoUpdate = true;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // RenderTexture_h__
