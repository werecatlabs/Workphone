#ifndef LuaManager_h__
#define LuaManager_h__



#include "WPLua/WPLuaPrerequisites.hpp"
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Base/String.hpp>
#include <Workphone/Base/Array.hpp>
#include <luabind/object.hpp>



#define FB_PROFILE_LUA_CALLS 0



//forward decs
struct lua_State;




namespace fb
{



	//forward decs
	class NullScriptObject;



	//---------------------------------------------------------------------------------------------------
	class  LuaManager : public CSharedObject<IScriptManager>
	{
	public:
		LuaManager();
		LuaManager(u32 taskId);
		~LuaManager();

		void update(const s32& task, const time_interval& t, const time_interval& dt);

		void createLuaState();

		void loadScript(const String& filename);

		void loadScripts(const Array<String>& scripts);

		void callFunction(const String& functionName);
		void callFunction(const String& functionName, const Parameters& parameters);
		void callFunction(const String& functionName, const Parameters& parameters, Parameters& results);

		s32 callMember(const String& className, const String& functionName);
		s32 callMember(const String& className, const String& functionName, const Parameters& parameters);
		s32 callMember(const String& className, const String& functionName, const Parameters& parameters, Parameters& results);

		void callObjectMember(IStandardObject* object, const String& functionName);
		void callObjectMember(IStandardObject* object, const String& functionName, const Parameters& parameters);
		void callObjectMember(IStandardObject* object, const String& functionName, const Parameters& parameters, Parameters& results);

		void createObject(const String& className, ObjectPtr object);
		void destroyObject(ObjectPtr object);

		void* createInstance(const String& className);

		void destroyInstance(void* instance);

		void setTaskId(u32 taskId){ m_taskId = taskId; }
		u32 getTaskId() const { return m_taskId; }

		void reloadScripts();

		void _reloadScripts();

		void clearStack();
		void dumpStack();	
		String getDebugInfo();

		void addFunctions(const ScriptBindPtr& scriptFunctions);
		bool getFunctions(ScriptBindPtr& scriptFunctions);
		bool removeFunctions(const ScriptBindPtr& scriptFunctions);
		void removeAllFunctions();

		void executeScript(const String& script);

		bool getEnableFullDebug() const { return m_enableFullDebug; }
		void setEnableFullDebug(bool enableFullDebug) { m_enableFullDebug = enableFullDebug; }

		lua_State* getLuaState() const { return m_luaState; }
		void setLuaState(lua_State* luaState) { m_luaState = luaState; }

		bool getDelayedCreation() const { return m_delayedCreation; }
		void setDelayedCreation(bool delayedCreation) { m_delayedCreation = delayedCreation; }

		
		bool isDebugEnabled() const { return m_isDebugEnabled; }	
		void setDebugEnabled(bool enable) { m_isDebugEnabled = enable; }

		bool getError() const { return m_bError; }
		void setError(bool error) { m_bError = error; }

		void removeBreakpoint(ScriptBreakpointPtr breakpoint);		
		void addBreakpoint(ScriptBreakpointPtr breakpoint);		
		Array<ScriptBreakpointPtr> getBreakpoints() const;

		void addScriptBinding(ScriptBindPtr scriptBinding);
		void removeScriptBinding(ScriptBindPtr scriptBinding);

		void _getObject(void** object);

		static void handleLuaError(lua_State* luaState);

	private:
		bool createLuaInstance( LuaObjectDataPtr objectData );

		s32 _callObjectMember(IStandardObject* object, const String &functionName );
		s32 _callObjectMember(IStandardObject* object, const String &functionName, const Parameters& parameters, Parameters& results );
		s32 _callObjectMember(IStandardObject* object, const String &functionName, const Parameters& parameters );

		void errorObjectNotFound();

		void _executeFunction( const String &functionNameStr, const Parameters &parameters, Parameters &results );

		/// The lua state 
		WeakPtr<lua_State> m_luaState;

		/// 
		WeakPtr<NullScriptObject> m_nullScriptObject;

		TimerPtr m_timer;

		/// 
		atomic_bool m_bReload;
		




		/// 
		u32 m_taskId;

		/// The scripts that were loaded
		Array<String> m_scripts;

		/// 
		Array<LuaObjectDataPtr> m_objectData;

		/// 
		Array<LuaObjectDataPtr> m_creationList;

		/// The script functions
		Array<ScriptBindPtr> m_functions;

		/// Instances
		Array<WeakPtr<luabind::object>> m_instances;

		/// Bindings
		Array<ScriptBindPtr> m_scriptBindings;
		
		FB_MUTEX(ScriptMutex);

		// 
		// Values for debugging 
		//

		/// 
		String m_curClass;

		/// 
		String m_curFunction;

		String m_profileClass;

		/// 
		String m_profileFunction;

		time_interval m_timeTaken;
		int m_callCounter;

		/// 
		bool m_enableFullDebug;

		bool m_delayedCreation;

		bool m_isDebugEnabled;

		bool m_bError;
	};



} // end namespace fb



#endif // LuaManager_h__