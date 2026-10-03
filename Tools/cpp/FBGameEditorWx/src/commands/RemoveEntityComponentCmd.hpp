#ifndef _REMOVE_ENTITY_COMPONENT_CMD_H
#define _REMOVE_ENTITY_COMPONENT_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
				
		
		
		//--------------------------------------------
		class RemoveEntityComponentCmd : public CSharedObject<ICommand>
		{
		public:
			RemoveEntityComponentCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~RemoveEntityComponentCmd();
		
			void undo();
			void redo();
			void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		typedef SmartPtr<RemoveEntityComponentCmd> RemoveEntityComponentCmdPtr;
		
	}
	
}

#endif // RemoveEntityComponentCmd