#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIImageCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <WPGraphicsOgreNext/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>
#include <OgreHlmsManager.h>
#include <OgreHlms.h>
#include <OgreRoot.h>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIImageCore, UIElementCore<UIImage> );

        UIImageCore::UIImageCore()
        {
            createStateContext();
        }

        UIImageCore::~UIImageCore()
        {
            destroyStateContext();
        }

        void UIImageCore::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );
                UIElementCore<UIImage>::load( data );
                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIImageCore::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );

                UIElementCore<UIImage>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIImageCore::update()
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

            auto iPosition = Vector2I::zero();
            auto iSize = Vector2I::zero();

            struct wp_rect bounds;
            UIUtilCore::calculateBounds( position, size, &bounds );

            auto *canvas = wp_window_get_canvas( ctx );
            if( !canvas )
            {
                return;
            }

            // Tint from element colour.
            auto colour = getColour();
            struct wp_color tint = wp_rgba(
                static_cast<wp_byte>( colour.r * 255.0f ), static_cast<wp_byte>( colour.g * 255.0f ),
                static_cast<wp_byte>( colour.b * 255.0f ), static_cast<wp_byte>( colour.a * 255.0f ) );

            // Build the wp_image from the assigned texture.
            auto tex = getTexture();
            auto ogreTexture = workphone::static_pointer_cast<render::CTextureOgreNext>( tex );

            wp_layout_space_push( ctx, bounds );

            struct wp_image img;
            if( ogreTexture )
            {
                if( !ogreTexture->isLoaded() )
                {
                    ogreTexture->load( nullptr );
                }

                auto *gpuTexture = ogreTexture->getTexture();
                img = gpuTexture ? wp_image_ptr( gpuTexture ) : wp_image_id( 0 );
            }
            else
            {
                img = wp_image_id( 0 );
            }

            // Fetch border values (pixels in reference-canvas space).
            const f32 bLeft = getBorderLeft();
            const f32 bRight = getBorderRight();
            const f32 bTop = getBorderTop();
            const f32 bBottom = getBorderBottom();
            const bool hasBorders = ( bLeft > 0.0f || bRight > 0.0f || bTop > 0.0f || bBottom > 0.0f );

            // -----------------------------------------------------------
            // Nine-slice path
            // -----------------------------------------------------------
            auto useNineSlice = getUseNineSlice();
            if( useNineSlice && hasBorders )
            {
                struct wp_nine_slice slice;
                slice.img = img;
                slice.l = static_cast<wp_u16>( bLeft );
                slice.t = static_cast<wp_u16>( bTop );
                slice.r = static_cast<wp_u16>( bRight );
                slice.b = static_cast<wp_u16>( bBottom );

                wp_draw_nine_slice( canvas, bounds, &slice, tint );
                return;
            }

            // -----------------------------------------------------------
            // Tiling path
            // -----------------------------------------------------------
            if( getUseTiling() )
            {
                auto tileScaleX = getTileScaleX();
                auto tileScaleY = getTileScaleY();

                const f32 tileW = ( tileScaleX > 0.0f ) ? ( bounds.w / tileScaleX ) : bounds.w;
                const f32 tileH = ( tileScaleY > 0.0f ) ? ( bounds.h / tileScaleY ) : bounds.h;

                if( tileW > 0.0f && tileH > 0.0f )
                {
                    // Clip subsequent draw calls to the element bounds.
                    wp_push_scissor( canvas, bounds );

                    for( f32 ty = bounds.y; ty < bounds.y + bounds.h; ty += tileH )
                    {
                        for( f32 tx = bounds.x; tx < bounds.x + bounds.w; tx += tileW )
                        {
                            struct wp_rect tile;
                            tile.x = tx;
                            tile.y = ty;
                            tile.w = tileW;
                            tile.h = tileH;
                            wp_draw_image( canvas, tile, &img, tint );
                        }
                    }

                    // Restore full-canvas scissor rect (use configurable reference dimensions).
                    struct wp_rect noClip;
                    noClip.x = 0.0f;
                    noClip.y = 0.0f;
                    noClip.w = m_referenceWidth;
                    noClip.h = m_referenceHeight;
                    wp_push_scissor( canvas, noClip );
                }
                return;
            }

            // -----------------------------------------------------------
            // Default: stretch the image across the element bounds, or
            // draw a filled placeholder rectangle if there is no texture.
            // -----------------------------------------------------------
            if( ogreTexture )
            {
                wp_draw_image( canvas, bounds, &img, tint );
            }
            else
            {
                // No texture assigned — fill with the element's tint colour as a
                // solid placeholder so the element is still visible in the editor.
                wp_fill_rect( canvas, bounds, 0.0f, tint );
            }

            UIElementCore<UIImage>::update();
        }

        bool UIImageCore::handleStateChanged( SmartPtr<IState> &state )
        {
            if( isLoaded() )
            {
                ScopedLock lock( this );

                auto stateData = state->getData();
                if( stateData->isDerived<UIImageStateData>() )
                {
                    auto imageStateData = workphone::static_pointer_cast<UIImageStateData>( stateData );

                    auto applicationManager = core::IApplicationManager::instance();
                    auto resourceDatabase = applicationManager->getResourceDatabase();

                    if( auto texture = getTexture() )
                    {
                        auto director = resourceDatabase->loadDirector( texture );
                        auto textureDirector =
                            dynamic_pointer_cast<scene::TextureResourceDirector>( director );
                        if( textureDirector )
                        {
                            setUseTiling( textureDirector->getUseTiling() );
                            setBorderLeft( (f32)textureDirector->getBorderLeft() );
                            setBorderRight( (f32)textureDirector->getBorderRight() );
                            setBorderTop( (f32)textureDirector->getBorderTop() );
                            setBorderBottom( (f32)textureDirector->getBorderBottom() );
                        }
                    }

                    if( auto texture = getTexture() )
                    {
                        auto size = texture->getSize();
                        setSpriteSize( size );
                    }

                    return true;
                }

                return UIElementCore<UIImage>::handleStateChanged( state );
            }

            return false;
        }

        SmartPtr<Properties> UIImageCore::getProperties() const
        {
            auto texture = getTexture();

            auto spriteSize = getSpriteSize();
            auto useTiling = getUseTiling();
            auto borderLeft = getBorderLeft();
            auto borderRight = getBorderRight();
            auto borderTop = getBorderTop();
            auto borderBottom = getBorderBottom();
            auto useNineSlice = getUseNineSlice();
            auto tileScaleX = getTileScaleX();
            auto tileScaleY = getTileScaleY();

            auto properties = UIElementCore<UIImage>::getProperties();
            properties->setProperty( borderLeftStr, borderLeft );
            properties->setProperty( borderRightStr, borderRight );
            properties->setProperty( borderTopStr, borderTop );
            properties->setProperty( borderBottomStr, borderBottom );
            properties->setProperty( useTilingStr, useTiling );
            properties->setProperty( spriteSizeStr, spriteSize );
            properties->setProperty( textureStr, texture );
            properties->setProperty( IUIImage::useNineSliceStr, useNineSlice );
            properties->setProperty( IUIImage::tileScaleXStr, tileScaleX );
            properties->setProperty( IUIImage::tileScaleYStr, tileScaleY );
            properties->setProperty( IUIImage::referenceWidthStr, m_referenceWidth );
            properties->setProperty( IUIImage::referenceHeightStr, m_referenceHeight );

            return properties;
        }

        void UIImageCore::setProperties( SmartPtr<Properties> properties )
        {
            auto spriteSize = getSpriteSize();
            auto useTiling = getUseTiling();
            auto borderLeft = getBorderLeft();
            auto borderRight = getBorderRight();
            auto borderTop = getBorderTop();
            auto borderBottom = getBorderBottom();
            auto useNineSlice = getUseNineSlice();
            auto tileScaleX = getTileScaleX();
            auto tileScaleY = getTileScaleY();

            UIElementCore<UIImage>::setProperties( properties );

            auto texture = SmartPtr<render::ITexture>();

            properties->getPropertyValue( borderLeftStr, borderLeft );
            properties->getPropertyValue( borderRightStr, borderRight );
            properties->getPropertyValue( borderTopStr, borderTop );
            properties->getPropertyValue( borderBottomStr, borderBottom );
            properties->getPropertyValue( useTilingStr, useTiling );
            properties->getPropertyValue( spriteSizeStr, spriteSize );
            properties->getPropertyValue( textureStr, texture );
            properties->getPropertyValue( IUIImage::useNineSliceStr, useNineSlice );
            properties->getPropertyValue( IUIImage::tileScaleXStr, tileScaleX );
            properties->getPropertyValue( IUIImage::tileScaleYStr, tileScaleY );
            properties->getPropertyValue( IUIImage::referenceWidthStr, m_referenceWidth );
            properties->getPropertyValue( IUIImage::referenceHeightStr, m_referenceHeight );

            setBorderLeft( borderLeft );
            setBorderRight( borderRight );
            setBorderTop( borderTop );
            setBorderBottom( borderBottom );
            setUseTiling( useTiling );
            setUseNineSlice( useNineSlice );
            setTileScaleX( tileScaleX );
            setTileScaleY( tileScaleY );
        }

        void UIImageCore::createStateContext()
        {
            WP_ASSERT( getStateContext() == nullptr );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateTask = graphicsSystem->getStateTask();

            auto stateContext = stateManager->addStateContext();
            stateContext->setOwner( this );
            setStateContext( stateContext );
            stateContext->setTaskId( stateTask );

            auto listener = factoryManager->make_ptr<ElementStateListener>();
            listener->setOwner( this );
            stateContext->addStateListener( listener );
            setStateListener( listener );

            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<UIElementStateData>();
            state->setData( stateData );

            auto transfromState = factoryManager->make_ptr<State>();
            transfromState->setId( getId() );
            transfromState->setOwner( this );
            stateContext->addState( transfromState );

            auto transformStateData = factoryManager->make_ptr<UITransformStateData>();
            transfromState->setData( transformStateData );

            auto imageState = factoryManager->make_ptr<State>();
            imageState->setId( getId() );
            imageState->setOwner( this );
            stateContext->addState( imageState );

            auto imageStateData = factoryManager->make_ptr<UIImageStateData>();
            imageState->setData( imageStateData );
        }

        f32 UIImageCore::getReferenceWidth() const
        {
            return m_referenceWidth;
        }

        void UIImageCore::setReferenceWidth( f32 width )
        {
            m_referenceWidth = width;
        }

        f32 UIImageCore::getReferenceHeight() const
        {
            return m_referenceHeight;
        }

        void UIImageCore::setReferenceHeight( f32 height )
        {
            m_referenceHeight = height;
        }

    }  // namespace ui
}  // namespace workphone
