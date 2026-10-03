#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUILayout.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WPGraphics/ClawUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>

namespace workphone::ui
{
    using namespace workphone::render;

    WP_CLASS_REGISTER_DERIVED( workphone, ClawUILayout, ClawUIElement<UILayout> );

    u32 ClawUILayout::m_nameExt = 0;

    ClawUILayout::ClawUILayout()
    {
        m_type = "Layout";
    }

    ClawUILayout::~ClawUILayout()
    {
    }

    void ClawUILayout::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        setName( "Layout_" + StringUtil::toString( m_nameExt++ ) );
        setLayout( this );
        setLoadingState( LoadingState::Loaded );
    }

    void ClawUILayout::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );
        ClawUIElement<UILayout>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    bool ClawUILayout::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return ClawUIElement::handleEvent( event );
    }

    void ClawUILayout::update()
    {
        if( !isLoaded() || !isVisible() || !isEnabled() )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            return;
        }

        auto ui = workphone::static_pointer_cast<ui::ClawUIManager>( applicationManager->getRenderUI() );
        if( !ui )
        {
            return;
        }

        auto *ctx = ui->getContext();
        if( !ctx )
        {
            return;
        }

        auto position = getPosition();
        auto size = getSize();

        struct wp_rect bounds;
        ClawUtil::calculateBounds( position, size, &bounds );

        // Push the authoritative bounds every frame
        // changes take effect even when MOVABLE/SCALABLE are active (wp_begin only
        // uses its bounds argument as a one-time initialiser in that case).
        wp_window_set_bounds( ctx, reinterpret_cast<const wp_c8 *>( "UIRoot" ), bounds );

        auto flags = static_cast<wp_flags>( static_cast<unsigned int>( getWindowFlags() ) );

        // Remove the window background fill so the layout root is fully transparent.
        ctx->style.window.fixed_background = ClawUtil::solidItem( ColourF( 0.0f, 0.0f, 0.0f, 0.0f ) );
        ctx->style.window.background = wp_rgba( 0, 0, 0, 0 );

        // sort by order
        std::sort( m_children.begin(), m_children.end(),
                   []( const SmartPtr<IUIElement> &a, const SmartPtr<IUIElement> &b ) {
                       return a->getOrder() < b->getOrder();
                   } );

        if( wp_begin( ctx, reinterpret_cast<const wp_c8 *>( "UIRoot" ), bounds, flags ) )
        {
            // Start free layout (no constraints)
            wp_layout_space_begin( ctx, WORKPHONE_STATIC, bounds.h, 1024 );

            for( auto child : m_children )
            {
                if( child && child->isLoaded() )
                {
                    child->update();
                }
            }

            wp_layout_space_end( ctx );
        }
        wp_end( ctx );
    }

    bool ClawUILayout::handleStateChanged( SmartPtr<IState> &state )
    {
        return ClawUIElement<UILayout>::handleStateChanged( state );
    }

    u8 ClawUILayout::GetStateFromName( const String &stateName ) const
    {
        if( stateName == "FadeIn" )
        {
            return FS_FADEIN;
        }
        if( stateName == "FadeOut" )
        {
            return FS_FADEOUT;
        }
        return FS_IDLE;
    }

    String ClawUILayout::GetStateNameFromId( u8 stateId ) const
    {
        switch( static_cast<LayoutStates>( stateId ) )
        {
        case FS_FADEIN:
            return "FadeIn";
        case FS_FADEOUT:
            return "FadeOut";
        default:
            return "Idle";
        }
    }

    void ClawUILayout::addChild( SmartPtr<IUIElement> child )
    {
        if( child )
        {
            child->setLayout( this );
            ClawUIElement<UILayout>::addChild( child );
        }
    }

    bool ClawUILayout::removeChild( SmartPtr<IUIElement> child )
    {
        if( !child )
        {
            return false;
        }

        const auto removed = ClawUIElement<UILayout>::removeChild( child );
        if( removed )
        {
            child->setLayout( nullptr );
        }
        return removed;
    }

    SmartPtr<IUIWindow> ClawUILayout::getParentWindow() const
    {
        return m_uiWindow;
    }

    void ClawUILayout::setParentWindow( SmartPtr<IUIWindow> uiWindow )
    {
        m_uiWindow = uiWindow;
    }

    void ClawUILayout::updateZOrder()
    {
        ClawUIElement<UILayout>::updateZOrder();
    }

    SmartPtr<Properties> ClawUILayout::getProperties() const
    {
        auto properties = ClawUIElement<UILayout>::getProperties();
        properties->setProperty( "Type", "UILayout" );
        properties->setProperty( "Name", getName() );
        return properties;
    }

    Array<SmartPtr<ISharedObject>> ClawUILayout::getChildObjects() const
    {
        auto objects = ClawUIElement<UILayout>::getChildObjects();
        if( m_uiWindow )
        {
            objects.emplace_back( m_uiWindow );
        }
        if( m_fsm )
        {
            objects.emplace_back( m_fsm );
        }
        return objects;
    }

    SmartPtr<IFSM> ClawUILayout::getFSM()
    {
        return m_fsm;
    }

    const SmartPtr<IFSM> &ClawUILayout::getFSM() const
    {
        return m_fsm;
    }

    void ClawUILayout::invalidate()
    {
        if( auto stateContext = getStateContext() )
        {
            stateContext->setDirty( true );
        }
    }

    void ClawUILayout::draw( struct wp_context *ctx )
    {
        if( ctx && isVisible() && isEnabled() )
        {
            sortZOrder();
            drawWorkphoneChildren( ctx );
        }
    }
}  // namespace workphone::ui
