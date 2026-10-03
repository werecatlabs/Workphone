#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUICharacterSelectMenu.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    ClawUICharacterSelectMenu::ClawUICharacterSelectMenu() :
        m_curCursorIndex( 0 ),
        m_uiNumMenuItems( 0 ),
        m_uiNumListItems( 0 ),
        m_iItemIdx( 0 )
    {
        setType( "CharacterSelectMenu" );
        m_cursorIdx.resize( 2, 0 );
        m_prevCursorIdx.resize( 2, 0 );
    }

    ClawUICharacterSelectMenu::~ClawUICharacterSelectMenu() = default;

    bool ClawUICharacterSelectMenu::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return isEnabled() && ( inputEvent( event ) || ClawUIElement::handleEvent( event ) );
    }

    bool ClawUICharacterSelectMenu::inputEvent( const SmartPtr<IInputEvent> &event )
    {
        return false;
    }

    void ClawUICharacterSelectMenu::populateMenuItemList()
    {
        m_menuItems = getChildren();
        m_uiNumMenuItems = static_cast<s32>( m_menuItems.size() );
        if( m_uiNumListItems == 0 )
        {
            m_uiNumListItems = m_uiNumMenuItems;
        }
    }

    void ClawUICharacterSelectMenu::increamentCursor( s32 cursorIndex )
    {
        populateMenuItemList();
        if( cursorIndex < 0 || m_menuItems.empty() )
        {
            return;
        }

        const auto index = static_cast<size_t>( cursorIndex );
        if( index >= m_cursorIdx.size() )
        {
            m_cursorIdx.resize( index + 1, 0 );
            m_prevCursorIdx.resize( index + 1, 0 );
        }
        setCursorPosition( static_cast<u32>( index ), static_cast<u32>( m_cursorIdx[index] + 1 ) );
    }

    void ClawUICharacterSelectMenu::decrementCursor( s32 cursorIndex )
    {
        populateMenuItemList();
        if( cursorIndex < 0 || m_menuItems.empty() )
        {
            return;
        }

        const auto index = static_cast<size_t>( cursorIndex );
        if( index >= m_cursorIdx.size() )
        {
            m_cursorIdx.resize( index + 1, 0 );
            m_prevCursorIdx.resize( index + 1, 0 );
        }

        auto next = m_cursorIdx[index] - 1;
        if( next < 0 )
        {
            next = static_cast<s32>( m_menuItems.size() ) - 1;
        }
        setCursorPosition( static_cast<u32>( index ), static_cast<u32>( next ) );
    }

    int ClawUICharacterSelectMenu::getNumMenuItems() const
    {
        return static_cast<s32>( getChildren().size() );
    }

    u32 ClawUICharacterSelectMenu::getCurrentCursorIndex() const
    {
        return m_curCursorIndex;
    }

    void ClawUICharacterSelectMenu::WrapCursor( s32 cursorIndex )
    {
        if( cursorIndex < 0 || static_cast<size_t>( cursorIndex ) >= m_cursorIdx.size() )
        {
            return;
        }

        const auto count = static_cast<s32>( m_menuItems.size() );
        if( count == 0 )
        {
            m_cursorIdx[static_cast<size_t>( cursorIndex )] = 0;
            return;
        }

        auto &cursor = m_cursorIdx[static_cast<size_t>( cursorIndex )];
        while( cursor < 0 )
        {
            cursor += count;
        }
        cursor %= count;
    }

    void ClawUICharacterSelectMenu::setCursorPosition( u32 cursorIdx, u32 cursorPosition )
    {
        populateMenuItemList();
        if( m_menuItems.empty() )
        {
            return;
        }

        if( cursorIdx >= m_cursorIdx.size() )
        {
            m_cursorIdx.resize( cursorIdx + 1, 0 );
            m_prevCursorIdx.resize( cursorIdx + 1, 0 );
        }

        m_curCursorIndex = cursorIdx;
        m_prevCursorIdx[cursorIdx] = m_cursorIdx[cursorIdx];
        m_cursorIdx[cursorIdx] = static_cast<s32>( cursorPosition );
        WrapCursor( static_cast<s32>( cursorIdx ) );

        const auto previous = static_cast<size_t>( m_prevCursorIdx[cursorIdx] );
        if( previous < m_menuItems.size() )
        {
            m_menuItems[previous]->setFocus( false );
            m_menuItems[previous]->setHighlighted( false );
            m_menuItems[previous]->onDeselect();
        }

        auto current = m_menuItems[static_cast<size_t>( m_cursorIdx[cursorIdx] )];
        current->setFocus( true );
        current->setHighlighted( true );
        current->onSelect();
    }

    u32 ClawUICharacterSelectMenu::getCursorPosition( u32 cursorIdx ) const
    {
        return cursorIdx < m_cursorIdx.size()
                   ? static_cast<u32>( MathI::max( m_cursorIdx[cursorIdx], 0 ) )
                   : 0;
    }

    void ClawUICharacterSelectMenu::OnSelectPrevItem()
    {
        decrementCursor( static_cast<s32>( m_curCursorIndex ) );
    }

    void ClawUICharacterSelectMenu::OnSelectNextItem()
    {
        increamentCursor( static_cast<s32>( m_curCursorIndex ) );
    }

    void ClawUICharacterSelectMenu::OnWrapSelectionStart()
    {
    }

    void ClawUICharacterSelectMenu::OnWrapSelectionEnd()
    {
    }

    void ClawUICharacterSelectMenu::onAddChild( IUIElement *child )
    {
        populateMenuItemList();
    }

    void ClawUICharacterSelectMenu::onChildChangedState( IUIElement *child )
    {
        populateMenuItemList();
    }

    void ClawUICharacterSelectMenu::onToggleVisibility()
    {
    }

    void ClawUICharacterSelectMenu::onGainFocus()
    {
    }

    void ClawUICharacterSelectMenu::onLostFocus()
    {
    }

    void ClawUICharacterSelectMenu::setNumListItems( u32 numListItems )
    {
        m_uiNumListItems = static_cast<s32>( numListItems );
    }

    u32 ClawUICharacterSelectMenu::getNumListItems() const
    {
        return static_cast<u32>( MathI::max( m_uiNumListItems, 0 ) );
    }

    void ClawUICharacterSelectMenu::setCurrentItemIndex( u32 index )
    {
        m_iItemIdx = static_cast<s32>( index );
    }

    u32 ClawUICharacterSelectMenu::getCurrentItemIndex() const
    {
        return static_cast<u32>( MathI::max( m_iItemIdx, 0 ) );
    }

    void ClawUICharacterSelectMenu::setPosition( const Vector2F &position )
    {
        ClawUIElement::setPosition( position );
    }

    void ClawUICharacterSelectMenu::draw( struct wp_context *ctx )
    {
        if( ctx && isVisible() && isEnabled() )
        {
            drawWorkphoneChildren( ctx );
        }
    }
}  // namespace workphone::ui
