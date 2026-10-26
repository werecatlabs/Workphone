#pragma once

// The same test-only fixture is compiled against original and current engine
// DLLs. It deliberately measures the real adapters and native submission rather
// than a reconstruction of the old material algorithm. Timings include clipped
// DX11 draws; they are not isolated material-only CPU or GPU/frame measurements.
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Graphics/MaterialShader.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_mesh.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <limits>
#include <stdexcept>
#include <vector>
#if !defined(WP_MATERIAL_MEMORY_PROFILE_BASELINE)
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <WPGraphics/ClawMaterialDrawRecords.hpp>
#endif

namespace material_cpu_profile
{
    using namespace workphone;
    using namespace workphone::render;
    using Microsoft::WRL::ComPtr;
    using Clock = std::chrono::steady_clock;

    inline void require(bool value, const char *message)
    { if(!value) throw std::runtime_error(message); }

    class Window : public GraphicsWindow
    {
    public:
        HWND handle = nullptr;
        Window()
        {
            handle = CreateWindowExW(0, L"STATIC", L"Material CPU comparison", WS_OVERLAPPEDWINDOW,
                0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            require(handle != nullptr, "CPU profile requires a DX11 host window");
        }
        ~Window() override { if(handle) DestroyWindow(handle); }
        Vector2I getSize() const override { return {64,64}; }
        void getWindowHandle(void *data) override { *static_cast<HWND *>(data) = handle; }
        void _getObject(void **data) const override { *data = nullptr; }
    };

    class Mesh : public ClawMesh
    {
    public:
        Mesh()
        {
            m_mesh = wp_graphics_mesh_create();
            require(m_mesh != nullptr, "CPU profile mesh exists");
            const wp_graphics_mesh_vertex_pnt vertices[] = {
                {{-0.25f,-0.25f,0.5f},{0,0,-1},{0,0}},
                {{0.25f,-0.25f,0.5f},{0,0,-1},{1,0}},
                {{0,0.25f,0.5f},{0,0,-1},{0.5f,1}}};
            const wp_u16 indices[] = {0,1,2};
            wp_graphics_mesh_set_vertices(m_mesh, WORKPHONE_VERTEX_FORMAT_PNT, vertices, 3);
            wp_graphics_mesh_set_indices_u16(m_mesh, indices, 3);
            require(wp_graphics_mesh_add_submesh(m_mesh,0,3,0) == 0 &&
                    wp_graphics_mesh_add_submesh(m_mesh,0,3,1) == 1,
                    "CPU profile draws two material sections");
        }
    };

    inline wp_render_statistics_dx11 statistics(wp_renderer_dx11 *native)
    {
        wp_render_statistics_dx11 result{};
        wp_renderer_dx11_get_statistics(native,&result);
        return result;
    }

