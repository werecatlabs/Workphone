#pragma once

#include "ClawMaterialPassLifetimeContracts.hpp"
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <array>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <memory>
#include <stdexcept>

namespace claw_material_retirement_contracts
{
    using namespace workphone;
    using namespace workphone::render;

    inline void require(bool condition, const char *message)
    { if(!condition) throw std::runtime_error(message); }

    // No decoded pixels, device views or resource-manager entry: this resource
    // isolates the lifetime owned by published material snapshots themselves.
    class Texture final : public ClawTexture
    {
    public:
        Texture(u64 &created, u64 &destroyed) : m_destroyed(destroyed) { ++created; }
        ~Texture() override { ++m_destroyed; }
    private:
        u64 &m_destroyed;
    };

    inline bool run(u32 cycles)
    {
        constexpr u32 materialCount = 24, frameCount = 24, viewCount = 4;
        constexpr auto source = size_t(StreamMemoryClass::SourceCpu);
        struct View
        {
            std::array<MaterialRenderState::Ptr, materialCount> states{};
            std::array<ClawMaterialRenderCache::Handle, materialCount> handles{};
            std::array<const ClawPreparedMaterialDX11 *, materialCount> prepared{};
            std::array<f32, materialCount> roughness{};
        };
        try
        {
            claw_material_pass_lifetime_contracts::Fixture services;
            const auto scheduler = WorldStreamingScheduler::shared();
            const auto coldSource = scheduler->stats().used[source];
            const auto contexts = services.states->getStateContexts().size();
            const auto poolBlocks = [] {
                std::array<u64, 2> counts{};
                if(auto types = TypeManager::instance())
                {
                    counts[0] = types->baseObjectDataPool.getSize();
                    counts[1] = types->sharedObjectDataPool.getSize();
                    for(const auto &pool : types->baseObjectDataPools) counts[0] += pool.getSize();
                    for(const auto &pool : types->sharedObjectDataPools) counts[1] += pool.getSize();
                }
                return counts;
            };
            const auto coldPools = poolBlocks();
            const auto coldQueueBytes = MaterialPublicationScope::retainedStorageBytes();
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
            const auto coldStrings = String::character_pool_stats();
#endif
            u64 warmCreated = 0, warmDestroyed = 0;
            {
                // ITexture's type metadata starts with 32 slots. The workload
                // intentionally pins 96 textures at once; returned generic pool
                // capacity belongs to TypeManager, not to one material cycle.
                // Warm that exact peak wrapper population and the actual update
                // queue before measuring all requested owner cycles.
                std::array<SmartPtr<MaterialPassStateData>, materialCount> authored;
                for(auto &state : authored) state = make_ptr<MaterialPassStateData>();
                std::array<SmartPtr<Texture>, materialCount * viewCount> textures;
                for(auto &texture : textures) texture = make_ptr<Texture>(warmCreated, warmDestroyed);
                MaterialPublicationScope update;
                for(u32 item = 0; item < materialCount; ++item)
                {
                    authored[item]->setFloatSetting(MaterialFloatSetting::roughness, 0.25f);
                    authored[item]->setFloatSetting(MaterialFloatSetting::metalness, 0.75f);
                    authored[item]->setTextureBinding(0, textures[item]);
                    authored[item]->publishRenderState();
                }
                update.commit();
            }
            const auto baseline = scheduler->stats().used[source];
            const auto initialPools = poolBlocks();
            const auto queueBytes = MaterialPublicationScope::retainedStorageBytes();
            require(warmCreated == materialCount * viewCount && warmCreated == warmDestroyed &&
                !MaterialPublicationScope::pendingEntryCount() && services.states->getStateContexts().size() == contexts,
                "peak-shape warm-up releases every temporary material, texture and publication owner");
            std::fprintf(stderr,
                "Material retirement warm-up: state_wrappers=%u texture_wrappers=%u SourceCpu=%llu->%llu "
                "base_pool_blocks=%llu->%llu shared_pool_blocks=%llu->%llu publication_queue_bytes=%llu->%llu "
                "warm_textures_created=%llu destroyed=%llu\n",
                materialCount, materialCount * viewCount, static_cast<unsigned long long>(coldSource),
                static_cast<unsigned long long>(baseline), static_cast<unsigned long long>(coldPools[0]),
                static_cast<unsigned long long>(initialPools[0]), static_cast<unsigned long long>(coldPools[1]),
                static_cast<unsigned long long>(initialPools[1]), static_cast<unsigned long long>(coldQueueBytes),
                static_cast<unsigned long long>(queueBytes), static_cast<unsigned long long>(warmCreated),
                static_cast<unsigned long long>(warmDestroyed));
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
            const auto initialStrings = String::character_pool_stats();
            std::fprintf(stderr,
                "Material retirement warm-up character pools: committed=%llu->%llu slabs=%llu->%llu live=%llu->%llu free=%llu->%llu\n",
                static_cast<unsigned long long>(coldStrings.committedBytes), static_cast<unsigned long long>(initialStrings.committedBytes),
                static_cast<unsigned long long>(coldStrings.slabCount), static_cast<unsigned long long>(initialStrings.slabCount),
                static_cast<unsigned long long>(coldStrings.liveBlocks), static_cast<unsigned long long>(initialStrings.liveBlocks),
                static_cast<unsigned long long>(coldStrings.freeBlocks), static_cast<unsigned long long>(initialStrings.freeBlocks));
#endif
            u64 created = 0, destroyed = 0, reused = 0, peak = baseline, peakTable = 0;
            u32 maxSlots = 0;
            const auto started = std::chrono::steady_clock::now();
            for(u32 cycle = 0; cycle < cycles; ++cycle)
            {
                {
                    auto cache = std::make_unique<ClawMaterialRenderCache>();
                    std::array<SmartPtr<MaterialPassStateData>, materialCount> authored;
                    for(auto &state : authored) state = make_ptr<MaterialPassStateData>();
                    std::array<View, viewCount> views{};
                    std::array<u64, materialCount * frameCount> generations{};
                    std::weak_ptr<const MaterialRenderState> oldest;
                    ClawMaterialRenderCache::Handle oldestHandle;
                    for(u32 frame = 0; frame < frameCount; ++frame)
                    {
                        auto &view = views[frame % viewCount];
                        view = {};
                        std::array<MaterialRenderState::Ptr, materialCount> before;
                        for(u32 item = 0; item < materialCount; ++item)
                            before[item] = authored[item]->getRenderState();
                        {
                            MaterialPublicationScope update;
                            for(u32 item = 0; item < materialCount; ++item)
                            {
                                const auto roughness = f32(1 + frame * materialCount + item) /
                                    f32(1 + materialCount * frameCount);
                                require(authored[item]->setFloatSetting(MaterialFloatSetting::roughness, roughness) &&
                                    authored[item]->setFloatSetting(MaterialFloatSetting::metalness, 1.0f - roughness),
                                    "typed edited-frame values accept their exact setting kinds");
                                authored[item]->setTextureBinding(0, make_ptr<Texture>(created, destroyed));
                                authored[item]->publishRenderState();
                                require(authored[item]->getRenderState() == before[item],
                                    "multiple edits stay private until the owner update commits");
                                view.roughness[item] = roughness;
                            }
                            update.commit();
                        }
                        require(!MaterialPublicationScope::pendingEntryCount(),
                            "edited update releases every temporary publication-queue owner");
                        for(u32 item = 0; item < materialCount; ++item)
                        {
                            view.states[item] = authored[item]->getRenderState();
                            require(view.states[item] != before[item] &&
                                view.states[item]->revision == before[item]->revision + 1 &&
                                !authored[item]->getPendingSurfaceStorageBytes(),
                                "one update publishes exactly one new immutable generation per edited material");
                            view.handles[item] = cache->resolve(view.states[item]);
                            view.prepared[item] = cache->lookup(view.handles[item]);
                            const auto handle = view.handles[item];
                            require(handle.slot < generations.size() && view.prepared[item],
                                "persistent edited records remain inside the bounded workload table");
                            if(generations[handle.slot] && generations[handle.slot] != handle.generation)
                            {
                                require(handle.generation > generations[handle.slot],
                                    "retired table slots advance their non-reused generation");
                                ++reused;
                            }
                            generations[handle.slot] = handle.generation;
                        }
                        before = {};
                        const auto preparations = cache->getPreparationCount();
                        const auto allocations = cache->getStorageAllocationCount();
                        // The same publications are consumed by three independent
                        // views while up to four earlier frame generations remain.
                        for(u32 consumer = 0; consumer < 3; ++consumer)
                            for(const auto &retained : views)
                                for(u32 item = 0; item < materialCount; ++item)
                                    if(retained.states[item])
                                    {
                                        const auto handle = cache->resolve(retained.states[item]);
                                        require(handle.table == retained.handles[item].table &&
                                            handle.slot == retained.handles[item].slot &&
                                            handle.generation == retained.handles[item].generation &&
                                            cache->lookup(handle) == retained.prepared[item] &&
                                            retained.prepared[item]->constants.surface.y == retained.roughness[item],
                                            "retained frame/view generations preserve exact values and stable record addresses");
                                    }
                        require(cache->getPreparationCount() == preparations &&
                            cache->getStorageAllocationCount() == allocations,
                            "unchanged retained-view consumption rebuilds and allocates no record storage");
                        cache->maintain(8);
                        if(frame == 0)
                        { oldest = view.states[0]; oldestHandle = view.handles[0]; }
                        if(frame >= viewCount)
                            require(oldest.expired() && !cache->lookup(oldestHandle),
                                "a dropped old view cannot resolve its stale record generation");
                        peak = std::max(peak, scheduler->stats().used[source]);
                        peakTable = std::max(peakTable, cache->getStorageBytes());
                        maxSlots = std::max(maxSlots, cache->getSlotCount());
                    }
                    require(cache->getPreparationCount() == u64(materialCount) * frameCount,
                        "every edited frame is prepared once despite repeated retained-view reads");
                    authored = {};
                    require(created - destroyed == u64(materialCount) * viewCount,
                        "only the four intentionally retained frame generations keep texture owners alive");
                    // Complete ordinary budgeted maintenance, rather than clearing
                    // pools, before dropping the cache owner. The lease is the
                    // same table-lifetime mechanism used by scene draw records.
                    const auto sweeps = (cache->getSlotCount() + 7) / 8 +
                        (ClawMaterialRenderCache::Capacity + 7) / 8;
                    for(u32 frame = 0; frame < sweeps; ++frame) cache->maintain(8);
                    require(cache->getResidentCount() == materialCount * viewCount,
                        "idle maintenance retires all unpinned edited generations");
                    auto lease = cache->retain();
                    cache.reset();
                    for(const auto &view : views)
                        for(u32 item = 0; item < materialCount; ++item)
                            require(lease.get(view.handles[item]) == view.prepared[item] &&
                                lease.get(view.handles[item])->constants.surface.y == view.roughness[item],
                                "retained views survive renderer-table owner destruction");
                    const auto finalHandle = views.back().handles.back();
                    views = {};
                    require(created == destroyed && !lease.get(finalHandle) && lease.getStorageBytes() &&
                        scheduler->stats().used[source] > baseline,
                        "released views destroy every texture while the explicit table lease stays charged");
                    lease = {};
                }
                if(scheduler->stats().used[source] != baseline ||
                    services.states->getStateContexts().size() != contexts ||
                    MaterialPublicationScope::pendingEntryCount() || created != destroyed ||
                    MaterialPublicationScope::retainedStorageBytes() != queueBytes)
                {
                    const auto afterPools = poolBlocks();
                    std::fprintf(stderr,
                        "Material retirement boundary: cycle=%u SourceCpu=%llu->%llu contexts=%llu->%llu "
                        "pending_publications=%llu publication_queue_bytes=%llu->%llu textures_created=%llu destroyed=%llu "
                        "base_pool_blocks=%llu->%llu shared_pool_blocks=%llu->%llu\n",
                        cycle + 1, static_cast<unsigned long long>(baseline),
                        static_cast<unsigned long long>(scheduler->stats().used[source]),
                        static_cast<unsigned long long>(contexts),
                        static_cast<unsigned long long>(services.states->getStateContexts().size()),
                        static_cast<unsigned long long>(MaterialPublicationScope::pendingEntryCount()),
                        static_cast<unsigned long long>(queueBytes),
                        static_cast<unsigned long long>(MaterialPublicationScope::retainedStorageBytes()),
                        static_cast<unsigned long long>(created), static_cast<unsigned long long>(destroyed),
                        static_cast<unsigned long long>(initialPools[0]), static_cast<unsigned long long>(afterPools[0]),
                        static_cast<unsigned long long>(initialPools[1]), static_cast<unsigned long long>(afterPools[1]));
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
                    const auto strings = String::character_pool_stats();
                    std::fprintf(stderr,
                        "Material retirement character pools: committed=%llu->%llu slabs=%llu->%llu live=%llu->%llu free=%llu->%llu\n",
                        static_cast<unsigned long long>(initialStrings.committedBytes), static_cast<unsigned long long>(strings.committedBytes),
                        static_cast<unsigned long long>(initialStrings.slabCount), static_cast<unsigned long long>(strings.slabCount),
                        static_cast<unsigned long long>(initialStrings.liveBlocks), static_cast<unsigned long long>(strings.liveBlocks),
                        static_cast<unsigned long long>(initialStrings.freeBlocks), static_cast<unsigned long long>(strings.freeBlocks));
#endif
                }
                require(scheduler->stats().used[source] == baseline &&
                    services.states->getStateContexts().size() == contexts &&
                    !MaterialPublicationScope::pendingEntryCount() && created == destroyed &&
                    MaterialPublicationScope::retainedStorageBytes() == queueBytes,
                    "final table and material owners return exactly to the SourceCpu and context baseline");
                if((cycle + 1) % 128 == 0)
                    std::fprintf(stderr, "Material retirement checkpoint: cycles=%u source_cpu=%llu textures_live=0\n",
                        cycle + 1, static_cast<unsigned long long>(baseline));
            }
            require(poolBlocks() == initialPools,
                "measured owner cycles do not increase retained TypeManager metadata pool blocks");
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
            const auto finalStrings = String::character_pool_stats();
            require(finalStrings.committedBytes == initialStrings.committedBytes &&
                finalStrings.slabCount == initialStrings.slabCount &&
                finalStrings.liveBlocks == initialStrings.liveBlocks && finalStrings.freeBlocks == initialStrings.freeBlocks,
                "measured owner cycles return all string blocks without growing shared character capacity");
#endif
            require(reused != 0, "the soak actually exercises recycled persistent record generations");
            const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
            std::fprintf(stderr,
                "Material retirement soak passed: cycles=%u edited_frames=%llu material_publications=%llu "
                "retained_frame_generations=%u view_consumers=3 textures_created=%llu textures_destroyed=%llu "
                "recycled_generations=%llu source_cpu_baseline=%llu source_cpu_frame_end_peak=%llu source_cpu_final=%llu "
                "table_peak_bytes=%llu max_slots=%u elapsed_seconds=%.6f\n",
                cycles, static_cast<unsigned long long>(u64(cycles) * frameCount),
                static_cast<unsigned long long>(u64(cycles) * frameCount * materialCount), viewCount,
                static_cast<unsigned long long>(created), static_cast<unsigned long long>(destroyed),
                static_cast<unsigned long long>(reused), static_cast<unsigned long long>(baseline),
                static_cast<unsigned long long>(peak), static_cast<unsigned long long>(scheduler->stats().used[source]),
                static_cast<unsigned long long>(peakTable), maxSlots, seconds);
            return true;
        }
        catch(const std::exception &error)
        {
            std::fprintf(stderr, "Material retirement soak failed: %s\n", error.what());
            return false;
        }
    }
}
