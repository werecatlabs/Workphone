#ifndef _ADD_ENTITY_GFXOBJ_CMD_H
#define _ADD_ENTITY_GFXOBJ_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
		//--------------------------------------------
		class AddEntityGfxObjCmd : public CSharedObject<ICommand>
		{
		public:
			AddEntityGfxObjCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~AddEntityGfxObjCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		
	}
}


#endif // _ADD_ENTITY_GFXOBJ_CMD_H