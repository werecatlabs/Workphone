#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/IGameEditor.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, IGameEditor, ISharedObject );

    const String IGameEditor::loadStr = "load";
    const String IGameEditor::unloadStr = "unload";
    const String IGameEditor::showStr = "show";
    const String IGameEditor::hideStr = "hide";

    IGameEditor::~IGameEditor() = default;

}  // namespace workphone::scene
