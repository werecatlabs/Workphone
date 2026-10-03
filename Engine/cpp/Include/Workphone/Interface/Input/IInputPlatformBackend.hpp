#ifndef IInputPlatformBackend_h__
#define IInputPlatformBackend_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    // Forward declarations
    class InputDeviceManager;

    /// Forward declaration of KeyCodes (defined in WorkphoneEnums.hpp).
    enum class KeyCodes;

    /**
     * @class IInputPlatformBackend
     * @brief Abstract interface for platform-specific input backend operations.
     *
     * The input platform backend encapsulates all OS-dependent input handling:
     * joystick/controller polling, keyboard and mouse button state queries,
     * cursor visibility control, and native window event processing.
     *
     * The concrete InputDeviceManager delegates to this interface so the core
     * manager logic remains portable across Win32, Linux, macOS, etc.
     */
    class IInputPlatformBackend
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IInputPlatformBackend() = default;

        /**
         * @brief Initialize the backend and create platform-specific device objects.
         *
         * Called during InputDeviceManager::load().  The backend should create
         * joysticks, attach window listeners, and set up any required OS hooks.
         *
         * @param manager The owning InputDeviceManager (non-owning reference).
         */
        virtual void initialize( InputDeviceManager &manager ) = 0;

        /**
         * @brief Tear down the backend, releasing platform resources.
         *
         * Called during InputDeviceManager::unload().  Listeners should be
         * detached and any OS handles released.
         */
        virtual void shutdown() = 0;

        /**
         * @brief Poll all connected joysticks/controllers and dispatch events.
         *
         * Called once per InputDeviceManager::update().  The backend should
         * read controller state, compare to previous state, and post
         * IInputEvent objects for any changes.
         *
         * @param manager The owning InputDeviceManager for event posting.
         */
        virtual void updateJoysticks( InputDeviceManager &manager ) = 0;

        /**
         * @brief Query whether a key is currently pressed.
         *
         * @param keyCode Platform-agnostic key code.
         * @return true if the key is down, false otherwise (or unsupported platform).
         */
        virtual bool isKeyPressed( KeyCodes keyCode ) const = 0;

        /**
         * @brief Query whether a mouse button is currently down.
         *
         * @param button 0-based button index (0=left, 1=right, 2=middle, ...).
         * @return true if the button is down, false otherwise.
         */
        virtual bool isMouseButtonDown( u32 button ) const = 0;

        /**
         * @brief Show or hide the system cursor.
         *
         * @param visible True to show the cursor, false to hide it.
         */
        virtual void setCursorVisible( bool visible ) = 0;

        /**
         * @brief Process a platform window event and dispatch input events.
         *
         * Called by the WindowListener for each event received from the
         * graphics window.  The backend interprets the OS-specific message
         * data and posts appropriate input events to the manager.
         *
         * @param manager The owning InputDeviceManager for event posting.
         * @param event The graphics window event to process.
         */
        virtual void processWindowEvent( InputDeviceManager &manager,
                                         SmartPtr<render::IGraphicsWindowEvent> event ) = 0;

        /**
         * @brief Create the platform-specific window listener.
         *
         * @param manager The owning InputDeviceManager.
         * @return SmartPtr<IGraphicsWindowListener> The created listener, or null.
         */
        virtual SmartPtr<render::IGraphicsWindowListener> createWindowListener(
            InputDeviceManager &manager ) = 0;
    };

}  // namespace workphone

#endif  // IInputPlatformBackend_h__
