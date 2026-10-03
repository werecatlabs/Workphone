#ifndef ScriptManager_h__
#define ScriptManager_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class EditorScriptManager : public CSharedObject<ISharedObject>
		{
		public:
			EditorScriptManager();
			~EditorScriptManager();

			void update();
	
			void createEvent( const String& className, const String& functionName );
		};
	
	
	
	} // end namespace editor
} // end namespace fb



#endif // ScriptManager_h__


