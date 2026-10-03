#ifndef NullScriptObject_h__
#define NullScriptObject_h__



#include <Workphone/Base/singleton.hpp>
#include <Workphone/Interface/Objects\IStandardObject.hpp>



namespace fb
{

	class NullScriptObject : public CSharedObject<IStandardObject>, public fb::Singleton<NullScriptObject>
	{
	public:
		NullScriptObject();
		~NullScriptObject(){}

		virtual void update( const s32& task, const time_interval& t, const time_interval& dt);
		s32 getFSM(u32 hash, FSMPtr& fsm);

		// 
		// IScriptObject functions 
		//

		/** Gets an object call script functions. */
		virtual ScriptInvokerPtr& getInvoker() { return m_scriptInvoker; }

		/** Gets an object call script functions. */
		virtual const ScriptInvokerPtr& getInvoker() const { return m_scriptInvoker; }

		/** Sets an object call script functions. */
		virtual void setInvoker(ScriptInvokerPtr invoker){ m_scriptInvoker = invoker; }

		/** Gets an object to receive script calls. */
		virtual ScriptReceiverPtr& getReceiver() { return m_scriptReceiver; } 

		/** Gets an object to receive script calls. */
		virtual const ScriptReceiverPtr& getReceiver() const { return m_scriptReceiver; } 

		/** Sets an object to receive script calls. */
		virtual void setReceiver(ScriptReceiverPtr receiver){ m_scriptReceiver = receiver; }

		virtual s32 setProperty(hash32 hash, const String& value);	
		virtual s32 getProperty(hash32 hash, String& value) const;	
		virtual s32 setProperty(hash32 hash, const Parameter& param);
		virtual s32 setProperty(hash32 hash, const Parameters& params);
		virtual s32 setProperty(hash32 hash, void* param);	
		virtual s32 getProperty(hash32 hash, Parameter& param) const;
		virtual s32 getProperty(hash32 hash, Parameters& params) const;
		virtual s32 getProperty(hash32 hash, void* param) const;

		virtual s32 callFunction(u32 hash, const Parameters& params, Parameters& results){ return 0; }
		virtual s32 callFunction(u32 hash, ObjectPtr object, Parameters& results){ return 0; }

		virtual void setObject(hash32 hash, ObjectPtr object) { }
		virtual void getObject(hash32 hash, ObjectPtr& object) const { }

	protected:
		/// Used to call script functions.
		ScriptInvokerPtr m_scriptInvoker;

		/// Used to receive script calls.
		ScriptReceiverPtr m_scriptReceiver;
	};
}



#endif // NullScriptObject_h__