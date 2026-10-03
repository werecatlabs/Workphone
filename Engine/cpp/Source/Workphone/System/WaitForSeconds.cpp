#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/WaitForSeconds.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WaitForSeconds, ISharedObject );

    WaitForSeconds::WaitForSeconds( ICoroutineData::PullType &yield, f64 seconds ) :
        m_duration( seconds )
    {
        using namespace workphone;

        auto applicationManager = core::IApplicationManager::instance();
        auto timer = applicationManager->getTimer();

        auto start = timer->now();
        auto end = start + seconds;

        while( timer->now() < end )
        {
            yield();
        }
    }

    WaitForSeconds::~WaitForSeconds() = default;

    f64 WaitForSeconds::getDuration() const
    {
        return m_duration;
    }
}  // namespace workphone
