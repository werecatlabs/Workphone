#ifndef Win32InputBackend_h__
#define Win32InputBackend_h__

#include <Workphone/Input/Platform/IInputPlatformBackend.hpp>

namespace workphone
{
    /**
     * @brief Win32-specific implementation of the input platform backend.
     */
    class Win32InputBackend : public IInputPlatformBackend
    {
    public:
        Win32InputBackend();
        ~Win32InputBackend() override;

        void initialize( InputDeviceManager &manager ) override;
        void shutdown() override;
        void updateJoysticks( InputDeviceManager &manager ) override;

        bool isKeyPressed( KeyCodes keyCode ) const override;
        bool isMouseButtonDown( u32 button ) const override;
        void setCursorVisible( bool visible ) override;

        void processWindowEvent( InputDeviceManager &manager,
                                 SmartPtr<render::IGraphicsWindowEvent> event ) override;

        SmartPtr<render::IGraphicsWindowListener> createWindowListener(
            InputDeviceManager &manager ) override;
    };

}  // namespace workphone

#endif  // Win32InputBackend_h__
