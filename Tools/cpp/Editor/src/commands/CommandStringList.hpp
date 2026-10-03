#ifndef __CommandStringList_h__
#define __CommandStringList_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/System/ICommand.hpp>

namespace workphone
{
    namespace editor
    {
        class CommandStringList : public ISharedObject
        {
        public:
            CommandStringList();
            ~CommandStringList() override;

            void addCommand( const String &command );

            void clear();

            const Array<String> &getCommands() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<String> m_commands;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // __CommandStringList_h__
