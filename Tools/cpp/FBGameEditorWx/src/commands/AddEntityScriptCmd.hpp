#ifndef _ADD_ENTITY_SCRIPT_CMD_H
#define _ADD_ENTITY_SCRIPT_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
	
		//--------------------------------------------
		class AddEntityScriptCmd : public CSharedObject<ICommand>
		{
		public:
			AddEntityScriptCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~AddEntityScriptCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		
		
		typedef SmartPtr<AddEntityScriptCmd> AddEntityScriptCmdPtr;
		
	
	
	} // end namespace editor
	
}


#endif // _ADD_ENTITY_SCRIPT_CMD_H