#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralScene.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IProceduralScene, ISharedObject );

    IProceduralScene::~IProceduralScene() = default;

}  // namespace workphone::procedural
