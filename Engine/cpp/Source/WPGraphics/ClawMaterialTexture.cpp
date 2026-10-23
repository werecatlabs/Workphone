#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawMaterialTexture.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawMaterialTexture, MaterialTexture );

    ClawMaterialTexture::ClawMaterialTexture() = default;

    ClawMaterialTexture::~ClawMaterialTexture()
    {
    }

    String ClawMaterialTexture::getTextureName() const
    {
        return MaterialTexture::getTextureName();
    }

    void ClawMaterialTexture::setTextureName( const String &name )
    {
        setTextureNameImpl( name, true );
    }

    SmartPtr<ITexture> ClawMaterialTexture::getTexture() const
    {
        return MaterialTexture::getTexture();
    }

    void ClawMaterialTexture::setTexture( SmartPtr<ITexture> texture )
    {
        MaterialTexture::setTexture( texture );
    }

    void ClawMaterialTexture::setScale( const Vector3<real_Num> &scale )
    {
        MaterialTexture::setScale( scale );
    }

    SmartPtr<IAnimator> ClawMaterialTexture::getAnimator() const
    {
        return MaterialTexture::getAnimator();
    }

    void ClawMaterialTexture::setAnimator( SmartPtr<IAnimator> animator )
    {
        MaterialTexture::setAnimator( animator );
    }

    ColourF ClawMaterialTexture::getTint() const
    {
        return MaterialTexture::getTint();
    }

    void ClawMaterialTexture::setTint( const ColourF &tint )
    {
        MaterialTexture::setTint( tint );
    }

    u32 ClawMaterialTexture::getTextureType() const
    {
        return MaterialTexture::getTextureType();
    }

    void ClawMaterialTexture::setTextureType( u32 textureType )
    {
        MaterialTexture::setTextureType( textureType );
    }

    void ClawMaterialTexture::_getObject( void **ppObject )
    {
        // This would point to a native renderer texture handle if applicable.
        *ppObject = nullptr;
    }

    SmartPtr<Properties> ClawMaterialTexture::getProperties() const
    {
        auto props = workphone::make_ptr<Properties>();
        props->setProperty( IMaterialTexture::texturePathStr, getTextureName() );
        props->setProperty( IMaterialTexture::scaleStr, getScale() );
        props->setProperty( IMaterialTexture::tintStr, getTint() );
        props->setProperty( IMaterialTexture::textureTypeStr, getTextureType() );
        if( auto texture = getTexture() )
        {
            if( auto handle = texture->getHandle() )
            {
                props->setProperty( MaterialTexture::textureStr, handle->getUUIDAsString() );
            }
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
            {
                MaterialTexture::fromData( data );
            }
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
