#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageMaterialName.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageMaterialName, StateMessage );

    StateMessageMaterialName::StateMessageMaterialName() = default;

    StateMessageMaterialName::~StateMessageMaterialName() = default;

    auto StateMessageMaterialName::getMaterialName() const -> String
    {
        return m_value;
    }

    void StateMessageMaterialName::setMaterialName( const String &value )
    {
        m_value = value;
    }

    auto StateMessageMaterialName::getIndex() const -> u32
    {
        return m_index;
    }

    void StateMessageMaterialName::setIndex( u32 index )
    {
        m_index = index;
    }
}  // namespace workphone
