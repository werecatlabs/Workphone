#ifndef ModifyPropertyCmd_h__
#define ModifyPropertyCmd_h__

#include <commands/Command.hpp>

namespace workphone
{
    namespace editor
    {
        class ModifyPropertyCmd : public Command
        {
        public:
            ModifyPropertyCmd();
            ~ModifyPropertyCmd() override;

            void undo() override;
            void redo() override;
            void execute() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // ModifyPropertyCmd_h__
