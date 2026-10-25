#ifndef WP_EDITOR_MATERIAL_SHADER_RELOAD_ACTIONS_HPP
#define WP_EDITOR_MATERIAL_SHADER_RELOAD_ACTIONS_HPP
#include <EditorPrerequisites.hpp>
namespace workphone::editor
{
    // UI-thread request/status bridge. Mutations only queue graphics-owner work;
    // reading status never invokes a compiler or touches a live native context.
    void processMaterialShaderReloadAction(SmartPtr<render::IMaterial> material,
                                           SmartPtr<Properties> request);
}
#endif
