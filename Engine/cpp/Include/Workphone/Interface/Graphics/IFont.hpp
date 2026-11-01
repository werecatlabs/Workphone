#ifndef IFont_h__
#define IFont_h__
#include <memory>
namespace workphone {class StreamBudgetLease;}

#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace render
    {
        struct FontGlyphInfo;
        struct BakedFontInfo;

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
            /** Immutable captured bitmap-font view. Unsupported fonts return
             * false/empty. The snapshot keeps glyph and atlas owners alive even
             * when the resource wrapper is explicitly unloaded. */
            virtual bool getRuntimeMetrics(BakedFontInfo &,u32 &fallback) const;
            virtual u32 getRuntimeGlyphCount() const;
            virtual bool getRuntimeGlyphAt(u32,FontGlyphInfo &) const;
            virtual SmartPtr<ITexture> getRuntimeAtlas() const;
            virtual std::shared_ptr<const void> retainRuntimeFont() const;
            virtual std::shared_ptr<StreamBudgetLease> reserveRuntimeFontStorage(u64,String &error) const;
            virtual bool getRuntimeWhitePixelUV(Vector2F &) const;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IFont_h__
