#ifndef CEvent_h__
#define CEvent_h__


#include <Workphone/Interface/System/IEvent.hpp>


namespace workphone
{
    class CEvent : public IEvent
    {
        WP_CLASS_REGISTER_DECL;

    public:
        CEvent();
        ~CEvent() override;

        void setEventType( hash32 type ) 
        {
            m_eventType = type;
        }

        hash32 getEventType() const 
        {
            return m_eventType;
        }

    protected:
        hash32 m_eventType;
    };
}


#endif // CEvent_h__
