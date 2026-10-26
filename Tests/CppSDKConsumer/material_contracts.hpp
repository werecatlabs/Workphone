#pragma once

#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/Graphics/MaterialShader.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <stdexcept>

namespace installed_material_contracts
{
    inline void require(bool value, const char *message)
    {
        if(!value) throw std::runtime_error(message);
    }

    // Exercise the installed DLL interface and its inline/header ABI together.
    // These contracts require neither an application manager nor a GPU device.
    inline void run()
    {
        using namespace workphone;
        using namespace workphone::render;
        using Type = MaterialShaderParameterType;

        MaterialShader schema("InstalledMaterialContract");
        schema.addParameter(Type::Scalar, "Gain")
              .addParameter(Type::Int, "Count")
              .addParameter(Type::Texture, "Map");
        MaterialShaderParametersInstance values, defaults;
        schema.buildInstance(values);
        schema.buildInstance(defaults);
        require(values.GetLayout() == defaults.GetLayout() &&
                !values.GetValueStorageIdentity() && !defaults.GetValueStorageIdentity(),
                "Installed material instances must share immutable layout defaults");
        const auto gain = values.Resolve<f32>("Gain");
        const auto count = values.Resolve<s32>("Count");
        const auto map = values.FindParameter("Map");
        require(gain.IsValid() && count.IsValid() && map.IsValid() &&
                values.Set(gain, .375f) && values.Set(count, s32(-17)) &&
                values.SetTexture(map, MaterialResourceHandle(37)),
                "Installed typed setters and resource slots must accept their exact kinds");
        const auto lookups = values.GetNameLookupCount();
        const auto revision = values.GetRevision();
        require(!values.SetScalar(gain.handle, s32(4)) && values.GetRevision() == revision,
                "Equal-width wrong-kind writes must reject in release consumers");
        auto retained = values;
        require(retained.GetLayout() == values.GetLayout() &&
                retained.GetValueStorageIdentity() == values.GetValueStorageIdentity(),
                "Copied installed instances must share their immutable value storage");
        require(values.Set(gain, .75f) && retained.GetScalar(gain.handle) == .375f &&
                values.GetValueStorageIdentity() != retained.GetValueStorageIdentity() &&
                values.GetNameLookupCount() == lookups,
                "Cached typed writes must detach values without another name lookup");

        const auto next = MaterialShaderParameterLayout::Create({
            {Type::Int, "Count"}, {Type::UInt, "Gain"}, {Type::Texture, "Map"},
            {Type::Bool, "Enabled"}});
        auto migration = values.PrepareMigration(next);
        require(migration && migration->IsCurrent() &&
                migration->Values().GetInt(next->FindParameter("Count")) == -17 &&
                migration->Values().GetUInt(next->FindParameter("Gain")) == 0 &&
                migration->Values().GetResource(next->FindParameter("Map")) == 37 &&
                migration->Commit(),
                "Installed schema migration must preserve exact kinds and default a changed kind");
        require(values.GetLayout() == next && !values.Set(gain, 1.0f) &&
                !values.Set(count, s32(9)) && retained.GetScalar(gain.handle) == .375f &&
                values.Set(values.Resolve<bool>("Enabled"), true),
                "New layout generations must reject old handles while retained values remain readable");
        migration.reset();

        auto state = make_ptr<MaterialPassStateData>();
        auto before = state->getRenderState();
        require(before && MaterialPassStateData::resolveFloatSetting("roughness") ==
                MaterialFloatSetting::roughness,
                "Installed named settings must resolve to the numeric schema");
        {
            MaterialPublicationScope update;
            require(state->setFloatSetting(MaterialFloatSetting::roughness, .625f) &&
                    state->setFloatSetting(MaterialFloatSetting::metalness, .25f),
                    "Installed numeric material controls must accept supported settings");
            {
                MaterialPublicationScope nested;
                require(state->setUIntSetting(MaterialUIntSetting::uvWrapU, 2),
                        "Installed nested update accepts sampler edits");
            }
            require(state->getRenderState() == before &&
                    MaterialPublicationScope::pendingEntryCount() == 1,
                    "Nested installed update scopes must defer one immutable publication");
        }
        auto current = state->getRenderState();
        require(current != before && current->revision == before->revision + 1 &&
                current->roughness == .625f && current->metalness == .25f &&
                current->uvWrapU == 2 && before->roughness == .5f &&
                !MaterialPublicationScope::pendingEntryCount(),
                "Installed update exit must publish one coherent immutable version");
        require(state->setEditorFloat("roughness", .625f) &&
                state->getRenderState() == current &&
                !state->setFloatSetting(MaterialFloatSetting::Count, .9f),
                "Installed compatibility adapters preserve no-op and invalid-setting behavior");

        ClawMaterialRenderCache cache, foreign;
        const auto handle = cache.resolve(current);
        const auto *prepared = cache.lookup(handle);
        require(prepared && prepared->constants.surface.x == .25f &&
                prepared->constants.surface.y == .625f && !foreign.lookup(handle),
                "Installed backend records must validate table identity and preserve typed values");
        const auto allocations = cache.getStorageAllocationCount();
        for(u32 frame = 0; frame != 32; ++frame)
        {
            const auto warm = cache.resolve(current);
            require(warm.table == handle.table && warm.slot == handle.slot &&
                    warm.generation == handle.generation && cache.lookup(warm) == prepared,
                    "Installed record handles must remain stable across warm frames");
        }
        require(cache.getPreparationCount() == 1 &&
                cache.getStorageAllocationCount() == allocations,
                "Warm installed record access must perform no preparation or storage allocation");
        auto lease = cache.retain();
        cache.clear();
        require(!cache.lookup(handle) && lease.get(handle) == prepared,
                "Installed cache teardown must retain in-flight record generations");
        const auto newEpoch = cache.resolve(current);
        require(newEpoch.table != handle.table && !cache.lookup(handle),
                "Installed cache recreation must not revive a retired handle");
        before.reset();
        current.reset();
        state = nullptr;
        cache.collectExpired();
        require(!cache.lookup(newEpoch) && !lease.get(handle),
                "Installed cache leases must not retain retired material owners");
    }
}
