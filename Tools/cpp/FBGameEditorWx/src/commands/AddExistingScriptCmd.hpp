#ifndef AddExistingScriptCmd_h__
#define AddExistingScriptCmd_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
	
		//--------------------------------------------
		class AddExistingScriptCmd : public CSharedObject<ICommand>
		{
		public:
			AddExistingScriptCmd(const Properties& properties);
			~AddExistingScriptCmd();
		
			void undo();
			void redo();
			void execute();
		
			String getCommandId() const;
	
			SmartPtr<ScriptTemplate> getScriptTemplate() const { return m_scriptTemplate; }
			void setScriptTemplate(SmartPtr<ScriptTemplate> val) { m_scriptTemplate = val; }
	
			String getFilterName() const { return m_filterName; }
			void setFilterName(const String& val) { m_filterName = val; }
		
		private:
			SmartPtr<ScriptTemplate> m_scriptTemplate;
			Properties m_properties;
			String m_entityName;
			String m_filterName;
		};
		
		
		
		typedef SmartPtr<AddExistingScriptCmd> AddExistingScriptCmdPtr;
		
	
	
	} // end namespace editor
	
}


#endif // AddExistingScriptCmd_h__


