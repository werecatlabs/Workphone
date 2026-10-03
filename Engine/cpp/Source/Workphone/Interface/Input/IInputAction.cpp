#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IInputAction.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IInputAction, ISharedObject );

    IInputAction::~IInputAction() = default;

}  // namespace workphone
