#pragma once
#include <deque>
#include <functional>
#include <mutex>

namespace workphone
{
    // FIFO admission with one drainer; callbacks always run outside the queue mutex.
    // A cancelled scheduler job must call cancel() to release the drain reservation.
    class AssetImportQueue
    {
    public:
        bool enqueue( std::function<void()> operation );
        void drain();
        void cancel();

    private:
        std::mutex m_mutex;
        std::deque<std::function<void()>> m_pending;
        bool m_reserved = false;
    };
}  // namespace workphone
