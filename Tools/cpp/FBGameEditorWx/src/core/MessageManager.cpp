#include <GameEditorPCH.hpp>
#include <core/MessageManager.hpp>
#include <FBCore/Interface/System/IMessage.hpp>
#include <core/IMessageListener.hpp>




namespace fb
{
	namespace editor
	{
	
	
		
		MessageManager::MessageManager()
		{
			
		}



		MessageManager::~MessageManager()
		{
			m_messages.clear();
			m_listeners.clear();
		}



		void MessageManager::addListener(SmartPtr<IMessageListener> listener)
		{
			m_listeners.push_back(listener);
		}
	
	
	
		bool MessageManager::removeListener( SmartPtr<IMessageListener> listener )
		{
			auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
			if (it != m_listeners.end())
			{
				m_listeners.erase(it);
				return true;
			}

			return false;
		}
	
	
	
		void MessageManager::postMessage( SmartPtr<IMessage> message )
		{
			for (auto listener : m_listeners)
			{
				listener->handleMessage(message);
			}
		}
	
	
	
	} // end namespace editor
} // end namespace fb


