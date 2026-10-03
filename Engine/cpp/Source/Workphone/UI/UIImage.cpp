#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UIImage.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Scene/Directors/TextureResourceDirector.hpp>
#include <Workphone/State/States/UIImageStateData.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIImage, UIElement<IUIImage> );

        UIImage::UIImage() : UIElement<IUIImage>( UIImage::typeInfo() )
        {
        }

        UIImage::UIImage( u32 poolTypeId ) : UIElement<IUIImage>( poolTypeId )
        {
        }

        UIImage::~UIImage() = default;

        void UIImage::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setTexture( nullptr );
                setMaterial( nullptr );

                UIElement::unload( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIImage::setTexture( SmartPtr<render::ITexture> texture )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->texture = texture;

                    auto applicationManager = core::IApplicationManager::instance();
                    auto resourceDatabase = applicationManager->getResourceDatabase();

                    auto director = resourceDatabase->loadDirector( texture );
                    if( director->isDerived<scene::TextureResourceDirector>() )
                    {
                        auto textureDirector =
                            workphone::static_pointer_cast<scene::TextureResourceDirector>( director );
                        if( textureDirector )
                        {
                            state->useTiling = textureDirector->getUseTiling();
                            state->borderLeft = (f32)textureDirector->getBorderLeft();
                            state->borderRight = (f32)textureDirector->getBorderRight();
                            state->borderTop = (f32)textureDirector->getBorderTop();
                            state->borderBottom = (f32)textureDirector->getBorderBottom();
                        }

                        state->spriteSize = texture->getSize();
                    }
                }
            }
        }

        SmartPtr<render::ITexture> UIImage::getTexture() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->texture;
                }
            }

            return nullptr;
        }

        void UIImage::setMaterial( SmartPtr<render::IMaterial> material )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->material = material;
                }
            }
        }

        SmartPtr<render::IMaterial> UIImage::getMaterial() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->material;
                }
            }

            return nullptr;
        }

        Vector2I UIImage::getSpriteSize() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->spriteSize;
                }
            }

            return {};
        }

        void UIImage::setSpriteSize( const Vector2I &spriteSize )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->spriteSize = spriteSize;
                }
            }
        }

        f32 UIImage::getBorderLeft() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->borderLeft;
                }
            }

            return 0.0f;
        }

        void UIImage::setBorderLeft( f32 borderLeft )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->borderLeft = borderLeft;
                }
            }
        }

        f32 UIImage::getBorderRight() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->borderRight;
                }
            }

            return 0.0f;
        }

        void UIImage::setBorderRight( f32 borderRight )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->borderRight = borderRight;
                }
            }
        }

        f32 UIImage::getBorderTop() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->borderTop;
                }
            }

            return 0.0f;
        }

        void UIImage::setBorderTop( f32 borderTop )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->borderTop = borderTop;
                }
            }
        }

        f32 UIImage::getBorderBottom() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->borderBottom;
                }
            }

            return 0.0f;
        }

        void UIImage::setBorderBottom( f32 borderBottom )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->borderBottom = borderBottom;
                }
            }
        }

        bool UIImage::getUseTiling() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->useTiling;
                }
            }

            return false;
        }

        void UIImage::setUseTiling( bool useTiling )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->useTiling = useTiling;
                }
            }
        }

        bool UIImage::getUseNineSlice() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->useNineSlice;
                }
            }

            return false;
        }

        void UIImage::setUseNineSlice( bool useNineSlice )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->useNineSlice = useNineSlice;
                }
            }
        }

        f32 UIImage::getTileScaleX() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->tileScaleX;
                }
            }

            return 1.0f;
        }

        void UIImage::setTileScaleX( f32 scale )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->tileScaleX = scale;
                }
            }
        }

        f32 UIImage::getTileScaleY() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIImageStateData>() )
                {
                    return state->tileScaleY;
                }
            }

            return 1.0f;
        }

        void UIImage::setTileScaleY( f32 scale )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIImageStateData>() )
                {
                    state->tileScaleY = scale;
                }
            }
        }

        SmartPtr<Properties> UIImage::getProperties() const
        {
            auto properties = UIElement::getProperties();
            if( !properties )
            {
                properties = workphone::make_ptr<Properties>();
            }

            // Add image-specific properties
            auto useTiling = getUseTiling();
            properties->setProperty( useTilingStr, useTiling );

            properties->setProperty( IUIImage::colourStr, getColour() );

            if( auto texture = getTexture() )
            {
                properties->setProperty( textureStr, texture->getName() );
            }

            if( auto material = getMaterial() )
            {
                properties->setProperty( materialStr, material->getName() );
            }

            return properties;
        }

        void UIImage::setProperties( SmartPtr<Properties> properties )
        {
            UIElement::setProperties( properties );

            if( properties )
            {
                // Set use tiling property
                bool useTiling = false;
                if( properties->getPropertyValue( useTilingStr, useTiling ) )
                {
                    setUseTiling( useTiling );
                }

                // Tint colour
                ColourF colour = getColour();
                if( properties->getPropertyValue( IUIImage::colourStr, colour ) )
                {
                    setColour( colour );
                }

                // Note: Texture and material are typically set via their respective setters
                // rather than through properties, but we could add support for loading by name here
                String textureName;
                if( properties->getPropertyValue( textureStr, textureName ) )
                {
                    // TODO: Load texture by name if needed
                    // This would require access to texture manager
                }

                String materialName;
                if( properties->getPropertyValue( materialStr, materialName ) )
                {
                    // TODO: Load material by name if needed
                    // This would require access to material manager
                }
            }
        }

        Array<SmartPtr<ISharedObject>> UIImage::getChildObjects() const
        {
            auto childObjects = UIElement::getChildObjects();

            if( auto texture = getTexture() )
            {
                childObjects.push_back( texture );
            }

            if( auto material = getMaterial() )
            {
                childObjects.push_back( material );
            }

            return childObjects;
        }

        void UIImage::onChangedState()
        {
            UIElement::onChangedState();

            // Additional state change handling for image-specific properties
            // This could include updating the visual representation, etc.
        }
    }  // namespace ui
}  // namespace workphone
