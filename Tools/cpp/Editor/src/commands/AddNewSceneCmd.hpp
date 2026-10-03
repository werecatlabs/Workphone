#ifndef AddNewSceneCmd_h__
#define AddNewSceneCmd_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{
    namespace editor
    {
        class AddNewSceneCmd : public ICommand
        {
        public:
            AddNewSceneCmd( Properties properties );
            ~AddNewSceneCmd() override;

            void undo() override;
            void redo() override;
            void execute() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Properties m_properties;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // AddNewSceneCmd_h__
