#include "WPLuaJit/LuaManager.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/WorkphoneInterface.hpp>
#include <FBGame/FBGame.hpp>
#include "WPLuabind/WPLuabind.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include "WPLua/NullScriptObject.hpp"
#include "WPLua/LuaObjectData.hpp"
#include <sstream>
#include <tinyxml/tinyxml.hpp>
#include <luajit.hpp>
#include <stdarg.hpp>
#include <iostream>
#include <luabind/luabind.hpp>
#include <luabind/adopt_policy.hpp>



namespace fb
{



	//---------------------------------------------------------------------------------------------------
	void LuaManager::handleLuaError(lua_State* luaState)
	{
		String errorStr = String(lua_tostring(luaState, -1));
		String message = String("Error: ") + errorStr + String("\n");
		String	debugStr;

		lua_pop(luaState, 1);

		Engine* engine = Engine::getSingletonPtr();
		LuaManagerPtr scriptMgr = engine->getScriptManager();			
		if(scriptMgr)
		{
			debugStr = scriptMgr->getDebugInfo();
			LOG_MESSAGE("LuaScriptMgr", message + debugStr);
		}

		TiXmlDocument doc;
		TiXmlElement* rootElem = new TiXmlElement("error_desc");
		doc.LinkEndChild(rootElem);
		if(rootElem)
		{
			TiXmlElement* sourceElem = new TiXmlElement("desc");
			rootElem->LinkEndChild(sourceElem);

			TiXmlText* sourceTxtElem = new TiXmlText(errorStr.c_str());
			sourceElem->LinkEndChild(sourceTxtElem);
		}

		TiXmlPrinter printer;
		printer.SetIndent( "	" );

		doc.Accept( &printer );

		String outputStr = printer.CStr();
		engine->getOutputManager()->output(outputStr);

		scriptMgr->setError(true);

		FB_SCRIPT_EXCEPTION(message + debugStr);
	}



	//---------------------------------------------------------------------------------------------------
	int handleLuaPCallError(lua_State* luaState)
	{
		String debugStr;

		lua_Debug d;

		int level = LUA_MINSTACK;
		int success = lua_getstack(luaState, level, &d);

		while(level >= 0)
		{
			success = lua_getstack(luaState, level, &d);
			if(success != 0)
			{
				lua_getinfo(luaState, "Sln", &d);

				std::stringstream msg;
				msg << d.short_src << ":" << d.currentline;

				if (d.name != 0)
				{
					msg << "(" << d.namewhat << " " << d.name << ")";
				}

				debugStr += String(msg.str().c_str()) + String("\n");

				TiXmlDocument doc;
				TiXmlElement* rootElem = new TiXmlElement("error");
				doc.LinkEndChild(rootElem);
				if(rootElem)
				{
					TiXmlElement* lineElem = new TiXmlElement("line");
					rootElem->LinkEndChild(lineElem);

					TiXmlText* lineTxtElem = new TiXmlText(StringUtil::toString(d.currentline).c_str());
					lineElem->LinkEndChild(lineTxtElem);

					TiXmlElement* sourceElem = new TiXmlElement("source");
					rootElem->LinkEndChild(sourceElem);

					TiXmlText* sourceTxtElem = new TiXmlText(d.short_src);
					sourceElem->LinkEndChild(sourceTxtElem);
				}

				TiXmlPrinter printer;
				printer.SetIndent( "	" );

				doc.Accept( &printer );

				Engine* engine = Engine::getSingletonPtr();
				String outputStr = printer.CStr();
				engine->getOutputManager()->output(outputStr);
			}

			--level;
		}

		LuaManagerPtr scriptMgr = Engine::getSingletonPtr()->getScriptManager();
		scriptMgr->setError(true);

		LOG_MESSAGE("LuaScriptMgr", debugStr.c_str());

		return 1;
	}


