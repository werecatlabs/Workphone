#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include <WPWxWidgets/wxManager.hpp>
#include <Workphone/Interface/UI/IUICursor.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        wxManager::wxManager()
        {
        }

        wxManager::~wxManager()
        {
        }

        SmartPtr<IUIElement> wxManager::addElement( hash64 type )
        {
            return nullptr;
        }

        SmartPtr<IUIElement> wxManager::addElement( SmartPtr<IUIElement> parent, u8 type )
        {
            return nullptr;
        }

        SmartPtr<IUIElement> wxManager::addElement( SmartPtr<IUIElement> parent, u8 type,
                                                    Properties &properties )
        {
            return nullptr;
        }

        void wxManager::clear()
        {
        }

        SmartPtr<IUICursor> wxManager::getCursor() const
        {
            return nullptr;
        }

        SmartPtr<IUIElement> wxManager::findElement( const String &id ) const
        {
            return nullptr;
        }

    }  // end namespace ui
}  // namespace workphone
