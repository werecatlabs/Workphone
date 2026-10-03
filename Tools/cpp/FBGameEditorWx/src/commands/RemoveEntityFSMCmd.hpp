#ifndef _REMOVE_ENTITY_FSM_CMD_H
#define _REMOVE_ENTITY_FSM_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
		
		//--------------------------------------------
		class RemoveEntityFSMCmd : public CSharedObject<ICommand>
		{
		public:
			RemoveEntityFSMCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~RemoveEntityFSMCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		
			Array<SmartPtr<ICommand>> m_commandsEvents;
			Array<SmartPtr<ICommand>> m_commandsStates;
		};
		
		typedef SmartPtr<RemoveEntityFSMCmd> RemoveEntityFSMCmdPtr;
		
	}
	
}

#endif // _REMOVE_ENTITY_FSM_CMD_H