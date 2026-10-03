#ifndef _ADD_FSM_STATE_CMD_H
#define _ADD_FSM_STATE_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
		
		//--------------------------------------------
		class AddFSMStateCmd : public CSharedObject<ICommand>
		{
		public:
			AddFSMStateCmd(const Properties& propertyGroup, SmartPtr<FSMTemplate> parentFsm, SmartPtr<EntityTemplate> parentEnt);
			~AddFSMStateCmd();
		
			void undo();
			void redo();
			void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<FSMTemplate> m_parentFsm;
			String m_fsmName;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		typedef SmartPtr<AddFSMStateCmd> AddFSMStateCmdPtr;
		
	}
	
}

#endif // _ADD_FSM_STATE_CMD_H