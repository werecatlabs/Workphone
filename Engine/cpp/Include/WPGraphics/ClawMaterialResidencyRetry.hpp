#pragma once

#include <chrono>

namespace workphone::render::detail
{
    /** Bound repeated idle inspection without caching an authoring decision. */
    class MaterialResidencyRetry
    {
    public:
        using Clock = std::chrono::steady_clock;

        bool ready( Clock::time_point now ) const noexcept
        {
            return now >= m_nextAttempt;
        }

        void defer( Clock::time_point now ) noexcept
        {
            m_nextAttempt = now + std::chrono::seconds( 1 );
        }

        void reset() noexcept
        {
            m_nextAttempt = {};
        }

    private:
        Clock::time_point m_nextAttempt{};
    };
}
