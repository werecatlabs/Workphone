#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>

namespace workphone
{
    namespace ui
    {

        void UIUtilCore::calculatePositionAndSize( const Vector2F &position, const Vector2F &size,
                                                   Vector2F &outAbsolutePosition,
                                                   Vector2F &outAbsoluteSize )
        {
            if( size.x <= 0.0f || size.y <= 0.0f )
            {
                outAbsolutePosition = Vector2F( 0.0f, 0.0f );
                outAbsoluteSize = Vector2F( 1.0f, 1.0f );
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

            auto iPosition = Vector2F::zero();
            auto iSize = Vector2F::zero();

            if( auto uiWindow = applicationManager->getSceneRenderWindow() )
            {
                auto sceneWindowPosition = uiWindow->getPosition();
                auto sceneWindowSize = uiWindow->getSize();

                auto pos = position * sceneWindowSize;
                auto sz = size * sceneWindowSize;

                iPosition = Vector2F( (f32)pos.X(), (f32)pos.Y() );
                iSize = Vector2F( (f32)sz.X(), (f32)sz.Y() );
            }
            else
            {
                if( auto mainWindow = applicationManager->getWindow() )
                {
                    auto mainWindowSize = mainWindow->getSize();
                    auto mainWindowSizeF = Vector2F( (f32)mainWindowSize.x, (f32)mainWindowSize.y );

                    auto pos = position * mainWindowSizeF;
                    auto sz = size * mainWindowSizeF;

                    iPosition = Vector2F( (f32)pos.X(), (f32)pos.Y() );
                    iSize = Vector2F( (f32)sz.X(), (f32)sz.Y() );
                }
            }

            if( iSize.x <= 0 || iSize.y <= 0 )
            {
                outAbsolutePosition = Vector2F( 0.0f, 0.0f );
                outAbsoluteSize = Vector2F( 1.0f, 1.0f );
                return;
            }

            outAbsolutePosition = Vector2F( iPosition.x, iPosition.y );
            outAbsoluteSize = Vector2F( iSize.x, iSize.y );
        }

        void UIUtilCore::calculateBounds( const Vector2F &position, const Vector2F &size,
                                          wp_rect *outBounds )
        {
            if( size.x <= 0.0f || size.y <= 0.0f )
            {
                *outBounds = wp_make_rect( 0.0f, 0.0f, 1.0f, 1.0f );
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

            auto iPosition = Vector2F::zero();
            auto iSize = Vector2F::zero();

            if( auto uiWindow = applicationManager->getSceneRenderWindow() )
            {
                auto sceneWindowPosition = uiWindow->getPosition();
                auto sceneWindowSize = uiWindow->getSize();

                auto pos = position * sceneWindowSize;
                auto sz = size * sceneWindowSize;

                iPosition = Vector2F( (f32)pos.X(), (f32)pos.Y() );
                iSize = Vector2F( (f32)sz.X(), (f32)sz.Y() );
            }
            else
            {
                if( auto mainWindow = applicationManager->getWindow() )
                {
                    auto mainWindowSize = mainWindow->getSize();
                    auto mainWindowSizeF = Vector2F( (f32)mainWindowSize.x, (f32)mainWindowSize.y );

                    auto pos = position * mainWindowSizeF;
                    auto sz = size * mainWindowSizeF;

                    iPosition = Vector2F( (f32)pos.X(), (f32)pos.Y() );
                    iSize = Vector2F( (f32)sz.X(), (f32)sz.Y() );
                }
            }

            if( iSize.x <= 0 || iSize.y <= 0 )
            {
                *outBounds = wp_make_rect( 0.0f, 0.0f, 1.0f, 1.0f );
                return;
            }

            *outBounds = wp_make_rect( iPosition.x, iPosition.y, iSize.x, iSize.y );
        }

        wp_color UIUtilCore::toWpColor( const ColourF &c )
        {
            return wp_rgba( static_cast<wp_byte>( c.r * 255.0f ), static_cast<wp_byte>( c.g * 255.0f ),
                            static_cast<wp_byte>( c.b * 255.0f ), static_cast<wp_byte>( c.a * 255.0f ) );
        }

        wp_style_item UIUtilCore::solidItem( const ColourF &c )
        {
            struct wp_style_item item;
            item.type = WORKPHONE_STYLE_ITEM_COLOR;
            item.data.color = toWpColor( c );
            return item;
        }

    }  // namespace ui
}  // namespace workphone
