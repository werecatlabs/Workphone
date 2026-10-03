#ifndef _REMOVE_FSM_STATE_CMD_H
#define _REMOVE_FSM_STATE_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
		
		//--------------------------------------------
		class RemoveFSMStateCmd : public CSharedObject<ICommand>
		{
		public:
			RemoveFSMStateCmd(const Properties& propertyGroup, SmartPtr<FSMTemplate> parentFsm, SmartPtr<EntityTemplate> parentEnt);
			~RemoveFSMStateCmd();
		
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
		


		typedef SmartPtr<RemoveFSMStateCmd> RemoveFSMStateCmdPtr;
		


	}
}




#endif // _REMOVE_FSM_STATE_CMD_H