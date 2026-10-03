#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CProfilerOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CProfileOgre.hpp>
#include <WPGraphicsOgre/Addons/Plot.hpp>
#include <WPGraphicsOgre/Addons/COgreProfiler.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreOverlaySystem.h>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {
        CProfilerOgre::CProfilerOgre() : m_nextUpdate( 0.f )
        {
            m_profiler = new Ogre::Profiler();

            m_profiler->setTimer( Ogre::Root::getSingletonPtr()->getTimer() );
        }

        CProfilerOgre::~CProfilerOgre()
        {
            delete m_profiler;
        }

        void CProfilerOgre::update( float t, float dt )
        {
        }

        bool CProfilerOgre::isVisible() const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            return m_overlay->isVisible();
        }

        void CProfilerOgre::setVisible( bool visible )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_profiler->setEnabled( visible );
        }

        SmartPtr<IProfile> CProfilerOgre::addProfile()
        {
            // SmartPtr<IProfile> profile(new CProfileOgre, true);
            // m_profiles.push_back(profile);
            // return profile;

            return nullptr;
        }

        SmartPtr<IProfile> CProfilerOgre::addProfile( hash32 id )
        {
            return nullptr;
        }

        void CProfilerOgre::removeProfile( SmartPtr<IProfile> profile )
        {
            // m_profiles.erase_element(profile);
        }

        SmartPtr<IProfile> CProfilerOgre::getProfile( hash32 id )
        {
            return nullptr;
        }

        void CProfilerOgre::start( const String &name )
        {
            m_profiler->beginProfile( name.c_str() );
        }

        void CProfilerOgre::end( const String &name )
        {
            m_profiler->endProfile( name.c_str() );
        }

        void CProfilerOgre::logResults()
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_profiler->logResults();
        }
    }  // namespace render
}  // namespace workphone
