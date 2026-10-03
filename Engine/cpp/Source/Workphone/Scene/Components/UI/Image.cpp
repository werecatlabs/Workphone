#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/Image.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/UI/IUILayoutWindow.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Image, UIComponent );
    const String Image::TextureStr = String( "Texture" );
    const String Image::UseTilingStr = String( "Tiling" );
    const String Image::BaseMaterialNameStr = String( "baseMaterialName" );

    Image::Image() = default;

    Image::~Image() = default;

    void Image::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            createUI();
            UIComponent::load( data );

            if( data )
            {
                if( data->isExactly<Properties>() )
                {
                    setProperties( data );
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Image::createUI()
    {
        if( auto actor = getActor() )
        {
            auto element = getElement();
            if( !element )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto renderUI = applicationManager->getRenderUIPtr();
                if( renderUI )
                {
                    auto image = renderUI->addElementByType<ui::IUIImage>();
                    setImage( image );
                    setElement( image );

                    setupMaterial();
                    updateMaterials();

                    updateVisibility();
                    updateColour();
                }
            }
        }
    }

    void Image::setupMaterial()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto resourceDatabase = applicationManager->getResourceDatabase();
            WP_ASSERT( resourceDatabase );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            auto baseMaterialName = m_baseMaterialName;
            auto materialResource =
                resourceDatabase->findResourceByType<render::IMaterial>( baseMaterialName );
            if( materialResource )
            {
                if( auto actor = getActor() )
                {
                    if( auto materialComponent = actor->getComponent<Material>() )
                    {
                        if( !m_material )
                        {
                            if( auto handle = getHandle() )
                            {
                                auto uuid = handle->getUUIDAsString();
                                if( StringUtil::isNullOrEmpty( uuid ) )
                                {
                                    uuid = StringUtil::getUUID();
                                }

                                //WP_ASSERT( materialResource->getMaterialType() ==
                                //           MaterialType::UI );

                                auto clonedMaterial =
                                    resourceDatabase->cloneResourceByType<render::IMaterial>(
                                        materialResource, uuid );
                                WP_ASSERT( clonedMaterial );
                                //WP_ASSERT( clonedMaterial->getMaterialType() ==
                                //           MaterialType::UI );

                                m_material = clonedMaterial;
                                //m_material = materialResource;
                            }
                        }

                        materialComponent->setMaterial( m_material );
                    }
                }
            }

            if( m_material )
            {
                if( !m_material->isLoaded() )
                {
                    if( graphicsSystem )
                    {
                        graphicsSystem->loadObject( m_material.load() );
                    }
                    else
                    {
                        m_material->load( nullptr );
                    }
                }
            }

            if( m_material )
            {
                m_material->setTexture( m_texture );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Image::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &state = getLoadingState();
            if( state != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                UIComponent::unload( data );
                m_image = nullptr;

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Image::getImage() const -> SmartPtr<ui::IUIImage>
    {
        return m_image;
    }

    void Image::setImage( SmartPtr<ui::IUIImage> image )
    {
        m_image = image;
    }

    void Image::updateMaterials()
    {
        if( auto texture = getTexture() )
        {
            if( m_image )
            {
                m_image->setTexture( texture );
            }
        }
    }

    auto Image::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = UIComponent::getProperties();

        auto texture = getTexture();
        properties->setProperty( TextureStr, texture );
        properties->setProperty( UseTilingStr, m_useTiling.load() );
        properties->setProperty( BaseMaterialNameStr, m_baseMaterialName );

        return properties;
    }

    void Image::setProperties( SmartPtr<Properties> properties )
    {
        UIComponent::setProperties( properties );

        SmartPtr<render::ITexture> texture;
        properties->getPropertyValue( TextureStr, texture );

        bool useTiling = m_useTiling.load();
        properties->getPropertyValue( UseTilingStr, useTiling );
        m_useTiling.store( useTiling );

        properties->getPropertyValue( BaseMaterialNameStr, m_baseMaterialName );

        if( texture != m_texture )
        {
            m_texture = texture;

            setupMaterial();

            if( auto actor = getActorPtr() )
            {
                if( auto material = actor->getComponentPtr<Material>() )
                {
                    material->updateImageComponent();
                }
            }

            updateMaterials();
        }

        if( auto image = getImage() )
        {
            image->setUseTiling( m_useTiling.load() );
        }
    }

    auto Image::isValid() const -> bool
    {
        const auto &state = getLoadingState();
        switch( state )
        {
        case LoadingState::Unloaded:
        {
            return m_image == nullptr;
        }
        break;
        case LoadingState::Loading:
        {
            return m_image == nullptr;
        }
        break;
        case LoadingState::Loaded:
        {
            //return m_image != nullptr && m_image->isValid();
            return true;
        }
        break;
        case LoadingState::Unloading:
        {
            return m_image == nullptr;
        }
        break;
        default:
        {
        }
        }

        return false;
    }

    auto Image::getTexture() const -> SmartPtr<render::ITexture>
    {
        return m_texture;
    }

    void Image::setTexture( SmartPtr<render::ITexture> texture )
    {
        if( texture != m_texture )
        {
            m_texture = texture;

            setupMaterial();

            if( auto actor = getActorPtr() )
            {
                if( auto material = actor->getComponent<Material>() )
                {
                    material->updateImageComponent();
                }
            }

            updateMaterials();
        }
    }

    auto Image::getTextureName() const -> String
    {
        if( auto texture = getTexture() )
        {
            return texture->getName();
        }

        return {};
    }

    void Image::setTextureName( const String &textureName )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabase();
        WP_ASSERT( resourceDatabase );

        auto texture = resourceDatabase->loadResourceByType<render::ITexture>( textureName );
        setTexture( texture );
    }

    void Image::setUseTiling( bool useTiling )
    {
        m_useTiling = useTiling;
    }

    bool Image::getUseTiling() const
    {
        return m_useTiling;
    }

    String Image::getBaseMaterialName() const
    {
        return m_baseMaterialName;
    }

    void Image::setBaseMaterialName( const String &name )
    {
        m_baseMaterialName = name;
    }

}  // namespace workphone::scene
