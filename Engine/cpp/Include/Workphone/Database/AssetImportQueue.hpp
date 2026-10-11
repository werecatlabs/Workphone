#pragma once
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace workphone
{
    // FIFO admission with one drainer; callbacks always run outside the queue mutex.
    // A cancelled scheduler job must call cancel() to release the drain reservation.
    class AssetImportQueue
    {
    public:
        bool enqueue( std::function<void()> operation )
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_pending.push_back( std::move( operation ) );
            if( m_reserved )
                return false;
            m_reserved = true;
            return true;
        }
        void drain()
        {
            for( ;; )
            {
                std::function<void()> operation;
                {
                    std::lock_guard<std::mutex> lock( m_mutex );
                    if( m_pending.empty() )
                    {
                        m_reserved = false;
                        return;
                    }
                    operation = std::move( m_pending.front() );
                    m_pending.pop_front();
                }
                // Production callbacks report errors themselves. Release admission
                // if an unexpected exception escapes, so later requests can recover.
                try
                {
                    operation();
                }
                catch( ... )
                {
                    cancel();
                    throw;
                }
            }
        }
        void cancel()
        {
            std::deque<std::function<void()>> abandoned;
            {
                std::lock_guard<std::mutex> lock( m_mutex );
                abandoned.swap( m_pending );
                m_reserved = false;
            }
            // Captured resource owners may run destructors; release them unlocked.
        }

    private:
        std::mutex m_mutex;
        std::deque<std::function<void()>> m_pending;
        bool m_reserved = false;
    };
}  // namespace workphone
