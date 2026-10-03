#ifndef __RuntimeApplication_h__
#define __RuntimeApplication_h__

#include <WPRuntime/WPRuntimePrerequisites.hpp>
#include <WPRuntime/WPRuntimeConfig.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Interface/System/ICoroutineData.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Application.hpp>
#include <WPRuntime/RuntimeProject.hpp>

namespace workphone
{
    /**
     * @brief Runtime game application.
     *
     * RuntimeApplication is the application entrypoint used by the runtime layer.
     * It owns and coordinates subsystems such as graphics, input and file system,
     * manages scene file path/state, and responds to framework events.
     *
     * The class extends core::Application to integrate with the engine lifecycle
     * (load/unload) and provides convenience helpers for creating the default
     * camera and driving scene load operations via a coroutine.
     */
    class RuntimeApplication : public core::Application
    {
    public:
        /**
         * @brief Input and event listener bound to the RuntimeApplication.
         *
         * ApplicationListener forwards events and raw input to the owning
         * RuntimeApplication instance. The listener holds a weak reference to
         * avoid ownership cycles and allows the application to be destroyed
         * independently of the listener instance.
         */
        class ApplicationListener : public IEventListener
        {
        public:
            /** @brief Construct an ApplicationListener. */
            ApplicationListener();

            /** @brief Virtual destructor. */
            ~ApplicationListener() override;

            /**
             * @brief Handle a framework event.
             *
             * Forwards or handles events coming from the engine/event bus.
             *
             * @param eventType Type of the event.
             * @param eventValue Hashed event identifier / value.
             * @param arguments Event arguments.
             * @param sender Event sender object.
             * @param object Related object, if any.
             * @param event Raw event instance.
             * @return Parameter result produced by the event handling routine.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Handle a raw input event.
             *
             * Called by the input subsystem to notify about input (keyboard, mouse, joystick).
             *
             * @param event Input event object.
             * @return true if the event was consumed/handled; false otherwise.
             */
            bool inputEvent( SmartPtr<IInputEvent> event );

            /**
             * @brief Get the owning RuntimeApplication.
             * @return SmartPtr to the owner RuntimeApplication or null if expired.
             */
            SmartPtr<RuntimeApplication> getOwner() const;

            /**
             * @brief Set the owning RuntimeApplication.
             * @param owner SmartPtr to the RuntimeApplication to associate with this listener.
             */
            void setOwner( SmartPtr<RuntimeApplication> owner );

            WP_CLASS_REGISTER_DECL;

        protected:
            /// Weak pointer to the owning RuntimeApplication to avoid ownership cycles.
            AtomicWeakPtr<RuntimeApplication> m_owner;
        };

        /// Default interval used for thread updates (seconds).
        static const f32 DEFAULT_THREAD_UPDATE;

        /// Default interval used for task updates (seconds).
        static const f32 DEFAULT_TASK_UPDATE;

        /// Event hash constants for initialization and creation commands.
        static const hash_type INITIALISE_GRAPHICS_SYSTEM_HASH;
        static const hash_type INITIALISE_VIDEO_SYSTEM_HASH;
        static const hash_type INITIALISE_INPUT_HASH;
        static const hash_type CREATE_INPUT_HASH;
        static const hash_type CREATE_VIEWPORT_HASH;
        static const hash_type CREATE_CAMERA_HASH;
        static const hash_type CREATE_SCENE_HASH;
        static const hash_type CREATE_SCENE_MANAGER_HASH;
        static const hash_type ON_INPUT_CHANGED_HASH;

        /// Default names used by the runtime when creating scene manager and camera.
        static const String DEFAULT_SCENE_MANAGER_NAME;
        static const String DEFAULT_CAMERA_NAME;

        /// Path to the runtime configuration file (relative or absolute as used by the project).
        static const String configFilePath;

        /** @brief Default constructor. */
        RuntimeApplication();

        /**
         * @brief Construct the runtime application with a project or scene path.
         * @param path Path to the project, scene or configuration file to use on startup.
         */
        RuntimeApplication( const String &path );

        /** @brief Virtual destructor. */
        ~RuntimeApplication() override;

        /**
         * @copydoc core::Application::load
         *
         * Performs runtime-specific initialization in addition to the base class:
         * creating plugins, setting up file system, registering event listeners and
         * queuing initial scene loads.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc core::Application::unload
         *
         * Tears down runtime-managed subsystems, listeners and any loaded scene
         * resources. Ensures proper shutdown ordering for subsystems created in load().
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        String getProjectFilePath() const;

        void setProjectFilePath( const String &projectFilePath );

        /**
         * @brief Get the currently configured scene file path.
         * @return Scene file path as a String. May be empty if not set.
         */
        String getSceneFilePath() const;

        /**
         * @brief Set the scene file path to be loaded by the runtime.
         * @param sceneFilePath Path to the scene file.
         */
        void setSceneFilePath( const String &sceneFilePath );

        /**
         * @brief Coroutine entry used to load a scene in a non-blocking manner.
         *
         * The coroutine pull parameter allows the loader to yield back to the main
         * loop so that long-running scene loads do not block rendering or input.
         *
         * @param pull Coroutine pull handle for yielding/resuming.
         */
        void loadSceneCoroutine( ICoroutineData::PullType &pull );

        /**
         * @brief Handle generic framework events.
         *
         * Called by the event bus; this method routes events relevant to the runtime
         * (initialization, input changes, etc.) to internal handlers.
         *
         * @param eventType Type of the event.
         * @param eventValue Hashed event identifier.
         * @param arguments Event arguments.
         * @param sender Sender of the event.
         * @param object Related object if any.
         * @param event Raw event object.
         * @return Parameter response depending on the handled event.
         */
        Parameter handleEvent( EventType eventType, hash_type eventValue,
                               const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Create and initialise required runtime plugins.
         *
         * Called during application startup to create plugin instances (graphics,
         * input, audio, etc.) that the runtime depends on.
         */
        void createPlugins() override;

        /// Path to the scene file to be loaded by the runtime.
        String m_sceneFilePath;

        /// Path to the current runtime project file.
        String m_projectFilePath;

        /// True once the initial scene-load coroutine has been queued.
        AtomicValue<bool> m_sceneLoadCoroutineQueued = false;
    };
}  // namespace workphone

#endif  // __RuntimeApplication_h__
