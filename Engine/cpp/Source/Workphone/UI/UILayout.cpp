#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UILayout.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UILayout, UIElement<IUILayoutWindow> );

        UILayout::UILayout() : UIElement<IUILayoutWindow>( UILayout::typeInfo() )
        {
        }

        UILayout::UILayout( u32 poolTypeId ) : UIElement<IUILayoutWindow>( poolTypeId )
        {
        }

        UILayout::~UILayout() = default;

        IUILayoutWindow::LayoutStates UILayout::getState()
        {
            return m_state;
        }

        void UILayout::setState( LayoutStates state )
        {
            m_state = state;
        }

        SmartPtr<IUIWindow> UILayout::getParentWindow() const
        {
            return m_uiWindow;
        }

        void UILayout::setParentWindow( SmartPtr<IUIWindow> uiWindow )
        {
            m_uiWindow = uiWindow;
        }

        void UILayout::invalidate()
        {
        }

        IUILayoutWindow::WindowFlags UILayout::getWindowFlags() const
        {
            return m_windowFlags;
        }

        void UILayout::setWindowFlags( WindowFlags flags )
        {
            m_windowFlags = flags;
        }

        bool UILayout::hasWindowFlag( WindowFlags flag ) const
        {
            return ( m_windowFlags & flag ) == flag;
        }
    }  // namespace ui
}  // namespace workphone
