#ifndef _COverlayElementText_H
#define _COverlayElementText_H

#include <WPGraphics/ClawOverlayElement.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementText.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class COverlayElementText
         * @brief Base implementation of a text overlay element.
         */
        class ClawOverlayElementText : public ClawOverlayElement<IOverlayElementText>
        {
        public:
            ClawOverlayElementText();
            ~ClawOverlayElementText() override;

            void setFontName( const String &fontName ) override;
            void setCharHeight( f32 charHeight ) override;
            void setVerticalAlignment( u8 alignment ) override;
            u8 getVerticalAlignment() const override;
            void setHorizontalAlignment( u8 alignment ) override;
            u8 getHorizontalAlignment() const override;
            void setSpaceWidth( f32 width ) override;
            f32 getSpaceWidth() const override;

            void update() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_fontName;
            f32 m_charHeight = 20.0f;
            u8 m_verticalAlignment = 0;
            u8 m_horizontalAlignment = 0;
            f32 m_spaceWidth = 0.0f;
        };
    }  // end namespace render
}  // namespace workphone

#endif
