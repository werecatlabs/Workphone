#ifndef __IMessageListener_h__
#define __IMessageListener_h__



#include <GameEditorPrerequisites.hpp>



namespace fb
{
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class IMessageListener : public ISharedObject
		{
		public:
			virtual ~IMessageListener() = default;
	
			/** Handles a message. */
			virtual void handleMessage(SmartPtr<IMessage> message){}
		};
	
	
	
	} // end namespace editor
} // end namespace fb




#endif // IMessageListener_h__


