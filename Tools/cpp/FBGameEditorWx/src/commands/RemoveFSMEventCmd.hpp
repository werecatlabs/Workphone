#ifndef _REMOVE_FSM_EVENT_CMD_H
#define _REMOVE_FSM_EVENT_CMD_H


#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
		
		//--------------------------------------------
		class RemoveFSMEventCmd : public CSharedObject<ICommand>
		{
		public:
			RemoveFSMEventCmd(const Properties& propertyGroup, SmartPtr<FSMTemplate> parentFsm, SmartPtr<EntityTemplate> parentEnt);
			~RemoveFSMEventCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<FSMTemplate> m_parentFsm;
			String m_fsmName;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		typedef SmartPtr<RemoveFSMEventCmd> RemoveFSMEventCmdPtr;
		
	}
	
}

#endif // _REMOVE_FSM_EVENT_CMD_H