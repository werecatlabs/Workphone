#include <EditorPCH.hpp>
#include "commands/Command.hpp"
#include <commands/CommandList.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include "ui/UIManager.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, Command, ICommand );

    Command::Command() = default;

    Command::~Command() = default;

    void Command::undo()
    {
    }

    void Command::redo()
    {
    }

    void Command::execute()
    {
    }

    ICommand::State Command::getState() const
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return m_commandState;
    }

    void Command::setState( State state )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        m_commandState = state;
    }

    bool Command::isPrimary() const
    {
        return m_isPrimary;
    }

    void Command::setPrimary( bool primary )
    {
        m_isPrimary = primary;
    }
}  // namespace workphone::editor
