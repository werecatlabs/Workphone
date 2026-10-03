#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UILayoutCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UILayoutCore, UIElementCore<UILayout> );

        UILayoutCore::UILayoutCore()
        {
            createStateContext();
            WP_LOG( "UILayoutCore constructor" );
        }

        UILayoutCore::~UILayoutCore()
        {
            unload( nullptr );
        }

        void UILayoutCore::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );
                m_children.reserve( 128 );
                setSize( Vector2F( 1280.0f, 720.0f ) );
                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UILayoutCore::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );
                UIElementCore<UILayout>::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UILayoutCore::update()
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

            auto ui = workphone::static_pointer_cast<UIManagerCore>( applicationManager->getRenderUI() );
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
            UIUtilCore::calculateBounds( position, size, &bounds );

            // Push the authoritative bounds every frame
            // changes take effect even when MOVABLE/SCALABLE are active (wp_begin only
            // uses its bounds argument as a one-time initialiser in that case).
            wp_window_set_bounds( ctx, reinterpret_cast<const wp_c8 *>( "UIRoot" ), bounds );

            auto flags = static_cast<wp_flags>( static_cast<unsigned int>( getWindowFlags() ) );

            // Remove the window background fill so the layout root is fully transparent.
            ctx->style.window.fixed_background = UIUtilCore::solidItem( ColourF( 0.0f, 0.0f, 0.0f, 0.0f ) );
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

    }  // namespace ui
}  // namespace workphone
