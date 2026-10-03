#ifndef _ClawUIIMAGE_H
#define _ClawUIIMAGE_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

struct wp_context;

namespace workphone
{
    namespace ui
    {
        // Use the Prototype<IUIImage> instantiation exported from Workphone.dll
        // to avoid LNK2005 (already defined) when linking against Workphone.lib.
#if defined( WP_PLATFORM_WIN32 ) && !defined( _WP_STATIC_LIB_ ) && !defined( WPCore_EXPORTS )
        extern template class core::Prototype<IUIImage>;
#endif
        class ClawUIImage : public ClawUIElement<IUIImage>
        {
        public:
            ClawUIImage();
            ~ClawUIImage() override;

            /** @copydoc IObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            void setMaterialName( const String &materialName );
            String getMaterialName() const;

            /** Sets the texture. */
            void setTexture( SmartPtr<render::ITexture> texture ) override;

            /** Gets the texture. */
            SmartPtr<render::ITexture> getTexture() const override;

            void setMaterial( SmartPtr<render::IMaterial> material );
            SmartPtr<render::IMaterial> getMaterial() const;

            void setPosition( const Vector2F &position ) override;

            void setSize( const Vector2F &size ) override;

            /** @copydoc IComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc IObject::isValid */
            bool isValid() const override;

            void _getObject( void **ppObject ) const override;

            SmartPtr<render::IOverlayElementContainer> getContainerObject() const;
            void setContainerObject( SmartPtr<render::IOverlayElementContainer> container );

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            Vector2I getSpriteSize() const override;

            void setSpriteSize( const Vector2I &spriteSize ) override;

            f32 getBorderLeft() const override;

            void setBorderLeft( f32 borderLeft ) override;

            f32 getBorderRight() const override;

            void setBorderRight( f32 borderRight ) override;

            f32 getBorderTop() const override;

            void setBorderTop( f32 borderTop ) override;

            f32 getBorderBottom() const override;

            void setBorderBottom( f32 borderBottom ) override;

            /** @copydoc GuiElement::draw */
            void draw( struct wp_context *ctx ) override;

            bool getUseTiling() const override;

            void setUseTiling( bool useTiling ) override;

            bool getUseNineSlice() const override;
            void setUseNineSlice( bool useNineSlice ) override;

            f32 getTileScaleX() const override;
            void setTileScaleX( f32 scale ) override;

            f32 getTileScaleY() const override;
            void setTileScaleY( f32 scale ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            String m_materialName;
            Vector2I m_spriteSize = Vector2I::zero();
            f32 m_borderLeft = 0.0f;
            f32 m_borderRight = 0.0f;
            f32 m_borderTop = 0.0f;
            f32 m_borderBottom = 0.0f;
            f32 m_tileScaleX = 1.0f;
            f32 m_tileScaleY = 1.0f;
            bool m_useTiling = false;
            bool m_useNineSlice = false;
            SmartPtr<render::ITexture> m_texture;
            SmartPtr<render::IMaterial> m_material;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
