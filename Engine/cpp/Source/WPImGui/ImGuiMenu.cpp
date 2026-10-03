#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiMenu.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiMenu, ImGuiElement<IUIMenu> );

    ImGuiMenu::ImGuiMenu() = default;

    ImGuiMenu::~ImGuiMenu() = default;

    void ImGuiMenu::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_menuItems.reserve( 32 );
        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiMenu::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        m_menuItems.clear();
        ImGuiElement<IUIMenu>::unload( data );

        setLoadingState( LoadingState::Unloaded );
    }

    void ImGuiMenu::update()
    {
        for( auto &menuItem : m_menuItems )
        {
            menuItem->update();
        }
    }

    void ImGuiMenu::addMenuItem( SmartPtr<IUIElement> item )
    {
        m_menuItems.push_back( item );
    }

    void ImGuiMenu::removeMenuItem( SmartPtr<IUIElement> item )
    {
        auto it = std::find( m_menuItems.begin(), m_menuItems.end(), item );
        if( it != m_menuItems.end() )
        {
            m_menuItems.erase( it );
        }
    }

    Array<SmartPtr<IUIElement>> ImGuiMenu::getMenuItems() const
    {
        return m_menuItems.snapshot();
    }

    void ImGuiMenu::setMenuItems( const Array<SmartPtr<IUIElement>> &menuItems )
    {
        m_menuItems = { menuItems.begin(), menuItems.end() };
    }

    void ImGuiMenu::setCursorPosition( u32 cursorIdx )
    {
        // Clamp cursor position to valid range
        auto numItems = static_cast<u32>( m_menuItems.size() );
        if( numItems > 0 )
        {
            m_cursorPosition = std::min( cursorIdx, numItems - 1 );
        }
        else
        {
            m_cursorPosition = 0;
        }
    }

    u32 ImGuiMenu::getCursorPosition() const
    {
        return m_cursorPosition;
    }

    void ImGuiMenu::setNumListItems( u32 numListItems )
    {
        m_numListItems = numListItems;
    }

    u32 ImGuiMenu::getNumListItems() const
    {
        return m_numListItems;
    }

    void ImGuiMenu::setCurrentItemIndex( u32 index )
    {
        // Clamp current item index to valid range
        auto numItems = static_cast<u32>( m_menuItems.size() );
        if( numItems > 0 )
        {
            m_currentItemIndex = std::min( index, numItems - 1 );
        }
        else
        {
            m_currentItemIndex = 0;
        }
    }

    u32 ImGuiMenu::getCurrentItemIndex() const
    {
        return m_currentItemIndex;
    }

    void ImGuiMenu::incrementCursor()
    {
        auto numItems = static_cast<u32>( m_menuItems.size() );
        if( numItems > 0 )
        {
            m_cursorPosition = ( m_cursorPosition + 1 ) % numItems;
        }
    }

    void ImGuiMenu::decrementCursor()
    {
        auto numItems = static_cast<u32>( m_menuItems.size() );
        if( numItems > 0 )
        {
            if( m_cursorPosition == 0 )
            {
                m_cursorPosition = numItems - 1;
            }
            else
            {
                m_cursorPosition--;
            }
        }
    }

    s32 ImGuiMenu::getNumMenuItems() const
    {
        return static_cast<s32>( m_menuItems.size() );
    }

    String ImGuiMenu::getLabel() const
    {
        return m_label;
    }

    void ImGuiMenu::setLabel( const String &label )
    {
        m_label = label;
    }
}  // namespace workphone::ui
