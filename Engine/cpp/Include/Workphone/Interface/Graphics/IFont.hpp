#ifndef IFont_h__
#define IFont_h__

#include <Workphone/Interface/System/IResource.hpp>

namespace workphone
{
    namespace render
    {

        /** An interface for a font. */
        class WPCore_API IFont : public IResource
        {
        public:
            /**
             * @brief Property key string for font type.
             */
            static const String fontTypeStr;

            /**
             * @brief Property key string for font source.
             */
            static const String fontSourceStr;

            /**
             * @brief Property key string for font size.
             */
            static const String fontSizeStr;

            /**
             * @brief Property key string for font resolution.
             */
            static const String fontResolutionStr;

            IFont();

            IFont( u32 poolTypeId );

            ~IFont() override;

            /** Get the font type (e.g., TrueType, Bitmap, etc.) */
            virtual String getFontType() const = 0;
            virtual void setFontType( const String &type ) = 0;

            /** Get the font source (e.g., filename or resource identifier) */
            virtual String getFontSource() const = 0;
            virtual void setFontSource( const String &source ) = 0;

            /** Get the font size (in points or pixels) */
            virtual u32 getFontSize() const = 0;
            virtual void setFontSize( u32 size ) = 0;

            /** Get the font resolution (DPI or similar) */
            virtual u32 getFontResolution() const = 0;
            virtual void setFontResolution( u32 resolution ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IFont_h__
