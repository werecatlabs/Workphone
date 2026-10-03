#ifndef LuaLibrary_h__
#define LuaLibrary_h__



#include <Workphone/Interface/System/ILibrary.hpp>



namespace fb
{


	class LuaLibrary : public CSharedObject<ILibrary>
	{
	public:
		LuaLibrary();
		~LuaLibrary();

		void initialise();

	};

	
} // end namespace fb


#endif // LuaLibrary_h__



