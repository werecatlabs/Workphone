#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawMaterialTexture.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawMaterialTexture, MaterialTexture );

    ClawMaterialTexture::ClawMaterialTexture() :
        m_textureName( "" ),
        m_texture( nullptr ),
        m_tint( ColourF::White ),
        m_textureType( 0 ),
        m_animator( nullptr )
    {
    }

    ClawMaterialTexture::~ClawMaterialTexture()
    {
    }

    String ClawMaterialTexture::getTextureName() const
    {
        return m_textureName;
    }

    void ClawMaterialTexture::setTextureName( const String &name )
    {
        m_textureName = name;
        m_texture = nullptr;
        if( StringUtil::isNullOrEmpty( name ) )
            return;

        if( auto applicationManager = core::IApplicationManager::instancePtr() )
        {
            if( auto database = applicationManager->getResourceDatabase() )
            {
                auto texture = database->loadResourceByType<ITexture>( name );
                if( texture && !texture->isLoaded() )
                    texture->load( nullptr );
                if( texture )
                    setTexture( texture );
            }
        }
    }

    SmartPtr<ITexture> ClawMaterialTexture::getTexture() const
    {
        return m_texture;
    }

    void ClawMaterialTexture::setTexture( SmartPtr<ITexture> texture )
    {
        m_texture = texture;
        if( texture )
        {
            m_textureName = texture->getFilePath();
            if( StringUtil::isNullOrEmpty( m_textureName ) )
                m_textureName = texture->getName();
        }
        else
        {
            m_textureName = String();
        }
    }

    void ClawMaterialTexture::setScale( const Vector3<real_Num> &scale )
    {
        m_scale = scale;
    }

    SmartPtr<IAnimator> ClawMaterialTexture::getAnimator() const
    {
        return m_animator;
    }

    void ClawMaterialTexture::setAnimator( SmartPtr<IAnimator> animator )
    {
        m_animator = animator;
    }

    ColourF ClawMaterialTexture::getTint() const
    {
        return m_tint;
    }

    void ClawMaterialTexture::setTint( const ColourF &tint )
    {
        m_tint = tint;
    }

    u32 ClawMaterialTexture::getTextureType() const
    {
        return m_textureType;
    }

    void ClawMaterialTexture::setTextureType( u32 textureType )
    {
        m_textureType = textureType;
    }

    void ClawMaterialTexture::_getObject( void **ppObject )
    {
        // This would point to a native renderer texture handle if applicable.
        *ppObject = nullptr;
    }

    SmartPtr<Properties> ClawMaterialTexture::getProperties() const
    {
        auto props = workphone::make_ptr<Properties>();
        props->setProperty( IMaterialTexture::texturePathStr, m_textureName );
        props->setProperty( IMaterialTexture::scaleStr, m_scale );
        props->setProperty( IMaterialTexture::tintStr, m_tint );
        props->setProperty( IMaterialTexture::textureTypeStr, m_textureType );
        if( m_texture )
        {
            if( auto handle = m_texture->getHandle() )
                props->setProperty( MaterialTexture::textureStr, handle->getUUIDAsString() );
        }
        return props;
    }

    SmartPtr<ISharedObject> ClawMaterialTexture::toData() const
    {
        return getProperties();
    }

    void ClawMaterialTexture::fromData( SmartPtr<ISharedObject> data )
    {
        if( auto properties = workphone::dynamic_pointer_cast<Properties>( data ) )
        {
            setTexture( nullptr );
            setProperties( properties );
            // Existing engine materials serialize texture resources by UUID.
            if( !StringUtil::isNullOrEmpty( properties->getProperty( MaterialTexture::textureStr ) ) )
                MaterialTexture::fromData( data );
        }
    }

    void ClawMaterialTexture::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        String name;
        if( properties->getPropertyValue( IMaterialTexture::texturePathStr, name ) )
        {
            setTextureName( name );
        }

        Vector3<real_Num> scale;
        if( properties->getPropertyValue( IMaterialTexture::scaleStr, scale ) )
        {
            setScale( scale );
        }

        ColourF tint;
        if( properties->getPropertyValue( IMaterialTexture::tintStr, tint ) )
        {
            setTint( tint );
        }

        u32 type = 0;
        if( properties->getPropertyValue( IMaterialTexture::textureTypeStr, type ) )
        {
            setTextureType( type );
        }
    }

    bool ClawMaterialTexture::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;  // Basic implementation; can be extended to react to texture loading messages.
    }

    bool ClawMaterialTexture::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;  // Basic implementation.
    }

}  // namespace workphone::render
