#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/IComponentEventListener.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, IComponentEventListener, ISharedObject );

    IComponentEventListener::~IComponentEventListener() = default;

}  // namespace workphone::scene
