#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/IGameSceneBuilder.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, IGameSceneBuilder, IObjectBuilder );

    IGameSceneBuilder::~IGameSceneBuilder() = default;

}  // namespace workphone::scene
