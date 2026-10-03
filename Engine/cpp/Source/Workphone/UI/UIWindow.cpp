#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UIWindow.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/UI/IUIMenu.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/State/States/UIWindowStateData.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIWindow, UIElement<IUIWindow> );

        UIWindow::UIWindow() = default;

        UIWindow::~UIWindow() = default;

        void UIWindow::setLabel( const String &label )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIWindowStateData>() )
                {
                    stateData->label = label;
                }
            }
        }

        String UIWindow::getLabel() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIWindowStateData>() )
                {
                    return stateData->label;
                }
            }

            return {};
        }

        void UIWindow::setContextMenu( SmartPtr<IUIMenu> menu )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIWindowStateData>() )
                {
                    stateData->menu = menu;
                }
            }
        }

        SmartPtr<IUIMenu> UIWindow::getContextMenu() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIWindowStateData>() )
                {
                    return stateData->menu;
                }
            }

            return nullptr;
        }

        bool UIWindow::hasBorder() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIWindowStateData>() )
                {
                    return stateData->border;
                }
            }

            return false;
        }

        void UIWindow::setHasBorder( bool border )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIWindowStateData>() )
                {
                    stateData->border = border;
                }
            }
        }

        bool UIWindow::isDocked() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIWindowStateData>() )
                {
                    return stateData->docked;
                }
            }

            return false;
        }

        void UIWindow::setDocked( bool docked )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIWindowStateData>() )
                {
                    stateData->docked = docked;
                }
            }
        }

    }  // namespace ui
}  // namespace workphone
