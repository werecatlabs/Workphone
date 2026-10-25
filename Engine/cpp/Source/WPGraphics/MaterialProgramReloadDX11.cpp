#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/MaterialProgramReloadDX11.hpp>
#include "MaterialShaderFilesDX11.hpp"
#include <Workphone/Core/Properties.hpp>
#include <mutex>
#include <atomic>
#include <d3d11shader.h>
#include <cassert>
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
        thread_local size_t reloadCaptureDepth = 0;
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
            u64 files=0, schema=0;
            struct Stage { wp_shader *identity = nullptr; u32 revision = 0; wp_shader_type type{}; };
            std::array<Stage, WORKPHONE_SHADER_TYPE_COUNT> stages;
            explicit SourceKey(wp_shader_program *source,u64 fileRevision=0,u64 schemaRevision=0) : files(fileRevision),schema(schemaRevision)
            {
                pipeline = wp_shader_program_get_pipeline_revision(source);
                definition = wp_shader_program_get_layout_revision(source);
                for(size_t i = 0; i < stages.size(); ++i)
                    if(auto shader = wp_shader_program_get_attached_shader(source,static_cast<wp_s32>(i)))
                        stages[i] = {shader,wp_shader_get_revision(shader),wp_shader_get_type(shader)};
            }
            bool operator==(const SourceKey &other) const
            {
                if(pipeline != other.pipeline || definition != other.definition || files!=other.files || schema!=other.schema) return false;
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
            explicit Definition(wp_shader_program *source,MaterialShaderParameterLayout::Ptr schema={})
            {
                budget = admit(sizeof(Definition) + wp_shader_program_get_storage_size(source) + (schema ? WP_SHADER_MAX_PARAMS*80u : 0));
                program.reset(wp_shader_program_create());
                if(!program) throw std::bad_alloc();
                if(schema) for(const auto &p:schema->GetParameterInfo())
                {
                    using T=MaterialShaderParameterType;wp_uniform_type type=WORKPHONE_UNIFORM_TYPE_INT;
                    wp_shader_resource_kind resource=WORKPHONE_SHADER_RESOURCE_NONE;
                    switch(p.m_type)
                    {
                    case T::Scalar:type=WORKPHONE_UNIFORM_TYPE_FLOAT;break;case T::Colour:case T::Vector4:type=WORKPHONE_UNIFORM_TYPE_VEC4;break;
                    case T::Vector2:type=WORKPHONE_UNIFORM_TYPE_VEC2;break;case T::Matrix:type=WORKPHONE_UNIFORM_TYPE_MAT4;break;
                    case T::UInt:type=WORKPHONE_UNIFORM_TYPE_UINT;break;case T::Bool:type=WORKPHONE_UNIFORM_TYPE_BOOL;break;
                    case T::IntVector2:type=WORKPHONE_UNIFORM_TYPE_IVEC2;break;case T::IntVector4:type=WORKPHONE_UNIFORM_TYPE_IVEC4;break;
                    case T::Texture:resource=WORKPHONE_SHADER_RESOURCE_TEXTURE;break;case T::Buffer:resource=WORKPHONE_SHADER_RESOURCE_BUFFER;break;
                    case T::Sampler:resource=WORKPHONE_SHADER_RESOURCE_SAMPLER;break;default:break;
                    }
                    if(wp_shader_program_register_param(program.get(),reinterpret_cast<const wp_c8 *>(p.m_name.c_str()),type,1,
                        WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,WORKPHONE_SHADER_AUTO_PARAM_NONE,resource)<0)
                        throw std::runtime_error("Shader schema exceeds native parameter capacity");
                }
                if(!schema)
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
                if(wp_shader_program_get_storage_size(program.get()) > wp_shader_program_get_storage_size(source)+(schema?WP_SHADER_MAX_PARAMS*80u:0))
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
    namespace
    {
        MaterialShaderParameterLayout::Ptr fileSchema(const Bytecode (&code)[2],MaterialShaderParameterLayout::Ptr existing,
            MaterialProgramReflectionSelection (&selections)[2])
        {
            using T=MaterialShaderParameterType;
            Array<MaterialShaderParameterInfo> info;
            const auto add=[&](const char *name,T type) {
                const auto old=existing->FindParameter(name);
                if(type==T::Vector4 && (!std::strcmp(name,"Albedo") || !std::strcmp(name,"Emissive") || !std::strcmp(name,"Color"))) type=T::Colour;
                if(type==T::Vector4 && old.IsValid() && existing->GetParameterInfo()[old.m_parameterIndex].m_type==T::Colour) type=T::Colour;
                for(auto &p:info) if(p.m_name==name)
                {
                    if(p.m_type!=type) throw std::runtime_error("Shader stages disagree on a parameter type");
                    return;
                }
                if(info.size()>=WP_SHADER_MAX_PARAMS) throw std::runtime_error("Reflected shader exceeds CPU parameter capacity");
                info.push_back({type,name});
            };
            for(size_t stage=0;stage<2;++stage) if(code[stage].bytes.data)
            {
                Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;
                if(FAILED(D3DReflect(code[stage].bytes.data,code[stage].bytes.size,IID_ID3D11ShaderReflection,
                    reinterpret_cast<void **>(reflection.GetAddressOf())))) throw std::runtime_error("Shader file reflection failed");
                D3D11_SHADER_DESC desc{};reflection->GetDesc(&desc);
                for(UINT i=0;i<desc.BoundResources;++i)
                {
                    D3D11_SHADER_INPUT_BIND_DESC input{};reflection->GetResourceBindingDesc(i,&input);
                    if(input.Type==D3D_SIT_CBUFFER)
                    {
                        if(input.BindPoint==0) continue;
                        selections[stage].constantBuffers.push_back(input.Name);
                        auto buffer=reflection->GetConstantBufferByName(input.Name);D3D11_SHADER_BUFFER_DESC b{};buffer->GetDesc(&b);
                        for(UINT v=0;v<b.Variables;++v)
                        {
                            auto variable=buffer->GetVariableByIndex(v);D3D11_SHADER_VARIABLE_DESC value{};variable->GetDesc(&value);
                            D3D11_SHADER_TYPE_DESC t{};variable->GetType()->GetDesc(&t);
                            if(t.Elements || t.Members) throw std::runtime_error("Array/struct file shader parameters are unsupported");
                            T type;
                            if((t.Class==D3D_SVC_MATRIX_COLUMNS || t.Class==D3D_SVC_MATRIX_ROWS) && t.Type==D3D_SVT_FLOAT && t.Rows==4 && t.Columns==4) type=T::Matrix;
                            else if(t.Class==D3D_SVC_SCALAR && t.Type==D3D_SVT_FLOAT) type=T::Scalar;
                            else if(t.Class==D3D_SVC_SCALAR && t.Type==D3D_SVT_INT) type=T::Int;
                            else if(t.Class==D3D_SVC_SCALAR && t.Type==D3D_SVT_UINT) type=T::UInt;
                            else if(t.Class==D3D_SVC_SCALAR && t.Type==D3D_SVT_BOOL) type=T::Bool;
                            else if(t.Class==D3D_SVC_VECTOR && t.Type==D3D_SVT_FLOAT && t.Columns==2) type=T::Vector2;
                            else if(t.Class==D3D_SVC_VECTOR && t.Type==D3D_SVT_FLOAT && t.Columns==4) type=T::Vector4;
                            else if(t.Class==D3D_SVC_VECTOR && t.Type==D3D_SVT_INT && t.Columns==2) type=T::IntVector2;
                            else if(t.Class==D3D_SVC_VECTOR && t.Type==D3D_SVT_INT && t.Columns==4) type=T::IntVector4;
                            else throw std::runtime_error("Unsupported reflected shader parameter type");
                            add(value.Name,type);
                        }
                    }
                    else
                    {
                        T type;
                        if(input.Type==D3D_SIT_SAMPLER) type=T::Sampler;
                        else if(input.Type==D3D_SIT_TEXTURE) type=input.Dimension==D3D_SRV_DIMENSION_BUFFER?T::Buffer:T::Texture;
                        else if(input.Type==D3D_SIT_TBUFFER || input.Type==D3D_SIT_STRUCTURED || input.Type==D3D_SIT_BYTEADDRESS) type=T::Buffer;
                        else throw std::runtime_error("Unsupported file shader resource kind");
                        selections[stage].resources.push_back(input.Name);add(input.Name,type);
                    }
                }
            }
            const auto &old=existing->GetParameterInfo();
            Array<MaterialShaderParameterInfo> ordered;
            for(const auto &p:old) for(const auto &next:info) if(p.m_name==next.m_name && p.m_type==next.m_type) ordered.push_back(next);
            for(const auto &next:info)
            {
                bool found=false;for(const auto &p:ordered) if(p.m_name==next.m_name) found=true;
                if(!found) ordered.push_back(next);
            }
            info=std::move(ordered);bool same=old.size()==info.size();
            for(size_t i=0;same && i<old.size();++i) same=old[i].m_name==info[i].m_name && old[i].m_type==info[i].m_type;
            return same?existing:MaterialShaderParameterLayout::Create(info);
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
        MaterialShaderParameterLayout::Ptr layout, requestedLayout, baseLayout;
        u64 schemaRevision=0;
        std::unique_ptr<detail::ShaderFilesDX11> files;
        bool watching=true, autoSchema=false, explicitSchema=false;
        void *deviceOwner=nullptr;
        std::atomic<bool> reloadCommand{false};
        std::atomic<int> watchCommand{-1};
        static std::mutex registryMutex;
        static Data *head;
        static u64 nextId;
        Data *previous=nullptr,*next=nullptr;
        MaterialProgramReloadDX11 *owner=nullptr;
        u64 id=0;
        struct Status { u64 generation=0,attempts=0,compiles=0; bool success=false,watching=true,program=false;
            SourceBudgetString vertex,pixel; std::array<char,1024> error{}; } status;
        void publish()
        {
            std::lock_guard<std::mutex> lock(registryMutex);
            status.generation=generation;status.attempts=attempts;status.compiles=compiles;
            status.success=!error[0] && owner->HasProgram(); status.program=owner->HasProgram();status.watching=watching;
            status.error=error;
        }
        StreamBudget ownedSourceBudget;
        wp_shader_program *source = nullptr;
        u64 deviceGeneration = 0, generation = 0, attempts = 0, compiles = 0;
        MaterialProgramReflectionSelection selections[2];
        bool selected[2]{};
        MaterialProgramDX11::Ptr current;
        Binding::Data *bindings = nullptr;
        SourceKey attempted{nullptr};
        bool hasAttempted = false, force = false, budgetDenied = false, subscriberBlocked = false, conflictBlocked=false;
        u64 subscriptionRevision=0,conflictRevision=0;
        std::array<u64,2> deniedHeadroom{};
        std::array<char,1024> error{};
        void diagnostic(const char *message)
        {
            std::strncpy(error.data(),message,error.size()-1); error.back() = 0;
        }
        ~Data() { wp_shader_program_destroy(source); }
    };
    std::mutex MaterialProgramReloadDX11::Data::registryMutex;
    MaterialProgramReloadDX11::Data *MaterialProgramReloadDX11::Data::head=nullptr;
    u64 MaterialProgramReloadDX11::Data::nextId=1;
    MaterialProgramReloadDX11::CaptureScope::CaptureScope() noexcept { ++reloadCaptureDepth; }
    MaterialProgramReloadDX11::CaptureScope::~CaptureScope() noexcept
    {
        assert(reloadCaptureDepth != 0);
        --reloadCaptureDepth;
    }
    MaterialProgramReloadDX11::MaterialProgramReloadDX11() : m_data(new Data)
    {
        auto &data=*m_data; std::lock_guard<std::mutex> lock(Data::registryMutex);
        data.id=Data::nextId++; data.owner=this; data.next=Data::head;
        if(data.next) data.next->previous=&data; Data::head=&data;
    }
    MaterialProgramReloadDX11::~MaterialProgramReloadDX11()
    {
        auto &data=*m_data; std::lock_guard<std::mutex> lock(Data::registryMutex);
        if(data.previous) data.previous->next=data.next; else Data::head=data.next;
        if(data.next) data.next->previous=data.previous;
    }
    MaterialProgramReloadDX11::Binding::Binding() : m_data(new Data) {}
    MaterialProgramReloadDX11::Binding::~Binding()
    {
        auto &binding = *m_data;
        if(!binding.linked) return;
        ++binding.owner->m_data->subscriptionRevision;
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
        Update();
        MaterialShaderParametersInstance::MigrationPtr migration;
        if(values.GetLayout()!=m_data->layout)
        {
            if(!m_data->autoSchema) throw std::invalid_argument("Reload binding requires its CPU schema");
            migration=values.PrepareMigration(m_data->layout);
        }
        BindingPtr binding(new Binding);
        auto &data = *binding->m_data;
        data.owner = shared_from_this(); data.values = &values; data.resources = std::move(resources);
        data.program = m_data->current;
        if(data.program) data.publication = data.program->Capture(migration?migration->Values():values,data.resources);
        if(migration && (!migration->IsCurrent() || !migration->Commit())) throw std::runtime_error("Material changed during shader attachment");
        data.revision=values.GetRevision();
        data.next = m_data->bindings;
        if(data.next) data.next->previous = &data;
        m_data->bindings = &data; data.linked = true;++m_data->subscriptionRevision;
        return binding;
    }
    bool MaterialProgramReloadDX11::Update()
    {
        auto &data = *m_data;
        // Do not consume commands or poll files midway through a frame's view
        // capture. The next owner boundary installs the complete pending cohort.
        if(reloadCaptureDepth) return bool(data.current) && !data.error[0];
        const bool manual=data.reloadCommand.exchange(false);
        const int watch=data.watchCommand.exchange(-1);
        if(watch>=0) data.watching=watch!=0;
        if(manual) data.force=true;
        if(data.files && (data.watching || manual)) data.files->Poll(manual);
        struct Publish { Data &data; u64 attempts; bool command; ~Publish() { if(attempts!=data.attempts || command) data.publish(); } } publish{data,data.attempts,manual || watch>=0};
        const SourceKey key(data.source,data.files?data.files->Revision():0,data.schemaRevision);
        if(data.hasAttempted && key == data.attempted && !data.force)
        {
            if(data.conflictBlocked)
            {
                if(data.conflictRevision==data.subscriptionRevision) return false;
            }
            else if(data.subscriberBlocked)
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
        if(data.requestedLayout || data.autoSchema)
            for(auto binding=data.bindings;binding;binding=binding->next)
                if(!binding->values->IsMigrationReady()) return false; // transient, no attempt/compile while busy
        data.force = false; data.hasAttempted = true; data.attempted = key;
        data.budgetDenied = data.subscriberBlocked = data.conflictBlocked = false;
        ++data.attempts;
        try
        {
            for(size_t i = 0; i < key.stages.size(); ++i) if(key.stages[i].identity)
                if(i > 1 || key.stages[i].type != wp_shader_type(i))
                { data.diagnostic("Reload supports only matching vertex/pixel stage attachments"); return false; }
            if(!key.stages[1].identity) { data.diagnostic("Reload requires a pixel shader"); return false; }
            auto preparedDefinition=data.autoSchema?std::shared_ptr<Definition>{}:
                std::make_shared<Definition>(data.source,data.requestedLayout?data.requestedLayout:
                    (data.explicitSchema?data.layout:MaterialShaderParameterLayout::Ptr{}));
            if(data.files) data.files->BeginCandidate();
            Bytecode code[2]; String error;
            for(size_t i = 0; i < 2; ++i)
            {
                bool compiled=true;
                if(data.files && data.autoSchema && !data.files->HasStage(i)) continue;
                if(data.files && data.files->HasStage(i))
                {
                    ++data.compiles;
                    compiled=data.files->Compile(key.stages[i].identity,i,code[i].compiled,error);
                    if(compiled) code[i].bytes={code[i].compiled->GetBufferPointer(),code[i].compiled->GetBufferSize()};
                }
                else compiled=compile(key.stages[i].identity,wp_shader_type(i),code[i],data.compiles,error);
                if(!compiled) { data.diagnostic(error.c_str()); return false; }
            }
            auto layout=data.requestedLayout?data.requestedLayout:data.layout;
            MaterialProgramReflectionSelection fileSelection[2];
            if(data.files && data.autoSchema) layout=fileSchema(code,layout,fileSelection);
            if(!layout) { data.diagnostic("Invalid reflected shader schema");return false; }
            auto definition=preparedDefinition?preparedDefinition:std::make_shared<Definition>(data.source,layout);
            auto candidate = MaterialProgramDX11::Create(data.device.Get(),layout,definition->program.get(),
                data.deviceGeneration,code[1].bytes,code[0].bytes,error,
                data.autoSchema?&fileSelection[1]:(data.selected[1] ? &data.selections[1] : nullptr),
                data.autoSchema?&fileSelection[0]:(data.selected[0] ? &data.selections[0] : nullptr));
            if(!candidate) { data.diagnostic(error.c_str()); return false; }
            candidate->retainDefinitionOwner(definition);
            struct Pending
            {
                Binding::Data *binding;
                MaterialProgramDX11::PublicationPtr publication;
                u64 revision;
                MaterialShaderParametersInstance::MigrationPtr migration;
            };
            std::vector<Pending,SourceBudgetAllocator<Pending>> pending;
            for(auto binding = data.bindings; binding; binding = binding->next)
            {
                MaterialShaderParametersInstance::MigrationPtr migration;
                if(layout!=data.layout && binding->values->GetLayout()!=layout)
                {
                    if(!binding->values->IsMigrationReady()) { data.force=true;data.diagnostic("Material publication is busy; schema reload deferred");return false; }
                    for(const auto &prior:pending) if(prior.binding->values==binding->values) { migration=prior.migration;break; }
                    if(!migration) migration=binding->values->PrepareMigration(layout);
                }
                else if(binding->values->GetLayout() != layout)
                {
                    data.subscriberBlocked = true;
                    data.diagnostic("Subscribed material changed CPU schema; shader reload was not published"); return false;
                }
                const auto revision = binding->values->GetRevision();
                pending.push_back({binding,candidate->Capture(migration?migration->Values():*binding->values,binding->resources),revision,migration});
            }
            bool changed = !(SourceKey(data.source,data.files?data.files->Revision():0,data.schemaRevision) == key) ||
                (data.files && !data.files->ValidateCandidate());
            for(const auto &entry : pending)
                changed = changed || entry.binding->values->GetRevision() != entry.revision ||
                    (entry.migration ? !entry.migration->IsCurrent() : entry.binding->values->GetLayout()!=layout);
            if(changed)
            {
                data.diagnostic("Shader or material changed during reload preparation");
                data.force = true; return false;
            }
            if(data.generation == std::numeric_limits<u64>::max())
            { data.diagnostic("Shader reload generation exhausted"); return false; }
            for(size_t i=0;i<pending.size();++i) if(pending[i].migration)
                for(size_t j=0;j<i;++j) if(pending[j].migration && pending[i].migration!=pending[j].migration &&
                    pending[i].migration->ConflictsWith(*pending[j].migration))
                { data.conflictBlocked=true;data.conflictRevision=data.subscriptionRevision;data.diagnostic("Shader schema subscribers share mutable pass state");return false; }
            // Owner-thread validation above makes each token non-stale throughout
            // this nonallocating, callback-free group commit. Deduplicate aliases.
            for(size_t i=0;i<pending.size();++i) if(pending[i].migration)
            {
                bool first=true;for(size_t j=0;j<i;++j) if(pending[j].migration==pending[i].migration) first=false;
                if(first) { const bool committed=pending[i].migration->Commit();assert(committed);if(!committed) std::terminate(); }
            }
            // No allocations, callbacks or observers between these assignments.
            // All subscriptions publish together on the graphics owner thread.
            for(auto &entry : pending)
            {
                entry.binding->program = candidate;
                entry.binding->publication = std::move(entry.publication);
                entry.binding->revision = entry.binding->values->GetRevision();
            }
            data.layout=std::move(layout);data.requestedLayout.reset();
            if(data.files) data.files->CommitCandidate();
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
        if(!data.program) return {};
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
    bool MaterialProgramReloadDX11::SetStageFiles(const String &vertex,const String &pixel,String &error)
    {
        try
        {
            auto files=std::make_unique<detail::ShaderFilesDX11>(vertex,pixel);
            SourceBudgetString v(files->Path(0)), p(files->Path(1));
            { std::lock_guard<std::mutex> lock(Data::registryMutex); m_data->status.vertex=std::move(v);m_data->status.pixel=std::move(p); }
            m_data->files=std::move(files);m_data->force=true;error.clear();return true;
        }
        catch(const std::exception &e) { error=e.what();return false; }
    }
    MaterialProgramReloadDX11::Ptr MaterialProgramReloadDX11::CreateFromFiles(void *device,
        MaterialShaderParameterLayout::Ptr layout,const String &vertex,const String &pixel,u64 generation,String &error)
    {
        if(!device || pixel.empty() || !generation) { error="Invalid shader file configuration";return {}; }
        const auto vertexPath=vertex.empty()?String():String(std::filesystem::absolute(std::filesystem::u8path(vertex.c_str())).lexically_normal().u8string().c_str());
        const auto pixelPath=String(std::filesystem::absolute(std::filesystem::u8path(pixel.c_str())).lexically_normal().u8string().c_str());
        for(auto cached=Data::head;cached;cached=cached->next)
            if(cached->autoSchema && cached->device.Get()==device && (cached->baseLayout==layout || cached->layout==layout) &&
                cached->status.vertex==vertexPath.c_str() && cached->status.pixel==pixelPath.c_str())
            { error.clear();return cached->owner->shared_from_this(); }
        Ptr result(new MaterialProgramReloadDX11); auto &data=*result->m_data;
        data.device=static_cast<ID3D11Device *>(device);data.baseLayout=layout;
        // Ordinary legacy materials have no CPU shader layout yet. Reflection
        // can install their first schema without changing live values up front.
        data.layout=layout?std::move(layout):MaterialShaderParameterLayout::Create({});
        if(!data.layout) {error="Cannot create an empty shader schema";return {};}
        data.deviceGeneration=generation;
        data.autoSchema=true;
        data.ownedSourceBudget=admit(wp_shader_program_descriptor_size()+WP_SHADER_MAX_PARAMS*80u+
            (vertex.empty()?1u:2u)*(wp_shader_descriptor_size()+1));
        data.source=wp_shader_program_create(); if(!data.source) throw std::bad_alloc();
        const auto c=[](const char *p) { return reinterpret_cast<const wp_c8 *>(p); };
        for(int i=0;i<2;++i) if(i || !vertex.empty())
        {
            auto stage=wp_shader_create(wp_shader_type(i),WORKPHONE_SHADER_LANGUAGE_HLSL,c(""),c("main"));
            if(!stage) throw std::bad_alloc();wp_shader_program_attach(data.source,stage);wp_shader_destroy(stage);
        }
        for(const auto &p:data.layout->GetParameterInfo())
        {
            using T=MaterialShaderParameterType; wp_uniform_type type=WORKPHONE_UNIFORM_TYPE_INT; wp_shader_resource_kind resource=WORKPHONE_SHADER_RESOURCE_NONE;
            switch(p.m_type)
            {
            case T::Scalar:type=WORKPHONE_UNIFORM_TYPE_FLOAT;break; case T::Colour:case T::Vector4:type=WORKPHONE_UNIFORM_TYPE_VEC4;break;
            case T::Vector2:type=WORKPHONE_UNIFORM_TYPE_VEC2;break;case T::Matrix:type=WORKPHONE_UNIFORM_TYPE_MAT4;break;
            case T::UInt:type=WORKPHONE_UNIFORM_TYPE_UINT;break;case T::Bool:type=WORKPHONE_UNIFORM_TYPE_BOOL;break;
            case T::IntVector2:type=WORKPHONE_UNIFORM_TYPE_IVEC2;break;case T::IntVector4:type=WORKPHONE_UNIFORM_TYPE_IVEC4;break;
            case T::Texture:resource=WORKPHONE_SHADER_RESOURCE_TEXTURE;break;case T::Buffer:resource=WORKPHONE_SHADER_RESOURCE_BUFFER;break;
            case T::Sampler:resource=WORKPHONE_SHADER_RESOURCE_SAMPLER;break;default:break;
            }
            if(wp_shader_program_register_param(data.source,c(p.m_name.c_str()),type,1,WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,
                WORKPHONE_SHADER_AUTO_PARAM_NONE,resource)<0) throw std::runtime_error("Shader schema exceeds native parameter capacity");
        }
        // File materials own all non-transform constant buffers and all resources.
        data.selected[0]=data.selected[1]=false;
        if(!result->SetStageFiles(vertex,pixel,error)) return {};
        result->Update(); return result; // A failed initial generation stays watchable and reports its real error.
    }
    u64 MaterialProgramReloadDX11::GetId() const { return m_data->id; }
    bool MaterialProgramReloadDX11::HasProgram() const { return m_data->current && m_data->current->matchesDevice(m_data->device.Get()); }
    bool MaterialProgramReloadDX11::QueueReload(u64 id)
    {
        std::lock_guard<std::mutex> lock(Data::registryMutex);
        for(auto data=Data::head;data;data=data->next) if(data->id==id) { data->reloadCommand=true;return true; } return false;
    }
    bool MaterialProgramReloadDX11::QueueWatching(u64 id,bool enabled)
    {
        std::lock_guard<std::mutex> lock(Data::registryMutex);
        for(auto data=Data::head;data;data=data->next) if(data->id==id) { data->watchCommand=enabled?1:0;return true; } return false;
    }
    SmartPtr<Properties> MaterialProgramReloadDX11::EditorStatus(u64 id)
    {
        auto result=make_ptr<Properties>();result->setProperty("available",false);result->setProperty("pending",false);
        std::lock_guard<std::mutex> lock(Data::registryMutex);
        for(auto data=Data::head;data;data=data->next) if(data->id==id)
        {
            const auto &s=data->status; const bool pending=data->reloadCommand.load() || data->watchCommand.load()>=0;
            result->setProperty("available",true);result->setProperty("pending",pending);result->setProperty("success",s.success);
            result->setProperty("generation",s.generation);result->setProperty("attempts",s.attempts);result->setProperty("compiles",s.compiles);
            result->setProperty("watching",s.watching);result->setProperty("vertexFile",static_cast<String>(s.vertex));result->setProperty("pixelFile",static_cast<String>(s.pixel));
            result->setProperty("diagnostic",String(s.error.data()));result->setProperty("status",pending?String("queued"):(s.success?String("ready"):String("failed")));
            return result;
        }
        result->setProperty("success",false);result->setProperty("status",String("unavailable"));return result;
    }
    void MaterialProgramReloadDX11::RecreateDevice(void *device,u64 generation)
    {
        if(!device || !generation) throw std::invalid_argument("Device and generation are required");
        if(m_data->device.Get()==device && m_data->deviceGeneration==generation) return;
        m_data->device=static_cast<ID3D11Device *>(device);m_data->deviceGeneration=generation;m_data->force=true;
    }
    void MaterialProgramReloadDX11::UpdateAll(void *device,void *owner)
    {
        if(reloadCaptureDepth) return;
        for(auto data=Data::head;data;)
        {
            auto next=data->next;
            if(device && owner)
            {
                if(!data->deviceOwner && data->device.Get()==device) data->deviceOwner=owner;
                if(data->deviceOwner==owner && data->device.Get()!=device)
                    data->owner->RecreateDevice(device,data->deviceGeneration+1);
            }
            data->owner->Update();data=next;
        }
    }

    void MaterialProgramReloadDX11::RequestSchema(MaterialShaderParameterLayout::Ptr layout)
    {
        if(!layout) throw std::invalid_argument("Shader schema is required");
        m_data->requestedLayout=std::move(layout);m_data->explicitSchema=true;
        ++m_data->schemaRevision;m_data->force=true;
    }

}
