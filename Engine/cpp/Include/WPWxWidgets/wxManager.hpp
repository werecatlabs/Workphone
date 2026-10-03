#ifndef wxManager_h__
#define wxManager_h__

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>

namespace workphone
{
    namespace ui
    {

        /** Implementation for wxWidgets. */
        class wxManager : public IUIManager
        {
        public:
            wxManager();
            ~wxManager();

            /** @copydoc IUIManager::addElement */
            SmartPtr<IUIElement> addElement( hash64 type );

            /** @copydoc IUIManager::addElement */
            SmartPtr<IUIElement> addElement( SmartPtr<IUIElement> parent, u8 type );

            /** @copydoc IUIManager::addElement */
            SmartPtr<IUIElement> addElement( SmartPtr<IUIElement> parent, u8 type,
                                             Properties &properties );

            /** @copydoc IUIManager::clear */
            void clear() override;

            /** @copydoc IUIManager::getCursor */
            SmartPtr<IUICursor> getCursor() const override;

            /** @copydoc IUIManager::findElement */
            SmartPtr<IUIElement> findElement( const String &id ) const override;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // wxManager_h__
