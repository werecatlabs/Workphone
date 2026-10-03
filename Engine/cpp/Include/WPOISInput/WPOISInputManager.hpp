#ifndef __WP_OISInputManager_H__
#define __WP_OISInputManager_H__

#include <WPOISInput/WPOISInputPrerequisites.hpp>
#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#include <Workphone/Memory/RawPtr.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <WPOISInput/Extern/OIS/OISKeyboard.h>
#include <WPOISInput/Extern/OIS/OISJoyStick.h>
#include <WPOISInput/Extern/OIS/OISMouse.h>

#ifdef WP_PLATFORM_WIN32
#    include <Windows.h>
#    include <WinUser.h>
#endif

namespace workphone
{
    class WPOISInput_API OISInputManager : public InputDeviceManager
    {
    public:
        class KeyListener : public OIS::KeyListener
        {
        public:
            KeyListener();
            ~KeyListener() override;

            bool keyPressed( const OIS::KeyEvent &arg ) override;
            bool keyReleased( const OIS::KeyEvent &arg ) override;

            OISInputManager *getOwner() const;

            void setOwner( OISInputManager *owner );

        protected:
            OISInputManager *m_owner = nullptr;
        };

        class MouseListener : public OIS::MouseListener
        {
        public:
            MouseListener();
            ~MouseListener() override;

            bool mouseMoved( const OIS::MouseEvent &arg ) override;

            bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id ) override;
            bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id ) override;

            OISInputManager *getOwner() const;

            void setOwner( OISInputManager *owner );

        protected:
            OISInputManager *m_owner = nullptr;
        };

        class JoyStickListener : public OIS::JoyStickListener
        {
        public:
            JoyStickListener();
            ~JoyStickListener() override;

            bool povMoved( const OIS::JoyStickEvent &arg, int index ) override;

            bool vector3Moved( const OIS::JoyStickEvent &arg, int index ) override;

            bool buttonPressed( const OIS::JoyStickEvent &arg, int button ) override;

            bool sliderMoved( const OIS::JoyStickEvent &arg, int index ) override;

            bool axisMoved( const OIS::JoyStickEvent &arg, int axis ) override;

            bool buttonReleased( const OIS::JoyStickEvent &arg, int button ) override;

            OISInputManager *getOwner() const;

            void setOwner( OISInputManager *owner );

        protected:
            OISInputManager *m_owner = nullptr;
        };

        class WindowListener : public render::IGraphicsWindowListener
        {
        public:
            WindowListener();
            ~WindowListener() override;

            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            void handleEvent( SmartPtr<render::IGraphicsWindowEvent> event ) override;

            void setOwner( OISInputManager *owner );
            OISInputManager *getOwner() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            OISInputManager *m_owner = nullptr;
        };

        /** Default constructor. */
        OISInputManager();

        // Constructor takes a RenderWindow because it uses that to determine input context
        OISInputManager( SmartPtr<render::IGraphicsWindow> win, bool bufferedKeys = true,
                         bool bufferedMouse = true, bool bufferedJoy = true );

        /** Destructor. */
        ~OISInputManager() override;

        /** @copydoc InputDeviceManager::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc InputDeviceManager::load */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc InputDeviceManager::update */
        void update() override;

        /** @copydoc InputDeviceManager::postEvent */
        bool postEvent( SmartPtr<IInputEvent> event ) override;

        /** @copydoc InputDeviceManager::createInputEvent */
        SmartPtr<IInputEvent> createInputEvent() override;

        /** @copydoc InputDeviceManager::createMouseState */
        SmartPtr<IMouseState> createMouseState() override;

        /** @copydoc InputDeviceManager::createKeyboardState */
        SmartPtr<IKeyboardState> createKeyboardState() override;

        SmartPtr<IInputEvent> getCurrentInputEvent() const override;
        SmartPtr<IMouseState> getCurrentMouseState() const override;
        SmartPtr<IKeyboardState> getCurrentKeyboardState() const override;

        SmartPtr<IGameInput> addGameInput( hash_type id ) override;
        SmartPtr<IGameInput> findGameInput( hash_type id ) const override;
        Array<SmartPtr<IGameInput>> getGameInputs() const override;

        bool isCursorVisible() const override;
        void setCursorVisible( bool visible ) override;

        bool isMouseButtonDown( u32 button ) const override;

        OIS::InputManager *getInputManager() const;

        void setInputManager( OIS::InputManager *inputManager );

        OIS::Mouse *getMouse() const;
        void setMouse( OIS::Mouse *mouse );

        OIS::Keyboard *getKeyboard() const;
        void setKeyboard( OIS::Keyboard *keyboard );

        Array<RawPtr<OIS::JoyStick>> getJoySticks() const;
        void setJoySticks( const Array<RawPtr<OIS::JoyStick>> &joySticks );

        Array<RawPtr<OIS::ForceFeedback>> getForceFeedbackDevices() const;
        void setForceFeedbackDevices( const Array<RawPtr<OIS::ForceFeedback>> &forceFeedbackDevices );

        void setUserData( void *ptr1, void *ptr2 );
        void *getUserData( void *ptr ) const;

        void triggerEvent( SmartPtr<IInputEvent> inputEvent ) override;

        void queueEvent( SmartPtr<IInputEvent> event ) override;

        void addListener( SmartPtr<IEventListener> listener ) override;

        void removeListener( SmartPtr<IEventListener> listener ) override;

        void removeListeners() override;

        Vector3<real_Num> getMouseScroll() const override;

        bool isShiftPressed() const override;

        void setShiftPressed( bool shiftPressed ) override;

        f64 getLastClickTime() const override;

        void setLastClickTime( f64 lastClickTime ) override;

        f64 getDoubleClickInterval() const override;

        void setDoubleClickInterval( f64 doubleClickInterval ) override;

        f64 getLastInputTime() const override;

        void setLastInputTime( f64 lastInputTime ) override;

        f64 getInputTime() const override;

        void setInputTime( f64 inputTime ) override;

        void addInputTime( f64 inputTime ) override;

        bool isKeyPressed( KeyCodes keyCode ) const override;

        bool getCreateMouse() const override;
        void setCreateMouse( bool createMouse ) override;

        bool getCreateKeyboard() const override;
        void setCreateKeyboard( bool createKeyboard ) override;

        bool getCreateJoysticks() const override;
        void setCreateJoysticks( bool createJoysticks ) override;

        SmartPtr<render::IGraphicsWindow> getWindow() const override;
        void setWindow( SmartPtr<render::IGraphicsWindow> window ) override;

        Array<SmartPtr<IJoystick>> getJoysticks() const override;

        void setJoysticks( const Array<SmartPtr<IJoystick>> &joysticks ) override;

        void _getObject( void **ppObject ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        // mouse events
        bool mouseMoved( const OIS::MouseEvent &arg );
        bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
        bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );

        // keyboard events
        bool keyPressed( const OIS::KeyEvent &arg );
        bool keyReleased( const OIS::KeyEvent &arg );

        // joystick events
        bool povMoved( const OIS::JoyStickEvent &arg, int index );
        bool vector3Moved( const OIS::JoyStickEvent &arg, int index );
        bool buttonPressed( const OIS::JoyStickEvent &arg, int button );
        bool sliderMoved( const OIS::JoyStickEvent &, int index );
        bool axisMoved( const OIS::JoyStickEvent &arg, int axis );
        bool buttonReleased( const OIS::JoyStickEvent &arg, int button );

        void setJoystickIdx( const String &gameInputDeviceName, s32 joystickIdx );
        s32 getJoystickIdx( const String &gameInputDeviceName ) const;

