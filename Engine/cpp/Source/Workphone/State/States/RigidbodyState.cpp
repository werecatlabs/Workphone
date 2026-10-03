#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/RigidbodyState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, RigidbodyState, StateData );

    RigidbodyState::RigidbodyState() : StateData( RigidbodyState::typeInfo() )
    {
    }

    RigidbodyState::~RigidbodyState() = default;

}  // namespace workphone
