#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageSkyBox.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageSkyBox, StateMessage );

    auto StateMessageSkyBox::getMaterial() const -> SmartPtr<render::IMaterial>
    {
        return m_material;
    }

    void StateMessageSkyBox::setMaterial( SmartPtr<render::IMaterial> material )
    {
        m_material = material;
    }

    auto StateMessageSkyBox::getMaterialName() const -> String
    {
        return m_materialName;
    }

    void StateMessageSkyBox::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
    }

    auto StateMessageSkyBox::getDistance() const -> f32
    {
        return m_distance;
    }

    void StateMessageSkyBox::setDistance( f32 value )
    {
        m_distance = value;
    }

    auto StateMessageSkyBox::getEnable() const -> bool
    {
        return m_enable;
    }

    void StateMessageSkyBox::setEnable( bool value )
    {
        m_enable = value;
    }

    auto StateMessageSkyBox::getDrawFirst() const -> bool
    {
        return m_drawFirst;
    }

    void StateMessageSkyBox::setDrawFirst( bool value )
    {
        m_drawFirst = value;
    }
}  // namespace workphone
