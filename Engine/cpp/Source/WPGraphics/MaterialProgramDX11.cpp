#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/MaterialProgramDX11.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <cstring>

namespace workphone::render
{
    namespace
    {
        using Microsoft::WRL::ComPtr;
        template<class T> using Storage = std::vector<T, SourceBudgetAllocator<T>>;
        StreamBudget admit(StreamMemoryClass kind, size_t bytes)
        {
            String error;
            auto lease = WorldStreamingScheduler::shared()->reserve(kind, bytes, error);
            if(!lease) throw std::bad_alloc();
            return lease;
        }
        struct Stage
        {
            MaterialProgramBindingMap::Ptr map;
            std::array<ComPtr<ID3D11Buffer>, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> buffers;
            std::array<D3D_SRV_DIMENSION,128> dimensions{};
        };
        struct Packed
        {
            Storage<u8> bytes;
            Storage<MaterialProgramResourceValue> resources;
        };
    }
    struct MaterialProgramResourceTableDX11::Data
    {
        StreamBudget cpu = admit(StreamMemoryClass::SourceCpu, sizeof(Data) + sizeof(MaterialProgramResourceTableDX11));
        struct Entry
        {
            MaterialResourceHandle handle;
            SmartPtr<ITexture> texture;
            ComPtr<IUnknown> native;
        };
        Storage<Entry> entries[3];
        static size_t index(MaterialShaderParameterType type)
        {
            switch(type)
            {
            case MaterialShaderParameterType::Texture: return 0;
            case MaterialShaderParameterType::Buffer: return 1;
            case MaterialShaderParameterType::Sampler: return 2;
            default: throw std::invalid_argument("Not a material resource kind");
            }
        }
        static auto find(Storage<Entry> &entries, MaterialResourceHandle handle)
        {
            return std::lower_bound(entries.begin(), entries.end(), handle,
                [](const Entry &entry, MaterialResourceHandle id) { return entry.handle < id; });
        }
        void set(size_t kind, MaterialResourceHandle handle, SmartPtr<ITexture> texture, IUnknown *native)
        {
            if(handle == InvalidMaterialResourceHandle) throw std::invalid_argument("Reserved material resource handle");
            auto &list = entries[kind];
            auto position = find(list, handle);
            Entry candidate{handle, std::move(texture), native};
            if(position != list.end() && position->handle == handle) *position = std::move(candidate);
            else list.insert(position,std::move(candidate));
        }
    };
    MaterialProgramResourceTableDX11::MaterialProgramResourceTableDX11() : m_data(new Data) {}
    MaterialProgramResourceTableDX11::~MaterialProgramResourceTableDX11() = default;
    void MaterialProgramResourceTableDX11::SetTexture(MaterialResourceHandle handle, SmartPtr<ITexture> texture)
    { m_data->set(0, handle, std::move(texture), nullptr); }
    void MaterialProgramResourceTableDX11::SetBuffer(MaterialResourceHandle handle, void *view)
    { m_data->set(1, handle, nullptr, static_cast<ID3D11ShaderResourceView *>(view)); }
    void MaterialProgramResourceTableDX11::SetSampler(MaterialResourceHandle handle, void *sampler)
    { m_data->set(2, handle, nullptr, static_cast<ID3D11SamplerState *>(sampler)); }
    bool MaterialProgramResourceTableDX11::Resolve(MaterialShaderParameterType type,
        MaterialResourceHandle handle, void *&native)
    {
        native = nullptr;
        auto &list = m_data->entries[Data::index(type)];
        const auto found = Data::find(list,handle);
        if(found == list.end() || found->handle != handle) return false;
        if(type == MaterialShaderParameterType::Texture)
        {
            if(found->texture) found->texture->getTextureFinal(&native);
        }
        else native = found->native.Get();
        return native != nullptr;
    }
    struct MaterialProgramDX11::Data
    {
        StreamBudget cpu = admit(StreamMemoryClass::SourceCpu, sizeof(Data) + sizeof(MaterialProgramDX11));
        StreamBudget gpu;
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11VertexShader> vertex;
        ComPtr<ID3D11PixelShader> pixel;
        ComPtr<ID3D11InputLayout> input;
        MaterialShaderParameterLayout::Ptr layout;
        wp_shader_program *program = nullptr; // Retained by both maps.
        u64 generation = 0;
        Stage stages[2]; // vertex, pixel
    };
    struct MaterialProgramDX11::Publication::Data
    {
        StreamBudget cpu = admit(StreamMemoryClass::SourceCpu, sizeof(Data) + sizeof(Publication));
        std::shared_ptr<const MaterialProgramDX11> program;
        std::shared_ptr<MaterialProgramResourcesDX11> resolver;
        Packed stages[2];
    };
    MaterialProgramDX11::MaterialProgramDX11() : m_data(new Data) {}
    MaterialProgramDX11::~MaterialProgramDX11() = default;
    MaterialProgramDX11::Publication::Publication() : m_data(new Data) {}
    MaterialProgramDX11::Publication::~Publication() = default;

