#ifndef WPLua_h__
#define WPLua_h__


#include "Workphone/WorkphoneAutolink.hpp"



#ifdef _DEBUG
	#pragma comment(lib, "WPLuaJit_d.lib")
	#pragma comment(lib, "WPLuabind_d.lib")
	#pragma comment(lib, "lua_d.lib")
	#pragma comment(lib, "luabind_d.lib")
#elif NDEBUG
	#pragma comment(lib, "WPLuaJit.lib")
	#pragma comment(lib, "WPLuabind.lib")
	#pragma comment(lib, "luajit.lib")
	#pragma comment(lib, "luabind_luajit.lib")
#else
	#pragma comment(lib, "WPLuaJit.lib")
	#pragma comment(lib, "WPLuabind.lib")
	#pragma comment(lib, "luajit.lib")
	#pragma comment(lib, "luabind_luajit.lib")
#endif



#include "WPLua/LuaLibrary.hpp"



#endif // WPLua_h__


