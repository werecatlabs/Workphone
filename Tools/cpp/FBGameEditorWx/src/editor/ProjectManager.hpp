#ifndef ProjectManager_h__
#define ProjectManager_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class ProjectManager : public CSharedObject<ISharedObject>
		{
		public:
			static const String DEFAULT_INITIALISE_LABEL;
			static const String DEFAULT_HANDLE_MSG_LABEL;
			static const String DEFAULT_SET_FREE_LABEL;
			static const String DEFAULT_IS_FREE_LABEL;
	
			static const String DEFAULT_INITIALISE_FUNC;
			static const String DEFAULT_HANDLE_MSG_FUNC;
			static const String DEFAULT_SET_FREE_FUNC;
			static const String DEFAULT_IS_FREE_FUNC;
			static const String DEFAULT_CONSTRUCT_START_FUNC;
			static const String DEFAULT_CONSTRUCT_END_FUNC;
	
			static const String DEFAULT_INITIALISING_STATE_NAME;
			static const String DEFAULT_FREE_STATE_NAME;
			static const String DEFAULT_ACTIVE_STATE_NAME;
	
			static const String DEFAULT_FUNC_LABEL_ENTER_STATE;
			static const String DEFAULT_FUNC_LABEL_UDPATE_STATE;
			static const String DEFAULT_FUNC_LABEL_LEAVE_STATE;
			static const String DEFAULT_FUNC_LABEL_CAN_CHANGE_STATE;
	
			static const String DEFAULT_FUNC_ENTER_STATE;
			static const String DEFAULT_FUNC_UDPATE_STATE;
			static const String DEFAULT_FUNC_LEAVE_STATE;
			static const String DEFAULT_FUNC_CAN_CHANGE_STATE;
	
			static const String DEFAULT_FUNC_NAME_ENTER_STATE;
			static const String DEFAULT_FUNC_NAME_UDPATE_STATE;
			static const String DEFAULT_FUNC_NAME_LEAVE_STATE;
			static const String DEFAULT_FUNC_NAME_CAN_CHANGE_STATE;
	
			enum 
			{
				PM_OK,
	
				PM_COUNT
			};
	
			ProjectManager();
			~ProjectManager();

			/** Creates a new entity. */
			s32 createNewEntity(const String& type, bool useDefaults = false);
	
			void setEntityEventDefaults(SmartPtr<EntityTemplate> entityTemplate);
	
			void setDefaultEvents( SmartPtr<FSMTemplate> gameFSM );
	
			void setDefaultFSMStates( SmartPtr<FSMTemplate> entityFSM );
	
		protected:
		};
	
	
		
	} // end namespace editor
}




#endif // ProjectManager_h__


