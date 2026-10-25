#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/MaterialProgramReloadDX11.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace workphone::render
{
    namespace
    {
        using Microsoft::WRL::ComPtr;
        StreamBudget admit(size_t bytes)
        {
            String error;
            auto budget = WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,bytes,error);
            if(!budget) throw std::bad_alloc();
            return budget;
        }
        const char *text(const wp_c8 *value) { return reinterpret_cast<const char *>(value); }
        struct SourceKey
        {
            u32 pipeline = 0, definition = 0;
            struct Stage { wp_shader *identity = nullptr; u32 revision = 0; wp_shader_type type{}; };
            std::array<Stage, WORKPHONE_SHADER_TYPE_COUNT> stages;
            explicit SourceKey(wp_shader_program *source)
            {
                pipeline = wp_shader_program_get_pipeline_revision(source);
                definition = wp_shader_program_get_layout_revision(source);
                for(size_t i = 0; i < stages.size(); ++i)
                    if(auto shader = wp_shader_program_get_attached_shader(source,static_cast<wp_s32>(i)))
                        stages[i] = {shader,wp_shader_get_revision(shader),wp_shader_get_type(shader)};
            }
            bool operator==(const SourceKey &other) const
            {
                if(pipeline != other.pipeline || definition != other.definition) return false;
                for(size_t i = 0; i < stages.size(); ++i)
                    if(stages[i].identity != other.stages[i].identity || stages[i].revision != other.stages[i].revision ||
                       stages[i].type != other.stages[i].type) return false;
                return true;
            }
        };
        std::array<u64,2> headroom()
        {
            const auto stats = WorldStreamingScheduler::shared()->stats();
            std::array<u64,2> available{};
            const StreamMemoryClass kinds[] = {StreamMemoryClass::SourceCpu,StreamMemoryClass::Gpu};
            for(size_t i = 0; i < 2; ++i)
            {
                auto k = size_t(kinds[i]);
                available[i] = stats.limits[k] > stats.used[k] ? stats.limits[k]-stats.used[k] : 0;
            }
            return available;
        }
        // No mutable watched shader is attached to this private definition.
        // The lease follows the runtime generation and its retained publications.
        struct Definition
        {
            StreamBudget budget;
            std::unique_ptr<wp_shader_program,decltype(&wp_shader_program_destroy)> program{nullptr,&wp_shader_program_destroy};
            explicit Definition(wp_shader_program *source)
            {
                budget = admit(sizeof(Definition) + wp_shader_program_get_storage_size(source));
                program.reset(wp_shader_program_create());
                if(!program) throw std::bad_alloc();
                for(wp_s32 i = 0; i < wp_shader_program_get_param_count(source); ++i)
                {
                    const auto p = wp_shader_program_get_param_def(source,i);
                    if(wp_shader_program_register_param(program.get(),p->name,p->type,p->array_size,p->variability,
                        p->auto_param,p->resource_kind) != i) throw std::bad_alloc();
                }
                for(wp_s32 i = 0; i < wp_shader_program_get_option_count(source); ++i)
                {
                    const auto option = wp_shader_program_get_option_at(source,i);
                    if(!wp_shader_program_set_option(program.get(),option->name,option->value))
                        throw std::runtime_error("Could not snapshot shader options");
                }
                wp_shader_program_set_permutation_mask(program.get(),wp_shader_program_get_permutation_mask(source));
                if(wp_shader_program_get_storage_size(program.get()) > wp_shader_program_get_storage_size(source))
                    throw std::runtime_error("Shader definition storage estimate changed");
            }
        };
        struct Bytecode
        {
            ComPtr<ID3DBlob> compiled;
            MaterialProgramDX11::Bytecode bytes;
        };
        bool compile(wp_shader *shader, wp_shader_type stage, Bytecode &out, u64 &compiles, String &error)
        {
            if(!shader) return stage == WORKPHONE_SHADER_TYPE_VERTEX;
            if(wp_shader_get_language(shader) == WORKPHONE_SHADER_LANGUAGE_DXBC)
            {
                wp_u32 size = 0;
                out.bytes.data = wp_shader_get_binary(shader,&size); out.bytes.size = size;
                if(out.bytes.data && size) return true;
                error = "DXBC stage has no bytecode"; return false;
            }
            if(wp_shader_get_language(shader) != WORKPHONE_SHADER_LANGUAGE_HLSL)
            { error = "Runtime reload supports HLSL and DXBC stages"; return false; }
            if(wp_shader_get_specialization_count(shader))
            { error = "HLSL specialization constants require an explicit compiler policy"; return false; }
            std::array<D3D_SHADER_MACRO,WP_SHADER_MAX_DEFINES+1> macros{};
            const auto count = wp_shader_get_define_count(shader);
            for(wp_s32 i = 0; i < count; ++i)
            {
                const auto define = wp_shader_get_define_at(shader,i);
                macros[i] = {text(define->name),text(define->value)};
            }
            const auto source = text(wp_shader_get_source(shader));
            auto entry = text(wp_shader_get_entry_point(shader));
            auto profile = text(wp_shader_get_profile(shader));
            if(!*entry) entry = "main";
            if(!*profile) profile = stage == WORKPHONE_SHADER_TYPE_VERTEX ? "vs_5_0" : "ps_5_0";
            ComPtr<ID3DBlob> diagnostic;
            ++compiles;
            const auto result = D3DCompile(source,std::strlen(source),"MaterialProgramReloadDX11",macros.data(),nullptr,
                entry,profile,D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3,0,
                &out.compiled,&diagnostic);
            if(FAILED(result))
            {
                error = diagnostic ? String(static_cast<const char *>(diagnostic->GetBufferPointer()),
                    diagnostic->GetBufferSize()) : String("HLSL compilation failed");
                return false;
            }
            out.bytes = {out.compiled->GetBufferPointer(),out.compiled->GetBufferSize()};
            return true;
        }
    }
    struct MaterialProgramReloadDX11::Binding::Data
    {
        StreamBudget budget = admit(sizeof(Data) + sizeof(Binding));
        Ptr owner;
        Data *previous = nullptr, *next = nullptr;
        bool linked = false;
        MaterialShaderParametersInstance *values = nullptr;
        std::shared_ptr<MaterialProgramResourcesDX11> resources;
        MaterialProgramDX11::Ptr program;
        MaterialProgramDX11::PublicationPtr publication;
        u64 revision = 0;
    };
    struct MaterialProgramReloadDX11::Data
    {
        StreamBudget budget = admit(sizeof(Data) + sizeof(MaterialProgramReloadDX11));
        StreamBudget selectionBudget;
        ComPtr<ID3D11Device> device;
        MaterialShaderParameterLayout::Ptr layout;
        wp_shader_program *source = nullptr;
        u64 deviceGeneration = 0, generation = 0, attempts = 0, compiles = 0;
        MaterialProgramReflectionSelection selections[2];
        bool selected[2]{};
        MaterialProgramDX11::Ptr current;
        Binding::Data *bindings = nullptr;
        SourceKey attempted{nullptr};
        bool hasAttempted = false, force = false, budgetDenied = false, subscriberBlocked = false;
        std::array<u64,2> deniedHeadroom{};
        std::array<char,1024> error{};
        void diagnostic(const char *message)
        {
            std::strncpy(error.data(),message,error.size()-1); error.back() = 0;
        }
        ~Data() { wp_shader_program_destroy(source); }
    };
    MaterialProgramReloadDX11::MaterialProgramReloadDX11() : m_data(new Data) {}
    MaterialProgramReloadDX11::~MaterialProgramReloadDX11() = default;
    MaterialProgramReloadDX11::Binding::Binding() : m_data(new Data) {}
    MaterialProgramReloadDX11::Binding::~Binding()
    {
        auto &binding = *m_data;
        if(!binding.linked) return;
        if(binding.previous) binding.previous->next = binding.next;
        else binding.owner->m_data->bindings = binding.next;
        if(binding.next) binding.next->previous = binding.previous;
    }
    MaterialProgramReloadDX11::Ptr MaterialProgramReloadDX11::Create(void *device,
        MaterialShaderParameterLayout::Ptr layout, wp_shader_program *source, u64 deviceGeneration,
        String &error, const MaterialProgramReflectionSelection *pixelSelection,
        const MaterialProgramReflectionSelection *vertexSelection)
    {
        error.clear();
        if(!device || !layout || !source || !deviceGeneration)
        { error = "Invalid shader reload inputs"; return {}; }
        Ptr result(new MaterialProgramReloadDX11);
        auto &data = *result->m_data;
        data.device = static_cast<ID3D11Device *>(device); data.layout = std::move(layout);
        wp_shader_program_retain(source); data.source = source; data.deviceGeneration = deviceGeneration;
        const MaterialProgramReflectionSelection *selections[] = {vertexSelection,pixelSelection};
        size_t selectionBytes = 0;
        for(size_t s = 0; s < 2; ++s) if(selections[s])
        {
            for(const auto list : {&selections[s]->constantBuffers,&selections[s]->resources})
            {
                selectionBytes += list->capacity()*sizeof(String);
                for(const auto &name : *list) selectionBytes += name.capacity()+1;
            }
        }
        if(selectionBytes) data.selectionBudget = admit(selectionBytes);
        size_t actualBytes = 0;
        for(size_t s = 0; s < 2; ++s) if(selections[s])
        {
            data.selected[s] = true; data.selections[s] = *selections[s];
            for(const auto list : {&data.selections[s].constantBuffers,&data.selections[s].resources})
            {
                actualBytes += list->capacity()*sizeof(String);
                for(const auto &name : *list) actualBytes += name.capacity()+1;
            }
        }
        if(actualBytes > selectionBytes) throw std::length_error("Reflection selection exceeded admitted capacity");
        if(!result->Update()) { error = result->GetLastError(); return {}; }
        return result;
    }
    MaterialProgramReloadDX11::BindingPtr MaterialProgramReloadDX11::Attach(
        MaterialShaderParametersInstance &values, std::shared_ptr<MaterialProgramResourcesDX11> resources)
    {
        if(values.GetLayout() != m_data->layout) throw std::invalid_argument("Reload binding requires its fixed CPU schema");
        Update();
        BindingPtr binding(new Binding);
        auto &data = *binding->m_data;
        data.owner = shared_from_this(); data.values = &values; data.resources = std::move(resources);
        data.program = m_data->current;
        data.publication = data.program->Capture(values,data.resources); data.revision = values.GetRevision();
        data.next = m_data->bindings;
        if(data.next) data.next->previous = &data;
        m_data->bindings = &data; data.linked = true;
        return binding;
    }
    bool MaterialProgramReloadDX11::Update()
    {
        auto &data = *m_data;
        const SourceKey key(data.source);
        if(data.hasAttempted && key == data.attempted && !data.force)
        {
            if(data.subscriberBlocked)
            {
                for(auto binding = data.bindings; binding; binding = binding->next)
                    if(binding->values->GetLayout() != data.layout) return false;
            }
            else
            {
                if(!data.budgetDenied) return !data.error[0];
                const auto available = headroom();
                if(available[0] <= data.deniedHeadroom[0] && available[1] <= data.deniedHeadroom[1]) return false;
            }
        }
        data.force = false; data.hasAttempted = true; data.attempted = key;
        data.budgetDenied = data.subscriberBlocked = false;
        ++data.attempts;
        try
        {
            for(size_t i = 0; i < key.stages.size(); ++i) if(key.stages[i].identity)
                if(i > 1 || key.stages[i].type != wp_shader_type(i))
                { data.diagnostic("Reload supports only matching vertex/pixel stage attachments"); return false; }
            if(!key.stages[1].identity) { data.diagnostic("Reload requires a pixel shader"); return false; }
            auto definition = std::make_shared<Definition>(data.source);
            Bytecode code[2]; String error;
            for(size_t i = 0; i < 2; ++i)
                if(!compile(key.stages[i].identity,wp_shader_type(i),code[i],data.compiles,error))
                { data.diagnostic(error.c_str()); return false; }
            auto candidate = MaterialProgramDX11::Create(data.device.Get(),data.layout,definition->program.get(),
                data.deviceGeneration,code[1].bytes,code[0].bytes,error,
                data.selected[1] ? &data.selections[1] : nullptr,data.selected[0] ? &data.selections[0] : nullptr);
            if(!candidate) { data.diagnostic(error.c_str()); return false; }
            candidate->retainDefinitionOwner(definition);
            struct Pending
            {
                Binding::Data *binding;
                MaterialProgramDX11::PublicationPtr publication;
                u64 revision;
            };
            std::vector<Pending,SourceBudgetAllocator<Pending>> pending;
            for(auto binding = data.bindings; binding; binding = binding->next)
            {
                if(binding->values->GetLayout() != data.layout)
                {
                    data.subscriberBlocked = true;
                    data.diagnostic("Subscribed material changed CPU schema; shader reload was not published"); return false;
                }
                const auto revision = binding->values->GetRevision();
                pending.push_back({binding,candidate->Capture(*binding->values,binding->resources),revision});
            }
            bool changed = !(SourceKey(data.source) == key);
            for(const auto &entry : pending)
                changed = changed || entry.binding->values->GetRevision() != entry.revision ||
                    entry.binding->values->GetLayout() != data.layout;
            if(changed)
            {
                data.diagnostic("Shader or material changed during reload preparation");
                data.force = true; return false;
            }
            if(data.generation == std::numeric_limits<u64>::max())
            { data.diagnostic("Shader reload generation exhausted"); return false; }
            // No allocations, callbacks or observers between these assignments.
            // All subscriptions publish together on the graphics owner thread.
            for(auto &entry : pending)
            {
                entry.binding->program = candidate;
                entry.binding->publication = std::move(entry.publication);
                entry.binding->revision = entry.revision;
            }
            data.current = std::move(candidate); ++data.generation; data.error[0] = 0;
            return true;
        }
        catch(const std::bad_alloc &)
        {
            // Candidate locals have already retired; their own rollback must not
            // count as newly available headroom on the next unchanged poll.
            data.budgetDenied = true; data.deniedHeadroom = headroom();
            data.diagnostic("Shader reload admission denied; last-good generation retained");
        }
        catch(const std::exception &error) { data.diagnostic(error.what()); }
        return false;
    }
    MaterialProgramDX11::PublicationPtr MaterialProgramReloadDX11::Binding::GetPublication()
    {
        auto &data = *m_data;
        data.owner->Update();
        if(data.values->GetLayout() != data.owner->m_data->layout) return {};
        if(data.revision != data.values->GetRevision())
        {
            // Value edits remain normal material publication. On denial retain
            // the cached owner but propagate/defer, never pair old bytes/new state.
            auto candidate = data.program->Capture(*data.values,data.resources);
            data.publication = std::move(candidate); data.revision = data.values->GetRevision();
        }
        return data.publication;
    }
    void MaterialProgramReloadDX11::RequestRetry() { m_data->force = true; }
    u64 MaterialProgramReloadDX11::GetGeneration() const { return m_data->generation; }
    u64 MaterialProgramReloadDX11::GetAttemptCount() const { return m_data->attempts; }
    u64 MaterialProgramReloadDX11::GetCompileCount() const { return m_data->compiles; }
    const char *MaterialProgramReloadDX11::GetLastError() const { return m_data->error.data(); }
}
