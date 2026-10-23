#pragma once
#include "GenericEngineCatalogBudgetContracts.hpp"
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/Mesh.hpp>

namespace snapshot_container_budget_contracts
{
    using namespace workphone;
    using generic_engine_catalog_budget_contracts::Capacity;
    using generic_engine_catalog_budget_contracts::require;

    inline void arrays()
    {
        Capacity budget;
        const auto baseline = budget.used();
        Array<int> external{7, 8};
        require(budget.used() == baseline && !external.get_allocator().isSourceBudgeted(),
                "ordinary caller arrays retain their explicit external allocation policy");
        Allocator<int> source(sourceBudget);
        std::allocator_traits<Allocator<int>>::rebind_alloc<u64> rebound(source);
        auto reboundStorage = rebound.allocate(3);
        require(rebound.isSourceBudgeted() && budget.used() == baseline + sourceBudgetStorageBytes(3 * sizeof(u64), alignof(u64)),
                "allocator rebinding preserves admitted storage policy and physical size");
        rebound.deallocate(reboundStorage, 3);
        {
            auto values = makeSourceBudgetSnapshot<int>();
            values.reserve(128);
            values.push_back(42);
            const auto resident = budget.used();
            require(resident == baseline + sourceBudgetStorageBytes(128 * sizeof(int), alignof(int)),
                    "snapshot spare capacity and physical header are admitted exactly once");
            auto address = values.data();
            budget.set(resident);
            bool denied = false;
            try { values.reserve(512); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && values.data() == address && values[0] == 42 && budget.used() == resident,
                    "denied snapshot growth preserves the old storage and value");
            denied = false;
            try { external = values; } catch(const std::bad_alloc &) { denied = true; }
            require(denied && external.size() == 2 && external[0] == 7 && budget.used() == resident,
                    "denied caller copy assignment preserves the external destination");
            denied = false;
            try { auto unexpected = values.snapshot(); } catch(const std::bad_alloc &) { denied = true; }
            require(denied, "std vector snapshot exports also require physical admission");
            Array<int> moved = std::move(values);
            moved.swap(external);
            require(external.data() == address && external.get_allocator().isSourceBudgeted() &&
                        !moved.get_allocator().isSourceBudgeted() && moved[0] == 7,
                    "cross-policy moves and swaps transfer storage with its deallocator at capacity");
            external.clear();
            require(budget.used() == resident, "empty snapshot spare capacity stays charged");
            external.push_back(99);
            budget.restore();
            {
                auto copy = external;
                auto exported = external.snapshot();
                decltype(exported) assigned;
                assigned = exported;
                require(copy.capacity() == external.capacity() && copy[0] == 99 &&
                            copy.get_allocator().isSourceBudgeted() &&
                            exported.get_allocator().isSourceBudgeted() && assigned.get_allocator().isSourceBudgeted(),
                        "array copies and std vector copy assignment preserve SourceCpu policy");
                const auto held = budget.used();
                budget.set(held);
                denied = false;
                try { assigned.reserve(1024); } catch(const std::bad_alloc &) { denied = true; }
                require(denied && assigned[0] == 99, "retained caller vector growth cannot escape admission");
                budget.restore();
            }
            require(budget.used() == resident, "independent snapshot copies release their own reservations");
            external = Array<int>{};
            require(budget.used() == baseline, "last detached snapshot physically releases its capacity");
        }
        auto nested = makeSourceBudgetSnapshot<Array<int>>();
        nested.push_back(makeSourceBudgetSnapshot<int>());
        nested[0].reserve(64);
        nested[0].push_back(11);
        const auto resident = budget.used();
        budget.set(resident + sourceBudgetStorageBytes(nested.capacity() * sizeof(Array<int>), alignof(Array<int>)));
        bool denied = false;
        try { auto unexpected = nested; } catch(const std::bad_alloc &) { denied = true; }
        require(denied && budget.used() == resident && nested[0][0] == 11,
                "nested admitted snapshot copy unwinds the outer buffer when inner admission fails");
        budget.restore();
        SourceBudgetConcurrentArray<int> inventory;
        inventory.push_back(73);
        auto retained = inventory.snapshot();
        inventory.releaseStorage();
        require(retained[0] == 73 && retained.get_allocator().isSourceBudgeted(),
                "public inventory snapshot retains admitted storage after its producer releases it");
    }

