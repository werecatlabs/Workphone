#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <atomic>
#include <limits>
#include <memory_resource>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <utility>

namespace workphone::render
{
    namespace
    {
        constexpr u32 noSlot = ~u32(0), pageSize = 64;
        std::atomic<u64> nextTable{1};
        u64 tableIdentity()
        {
            auto id = nextTable.load(std::memory_order_relaxed);
            do {
                if(id == std::numeric_limits<u64>::max())
                    throw std::overflow_error("material record table identities exhausted");
            } while(!nextTable.compare_exchange_weak(id, id + 1, std::memory_order_relaxed));
            return id;
        }
    }

    struct ClawMaterialRenderCache::Slot
    {
        std::weak_ptr<const MaterialRenderState> owner;
        const MaterialRenderState *address = nullptr;
        ClawPreparedMaterialDX11 prepared;
        u64 generation = 1;
        u32 index = noSlot, nextFree = noSlot, nextIdentity = noSlot;
        bool matches(const MaterialRenderState::Ptr &state) const
        {
            return address == state.get() && !owner.owner_before(state) && !state.owner_before(owner);
        }
    };

    struct ClawMaterialRenderCache::Store
    {
        class Memory final : public std::pmr::memory_resource
        {
        public:
            u64 bytes = 0, allocations = 0;
        private:
            void *do_allocate(size_t count, size_t alignment) override
            {
                auto result = std::pmr::new_delete_resource()->allocate(count, alignment);
                bytes += count; ++allocations; return result;
            }
            void do_deallocate(void *pointer, size_t count, size_t alignment) override
            {
                std::pmr::new_delete_resource()->deallocate(pointer, count, alignment); bytes -= count;
            }
            bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override
            { return this == &other; }
        } memory;
        const u64 identity = tableIdentity();
        std::pmr::vector<Slot *> pages{&memory};
        // Values are chain heads: distinct shared ownership identities at the
        // same address must not alias (including externally aliased snapshots).
        std::pmr::unordered_map<const MaterialRenderState *, u32> identities{&memory};
        u32 size = 0, resident = 0, freeHead = noSlot, sweep = 0;

        ~Store()
        {
            std::pmr::polymorphic_allocator<Slot> allocator(&memory);
            for(auto page : pages)
            {
                std::destroy_n(page, pageSize);
                allocator.deallocate(page, pageSize);
            }
        }
        Slot &at(u32 index) const { return pages[index / pageSize][index % pageSize]; }
        Slot *find(const MaterialRenderState::Ptr &state)
        {
            const auto found = identities.find(state.get());
            for(auto index = found == identities.end() ? noSlot : found->second; index != noSlot;)
            {
                auto &slot = at(index);
                if(slot.matches(state)) return &slot;
                index = slot.nextIdentity;
            }
            return nullptr;
        }
        void collect(u32 budget)
        {
            for(u32 checked = 0, limit = std::min(budget, size); checked < limit; ++checked)
            {
                if(sweep >= size) sweep = 0;
                auto &slot = at(sweep++);
                if(!slot.address || !slot.owner.expired()) continue;
                auto found = identities.find(slot.address);
                if(found != identities.end())
                {
                    auto link = &found->second;
                    while(*link != noSlot && *link != slot.index) link = &at(*link).nextIdentity;
                    if(*link == slot.index) *link = slot.nextIdentity;
                    if(found->second == noSlot) identities.erase(found);
                }
                slot.owner.reset(); slot.address = nullptr; slot.nextIdentity = noSlot; --resident;
                // Saturate rather than reusing a generation after overflow.
                if(slot.generation != std::numeric_limits<u64>::max())
                {
                    ++slot.generation; slot.nextFree = freeHead; freeHead = slot.index;
                }
            }
        }
        Slot &allocate()
        {
            collect(32);
            if(freeHead != noSlot)
            {
                auto &slot = at(freeHead); freeHead = slot.nextFree;
                slot.nextFree = noSlot; return slot;
            }
            if(size == noSlot) throw std::length_error("too many persistent material records");
            if(size % pageSize == 0)
            {
                std::pmr::polymorphic_allocator<Slot> allocator(&memory);
                auto page = allocator.allocate(pageSize);
                std::uninitialized_value_construct_n(page, pageSize);
                try { pages.push_back(page); }
                catch(...) { std::destroy_n(page, pageSize); allocator.deallocate(page, pageSize); throw; }
            }
            auto &slot = at(size); slot.index = size++; return slot;
        }
        Slot &insert(const MaterialRenderState::Ptr &state)
        {
            auto &slot = allocate();
            try
            {
                auto inserted = identities.try_emplace(state.get(), noSlot);
                slot.prepared = ClawMaterialRenderCache::prepare(*state);
                slot.owner = state; slot.address = state.get();
                slot.nextIdentity = inserted.first->second; inserted.first->second = slot.index;
                ++resident;
            }
            catch(...) { slot.nextFree = freeHead; freeHead = slot.index; throw; }
            return slot;
        }
        const ClawPreparedMaterialDX11 *get(Handle handle, const MaterialRenderState *pinned) const
        {
            if(handle.table != identity || handle.slot >= size) return nullptr;
            const auto &slot = at(handle.slot);
            if(handle.generation != slot.generation || !slot.address ||
                (pinned ? slot.address != pinned : slot.owner.expired())) return nullptr;
            return &slot.prepared;
        }
    };

