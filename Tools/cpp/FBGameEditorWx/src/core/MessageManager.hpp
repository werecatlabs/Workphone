#ifndef MessageManager_h__
#define MessageManager_h__



#include <GameEditorPrerequisites.hpp>



namespace fb
{	
	namespace editor
	{
	

	
		class MessageManager : public CSharedObject<ISharedObject>
		{
		public:
			MessageManager();
			virtual ~MessageManager();
	
			void addListener(SmartPtr<IMessageListener> listener);
			bool removeListener(SmartPtr<IMessageListener> listener);
	
			void postMessage(SmartPtr<IMessage> message);
	
		protected:
			Array<SmartPtr<IMessage>> m_messages;
			Array<SmartPtr<IMessageListener>> m_listeners;
		};
	
	
		
	} // end namespace editor
} // end namespace fb




#endif // MessageManager_h__