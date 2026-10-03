#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageDirty.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageDirty, StateMessage );

    StateMessageDirty::StateMessageDirty() = default;
    StateMessageDirty::~StateMessageDirty() = default;

    auto StateMessageDirty::isDirty() const -> bool
    {
        return m_isDirty;
    }

    void StateMessageDirty::setDirty( bool dirty )
    {
        m_isDirty = dirty;
    }

}  // namespace workphone
