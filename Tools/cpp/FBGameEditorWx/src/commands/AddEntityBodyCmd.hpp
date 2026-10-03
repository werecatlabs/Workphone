#ifndef _ADD_ENTITY_BODY_CMD_H
#define _ADD_ENTITY_BODY_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		


		//--------------------------------------------
		class AddEntityBodyCmd : public CSharedObject<ICommand>
		{
		public:
			AddEntityBodyCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~AddEntityBodyCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		


		typedef SmartPtr<AddEntityBodyCmd> AddEntityBodyCmdPtr;
		


	}
}


#endif // _ADD_ENTITY_BODY_CMD_H