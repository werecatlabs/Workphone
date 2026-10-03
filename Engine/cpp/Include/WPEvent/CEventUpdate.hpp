#ifndef CEventUpdate_h__
#define CEventUpdate_h__

#include <Workphone/Interface/System/IEvent.hpp>
#include "WPEvent/CEvent.hpp"

namespace workphone
{

    class CEventUpdate : public IEvent
    {
        WP_CLASS_REGISTER_DECL;

    public:
        CEventUpdate()
        {
        }

        ~CEventUpdate() override
        {
        }

        void setEventType( hash32 type )
        {
            m_eventType = type;
        }

        hash32 getEventType() const
        {
            return m_eventType;
        }

        s32 getTask() const
        {
            return m_task;
        }
        void setTask( s32 task )
        {
            m_task = task;
        }

        time_interval getT() const
        {
            return m_t;
        }
        void setT( time_interval t )
        {
            m_t = t;
        }

        time_interval getDt() const
        {
            return m_dt;
        }
        void setDt( time_interval dt )
        {
            m_dt = dt;
        }

    protected:
        time_interval m_t;
        time_interval m_dt;
        s32 m_task;
        hash32 m_eventType;
    };
}  // namespace workphone

#endif  // CEventUpdate_h__