    inline void properties()
    {
        Capacity budget;
        Properties properties;
        properties.setProperty("old", "kept");
        properties.getPropertyObject("old").setAttribute("resourceType", "Texture");
        auto child = make_ptr<Properties>();
        properties.addChild(child);
        auto fields = properties.getPropertiesAsArray();
        auto children = properties.getChildren();
        auto attributes = fields[0].getAttributes();
        require(fields.get_allocator().isSourceBudgeted() && children.get_allocator().isSourceBudgeted() &&
                    attributes.get_allocator().isSourceBudgeted(),
                "property fields, children and attribute snapshots carry their physical allocation policy");
        Property target("old", "value");
        target.setAttribute("type", "old");
        Property replacement("new", "candidate");
        replacement.setAttribute("type", "new");
        budget.set(budget.used());
        bool propertyDenied = false;
        try { target = replacement; } catch(const std::bad_alloc &) { propertyDenied = true; }
        require(propertyDenied && target.getName() == "old" && target.getValue() == "value" &&
                    target.getAttribute("type") == "old",
                "denied property attribute copy preserves the old name, value and metadata together");
        budget.restore();
        Array<Property> external;
        external.reserve(50);
        external.emplace_back("replacement", "candidate");
        const auto resident = budget.used();
        budget.set(resident);
        bool denied = false;
        try { properties.setPropertiesAsArray(external); } catch(const std::bad_alloc &) { denied = true; }
        require(denied && properties.getNumProperties() == 1 && properties.hasProperty("old") &&
                    fields[0].getValue() == "kept" && budget.used() == resident,
                "denied caller property-container import preserves the live document and old snapshot");
        budget.restore();
        properties.setPropertiesAsArray(external);
        auto imported = properties.getPropertiesAsArray();
        require(imported.capacity() == 50 && imported.get_allocator().isSourceBudgeted() &&
                    imported[0].getName() == "replacement" && fields[0].getName() == "old",
                "caller property imports admit spare capacity and detach from previous snapshots");
        Properties replacementTree;
        Array<Property> replacementFields;
        replacementFields.emplace_back("new", "tree");
        replacementTree.setPropertiesAsArray(replacementFields);
        replacementTree.addChild(child);
        const auto shape = replacementTree.getPropertiesAsArray();
        const auto beforeTree = budget.used();
        budget.set(beforeTree + sourceBudgetStorageBytes(shape.capacity() * sizeof(Property), alignof(Property)));
        denied = false;
        try { properties = replacementTree; } catch(const std::bad_alloc &) { denied = true; }
        require(denied && budget.used() == beforeTree && properties.hasProperty("replacement") &&
                    !properties.hasProperty("new") && properties.getNumChildren() == 1,
                "child-array admission failure unwinds the staged field table without partially replacing the document");
        budget.restore();
        auto binary = PropertiesBinarySerializer::serialize(properties);
        require(binary.get_allocator().isSourceBudgeted(),
                "caller-owned binary document snapshots retain physical SourceCpu admission");
        budget.set(budget.used());
        denied = false;
        try { auto candidate = PropertiesBinarySerializer::serialize(properties); }
        catch(const std::bad_alloc &) { denied = true; }
        require(denied && properties.hasProperty("replacement") && !binary.empty(),
                "denied binary document serialization preserves the document and prior byte snapshot");
        budget.restore();
        const auto held = budget.used();
        budget.set(held);
        children = {};
        attributes = {};
        fields = {};
        require(budget.used() < held, "detached property snapshots release at a full budget");
    }

    inline void run()
    {
        arrays(); properties();
        {
            Capacity budget;
            MeshManager manager;
            auto mesh = make_ptr<Mesh>();
            mesh->setLoadingState(LoadingState::Loaded);
            manager.addMesh(mesh);
            const auto resident = budget.used();
            budget.set(resident);
            manager.unload(nullptr);
            require(!manager.isLoaded() && !mesh->isLoaded() && budget.used() < resident,
                    "mesh-manager teardown transfers its existing inventory at full admission capacity");
        }
        std::cout << "PASS: retained caller container and property/document snapshot budgets\n";
    }
}