    ClawMaterialRenderCache::ClawMaterialRenderCache() = default;
    ClawMaterialRenderCache::~ClawMaterialRenderCache() = default;
    ClawMaterialRenderCache::ClawMaterialRenderCache(const ClawMaterialRenderCache &) = default;
    ClawMaterialRenderCache &ClawMaterialRenderCache::operator=(const ClawMaterialRenderCache &) = default;
    ClawMaterialRenderCache::ClawMaterialRenderCache(ClawMaterialRenderCache &&other) noexcept
    { *this = std::move(other); }
    ClawMaterialRenderCache &ClawMaterialRenderCache::operator=(ClawMaterialRenderCache &&other) noexcept
    {
        if(this != &other)
        {
            m_store = std::move(other.m_store); m_sets = std::exchange(other.m_sets, {});
            m_lastIndex = std::exchange(other.m_lastIndex, Capacity);
            m_maintenanceIndex = std::exchange(other.m_maintenanceIndex, 0);
            m_preparations = std::exchange(other.m_preparations, 0);
        }
        return *this;
    }
    const ClawPreparedMaterialDX11 &ClawMaterialRenderCache::defaults()
    { static const auto value = prepare(MaterialRenderState{}); return value; }
    const ClawPreparedMaterialDX11 &ClawMaterialRenderCache::getSlow(
        const MaterialRenderState::Ptr &state, size_t setIndex)
    {
        auto &set = m_sets[setIndex];
        if(!m_store) m_store = std::make_shared<Store>();
        auto slot = m_store->find(state);
        if(!slot) { slot = &m_store->insert(state); ++m_preparations; }
        const auto way = set.next++ % Ways;
        set.tags[way] = {state.get(), slot, &slot->prepared, state}; m_lastIndex = setIndex * Ways + way;
        return slot->prepared;
    }
    ClawMaterialRenderCache::Handle ClawMaterialRenderCache::resolve(const MaterialRenderState::Ptr &state)
    {
        if(!state) return {};
        get(state);
        const auto slot = m_sets[m_lastIndex / Ways].tags[m_lastIndex % Ways].slot;
        return {m_store->identity, slot->generation, slot->index};
    }
    const ClawPreparedMaterialDX11 *ClawMaterialRenderCache::lookup(Handle handle) const
    { return m_store ? m_store->get(handle, nullptr) : nullptr; }
    ClawMaterialRenderCache::Lease ClawMaterialRenderCache::retain() const
    { Lease result; result.m_store = m_store; return result; }
    const ClawPreparedMaterialDX11 *ClawMaterialRenderCache::Lease::get(Handle handle) const
    { return m_store ? m_store->get(handle, nullptr) : nullptr; }
    u64 ClawMaterialRenderCache::Lease::getStorageBytes() const
    { return m_store ? sizeof(Store) + m_store->memory.bytes : 0; }
    u64 ClawMaterialRenderCache::Lease::getIdentity() const
    { return m_store ? m_store->identity : 0; }
    const ClawPreparedMaterialDX11 *ClawMaterialRenderCache::Lease::getPinned(
        Handle handle, const MaterialRenderState *state) const
    { return m_store ? m_store->get(handle, state) : nullptr; }
    void ClawMaterialRenderCache::clear()
    { m_sets = {}; m_lastIndex = Capacity; m_maintenanceIndex = 0; m_store.reset(); m_preparations = 0; }
    void ClawMaterialRenderCache::collectExpired()
    {
        if(m_store) m_store->collect(m_store->size);
        for(auto &set : m_sets) for(auto &tag : set.tags)
            if(tag.owner.expired()) tag = {};
    }
    void ClawMaterialRenderCache::maintain(u32 budget)
    {
        if(m_store) m_store->collect(budget);
        for(u32 checked = 0, limit = std::min<u32>(budget, Capacity); checked < limit; ++checked)
        {
            auto &tag = m_sets[m_maintenanceIndex / Ways].tags[m_maintenanceIndex % Ways];
            if(tag.owner.expired()) tag = {};
            m_maintenanceIndex = (m_maintenanceIndex + 1) % Capacity;
        }
    }
    u64 ClawMaterialRenderCache::getStorageBytes() const
    { return sizeof(*this) + (m_store ? sizeof(Store) + m_store->memory.bytes : 0); }
    u64 ClawMaterialRenderCache::getStorageAllocationCount() const
    { return m_store ? m_store->memory.allocations : 0; }
    u32 ClawMaterialRenderCache::getSlotCount() const { return m_store ? m_store->size : 0; }
    u32 ClawMaterialRenderCache::getResidentCount() const { return m_store ? m_store->resident : 0; }
}
