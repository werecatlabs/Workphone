#ifndef __CameraManager_H
#define __CameraManager_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class CameraManager
         * @brief Manages scene cameras for editor and runtime usage.
         *
         * CameraManager is responsible for keeping track of all camera actors present
         * in the scene, providing search and enumeration, updating cameras each frame
         * and managing editor-specific resources such as the editor render target.
         *
         * It provides a small thread-safety surface: callers can use the provided
         * lock/try_lock/unlock API to synchronize access to the manager's collections
         * when performing multi-step operations. Camera arrays are stored in a
         * ConcurrentArray to allow safe iteration from multiple threads where possible.
         */
        class WPCore_API CameraManager : public ICameraManager
        {
        public:
            /**
             * @brief Property key for editor render-target texture in serialized data.
             *
             * Used when loading/saving CameraManager configuration from properties.
             */
            static const String editorTextureStr;

            /**
             * @brief Property key for the editor camera reference in serialized data.
             */
            static const String editorCameraStr;

            /** @brief Property key used to list managed cameras in serialized data. */
            static const String camerasStr;

            /** @brief Property key used to indicate a reset operation in serialized data. */
            static const String resetStr;

            /**
             * @brief Construct a CameraManager.
             *
             * The manager starts in State::None and with no cameras registered.
             */
            CameraManager();

            /**
             * @brief Destructor.
             *
             * Releases any editor render-target and clears camera references.
             */
            ~CameraManager() override;

            /**
             * @brief Load configuration and resources from a shared object.
             * @param data Shared object containing initialization data (properties, camera refs).
             *
             * Typical use: called by the resource system to initialize the manager from
             * serialized properties. May create or acquire the editor RTT and set up
             * the editor camera reference.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload data and release resources held by the manager.
             * @param data Optional shared object passed during the unload operation.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Per-frame update.
             *
             * Called each frame to perform camera updates and any editor-related
             * housekeeping (for example updating the editor RTT).
             */
            void update() override;

            /**
             * @brief Register a camera actor with the manager.
             * @param camera Smart pointer to the camera actor to add. If the camera is
             *        already present this is a no-op.
             */
            void addCamera( SmartPtr<IGameActor> camera ) override;

            /**
             * @brief Unregister a camera actor from the manager.
             * @param camera Smart pointer to the camera actor to remove.
             * @return True if the camera was found and removed; false if it was not managed.
             */
            bool removeCamera( SmartPtr<IGameActor> camera ) override;

            /**
             * @brief Find a camera actor by its name.
             * @param name Name to search for. Comparison semantics follow String equality.
             * @return Smart pointer to the found camera actor or a null smart pointer if none.
             */
            SmartPtr<IGameActor> findCamera( const String &name ) const override;

            /**
             * @brief Return a snapshot of all managed cameras.
             * @return Array of SmartPtr<IGameActor> containing the currently managed cameras.
             */
            Array<SmartPtr<IGameActor>> getCameras() const override;

            /**
             * @brief Reset the manager to an empty state.
             *
             * This clears the camera list, releases any editor RTT and resets the
             * state machine to State::None.
             */
            void reset() override;

            /**
             * @brief Get a raw (non-owning) pointer to the editor camera actor.
             * @return Raw pointer to the editor camera actor or nullptr if not assigned.
             */
            IGameActor *getEditorCameraPtr() const override;

            /**
             * @brief Get the editor camera as a smart pointer.
             * @return SmartPtr<IGameActor> referencing the editor camera; may be null.
             */
            SmartPtr<IGameActor> getEditorCamera() const override;

            /**
             * @brief Assign the editor camera actor.
             * @param editorCamera Smart pointer to the camera actor used by editor views.
             */
            void setEditorCamera( SmartPtr<IGameActor> editorCamera ) override;

            /**
             * @brief Return the properties object associated with this manager.
             * @return SmartPtr<Properties> containing persistent configuration properties.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Set or replace the persistent properties object for the manager.
             * @param properties Smart pointer to a Properties object (may be null).
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Return child shared objects owned/managed by this component.
             * @return Array of SmartPtr<ISharedObject> for serialization and traversal.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Check whether the editor camera is currently enabled.
             * @return True when the editor camera is active for rendering/editor preview.
             */
            bool isEditorCameraEnabled() const override;

            /**
             * @brief Enable or disable the camera manager component.
             * @param enabled True to enable, false to disable.
             */
            void setEnabled( bool enabled ) override;

            /**
             * @brief Query whether the manager is enabled.
             * @return True if the manager is enabled and should perform updates.
             */
            bool isEnabled() const override;

            /**
             * @brief Set the manager state (Edit, Play, Reset, etc.).
             * @param state The new state to set.
             *
             * Setting a new state may trigger a reset or other reconfiguration so
             * callers should expect the setState call to have side-effects such as
             * changing which cameras are active.
             */
            void setState( State state ) override;

            /**
             * @brief Retrieve the current state of the camera manager.
             * @return The current State enum value.
             */
            State getState() const override;

            /**
             * @brief Get the editor render-target texture used for preview rendering.
             * @return Smart pointer to the render target texture (may be null).
             */
            SmartPtr<render::ITexture> getEditorRTT() const override;

            /**
             * @brief Set or replace the editor render-target texture.
             * @param editorRTT Smart pointer to the new render target texture.
             */
            void setEditorRTT( SmartPtr<render::ITexture> editorRTT ) override;

            /**
             * @brief Acquire the manager's recursive mutex for exclusive access.
             *
             * Use this when performing compound operations that must not be
             * interleaved with updates from other threads.
             */
            void lock() override;

            /**
             * @brief Try to acquire the manager's mutex without blocking.
             * @return True if the lock was successfully acquired.
             */
            bool try_lock() override;

            /**
             * @brief Release the previously acquired mutex.
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        private:
            /**
             * @brief Editor render-target used to draw the editor camera's view.
             *
             * The texture is owned by the manager as a smart pointer and may be null
             * when no editor preview is active.
             */
            AtomicSmartPtr<render::ITexture> m_editorRTT;

            /** @brief Smart pointer to the actor used as the editor camera (may be null). */
            AtomicSmartPtr<IGameActor> m_editorCamera;

            /** @brief Collection of cameras currently managed by this component. */
            ConcurrentArray<SmartPtr<IGameActor>> m_cameras;

            /** @brief Current manager state (None, Edit, Play, Reset, ...). */
            State m_state = State::None;

            /** @brief Enabled flag indicating whether the manager should perform updates. */
            atomic_bool m_enabled = true;

            /** @brief Mutex for synchronizing access to the manager's collections. */
            RecursiveMutex m_mutex;
        };
    }  // namespace scene
}  // namespace workphone

#endif
