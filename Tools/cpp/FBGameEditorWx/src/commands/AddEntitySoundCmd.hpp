#ifndef _ADD_ENTITY_SOUND_CMD_H
#define _ADD_ENTITY_SOUND_CMD_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
		
		
		
		//--------------------------------------------
		class AddEntitySoundCmd : public CSharedObject<ICommand>
		{
		public:
			AddEntitySoundCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt);
			~AddEntitySoundCmd();
		
			virtual void undo();
			virtual void redo();
			virtual void execute();
		
			String getCommandId() const;
		
		private:
			Properties m_propertyGroup;
			SmartPtr<EntityTemplate> m_parentEnt;
			String m_entityName;
		};
		
		typedef SmartPtr<AddEntitySoundCmd> AddEntitySoundCmdPtr;
		


	}
}


#endif // _ADD_ENTITY_NODE_CMD_H