#ifdef WP_PLATFORM_WIN32
        bool registerRawInput( HWND hwnd );
#endif

        Vector2F m_mousePosition = Vector2F::zero();
        Vector3<real_Num> m_mouseScroll = Vector3<real_Num>::zero();

        Array<ConcurrentQueue<SmartPtr<IInputEvent>>> m_inputEventQueue;

        SmartPtr<IInputEvent> m_currentInputEvent;
        SmartPtr<IMouseState> m_currentMouseState;
        SmartPtr<IKeyboardState> m_currentKeyboardState;

        SharedPtr<KeyListener> m_keyListener;
        SharedPtr<MouseListener> m_mouseListener;
        SharedPtr<JoyStickListener> m_joyStickListener;

        ///
        RawPtr<OISKeyConverter> m_keyConverter;

        ///
        RawPtr<OIS::InputManager> m_inputManager;

        ///
        RawPtr<OIS::Mouse> m_mouse;

        ///
        RawPtr<OIS::Keyboard> m_keyboard;

        ///
        Array<RawPtr<OIS::JoyStick>> m_oisJoysticks;

        Array<SmartPtr<IJoystick>> m_joysticks;

        ///
        Array<RawPtr<OIS::ForceFeedback>> m_forceFeedbackDevices;

        ConcurrentArray<SmartPtr<IEventListener>> m_listeners;

        using GameDeviceJoystickMap = std::map<String, s32>;
        GameDeviceJoystickMap m_gameDeviceJoystickMap;

        SmartPtr<render::IGraphicsWindow> m_window;
        SmartPtr<render::IGraphicsWindowListener> m_windowListener;

        u32 m_numSticks = 0;

        s32 m_lastWheelPos = 0;

        OIS::MouseState m_initialState;

        SmartPtr<PlatformInputManager> m_platformInputManager;

        using GameInputs = HashMap<hash_type, SmartPtr<IGameInput>>;
        GameInputs m_gameInputs;

        using UserData = HashMap<void *, void *>;
        UserData m_userData;
    };
}  // namespace workphone

#endif