    // An actual constant-buffer readback forces completion outside timed work
    // and verifies the last material was submitted. Combined upload is default
    // in both revisions and its byte layout is shared by their HLSL contract.
    inline void verifySubmitted(wp_renderer_dx11 *native, const ColourF &diffuse, f32 roughness)
    {
        auto device = static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));
        auto context = static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(native));
        ComPtr<ID3D11Buffer> buffer;
        context->PSGetConstantBuffers(1,1,buffer.GetAddressOf());
        require(buffer != nullptr, "CPU profile material buffer is bound");
        D3D11_BUFFER_DESC description{}; buffer->GetDesc(&description);
        require(description.ByteWidth >= sizeof(wp_material_dx11), "CPU profile combined constants exist");
        description.Usage = D3D11_USAGE_STAGING; description.BindFlags = 0;
        description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Buffer> staging;
        require(SUCCEEDED(device->CreateBuffer(&description,nullptr,staging.GetAddressOf())),
                "CPU profile readback buffer exists");
        context->CopyResource(staging.Get(),buffer.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        require(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)),
                "CPU profile submitted constants are readable");
        wp_material_dx11 submitted{}; std::memcpy(&submitted,mapped.pData,sizeof(submitted));
        context->Unmap(staging.Get(),0);
        require(submitted.base_color.x == diffuse.r && submitted.base_color.y == diffuse.g &&
                submitted.base_color.z == diffuse.b && submitted.base_color.w == diffuse.a && submitted.surface.y == roughness,
                "CPU profile GPU constant values equal final authored values");
    }

    struct Sample
    {
        double updateNs = 0, prepareNs = 0, submitNs = 0, retireNs = 0;
        u64 draws = 0, preparations = 0, storageAllocations = 0;
        double checksum = 0;
    };

    inline double ns(Clock::time_point begin, Clock::time_point end)
    { return std::chrono::duration<double,std::nano>(end-begin).count(); }

    template <class Fixture>
    void run(Fixture &fixture, size_t count, u32 width, u32 frames, u32 sampleCount)
    {
        require(count && count <= 10000 && (width == 8 || width == 32 || width == 128) &&
                frames && frames <= 300 && sampleCount && sampleCount <= 31,
                "CPU profile dimensions are valid");
        auto window = make_ptr<Window>();
        ClawRendererDX11 renderer; renderer.load(window);
        require(renderer.isLoaded(), "CPU profile requires the actual DX11 renderer");
        auto target = make_ptr<ClawRenderTarget>(); target->setSize({64,64});
        renderer.setRenderTarget(target); renderer.setCamera(nullptr); renderer.disableShadows();
        renderer.setSceneLighting(ColourF::White,Vector3F(0,-1,0),ColourF::White,0);
        auto native = wp_renderer_get_dx11(renderer.getNativeRenderer());
        require(native != nullptr, "CPU profile native DX11 renderer exists");
        auto device = static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));
        const auto featureLevel = device->GetFeatureLevel();
        ComPtr<IDXGIDevice> dxgi; ComPtr<IDXGIAdapter> adapter;
        require(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(dxgi.GetAddressOf()))) &&
                SUCCEEDED(dxgi->GetAdapter(adapter.GetAddressOf())), "CPU profile adapter is available");
        DXGI_ADAPTER_DESC adapterDesc{};
        require(SUCCEEDED(adapter->GetDesc(&adapterDesc)), "CPU profile adapter metadata is available");
        LARGE_INTEGER driverVersion{};
        const bool driverVersionAvailable = SUCCEEDED(adapter->CheckInterfaceSupport(
            __uuidof(IDXGIDevice), &driverVersion));

        MaterialShader schema("MaterialCpuProfile");
        for(u32 i=0;i<width;++i)
            schema.addParameter(i%4 == 0 ? MaterialShaderParameterType::Scalar :
                                i%4 == 1 ? MaterialShaderParameterType::Colour :
                                i%4 == 2 ? MaterialShaderParameterType::Matrix : MaterialShaderParameterType::Texture,
                                "Value" + String(std::to_string(i).c_str()));
        MaterialShaderRegistry::instance().registerBuiltinShaders();
        MaterialShaderRegistry::instance().registerShader(schema);
        std::vector<SmartPtr<ClawMaterial>> materials(count);
        std::vector<MaterialShaderParameterHandle> handles(count);
        std::vector<std::unique_ptr<Mesh>> meshes(count);
        for(size_t i=0;i<count;++i)
        {
            auto material = make_ptr<ClawMaterial>(); material->load(nullptr);
            material->setShaderParameters(schema.getParameterInfo(),schema.getShaderID());
            material->setDiffuse(ColourF(0.25f,0.5f,0.75f,1));
            material->setRoughness(0.5f); material->setMetalness(0.25f);
            handles[i] = material->getShaderParameters().FindParameter("Value0");
            require(handles[i].IsValid(), "CPU profile retains a real parameter handle");
            material->getShaderParameters().SetScalar(handles[i],0.5f);
            auto mesh = std::make_unique<Mesh>();
            mesh->setMaterial(material,0); mesh->setMaterial(material,1);
            require(wp_renderer_dx11_prepare_mesh_geometry(native,mesh->getNativeMesh(),0,3) != nullptr,
                    "CPU profile immutable geometry prepares before timing");
            materials[i] = material; meshes[i] = std::move(mesh);
        }
        fixture.graphics->pending.clear();
        auto transform = Matrix4F::identity(); transform[0][3] = 4; // Fully clipped, actual draw calls.
#if !defined(WP_MATERIAL_MEMORY_PROFILE_BASELINE)
        ClawMaterialDrawRecords records;
