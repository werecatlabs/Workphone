#include <EditorPCH.hpp>
#include <commands/CommandList.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace editor
    {
        WP_CLASS_REGISTER_DERIVED( workphone::editor, CommandList, ISharedObject );

        CommandList::CommandList()
        {
        }

        CommandList::~CommandList()
        {
            m_commands.clear();
        }

        void CommandList::addCommand( SmartPtr<ICommand> command )
        {
            m_commands.push_back( command );
        }

        void CommandList::undo()
        {
            if( m_commands.empty() )
            {
                return;
            }

            m_commands.back()->undo();
            m_commands.pop_back();
        }

        void CommandList::redo()
        {
            if( m_commands.empty() )
            {
                return;
            }

            m_commands.back()->redo();
        }

        void CommandList::clear()
        {
            m_commands.clear();
        }

        const Array<SmartPtr<ICommand>> &CommandList::getCommands() const
        {
            return m_commands;
        }
    }  // namespace editor
}  // namespace workphone
