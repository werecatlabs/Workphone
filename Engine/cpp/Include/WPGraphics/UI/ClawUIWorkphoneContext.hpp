#ifndef _ClawUIWORKPHONECONTEXT_H
#define _ClawUIWORKPHONECONTEXT_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <WorkphoneCore/workphone.h>
#include <vector>

namespace workphone
{
    namespace ui
    {
        /**
         * @brief RAII wrapper around the c89 WorkphoneCore immediate-mode UI context.
         *
         * Owns the wp_context, default font atlas and backing memory.  This replaces the
         * previous MyGUI/overlay retained-mode backend used by WPGraphics::UI with the
         * Nuklear-fork API exposed by WorkphoneCore.
         */
        class WPGraphics_API ClawUIWorkphoneContext
        {
        public:
            ClawUIWorkphoneContext();
            ~ClawUIWorkphoneContext();

            /** @return true if the context was successfully initialised. */
            bool isValid() const;

            /** @return the underlying Nuklear-style context. */
            struct wp_context *getContext() const;

            /** @return the backing command/vertex buffer memory. */
            void *getMemory() const;
            static constexpr size_t kMemorySize = 1024 * 1024;  // 1 MiB

            const void *getFontPixels() const;
            wp_s32 getFontWidth() const;
            wp_s32 getFontHeight() const;
            wp_handle getFontTexture() const;
            const struct wp_draw_null_texture &getNullTexture() const;

            /** Convert a Workphone C++ colour to a WorkphoneCore colour. */
            static struct wp_color toWorkphoneColor( const ColourF &colour );

            /** Convert a Workphone C++ vector to a WorkphoneCore vector. */
            static wp_vec2f toWorkphoneVec2( const Vector2F &vec );

            /** Convert a Workphone C++ rectangle to a WorkphoneCore rectangle. */
            static struct wp_rect toWorkphoneRect( const Vector2F &position, const Vector2F &size );

        private:
            void shutdown();

            struct wp_context *m_context = nullptr;
            struct wp_font_atlas m_atlas = {};
            struct wp_draw_null_texture m_nullTexture = {};
            std::vector<wp_byte> m_fontPixels;
            wp_handle m_fontTexture = {};
            wp_s32 m_fontWidth = 0;
            wp_s32 m_fontHeight = 0;
            void *m_memory = nullptr;
            bool m_valid = false;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // _ClawUIWORKPHONECONTEXT_H
