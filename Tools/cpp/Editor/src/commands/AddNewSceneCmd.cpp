#include <EditorPCH.hpp>
#include "commands/AddNewSceneCmd.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AddNewSceneCmd, ICommand );

    AddNewSceneCmd::AddNewSceneCmd( Properties properties ) : m_properties( properties )
    {
    }

    AddNewSceneCmd::~AddNewSceneCmd() = default;

    void AddNewSceneCmd::undo()
    {
    }

    void AddNewSceneCmd::redo()
    {
    }

    void AddNewSceneCmd::execute()
    {
    }
}  // namespace workphone::editor
