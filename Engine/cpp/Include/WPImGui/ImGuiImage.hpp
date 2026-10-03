#ifndef ImGuiImage_h__
#define ImGuiImage_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/UI/UIImage.hpp>

namespace workphone
{
    namespace core
    {
        extern template class WPCore_API Prototype<ui::IUIImage>;
    }

    namespace ui
    {
        /** Implementation of a ui image. */
        class ImGuiImage : public ImGuiElement<UIImage>
        {
        public:
            /** Constructor. */
            ImGuiImage();

            /** Destructor. */
            ~ImGuiImage() override;

            void update() override;

            String getLabel() const override;
            void setLabel( const String &label ) override;

            void setTexture( SmartPtr<render::ITexture> texture ) override;
            SmartPtr<render::ITexture> getTexture() const override;

            SmartPtr<IResource> getResource() const;

            void setResource( SmartPtr<IResource> resource );

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

            bool getUseTiling() const override;

            void setUseTiling( bool useTiling ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<render::IMaterial> m_material;
            SmartPtr<render::ITexture> m_texture;
            SmartPtr<IResource> m_resource;
            size_t m_textureHandle = 0;
            bool m_useTiling = true;

            FixedString<128> m_label;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiImage_h__
