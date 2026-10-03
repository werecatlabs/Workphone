#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageMaterial.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageMaterial, StateMessage );

    auto StateMessageMaterial::getMaterial() const -> SmartPtr<render::IMaterial>
    {
        return m_material;
    }

    void StateMessageMaterial::setMaterial( SmartPtr<render::IMaterial> material )
    {
        m_material = material;
    }

    auto StateMessageMaterial::getIndex() const -> s32
    {
        return m_index;
    }

    void StateMessageMaterial::setIndex( s32 index )
    {
        m_index = index;
    }

}  // namespace workphone
