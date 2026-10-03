#include "WPLuaJit/NullScriptObject.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/WorkphoneInterface.hpp>
#include <FBGame/FBGame.hpp>



namespace fb
{



	template<> NullScriptObject* fb::Singleton<NullScriptObject>::m_singleton = NULL;



	//---------------------------------------------------------------------------------------------------
	NullScriptObject::NullScriptObject()
	{
		setReceiver(ScriptReceiverPtr(new ScriptReceiverAdapter<NullScriptObject>(this), true));
	}



	//---------------------------------------------------------------------------------------------------
	void NullScriptObject::update(  const s32& task, const time_interval& t, const time_interval& dt )
	{
		LOG_MESSAGE("LuaScriptMgr", "Null object.");
	}



	//---------------------------------------------------------------------------------------------------
	s32 NullScriptObject::getFSM( u32 hash, FSMPtr& fsm )
	{
		LOG_MESSAGE("LuaScriptMgr", "Null object.");
		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	s32 NullScriptObject::setProperty( hash32 hash, const String& value )
	{
		LOG_MESSAGE("LuaScriptMgr", "Null object.");
		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	s32 NullScriptObject::getProperty( hash32 hash, String& value ) const
	{
		LOG_MESSAGE("LuaScriptMgr", "Null object.");
		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	s32 NullScriptObject::setProperty( hash32 hash, const Parameter& param )
	{
		LOG_MESSAGE("LuaScriptMgr", "Null object.");
		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	s32 NullScriptObject::setProperty( hash32 hash, const Parameters& params )
	{
		LOG_MESSAGE("LuaScriptMgr", "Null object.");
		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	s32 NullScriptObject::setProperty( hash32 hash, void* param )
	{
		LOG_MESSAGE("LuaScriptMgr", "Null object.");
		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	s32 NullScriptObject::getProperty( hash32 hash, Parameter& param ) const
	{
		LOG_MESSAGE("LuaScriptMgr", "Null object.");
		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	s32 NullScriptObject::getProperty( hash32 hash, Parameters& params ) const
	{
		LOG_MESSAGE("LuaScriptMgr", "Null object.");
		return 0;
	}



	//---------------------------------------------------------------------------------------------------
	s32 NullScriptObject::getProperty( hash32 hash, void* param ) const
	{
		LOG_MESSAGE("LuaScriptMgr", "Null object.");
		return 0;
	}



} // end namespace fb