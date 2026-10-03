#ifndef AddNewScriptCmd_h__
#define AddNewScriptCmd_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Core/Properties.hpp>
#include <commands/Command.hpp>

namespace workphone
{
    namespace editor
    {
        class AddNewScriptCmd : public Command
        {
        public:
            AddNewScriptCmd();
            AddNewScriptCmd( const Properties &properties );
            ~AddNewScriptCmd() override;

            void redo() override;
            void execute() override;
            void undo() override;

            String getPath() const;
            void setPath( const String &filePath );

            String getFileName() const;
            void setFileName( const String &fileName );

            WP_CLASS_REGISTER_DECL;

        protected:
            Properties m_properties;
            String m_filePath;
            String m_fileName;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // AddNewScriptCmd_h__
