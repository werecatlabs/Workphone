#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/Profile.hpp>
#include <Workphone/Core/Util.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <chrono>

namespace workphone
{
    const s32 Profile::DEFAULT_NUM_SAMPLES = 100;
    u32 Profile::m_idExt = 0;

    Profile::Profile()
    {
        auto id = m_idExt++;

        if( auto handle = getHandle() )
        {
            handle->setId( id );
        }

        m_deltaTime = 0.0;

        m_averageTimeTaken.reserve( DEFAULT_NUM_SAMPLES );
        m_averageDeltaTimes.reserve( DEFAULT_NUM_SAMPLES );
    }

    Profile::~Profile() = default;

    void Profile::start()
    {
        ScopedLock lock( this );

        m_start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    void Profile::end()
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto timer = applicationManager->getTimer();

        m_end = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        auto interval = m_end - m_start;
        m_timeTaken =
            static_cast<float>( static_cast<f64>( interval ) / static_cast<f64>( 1000000000LL ) );

        m_averageTimeTaken.push_back( m_timeTaken );

        if( m_averageTimeTaken.size() > DEFAULT_NUM_SAMPLES )
        {
            m_averageTimeTaken.erase(
                m_averageTimeTaken.begin(),
                m_averageTimeTaken.begin() + ( m_averageTimeTaken.size() - DEFAULT_NUM_SAMPLES ) );
        }

        auto deltaTime = timer->getDeltaTime();
        m_averageDeltaTimes.push_back( deltaTime );

        if( m_averageDeltaTimes.size() > DEFAULT_NUM_SAMPLES )
        {
            m_averageDeltaTimes.erase(
                m_averageDeltaTimes.begin(),
                m_averageDeltaTimes.begin() + ( m_averageDeltaTimes.size() - DEFAULT_NUM_SAMPLES ) );
        }
    }

    auto Profile::getAverageTimeTaken() const -> f64
    {
        ScopedLock lock( this );

        if( m_averageTimeTaken.size() > 0.0f )
        {
            return Util::average( m_averageTimeTaken );
        }

        return 0.0f;
    }

    f64 Profile::getAverageDeltaTime() const
    {
        ScopedLock lock( this );

        if( m_averageDeltaTimes.size() > 0.0 )
        {
            return Util::average( m_averageDeltaTimes );
        }

        return 0.0;
    }

    void Profile::setTimeTaken( float timeTaken )
    {
        ScopedLock lock( this );

        m_timeTaken = timeTaken;

        m_averageTimeTaken.push_back( m_timeTaken );

        if( m_averageTimeTaken.size() > DEFAULT_NUM_SAMPLES )
        {
            m_averageTimeTaken.erase(
                m_averageTimeTaken.begin(),
                m_averageTimeTaken.begin() + ( m_averageTimeTaken.size() - DEFAULT_NUM_SAMPLES ) );
        }
    }

    f64 Profile::getDeltaTime() const
    {
        return m_deltaTime;
    }

    void Profile::setDeltaTime( f64 deltaTime )
    {
        m_deltaTime = deltaTime;
    }

    void Profile::clear()
    {
        ScopedLock lock( this );

        m_timeTaken = 0.0f;
        m_averageTimeTaken.clear();
    }

    void Profile::setLabel( const String &label )
    {
        m_label = label;
    }

    auto Profile::getLabel() const -> String
    {
        return m_label;
    }

    void Profile::setUserData( void *userData )
    {
        m_userData = userData;
    }

    auto Profile::getUserData() const -> void *
    {
        return m_userData;
    }

    auto Profile::getTimeTaken() const -> float
    {
        return m_timeTaken;
    }

    auto Profile::getDescription() const -> String
    {
        return m_description;
    }

    void Profile::setDescription( const String &description )
    {
        m_description = description;
    }

    auto Profile::getTotal() const -> f64
    {
        return m_total;
    }

    void Profile::setTotal( f64 total )
    {
        m_total = total;
    }

    void Profile::lock()
    {
        m_mutex.lock();
    }

    bool Profile::try_lock()
    {
        return m_mutex.try_lock();
    }

    void Profile::unlock()
    {
        m_mutex.unlock();
    }

}  // namespace workphone
