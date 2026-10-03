#ifndef WP_CFRAMESTATISTICS_H
#define WP_CFRAMESTATISTICS_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IFrameStatistics.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>

namespace workphone
{
    /**
     * @brief Displays and manages runtime frame statistics overlay.
     *
     * The FrameStatistics class is responsible for creating, updating and
     * showing an overlay in the renderer that displays run-time statistics
     * such as FPS for rendering, physics and the application.
     *
     * The class implements IFrameStatistics and ISharedObject lifecycle
     * methods so it can be loaded/unloaded by the framework.
     *
     * Thread-safety:
     * - Some internal state is stored per-thread/task in @c m_nextUpdate to
     *   avoid contention when multiple engine tasks update statistics.
     */
    class WPCore_API FrameStatistics : public IFrameStatistics
    {
    public:
        /**
         * @brief Construct a new FrameStatistics instance.
         *
         * The constructor initializes internal state but does not create
         * the overlay elements. Use load() to initialize renderer resources.
         */
        FrameStatistics();

        /**
         * @brief Destroy the FrameStatistics instance.
         *
         * The destructor will release any references to renderer overlays
         * and related resources.
         */
        ~FrameStatistics() override;

        /**
         * @brief Load resources from the provided data object.
         *
         * @copydoc ISharedObject::load
         *
         * Typical behaviour:
         * - Create or acquire overlay and overlay text elements used to
         *   display FPS and other stats.
         * - Initialize per-task update timers in @c m_nextUpdate.
         *
         * @param data Optional configuration or data passed from the loader.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload and release resources.
         *
         * @copydoc ISharedObject::unload
         *
         * This will release overlay references and clear any renderer
         * related state created during load().
         *
         * @param data Optional data passed from the loader/unloader.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Update the displayed statistics.
         *
         * @copydoc ISharedObject::unload
         *
         * Called regularly (e.g. once per frame) to refresh the overlay text
         * with current FPS / timing information.
         *
         * Implementation notes:
         * - Should be low-overhead and avoid expensive allocations.
         * - Uses per-task entries in @c m_nextUpdate to determine when to
         *   perform the next refresh for that task.
         */
        void update() override;

        /**
         * @brief Returns whether the object is in a valid loaded state.
         *
         * @copydoc ISharedObject::isValid
         *
         * Typically returns true when overlay elements are created and
         * references are valid.
         *
         * @return true if resources are loaded and valid, false otherwise.
         */
        bool isValid() const override;

        /**
         * @brief Set whether the statistics overlay is visible.
         *
         * @copydoc IFrameStatistics::setVisible
         *
         * When visible, overlay text elements are shown by the renderer.
         *
         * @param visible True to show the overlay, false to hide it.
         */
        void setVisible( bool visible ) override;

        /**
         * @brief Query whether the statistics overlay is visible.
         *
         * @copydoc IFrameStatistics::isVisible
         *
         * @return True if overlay is currently visible.
         */
        bool isVisible() const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Overlay container that holds the statistics elements.
         *
         * This is a renderer overlay object that groups the text elements.
         */
        SmartPtr<render::IOverlay> m_statsOverlay;

        /**
         * @brief Text element used to display overall frames per second.
         *
         * Typically shows current FPS based on render timing.
         */
        SmartPtr<render::IOverlayElementText> m_fpsText;

        /**
         * @brief Text element used to display render thread statistics.
         *
         * Can show render-specific timing or frame time information.
         */
        SmartPtr<render::IOverlayElementText> m_renderText;

        /**
         * @brief Text element used to display physics FPS.
         *
         * Shows physics simulation update rate (steps per second).
         */
        SmartPtr<render::IOverlayElementText> m_physicsFPSText;

        /**
         * @brief Text element used to display application/main-loop FPS.
         *
         * Shows the application's main update loop frequency.
         */
        SmartPtr<render::IOverlayElementText> m_applicationFPSText;

        /**
         * @brief Per-task timing used to throttle updates.
         *
         * Stores the next scheduled update time (or delta) per engine task.
         * The array size equals the number of Task enum entries defined in
         * TaskId. Using an atomic type allows lock-free updates
         * across tasks.
         */
        FixedArray<atomic_f64, static_cast<u32>( TaskId::Count )> m_nextUpdate;
    };
}  // namespace workphone

#endif  // WP_CFRAMESTATISTICS_H
