#ifndef WP_EDITOR_DEFERRED_SCRIPT_EVENTS_HPP
#define WP_EDITOR_DEFERRED_SCRIPT_EVENTS_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <utility>

namespace workphone::editor
{
    template <class Invoker, class Member, class Parameters>
    void invokeEditorScriptCall( Invoker &invoker, const Member &member,
                                 const Parameters &parameters, bool hasParameters,
                                 bool discardResults )
    {
        if( discardResults )
        {
            // The mutable results argument is part of the Lua callback ABI even
            // when its native event listener ignores the returned values.
            Parameters results;
            invoker.callObjectMember( member, parameters, results );
        }
        else if( hasParameters ) invoker.callObjectMember( member, parameters );
        else invoker.callObjectMember( member );
    }

    // Only Editor callbacks use this queue. Runtime script calls retain their
    // synchronous ABI; a suspended callback never owns an active ImGui frame.
    class DeferredEditorScriptEvents
    {
    public:
        explicit DeferredEditorScriptEvents( std::size_t capacity = 1024,
                                              std::size_t debuggerCapacity = 64 ) :
            m_capacity( capacity ), m_debuggerCapacity( debuggerCapacity ) {}

        bool enqueue( std::function<void()> call, bool debuggerControl,
                      std::uintptr_t owner = 0, std::string refreshKey = {} )
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            if( !refreshKey.empty() && !m_pending.empty() )
            {
                auto &entry = m_pending.back();
                // Never move a refresh/cancel across an intervening command.
                if( entry.owner == owner && entry.refreshKey == refreshKey &&
                    entry.debuggerControl == debuggerControl )
                {
                    entry.call = std::move( call );
                    return true;
                }
            }
            auto &laneCount = debuggerControl ? m_debuggerCount : m_authoringCount;
            const auto laneCapacity = debuggerControl ? m_debuggerCapacity : m_capacity;
            if( laneCount >= laneCapacity ) return false;
            m_pending.push_back( { std::move( call ), debuggerControl, owner, std::move( refreshKey ) } );
            ++laneCount;
            return true;
        }

        void dispatch( bool debuggerOnly )
        {
            std::deque<Entry> ready;
            {
                std::lock_guard<std::mutex> lock( m_mutex );
                const auto count = m_pending.size();
                for( std::size_t i = 0; i < count; ++i )
                {
                    auto entry = std::move( m_pending.front() );
                    m_pending.pop_front();
                    if( !debuggerOnly || entry.debuggerControl )
                    {
                        auto &laneCount = entry.debuggerControl ? m_debuggerCount : m_authoringCount;
                        --laneCount;
                        ready.push_back( std::move( entry ) );
                    }
                    else m_pending.push_back( std::move( entry ) );
                }
            }
            // Release the queue lock before Lua can stop. Its pause pump may
            // drain newly arrived debugger controls from this same queue.
            for( auto &entry : ready ) entry.call();
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_pending.clear();
            m_authoringCount = 0;
            m_debuggerCount = 0;
        }

        std::size_t size() const
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            return m_pending.size();
        }

    private:
        struct Entry
        {
            std::function<void()> call;
            bool debuggerControl;
            std::uintptr_t owner;
            std::string refreshKey;
        };
        const std::size_t m_capacity;
        const std::size_t m_debuggerCapacity;
        std::size_t m_authoringCount = 0, m_debuggerCount = 0;
        mutable std::mutex m_mutex;
        std::deque<Entry> m_pending;
    };
}
#endif
