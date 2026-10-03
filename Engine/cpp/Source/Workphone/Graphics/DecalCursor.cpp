#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/DecalCursor.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, DecalCursor, IDecalCursor );

    DecalCursor::~DecalCursor() = default;

    DecalCursor::DecalCursor() = default;

    bool DecalCursor::isVisible() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void DecalCursor::setVisible( bool visible )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    workphone::Vector3<workphone::real_Num> DecalCursor::getPosition() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void DecalCursor::setPosition( const Vector3<real_Num> &position )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    workphone::Vector2<workphone::real_Num> DecalCursor::getSize() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void DecalCursor::setSize( const Vector2<real_Num> &size )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    workphone::SmartPtr<workphone::render::IMaterial> DecalCursor::getTerrainMaterial() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void DecalCursor::setTerrainMaterial( SmartPtr<IMaterial> material )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    String DecalCursor::getDecalTextureName() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void DecalCursor::setDecalTextureName( const String &textureName )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    workphone::SmartPtr<workphone::render::ITexture> DecalCursor::getDecalTexture() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void DecalCursor::setDecalTexture( SmartPtr<ITexture> texture )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void DecalCursor::addDebugEntity( const String &entityName,
                                      const Vector3<real_Num> &scale /*= Vector3<real_Num>::unit() */ )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void DecalCursor::removeDebugEntity()
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

}  // namespace workphone::render
