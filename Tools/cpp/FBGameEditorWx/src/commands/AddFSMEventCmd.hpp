#ifndef _ADD_FSM_EVENT_CMD_H
#define _ADD_FSM_EVENT_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
		
		//--------------------------------------------
		class AddFSMEventCmd : public CSharedObject<ICommand>
		{
		public:
			AddFSMEventCmd(const Properties& propertyGroup, SmartPtr<FSMTemplate> parentFsm, SmartPtr<EntityTemplate> parentEnt);
			~AddFSMEventCmd();
		
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
		
		typedef SmartPtr<AddFSMEventCmd> AddFSMEventCmdPtr;
		
	}
	
}

#endif // _ADD_FSM_EVENT_CMD_H