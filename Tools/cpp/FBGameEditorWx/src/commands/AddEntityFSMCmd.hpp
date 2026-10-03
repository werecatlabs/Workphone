#ifndef _ADD_ENTITY_FSM_CMD_H
#define _ADD_ENTITY_FSM_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
				
		
		
		//--------------------------------------------
		class AddEntityFSMCmd : public CSharedObject<ICommand>
		{
		public:
			AddEntityFSMCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~AddEntityFSMCmd();
		
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
		
		typedef SmartPtr<AddEntityFSMCmd> AddEntityFSMCmdPtr;
		
	}
	
}



#endif // _ADD_ENTITY_FSM_CMD_H