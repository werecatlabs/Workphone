#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessageSetFlag.hpp"
#include "Workphone/Core/Properties.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    StateMessageSetFlag::StateMessageSetFlag() = default;

    StateMessageSetFlag::~StateMessageSetFlag() = default;

    u32 StateMessageSetFlag::getFlags() const
    {
        return m_flags;
    }

    void StateMessageSetFlag::setFlags( u32 flags )
    {
        m_flags = flags;
    }

}  // namespace workphone
