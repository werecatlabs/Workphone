#ifndef WPLuaPrerequisites_h__
#define WPLuaPrerequisites_h__



#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Object/SmartPtr.hpp>



namespace fb
{


	/// class forward decs
	class LuaManager;
	class LuaObjectData;

	/// smart pointer forward decs
	typedef SmartPtr<LuaManager> LuaManagerPtr;
	typedef SmartPtr<LuaObjectData> LuaObjectDataPtr;


	
} // end namespace fb



#endif // WPLuaPrerequisites_h__