#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UIManager.hpp>
#include <Workphone/UI/UILayout.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Interface/UI/IUIApplication.hpp>
#include <Workphone/Interface/UI/IUICursor.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>

namespace workphone
{
    namespace ui
    {

        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIManager, IUIManager );

        UIManager::UIManager() = default;

        UIManager::~UIManager() = default;

        size_t UIManager::messagePump( SmartPtr<ISharedObject> data )
        {
            return 0;
        }

        void UIManager::render()
        {
        }

        SmartPtr<IUIApplication> UIManager::addApplication()
        {
            return nullptr;
        }

        void UIManager::removeApplication( SmartPtr<IUIApplication> application )
        {
        }

        IUIApplication *UIManager::getApplicationPtr() const
        {
            return nullptr;
        }

        SmartPtr<IUIApplication> UIManager::getApplication() const
        {
            return nullptr;
        }

        void UIManager::setApplication( SmartPtr<IUIApplication> application )
        {
        }

        SmartPtr<IUIElement> UIManager::addElement( hash64 type )
        {
            return nullptr;
        }

        void UIManager::removeElement( SmartPtr<IUIElement> element )
        {
        }

        void UIManager::removeElements( const Array<SmartPtr<IUIElement>> &elementsToRemove )
        {
        }

        Array<SmartPtr<IUIElement>> UIManager::getElements() const
        {
            return m_elements.snapshot();
        }

        void UIManager::clear()
        {
        }

        SmartPtr<IUICursor> UIManager::getCursor() const
        {
            return m_cursor;
        }

        SmartPtr<IUIElement> UIManager::findElement( const String &id ) const
        {
            if( id.empty() )
            {
                return nullptr;
            }

            for( auto element : m_elements )
            {
                if( element->getName() == id )
                {
                    return element;
                }
            }

            return nullptr;
        }

        bool UIManager::isDragging() const
        {
            return m_dragging;
        }

        void UIManager::setDragging( bool dragging )
        {
            m_dragging = dragging;
        }

        SmartPtr<IUIWindow> UIManager::getMainWindow() const
        {
            return m_mainWindow;
        }

        void UIManager::setMainWindow( SmartPtr<IUIWindow> uiWindow )
        {
            m_mainWindow = uiWindow;
        }

        void UIManager::invalidate()
        {
        }

        void UIManager::_getObject( void **ppObject )
        {
        }

        void UIManager::loadObject( SmartPtr<ISharedObject> object, bool forceQueue /*= false */ )
        {
        }

        void UIManager::unloadObject( SmartPtr<ISharedObject> object, bool forceQueue /*= false */ )
        {
        }
    }  // namespace ui
}  // namespace workphone
