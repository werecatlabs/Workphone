#include <EditorPCH.hpp>
#include <commands/CommandStringList.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace editor
    {
        WP_CLASS_REGISTER_DERIVED( workphone::editor, CommandStringList, ISharedObject );

        CommandStringList::CommandStringList()
        {
        }

        CommandStringList::~CommandStringList()
        {
        }

        void CommandStringList::addCommand( const String &command )
        {
            m_commands.push_back( command );
        }

        void CommandStringList::clear()
        {
            m_commands.clear();
        }

        const Array<String> &CommandStringList::getCommands() const
        {
            return m_commands;
        }
    }  // namespace editor
}  // namespace workphone
