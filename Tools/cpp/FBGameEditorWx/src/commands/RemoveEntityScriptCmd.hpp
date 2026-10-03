#ifndef _REMOVE_ENTITY_SCRIPT_CMD_H
#define _REMOVE_ENTITY_SCRIPT_CMD_H


#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
		
		//--------------------------------------------
		class RemoveEntityScriptCmd : public CSharedObject<ICommand>
		{
		public:
			RemoveEntityScriptCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~RemoveEntityScriptCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		typedef SmartPtr<RemoveEntityScriptCmd> RemoveEntityScriptCmdPtr;
		
	}
}


#endif // _REMOVE_ENTITY_SCRIPT_CMD_H