#pragma once

#include <Workphone/Core/CharacterPoolAllocator.hpp>
#include <Workphone/Core/StringPool.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/System/Rttr.hpp>
#include "GenericEngineCatalogBudgetContracts.hpp"

namespace reflection_string_budget_contracts
{
    using namespace workphone;
    using generic_engine_catalog_budget_contracts::Capacity;
    using generic_engine_catalog_budget_contracts::require;

    template<class T> void characterPool(size_t count)
    {
        using Pool = CharacterPoolAllocator<T>;
        Pool::release_unused();
        Capacity budget;
        const auto baseline = budget.used();
        const auto before = Pool::stats();
        Pool allocator;
        auto first = allocator.allocate(count);
        first[0] = T('a');
        const auto firstResident = budget.used();
        budget.set(firstResident);
        bool pooledDenied = false;
        try { auto unexpected = allocator.allocate(count); allocator.deallocate(unexpected, count); }
        catch(const std::bad_alloc &) { pooledDenied = true; }
        require(pooledDenied && first[0] == T('a') && budget.used() == firstResident,
                "denied additional character slab preserves live storage and free-list metadata");
        budget.restore();
        auto second = allocator.allocate(count);
        const auto resident = budget.used();
        const auto live = Pool::stats();
        require(live.slabCount == before.slabCount + 2 &&
                    resident - baseline > 2 * Pool::rounded_count(count) * sizeof(T),
                "character slabs and their metadata are admitted as physical storage");
        first[0] = T('a'); second[0] = T('b');
        budget.set(resident);
        Pool::release_unused();
        require(first[0] == T('a') && second[0] == T('b') && budget.used() == resident,
                "trim preserves live string slabs at capacity");
        allocator.deallocate(first, count);
        allocator.deallocate(second, count);
        require(budget.used() == resident && Pool::stats().freeBlocks == before.freeBlocks + 2,
                "all slab returns succeed at the cap and spare blocks stay charged");
        auto reused = allocator.allocate(count);
        require(budget.used() == resident, "cached character blocks reuse admitted storage");
        allocator.deallocate(reused, count);
        Pool::release_unused();
        // The shared directory can retain capacity for unrelated live buckets.
        require(Pool::stats().slabCount == before.slabCount && budget.used() < resident,
                "trim retires idle slab and per-bucket metadata storage");
        budget.restore();
        const auto beforeDirect = budget.used();
        const auto directCount = Pool::MaxPooledBlockBytes / sizeof(T) + 7;
        auto direct = allocator.allocate(directCount);
        require(budget.used() == beforeDirect + sourceBudgetStorageBytes(directCount * sizeof(T), alignof(T)),
                "oversized character buffers use the same physical allocation ledger");
        allocator.deallocate(direct, directCount);
        require(budget.used() == beforeDirect, "oversized buffer release returns its reservation");
        budget.set(beforeDirect);
        bool denied = false;
        try { auto unexpected = allocator.allocate(directCount); allocator.deallocate(unexpected, directCount); }
        catch(const std::bad_alloc &) { denied = true; }
        require(denied && budget.used() == beforeDirect, "oversized string admission rejects without a leak");
    }

