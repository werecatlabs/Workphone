#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IAnimationState.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( render, IAnimationState, ISharedObject );

    IAnimationState::~IAnimationState() = default;

}  // namespace workphone::render
