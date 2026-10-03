#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiMenuBar.hpp>
#include <Workphone/Interface/UI/IUIMenu.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiMenuBar, ImGuiElement<IUIMenubar> );

    ImGuiMenuBar::ImGuiMenuBar() = default;

    ImGuiMenuBar::~ImGuiMenuBar() = default;

    void ImGuiMenuBar::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        m_menus.reserve( 10 );
        m_orientation = Direction::Horizontal;
        m_keyboardNavigationEnabled = true;
        m_autoCloseEnabled = true;
        m_highlightedMenuIndex = -1;

        ImGuiElement<IUIMenubar>::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiMenuBar::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        m_menus.clear();
        m_activeMenu = nullptr;
        m_highlightedMenu = nullptr;

        ImGuiElement<IUIMenubar>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void ImGuiMenuBar::update()
    {
        ScopedLock lock( this );

        if( !isVisible() )
        {
            return;
        }

        bool isMainMenuBar = ( m_orientation == Direction::Horizontal );
        bool menuBarOpen = false;

        if( isMainMenuBar )
        {
            menuBarOpen = ImGui::BeginMainMenuBar();
        }
        else
        {
            menuBarOpen = ImGui::BeginMenuBar();
        }

        if( menuBarOpen )
        {
            // Handle keyboard navigation
            if( m_keyboardNavigationEnabled )
            {
                handleKeyboardNavigation();
            }

            // Render menus
            for( size_t i = 0; i < m_menus.size(); ++i )
            {
                auto menu = m_menus[i];
                if( menu && menu->isVisible() )
                {
                    bool isHighlighted = ( static_cast<s32>( i ) == m_highlightedMenuIndex );

                    // Set style for highlighted menu
                    if( isHighlighted && m_keyboardNavigationEnabled )
                    {
                        ImGui::PushStyleColor( ImGuiCol_Header,
                                               ImGui::GetStyleColorVec4( ImGuiCol_HeaderHovered ) );
                    }

                    // Render the menu
                    menu->update();

                    if( isHighlighted && m_keyboardNavigationEnabled )
                    {
                        ImGui::PopStyleColor();
                    }

                    // Check if this menu is now active (opened)
                    if( ImGui::IsItemHovered() && ImGui::IsMouseClicked( 0 ) )
                    {
                        setActiveMenu( menu );
                    }
                }
            }

            if( isMainMenuBar )
            {
                ImGui::EndMainMenuBar();
            }
            else
            {
                ImGui::EndMenuBar();
            }
        }

        // Auto-close handling
        if( m_autoCloseEnabled && m_activeMenu && !ImGui::IsAnyItemHovered() )
        {
            // Check if we should close the active menu
            static auto lastActiveTime = 0.0;
            auto currentTime = ImGui::GetTime();
            if( currentTime - lastActiveTime > 0.1 )  // Small delay to prevent immediate closing
            {
                if( !hasOpenMenu() )
                {
                    setActiveMenu( nullptr );
                }
            }
        }
    }

    void ImGuiMenuBar::addMenu( SmartPtr<IUIMenu> menu )
    {
        if( !menu )
        {
            return;
        }

        addChild( menu );

        ScopedLock lock( this );
        m_menus.push_back( menu );
    }

    void ImGuiMenuBar::removeMenu( SmartPtr<IUIMenu> menu )
    {
        if( !menu )
        {
            return;
        }

        removeChild( menu );

        ScopedLock lock( this );
        auto it = std::find( m_menus.begin(), m_menus.end(), menu );
        if( it != m_menus.end() )
        {
            // Update indices if we're removing the highlighted or active menu
            auto removedIndex = static_cast<s32>( std::distance( m_menus.begin(), it ) );

            if( m_highlightedMenuIndex == removedIndex )
            {
                m_highlightedMenuIndex = -1;
                m_highlightedMenu = nullptr;
            }
            else if( m_highlightedMenuIndex > removedIndex )
            {
                m_highlightedMenuIndex--;
            }

            if( m_activeMenu == menu )
            {
                m_activeMenu = nullptr;
            }

            m_menus.erase( it );
        }
    }

    Array<SmartPtr<IUIMenu>> ImGuiMenuBar::getMenus() const
    {
        return m_menus.snapshot();
    }

    void ImGuiMenuBar::setMenus( const Array<SmartPtr<IUIMenu>> &menus )
    {
        ScopedLock lock( this );

        // Clear existing menus
        for( auto &menu : m_menus )
        {
            removeChild( menu );
        }

        m_menus = { menus.begin(), menus.end() };

        // Add new menus as children
        for( auto &menu : m_menus )
        {
            addChild( menu );
        }

        // Reset navigation state
        m_highlightedMenuIndex = -1;
        m_highlightedMenu = nullptr;
        m_activeMenu = nullptr;
    }

    SmartPtr<IUIMenu> ImGuiMenuBar::findMenuByLabel( const String &label ) const
    {
        ScopedLock lock( this );

        for( auto &menu : m_menus )
        {
            if( menu && menu->getLabel() == label )
            {
                return menu;
            }
        }

        return nullptr;
    }

    SmartPtr<IUIMenu> ImGuiMenuBar::getActiveMenu() const
    {
        ScopedLock lock( this );
        return m_activeMenu;
    }

    void ImGuiMenuBar::setActiveMenu( SmartPtr<IUIMenu> menu )
    {
        ScopedLock lock( this );

        if( m_activeMenu != menu )
        {
            m_activeMenu = menu;

            // Update highlighted menu to match active menu
            if( menu )
            {
                auto it = std::find( m_menus.begin(), m_menus.end(), menu );
                if( it != m_menus.end() )
                {
                    m_highlightedMenuIndex = static_cast<s32>( std::distance( m_menus.begin(), it ) );
                    m_highlightedMenu = menu;
                }
            }
        }
    }

    SmartPtr<IUIMenu> ImGuiMenuBar::getHighlightedMenu() const
    {
        ScopedLock lock( this );
        return m_highlightedMenu;
    }

    void ImGuiMenuBar::setHighlightedMenu( SmartPtr<IUIMenu> menu )
    {
        ScopedLock lock( this );

        m_highlightedMenu = menu;

        if( menu )
        {
            auto it = std::find( m_menus.begin(), m_menus.end(), menu );
            if( it != m_menus.end() )
            {
                m_highlightedMenuIndex = static_cast<s32>( std::distance( m_menus.begin(), it ) );
            }
            else
            {
                m_highlightedMenuIndex = -1;
            }
        }
        else
        {
            m_highlightedMenuIndex = -1;
        }
    }

    void ImGuiMenuBar::navigateNext()
    {
        ScopedLock lock( this );

        if( m_menus.empty() )
        {
            return;
        }

        m_highlightedMenuIndex++;
        if( m_highlightedMenuIndex >= static_cast<s32>( m_menus.size() ) )
        {
            m_highlightedMenuIndex = 0;
        }

        m_highlightedMenu = m_menus[m_highlightedMenuIndex];
    }

    void ImGuiMenuBar::navigatePrevious()
    {
        ScopedLock lock( this );

        if( m_menus.empty() )
        {
            return;
        }

        m_highlightedMenuIndex--;
        if( m_highlightedMenuIndex < 0 )
        {
            m_highlightedMenuIndex = static_cast<s32>( m_menus.size() ) - 1;
        }

        m_highlightedMenu = m_menus[m_highlightedMenuIndex];
    }

    void ImGuiMenuBar::activateHighlightedMenu()
    {
        ScopedLock lock( this );

        if( m_highlightedMenu )
        {
            setActiveMenu( m_highlightedMenu );
        }
    }

    void ImGuiMenuBar::closeAllMenus()
    {
        ScopedLock lock( this );

        m_activeMenu = nullptr;

        // Close all menu popups
        ImGui::CloseCurrentPopup();
    }

    bool ImGuiMenuBar::isKeyboardNavigationEnabled() const
    {
        ScopedLock lock( this );
        return m_keyboardNavigationEnabled;
    }

    void ImGuiMenuBar::setKeyboardNavigationEnabled( bool enabled )
    {
        ScopedLock lock( this );
        m_keyboardNavigationEnabled = enabled;

        if( !enabled )
        {
            m_highlightedMenuIndex = -1;
            m_highlightedMenu = nullptr;
        }
    }

    bool ImGuiMenuBar::isAutoCloseEnabled() const
    {
        ScopedLock lock( this );
        return m_autoCloseEnabled;
    }

    void ImGuiMenuBar::setAutoCloseEnabled( bool enabled )
    {
        ScopedLock lock( this );
        m_autoCloseEnabled = enabled;
    }

    Direction ImGuiMenuBar::getOrientation() const
    {
        ScopedLock lock( this );
        return m_orientation;
    }

    void ImGuiMenuBar::setOrientation( Direction orientation )
    {
        ScopedLock lock( this );
        m_orientation = orientation;
    }

    bool ImGuiMenuBar::handleKeyboardInput( u32 key, u32 modifiers )
    {
        if( !m_keyboardNavigationEnabled )
        {
            return false;
        }

        bool handled = false;

        switch( key )
        {
        case static_cast<u32>( KeyCodes::KEY_LEFT ):
            if( m_orientation == Direction::Horizontal )
            {
                navigatePrevious();
                handled = true;
            }
            break;

        case static_cast<u32>( KeyCodes::KEY_RIGHT ):
            if( m_orientation == Direction::Horizontal )
            {
                navigateNext();
                handled = true;
            }
            break;

        case static_cast<u32>( KeyCodes::KEY_UP ):
            if( m_orientation == Direction::Vertical )
            {
                navigatePrevious();
                handled = true;
            }
            break;

        case static_cast<u32>( KeyCodes::KEY_DOWN ):
            if( m_orientation == Direction::Vertical )
            {
                navigateNext();
                handled = true;
            }
            break;

        case static_cast<u32>( KeyCodes::KEY_RETURN ):
        case static_cast<u32>( KeyCodes::KEY_SPACE ):
            activateHighlightedMenu();
            handled = true;
            break;

        case static_cast<u32>( KeyCodes::KEY_ESCAPE ):
            closeAllMenus();
            handled = true;
            break;
        }

        return handled;
    }

    bool ImGuiMenuBar::hasOpenMenu() const
    {
        // Check if any ImGui menu is currently open
        return ImGui::IsPopupOpen( "", ImGuiPopupFlags_AnyPopupId );
    }

    s32 ImGuiMenuBar::getHighlightedMenuIndex() const
    {
        ScopedLock lock( this );
        return m_highlightedMenuIndex;
    }

    void ImGuiMenuBar::setHighlightedMenuIndex( s32 index )
    {
        ScopedLock lock( this );

        if( index >= 0 && index < static_cast<s32>( m_menus.size() ) )
        {
            m_highlightedMenuIndex = index;
            m_highlightedMenu = m_menus[index];
        }
        else
        {
            m_highlightedMenuIndex = -1;
            m_highlightedMenu = nullptr;
        }
    }

    void ImGuiMenuBar::handleKeyboardNavigation()
    {
        // Handle automatic keyboard focus based on ImGui input
        if( ImGui::IsKeyPressed( ImGuiKey_LeftArrow ) )
        {
            navigatePrevious();
        }
        else if( ImGui::IsKeyPressed( ImGuiKey_RightArrow ) )
        {
            navigateNext();
        }
        else if( ImGui::IsKeyPressed( ImGuiKey_Enter ) || ImGui::IsKeyPressed( ImGuiKey_Space ) )
        {
            activateHighlightedMenu();
        }
        else if( ImGui::IsKeyPressed( ImGuiKey_Escape ) )
        {
            closeAllMenus();
        }
    }
}  // namespace workphone::ui
