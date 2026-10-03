#ifndef _REMOVE_ENTITY_NODE_CMD_H
#define _REMOVE_ENTITY_NODE_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		

		
		//--------------------------------------------
		class RemoveEntityNodeCmd : public CSharedObject<ICommand>
		{
		public:
			RemoveEntityNodeCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~RemoveEntityNodeCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		typedef SmartPtr<RemoveEntityNodeCmd> RemoveEntityNodeCmdPtr;
		
	}
	
}

#endif // _REMOVE_ENTITY_NODE_CMD_H