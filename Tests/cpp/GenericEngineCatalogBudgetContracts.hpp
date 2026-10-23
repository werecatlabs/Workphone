#pragma once

#include <Workphone/Core/GenericPool.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <Workphone/Interface/Database/IDatabaseQuery.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/System/StateQueue.hpp>
#include <Workphone/System/StateContext.hpp>
#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <array>
#include <stdexcept>

namespace generic_engine_catalog_budget_contracts
{
    using namespace workphone;
    inline void require(bool value, const char *message)
    { if (!value) throw std::runtime_error(message); }

    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t(StreamMemoryClass::SourceCpu);
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set(u64 bytes)
        {
            String error;
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu, bytes, error), "generic budget capacity applies");
        }
        void restore() { set(original); }
        ~Capacity() { restore(); }
    };
    inline bool testStartupAdmission = false;

    inline void initializeTypes(TypeManager &types)
    {
        if (!testStartupAdmission) { types.load(); return; }
        Capacity budget;
        budget.set(budget.used());
        bool denied = false;
        try { types.load(); } catch (const std::bad_alloc &) { denied = true; }
        require(denied && types.baseObjectDataPools.empty(),
                "engine metadata bootstrap rejects unavailable pool-state capacity");
        budget.restore();
        types.load();
        require(!types.baseObjectDataPools.empty() &&
                    types.baseObjectDataPools.size() == types.sharedObjectDataPools.size(),
                "engine metadata bootstrap can retry complete type storage after denial");
        testStartupAdmission = false;
    }

    // Run before the application fixture: low-level object pools must not need
    // an ApplicationManager or start streaming workers just to allocate storage.
    inline void bootstrap()
    {
        auto storage = allocateSourceBudgetStorage(257, 64);
        auto first = WorldStreamingScheduler::shared();
        const auto before = first->stats();
        require(before.used[Capacity::source] >= sourceBudgetStorageBytes(257, 64),
                "bootstrap allocation is visible to the shared service");
        String error;
        require(first->setBudget(StreamMemoryClass::SourceCpu, before.limits[Capacity::source] - 1, error),
                "shared ledger accepts a configured startup limit");
        WorldStreamingScheduler::shutdownShared();
        auto restarted = WorldStreamingScheduler::shared();
        require(restarted->stats().used[Capacity::source] == before.used[Capacity::source] &&
                    restarted->stats().limits[Capacity::source] == before.limits[Capacity::source] - 1,
                "service restart preserves live physical allocations and configured limits");
        require(restarted->setBudget(StreamMemoryClass::SourceCpu, before.limits[Capacity::source], error),
                "startup limit restores");
        { WorldStreamingScheduler isolated;
          require(!isolated.stats().used[Capacity::source], "isolated scheduler retains its independent ledger"); }
        freeSourceBudgetStorage(storage, 64);
        require(restarted->stats().used[Capacity::source] + sourceBudgetStorageBytes(257, 64) ==
                    before.used[Capacity::source], "bootstrap storage retires through the same ledger");
        testStartupAdmission = true;
    }

    inline void pools()
    {
        Capacity budget;
        const auto baseline = budget.used();
        struct Cell { u64 value; };
        {
            GenericPool<Cell, SourceBudgetAllocator<Cell>> pool(2);
            const auto empty = budget.used();
            require(empty > baseline, "generic pool state and control storage are admitted");
            budget.set(empty);
            bool denied = false;
            try { pool.allocate_object(); } catch (const std::bad_alloc &) { denied = true; }
            require(denied && !pool.getSize() && budget.used() == empty,
                    "denied pool growth leaves no block or reservation");
            budget.restore();
            std::array<Cell *, 4> cells;
            for (auto &cell : cells) cell = pool.allocate_object();
            auto alias = pool;
            require(pool.getSize() == 2 && alias.getSize() == 2, "generic pool aliases share admitted blocks");
            const auto resident = budget.used();
            budget.set(resident);
            for (auto cell : cells) alias.free_object(cell);
            for (auto &cell : cells) cell = pool.allocate_object();
            require(budget.used() == resident, "all pool returns and reuse succeed at capacity");
            for (auto cell : cells) pool.free_object(cell);
            pool.clear();
            require(!alias.getSize() && budget.used() == empty,
                    "pool clear retires raw blocks, free lists and block inventories");
            budget.restore();
        }
        require(budget.used() == baseline, "last generic pool alias retires control storage");
        {
            GenericPoolData<Cell, SourceBudgetAllocator<Cell>> block;
            block.setNumElements(4);
            auto data = block.getData();
            block.getFreeElement();
            const auto resident = budget.used();
            budget.set(resident);
            bool denied = false;
            try { block.setNumElements(8); } catch (const std::bad_alloc &) { denied = true; }
            require(denied && block.getData() == data && block.getNumAllocated() == 4 &&
                        block.getNumFree() == 3 && budget.used() == resident,
                    "denied raw pool replacement preserves allocation and free list");
            budget.restore();
            block.setNumElements(8);
            require(block.getNumAllocated() == 8 && block.getNumFree() == 8,
                    "raw pool replacement retries after capacity is restored");
        }
        require(budget.used() == baseline, "raw pool replacement retires all storage");
        {
            GenericPoolData<Cell, SourceBudgetAllocator<Cell>> block(4);
            budget.set(baseline);
            bool denied = false;
            try { block.load(nullptr); } catch (const std::bad_alloc &) { denied = true; }
            require(denied && !block.getData() && !block.getNumFree() && budget.used() == baseline,
                    "denied raw pool load leaves a retryable empty block");
            budget.restore();
            block.load(nullptr);
            require(block.getData() && block.getNumFree() == 4, "raw pool load retries after denial");
        }
        require(budget.used() == baseline, "retried pool load retires all storage");
    }

    class Registry final : public ResourceDatabase
    {
    public:
        void publish(const Array<SmartPtr<ISharedObject>> &objects) { replaceObjectInventory(objects); }
        SmartPtr<ISharedObject> lookup(const workphone::UUID &id) const
        { SmartPtr<ISharedObject> value; m_objects.tryGet(id, value); return value; }
        void cache(const String &path, SmartPtr<IResource> resource)
        { m_resourceMap[1][SourceBudgetString(path)] = resource; }
        SmartPtr<IResource> cached(const String &path)
        {
            auto outer = m_resourceMap.find(1);
            if (outer == m_resourceMap.end()) return nullptr;
            auto inner = outer->second.findAs(path);
            return inner == outer->second.end() ? nullptr : inner->second.lock();
        }
        void clearCache() { m_resourceMap = decltype(m_resourceMap){}; }
    };

    inline void registries()
    {
        Registry registry;
        auto first = make_ptr<render::ClawTexture>();
        auto second = make_ptr<render::ClawTexture>();
        first->getHandle()->setUUID(StringUtil::getUUID());
        second->getHandle()->setUUID(StringUtil::getUUID());
        const auto firstId = first->getHandle()->getUUID();
        const auto secondId = second->getHandle()->getUUID();
        const String path("city/catalog/long/path/that/owns/its/retained/metadata/first.texture");
        const String otherPath("city/catalog/long/path/that/owns/its/retained/metadata/second.texture");
        const Array<SmartPtr<ISharedObject>> oldObjects{first};
        const Array<SmartPtr<ISharedObject>> newObjects{second};
        Capacity budget;
        const auto baseline = budget.used();
        registry.publish(oldObjects);
        const auto resident = budget.used();
        budget.set(resident);
        bool denied = false;
        try { registry.publish(newObjects); } catch (const std::bad_alloc &) { denied = true; }
        require(denied && registry.lookup(firstId) == first && !registry.lookup(secondId) &&
                    budget.used() == resident, "registry denial preserves complete old identity inventory");
        budget.restore();
        registry.publish(newObjects);
        require(!registry.lookup(firstId) && registry.lookup(secondId) == second,
                "registry retry atomically replaces object identities");
        registry.publish({});
        require(budget.used() == baseline, "empty registry publication releases retained map blocks");
        registry.cache(path, first);
        const auto cached = budget.used();
        const auto rejected = budget.scheduler->stats().rejectedReservations;
        budget.set(cached);
        require(registry.cached(path) == first && budget.used() == cached &&
                    budget.scheduler->stats().rejectedReservations == rejected,
                "cached path lookup requires no new key allocation at capacity");
        denied = false;
        try { registry.cache(otherPath, second); } catch (const std::bad_alloc &) { denied = true; }
        require(denied && registry.cached(path) == first && !registry.cached(otherPath) &&
                    budget.used() == cached, "denied path growth preserves old weak resource cache");
        budget.restore();
        registry.cache(otherPath, second);
        require(registry.cached(otherPath) == second, "resource cache retries after capacity restoration");
        registry.clearCache();
        require(budget.used() == baseline, "cache teardown releases keys, nodes and map inventories");
        const Array<SmartPtr<IResource>> instances{first, second};
        budget.set(baseline);
        denied = false;
        try { registry.setInstances(instances); } catch (const std::bad_alloc &) { denied = true; }
        require(denied && registry.getInstances().empty() && budget.used() == baseline,
                "resource instance inventory denies before publishing owners");
        budget.restore();
        registry.setInstances(instances);
        const auto inventory = budget.used();
        budget.set(inventory);
        denied = false;
        try { registry.setInstances(instances); } catch (const std::bad_alloc &) { denied = true; }
        const auto retained = registry.getInstances();
        require(denied && retained.size() == 2 && retained[0] == first && retained[1] == second &&
                    budget.used() == inventory, "denied instance replacement preserves old inventory");
        registry.setInstances({});
        require(budget.used() == baseline, "empty instance replacement retires inventory storage");
        budget.restore();
    }

    template<class Fixture> void queries(Fixture &fixture)
    {
        auto database = fixture.catalog->getDatabase();
        auto bound = dynamic_cast<IParameterizedDatabase *>(database.get());
        require(bound != nullptr, "catalog exposes bound SQL queries");
        const String value("a-city-catalog-field-with-retained-storage-longer-than-inline-string-capacity");
        String sql;
        for (int i = 0; i < 16; ++i)
            sql += (i ? " UNION ALL SELECT '" : "SELECT '") + value + "' AS value";
        const String one = "SELECT '" + value + "' AS value";
        // Warm generic query control slots; their retained pool capacity has a
        // different lifetime from an individual query's rows and field strings.
        { auto a = bound->queryBound(one, {}); auto b = bound->queryBound(one, {});
          require(a && b, "query control slots warm successfully"); }
        Capacity budget;
        const auto baseline = budget.used();
        auto previous = bound->queryBound(one, {});
        require(previous && !previous->eof(), "resident query has a complete row");
        const auto resident = budget.used();
        require(resident > baseline, "catalog result storage is admitted into SourceCpu");
        for (u64 headroom : {0ULL, 64ULL, 128ULL, 256ULL, 512ULL, 1024ULL})
        {
            budget.set(resident + headroom);
            require(!bound->queryBound(sql, {}) && budget.used() == resident,
                    "denied eager query returns no partial rows and rolls back all reservations");
            require(previous->getFieldValue("value") == value && !previous->eof(),
                    "query denial preserves other live results and cursors");
        }
        budget.set(resident);
        require(!database->query(sql) && budget.used() == resident,
                "unbound query has the same denial contract");
        auto mutation = bound->queryBound("CREATE TEMP TABLE wp_memory_budget_probe(value INTEGER)", {});
        require(mutation && mutation->eof() && budget.used() == resident,
                "successful SQL mutations are acknowledged at capacity without row storage");
        mutation = nullptr;
        budget.restore();
        auto result = bound->queryBound(sql, {});
        require(result != nullptr, "catalog query retries after budget restoration");
        auto alias = result;
        const auto rowsUsed = budget.used();
        unsigned rows = 0;
        while (!result->eof())
        {
            require(result->getNumFields() == 1 && result->getFieldValue("value") == value,
                    "complete admitted query preserves fields");
            ++rows; result->nextRow();
        }
        require(rows == 16 && rowsUsed > resident, "all catalog rows are retained and charged");
        result = nullptr;
        require(budget.used() == rowsUsed, "query alias retains physical result storage");
        alias = nullptr;
        require(budget.used() == resident, "last query alias releases all rows and field buffers");
        previous = nullptr;
        require(budget.used() == baseline, "all catalog result storage retires");
        database->queryDML("DROP TABLE wp_memory_budget_probe");
    }

    inline void queues()
    {
        auto message = make_ptr<StateMessage>();
        auto queue = std::make_unique<StateQueue>();
        queue->load(nullptr);
        Capacity budget;
        const auto baseline = budget.used();
        {
            const auto empty = budget.used();
            budget.set(empty);
            bool denied = false;
            try { queue->queueMessage(message); } catch (const std::bad_alloc &) { denied = true; }
            require(denied && queue->isEmpty() && budget.used() == empty,
                    "denied state enqueue retains no partial message or reservation");
            budget.restore();
            queue->queueMessage(message);
            const auto resident = budget.used();
            auto messages = queue->getMessagesAndClear();
            require(resident > empty && messages.size() == 1 && messages[0] == message,
                    "state queue storage is admitted and retains its message");
            queue->queueMessage(message);
            budget.set(resident);
            queue->clear();
            queue->queueMessage(message);
            messages = queue->getMessagesAndClear();
            require(budget.used() == resident && messages.size() == 1 && messages[0] == message,
                    "state queue reuses retained capacity when the budget is full");
            queue->unload(nullptr);
            budget.restore();
        }
        queue.reset();
        require(budget.used() == baseline,
                "state queue message capacity retires with its owner");
    }

    template<class Fixture> void contexts(Fixture &fixture)
    {
        auto context = make_ptr<StateContext>();
        context->load(nullptr);
        require(context->isLoaded(), "budgeted state context loads");
        std::array<SmartPtr<State>, 13> states;
        for (auto &state : states) state = make_ptr<State>();
        for (size_t i = 0; i < 12; ++i) context->addState(states[i]);
        Capacity budget;
        const auto resident = budget.used();
        budget.set(resident);
        bool denied = false;
        try { context->addState(states.back()); } catch (const std::bad_alloc &) { denied = true; }
        require(denied && context->getStates().size() == 12 && !states.back()->getStateContext() &&
                    budget.used() == resident, "denied context growth preserves state ownership and inventory");
        budget.restore();
        context->addState(states.back());
        require(context->getStates().size() == 13 && states.back()->getStateContext() == context,
                "context state inventory retries after capacity restoration");
        const auto grown = budget.used();
        budget.set(grown);
        context->removeState(states[4]);
        const auto compacted = context->getStates();
        require(compacted.size() == 12 && compacted[4] == states[5] && budget.used() == grown,
                "state removal compacts in order without requiring budget headroom");
        context->addState(states[4]);
        require(context->getStates().size() == 13 && budget.used() == grown,
                "state registration reuses retained capacity at the cap");
        budget.restore();
        auto foreign = make_ptr<StateContext>();
        auto message = make_ptr<StateMessage>();
        message->setStateContext(foreign);
        const auto beforeMessage = budget.used();
        budget.set(beforeMessage);
        denied = false;
        try { context->addMessage(TaskId::Primary, message); } catch (const std::bad_alloc &) { denied = true; }
        auto queue = context->getStateQueue(u32(TaskId::Primary));
        require(denied && message->getStateContext() == foreign && (!queue || queue->isEmpty()) &&
                    budget.used() == beforeMessage, "denied message admission restores its previous context");
        budget.restore();
        context->addMessage(TaskId::Primary, message);
        queue = context->getStateQueue(u32(TaskId::Primary));
        require(queue && !queue->isEmpty() && message->getStateContext() == context,
                "message enqueue retries through the normal state manager");
        context->unload(nullptr);
        require(context->getStates().empty(), "context unload clears admitted state inventory");
        auto first = fixture.states->addStateContext();
        auto second = fixture.states->addStateContext();
        require(first && second, "manager creates admitted contexts");
        const auto managed = fixture.states->getStateContexts().size();
        const auto beforeRetirement = budget.used();
        budget.set(beforeRetirement);
        require(fixture.states->removeStateContext(first) && !first->isLoaded() && second->isLoaded() &&
                    fixture.states->getStateContexts().size() == managed - 1 && budget.used() < beforeRetirement,
                "manager retirement reclaims context storage without allocating a replacement inventory");
        require(fixture.states->removeStateContext(second), "remaining managed context retires at capacity");
    }

    template<class Fixture> void run(Fixture &fixture)
    {
        pools();
        registries();
        queues();
        queries(fixture);
        contexts(fixture);
        std::cout << "PASS: generic pool, catalog inventory and eager query memory budgets\n";
    }
}
