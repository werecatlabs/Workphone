#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IAnimationControllerListener.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( render, IAnimationControllerListener, ISharedObject );

    IAnimationControllerListener::~IAnimationControllerListener() = default;
}  // namespace workphone::render
