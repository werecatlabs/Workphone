#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/AsyncOperation.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AsyncOperation, IAsyncOperation );

    AsyncOperation::AsyncOperation() = default;
    AsyncOperation::~AsyncOperation() = default;

    void AsyncOperation::removeCompleteEvent( std::function<void()> func )
    {
        auto target = func.target<void ( * )()>();
        m_completeEvents.erase( std::remove_if( m_completeEvents.begin(), m_completeEvents.end(),
                                                [&]( const std::function<void()> &f ) {
                                                    if( f.target_type() != func.target_type() )
                                                    {
                                                        return false;
                                                    }
                                                    auto fTarget = f.target<void ( * )()>();
                                                    return fTarget && target && *fTarget == *target;
                                                } ),
                                m_completeEvents.end() );
    }

    void AsyncOperation::addCompleteEvent( std::function<void()> func )
    {
        m_completeEvents.push_back( func );
    }
}  // namespace workphone
