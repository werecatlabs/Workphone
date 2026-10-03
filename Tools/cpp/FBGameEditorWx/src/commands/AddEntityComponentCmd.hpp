#ifndef _ADD_ENTITY_COMPONENT_CMD_H
#define _ADD_ENTITY_COMPONENT_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
		
		//--------------------------------------------
		class AddEntityComponentCmd : public CSharedObject<ICommand>
		{
		public:
			AddEntityComponentCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~AddEntityComponentCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		typedef SmartPtr<AddEntityComponentCmd> AddEntityComponentCmdPtr;
		
	}
}




#endif // _ADD_ENTITY_COMPONENT_CMD_H