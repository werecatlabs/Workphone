#pragma once

#include <atomic>
#include <cmath>

namespace workphone::editor
{
    // Render-task clock state with activity notifications from input/UI threads.
    // Periodic refresh is deliberate: not every asset/terrain change has a revision.
    class EditorSceneRefreshPolicy
    {
    public:
        void requestActivity() { m_activity.store( true, std::memory_order_relaxed ); }

        bool needsFrame( double now, bool realtime, bool viewChanged )
        {
            const bool activity = m_activity.exchange( false, std::memory_order_relaxed );
            if( !std::isfinite( now ) )
            {
                m_initialized = false;
                return true;
            }
            if( !m_initialized || now < m_lastTime || activity || viewChanged )
            {
                m_activeUntil = now + 0.5;
                m_nextRefresh = now;
                m_initialized = true;
            }
            m_lastTime = now;
            if( realtime || now < m_activeUntil || now >= m_nextRefresh )
            {
                m_nextRefresh = now + 1.0 / 15.0;
                return true;
            }
            return false;
        }

    private:
        std::atomic<bool> m_activity{ true };
        bool m_initialized = false;
        double m_lastTime = 0.0, m_activeUntil = 0.0, m_nextRefresh = 0.0;
    };
}
