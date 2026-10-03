#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiScene.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAiScene, ISharedObject );

    IAiScene::~IAiScene() = default;

}  // namespace workphone
