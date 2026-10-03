#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIImage.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ClawUIImage, ClawUIElement<IUIImage> );

    ClawUIImage::ClawUIImage()
    {
        m_type = "Image";
    }

    ClawUIImage::~ClawUIImage()
    {
        unload( nullptr );
    }

    void ClawUIImage::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        if( auto stateContext = getStateContext() )
        {
            stateContext->setDirty( true );
        }
        setLoadingState( LoadingState::Loaded );
    }

    void ClawUIImage::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );
        ClawUIElement<IUIImage>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void ClawUIImage::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
    }

    String ClawUIImage::getMaterialName() const
    {
        return m_materialName;
    }

    void ClawUIImage::setTexture( SmartPtr<render::ITexture> texture )
    {
        m_texture = texture;
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateData<UIElementStateData>() )
            {
                state->texture = texture;
            }
        }
    }

    SmartPtr<render::ITexture> ClawUIImage::getTexture() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateData<UIElementStateData>() )
            {
                return state->texture;
            }
        }
        return m_texture;
    }

    void ClawUIImage::setMaterial( SmartPtr<render::IMaterial> material )
    {
        m_material = material;
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateData<UIElementStateData>() )
            {
                state->material = material;
            }
        }
    }

    SmartPtr<render::IMaterial> ClawUIImage::getMaterial() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateData<UIElementStateData>() )
            {
                return state->material;
            }
        }
        return m_material;
    }

    void ClawUIImage::setPosition( const Vector2F &position )
    {
        ClawUIElement<IUIImage>::setPosition( position );
    }

    void ClawUIImage::setSize( const Vector2F &size )
    {
        ClawUIElement<IUIImage>::setSize( size );
    }

    void ClawUIImage::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = nullptr;
        }
    }

    SmartPtr<render::IOverlayElementContainer> ClawUIImage::getContainerObject() const
    {
        return nullptr;
    }

    void ClawUIImage::setContainerObject( SmartPtr<render::IOverlayElementContainer> container )
    {
        // Retained for source compatibility. WorkphoneCore images are emitted per frame.
    }

    bool ClawUIImage::handleStateChanged( SmartPtr<IState> &state )
    {
        return ClawUIElement<IUIImage>::handleStateChanged( state );
    }

    SmartPtr<Properties> ClawUIImage::getProperties() const
    {
        auto properties = ClawUIElement<IUIImage>::getProperties();
        properties->setProperty( borderLeftStr, m_borderLeft );
        properties->setProperty( borderRightStr, m_borderRight );
        properties->setProperty( borderTopStr, m_borderTop );
        properties->setProperty( borderBottomStr, m_borderBottom );
        properties->setProperty( useTilingStr, m_useTiling );
        properties->setProperty( spriteSizeStr, m_spriteSize );
        properties->setProperty( useNineSliceStr, m_useNineSlice );
        properties->setProperty( tileScaleXStr, m_tileScaleX );
        properties->setProperty( tileScaleYStr, m_tileScaleY );
        return properties;
    }

    void ClawUIImage::setProperties( SmartPtr<Properties> properties )
    {
        ClawUIElement<IUIImage>::setProperties( properties );
        if( !properties )
        {
            return;
        }

        properties->getPropertyValue( borderLeftStr, m_borderLeft );
        properties->getPropertyValue( borderRightStr, m_borderRight );
        properties->getPropertyValue( borderTopStr, m_borderTop );
        properties->getPropertyValue( borderBottomStr, m_borderBottom );
        properties->getPropertyValue( useTilingStr, m_useTiling );
        properties->getPropertyValue( spriteSizeStr, m_spriteSize );
        properties->getPropertyValue( useNineSliceStr, m_useNineSlice );
        properties->getPropertyValue( tileScaleXStr, m_tileScaleX );
        properties->getPropertyValue( tileScaleYStr, m_tileScaleY );
    }

    Array<SmartPtr<ISharedObject>> ClawUIImage::getChildObjects() const
    {
        return ClawUIElement<IUIImage>::getChildObjects();
    }

    bool ClawUIImage::isValid() const
    {
        return getLoadingState() == LoadingState::Loaded;
    }

    void ClawUIImage::setUseTiling( bool useTiling )
    {
        m_useTiling = useTiling;
    }

    bool ClawUIImage::getUseTiling() const
    {
        return m_useTiling;
    }

    bool ClawUIImage::getUseNineSlice() const
    {
        return m_useNineSlice;
    }

    void ClawUIImage::setUseNineSlice( bool useNineSlice )
    {
        m_useNineSlice = useNineSlice;
    }

    f32 ClawUIImage::getTileScaleX() const
    {
        return m_tileScaleX;
    }

    void ClawUIImage::setTileScaleX( f32 scale )
    {
        m_tileScaleX = MathF::max( scale, 0.01f );
    }

    f32 ClawUIImage::getTileScaleY() const
    {
        return m_tileScaleY;
    }

    void ClawUIImage::setTileScaleY( f32 scale )
    {
        m_tileScaleY = MathF::max( scale, 0.01f );
    }

    Vector2I ClawUIImage::getSpriteSize() const
    {
        return m_spriteSize;
    }

    void ClawUIImage::setSpriteSize( const Vector2I &spriteSize )
    {
        m_spriteSize = spriteSize;
    }

    f32 ClawUIImage::getBorderLeft() const
    {
        return m_borderLeft;
    }

    void ClawUIImage::setBorderLeft( f32 borderLeft )
    {
        m_borderLeft = MathF::max( borderLeft, 0.0f );
    }

    f32 ClawUIImage::getBorderRight() const
    {
        return m_borderRight;
    }

    void ClawUIImage::setBorderRight( f32 borderRight )
    {
        m_borderRight = MathF::max( borderRight, 0.0f );
    }

    f32 ClawUIImage::getBorderTop() const
    {
        return m_borderTop;
    }

    void ClawUIImage::setBorderTop( f32 borderTop )
    {
        m_borderTop = MathF::max( borderTop, 0.0f );
    }

    f32 ClawUIImage::getBorderBottom() const
    {
        return m_borderBottom;
    }

    void ClawUIImage::setBorderBottom( f32 borderBottom )
    {
        m_borderBottom = MathF::max( borderBottom, 0.0f );
    }

    void ClawUIImage::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        auto canvas = wp_window_get_canvas( ctx );
        if( !canvas )
        {
            return;
        }

        const auto bounds = getWorkphoneBounds();
        const auto tint = ClawUIWorkphoneContext::toWorkphoneColor( getColour() );
        auto texture = getTexture();
        auto image = texture ? wp_image_ptr( texture.get() ) : wp_image_id( 0 );

        if( texture && m_useNineSlice &&
            ( m_borderLeft > 0.0f || m_borderRight > 0.0f || m_borderTop > 0.0f ||
              m_borderBottom > 0.0f ) )
        {
            struct wp_nine_slice slice;
            slice.img = image;
            slice.l = static_cast<wp_u16>( m_borderLeft );
            slice.r = static_cast<wp_u16>( m_borderRight );
            slice.t = static_cast<wp_u16>( m_borderTop );
            slice.b = static_cast<wp_u16>( m_borderBottom );
            wp_draw_nine_slice( canvas, bounds, &slice, tint );
        }
        else if( texture && m_useTiling )
        {
            const auto tileWidth = bounds.w / MathF::max( m_tileScaleX, 0.01f );
            const auto tileHeight = bounds.h / MathF::max( m_tileScaleY, 0.01f );
            wp_push_scissor( canvas, bounds );

            for( auto y = bounds.y; y < bounds.y + bounds.h; y += tileHeight )
            {
                for( auto x = bounds.x; x < bounds.x + bounds.w; x += tileWidth )
                {
                    struct wp_rect tile = { x, y, tileWidth, tileHeight };
                    wp_draw_image( canvas, tile, &image, tint );
                }
            }

            wp_push_scissor( canvas, wp_window_get_bounds( ctx ) );
        }
        else if( texture )
        {
            wp_draw_image( canvas, bounds, &image, tint );
        }
        else
        {
            wp_fill_rect( canvas, bounds, 0.0f, tint );
        }

        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
