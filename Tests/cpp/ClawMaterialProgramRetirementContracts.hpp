#pragma once
#include <WPGraphics/MaterialProgramDX11.hpp>
#include <WPGraphics/ClawMaterialDrawRecords.hpp>
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <chrono>

// Included after the existing cooked-resource Fixture, Triangle and image helpers.
namespace claw_material_program_retirement_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using namespace workphone::resource;
    using Microsoft::WRL::ComPtr;

    inline void run(Fixture &fixture, const std::shared_ptr<const RuntimeResource> &payload,
                    const String &targetName, u32 cycles)
    {
        require(cycles && cycles <= 10000, "Program retirement cycles are bounded");
        auto renderer = fixture.graphics.renderer;
        auto native = wp_renderer_get_dx11(renderer->getNativeRenderer());
        auto device = static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));
        auto context = static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(native));
        auto target = make_ptr<ClawRenderTarget>(); target->setSize({32,32});
        struct ResetTarget
        {
            SmartPtr<ClawRendererDX11> renderer;
            ~ResetTarget() { renderer->setRenderTarget(nullptr); }
        } reset{renderer};
        renderer->setRenderTarget(target); renderer->setViewport(nullptr); renderer->setCamera(nullptr);
        Triangle mesh; mesh.load(nullptr);
        require(mesh.isLoaded(), "Program retirement uploads its fixed native geometry before measurement");
        renderer->clear(ColourF::Black); context->ClearState(); wp_renderer_dx11_invalidate_state(native);
        claw_texture_mip_contracts::completeGpuWork(context);
        auto scheduler = WorldStreamingScheduler::shared();
        constexpr auto source = size_t(StreamMemoryClass::SourceCpu);
        constexpr auto gpu = size_t(StreamMemoryClass::Gpu);
        constexpr auto working = size_t(StreamMemoryClass::WorkingCpu);
        // The existing source-free shader fixture has a 1x1 RGBA texture and
        // one independently reflected 16-byte constant block in each stage.
        constexpr u64 textureBytes = 4, blockBytes = 16, programBytes = 2 * blockBytes;
        const auto finishFrame = [&] {
            struct RestoreTask { TaskId task = Thread::getCurrentTask();
                ~RestoreTask() { Thread::setCurrentTask(task); } } restore;
            for(u32 task=0;task<static_cast<u32>(TaskId::Count);++task)
            { Thread::setCurrentTask(static_cast<TaskId>(task)); fixture.states->update(); }
        };
        const auto draw = [&](const ClawMaterialDrawRecords &records, bool green) {
            renderer->setRenderTarget(target); renderer->setViewport(nullptr); renderer->clear(ColourF::Black);
            const auto binding = records.find(&mesh);
            require(binding && renderer->renderMeshRecords(&mesh,Matrix4F::identity(),records,*binding),
                    "Retained custom-program records draw through the real renderer");
            ComPtr<ID3D11RenderTargetView> view; context->OMGetRenderTargets(1,&view,nullptr);
            require(view != nullptr, "Program retirement draw has a native target");
            ComPtr<ID3D11Resource> resource; view->GetResource(&resource);
            ComPtr<ID3D11Texture2D> texture; require(SUCCEEDED(resource.As(&texture)), "Program target is 2D");
            D3D11_TEXTURE2D_DESC desc{}; texture->GetDesc(&desc); const auto format = desc.Format;
            desc.Usage=D3D11_USAGE_STAGING; desc.BindFlags=desc.MiscFlags=0; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
            ComPtr<ID3D11Texture2D> staging;
            require(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging)), "Program pixel readback allocates");
            context->CopyResource(staging.Get(),texture.Get()); D3D11_MAPPED_SUBRESOURCE mapped{};
            require(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)), "Program readback maps");
            const auto pixel=static_cast<const u8 *>(mapped.pData)+16*mapped.RowPitch+16*4;
            const auto red=pixel[format==DXGI_FORMAT_B8G8R8A8_UNORM?2:0], actualGreen=pixel[1];
            context->Unmap(staging.Get(),0);
            require(green ? actualGreen>60 && red<20 : red>60 && actualGreen<20,
                    "Old/new custom shader publications retain their own red/green packed values");
        };
        const auto readBlock = [&](ID3D11Buffer *buffer, bool pixel) {
            D3D11_BUFFER_DESC desc{}; buffer->GetDesc(&desc);
            require(desc.ByteWidth==blockBytes && (desc.BindFlags&D3D11_BIND_CONSTANT_BUFFER),
                    "Escaped vertex/pixel constants retain their physical reflected allocation");
            desc.Usage=D3D11_USAGE_STAGING; desc.BindFlags=desc.MiscFlags=0;
            desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ; desc.StructureByteStride=0;
            ComPtr<ID3D11Buffer> staging;
            require(SUCCEEDED(device->CreateBuffer(&desc,nullptr,&staging)), "Escaped constant readback allocates");
            context->CopyResource(staging.Get(),buffer); D3D11_MAPPED_SUBRESOURCE mapped{};
            require(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)), "Escaped constant readback maps");
            const auto words=static_cast<const f32 *>(mapped.pData);
            const bool valid=words[0]==1.f && (!pixel || (words[1]==0.f && words[2]==0.f && words[3]==1.f));
            context->Unmap(staging.Get(),0);
            require(valid, "Escaped native constants preserve the last submitted packed values after program retirement");
        };
        const auto cycle = [&] {
            const auto before=scheduler->stats();
            ClawMaterialRenderCache cache; ClawMaterialDrawRecords redRecords, greenRecords;
            std::weak_ptr<const ClawMaterialResource::Candidate> redCandidate, greenCandidate;
            std::weak_ptr<const MaterialProgramDX11::Publication> redPublication, greenPublication;
            WeakPtr<IStateContext> redContext, greenContext;
            ComPtr<ID3D11Buffer> vertexConstants, pixelConstants;
            ComPtr<ID3D11SamplerState> sampler; ComPtr<ID3D11PixelShader> shader;
            D3D11_SAMPLER_DESC samplerDescription{};
            {
                ClawMaterialResource publisher(nullptr,nullptr,"",{},renderer,targetName,true);
                String error; auto red=publisher.stageRuntime(payload,error);
                require(red && publisher.publish(red,error),error);
                auto &values=red->material()->getShaderParameters();
                require(values.SetColour(values.FindParameter("Tint"),ColourF(1,0,0,1)),
                        "Program retirement authors a distinct old-generation tint");
                redCandidate=red; redContext=red->material()->getTechnique(0)->getPass(0)->getStateContext().get();
                mesh.setMaterial(red->material()); redRecords.capture(&mesh); redRecords.seal(&cache);
                auto old=red->material()->getRuntimePublication(); redPublication=old;
                require(old && old->Bind(native,false), "Old program binds its retained constants and sampler");
                context->VSGetConstantBuffers(8,1,&vertexConstants); context->PSGetConstantBuffers(4,1,&pixelConstants);
                context->PSGetSamplers(5,1,&sampler); context->PSGetShader(&shader,nullptr,nullptr);
                require(vertexConstants && pixelConstants && sampler && shader,
                        "Program fixture escapes both native constant blocks, sampler and shader");
                sampler->GetDesc(&samplerDescription); old->Unbind(native);
                auto green=publisher.stageRuntime(payload,error); require(green && publisher.publish(green,error),error);
                greenCandidate=green;
                greenContext=green->material()->getTechnique(0)->getPass(0)->getStateContext().get();
                mesh.setMaterial(green->material()); greenRecords.capture(&mesh); greenRecords.seal(&cache);
                greenPublication=green->material()->getRuntimePublication();
                draw(redRecords,false); draw(greenRecords,true);
                require(scheduler->stats().used[gpu]==before.used[gpu]+2*(textureBytes+programBytes),
                        "Two custom-program generations admit both textures and independent stage constant blocks");
                {
                    const auto resident=scheduler->stats();
                    struct RestoreGpuLimit
                    {
                        std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
                        ~RestoreGpuLimit() { String ignored; scheduler->setBudget(StreamMemoryClass::Gpu,limit,ignored); }
                    } restoreGpu{scheduler,resident.limits[gpu]};
                    String quotaError;
                    // Admit the candidate texture and its first stage block,
                    // then deny the second block. No replacement may publish.
                    require(scheduler->setBudget(StreamMemoryClass::Gpu,
                        resident.used[gpu]+textureBytes+blockBytes,quotaError),quotaError);
                    auto denied=publisher.stageRuntime(payload,error);
                    require(!denied && publisher.current()==green && scheduler->stats().used[gpu]==resident.used[gpu],
                            "Late stage-buffer admission denial rolls back every partial lease and preserves the visible program");
                }
                require(publisher.unload(error) && !publisher.current(),error); mesh.setMaterial(nullptr);
            }
            require(redCandidate.expired() && greenCandidate.expired() &&
                    !redPublication.expired() && !greenPublication.expired(),
                    "Captured records own both custom-program publications after publisher retirement");
            draw(redRecords,false); draw(greenRecords,true);
            context->ClearState(); wp_renderer_dx11_invalidate_state(native);
            redRecords.clear(); greenRecords.clear();
            require(redPublication.expired() && greenPublication.expired(),
                    "Final record release retires both C++ program-publication owners");
            claw_texture_mip_contracts::completeGpuWork(context);
            const auto escaped=scheduler->stats().used[gpu];
            if(escaped!=before.used[gpu]+programBytes)
                std::fprintf(stderr,"Program retirement escaped-buffer boundary: expected=%llu actual=%llu\n",
                    static_cast<unsigned long long>(before.used[gpu]+programBytes),static_cast<unsigned long long>(escaped));
            require(escaped==before.used[gpu]+programBytes,
                    "Both externally retained native constant buffers keep their GPU reservations after program destruction");
            readBlock(vertexConstants.Get(),false); readBlock(pixelConstants.Get(),true);
            D3D11_SAMPLER_DESC after{}; sampler->GetDesc(&after);
            require(after.Filter==samplerDescription.Filter && after.AddressU==samplerDescription.AddressU,
                    "Escaped native sampler remains usable after resource-table retirement");
            ComPtr<ID3D11Device> shaderDevice; shader->GetDevice(&shaderDevice);
            require(shaderDevice.Get()==device, "Escaped native shader remains device-owned after program retirement");
            shaderDevice.Reset(); sampler.Reset(); shader.Reset();
            vertexConstants.Reset(); claw_texture_mip_contracts::completeGpuWork(context);
            require(scheduler->stats().used[gpu]==before.used[gpu]+blockBytes,
                    "Releasing one native constant block retires only its own physical GPU reservation");
            readBlock(pixelConstants.Get(),true); pixelConstants.Reset();
            claw_texture_mip_contracts::completeGpuWork(context);
            require(scheduler->stats().used[gpu]==before.used[gpu], "Last native constant release returns the exact GPU baseline");
            require(!redContext.expired() && !greenContext.expired(), "Dirty task owners remain visible before the frame boundary");
            finishFrame(); require(redContext.expired() && greenContext.expired(), "Normal task updates retire scheduled program contexts");
            redContext=nullptr; greenContext=nullptr;
        };
        const auto cold=scheduler->stats(); cycle(); const auto baseline=scheduler->stats();
        const auto contexts=fixture.states->getStateContexts().size();
        const auto queueBytes=MaterialPublicationScope::retainedStorageBytes();
        std::fprintf(stderr,"Program retirement warm-up: SourceCpu=%llu->%llu Gpu=%llu->%llu WorkingCpu=%llu->%llu\n",
            static_cast<unsigned long long>(cold.used[source]),static_cast<unsigned long long>(baseline.used[source]),
            static_cast<unsigned long long>(cold.used[gpu]),static_cast<unsigned long long>(baseline.used[gpu]),
            static_cast<unsigned long long>(cold.used[working]),static_cast<unsigned long long>(baseline.used[working]));
        const auto started=std::chrono::steady_clock::now();
        for(u32 i=0;i<cycles;++i)
        {
            cycle(); const auto after=scheduler->stats();
            require(after.used[source]==baseline.used[source] && after.used[gpu]==baseline.used[gpu] &&
                    after.used[working]==baseline.used[working] && fixture.states->getStateContexts().size()==contexts &&
                    !MaterialPublicationScope::pendingEntryCount() && MaterialPublicationScope::retainedStorageBytes()==queueBytes,
                    "Every measured custom-program/native-resource cycle returns exact warmed ownership");
        }
        std::fprintf(stderr,"Program retirement passed: cycles=%u programs=%llu stage_buffers=%llu retained_record_draws=%llu denied_candidates=%u SourceCpu=%llu Gpu=%llu WorkingCpu=%llu elapsed_seconds=%.6f\n",
            cycles,static_cast<unsigned long long>(cycles)*2,static_cast<unsigned long long>(cycles)*4,
            static_cast<unsigned long long>(cycles)*4,cycles,static_cast<unsigned long long>(baseline.used[source]),
            static_cast<unsigned long long>(baseline.used[gpu]),static_cast<unsigned long long>(baseline.used[working]),
            std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count());
    }
}
