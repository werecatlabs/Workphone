#ifndef WindowMessageData_h__
#define WindowMessageData_h__

#include <Workphone/Interface/Graphics/IGraphicsWindowEvent.hpp>

namespace workphone
{

    /**
     * @brief Window message data container for inter-thread communication
     *
     * This class encapsulates window message data that needs to be passed between threads,
     * particularly from the main thread to the render thread. It implements the IGraphicsWindowEvent
     * interface and provides a complete message context including window handles, message
     * parameters, and associated event data.
     *
     * The class is designed to work with the WindowMessageQueue system to ensure thread-safe
     * message passing in windowed applications.
     *
     * @see WindowMessageQueue
     * @see render::IGraphicsWindowEvent
     */
    class WPCore_API WindowMessageData : public render::IGraphicsWindowEvent
    {
    public:
        /**
         * @brief Default constructor
         *
         * Initializes all member variables to their default values:
         * - Window handle: nullptr
         * - Event pointer: nullptr
         * - Self pointer: nullptr
         * - wParam: 0
         * - lParam: 0
         * - Message ID: 0
         */
        WindowMessageData();

        /**
         * @brief Virtual destructor
         *
         * Ensures proper cleanup when the object is destroyed through a base class pointer.
         */
        ~WindowMessageData() override;

        /**
         * @brief Gets the window handle associated with this message
         *
         * @return void* The window handle (typically HWND on Windows platforms), or nullptr if not set
         */
        void *getWindowHandle() const;

        /**
         * @brief Sets the window handle for this message
         *
         * @param windowHandle The window handle to associate with this message (typically HWND on
         * Windows)
         */
        void setWindowHandle( void *windowHandle );

        /**
         * @brief Gets the Windows message identifier
         *
         * @return u32 The message ID (e.g., WM_PAINT, WM_KEYDOWN, etc.)
         */
        u32 getMessage() const;

        /**
         * @brief Sets the Windows message identifier
         *
         * @param message The message ID to set (e.g., WM_PAINT, WM_KEYDOWN, etc.)
         */
        void setMessage( u32 message );

        /**
         * @brief Gets the wParam value of the window message
         *
         * The wParam contains additional message-specific information. Its meaning
         * depends on the specific message type.
         *
         * @return size_t The wParam value
         */
        size_t getWParam() const;

        /**
         * @brief Sets the wParam value of the window message
         *
         * @param wParam The wParam value containing message-specific information
         */
        void setWParam( size_t wParam );

        /**
         * @brief Gets the lParam value of the window message
         *
         * The lParam contains additional message-specific information. Its meaning
         * depends on the specific message type.
         *
         * @return size_t The lParam value
         */
        size_t getLParam() const;

        /**
         * @brief Sets the lParam value of the window message
         *
         * @param lparam The lParam value containing message-specific information
         */
        void setLParam( size_t lparam );

        /**
         * @brief Gets the event object associated with this message
         *
         * This can be used to store platform-specific event data or custom event objects
         * that provide additional context for the message.
         *
         * @return void* Pointer to the associated event object, or nullptr if not set
         */
        void *getEvent() const;

        /**
         * @brief Sets the event object associated with this message
         *
         * @param event Pointer to an event object that provides additional context
         */
        void setEvent( void *event );

        /**
         * @brief Gets the self-reference pointer
         *
         * This can be used to store a reference to the object that created this message
         * or any other contextual object reference needed for message processing.
         *
         * @return void* Pointer to the self-reference object, or nullptr if not set
         */
        void *getSelf() const;

        /**
         * @brief Sets the self-reference pointer
         *
         * @param self Pointer to an object that should be associated with this message
         */
        void setSelf( void *self );

        WP_CLASS_REGISTER_DECL;

    private:
        /** @brief Handle to the window that received the message */
        void *m_windowHandle = nullptr;

        /** @brief Pointer to associated event data */
        void *m_event = nullptr;

        /** @brief Self-reference pointer for contextual object association */
        void *m_self = nullptr;

        /** @brief First message parameter (wParam) */
        size_t m_wParam = 0;

        /** @brief Second message parameter (lParam) */
        size_t m_lParam = 0;

        /** @brief Windows message identifier */
        u32 m_message = 0;
    };
}  // namespace workphone

#endif  // WindowMessageData_h__
