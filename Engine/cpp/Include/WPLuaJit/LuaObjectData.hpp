#ifndef LuaObjectData_h__
#define LuaObjectData_h__



#include <Workphone/Interface/Script/IScriptData.hpp>
#include <Workphone/Memory/CSharedObject.hpp>
#include <Workphone/Base/String.hpp>
#include <luabind/luabind.hpp>



namespace fb
{
	


	//---------------------------------------------------------------------------------------------------
	class LuaObjectData : public CSharedObject<IScriptData>
	{
	public:
		LuaObjectData();
		~LuaObjectData();
	
		virtual void setOwner( IStandardObject* owner );
		virtual IStandardObject* getOwner() const;

		void* getObjectData() const;
	
		String getClassName() const { return m_className; }
		void setClassName(const String& className) { m_className = className; }
	
		inline luabind::object& getObject() { return m_object; }
			
	protected:
		IStandardObject* m_owner;
		String m_className;

		/// Object if using a single lua state.
		luabind::object m_object;
	};
	
	
	
	typedef SmartPtr<LuaObjectData> LuaObjectDataPtr;
	
	

} // end namespace fb



#endif // LuaObjectData_h__