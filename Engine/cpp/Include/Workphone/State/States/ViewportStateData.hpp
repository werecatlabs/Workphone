#ifndef ViewportState_h__
#define ViewportState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>

namespace workphone
{

    /**
     * @class ViewportState
     * @brief Serializable state container describing a single viewport.
     *
     * This class contains the data required to create or restore a viewport.
     * It is a lightweight POD-style state object used by the engine state
     * system (derived from `StateData`) and intended to be copied/serialized.
     *
     * It stores references (weak pointers) to render objects such as camera
     * and textures, viewport layout in normalized or absolute coordinates,
     * rendering flags, and optional material/background identifiers.
     *
     * @note Members are public for easy serialization and state copying.
     */
    class WPCore_API ViewportStateData : public StateData
    {
    public:
        /** Default constructor. Initializes members to sensible defaults. */
        ViewportStateData();

        /** Virtual destructor. */
        ~ViewportStateData() override;

        /** Unload the state data. */
        void unload( SmartPtr<ISharedObject> data ) override;

        WP_CLASS_REGISTER_DECL;

        /**
         * Weak reference to the camera used by this viewport.
         * If empty, the viewport will not have a camera assigned.
         */
        WeakPtr<render::IGraphicsCamera> camera;

        /**
         * Weak reference to a texture used as the viewport render target.
         * If set, the viewport will render into this texture instead of the
         * default back buffer.
         */
        WeakPtr<render::ITexture> texture;

        /**
         * Weak reference to a background texture used to fill the viewport
         * background (behind the scene). This is separate from `texture`.
         */
        WeakPtr<render::ITexture> backgroundTexture;

        /**
         * Background colour used when the viewport is cleared.
         * Typically used when `render::IViewport::clearFlag` is enabled.
         */
        ColourF backgroundColour;

        /**
         * Normalized or pixel position of the viewport origin (x, y).
         * Interpretation depends on how viewports are consumed by the renderer
         * (often normalized [0,1] where (0,0) is bottom-left).
         */
        Vector2<real_Num> position;

        /**
         * Normalized or pixel size of the viewport (width, height).
         * Interpretation depends on the renderer's viewport coordinate convention.
         */
        Vector2<real_Num> size;

        /**
         * Actual position computed when the viewport is applied to the target
         * (in pixels or render target coordinates). Populated at runtime.
         */
        Vector2<real_Num> actualPosition;

        /**
         * Actual size computed when the viewport is applied to the target
         * (in pixels or render target coordinates). Populated at runtime.
         */
        Vector2<real_Num> actualSize;

        /**
         * Z-order for overlaying viewports. Lower values are drawn first.
         * Default is -1 (unspecified / lowest priority).
         */
        s32 zorder = -1;

        /**
         * Visibility mask bitfield used to filter which objects are visible
         * in this viewport. Default is all bits set (visible).
         */
        u32 visibilityMask = std::numeric_limits<u32>::max();

        /**
         * Priority value used by the renderer to order render operations or
         * resolve conflicts between multiple viewports/render targets.
         */
        u32 priority = 0;

        /**
         * Buffer flags specifying which buffers the viewport uses (colour,
         * depth, stencil, etc.). Interpretation depends on the renderer.
         */
        u32 buffers = 0;

        /**
         * Viewport behaviour flags. By default this enables:
         * - overlaysEnabledFlag
         * - skiesEnabledFlag
         * - activeFlag
         * - shadowsEnabledFlag
         * - autoUpdatedFlag
         * - clearFlag
         * - enableSceneRenderFlag
         *
         * These flags are defined in `render::IViewport` and can be combined
         * with bitwise operations to enable/disable specific behaviours.
         */
        u32 flags = render::IViewport::overlaysEnabledFlag | render::IViewport::skiesEnabledFlag |
                    render::IViewport::activeFlag | render::IViewport::shadowsEnabledFlag |
                    render::IViewport::autoUpdatedFlag | render::IViewport::clearFlag |
                    render::IViewport::enableSceneRenderFlag;

        /**
         * Optional material scheme name used when rendering this viewport.
         * If empty, the default material scheme will be used.
         */
        String materialScheme;

        /**
         * Optional name of a background texture resource. This can be used by
         * resource managers to locate and assign `backgroundTexture` at runtime.
         */
        String backgroundTextureName;
    };
}  // namespace workphone

#endif  // ViewportState_h__