	void handleCastFailed(lua_State* luaState, luabind::type_id const& id)
	{
		String debugStr;

		lua_Debug d;

		int level = LUA_MINSTACK;
		int success = lua_getstack(luaState, level, &d);

		while(level >= 0)
		{
			success = lua_getstack(luaState, level, &d);
			if(success != 0)
			{
				lua_getinfo(luaState, "Sln", &d);

				std::stringstream msg;
				msg << d.short_src << ":" << d.currentline;

				if (d.name != 0)
				{
					msg << "(" << d.namewhat << " " << d.name << ")";
				}

				debugStr += String(msg.str().c_str()) + String("\n");
			}

			--level;
		}

		LuaManagerPtr scriptMgr = Engine::getSingletonPtr()->getScriptManager();
		scriptMgr->setError(true);

		LOG_MESSAGE("LuaScriptMgr", debugStr.c_str());
	}



	//---------------------------------------------------------------------------------------------------
	LuaManager::LuaManager()
		: m_enableFullDebug(true), m_delayedCreation(false), m_bError(false)
	{
		m_nullScriptObject = new NullScriptObject;
		createLuaState();
		m_bReload = FALSE;
	}



	//---------------------------------------------------------------------------------------------------
	LuaManager::LuaManager(u32 taskId)
		: m_enableFullDebug(true), m_bError(false)
	{
		setTaskId(taskId);
		m_nullScriptObject = new NullScriptObject;
		createLuaState();
	}



