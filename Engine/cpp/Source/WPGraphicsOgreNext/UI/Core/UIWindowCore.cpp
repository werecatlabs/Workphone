#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIWindowCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>
#include <workphone_window.h>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIWindowCore, UIElementCore<UIWindow> );

        UIWindowCore::UIWindowCore()
        {
            createStateContext();
        }

        UIWindowCore::~UIWindowCore()
        {
            unload( nullptr );
        }

        void UIWindowCore::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );
                m_children.reserve( 128 );
                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIWindowCore::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );
                UIElementCore<UIWindow>::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIWindowCore::update()
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

            const auto position = getPosition();
            const auto size = getSize();

            struct wp_rect bounds;
            bounds.x = static_cast<wp_f32>( position.x );
            bounds.y = static_cast<wp_f32>( position.y );
            bounds.w = static_cast<wp_f32>( size.x );
            bounds.h = static_cast<wp_f32>( size.y );

            auto flags = WORKPHONE_WINDOW_MOVABLE | WORKPHONE_WINDOW_SCALABLE |
                         WORKPHONE_WINDOW_MINIMIZABLE | WORKPHONE_WINDOW_TITLE;
            if( hasBorder() )
            {
                flags |= WORKPHONE_WINDOW_BORDER;
            }

            const auto label = getLabel();
            const auto *windowLabel = label.empty() ? reinterpret_cast<const wp_c8 *>( "Window" ) :
                                                      reinterpret_cast<const wp_c8 *>( label.c_str() );

            if( wp_begin( ctx, windowLabel, bounds, flags ) )
            {
                for( auto child : m_children )
                {
                    if( child && child->isLoaded() )
                    {
                        child->update();
                    }
                }
            }

            wp_end( ctx );
        }

    }  // namespace ui
}  // namespace workphone