    MaterialProgramDX11::Ptr MaterialProgramDX11::Create(void *device,
        MaterialShaderParameterLayout::Ptr layout, wp_shader_program *program, u64 generation,
        Bytecode pixel, Bytecode vertex, String &error,
        const MaterialProgramReflectionSelection *pixelSelection,
        const MaterialProgramReflectionSelection *vertexSelection)
    {
        error.clear();
        const auto fail = [&](const char *message) -> Ptr { error = message; return {}; };
        if(!device || !layout || !program || !generation || !pixel.data || !pixel.size ||
           bool(vertex.data) != bool(vertex.size)) return fail("Invalid runtime shader inputs");
        Ptr result(new MaterialProgramDX11);
        auto &data = *result->m_data;
        data.device = static_cast<ID3D11Device *>(device);
        data.layout = layout; data.program = program; data.generation = generation;
        data.stages[1].map = CreateDX11MaterialProgramBindingMap(layout, program, generation,
            WORKPHONE_SHADER_TYPE_PIXEL, pixel.data, pixel.size, error, pixelSelection);
        if(!data.stages[1].map) return {};
        if(vertex.data)
        {
            data.stages[0].map = CreateDX11MaterialProgramBindingMap(layout, program, generation,
                WORKPHONE_SHADER_TYPE_VERTEX, vertex.data, vertex.size, error, vertexSelection);
            if(!data.stages[0].map) return {};
        }
        const Bytecode bytecodes[] = {vertex,pixel};
        for(size_t s = 0; s < 2; ++s) if(bytecodes[s].data)
        {
            ComPtr<ID3D11ShaderReflection> reflection;
            if(FAILED(D3DReflect(bytecodes[s].data,bytecodes[s].size,IID_ID3D11ShaderReflection,
                reinterpret_cast<void **>(reflection.GetAddressOf())))) return fail("Shader reflection failed");
            D3D11_SHADER_DESC shader{}; reflection->GetDesc(&shader);
            for(UINT i = 0; i < shader.BoundResources; ++i)
            {
                D3D11_SHADER_INPUT_BIND_DESC binding{};
                if(FAILED(reflection->GetResourceBindingDesc(i,&binding))) return fail("Resource reflection failed");
                if(binding.BindPoint < 128 && (binding.Type == D3D_SIT_TEXTURE || binding.Type == D3D_SIT_TBUFFER ||
                    binding.Type == D3D_SIT_STRUCTURED || binding.Type == D3D_SIT_BYTEADDRESS))
                    data.stages[s].dimensions[binding.BindPoint] = binding.Dimension;
            }
        }
        size_t gpuBytes = 0;
        for(auto &stage : data.stages) if(stage.map)
            for(size_t i = 0; i < stage.map->GetBufferCount(); ++i)
            {
                const auto buffer = stage.map->GetBuffer(i);
                if(buffer->slot == 0) return fail("b0 is owned by geometry transforms");
                gpuBytes += buffer->size;
            }
        if(gpuBytes) data.gpu = admit(StreamMemoryClass::Gpu, gpuBytes);
        if(FAILED(data.device->CreatePixelShader(pixel.data, pixel.size, nullptr, &data.pixel)))
            return fail("DX11 pixel shader creation failed");
        if(vertex.data)
        {
            if(FAILED(data.device->CreateVertexShader(vertex.data, vertex.size, nullptr, &data.vertex)))
                return fail("DX11 vertex shader creation failed");
            const D3D11_INPUT_ELEMENT_DESC elements[] = {
                {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
                {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
                {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0},
                {"COLOR",0,DXGI_FORMAT_R32_UINT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0}};
            if(FAILED(data.device->CreateInputLayout(elements, 4, vertex.data, vertex.size, &data.input)))
                return fail("Vertex shader does not accept engine PNTC geometry");
        }
        for(auto &stage : data.stages) if(stage.map)
            for(size_t i = 0; i < stage.map->GetBufferCount(); ++i)
            {
                D3D11_BUFFER_DESC desc{};
                desc.ByteWidth = stage.map->GetBuffer(i)->size;
                desc.Usage = D3D11_USAGE_DYNAMIC; desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                if(FAILED(data.device->CreateBuffer(&desc, nullptr, &stage.buffers[i])))
                    return fail("Material constant buffer creation failed");
            }
        return result;
    }
    bool MaterialProgramDX11::IsCurrent() const
    {
        for(const auto &stage : m_data->stages)
            if(stage.map && !stage.map->IsCurrent(m_data->layout, m_data->program, m_data->generation)) return false;
        return true;
    }
    bool MaterialProgramDX11::IsCurrent(const MaterialShaderParameterLayout::Ptr &layout) const
    { return layout == m_data->layout && IsCurrent(); }
    MaterialProgramDX11::PublicationPtr MaterialProgramDX11::Capture(
        const MaterialShaderParametersInstance &values, std::shared_ptr<MaterialProgramResourcesDX11> resolver) const
    {
        if(!IsCurrent(values.GetLayout()))
            throw std::runtime_error("Stale runtime material shader/schema");
        std::shared_ptr<Publication> result(new Publication);
        result->m_data->program = shared_from_this();
        result->m_data->resolver = std::move(resolver);
        for(size_t i = 0; i < 2; ++i) if(const auto &map = m_data->stages[i].map)
        {
            auto &out = result->m_data->stages[i];
            out.bytes.resize(map->GetPackedSize()); out.resources.resize(map->GetResourceCount());
            if(!map->Pack(values, m_data->generation, out.bytes.data(), out.bytes.size(),
                          out.resources.data(), out.resources.size()))
                throw std::runtime_error("Runtime material packing failed");
        }
        return result;
    }
    bool MaterialProgramDX11::Publication::Bind(wp_renderer_dx11 *renderer, bool instanced) const
    {
        const auto &program = *m_data->program;
        const auto &data = *program.m_data;
        if(!renderer || wp_renderer_dx11_get_device(renderer) != data.device.Get() ||
           !program.IsCurrent() || (instanced && data.vertex)) return false;
        auto context = static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(renderer));
        // Acquire every resource before changing context state or uploading. Strong
        // temporary COM owners span submission; no device pointers enter CPU records.
        std::array<std::array<ComPtr<ID3D11ShaderResourceView>,128>,2> views;
        std::array<std::array<ComPtr<ID3D11SamplerState>,16>,2> samplers;
        for(size_t s = 0; s < 2; ++s)
            for(const auto &value : m_data->stages[s].resources)
            {
                if(value.handle == InvalidMaterialResourceHandle) continue;
                void *native = nullptr;
                if(!m_data->resolver || !m_data->resolver->Resolve(value.type, value.handle, native) || !native) return false;
                ComPtr<ID3D11Device> owner;
                if(value.type == MaterialShaderParameterType::Sampler)
                {
                    auto &sampler = samplers[s][value.slot];
                    if(FAILED(static_cast<IUnknown *>(native)->QueryInterface(IID_PPV_ARGS(&sampler)))) return false;
                    sampler->GetDevice(&owner);
                }
                else
                {
                    auto &view = views[s][value.slot];
                    if(FAILED(static_cast<IUnknown *>(native)->QueryInterface(IID_PPV_ARGS(&view)))) return false;
                    view->GetDevice(&owner);
                    D3D11_SHADER_RESOURCE_VIEW_DESC desc{}; view->GetDesc(&desc);
                    const bool buffer = desc.ViewDimension == D3D11_SRV_DIMENSION_BUFFER ||
                                        desc.ViewDimension == D3D11_SRV_DIMENSION_BUFFEREX;
                    if(buffer != (value.type == MaterialShaderParameterType::Buffer) ||
                       (!buffer && UINT(desc.ViewDimension) != UINT(data.stages[s].dimensions[value.slot])) ||
                       !wp_renderer_dx11_material_view_allowed(renderer, view.Get())) return false;
                }
                if(owner.Get() != data.device.Get()) return false;
            }
        for(size_t s = 0; s < 2; ++s) if(const auto &map = data.stages[s].map)
            for(size_t b = 0; b < map->GetBufferCount(); ++b)
            {
                D3D11_MAPPED_SUBRESOURCE mapped{};
                if(FAILED(context->Map(data.stages[s].buffers[b].Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped))) return false;
                const auto buffer = map->GetBuffer(b);
                std::memcpy(mapped.pData, m_data->stages[s].bytes.data() + buffer->packedOffset, buffer->size);
                context->Unmap(data.stages[s].buffers[b].Get(),0);
            }
        if(data.vertex)
        {
            context->IASetInputLayout(data.input.Get());
            context->VSSetShader(data.vertex.Get(),nullptr,0);
        }
        context->PSSetShader(data.pixel.Get(),nullptr,0);
        for(size_t s = 0; s < 2; ++s) if(const auto &map = data.stages[s].map)
        {
            for(size_t b = 0; b < map->GetBufferCount(); ++b)
            {
                auto buffer = data.stages[s].buffers[b].Get(); const auto slot = map->GetBuffer(b)->slot;
                if(s) context->PSSetConstantBuffers(slot,1,&buffer); else context->VSSetConstantBuffers(slot,1,&buffer);
            }
            for(const auto &value : m_data->stages[s].resources)
                if(value.type == MaterialShaderParameterType::Sampler)
                {
                    auto sampler = samplers[s][value.slot].Get();
                    if(s) context->PSSetSamplers(value.slot,1,&sampler); else context->VSSetSamplers(value.slot,1,&sampler);
                }
                else
                {
                    auto view = views[s][value.slot].Get();
                    if(s) context->PSSetShaderResources(value.slot,1,&view); else context->VSSetShaderResources(value.slot,1,&view);
                }
        }
        return true;
    }
    void MaterialProgramDX11::Publication::Unbind(wp_renderer_dx11 *renderer) const
    {
        if(!renderer || wp_renderer_dx11_get_device(renderer) != m_data->program->m_data->device.Get()) return;
        auto context = static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(renderer));
        context->PSSetShader(nullptr, nullptr, 0);
        if(m_data->program->m_data->vertex)
        {
            context->VSSetShader(nullptr, nullptr, 0);
            context->IASetInputLayout(nullptr);
        }
        for(size_t s = 0; s < 2; ++s)
        {
            const auto &map = m_data->program->m_data->stages[s].map;
            if(map) for(size_t b = 0; b < map->GetBufferCount(); ++b)
            {
                ID3D11Buffer *empty = nullptr;
                const auto slot = map->GetBuffer(b)->slot;
                if(s) context->PSSetConstantBuffers(slot, 1, &empty); else context->VSSetConstantBuffers(slot, 1, &empty);
            }
            for(const auto &value : m_data->stages[s].resources)
                if(value.type == MaterialShaderParameterType::Sampler)
                {
                    ID3D11SamplerState *empty = nullptr;
                    if(s) context->PSSetSamplers(value.slot,1,&empty); else context->VSSetSamplers(value.slot,1,&empty);
                }
                else
                {
                    ID3D11ShaderResourceView *empty = nullptr;
                    if(s) context->PSSetShaderResources(value.slot,1,&empty); else context->VSSetShaderResources(value.slot,1,&empty);
                }
        }
    }
}