	//---------------------------------------------------------------------------------------------------
	LuaManager::~LuaManager()
	{
		for(u32 i=0; i<m_objectData.size(); ++i)
		{
			LuaObjectDataPtr objectData = m_objectData[i];
			luabind::object& luaObject = objectData->getObject();
			luaObject = luabind::object();
		}

		m_objectData.clear();
		m_creationList.clear();

		for(u32 i=0; i<m_instances.size(); ++i)
		{
			luabind::object* pObject = m_instances[i];

			//StandardGameObject* ptr = luabind::object_cast<StandardGameObject*>((*pObject)(), luabind::adopt(boost::arg<0>()));
			//IScriptObject* ptr = luabind::object_cast<IScriptObject*>((*pObject));
			//ptr->~IScriptObject(); // let lua free the memory

			delete pObject;
		}

		lua_gc (m_luaState, LUA_GCCOLLECT, 0);

		// closes the lua state
		lua_close(m_luaState);

		FB_SAFE_DELETE(m_nullScriptObject);
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::loadScript(const String& filename)
	{
		Engine* engine = Engine::getSingletonPtr();
		FileSystemPtr& fileSystem = engine->getFileSystem();
		if(!fileSystem)
		{
			LOG_MESSAGE("Script", "No file system. "); 
			return;
		}

#ifdef _FINAL_

		StreamPtr data = fileSystem->open(filename);
		if(!data)
		{
			return;
		}

		String script = data->getString();
		int error = luaL_dostring(m_luaState, script.c_str());
		if (error != 0) 
		{
			String message = String("ScriptMgr::loadScript - error - couldn't open ") + filename + 
				String(" errorcode = ") + String(lua_tostring(m_luaState, -1));
			LOG_MESSAGE("Script", message); 
		}
		else
		{
			String message = String("Loaded: ") + filename;
			LOG_MESSAGE("Script", message); 
		}

#else

		String absolutePath = fileSystem->getFileDir(filename);
		int error = luaL_dofile(m_luaState, absolutePath.c_str());
		if (error != 0) 
		{
			StreamPtr data = fileSystem->open(filename);
			if(!data)
			{
				return;
			}

			String script = data->getString();
			int error = luaL_dostring(m_luaState, script.c_str());
			if (error) 
			{
				String message = String("ScriptMgr::loadScript - error - couldn't open ") + filename + 
					String(" errorcode = ") + String(lua_tostring(m_luaState, -1));
				LOG_MESSAGE("Script", message); 
			}
			else
			{
				String message = String("Loaded: ") + filename;
				LOG_MESSAGE("Script", message); 
			}
		}
		else
		{
			String message = String("Loaded: ") + filename;
			LOG_MESSAGE("Script", message); 
		}

#endif

		// add filename
		m_scripts.push_back(filename);
	}


	void LuaManager::loadScripts(const Array<String>& scripts)
	{

	}

	//---------------------------------------------------------------------------------------------------
	void LuaManager::addFunctions(const ScriptBindPtr& scriptFunctions)
	{
	}



	//---------------------------------------------------------------------------------------------------
	bool LuaManager::getFunctions(ScriptBindPtr& scriptFunctions)
	{
		_FB_DEBUG_BREAK_IF(false); //not implemented
		return false;
	}


	//---------------------------------------------------------------------------------------------------
	bool LuaManager::removeFunctions(const ScriptBindPtr& scriptFunctions)
	{
		_FB_DEBUG_BREAK_IF(false); //not implemented
		return false;
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::removeAllFunctions()
	{	
		_FB_DEBUG_BREAK_IF(false); //not implemented	
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::executeScript(const String& script)
	{
		int error = luaL_loadbuffer(m_luaState, script.c_str(), script.length(), "script") || lua_pcall(m_luaState, 0, 0, 0);

		if (error)
		{
			String message = String("Error: ") + String(lua_tostring(m_luaState, -1));
			LOG_MESSAGE("LuaScriptMgr", message); 
			lua_pop(m_luaState, 1);
		}
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::callFunction(const String& functionName)
	{
		FB_LOCK_MUTEX(ScriptMutex);

		Parameters parameters;
		Parameters results;
		callFunction(functionName, parameters, results);
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::callFunction(const String& functionName, const Parameters& parameters)
	{
		FB_LOCK_MUTEX(ScriptMutex);

		Parameters results;
		callFunction(functionName, parameters, results);
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::callFunction(const String& functionNameStr, const Parameters& parameters, Parameters& results)
	{
		try
		{
			FB_LOCK_MUTEX(ScriptMutex);
			_executeFunction(functionNameStr, parameters, results);
		}
		catch (std::exception& e)
		{
			String msg = String("lua exception: ") + String(e.what());
			LOG_MESSAGE("LuaScriptMgr", msg.c_str());
		}
	}



	//---------------------------------------------------------------------------------------------------
	s32 LuaManager::callMember( const String& className, const String& functionName )
	{
		try
		{
			FB_LOCK_MUTEX(ScriptMutex);

			int returnValue = 0; //default value

			luabind::object object = luabind::globals(m_luaState)[className.c_str()];
			if(object)
			{
				if(m_enableFullDebug)
				{
					m_curClass = className;
					m_curFunction = functionName;
				}

				luabind::call_member<void>(object, functionName.c_str());
			}
			else
			{
				String msg = String("Object not found: ") + className;
				LOG_MESSAGE("LuaScriptMgr", msg.c_str());
			}

			return returnValue;
		}
		catch (std::exception& e)
		{
			String msg = String("Error calling function: ")  + className + String(":")  
				+ functionName + String(" lua exception: ") + String(e.what());
			LOG_MESSAGE("LuaScriptMgr", msg.c_str());
		}

		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	s32 LuaManager::callMember( const String& className, const String& functionName, const Parameters& parameters )
	{
		try
		{
			FB_LOCK_MUTEX(ScriptMutex);

			int returnValue = 0; //default value

			luabind::object object = luabind::globals(m_luaState)[className.c_str()];			
			if(object)
			{
				if(m_enableFullDebug)
				{
					m_curClass = className;
					m_curFunction = functionName;
				}

				luabind::call_member<void>(object, functionName.c_str(), parameters);
			}
			else
			{
				String msg = String("Object not found: ") + className;
				LOG_MESSAGE("LuaScriptMgr", msg.c_str());
			}

			return returnValue;
		}
		catch (std::exception& e)
		{
			String msg = String("Error calling function: ")  + className + String(":")  
				+ functionName + String(" lua exception: ") + String(e.what());
			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			throw;
		}

		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	s32 LuaManager::callMember( const String& className, const String& functionName, const Parameters& parameters, Parameters& results )
	{
		try
		{
			FB_LOCK_MUTEX(ScriptMutex);

			int returnValue = 0; //default value

			luabind::object object = luabind::globals(m_luaState)[className.c_str()];
			if(object)
			{
				if(m_enableFullDebug)
				{
					m_curClass = className;
					m_curFunction = functionName;
				}

				luabind::call_member<void>(object, functionName.c_str(), parameters, boost::ref(results));
			}
			else
			{
				String msg = String("Object not found: ") + className;
				LOG_MESSAGE("LuaScriptMgr", msg.c_str());
			}

			return returnValue;
		}
		catch (std::exception& e)
		{
			String msg = String("Error calling function: ")  + className + String(":")  
				+ functionName + String(" lua exception: ") + String(e.what());
			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			throw;
		}

		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::callObjectMember(IStandardObject* object, const String& functionName)
	{
		try
		{
			_callObjectMember(object, functionName);
		}
		catch (ScriptException& e)
		{
			String msg = String("Error calling function: ")  + m_curClass + String(":")  
				+ functionName + String(" lua exception: ") + String(e.what()) + 
				String(" Lua Debug: ") + getDebugInfo();

			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			std::stringstream strStream;
			strStream <<  e.getFullDescription().c_str();

#if FB_ENABLE_DEBUG_TRACE
			strStream <<  boost::trace(e);
#endif
			LOG_MESSAGE("LuaScriptMgr", strStream.str().c_str());

			throw;
		}
		catch (std::exception& e)
		{
			String msg = String("Error calling function: ")  + m_curClass + String(":")  
				+ functionName + String(" lua exception: ") + String(e.what()) + 
				String(" Lua Debug: ") + getDebugInfo();

			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			std::stringstream strStream;

#if FB_ENABLE_DEBUG_TRACE
			strStream <<  boost::trace(e);
#endif
			LOG_MESSAGE("LuaScriptMgr", strStream.str().c_str());

			throw;
		}
		catch (...)
		{
			String msg = String("Error calling function: ")  + m_curClass + String(":")  
				+ functionName + String(" Lua Debug: ") + getDebugInfo();

			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			throw;
		}
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::callObjectMember(IStandardObject* object, const String& functionName, const Parameters& parameters)
	{
		try
		{
			_callObjectMember(object, functionName, parameters);
		}
		catch (ScriptException& e)
		{
			String msg = String("Error calling function: ")  + m_curClass + String(":")  
				+ functionName + String(" lua exception: ") + String(e.what()) + 
				String(" Lua Debug: ") + getDebugInfo();

			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			std::stringstream strStream;
			strStream <<  e.getFullDescription().c_str();

#if FB_ENABLE_DEBUG_TRACE
			strStream <<  boost::trace(e);
#endif
			LOG_MESSAGE("LuaScriptMgr", strStream.str().c_str());

			throw;
		}
		catch (std::exception& e)
		{
			String msg = String("Error calling function: ")  + m_curClass + String(":")  
				+ functionName + String(" lua exception: ") + String(e.what()) + 
				String(" Lua Debug: ") + getDebugInfo();

			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			std::stringstream strStream;

#if FB_ENABLE_DEBUG_TRACE
			strStream <<  boost::trace(e);
#endif
			LOG_MESSAGE("LuaScriptMgr", strStream.str().c_str());

			throw;
		}
		catch (...)
		{
			String msg = String("Error calling function: ")  + m_curClass + String(":")  
				+ functionName + String(" Lua Debug: ") + getDebugInfo();

			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			throw;
		}
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::callObjectMember(IStandardObject* object, const String& functionName, const Parameters& parameters, Parameters& results)
	{
		try
		{
			_callObjectMember(object, functionName, parameters, results);
		}
		catch (ScriptException& e)
		{
			String msg = String("Error calling function: ")  + m_curClass + String(":")  
				+ functionName + String(" lua exception: ") + String(e.what()) + 
				String(" Lua Debug: ") + getDebugInfo();

			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			std::stringstream strStream;
			strStream <<  e.getFullDescription().c_str();

#if FB_ENABLE_DEBUG_TRACE
			strStream <<  boost::trace(e);
#endif
			LOG_MESSAGE("LuaScriptMgr", strStream.str().c_str());

			throw;
		}
		catch (std::exception& e)
		{
			String msg = String("Error calling function: ")  + m_curClass + String(":")  
				+ functionName + String(" lua exception: ") + String(e.what()) + 
				String(" Lua Debug: ") + getDebugInfo();

			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			std::stringstream strStream;

#if FB_ENABLE_DEBUG_TRACE
			strStream <<  boost::trace(e);
#endif
			LOG_MESSAGE("LuaScriptMgr", strStream.str().c_str());

			throw;
		}
		catch (...)
		{
			String msg = String("Error calling function: ")  + m_curClass + String(":")  
				+ functionName + String(" Lua Debug: ") + getDebugInfo();

			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			throw;
		}
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::reloadScripts()
	{
		m_bReload = TRUE;
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::clearStack()
	{	
		lua_settop(m_luaState, 0);
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::dumpStack()
	{
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::_executeFunction( const String &functionNameStr, const Parameters &parameters, Parameters &results )
	{
	}



	void _getLuaInstance(LuaObjectData* data) 
	{
		Engine* engine = Engine::getSingletonPtr();
		LuaManagerPtr mgr = engine->getScriptManager();
		data->getObject().push(mgr->getLuaState());
	}


	//---------------------------------------------------------------------------------------------------
	void LuaManager::createLuaState()
	{
		// create the lua state
		m_luaState = lua_open();
		//luaL_openlibs (m_luaState);

		luabind::open(m_luaState);

		//bindings
		bindParam( m_luaState );
		bindParamList( m_luaState );

		bindMath( m_luaState );

		bindVector2( m_luaState );
		bindVector3( m_luaState );
		bindVector4( m_luaState );

		bindQuaternion( m_luaState );

		bindString( m_luaState );

		bindScriptObject( m_luaState );
		bindCore( m_luaState );
		bindAABB( m_luaState );
		bindMessage( m_luaState );


		bindAi( m_luaState );
		bindScriptProperties( m_luaState );
		bindFSM( m_luaState );

		bindEntityMgr( m_luaState );
		bindSColorf( m_luaState );
		bindSpecialFx( m_luaState );
		bindGame( m_luaState );		
		bindInput( m_luaState );
		bindFlash( m_luaState );
		bindVideo( m_luaState );
		bindSound( m_luaState );
		bindComponent( m_luaState );
		bindGraphicsSystem( m_luaState );
		bindGUIManager( m_luaState );
		bindPhysics( m_luaState );
		bindDatabase( m_luaState );
		bindCombat( m_luaState );
		bindQuery( m_luaState );

		bindSystem( m_luaState );

		bindObjectTemplates( m_luaState );
		bindEntity( m_luaState );
		workphone::bindInterfaces( m_luaState );

		{


			using namespace luabind;

			module(m_luaState)
				[ 
					class_<IScriptData, ScriptDataPtr>( "IScriptData" )
				]; 

			module(m_luaState)
				[ 
					class_<LuaObjectData, IScriptData, ScriptDataPtr>( "LuaObjectData" )
					.def("getInstance", _getLuaInstance )
				]; 


		}

		//lua_gc(m_luaState, LUA_GCSETPAUSE, 200);
		//lua_gc(m_luaState, LUA_GCSETSTEPMUL, 300);

		luabind::set_error_callback(handleLuaError);
		luabind::set_pcall_callback(handleLuaPCallError);
		luabind::set_cast_failed_callback(handleCastFailed);
	}



	//---------------------------------------------------------------------------------------------------
	String LuaManager::getDebugInfo()
	{
		String debugStr = String("Class: ") + m_curClass + 
			String(" Function: ") + m_curFunction;

		lua_Debug d;

		int top = lua_gettop(m_luaState);
		int level = top; //LUA_MINSTACK;
		int success = lua_getstack(m_luaState, top, &d);

		while(level >= 0)
		{
			success = lua_getstack(m_luaState, level, &d);
			if(success != 0)
			{
				lua_getinfo(m_luaState, "Sln", &d);

				std::stringstream msg;
				msg << d.short_src << ":" << d.currentline;

				if (d.name != 0)
				{
					msg << "(" << d.namewhat << " " << d.name << ")";
				}

				debugStr += String(msg.str().c_str()) + String("\n");
			}

			--level;
		}

		return debugStr;
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::createObject( const String& className, ObjectPtr object )
	{
		FB_LOCK_MUTEX(ScriptMutex);

		try
		{
			Parameter objectParam;
			objectParam.setPtr(object.getPtr());

			LuaObjectDataPtr objectData = object->_getData();
			if(!objectData)
			{
				objectData = ScriptDataPtr(new LuaObjectData, true);
				objectData->setClassName(className);
				objectData->setOwner(object.getPtr());
				object->_setData(objectData);

				m_objectData.push_back(objectData);

				if(m_delayedCreation)
				{
					m_creationList.push_back(objectData);
				}
				else
				{
					createLuaInstance(objectData);
				}
			}
			else
			{
				m_objectData.push_back(objectData);

				if(m_delayedCreation)
				{
					m_creationList.push_back(objectData);
				}
				else
				{
					createLuaInstance(objectData);
				}
			}
		}
		catch (std::exception& e)
		{
			String msg = String("Error : ") + String(e.what());
			LOG_MESSAGE("Script", msg.c_str());
		}
		catch (...)
		{
		}
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::destroyObject( ObjectPtr object )
	{
		LuaObjectDataPtr objectData = object->_getData();
		if(objectData)
		{
			m_objectData.erase_element(objectData);
		}

		lua_gc (m_luaState, LUA_GCCOLLECT, 0);
	}



	//---------------------------------------------------------------------------------------------------
	s32 LuaManager::_callObjectMember(IStandardObject* object, const String &functionName )
	{
		setError(false);

		int returnValue = 0; //default value

		LuaObjectDataPtr data = object->_getData();
		if(data)
		{
			if(m_enableFullDebug)
			{
				m_curClass = data->getClassName();
				m_curFunction = functionName;
			}

			luabind::object& luaObject = data->getObject();
			if(!luaObject)
			{
				if(!createLuaInstance(data))
				{
					String msg = String("Object not found: ");
					LOG_MESSAGE("LuaScriptMgr", msg.c_str());

					errorObjectNotFound();

					returnValue = 0;
				}
			}

			if(luaObject)
			{
				luabind::call_member<void>(luaObject, functionName.c_str());
			}
		}
		else
		{
			String msg = String("Object not found: ");
			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			errorObjectNotFound();

			returnValue = 0;
		}

		if(getError())
		{
			returnValue = 0;
		}

		return returnValue;
	}



	//---------------------------------------------------------------------------------------------------
	s32 LuaManager::_callObjectMember(IStandardObject* object, const String &functionName, const Parameters& parameters )
	{
		setError(false);

		int returnValue = 0; //default value

		LuaObjectDataPtr data = object->_getData();
		if(data)
		{
			if(m_enableFullDebug)
			{
				m_curClass = data->getClassName();
				m_curFunction = functionName;
			}

			luabind::object& luaObject = data->getObject();
			if(!luaObject)
			{
				if(!createLuaInstance(data))
				{
					String msg = String("Object not found: ");
					LOG_MESSAGE("LuaScriptMgr", msg.c_str());

					errorObjectNotFound();

					returnValue = 0;
				}
			}

			if(luaObject)
			{
#if FB_PROFILE_LUA_CALLS
				Engine* engine = Engine::getSingletonPtr();
				ProfilerPtr& profiler = engine->getProfiler();

				String profileClass = data->getClassName();
				String profileFunction = functionName;

				if(m_callCounter == 1 && profiler)
				{
					FB_PROFILE_START(profileClass + String(":") + profileFunction);
				}
#endif
				
				luabind::call_member<void>(luaObject, functionName.c_str(), parameters);

#if FB_PROFILE_LUA_CALLS
				if(m_callCounter == 1 && profiler)
				{
					FB_PROFILE_END(profileClass + String(":") + profileFunction);
				}
#endif
			}
		}
		else
		{
			String msg = String("Object not found: ");
			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			errorObjectNotFound();

			returnValue = 0;
		}

		if(getError())
		{
			returnValue = 0;
		}

		return returnValue;
	}



	//---------------------------------------------------------------------------------------------------
	s32 LuaManager::_callObjectMember(IStandardObject* object, const String &functionName, const Parameters& parameters, Parameters& results )
	{
		setError(false);

		int returnValue = 0; //default value

		LuaObjectDataPtr data = object->_getData();
		if(data)
		{
			if(m_enableFullDebug)
			{
				m_curClass = data->getClassName();
				m_curFunction = functionName;
			}

			luabind::object& luaObject = data->getObject();
			if(!luaObject)
			{
				if(!createLuaInstance(data))
				{
					String msg = String("Object not found: ");
					LOG_MESSAGE("LuaScriptMgr", msg.c_str());

					errorObjectNotFound();

					returnValue = 0;
				}
			}

			if(luaObject)
			{
				luabind::call_member<void>(luaObject, functionName.c_str(), parameters, boost::ref(results));
			}
		}
		else
		{
			String msg = String("Object not found: ");
			LOG_MESSAGE("LuaScriptMgr", msg.c_str());

			errorObjectNotFound();

			returnValue = 0;
		}

		if(getError())
		{
			returnValue = 0;
		}

		return returnValue;
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::update(const s32& task, const time_interval& t, const time_interval& dt)
	{
		if(m_bReload == TRUE)
		{
			_reloadScripts();
			m_bReload = FALSE;
		}

		{
			FB_LOCK_MUTEX(ScriptMutex);
			for(u32 i=0; i<m_creationList.size(); ++i)
			{
				LuaObjectDataPtr objectData = m_creationList[i];
				createLuaInstance(objectData);
			}

			m_creationList.set_used(0);
		}

		lua_gc (m_luaState, LUA_GCCOLLECT, 0);
		lua_gc (m_luaState, LUA_GCSTOP, 0);
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::_reloadScripts()
	{
		try
		{
			FB_LOCK_MUTEX(ScriptMutex);

			for(u32 i=0; i<m_objectData.size(); ++i)
			{
				LuaObjectDataPtr objectData = m_objectData[i];
				if(objectData)
				{
					objectData->getObject() = luabind::object();			 
				}
			}

			lua_close(m_luaState);
			m_luaState = NULL;

			createLuaState();

			for(u32 i=0; i<m_scriptBindings.size(); ++i)
			{
				m_scriptBindings[i]->bind(m_luaState);
			}

			for(u32 i=0; i<m_functions.size(); ++i)
			{
				ScriptBindPtr& addScriptFunctions = m_functions[i];
				//addScriptFunctions->addFunctions(ScriptManagerPtr(this));
			}

			FileSystemPtr& fileSystem = Engine::getSingletonPtr()->getFileSystem();

			if(m_scripts.empty())
			{
				fileSystem->getFileNamesWithExtension(".lua", m_scripts);
			}

			for(u32 i=0; i<m_scripts.size(); ++i)
			{
				String filename = m_scripts[i];
				if(filename.length() <= 0)
					continue;

				StreamPtr data = fileSystem->open(filename);
				if(!data)
					continue;

				String script = data->getString();
				int error = luaL_dostring(m_luaState, script.c_str());
				if (error) 
				{
					String message = String("LuaScriptMgr::loadScript - error - couldn't open ") + filename + String(" errorcode = ") + String(lua_tostring(m_luaState, -1));
					LOG_MESSAGE("Script", message.c_str()); 
				}
				else
				{
					String message = String("LuaScriptMgr loaded script: ") + filename;
					LOG_MESSAGE("Script", message.c_str()); 
				}
			}

			for(u32 i=0; i<m_objectData.size(); ++i)
			{
				LuaObjectDataPtr objectData = m_objectData[i];
				if(objectData)
				{
					String className = objectData->getClassName();
					if(className.length() > 0)
					{
						luabind::object _LuaObject = luabind::globals(m_luaState)[className.c_str()];
						if(_LuaObject)
						{
							ObjectPtr pObject = objectData->getOwner();
							objectData->getObject() = _LuaObject(pObject);	
						}
						else
						{
							String msg = String("Error : ") + getDebugInfo();
							LOG_MESSAGE("Script", msg.c_str());
						}
					}
				}
			}
		}
		catch (std::exception& e)
		{
			String msg = String("Error : ") + String(e.what());
			LOG_MESSAGE("Script", msg.c_str());
		}
		catch (...)
		{
		}
	}



	//---------------------------------------------------------------------------------------------------
	bool LuaManager::createLuaInstance( LuaObjectDataPtr objectData )
	{
		FB_LOCK_MUTEX(ScriptMutex);

		if(objectData)
		{
			String className = objectData->getClassName();
			luabind::object _LuaObject = luabind::globals(m_luaState)[className.c_str()];
			if(_LuaObject)
			{	
				luabind::object& currentLuaObject = objectData->getObject();
				if(!currentLuaObject)
				{
					currentLuaObject = _LuaObject(objectData->getOwner());
					return true;
				}
			}
		}

		return false;
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::errorObjectNotFound()
	{
		TiXmlDocument doc;
		TiXmlElement* rootElem = new TiXmlElement("error_call");
		doc.LinkEndChild(rootElem);
		if(rootElem)
		{
			TiXmlElement* sourceElem = new TiXmlElement("desc");
			rootElem->LinkEndChild(sourceElem);

			TiXmlText* sourceTxtElem = new TiXmlText("Object not found.");
			sourceElem->LinkEndChild(sourceTxtElem);

			TiXmlElement* classElem = new TiXmlElement("class");
			rootElem->LinkEndChild(classElem);

			TiXmlText* classTxtElem = new TiXmlText(m_curClass.c_str());
			classElem->LinkEndChild(classTxtElem);

			TiXmlElement* functionElem = new TiXmlElement("function");
			rootElem->LinkEndChild(functionElem);

			TiXmlText* functionTxtElem = new TiXmlText(m_curFunction.c_str());
			functionElem->LinkEndChild(functionTxtElem);
		}

		TiXmlPrinter printer;
		printer.SetIndent( "	" );

		doc.Accept( &printer );

		String outputStr = printer.CStr();

		Engine* engine = Engine::getSingletonPtr();
		engine->getOutputManager()->output(outputStr);
	}

	void LuaManager::removeBreakpoint( ScriptBreakpointPtr breakpoint )
	{

	}

	void LuaManager::addBreakpoint( ScriptBreakpointPtr breakpoint )
	{

	}

	Array<ScriptBreakpointPtr> LuaManager::getBreakpoints() const
	{
		Array<ScriptBreakpointPtr> breakpoints;
		return breakpoints;
	}

	void LuaManager::_getObject( void** object )
	{
		*object = m_luaState;
	}



	//---------------------------------------------------------------------------------------------------
	void* LuaManager::createInstance( const String& className )
	{
		luabind::object luaClass = luabind::globals(m_luaState)[className.c_str()];
		if(luaClass)
		{	
			luabind::object* instance = new luabind::object(luaClass());
			m_instances.push_back(instance);
			return instance;
		}

		return nullptr;
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::destroyInstance( void* instance )
	{
		luabind::object* object = static_cast<luabind::object*>(instance);
		m_instances.erase_element(object);
		FB_SAFE_DELETE(object);
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::addScriptBinding( ScriptBindPtr scriptBinding )
	{
		scriptBinding->bind(m_luaState);
		m_scriptBindings.push_back(scriptBinding);

		
	}



	//---------------------------------------------------------------------------------------------------
	void LuaManager::removeScriptBinding( ScriptBindPtr scriptBinding )
	{
		m_scriptBindings.erase_element(scriptBinding);
		
	}





} // end namespace fb



