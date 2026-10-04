#ifndef _CCameraController_H
#define _CCameraController_H

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Base class for camera controllers used in the scene.
         *
         * CameraController provides a common interface and shared functionality
         * for different camera types (e.g. orbital, first-person) used by the
         * editor and runtime. It manages camera flags, viewport association,
         * projection type (orthographic vs perspective), and exposes hooks for
         * focusing and resetting the camera.
         */
        class WPCore_API CameraController : public Component
        {
        public:
            static const String viewportIdStr;
            static const String cameraFlagsStr;
            static const String isActiveStr;
            static const String isMainCameraStr;
            static const String isOrthographicStr;

            // Camera flag constants
            static const u32 CameraFlags_None;
            static const u32 CameraFlags_Active;
            static const u32 CameraFlags_MainCamera;
            static const u32 CameraFlags_Orthographic;
            static const u32 CameraFlags_RenderToTexture;
            static const u32 CameraFlags_PostProcessing;

            // Property key strings
            static const String positionStr;
            static const String targetStr;
            static const String sphericalCoordsStr;
            static const String rotationSpeedStr;
            static const String zoomSpeedStr;
            static const String moveSpeedStr;
            static const String maxDistanceStr;
            static const String nearDistanceStr;

            /**
             * @brief Event listener wrapper that forwards events to the
             * CameraController instance that owns it.
             *
             * Camera controllers can receive editor or system events via a
             * dedicated listener object. This nested class holds a weak
             * reference to the owning CameraController and translates incoming
             * events into controller actions.
             */
            class EventListener : public IEventListener
            {
            public:
                EventListener();
                ~EventListener() override;

                /**
                 * @brief Handle an incoming event.
                 *
                 * This method is called by the event system. It should interpret
                 * the event parameters and delegate handling to the owning
                 * CameraController where appropriate.
                 *
                 * @param eventType The type of the event.
                 * @param eventValue A hashed value identifying the event.
                 * @param arguments Event-specific parameters.
                 * @param sender The object that sent the event.
                 * @param object Optional associated object instance.
                 * @param event The raw event object.
                 * @return A Parameter result from the event handling.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Set the owner camera controller for this listener.
                 * @param owner Smart pointer to the owning CameraController.
                 */
                void setOwner( SmartPtr<CameraController> owner );

                /**
                 * @brief Get the owning CameraController if it still exists.
                 * @return Smart pointer to the owner or null if expired.
                 */
                SmartPtr<CameraController> getOwner() const;

                WP_CLASS_REGISTER_DECL;

            protected:
                /// Weak reference to the camera controller that owns this listener
                AtomicWeakPtr<CameraController> m_cameraController;
            };

            /**
             * @brief Construct a CameraController with default settings.
             */
            CameraController();

            /**
             * @brief Virtual destructor.
             */
            ~CameraController() override;

            /**
             * @copydoc Component::load
             *
             * The provided data object may contain configuration for initial
             * camera position, target, control speeds and other serialized
             * properties. Implementations should read supported properties
             * and apply them to the controller state.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::unload
             *
             * Clean up any runtime resources such as event listeners or
             * viewport bindings created by the controller.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::reload
             *
             * A reload should re-apply configuration from the provided
             * data object and refresh runtime bindings.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Set or clear a camera flag.
             *
             * Flags control camera behaviour such as whether the camera is
             * active, designated as the main camera, or uses an
             * orthographic projection.
             *
             * @param flag Flag bit to modify.
             * @param value True to set the flag, false to clear it.
             */
            void setCameraFlag( u32 flag, bool value );

            /**
             * @brief Query the current value of a camera flag.
             * @param flag Flag bit to query.
             * @return True if the flag is set, false otherwise.
             */
            bool getCameraFlag( u32 flag ) const;

            /**
             * @brief Associate this controller with a viewport index.
             *
             * Viewport id is used to lookup or bind the camera to a specific
             * rendering viewport in multi-viewport setups.
             *
             * @param viewportId The viewport index to associate with.
             */
            void setViewportId( u32 viewportId );

            /**
             * @brief Get the currently associated viewport index.
             * @return The associated viewport id.
             */
            u32 getViewportId() const;

            /**
             * @brief Enable or disable the camera controller.
             *
             * When disabled the controller should stop updating camera state
             * or responding to input/events.
             *
             * @param active True to enable the controller, false to disable.
             */
            void handleSetActive( bool active );

            /**
             * @copydoc IComponent::getChildObjects
             *
             * Returns any additional shared objects (for example the
             * EventListener) that should be owned/serialized with the
             * component.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc IComponent::getProperties
             *
             * Construct a Properties object containing the controller's
             * configurable values (position, speeds, flags etc.).
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IComponent::setProperties
             *
             * Apply values from a Properties object to configure or update
             * the controller state.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Focus the camera on the currently selected scene objects.
             *
             * Implementations should compute a reasonable view that frames the
             * selection and move the camera/target accordingly.
             */
            virtual void focusSelection();

            /**
             * @brief Focus the camera to frame the provided axis-aligned
             * bounding box.
             *
             * Controllers should update camera position/target and zoom so
             * that the entire bounds are visible.
             *
             * @param bounds The axis-aligned bounding box to focus on.
             */
            virtual void focusOnBounds( const AABB3<real_Num> &bounds );

            /**
             * @brief Reset the camera to its default transform and settings.
             *
             * Implementations should restore default position, orientation,
             * projection parameters and any other controller-specific state.
             */
            virtual void resetCamera();

            /**
             * @brief Check whether this camera is flagged as the main camera.
             * @return True if the controller is the main camera, false
             * otherwise.
             */
            bool isMainCamera() const;

            /**
             * @brief Mark or unmark this controller as the scene's main
             * camera.
             *
             * Setting this flag may trigger changes to rendering order or
             * active camera selection.
             *
             * @param mainCamera True to set as main camera, false to unset.
             */
            void setMainCamera( bool mainCamera );

            /**
             * @brief Query whether the camera uses an orthographic projection.
             * @return True if orthographic, false for perspective.
             */
            bool isOrthographic() const;

            /**
             * @brief Set the camera projection mode.
             * @param orthographic True to use orthographic projection, false
             * for perspective.
             */
            void setOrthographic( bool orthographic );

            /**
             * @brief Get the editor UI window associated with this controller.
             * @return Smart pointer to the UI window, or null if none.
             */
            SmartPtr<ui::IUIWindow> getUiWindow() const;

            /**
             * @brief Associate an editor UI window with this controller.
             * @param uiWindow Smart pointer to the UI window.
             */
            void setUiWindow( SmartPtr<ui::IUIWindow> uiWindow );

            /**
             * @brief Handles a component-specific event for the mesh renderer.
             * @param state The current state of the component.
             * @param eventType The type of FSM event.
             * @return FSMReturnType indicating the result of the event handling.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Handle camera flags change.
             * @param oldFlags Previous flags.
             * @param newFlags New flags.
             */
            virtual void onCameraFlagsChanged( u32 oldFlags, u32 newFlags );

            /** Update viewport association. */
            void updateViewport();

            /** Set this camera as the main camera. */
            void setAsMainCamera();

            /** Update projection type based on orthographic flag. */
            void updateProjectionType();

            SmartPtr<ui::IUIWindow> m_uiWindow;

            u8 m_viewportId = 0;
            atomic_u32 m_cameraFlags = 0;
            SmartPtr<IEventListener> m_eventListener;
        };
    }  // namespace scene
}  // namespace workphone

#endif
