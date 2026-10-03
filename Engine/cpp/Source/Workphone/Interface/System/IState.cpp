#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IState, ISharedObject );

    IState::IState() : ISharedObject( IState::typeInfo() )
    {
    }

    IState ::IState( u32 poolTypeId ) : ISharedObject( poolTypeId )
    {
    }

    IState::~IState() = default;

}  // namespace workphone