    inline void strings()
    {
        characterPool<char>(100003);
        characterPool<wchar_t>(25003);
        Capacity budget;
        String value(80, 'x');
        auto original = value.data();
        const auto resident = budget.used();
        budget.set(resident);
        bool denied = false;
        try { value.reserve(400000); } catch(const std::bad_alloc &) { denied = true; }
        require(denied && value.data() == original && value.size() == 80 && value[79] == 'x',
                "denied engine String growth preserves its buffer and contents");
        String error;
        require(!budget.scheduler->reserve(StreamMemoryClass::SourceCpu, 1, error) && !error.empty(),
                "budget denial reports an error without recursively locking the string ledger");
        require(!budget.scheduler->setBudget(StreamMemoryClass::SourceCpu, resident - 1, error) && !error.empty(),
                "budget shrink denial is safe when string admission has no headroom");
        // A zero-payload lease still owns admitted implementation/control storage.
        // Keep denial and successful diagnostic clearing as separate contracts.
        const auto beforeZeroDenial = budget.used();
        require(!budget.scheduler->reserve(StreamMemoryClass::SourceCpu, 0, error) &&
                    !error.empty() && budget.used() == beforeZeroDenial,
                "zero-payload admission rejects unavailable lease metadata without leaking storage");
        budget.restore();
        const auto beforeToken = budget.used();
        {
            auto token = budget.scheduler->reserve(StreamMemoryClass::SourceCpu, 0, error);
            require(token && token->bytes() == 0 &&
                        token->memoryClass() == StreamMemoryClass::SourceCpu && error.empty(),
                    "successful admission after capacity returns clears an existing denial diagnostic");
            require(budget.used() > beforeToken,
                    "zero-payload lease implementation and control storage remain charged");
        }
        require(budget.used() == beforeToken,
                "last zero-payload lease owner releases all admitted metadata storage");
        value.reserve(400000);
        require(value.size() == 80 && value[79] == 'x', "engine String growth retries after admission returns");
        {
            StringPool<char> legacy(512, 64);
            const auto held = budget.used();
            auto block = legacy.create("preserved", 9);
            budget.set(held);
            denied = false;
            try { legacy.resize(4096); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && std::strcmp(block, "preserved") == 0 && budget.used() == held,
                    "legacy string-pool replacement preserves live data on denied admission");
            denied = false;
            try { legacy.setMaxStringSize(1); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && legacy.getMaxStringSize() == 64 && std::strcmp(block, "preserved") == 0 &&
                        budget.used() == held,
                    "denied legacy free-list growth preserves block layout and live contents");
            legacy.destroy(block);
            for(int i = 0; i < 8; ++i) require(legacy.create(1), "legacy spare blocks remain reusable at the cap");
            require(!legacy.create(1) && budget.used() == held, "legacy pool spare capacity stays charged");
            budget.restore();
            legacy.resize(4096);
            require(legacy.create(1), "legacy pool resize retries after admission returns");
        }
    }

