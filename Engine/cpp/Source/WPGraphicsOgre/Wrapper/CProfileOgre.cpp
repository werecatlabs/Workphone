#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CProfileOgre.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        CProfileOgre::CProfileOgre()
        {
        }

        CProfileOgre::~CProfileOgre()
        {
        }

        void CProfileOgre::start()
        {
            auto applicationManager = core::IApplicationManager::instance();

            // m_start = boost::chrono::high_resolution_clock::now().time_since_epoch().count();
        }

        void CProfileOgre::end()
        {
            // m_end = boost::chrono::high_resolution_clock::now().time_since_epoch().count();
            // unsigned int interval = m_end - m_start;
            // m_timeTaken = (double)(interval) / (double)(1000000000LL);
        }

        String CProfileOgre::getDescription() const
        {
            return m_description;
        }

        void CProfileOgre::setDescription( const String &description )
        {
            m_description = description;
        }

        time_interval CProfileOgre::getTimeTaken() const
        {
            return m_timeTaken;
        }

        void CProfileOgre::setTimeTaken( time_interval timeTaken )
        {
            m_timeTaken = timeTaken;
        }

        String CProfileOgre::getLabel() const
        {
            return m_label;
        }

        void CProfileOgre::setLabel( const String &label )
        {
            m_label = label;
        }
    }  // namespace render
}  // namespace workphone
