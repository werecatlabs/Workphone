#include "WPLuaJit/LuaObjectData.hpp"



namespace fb
{
	
	//---------------------------------------------------------------------------------------------------
	LuaObjectData::LuaObjectData()
	{
	}


	//---------------------------------------------------------------------------------------------------
	LuaObjectData::~LuaObjectData()
	{
		m_object = luabind::object();
	}
	
	//---------------------------------------------------------------------------------------------------
	void LuaObjectData::setOwner( IStandardObject* owner )
	{
		m_owner = owner;
	}



	//---------------------------------------------------------------------------------------------------
	IStandardObject* LuaObjectData::getOwner() const
	{
		return m_owner;
	}

	void* LuaObjectData::getObjectData() const
	{
		return (void*)&m_object;
	}



	

} // end namespace fb