#endif
        // Fixed output storage avoids allocation/printing in timed intervals.
        std::array<std::array<Sample,31>,3> results{};
        u32 frameSerial = 0;
        const auto execute = [&](u32 editsPercent, u32 measuredFrames) {
            Sample result;
            const auto before = statistics(native);
#if !defined(WP_MATERIAL_MEMORY_PROFILE_BASELINE)
            const auto storageBefore = records.getStorageAllocationCount();
            const auto preparedBefore = renderer.getMaterialRecordStatistics().preparations;
#endif
            for(u32 frame=0;frame<measuredFrames;++frame)
            {
                const auto begin = Clock::now();
                const auto edited = count * editsPercent / 100;
                const f32 value = (++frameSerial % 2) ? 0.25f : 0.75f;
                {
#if !defined(WP_MATERIAL_MEMORY_PROFILE_BASELINE)
                    MaterialPublicationScope publication;
#endif
                    for(size_t i=0;i<edited;++i)
                    {
                        // Include the last submitted material so the readback
                        // validates partial edits as well as full-population edits.
                        const auto index = count-edited+i;
                        materials[index]->setDiffuse(ColourF(value,0.5f,0.75f,1));
                        materials[index]->setRoughness(value);
                        materials[index]->getShaderParameters().SetScalar(handles[index],value);
                    }
#if !defined(WP_MATERIAL_MEMORY_PROFILE_BASELINE)
                    publication.commit();
#endif
                }
                const auto updated = Clock::now();
#if !defined(WP_MATERIAL_MEMORY_PROFILE_BASELINE)
                for(auto &mesh:meshes) records.capture(mesh.get());
                renderer.prepareMaterialRecords(records);
#endif
                const auto prepared = Clock::now();
                for(u32 view=0;view<2;++view)
                    for(auto &mesh:meshes)
                    {
#if defined(WP_MATERIAL_MEMORY_PROFILE_BASELINE)
                        renderer.renderMesh(mesh.get(),transform);
#else
                        require(renderer.renderMeshRecords(mesh.get(),transform,records,*records.find(mesh.get())),
                                "CPU profile captured records submit successfully");
#endif
                    }
                const auto submitted = Clock::now();
#if !defined(WP_MATERIAL_MEMORY_PROFILE_BASELINE)
                records.clear();
#endif
                const auto retired = Clock::now();
                result.updateNs += ns(begin,updated); result.prepareNs += ns(updated,prepared);
                result.submitNs += ns(prepared,submitted); result.retireNs += ns(submitted,retired);
            }
            const auto after = statistics(native);
            result.draws = after.draws-before.draws;
            require(result.draws == u64(count)*2*2*measuredFrames,
                    "CPU profile submits every material/section/view without skipped draws");
            verifySubmitted(native,materials.back()->getDiffuse(),materials.back()->getRoughness());
            for(size_t i=0;i<count;++i)
                result.checksum += materials[i]->getShaderParameters().GetScalar(handles[i]) +
                                   materials[i]->getDiffuse().r + materials[i]->getRoughness();
#if !defined(WP_MATERIAL_MEMORY_PROFILE_BASELINE)
            result.preparations = renderer.getMaterialRecordStatistics().preparations-preparedBefore;
            result.storageAllocations = records.getStorageAllocationCount()-storageBefore;
#endif
            return result;
        };
        execute(100,3); // Warm compilation, geometry, driver and both value variants.
        for(u32 sample=0;sample<sampleCount;++sample)
            for(u32 order=0;order<3;++order)
            {
                const u32 mode = (sample+order)%3;
                const u32 edits = mode == 0 ? 0 : mode == 1 ? 1 : 100;
                // Reset every material to the same authored start between modes.
                frameSerial=0; execute(100,2);
                results[mode][sample] = execute(edits,frames);
            }
        fixture.graphics->pending.clear();
        meshes.clear();
        for(auto &material:materials) material->unload(nullptr);
        materials.clear(); fixture.graphics->pending.clear();
        renderer.setRenderTarget(nullptr); target=nullptr; renderer.unload(nullptr); window=nullptr;
        for(u32 mode=0;mode<3;++mode) for(u32 sample=0;sample<sampleCount;++sample)
        {
            const auto &result = results[mode][sample];
            std::cout << "material_cpu implementation="
#if defined(WP_MATERIAL_MEMORY_PROFILE_BASELINE)
                << "baseline957"
#else
                << "optimized"
#endif
                << " materials=" << count << " parameters=" << width
                << " schema=mixed_scalar_colour_matrix_texture edits_percent=" << (mode==0?0:mode==1?1:100)
                << " edited_materials_per_frame=" << count*(mode==0?0:mode==1?1:100)/100
                << " frames=" << frames << " sections=2 views=2 sample=" << sample
                << " samples=" << sampleCount << " gpu_pixels=unmeasured geometry=translated_x4 present_calls=0 heap_walks=0"
                << " timer=steady_clock_wall_interval"
                << " update_publication_ns=" << result.updateNs << " capture_prepare_ns=" << result.prepareNs
                << " submit_ns=" << result.submitNs << " retirement_ns=" << result.retireNs
                << " total_ns=" << result.updateNs+result.prepareNs+result.submitNs+result.retireNs
                << " draws=" << result.draws << " record_preparations=" << result.preparations
                << " workspace_allocations=" << result.storageAllocations << " checksum=" << result.checksum
                << " adapter_vendor=" << adapterDesc.VendorId << " adapter_device=" << adapterDesc.DeviceId
                << " dedicated_video_memory=" << adapterDesc.DedicatedVideoMemory
                << " feature_level=" << featureLevel
                << " driver_version_available=" << (driverVersionAvailable ? 1 : 0)
                << " driver_version=" << static_cast<u64>(driverVersion.QuadPart)
                << " material_only_attribution=0 representative_frame_acceptance=0\n";
        }
    }
}
