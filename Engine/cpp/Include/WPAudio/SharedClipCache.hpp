#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace workphone::audio
{
    // Control/loader-thread only. Voices pin immutable clip generations. The
    // cache keeps weak references, so the last voice releases resident memory.
    // Content equality (not filenames or timestamps) prevents stale reimport
    // reuse and also deduplicates aliases without another asset catalog.
    template<class Clip>
    class SharedClipCache
    {
    public:
        enum class Result { Ready, Invalid, BudgetExceeded };
        struct Stats
        {
            size_t residentBytes = 0, pinnedClips = 0;
            uint64_t reuses = 0, rejections = 0;
        };
        explicit SharedClipCache(size_t budget = 64u * 1024u * 1024u, size_t capacity = 1024) :
            m_budget(budget), m_capacity(capacity) {}

        template<class Equal>
        Result intern(std::shared_ptr<const Clip> candidate, size_t bytes, Equal equal,
                      std::shared_ptr<const Clip> &out)
        {
            out.reset();
            if(!candidate || !bytes) return Result::Invalid;
            std::lock_guard<std::mutex> lock(m_mutex);
            prune();
            size_t resident = 0;
            for(const auto &entry : m_entries)
            {
                if(auto existing = entry.clip.lock())
                {
                    resident += entry.bytes;
                    if(entry.bytes == bytes && equal(*existing, *candidate))
                    {
                        ++m_reuses; out = std::move(existing); return Result::Ready;
                    }
                }
            }
            if(m_entries.size() >= m_capacity || bytes > m_budget || resident > m_budget - bytes)
            {
                ++m_rejections; return Result::BudgetExceeded;
            }
            m_entries.push_back({candidate, bytes});
            out = std::move(candidate);
            return Result::Ready;
        }

        Stats stats()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            prune();
            Stats result{}; result.reuses = m_reuses; result.rejections = m_rejections;
            for(const auto &entry : m_entries)
                if(auto clip = entry.clip.lock()) { result.residentBytes += entry.bytes; ++result.pinnedClips; }
            return result;
        }

    private:
        struct Entry { std::weak_ptr<const Clip> clip; size_t bytes; };
        void prune()
        {
            m_entries.erase(std::remove_if(m_entries.begin(),m_entries.end(),
                [](const Entry &entry) { return entry.clip.expired(); }),m_entries.end());
        }
        std::mutex m_mutex;
        std::vector<Entry> m_entries;
        size_t m_budget, m_capacity;
        uint64_t m_reuses = 0, m_rejections = 0;
    };
}
