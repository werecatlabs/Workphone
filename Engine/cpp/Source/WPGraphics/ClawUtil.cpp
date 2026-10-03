#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawUtil.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>

namespace workphone::render
{

    void ClawUtil::calculatePositionAndSize( const Vector2F &position, const Vector2F &size,
                                             Vector2F &outAbsolutePosition, Vector2F &outAbsoluteSize )
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

    void ClawUtil::calculateBounds( const Vector2F &position, const Vector2F &size, wp_rect *outBounds )
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

    wp_color ClawUtil::toWpColor( const ColourF &c )
    {
        return wp_rgba( static_cast<wp_byte>( c.r * 255.0f ), static_cast<wp_byte>( c.g * 255.0f ),
                        static_cast<wp_byte>( c.b * 255.0f ), static_cast<wp_byte>( c.a * 255.0f ) );
    }

    wp_style_item ClawUtil::solidItem( const ColourF &c )
    {
        struct wp_style_item item;
        item.type = WORKPHONE_STYLE_ITEM_COLOR;
        item.data.color = toWpColor( c );
        return item;
    }

    u32 ClawUtil::fromCCullMode( wp_cull_mode mode )
    {
        if( mode == WORKPHONE_CULL_MODE_NONE )
            return 1u;
        if( mode == WORKPHONE_CULL_MODE_FRONT )
            return 3u;
        return 2u;
    }

    wp_cull_mode ClawUtil::toCCullMode( u32 mode )
    {
        if( mode == 1u )
            return WORKPHONE_CULL_MODE_NONE;
        if( mode == 3u )
            return WORKPHONE_CULL_MODE_FRONT;
        return WORKPHONE_CULL_MODE_BACK;
    }

    u32 ClawUtil::fromCBlendMode( wp_blend_mode mode )
    {
        switch( mode )
        {
        case WORKPHONE_BLEND_MODE_NONE:
            return 0u;
        case WORKPHONE_BLEND_MODE_ADDITIVE:
              return 3u;
        case WORKPHONE_BLEND_MODE_MULTIPLY:
              return 4u;
          case WORKPHONE_BLEND_MODE_PREMULTIPLIED:
              return 2u;
        default:
            return 1u;
        }
    }

    wp_blend_mode ClawUtil::toCBlendMode( u32 mode )
    {
        switch( mode )
        {
        case 0:
            return WORKPHONE_BLEND_MODE_NONE;
        case 2:
              return WORKPHONE_BLEND_MODE_PREMULTIPLIED;
          case 3:
              return WORKPHONE_BLEND_MODE_ADDITIVE;
        case 4:
              return WORKPHONE_BLEND_MODE_MULTIPLY;
        default:
            return WORKPHONE_BLEND_MODE_ALPHA;
        }
    }

    ColourF ClawUtil::fromCColour( const wp_colour_f &colour )
    {
        return ColourF( colour.r, colour.g, colour.b, colour.a );
    }

    wp_colour_f ClawUtil::toCColour( const ColourF &colour )
    {
        return { colour.r, colour.g, colour.b, colour.a };
    }

    Vector3<real_Num> ClawUtil::fromCVector( wp_vec3f vector )
    {
        return Vector3<real_Num>( vector.x, vector.y, vector.z );
    }

    wp_vec3f ClawUtil::toCVector( const Vector3<real_Num> &vector )
    {
        return wp_vec3f_make( vector.x, vector.y, vector.z );
    }

    wp_render_debug_view ClawUtil::toNativeDebugView( IGraphicsPipeline::DebugView view )
    {
        switch( view )
        {
        case IGraphicsPipeline::DebugView::Normals:
            return WP_PIPELINE_DEBUG_NORMALS;
        case IGraphicsPipeline::DebugView::Depth:
            return WP_PIPELINE_DEBUG_DEPTH;
        case IGraphicsPipeline::DebugView::Velocity:
            return WP_PIPELINE_DEBUG_VELOCITY;
        case IGraphicsPipeline::DebugView::AO:
            return WP_PIPELINE_DEBUG_AO;
        case IGraphicsPipeline::DebugView::SSR:
            return WP_PIPELINE_DEBUG_SSR;
        case IGraphicsPipeline::DebugView::Bloom:
            return WP_PIPELINE_DEBUG_BLOOM;
        case IGraphicsPipeline::DebugView::Exposure:
            return WP_PIPELINE_DEBUG_EXPOSURE;
        default:
            return WP_PIPELINE_DEBUG_NONE;
        }
    }

    wp_mat4f ClawUtil::toNativeMatrix( const Matrix4F &matrix )
    {
        wp_mat4f result{};
        for( u32 row = 0; row < 4; ++row )
        {
            for( u32 column = 0; column < 4; ++column )
            {
                result.m[row][column] = static_cast<wp_f32>( matrix[row][column] );
            }
        }
        return result;
    }

}  // namespace workphone::render
