#ifndef _REMOVE_ENTITY_BODY_CMD_H
#define _REMOVE_ENTITY_BODY_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
		//--------------------------------------------
		class RemoveEntityBodyCmd : public CSharedObject<ICommand>
		{
		public:
			RemoveEntityBodyCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~RemoveEntityBodyCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		
		
		typedef SmartPtr<RemoveEntityBodyCmd> RemoveEntityBodyCmdPtr;
		
	}
}


#endif // _REMOVE_ENTITY_BODY_CMD_H


