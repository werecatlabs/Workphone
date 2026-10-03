#ifndef __CFontWPGraphics_h__
#define __CFontWPGraphics_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/Font.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>

// Forward declarations for C font structures
struct wp_font;
struct wp_font_atlas;
struct wp_font_glyph;
struct wp_baked_font;

namespace workphone
{
    namespace render
    {
        /**
         * @brief Format for font atlas baking.
         */
        enum class FontAtlasFormat
        {
            Alpha8, /**< 8-bit alpha texture */
            RGBA32  /**< 32-bit RGBA texture */
        };

        /**
         * @brief Concrete Font implementation for the WPGraphics/ClawHammer renderer.
         *
         * CFont adapts the engine's Font base class for the WPGraphics backend.
         * It wraps the C font API (workphone_font.h) and provides texture atlas
         * functionality for font rendering.
         */
        class WPGraphics_API ClawFont : public Font
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Creates an empty font wrapper. Heavy work is performed in load().
             */
            ClawFont();

            /**
             * @brief Construct and register with a resource manager.
             * \param resourceManager Pointer to the resource manager that owns this font.
             */
            ClawFont( IResourceManager *resourceManager );

            /** \brief Virtual destructor. Releases any held resources. */
            ~ClawFont() override;

            /**
             * \brief Load font data from the provided shared object.
             * \param data Shared object containing font resource data or descriptors.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * \brief Unload the font and free associated resources.
             * \param data Optional shared object associated with the resource.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Add a font from memory.
             * @param memory Pointer to font data in memory.
             * @param size Size of the font data in bytes.
             * @param height Font height in pixels.
             * @param mergeMode If true, merge with the previous font.
             */
            void addFontFromMemory( void *memory, size_t size, f32 height, bool mergeMode = false );

            /**
             * @brief Add a font from compressed memory (LZFSE/LZ4/zlib).
             * @param memory Pointer to compressed font data.
             * @param size Size of the compressed data in bytes.
             * @param height Font height in pixels.
             * @param mergeMode If true, merge with the previous font.
             */
            void addFontFromCompressed( void *memory, size_t size, f32 height, bool mergeMode = false );

            /**
             * @brief Add a font from base85 compressed data.
             * @param data Base85 encoded compressed font data.
             * @param height Font height in pixels.
             * @param mergeMode If true, merge with the previous font.
             */
            void addFontFromCompressedBase85( const char *data, f32 height, bool mergeMode = false );

            /**
             * @brief Add a font from a file.
             * @param filePath Path to the font file.
             * @param height Font height in pixels.
             * @param mergeMode If true, merge with the previous font.
             */
            void addFontFromFile( const String &filePath, f32 height, bool mergeMode = false );

            /**
             * @brief Set the default glyph ranges for newly added fonts.
             * @param ranges Array of codepoint ranges (pairs of from/to, terminated with 0).
             */
            void setDefaultGlyphRanges( const wp_rune *ranges );

            /** @brief Use Chinese glyph ranges for default fonts. */
            void setDefaultGlyphRangesChinese();

            /** @brief Use Cyrillic glyph ranges for default fonts. */
            void setDefaultGlyphRangesCyrillic();

            /** @brief Use Korean glyph ranges for default fonts. */
            void setDefaultGlyphRangesKorean();

            /** @brief Get the default glyph ranges. */
            const wp_rune *getDefaultGlyphRanges() const;

            /**
             * @brief Begin font atlas building.
             *
             * Call this before adding fonts to the atlas.
             */
            void beginAtlas();

            /**
             * @brief Bake the font atlas.
             * @param format Texture format for the atlas.
             * @param outWidth Output parameter for atlas width.
             * @param outHeight Output parameter for atlas height.
             * @return Pointer to the baked pixel data.
             */
            const void *bakeAtlas( FontAtlasFormat format, s32 *outWidth, s32 *outHeight );

            /**
             * @brief End font atlas building and upload to GPU.
             * @param textureHandle Optional native texture handle.
             */
            void endAtlas( void *textureHandle = nullptr );

            /**
             * @brief Find glyph information for a codepoint.
             * @param codepoint Unicode codepoint.
             * @return Glyph information structure.
             */
            FontGlyphInfo findGlyph( wp_rune codepoint ) const;

            /**
             * @brief Find glyph information for a specific font and codepoint.
             * @param font Native font pointer.
             * @param codepoint Unicode codepoint.
             * @return Glyph information structure.
             */
            FontGlyphInfo findGlyphFromFont( struct wp_font *font, wp_rune codepoint ) const;

            /** @brief Get baked font information for the default font. */
            BakedFontInfo getBakedFontInfo() const;

            /** @brief Get the array of baked glyphs. */
            const struct wp_font_glyph *getGlyphs() const;

            /** @brief Get the number of glyphs in the atlas. */
            s32 getGlyphCount() const;

            /** @brief Get the linked list of fonts in the atlas. */
            struct wp_font *getFonts() const;

            /** @brief Get the number of fonts in the atlas. */
            s32 getFontCount() const;

            /** @brief Get the raw atlas pixel data. */
            void *getAtlasPixelData() const;

            /** @brief Get the atlas texture width. */
            s32 getAtlasWidth() const;

            /** @brief Get the atlas texture height. */
            s32 getAtlasHeight() const;

            /** @brief Check if the atlas has been baked. */
            bool isAtlasBaked() const;

            /** @brief Get the native C font structure. */
            struct wp_font *getNativeFont() const;

            /** @brief Get the native C font atlas structure. */
            struct wp_font_atlas *getNativeAtlas();

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * \brief Static name extension used to generate unique font names.
             *
             * Incremented for each created font wrapper so that generated resource
             * names do not collide.
             */
            static u32 m_nameExt;

            struct Impl;
            std::unique_ptr<Impl> m_impl;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // __CFontWPGraphics_h__
