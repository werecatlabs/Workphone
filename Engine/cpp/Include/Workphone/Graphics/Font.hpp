#ifndef Font_h__
#define Font_h__

#include <Workphone/Interface/Graphics/IFont.hpp>
#include <Workphone/Graphics/ResourceGraphics.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Represents a font resource used for rendering text.
         *
         * The Font class encapsulates font properties such as type, source, size, and resolution.
         * It provides methods to load and unload font data, as well as to get and set font properties.
         * Inherits from ResourceGraphics<IFont>.
         */
        struct FontGlyphInfo
        {
            u32 codepoint;
            f32 xadvance;
            f32 x0, y0, x1, y1;
            f32 width, height;
            f32 u0, v0, u1, v1;
        };

        struct BakedFontInfo
        {
            f32 height = 0.0f;
            f32 ascent = 0.0f;
            f32 descent = 0.0f;
            u32 glyphOffset = 0;
            u32 glyphCount = 0;
            const u32 *ranges = nullptr;
        };

        class WPCore_API Font : public ResourceGraphics<IFont>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes a new Font object with default properties.
             */
            Font();

            Font( u32 poolTypeId );

            /**
             * @brief Destructor.
             *
             * Cleans up any resources used by the Font object.
             */
            ~Font() override;

            /**
             * @brief Loads font data from the provided shared object.
             * @param data Shared pointer to the data required for loading the font.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads font data and releases associated resources.
             * @param data Shared pointer to the data required for unloading the font.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the properties of the font as a Properties object.
             * @return Smart pointer to the Properties object containing font properties.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the font from a Properties object.
             * @param properties Smart pointer to the Properties object containing new font properties.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Gets the type of the font (e.g., "TrueType", "Bitmap").
             * @return The font type as a string.
             */
            String getFontType() const override;

            /**
             * @brief Sets the type of the font (e.g., "TrueType", "Bitmap").
             * @param type The font type as a string.
             */
            void setFontType( const String &type ) override;

            /**
             * @brief Gets the source of the font (e.g., file path or resource name).
             * @return The font source as a string.
             */
            String getFontSource() const override;

            /**
             * @brief Sets the source of the font (e.g., file path or resource name).
             * @param source The font source as a string.
             */
            void setFontSource( const String &source ) override;

            /**
             * @brief Gets the size of the font in points.
             * @return The font size as an unsigned 32-bit integer.
             */
            u32 getFontSize() const override;

            /**
             * @brief Sets the size of the font in points.
             * @param size The font size as an unsigned 32-bit integer.
             */
            void setFontSize( u32 size ) override;

            /**
             * @brief Gets the resolution of the font in DPI (dots per inch).
             * @return The font resolution as an unsigned 32-bit integer.
             */
            u32 getFontResolution() const override;

            /**
             * @brief Sets the resolution of the font in DPI (dots per inch).
             * @param resolution The font resolution as an unsigned 32-bit integer.
             */
            void setFontResolution( u32 resolution ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief The type of the font (e.g., "TrueType", "Bitmap"). */
            String m_fontType;

            /** @brief The source of the font (e.g., file path or resource name). */
            String m_fontSource;

            /** @brief The size of the font in points. Default is 12. */
            u32 m_fontSize = 12;

            /** @brief The resolution of the font in DPI. Default is 96. */
            u32 m_fontResolution = 96;
        };

    }  // namespace render
}  // namespace workphone

#endif  // Font_h__
