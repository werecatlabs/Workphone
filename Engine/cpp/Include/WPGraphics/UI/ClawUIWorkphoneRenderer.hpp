#ifndef _ClawUIWORKPHONERENDERER_H
#define _ClawUIWORKPHONERENDERER_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WorkphoneCore/workphone.h>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>

namespace workphone
{
    namespace ui
    {
        /**
         * @brief Bridge between WorkphoneCore's Nuklear-style command/draw buffers and the
         * Workphone renderer.
         *
         * This is intentionally lightweight: it owns the transient vertex/index buffers
         * used by wp_convert() and exposes a single submit() method that a renderer
         * backend can implement.
         */
        class WPGraphics_API ClawUIWorkphoneRenderer
        {
        public:
            ClawUIWorkphoneRenderer();
            ~ClawUIWorkphoneRenderer();

            /**
             * @brief Convert the recorded UI commands into vertex/index data.
             *
             * @param ctx The active WorkphoneCore context after wp_end().
             * @return true if conversion succeeded.
             */
            bool convert( struct wp_context *ctx );

            /**
             * @brief Submit the converted draw list to a WorkphoneGraphics C89 renderer.
             *
             * The font atlas (if any) is uploaded once via the renderer's native SRV path
             * (or stored as a pixel pointer for the software backend) and then bound through
             * `wp_renderer_set_texture_native` for every text draw.  This avoids the per-text
             * CPU pixel upload that the old mixed-API path performed and matches the way
             * `ClawImguiManager` binds its font atlas.  The font SRV is destroyed lazily when
             * the renderer is destroyed or `clearCachedFontTexture()` is called.
             */
            bool submit( struct wp_context *ctx, wp_renderer *renderer, const void *fontPixels,
                         wp_s32 fontWidth, wp_s32 fontHeight, wp_handle fontTexture );

            /**
             * @brief Release the cached native font texture (if any).
             *
             * Safe to call when no native texture has been cached.  Useful when the renderer
             * is being swapped (e.g. DX11 -> Software) so we don't leak the previous backend's
             * SRV.
             */
            void clearCachedFontTexture();

            void setNullTexture( const struct wp_draw_null_texture &nullTexture );

            /** @return number of draw commands produced by the last convert(). */
            size_t getCommandCount() const;

            /** @return pointer to the draw commands produced by wp_convert(). */
            const struct wp_draw_command *getCommands() const;

            /** @return pointer to the vertex data. */
            const void *getVertices() const;

            /** @return number of vertices produced by the last convert(). */
            size_t getVertexCount() const;

            /** @return pointer to the index data. */
            const wp_draw_index *getIndices() const;

            /** @return number of indices produced by the last convert(). */
            size_t getIndexCount() const;

            /** @return the configured vertex layout (position/colour/uv). */
            static const struct wp_draw_vertex_layout_element *getVertexLayout();

        private:
            void resizeBuffers( size_t commandBytes, size_t vertexBytes, size_t indexBytes );

            /**
             * @brief Lazily upload (or re-upload) the font atlas into a native SRV owned by
             * `renderer` (or store the pixel pointer for the software backend).
             *
             * Re-uploads only when the renderer pointer, dimensions, or pixel pointer change.
             * Returns the SRV pointer for hardware backends, or the raw pixel pointer for the
             * software backend.  Returns nullptr when no font pixels are available.
             */
            void *ensureFontTextureNative( struct wp_renderer *renderer, const void *fontPixels,
                                          wp_s32 fontWidth, wp_s32 fontHeight );

            /** Release any cached native font SRV. */
            void releaseFontTextureNative();

            struct wp_buffer m_commandBuffer;
            struct wp_buffer m_vertexBuffer;
            struct wp_buffer m_indexBuffer;
            struct wp_convert_config m_config;

            size_t m_commandCount = 0;
            size_t m_vertexCount = 0;
            size_t m_indexCount = 0;

            /** Native SRV handle for the font atlas, cached after first upload. */
            void *m_fontTextureNative = nullptr;
            /** Renderer that owns @ref m_fontTextureNative (matched by pointer each frame). */
            const struct wp_renderer *m_fontTextureRenderer = nullptr;
            /** Width of the cached font texture (used to detect a font atlas swap). */
            wp_s32 m_fontTextureCachedWidth = 0;
            /** Height of the cached font texture. */
            wp_s32 m_fontTextureCachedHeight = 0;
            /** True if the cached font is just a pixel pointer (software backend). */
            bool m_fontTextureIsPixelPointer = false;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // _ClawUIWORKPHONERENDERER_H