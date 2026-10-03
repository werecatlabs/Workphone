#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIMenu.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUIMenu::ClawUIMenu() :
        m_uiNumMenuItems( 0 ),
        m_uiNumListItems( 0 ),
        m_iItemIdx( 0 ),
        m_cursorIdx( 0 ),
        m_prevCursorIdx( 0 )
    {
        setType( "Menu" );
    }

    ClawUIMenu::~ClawUIMenu() = default;

    bool ClawUIMenu::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return isEnabled() && ClawUIElement::handleEvent( event );
    }

    void ClawUIMenu::populateMenuItemList()
    {
        m_aMenuItemList = getChildren();
        m_uiNumMenuItems = static_cast<s32>( m_aMenuItemList.size() );
        if( m_uiNumListItems == 0 )
        {
            m_uiNumListItems = m_uiNumMenuItems;
        }
        WrapCursor();
    }

    void ClawUIMenu::incrementCursor()
    {
        populateMenuItemList();
        if( m_aMenuItemList.empty() )
        {
            return;
        }

        m_prevCursorIdx = m_cursorIdx;
        for( size_t attempts = 0; attempts < m_aMenuItemList.size(); ++attempts )
        {
            ++m_cursorIdx;
            WrapCursor();
            if( m_aMenuItemList[static_cast<size_t>( m_cursorIdx )]->isEnabled() )
            {
                break;
            }
        }
        setCursorPosition( static_cast<u32>( m_cursorIdx ) );
    }

    void ClawUIMenu::decrementCursor()
    {
        populateMenuItemList();
        if( m_aMenuItemList.empty() )
        {
            return;
        }

        m_prevCursorIdx = m_cursorIdx;
        for( size_t attempts = 0; attempts < m_aMenuItemList.size(); ++attempts )
        {
            --m_cursorIdx;
            WrapCursor();
            if( m_aMenuItemList[static_cast<size_t>( m_cursorIdx )]->isEnabled() )
            {
                break;
            }
        }
        setCursorPosition( static_cast<u32>( m_cursorIdx ) );
    }

    int ClawUIMenu::getNumMenuItems() const
    {
        return static_cast<s32>( getChildren().size() );
    }

    void ClawUIMenu::WrapCursor()
    {
        const auto count = static_cast<s32>( m_aMenuItemList.size() );
        if( count <= 0 )
        {
            m_cursorIdx = 0;
            m_iItemIdx = 0;
            return;
        }

        while( m_cursorIdx < 0 )
        {
            m_cursorIdx += count;
        }
        m_cursorIdx %= count;
    }

    void ClawUIMenu::setCursorPosition( u32 cursorIdx )
    {
        populateMenuItemList();
        if( m_aMenuItemList.empty() )
        {
            return;
        }

        if( m_prevCursorIdx >= 0 && m_prevCursorIdx < static_cast<s32>( m_aMenuItemList.size() ) )
        {
            auto previous = m_aMenuItemList[static_cast<size_t>( m_prevCursorIdx )];
            previous->setFocus( false );
            previous->setHighlighted( false );
            previous->onDeselect();
        }

        m_prevCursorIdx = m_cursorIdx;
        m_cursorIdx = static_cast<s32>( cursorIdx );
        WrapCursor();

        auto current = m_aMenuItemList[static_cast<size_t>( m_cursorIdx )];
        current->setFocus( true );
        current->setHighlighted( true );
        current->onSelect();
    }

    u32 ClawUIMenu::getCursorPosition() const
    {
        return static_cast<u32>( MathI::max( m_cursorIdx, 0 ) );
    }

    void ClawUIMenu::setNumListItems( u32 numListItems )
    {
        m_uiNumListItems = static_cast<s32>( numListItems );
    }

    u32 ClawUIMenu::getNumListItems() const
    {
        return static_cast<u32>( MathI::max( m_uiNumListItems, 0 ) );
    }

    void ClawUIMenu::setCurrentItemIndex( u32 index )
    {
        m_iItemIdx = static_cast<s32>( index );
    }

    u32 ClawUIMenu::getCurrentItemIndex() const
    {
        return static_cast<u32>( MathI::max( m_iItemIdx, 0 ) );
    }

    void ClawUIMenu::setPosition( const Vector2F &position )
    {
        ClawUIElement::setPosition( position );
    }

    void ClawUIMenu::addMenuItem( SmartPtr<IUIElement> item )
    {
        if( item )
        {
            addChild( item );
            m_aMenuItemList.push_back( item );
            m_uiNumMenuItems = static_cast<s32>( m_aMenuItemList.size() );
        }
    }

    void ClawUIMenu::removeMenuItem( SmartPtr<IUIElement> item )
    {
        if( item && removeChild( item ) )
        {
            populateMenuItemList();
        }
    }

    Array<SmartPtr<IUIElement>> ClawUIMenu::getMenuItems() const
    {
        return getChildren();
    }

    void ClawUIMenu::setMenuItems( const Array<SmartPtr<IUIElement>> &menuItems )
    {
        removeAllChildren();
        m_aMenuItemList.clear();
        for( auto &item : menuItems )
        {
            addMenuItem( item );
        }
    }

    String ClawUIMenu::getLabel() const
    {
        return m_label;
    }

    void ClawUIMenu::setLabel( const String &label )
    {
        m_label = label;
    }

    void ClawUIMenu::draw( struct wp_context *ctx )
    {
        if( !ctx || !isVisible() || !isEnabled() )
        {
            return;
        }

        if( !m_label.empty() && beginWorkphoneWidget( ctx ) )
        {
            wp_label( ctx, reinterpret_cast<const wp_c8 *>( m_label.c_str() ), WORKPHONE_TEXT_LEFT );
        }
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
