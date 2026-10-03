#include <EditorPCH.hpp>
#include <commands/ModifyPropertyCmd.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, ModifyPropertyCmd, Command );

    ModifyPropertyCmd::ModifyPropertyCmd() = default;

    ModifyPropertyCmd::~ModifyPropertyCmd() = default;

    void ModifyPropertyCmd::undo()
    {
    }

    void ModifyPropertyCmd::redo()
    {
    }

    void ModifyPropertyCmd::execute()
    {
    }
}  // namespace workphone::editor
