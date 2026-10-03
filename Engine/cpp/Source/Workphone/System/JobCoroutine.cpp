#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/JobCoroutine.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, JobCoroutine, Job );

    JobCoroutine::JobCoroutine() :
        m_coroutine( [this]( ICoroutineData::PullType &pull ) { coroutine_step( pull ); } )
    {
    }

    JobCoroutine::~JobCoroutine() = default;

    void JobCoroutine::coroutine_execute()
    {
        if( isInterrupted() )
        {
            setState( State::Finish );
            return;
        }

        auto func = getFunction();
        if( !func )
        {
            setState( State::Finish );
            return;
        }

        auto coroutineFunc = [&func]( ICoroutineData::PullType &pull ) { func( pull ); };

        ICoroutineData::PushType coroutine( coroutineFunc );

        // Execute the coroutine until it's done
        auto count = 0;
        while( coroutine && !isInterrupted() )
        {
            coroutine();
            count++;
        }

        setState( State::Finish );
    }

    void JobCoroutine::coroutine_step( ICoroutineData::PullType &pull )
    {
        if( m_function )
        {
            m_function( pull );
        }

        setState( State::Finish );
    }

    void JobCoroutine::coroutine_execute_step( SmartPtr<ICoroutineData> &yield )
    {
        auto state = getState();
        if( state != State::Finish )
        {
            if( !m_function || isInterrupted() )
            {
                setState( State::Finish );
            }
            else if( m_coroutine )
            {
                m_coroutine();
                m_coroutineCount++;
            }
            else
            {
                setState( State::Finish );
            }
        }
    }

    auto JobCoroutine::getFunction() const -> std::function<void( ICoroutineData::PullType & )>
    {
        return m_function;
    }

    void JobCoroutine::setFunction( std::function<void( ICoroutineData::PullType & )> &function )
    {
        m_function = function;
    }

}  // namespace workphone
