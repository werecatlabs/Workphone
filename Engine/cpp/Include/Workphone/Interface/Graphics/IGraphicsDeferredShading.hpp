#ifndef IGraphicsDeferredShading_h__
#define IGraphicsDeferredShading_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for managing the engine's deferred shading pipeline.
         *
         * IGraphicsDeferredShading provides lightweight runtime control over a
         * deferred rendering pipeline used by the renderer. Implementations
         * typically own and configure the G-buffer, lighting passes, shadowing,
         * and optional screen-space/post-process effects (for example SSAO).
         *
         * Responsibilities and expectations:
         * - Expose runtime toggles and compositor selection used by the renderer.
         * - Be implemented as a reference-counted shared object (see ISharedObject).
         * - Changes made via setters generally affect the next rendered frame.
         *
         * Notes:
         * - The integer mode used by getMode()/setMode() corresponds to an engine
         *   DSMode enumeration (not defined in this header). Use that enum for
         *   valid values and semantics.
         * - Implementations should be careful about thread-safety: callers may
         *   query state from the render thread while other systems modify it.
         *
         * @see ISharedObject
         */
        class WPCore_API IGraphicsDeferredShading : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensure correct cleanup through base pointers. Concrete implementations
             * should release any GPU resources and unregister from renderer subsystems
             * during destruction.
             */
            ~IGraphicsDeferredShading() override;

            /**
             * @brief Get the current deferred shading mode.
             *
             * The returned value maps to the engine's DSMode enumeration (for
             * example: full deferred, forward+ hybrid, debug view modes, etc.).
             *
             * @return Current rendering mode as a u32 (DSMode enum value).
             */
            virtual u32 getMode() const = 0;

            /**
             * @brief Set the deferred shading rendering mode.
             *
             * Changing the mode may alter which passes are executed, which
             * compositors are used, and which debug visualisations are visible.
             * The change should take effect on the next frame or when the renderer
             * next rebuilds its pass/compositor configuration.
             *
             * @param mode A u32 value corresponding to a DSMode enumerator.
             */
            virtual void setMode( u32 mode ) = 0;

            /**
             * @brief Query whether Screen-Space Ambient Occlusion (SSAO) is enabled.
             *
             * SSAO is a screen-space approximation of ambient occlusion applied as
             * a post-process. When enabled it increases visual depth cues at the
             * cost of extra GPU work.
             *
             * @return true if SSAO is enabled; false otherwise.
             */
            virtual bool getSSAO() const = 0;

            /**
             * @brief Enable or disable Screen-Space Ambient Occlusion (SSAO).
             *
             * The request should be applied safely; implementations may defer the
             * actual toggle to the render thread or next frame to avoid stalling.
             *
             * @param ssao true to enable SSAO; false to disable.
             */
            virtual void setSSAO( bool ssao ) = 0;

            /**
             * @brief Determine whether the deferred shading system is active.
             *
             * When inactive, the renderer may bypass deferred passes entirely and
             * fall back to an alternative pipeline (for example a forward renderer).
             *
             * @return true if the deferred system is active; false if deactivated.
             */
            virtual bool getActive() const = 0;

            /**
             * @brief Activate or deactivate the deferred shading system.
             *
             * Deactivating the system should allow the renderer to safely use a
             * fallback rendering path. Activating may trigger resource allocation
             * (G-buffer creation, compositors) on the render thread.
             *
             * @param active true to activate; false to deactivate.
             */
            virtual void setActive( bool active ) = 0;

            /**
             * @brief Query whether shadow rendering is enabled.
             *
             * If enabled, the deferred pipeline will incorporate shadowing
             * (shadow map generation and application) during lighting passes.
             *
             * @return true if shadows are enabled; false otherwise.
             */
            virtual bool getShadowsEnabled() const = 0;

            /**
             * @brief Enable or disable shadow rendering in the deferred pipeline.
             *
             * Enabling shadows may allocate shadow maps and increase GPU/CPU cost.
             * Disabling should free or skip shadow-related resources and passes.
             *
             * @param enabled true to enable shadows; false to disable them.
             */
            virtual void setShadowsEnabled( bool enabled ) = 0;

            /**
             * @brief Get the name/identifier of the compositor used to build the G-buffer.
             *
             * The compositor name identifies the compositor or render pass configuration
             * responsible for producing the G-buffer textures (normals, albedo,
             * material properties, depth, etc.). An empty string indicates no
             * explicit compositor configured and the implementation's default will be used.
             *
             * @return Compositor name as a String. May be empty.
             */
            virtual String getGBufferCompositorName() const = 0;

            /**
             * @brief Set the compositor name used to construct the G-buffer.
             *
             * Changing this value should cause the renderer to (re)bind or rebuild
             * the compositor sequence used for G-buffer generation.
             *
             * @param compositorName Name of the compositor to use for G-buffer generation.
             */
            virtual void setGBufferCompositorName( const String &compositorName ) = 0;

            /**
             * @brief Get the compositor name used to visualise combined lighting.
             *
             * This compositor typically displays the result of lighting resolution
             * (useful for debugging lighting composition or for final presentation).
             *
             * @return Compositor name as a String.
             */
            virtual String getShowLightingCompositorName() const = 0;

            /**
             * @brief Set the compositor used to visualise combined lighting.
             *
             * @param compositorName Compositor name used to display lighting results.
             */
            virtual void setShowLightingCompositorName( const String &compositorName ) = 0;

            /**
             * @brief Get the compositor name used to visualise normals.
             *
             * Useful for verifying normal encoding in the G-buffer and identifying
             * reconstruction errors in lighting.
             *
             * @return Compositor name as a String.
             */
            virtual String getShowNormalsCompositorName() const = 0;

            /**
             * @brief Set the compositor used to visualise normals.
             *
             * @param compositorName Compositor name used for normal visualization.
             */
            virtual void setShowNormalsCompositorName( const String &compositorName ) = 0;

            /**
             * @brief Get the compositor name used to visualise depth and specular.
             *
             * Often used for debugging the depth buffer, specular intensity, or
             * material attributes stored in the G-buffer.
             *
             * @return Compositor name as a String.
             */
            virtual String getShowDepthSpecularCompositorName() const = 0;

            /**
             * @brief Set the compositor used to visualise depth and specular channels.
             *
             * @param compositorName Compositor name for depth/specular visualization.
             */
            virtual void setShowDepthSpecularCompositorName( const String &compositorName ) = 0;

            /**
             * @brief Get the compositor name used to visualise the final colour output.
             *
             * This compositor typically represents the final resolve / post-process
             * stage that produces the displayed colour buffer.
             *
             * @return Compositor name as a String.
             */
            virtual String getShowColourCompositorName() const = 0;

            /**
             * @brief Set the compositor used to display the final colour output.
             *
             * @param compositorName Compositor name used for the final colour/output visualization.
             */
            virtual void setShowColourCompositorName( const String &compositorName ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // IGraphicsDeferredShading_h__
