#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IAnimationStateController.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( render, IAnimationStateController, ISharedObject );

    IAnimationStateController::~IAnimationStateController() = default;

}  // namespace workphone::render
