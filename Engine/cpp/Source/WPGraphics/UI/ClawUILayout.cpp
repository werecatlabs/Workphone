#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUILayout.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WPGraphics/ClawUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>

namespace workphone::ui
{
    using namespace workphone::render;

    namespace
    {
        // Scene UI elements share one immediate-mode window. Restrict widgets belonging to
        // scroll content without clipping the viewport's sibling scrollbar or popup controls.
        wp_rect contentClip( SmartPtr<IUIElement> element, wp_rect clip, wp_rect rootBounds,
                             Vector2I referenceSize )
        {
            auto owner = workphone::dynamic_pointer_cast<scene::UIComponent>( element->getOwner() );
            if( !owner )
            {
                return clip;
            }
            auto actor = owner->getActor();
            if( !actor )
            {
                return clip;
            }
            if( referenceSize.x <= 0 || referenceSize.y <= 0 )
            {
                return clip;
            }
            auto scaleX = rootBounds.w / static_cast<f32>( referenceSize.x );
            auto scaleY = rootBounds.h / static_cast<f32>( referenceSize.y );
            for( auto parent = actor->getParent(); parent; parent = parent->getParent() )
            {
                auto view = parent->getComponent<scene::ScrollView>();
                if( !view || !view->getContentPanel() )
                {
                    continue;
                }
                auto contentActor = view->getContentPanel()->getActor();
                auto inContent = false;
                for( auto current = actor; current && current != parent; current = current->getParent() )
                {
                    if( current == contentActor )
                    {
                        inContent = true;
                        break;
                    }
                }
                auto transform = parent->getComponent<scene::LayoutTransform>();
                if( !inContent || !transform )
                {
                    continue;
                }
                auto position = transform->getAbsolutePosition();
                auto size = transform->getAbsoluteSize();
                auto left = MathF::max(
                    clip.x, rootBounds.x + static_cast<f32>( position.x ) * scaleX );
                auto top = MathF::max(
                    clip.y, rootBounds.y + static_cast<f32>( position.y ) * scaleY );
                auto right = MathF::min(
                    clip.x + clip.w,
                    rootBounds.x + static_cast<f32>( position.x + size.x ) * scaleX );
                auto bottom = MathF::min(
                    clip.y + clip.h,
                    rootBounds.y + static_cast<f32>( position.y + size.y ) * scaleY );
                clip = { left, top, MathF::max( 0.0f, right - left ), MathF::max( 0.0f, bottom - top ) };
            }
            return clip;
        }
    }  // namespace

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
        ClawUIElement<UILayout>::update();
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
        if( ctx && ctx->current && ctx->current->layout && isLoaded() && isVisible() && isEnabled() )
        {
            sortZOrder();
            auto owner = workphone::dynamic_pointer_cast<scene::UIComponent>( getOwner() );
            auto layout = workphone::dynamic_pointer_cast<scene::Layout>( owner );
            auto referenceSize = layout ? layout->getReferenceSize() : Vector2I( 1920, 1080 );
            wp_rect viewportBounds;
            ClawUtil::calculateBounds( Vector2F::zero(), Vector2F( 1, 1 ), &viewportBounds );
            auto *panel = ctx->current->layout;
            auto *canvas = wp_window_get_canvas( ctx );
            const auto originalClip = panel->clip;
            struct RestoreClip
            {
                wp_panel *panel;
                wp_command_buffer *canvas;
                wp_rect clip;

                ~RestoreClip()
                {
                    panel->clip = clip;
                    wp_push_scissor( canvas, clip );
                }
            } restore{ panel, canvas, originalClip };

            for( auto child : m_children )
            {
                if( child )
                {
                    auto widget = dynamic_cast<IWorkphoneWidget *>( child.get() );
                    if( !widget )
                    {
                        continue;
                    }
                    auto previousClip = panel->clip;
                    auto clip = contentClip( child, originalClip, viewportBounds, referenceSize );
                    if( clip.w <= 0 || clip.h <= 0 )
                    {
                        continue;
                    }
                    if( clip.x != previousClip.x || clip.y != previousClip.y ||
                        clip.w != previousClip.w || clip.h != previousClip.h )
                    {
                        panel->clip = clip;
                        wp_push_scissor( canvas, clip );
                    }
                    widget->draw( ctx );
                }
            }
        }
    }
}  // namespace workphone::ui
