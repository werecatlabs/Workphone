#ifndef UIImageStateData_h__
#define UIImageStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>

namespace workphone
{

    class WPCore_API UIImageStateData : public StateData
    {
    public:
        UIImageStateData();
        ~UIImageStateData() override;

        WP_CLASS_REGISTER_DECL;

        /// The texture resource used by this image.
        SmartPtr<render::ITexture> texture;

        /// The material resource used by this image.
        SmartPtr<render::IMaterial> material;

        /// The size of the sprite in pixels (default: 1024x1024).
        Vector2I spriteSize = Vector2I( 1024, 1024 );

        /// Border size on the left side (in pixels).
        f32 borderLeft = 0.0f;

        /// Border size on the right side (in pixels).
        f32 borderRight = 0.0f;

        /// Border size on the top side (in pixels).
        f32 borderTop = 0.0f;

        /// Border size on the bottom side (in pixels).
        f32 borderBottom = 0.0f;

        /**
         * @brief Horizontal tile scale factor (default 1.0).
         *
         * Tile step along X = bounds.w / m_tileScaleX.  Only used when
         * `getUseTiling()` returns true.
         */
        f32 tileScaleX = 1.0f;

        /**
         * @brief Vertical tile scale factor (default 1.0).
         *
         * Tile step along Y = bounds.h / m_tileScaleY.
         */
        f32 tileScaleY = 1.0f;

        /// Whether tiling is enabled for this image.
        bool useTiling = false;

        /**
         * @brief Whether nine-slice rendering is active.
         *
         * When true and all border values are non-zero, `wp_draw_nine_slice` is
         * used instead of `wp_draw_image`.
         */
        bool useNineSlice = false;
    };

}  // namespace workphone

#endif  // UIImageStateData_h__
