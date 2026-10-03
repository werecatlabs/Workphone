#include "WPLuaJit/LuaLibrary.hpp"
#include "WPLuaJit/LuaManager.hpp"
#include <Workphone/Base/FactoryManager.hpp>



namespace fb
{


	LuaLibrary::LuaLibrary()
	{

	}

	LuaLibrary::~LuaLibrary()
	{

	}

	void LuaLibrary::initialise()
	{
		FactoryManager& factoryManager = FactoryManager::instance();
		ADD_FACTORY_NAMED(factoryManager, LuaManager, "LuaManager", StringUtil::getHash("LuaManager"));
	}

} // end namespace fb


