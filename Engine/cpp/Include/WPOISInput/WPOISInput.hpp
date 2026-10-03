#ifndef WPOISInput_h__
#define WPOISInput_h__

#include <WPOISInput/WPOISInputPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    class WPOISInput_API OISInput : public ISharedObject
    {
    public:
        OISInput();
        ~OISInput() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        static SmartPtr<IInputDeviceManager> createInputManager(
            SmartPtr<render::IGraphicsWindow> window );

        static SmartPtr<OISInput> instance();
        static void setInstance( SmartPtr<OISInput> plugin );

    protected:
        static SmartPtr<OISInput> m_sPlugin;
    };
}  // namespace workphone

#endif  // WPOISInput_h__
