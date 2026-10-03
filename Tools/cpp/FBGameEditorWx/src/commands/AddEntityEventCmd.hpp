#ifndef _ADD_ENTITY_EVENT_CMD_H
#define _ADD_ENTITY_EVENT_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		

		
		//--------------------------------------------
		class AddEntityEventCmd : public CSharedObject<ICommand>
		{
		public:
			AddEntityEventCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~AddEntityEventCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		


	} // end namespace editor	
} // end namespace fb	



#endif // ADD_ENTITY_EVENT_CMD_H


