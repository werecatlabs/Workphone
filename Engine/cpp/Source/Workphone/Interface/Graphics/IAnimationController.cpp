#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IAnimationController.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( render, IAnimationController, ISharedObject );

    IAnimationController::~IAnimationController() = default;

}  // namespace workphone::render
