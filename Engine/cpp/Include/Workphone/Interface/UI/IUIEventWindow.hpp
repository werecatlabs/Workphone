#ifndef IUIEventWindow_h__
#define IUIEventWindow_h__

#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace ui
    {

        class WPCore_API IUIEventWindow : public IUIWindow
        {
        public:
            IUIEventWindow() : IUIWindow( IUIEventWindow::typeInfo() )
            {
            }

            IUIEventWindow( u32 poolTypeId ) : IUIWindow( poolTypeId )
            {
            }

            /** Destructor. */
            ~IUIEventWindow() override;

            /** Gets the events. */
            virtual Array<SmartPtr<IEvent>> getEvents() const = 0;

            /** Sets the events. */
            virtual void setEvents( const Array<SmartPtr<IEvent>> &events ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIEvent_h__
