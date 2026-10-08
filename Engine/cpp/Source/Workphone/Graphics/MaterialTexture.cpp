#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Interface/Animation/IAnimator.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::render
{
    const String MaterialTexture::textureStr = String( "texture" );
    const String MaterialTexture::generateStr = String( "Generate" );

    WP_CLASS_REGISTER_DERIVED( workphone::render, MaterialTexture, MaterialNode<IMaterialTexture> );

    MaterialTexture::MaterialTexture()
    {
        load( nullptr );
    }

    MaterialTexture::~MaterialTexture()
    {
        unload( nullptr );
    }

    void MaterialTexture::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            if( auto texture = getTexture() )
            {
                if( !texture->isLoaded() )
                {
                    graphicsSystem->loadObject( texture );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialTexture::createTextureUnitState()
    {
        //auto parent = getParent();
        //auto parentPass = fb::static_pointer_cast<CMaterialPass>( parent );
        //if( parentPass )
        //{
        //    auto pass = parentPass->getPass();
        //    WP_ASSERT( pass );

        //    if( pass )
        //    {
        //        //m_textureUnitState = pass->createTextureUnitState();
        //    }
        //}
    }

    void MaterialTexture::reload( SmartPtr<ISharedObject> data )
    {
    }

    void MaterialTexture::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                m_animator = nullptr;
                m_texture = nullptr;
                MaterialNode<IMaterialTexture>::unload( data );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialTexture::getTextureName() const -> String
    {
        if( auto texture = getTexture() )
        {
            return texture->getFilePath();
        }

        return {};
    }

    void MaterialTexture::setTextureName( const String &texturePath )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto resourceDatabase = applicationManager->getResourceDatabase();
        auto texture = resourceDatabase->loadResourceByType<ITexture>( texturePath );
        if( texture )
        {
            setTexture( texture );
        }
    }

    auto MaterialTexture::getTexture() const -> SmartPtr<ITexture>
    {
        return m_texture;
    }

    void MaterialTexture::setTexture( SmartPtr<ITexture> texture )
    {
        m_texture = texture;
    }

    void MaterialTexture::setScale( const Vector3<real_Num> &scale )
    {
        m_scale = scale;
    }

    auto MaterialTexture::getAnimator() const -> SmartPtr<IAnimator>
    {
        return m_animator;
    }

    void MaterialTexture::setAnimator( SmartPtr<IAnimator> animator )
    {
        m_animator = animator;
    }

    auto MaterialTexture::getTint() const -> ColourF
    {
        return m_tint;
    }

    void MaterialTexture::setTint( const ColourF &tint )
    {
        m_tint = tint;
    }

    void MaterialTexture::_getObject( void **ppObject )
    {
        *ppObject = nullptr;
    }

    auto MaterialTexture::toData() const -> SmartPtr<ISharedObject>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            ScopedLock lock( graphicsSystem );

            auto properties = factoryManager->make_ptr<Properties>();

            if( auto texture = getTexture() )
            {
                auto handle = texture->getHandle();
                properties->setProperty( MaterialTexture::textureStr, handle->getUUIDAsString() );
                // Keep a path fallback for materials used without an asset database entry.
                properties->setProperty( texturePathStr, texture->getFilePath() );
            }

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void MaterialTexture::fromData( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            ScopedLock lock( graphicsSystem );

            auto properties = workphone::static_pointer_cast<Properties>( data );

            auto resourceDatabase = applicationManager->getResourceDatabase();
            if( !resourceDatabase )
            {
                WP_LOG( "Resource database null." );
            }

            if( resourceDatabase )
            {
                auto sTextureUUID = properties->getProperty( MaterialTexture::textureStr );
                // Older material files stored a texture path in the texture field.
                // Only parse identifiers here; retain legacy paths as a fallback.
                const auto isUUID = sTextureUUID.size() == 36u && sTextureUUID[8] == '-' &&
                                    sTextureUUID[13] == '-' && sTextureUUID[18] == '-' &&
                                    sTextureUUID[23] == '-';
                if( isUUID )
                {
                    auto uuid = StringUtil::parseUUID( sTextureUUID );
                    if( auto textureResource = resourceDatabase->loadResourceById( uuid ) )
                    {
                        auto texture = workphone::static_pointer_cast<ITexture>( textureResource );

                        if( !texture->isLoaded() )
                        {
                            texture->load( nullptr );
                        }

                        setTexture( texture );
                    }
                }
                if( !getTexture() )
                {
                    auto path = properties->getProperty( texturePathStr );
                    if( path.empty() && !isUUID )
                        path = sTextureUUID;
                    if( !path.empty() )
                    {
                        if( auto texture = resourceDatabase->loadResourceByType<ITexture>( path ) )
                        {
                            if( !texture->isLoaded() )
                                texture->load( nullptr );
                            setTexture( texture );
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialTexture::getProperties() const -> SmartPtr<Properties>
    {
        try
        {
            auto properties = MaterialNode<IMaterialTexture>::getProperties();

            properties->setProperty( scaleStr, m_scale );
            properties->setProperty( tintStr, m_tint );

            auto texture = getTexture();
            properties->setProperty( texturePathStr, texture );

            auto material = getMaterial();
            if( material )
            {
                auto materialType = material->getMaterialType();
                switch( materialType )
                {
                case MaterialType::Standard:
                case MaterialType::StandardSpecular:
                case MaterialType::StandardTriPlanar:
                case MaterialType::TerrainStandard:
                case MaterialType::TerrainSpecular:
                case MaterialType::TerrainDiffuse:
                case MaterialType::Skybox:
                {
                    //auto textureType =
                    //    static_cast<SkyboxTextureTypes>( getTextureType() );
                    //switch( textureType )
                    //{
                    //case SkyboxCubeTextureTypes::Cube:
                    //{
                    //    properties->setProperty( MaterialTexture::generateStr, "GenerateButton", "button", false );
                    //}
                    //break;
                    //default:
                    //{
                    //}
                    //break;
                    //};
                }
                break;
                case MaterialType::SkyboxCubemap:
                case MaterialType::UI:
                case MaterialType::Custom:
                {
                }
                break;
                default:
                {
                }
                break;
                };
            }

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void MaterialTexture::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto textureManager = graphicsSystem->getTextureManager();
            WP_ASSERT( textureManager );

            properties->getPropertyValue( scaleStr, m_scale );
            properties->getPropertyValue( tintStr, m_tint );

            auto texture = SmartPtr<ITexture>();
            properties->getPropertyValue( texturePathStr, texture );

            if( getTexture() != texture )
            {
                setTexture( texture );
            }

            if( properties->hasProperty( "Generate" ) )
            {
                auto &generateButton = properties->getPropertyObject( "Generate" );
                if( generateButton.getAttribute( "click" ) == "true" )
                {
                    Array<SmartPtr<ITexture>> skyboxTextures;

                    auto parent = getParent();
                    auto pass = workphone::static_pointer_cast<IMaterialPass>( parent );

                    auto textureUnits = pass->getTextureUnits();
                    for( auto textureUnit : textureUnits )
                    {
                        auto texture = textureUnit->getTexture();
                        skyboxTextures.push_back( texture );
                    }

                    textureManager->createCubeMap( skyboxTextures );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialTexture::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        try
        {
            auto objects = MaterialNode<IMaterialTexture>::getChildObjects();
            objects.reserve( 2 );

            objects.emplace_back( m_animator );
            objects.emplace_back( m_texture.load() );
            return objects;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    auto MaterialTexture::getTextureType() const -> u32
    {
        return m_textureType;
    }

    void MaterialTexture::setTextureType( u32 textureType )
    {
        m_textureType = textureType;
    }

    bool MaterialTexture::MaterialTextureStateListener::handleStateMessage(
        const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    bool MaterialTexture::MaterialTextureStateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    MaterialTexture::MaterialTextureStateListener::MaterialTextureStateListener() = default;

    MaterialTexture::MaterialTextureStateListener::~MaterialTextureStateListener() = default;

}  // namespace workphone::render
