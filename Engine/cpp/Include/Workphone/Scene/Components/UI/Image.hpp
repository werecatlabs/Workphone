#ifndef ImageComponent_h__
#define ImageComponent_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * Image component.
         *  This component is used to display an image on the screen.
         *  @note This component is not yet implemented.
         *
         *  @see UIComponent
         */
        class WPCore_API Image : public UIComponent
        {
        public:
            static const String TextureStr;
            static const String UseTilingStr;
            static const String BaseMaterialNameStr;

            /** Constructor. */
            Image();

            /** Destructor. */
            ~Image() override;

            /** @copydoc UIComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::updateMaterials */
            void updateMaterials() override;

            /** @copydoc UIComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc UIComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc UIComponent::isValid */
            bool isValid() const override;

            /** Get the image.
             * @return The image.
             */
            SmartPtr<ui::IUIImage> getImage() const;

            /** Set the image.
             * @param image The image.
             */
            void setImage( SmartPtr<ui::IUIImage> image );

            /** Get the texture.
             * @return The texture.
             */
            SmartPtr<render::ITexture> getTexture() const;

            /** Set the texture.
             * @param texture The texture.
             */
            void setTexture( SmartPtr<render::ITexture> texture );

            /** Get the texture name.
             * @return The texture name.
             */
            String getTextureName() const;

            /** Set the texture name.
             * @param textureName The texture name.
             */
            void setTextureName( const String &textureName );

            /** Get the use tiling flag.
             * @return The use tiling flag.
             */
            bool getUseTiling() const;

            /** Set the use tiling flag.
             * @param useTiling The use tiling flag.
             */
            void setUseTiling( bool useTiling );

            /** Gets the name of the base UI material cloned for this image.
             * @return The base material name.
             */
            String getBaseMaterialName() const;

            /** Sets the name of the base UI material cloned for this image.
             *  Change takes effect on the next call to setupMaterial().
             * @param name The base material name.
             */
            void setBaseMaterialName( const String &name );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @copydoc UIComponent::createUI */
            void createUI() override;

            /** Sets up the material. */
            void setupMaterial();

            /** The image. */
            AtomicSmartPtr<ui::IUIImage> m_image;

            /** The image texture. */
            AtomicSmartPtr<render::ITexture> m_texture;

            /** The image material. */
            AtomicSmartPtr<render::IMaterial> m_material;

            /** To use sprite tiling in the graphics system. */
            atomic_bool m_useTiling = true;

            /** Name of the base UI material that is cloned per image instance. */
            String m_baseMaterialName = String( "DefaultUI" );
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ImageComponent_h__
