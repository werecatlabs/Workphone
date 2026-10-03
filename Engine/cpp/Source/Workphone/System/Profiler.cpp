#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/Profiler.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/System/Timer.hpp>
#include <Workphone/System/Profile.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IProfile.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    Profiler::Profiler() = default;

    Profiler::~Profiler() = default;

    void Profiler::update()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto timer = applicationManager->getTimer();
            auto task = Thread::getCurrentTask();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Profiler::unload( SmartPtr<ISharedObject> data )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        m_profiles.clear();
        IProfiler::unload( data );
    }

    auto Profiler::addProfile() -> SmartPtr<IProfile>
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        auto profile = workphone::make_ptr<Profile>();
        m_profiles.emplace_back( profile );

        return profile;
    }

    void Profiler::removeProfile( SmartPtr<IProfile> profile )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        auto it = std::find( m_profiles.begin(), m_profiles.end(), profile );
        if( it != m_profiles.end() )
        {
            m_profiles.erase( it );
        }
    }

    auto Profiler::getProfile( hash64 id ) const -> SmartPtr<IProfile>
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        for( auto &p : m_profiles )
        {
            if( auto handle = p->getHandle() )
            {
                if( handle->getId() == id )
                {
                    return p;
                }
            }
        }

        return nullptr;
    }

    auto Profiler::getProfiles() const -> Array<SmartPtr<IProfile>>
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return m_profiles;
    }

    void Profiler::logResults()
    {
    }
}  // namespace workphone
