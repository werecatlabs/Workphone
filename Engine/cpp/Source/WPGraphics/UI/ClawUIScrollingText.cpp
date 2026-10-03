#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIScrollingText.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    ClawUIScrollingText::ClawUIScrollingText() :
        m_uiNumMenuItems( 0 ),
        m_uiNumListItems( 0 ),
        m_iItemIdx( 0 ),
        m_cursorIdx( 0 ),
        m_prevCursorIdx( 0 )
    {
        setType( "ScrollingText" );
    }

    ClawUIScrollingText::~ClawUIScrollingText() = default;

    bool ClawUIScrollingText::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return isEnabled() && ClawUIElement::handleEvent( event );
    }

    void ClawUIScrollingText::populateMenuItemList()
    {
        m_elements = getChildren();
        m_uiNumMenuItems = static_cast<s32>( m_elements.size() );
        if( m_uiNumListItems == 0 )
        {
            m_uiNumListItems = m_uiNumMenuItems;
        }
        WrapCursor();
    }

    void ClawUIScrollingText::increamentCursor()
    {
        populateMenuItemList();
        if( m_elements.empty() )
        {
            return;
        }

        m_prevCursorIdx = m_cursorIdx++;
        WrapCursor();
        setCursorPosition( static_cast<u32>( m_cursorIdx ) );
    }

    void ClawUIScrollingText::decrementCursor()
    {
        populateMenuItemList();
        if( m_elements.empty() )
        {
            return;
        }

        m_prevCursorIdx = m_cursorIdx--;
        WrapCursor();
        setCursorPosition( static_cast<u32>( m_cursorIdx ) );
    }

    int ClawUIScrollingText::getNumMenuItems() const
    {
        return static_cast<s32>( getChildren().size() );
    }

    void ClawUIScrollingText::WrapCursor()
    {
        const auto count = static_cast<s32>( m_elements.size() );
        if( count <= 0 )
        {
            m_cursorIdx = 0;
            return;
        }
        while( m_cursorIdx < 0 )
        {
            m_cursorIdx += count;
        }
        m_cursorIdx %= count;
    }

    void ClawUIScrollingText::setCursorPosition( u32 cursorIdx )
    {
        populateMenuItemList();
        if( m_elements.empty() )
        {
            return;
        }

        if( m_prevCursorIdx >= 0 && m_prevCursorIdx < static_cast<s32>( m_elements.size() ) )
        {
            auto previous = m_elements[static_cast<size_t>( m_prevCursorIdx )];
            previous->setFocus( false );
            previous->setHighlighted( false );
            previous->onDeselect();
        }

        m_prevCursorIdx = m_cursorIdx;
        m_cursorIdx = static_cast<s32>( cursorIdx );
        WrapCursor();

        auto current = m_elements[static_cast<size_t>( m_cursorIdx )];
        current->setFocus( true );
        current->setHighlighted( true );
        current->onSelect();
    }

    u32 ClawUIScrollingText::getCursorPosition() const
    {
        return static_cast<u32>( MathI::max( m_cursorIdx, 0 ) );
    }

    void ClawUIScrollingText::setNumListItems( u32 numListItems )
    {
        m_uiNumListItems = static_cast<s32>( numListItems );
    }

    u32 ClawUIScrollingText::getNumListItems() const
    {
        return static_cast<u32>( MathI::max( m_uiNumListItems, 0 ) );
    }

    void ClawUIScrollingText::setCurrentItemIndex( u32 index )
    {
        m_iItemIdx = static_cast<s32>( index );
    }

    u32 ClawUIScrollingText::getCurrentItemIndex() const
    {
        return static_cast<u32>( MathI::max( m_iItemIdx, 0 ) );
    }

    void ClawUIScrollingText::draw( struct wp_context *ctx )
    {
        if( ctx && isVisible() && isEnabled() )
        {
            drawWorkphoneChildren( ctx );
        }
    }
}  // namespace workphone::ui