    inline void types(TypeManager &manager)
    {
        Capacity budget;
        const u32 actor = scene::GameActor::typeInfo();
        const auto originalHash = manager.getHash(actor);
        const auto capacity = manager.baseObjectDataPools.size();
        while(manager.getTotalNumTypes() < capacity) manager.getNewTypeId();
        const auto count = manager.getTotalNumTypes();
        const auto originalPools = manager.baseObjectDataPools.data();
        const auto resident = budget.used();
        budget.set(resident);
        bool denied = false;
        try { manager.getNewTypeId("BudgetedDynamicType", actor); }
        catch(const std::bad_alloc &) { denied = true; }
        require(denied && manager.getTotalNumTypes() == count &&
                    manager.baseObjectDataPools.data() == originalPools && manager.getHash(actor) == originalHash &&
                    manager.getTypeByName("BudgetedDynamicType") == 0 && budget.used() == resident,
                "denied type-table growth consumes no id and preserves published tables");
        // Admit the first staging copy, then deny further table preparation.
        using Pool = decltype(manager.baseObjectDataPools)::value_type;
        budget.set(resident + sourceBudgetStorageBytes(capacity * sizeof(Pool), alignof(Pool)));
        denied = false;
        try { manager.getNewTypeId(); } catch(const std::bad_alloc &) { denied = true; }
        require(denied && manager.getTotalNumTypes() == count &&
                    manager.baseObjectDataPools.data() == originalPools && budget.used() == resident,
                "partial type-table preparation rolls back its admitted staging storage");
        budget.restore();
        const auto id = manager.getNewTypeId("BudgetedDynamicType", actor);
        require(id == count && manager.getBaseType(id) == actor && manager.getHash(actor) == originalHash &&
                    manager.baseObjectDataPools.size() > capacity && budget.used() > resident,
                "type-table growth retries with stable ids and charges replacement directories");
        const auto objectCount = manager.m_objectDataSize.load();
        const auto objectsResident = budget.used();
        budget.set(objectsResident);
        manager.resizeObjectData(0);
        require(manager.getNewTypeId("BudgetedDynamicType", actor) == id && budget.used() == objectsResident,
                "existing type lookup and zero object growth require no admission at the cap");
        denied = false;
        try { manager.resizeObjectData(7); } catch(const std::bad_alloc &) { denied = true; }
        require(denied && manager.m_objectDataSize == objectCount && manager.iTypeInfo.size() == objectCount &&
                    manager.objectFlags.size() == objectCount && budget.used() == objectsResident,
                "object metadata admission preserves both tables on failure");
        budget.restore();
        manager.resizeObjectData(7);
        require(manager.m_objectDataSize == objectCount + 7 && manager.iTypeInfo[objectCount + 6] == 0 &&
                    manager.objectFlags[objectCount + 6] == (OBJECT_FLAG_ALIVE | OBJECT_FLAG_GARBAGE_COLLECTED),
                "object metadata growth initializes the entire added range");

        const auto beforeReflection = budget.used();
        u64 registryBytes = 0;
        {
            auto reflected = workphone::make_shared<reflection::Type>();
            reflected->name = "A reflection type name which exceeds inline storage capacity";
            reflection::Property property;
            property.name = "A property name which exceeds inline storage capacity";
            property.getter = [](void *) -> reflection::Any { return 42; };
            reflected->addProperty(property);
            reflection::Method method;
            method.name = "A method name which exceeds inline storage capacity";
            method.invoker = [](void *, const reflection::Arguments &) -> reflection::Any { return 7; };
            reflected->addMethod(method);
            require(budget.used() > beforeReflection + sizeof(reflection::Type),
                    "reflection wrapper, control block, names and member tables are admitted");
            const auto members = budget.used();
            budget.set(members);
            denied = false;
            try { reflected->addProperty({"unadmitted", {}, {}}); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && reflected->properties.size() == 1 && reflected->methods.size() == 1 && budget.used() == members,
                    "denied reflection member insertion publishes no empty property");
            denied = false;
            try { reflected->addMethod({"unadmitted", {}}); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && reflected->methods.size() == 1 &&
                        reflected->properties.at(property.name).getter && reflected->methods.at(method.name).invoker &&
                        budget.used() == members,
                    "denied method insertion preserves callable reflection metadata");
            denied = false;
            try { manager.registerType(id, reflected); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && !manager.getType(id) && budget.used() == members,
                    "denied registry insertion retains the caller-owned reflection descriptor");
            budget.restore();
            require(reflection::any_cast<int>(reflected->properties.at(property.name).getter(nullptr)) == 42 &&
                        reflection::any_cast<int>(reflected->methods.at(method.name).invoker(nullptr, {})) == 7,
                    "reflection callbacks retain callable values after admission returns");
            const auto beforeRegistration = budget.used();
            manager.registerType(id, reflected);
            registryBytes = budget.used() - beforeRegistration;
            require(manager.getType(id) == reflected, "reflection registration retries through TypeManager");
            manager.registerType(id, nullptr);
            require(!manager.getType(id), "reflection registry replacement releases its previous owner");
        }
        // Empty registry nodes and bucket capacity remain physically owned and charged.
        require(budget.used() == beforeReflection + registryBytes,
                "final reflection owner releases wrapper, control, names and tables while registry capacity stays charged");

        struct Observed : SourceBudgetObject
        {
            bool &destroyed;
            explicit Observed(bool &value) : destroyed(value) {}
            ~Observed() { destroyed = true; }
        };
        bool destroyed = false;
        const auto beforeControl = budget.used();
        auto raw = new Observed(destroyed);
        budget.set(budget.used());
        denied = false;
        try { SharedPtr<Observed> owner(raw); } catch(const std::bad_alloc &) { denied = true; }
        require(denied && destroyed && budget.used() == beforeControl,
                "denied SharedPtr control admission destroys its unpublished candidate");
        budget.restore();
        bool originalDestroyed = false;
        SharedPtr<Observed> retained(new Observed(originalDestroyed));
        auto originalOwner = retained.get();
        const auto retainedBytes = budget.used();
        destroyed = false;
        raw = new Observed(destroyed);
        const auto beforeReset = budget.used();
        budget.set(beforeReset);
        denied = false;
        try { retained.reset(raw); } catch(const std::bad_alloc &) { denied = true; }
        require(denied && destroyed && !originalDestroyed && retained.get() == originalOwner &&
                    budget.used() == retainedBytes,
                "denied shared-owner reset preserves the previous descriptor and retires the candidate");
    }

    template<class Fixture> void run(Fixture &fixture)
    {
        strings();
        types(fixture.types);
        std::cout << "PASS: reflection/type tables and physical string-pool capacity budgets\n";
    }
}
