#ifndef __IGraphicsCameraManager_H
#define __IGraphicsCameraManager_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class ICameraManager
         * @brief Abstract interface for a scene camera manager.
         *
         * Implementations of ICameraManager are responsible for tracking camera actors in a
         * scene, providing lookup and enumeration, and managing editor-specific features such as
         * an editor camera and an editor render target texture (RTT). The interface focuses on
         * API contracts; concrete behaviour (thread-safety, lifetime rules, event emission)
         * depends on the implementation.
         */
        class WPCore_API ICameraManager : public ISharedObject
        {
        public:
            /**
             * @brief Enumerates the possible states of the camera manager.
             */
            enum class State
            {
                None,  /**< No state is set. */
                Edit,  /**< Edit mode: manager should configure cameras for scene editing. */
                Play,  /**< Play mode: manager should configure cameras for runtime/simulation. */
                Reset, /**< Reset state: trigger camera(s) to return to initial configuration. */
                Count  /**< Number of valid states (helper value). */
            };

            /**
             * @brief Reserved flag value for cameras.
             *
             * Concrete implementations may use this constant as a mask or sentinel when
             * allocating camera-specific flags. Value is defined in the implementation.
             */
            static const u32 CameraFlagReserved;

            /**
             * @brief Virtual destructor.
             *
             * Implementations must ensure that any held resources are released on
             * destruction. Override to implement cleanup behaviour.
             */
            ~ICameraManager() override;

            /**
             * @brief Register a camera actor with the manager.
             * @param camera Smart pointer to the camera actor to register.
             *
             * After a successful call the manager is responsible for tracking the camera.
             * Implementations may ignore duplicate adds or reorder internal lists. The
             * caller retains ownership of the camera via the SmartPtr passed in.
             */
            virtual void addCamera( SmartPtr<IGameActor> camera ) = 0;

            /**
             * @brief Unregister a camera actor from the manager.
             * @param camera Smart pointer to the camera actor to remove.
             * @return True if the camera was found and removed; false if it was not managed.
             *
             * Note: Removal does not destroy the camera actor; it merely removes it from
             * the manager's internal tracking structures.
             */
            virtual bool removeCamera( SmartPtr<IGameActor> camera ) = 0;

            /**
             * @brief Find a camera by its name.
             * @param name Name of the camera to search for.
             * @return SmartPtr<IGameActor> to the camera if found, otherwise a null smart pointer.
             *
             * Implementations should define name-matching semantics (case sensitivity, full
             * vs partial match). The caller should assume the returned pointer may be
             * concurrently invalidated by other threads unless external synchronization is used.
             */
            virtual SmartPtr<IGameActor> findCamera( const String &name ) const = 0;

            /**
             * @brief Get a snapshot of all cameras currently managed.
             * @return Array of SmartPtr<IGameActor> containing the cameras.
             *
             * The returned array is a copy (snapshot) of the manager's internal list and
             * safe to iterate without holding manager locks. It may become stale if
             * cameras are added/removed by other threads.
             */
            virtual Array<SmartPtr<IGameActor>> getCameras() const = 0;

            /**
             * @brief Reset managed camera(s) to their initial configuration.
             *
             * Calling reset typically causes cameras to return to a stored default
             * transform/state. Implementations may broadcast reset events to cameras
             * or perform internal reconfiguration as required.
             */
            virtual void reset() = 0;

            /**
             * @brief Get a raw (non-owning) pointer to the editor camera actor.
             * @return Raw pointer to the editor camera actor, or nullptr if none assigned.
             *
             * The caller must not assume ownership; the manager retains responsibility for
             * the camera's lifetime. Use getEditorCamera() for an owning smart pointer.
             */
            virtual IGameActor *getEditorCameraPtr() const = 0;

            /**
             * @brief Get the editor camera as a smart pointer.
             * @return SmartPtr<IGameActor> referencing the editor camera; may be null.
             */
            virtual SmartPtr<IGameActor> getEditorCamera() const = 0;

            /**
             * @brief Assign the editor camera actor used by editor views/previews.
             * @param editorCamera Smart pointer to the camera to use as the editor camera.
             *
             * Passing a null pointer clears the editor camera reference.
             */
            virtual void setEditorCamera( SmartPtr<IGameActor> editorCamera ) = 0;

            /**
             * @brief Query whether the editor camera is enabled for preview rendering.
             * @return True when the editor camera is active and should be rendered in editor UI.
             */
            virtual bool isEditorCameraEnabled() const = 0;

            /**
             * @brief Enable or disable the camera manager component.
             * @param enabled True to enable, false to disable.
             *
             * When disabled the manager may skip updates and stop responding to events.
             */
            virtual void setEnabled( bool enabled ) = 0;

            /**
             * @brief Query whether the camera manager is enabled.
             * @return True if enabled; false otherwise.
             */
            virtual bool isEnabled() const = 0;

            /**
             * @brief Set the camera manager's state.
             * @param state New state value (Edit, Play, Reset, etc.).
             *
             * Implementations may react to state transitions by changing which cameras
             * are active or by resetting camera transforms.
             */
            virtual void setState( State state ) = 0;

            /**
             * @brief Get the current manager state.
             * @return The current State enum value.
             */
            virtual State getState() const = 0;

            /**
             * @brief Get the editor render-target texture used for preview rendering.
             * @return Smart pointer to the texture or null if none assigned.
             */
            virtual SmartPtr<render::ITexture> getEditorRTT() const = 0;

            /**
             * @brief Set the editor render-target texture used for preview rendering.
             * @param editorRTT Smart pointer to the render target texture (may be null).
             */
            virtual void setEditorRTT( SmartPtr<render::ITexture> editorRTT ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace scene
}  // namespace workphone

#endif
