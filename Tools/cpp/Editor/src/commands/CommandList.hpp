#ifndef CommandList_h__
#define CommandList_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/System/ICommand.hpp>

namespace workphone
{
    namespace editor
    {
        class CommandList : public ISharedObject
        {
        public:
            CommandList();
            ~CommandList() override;

            void addCommand( SmartPtr<ICommand> command );

            void undo();
            void redo();

            void clear();

            const Array<SmartPtr<ICommand>> &getCommands() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<SmartPtr<ICommand>> m_commands;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // CommandList_h__
