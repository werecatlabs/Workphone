#ifndef _REMOVE_ENTITY_SHAPE_CMD_H
#define _REMOVE_ENTITY_SHAPE_CMD_H


#include <GameEditorPrerequisites.hpp>

#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>


namespace fb
{	
	namespace editor
	{
		
		
		
		
		//--------------------------------------------
		class RemoveEntityShapeCmd : public CSharedObject<ICommand>
		{
		public:
			RemoveEntityShapeCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~RemoveEntityShapeCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		typedef SmartPtr<RemoveEntityShapeCmd> RemoveEntityShapeCmdPtr;
		
	}
	
}

#endif // _REMOVE_ENTITY_SHAPE_CMD_H