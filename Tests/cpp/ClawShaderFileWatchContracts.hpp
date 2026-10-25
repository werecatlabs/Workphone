#pragma once
#include "ClawMaterialReloadContracts.hpp"
#include "../../Engine/cpp/Source/WPGraphics/MaterialShaderFilesDX11.hpp"
#include <Workphone/Core/StringUtil.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <cmath>

namespace claw_shader_file_watch_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using claw_material_reload_contracts::require;

    struct Files
    {
        std::filesystem::path parent = std::filesystem::temp_directory_path();
        std::filesystem::path root = parent / ("wp-shader-watch-" + std::string(StringUtil::getUUID().c_str()));
        Files()
        {
            std::filesystem::create_directories(root / "includes" / "nested");
            std::filesystem::create_directories(root / "variants");
        }
        ~Files()
        {
            // Restrict recursive fixture cleanup to its explicit temporary root.
            std::error_code ignored;
            const auto resolved = std::filesystem::weakly_canonical(root,ignored);
            const auto sameParent = !ignored && std::filesystem::equivalent(resolved.parent_path(),parent,ignored);
            if(!ignored && sameParent && resolved.filename().string().find("wp-shader-watch-") == 0)
                std::filesystem::remove_all(root,ignored);
        }
        void write(const std::filesystem::path &relative, const char *source)
        {
            std::ofstream output(root / relative,std::ios::binary | std::ios::trunc);
            require(bool(output),"shader file fixture opens its source");
            output << source; output.close();
            require(bool(output),"shader file fixture writes and closes its source");
        }
        String path(const std::filesystem::path &relative) const
        { return String((root / relative).u8string().c_str()); }
        static void poll()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(275));
            MaterialProgramReloadDX11::UpdateAll();
        }
    };

    inline void admissionAndValidation(void *device,MaterialShaderParameterLayout::Ptr layout,Files &files)
    {
        auto scheduler=WorldStreamingScheduler::shared();
        const auto source=size_t(StreamMemoryClass::SourceCpu),gpu=size_t(StreamMemoryClass::Gpu);
        const auto c=[](const char *p) { return reinterpret_cast<const wp_c8 *>(p); };
        const char *plain="cbuffer Values:register(b4){float4 Tint;}float4 main():SV_TARGET{return Tint;}";
        const char *included="#include \"cold-budget.hlsli\"\ncbuffer Values:register(b4){float4 Tint;}"
            "float4 main():SV_TARGET{return Tint*budgetColour();}";
        files.write("budget-pixel.hlsl",plain);
        MaterialShaderParametersInstance values;values.Reset(layout);
        values.SetColour(values.FindParameter("Tint"),ColourF::White);
        const auto baseline=scheduler->stats();
        {
            claw_material_reload_contracts::BudgetLimit limit(StreamMemoryClass::SourceCpu,baseline.used[source]);
            bool denied=false;
            try { workphone::render::detail::ShaderFilesDX11 cold(String(),files.path("budget-pixel.hlsl")); }
            catch(const std::bad_alloc &) { denied=true; }
            require(denied && scheduler->stats().used[source]==baseline.used[source],
                "cold watcher admission fails before retaining unbudgeted metadata");
        }
        {
            String error;
            auto isolated=MaterialProgramReloadDX11::CreateFromFiles(device,layout,String(),files.path("budget-pixel.hlsl"),1,error);
            require(isolated && isolated->HasProgram(),error.empty()?"isolated file watcher creates":error.c_str());
            auto binding=isolated->Attach(values);
            auto retained=binding->GetPublication();
            require(bool(retained),"file watcher admission fixture publishes last-good values");
            const auto generation=isolated->GetGeneration(),attempts=isolated->GetAttemptCount();
            // A new dependency and its include block fit, but its cold source
            // buffer exceeds headroom. Neither root is manually touched in C.
            const auto large=std::string(64u<<10,' ')+"float4 budgetColour(){return float4(0,1,0,1);}";
            files.write("cold-budget.hlsli",large.c_str());files.write("budget-pixel.hlsl",included);
            const auto before=scheduler->stats();
            {
                claw_material_reload_contracts::BudgetLimit limit(StreamMemoryClass::SourceCpu,before.used[source]+(32u<<10));
                std::this_thread::sleep_for(std::chrono::milliseconds(275));isolated->Update();
                require(binding->GetPublication()==retained && isolated->GetGeneration()==generation && isolated->GetLastError()[0],
                    "cold include admission denial retains the complete last-good publication");
                const auto denied=scheduler->stats();
                require(denied.used[source]<=before.used[source]+(32u<<10) && denied.used[gpu]==before.used[gpu] &&
                    denied.rejectedReservations[source]>before.rejectedReservations[source],
                    "new dependency metadata stays admitted and denied include buffers unwind without GPU growth");
                require(isolated->GetAttemptCount()==attempts+1,"cold include denial attempts the changed graph once");
                binding->GetPublication();
                require(isolated->GetAttemptCount()==attempts+1 && scheduler->stats().used[source]==denied.used[source],
                    "unchanged denied dependency graph does not allocate or compile repeatedly");
            }
            auto recovered=binding->GetPublication();
            require(recovered && recovered!=retained && isolated->GetGeneration()==generation+1 && !isolated->GetLastError()[0],
                "restoring SourceCpu headroom automatically recompiles the cold include graph");
            recovered.reset();binding.reset();isolated.reset();
            require(scheduler->stats().used[source]>baseline.used[source] && scheduler->stats().used[gpu]>baseline.used[gpu],
                "last-good retained publication outlives its watcher metadata");
            retained.reset();
        }
        require(scheduler->stats().used[source]==baseline.used[source] && scheduler->stats().used[gpu]==baseline.used[gpu],
            "file coordinator metadata, control blocks, include blocks and retained programs retire exact leases");
        {
            workphone::render::detail::ShaderFilesDX11 probe(String(),files.path("budget-pixel.hlsl"));
            std::unique_ptr<wp_shader,decltype(&wp_shader_destroy)> shader(
                wp_shader_create(WORKPHONE_SHADER_TYPE_PIXEL,WORKPHONE_SHADER_LANGUAGE_HLSL,c(""),c("main")),&wp_shader_destroy);
            require(bool(shader),"file candidate validation fixture creates an authoring stage");
            Microsoft::WRL::ComPtr<ID3DBlob> bytecode;String error;
            probe.BeginCandidate();require(probe.Compile(shader.get(),1,bytecode,error),error.c_str());
            require(probe.ValidateCandidate(),"unchanged compiled dependency graph validates");
            files.write("cold-budget.hlsli","float4 budgetColour(){return float4(0,0,1,1);}");
            require(!probe.ValidateCandidate(),"include edits after compilation invalidate the entire candidate");
            bytecode.Reset();probe.BeginCandidate();
            require(probe.Compile(shader.get(),1,bytecode,error) && probe.ValidateCandidate(),
                "a fresh candidate validates after capturing the changed dependency");
            probe.CommitCandidate();
        }
        require(scheduler->stats().used[source]==baseline.used[source] && scheduler->stats().used[gpu]==baseline.used[gpu],
            "direct dependency validation retires every watcher and include allocation");
    }

    inline bool run(ClawRendererDX11 &renderer)
    {
        struct Restore
        {
            ClawRendererDX11 &renderer;
            ~Restore() { renderer.setCamera(nullptr); renderer.setRenderTarget(nullptr); }
        } restore{renderer};
        try
        {
            claw_foliage_scene_contracts::Fixture services;
            Files files;
            const char *vertex =
                "cbuffer Transform:register(b0){row_major float4x4 Mvp;}"
                "float4 main(float3 p:POSITION):SV_POSITION{return mul(Mvp,float4(p,1));}";
            const char *shiftedVertex =
                "cbuffer Transform:register(b0){row_major float4x4 Mvp;}"
                "float4 main(float3 p:POSITION):SV_POSITION{return mul(Mvp,float4(p+float3(.5,0,0),1));}";
            const char *pixel =
                "#include \"includes/color.hlsli\"\n"
                "cbuffer Values:register(b4){float4 Tint;}"
                "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*shadeColour();}";
            const char *scalarPixel =
                "#include \"includes/color.hlsli\"\n"
                "cbuffer Values:register(b9){float4 Tint:packoffset(c1);float Gain:packoffset(c2.x);}"
                "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*shadeColour()+float4(Gain,0,0,0);}";
            const char *variantPixel =
                "#include \"../includes/color.hlsli\"\n"
                "cbuffer Values:register(b10){float4 Tint:packoffset(c1);float Gain:packoffset(c2.x);}"
                "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*float4(shadeColour().brg,1)+float4(Gain,0,0,0);}";
            const char *red = "float4 shadeColour(){return float4(1,0,0,1);}";
            const char *green = "float4 shadeColour(){return float4(0,1,0,1);}";
            const char *blue = "float4 shadeColour(){return float4(0,0,1,1);}";
            files.write("vertex.hlsl",vertex); files.write("pixel.hlsl",pixel);
            files.write("includes/color.hlsli","#include \"nested/value.hlsli\"\n");
            files.write("includes/nested/value.hlsli",red);
            auto target = make_ptr<ClawRenderTarget>(); target->setSize({64,64});
            renderer.setRenderTarget(target); renderer.setCamera(nullptr); renderer.setViewport(nullptr);
            renderer.disableShadows();
            auto native = wp_renderer_get_dx11(renderer.getNativeRenderer());
            auto material = make_ptr<ClawMaterial>();
            auto technique = make_ptr<ClawMaterialTechnique>(); technique->setMaterial(material.get()); technique->createPass();
            material->setTechniques({technique}); material->setCullMode(1); material->setBlendMode(0);
            material->setMaterialType(MaterialType::UI);
            material->setShaderParameters({{MaterialShaderParameterType::Colour,"Tint"}},"FileWatchContract");
            auto &values = material->getShaderParameters();
            const auto initialLayout = values.GetLayout();
            const auto initialTint = values.FindParameter("Tint");
            const ColourF tint(.75f,.5f,.25f,1);
            values.SetColour(initialTint,tint);
            admissionAndValidation(wp_renderer_dx11_get_device(native),initialLayout,files);
            String error;
            auto reload = MaterialProgramReloadDX11::CreateFromFiles(wp_renderer_dx11_get_device(native),
                initialLayout,files.path("vertex.hlsl"),files.path("pixel.hlsl"),1,error);
            require(bool(reload),error.empty()?"shader file coordinator creates":error.c_str());
            require(reload->HasProgram(),reload->GetLastError());
            material->setRuntimeReload(reload);
            claw_material_reload_contracts::Mesh mesh; mesh.setMaterial(material);
            const auto capture = [&](ClawMaterialDrawRecords &records) {
                records.capture(&mesh); renderer.prepareMaterialRecords(records);
                require(records.find(&mesh)!=nullptr,"file shader publication captures its mesh");
            };
            const auto draw = [&](const ClawMaterialDrawRecords &records,int channel,bool shifted=false) {
                renderer.clear(ColourF::Black);
                require(renderer.renderMeshRecords(&mesh,Matrix4F::identity(),records,*records.find(&mesh)),
                    "retained file shader draw remains valid");
                const auto pixels = claw_foliage_contracts::pixels(native);
                const auto position=(32u*64u+(shifted?48u:32u))*4u;
                const int expected[]={191,128,64};
                for(int c=0;c<3;++c)
                    require(c==channel ? std::abs(int(pixels[position+c])-expected[c])<18 : pixels[position+c]<10,
                        "watched shader output matches its active color and material values");
                const auto other=(32u*64u+(shifted?32u:48u))*4u;
                require(pixels[other]<10 && pixels[other+1]<10 && pixels[other+2]<10,
                    "watched vertex shader footprint is coherent with its pixel stage");
            };
            const auto id=reload->GetId();
            auto status=MaterialProgramReloadDX11::EditorStatus(id);
            require(id && status->getPropertyAsBool("available") && status->getPropertyAsBool("success") &&
                status->getPropertyAsBool("watching"),"Editor reads initial active file generation without compiling");
            {
                auto sibling=make_ptr<ClawMaterial>();
                sibling->setShaderID(material->getShaderID());sibling->getShaderParameters()=values;
                require(material->requestRuntimeShaderFiles(files.path("vertex.hlsl"),files.path("pixel.hlsl")) &&
                    sibling->requestRuntimeShaderFiles(files.path("vertex.hlsl"),files.path("pixel.hlsl")),
                    "materials can configure a shared file coordinator");
                ClawMaterial::processRuntimeShaderReloads(renderer);
                MaterialProgramReloadDX11::UpdateAll();
                require(material->setRuntimeShaderWatching(false),"first subscriber can pause shared file watching");
                MaterialProgramReloadDX11::UpdateAll();
                require(!material->getRuntimeShaderReloadStatus()->getPropertyAsBool("watching") &&
                    !sibling->getRuntimeShaderReloadStatus()->getPropertyAsBool("watching"),
                    "all subscribers report the applied shared watch pause");
                require(sibling->setRuntimeShaderWatching(true),"second subscriber can resume shared file watching");
                MaterialProgramReloadDX11::UpdateAll();
                require(material->getRuntimeShaderReloadStatus()->getPropertyAsBool("watching") &&
                    sibling->getRuntimeShaderReloadStatus()->getPropertyAsBool("watching"),
                    "all subscribers report the applied shared watch resumption");
            }
            require(!MaterialProgramReloadDX11::QueueReload(0) && !MaterialProgramReloadDX11::QueueWatching(0,true) &&
                !MaterialProgramReloadDX11::EditorStatus(0)->getPropertyAsBool("available"),"expired/unknown Editor identities fail explicitly");
            ClawMaterialDrawRecords original; capture(original); draw(original,0);
            require(!values.FindParameter("Mvp").IsValid(),"transform-owned b0 does not become a material parameter");

            auto generation=reload->GetGeneration();
            files.write("includes/nested/value.hlsli",green); Files::poll();
            require(reload->GetGeneration()>generation && reload->GetLastError()[0]==0,
                "nested relative include edits reload automatically without touching descriptors");
            ClawMaterialDrawRecords changedInclude; capture(changedInclude); draw(changedInclude,1);
            draw(original,0);

            generation=reload->GetGeneration();
            const auto good=material->getRuntimePublication();
            files.write("includes/nested/value.hlsli","This nested include does not compile"); Files::poll();
            require(reload->GetGeneration()==generation && material->getRuntimePublication()==good && reload->GetLastError()[0],
                "invalid watched include keeps the complete last-good generation");
            status=MaterialProgramReloadDX11::EditorStatus(id);
            require(!status->getPropertyAsBool("success") && !status->getProperty("diagnostic").empty(),
                "Editor status exposes the actual watched-include compiler error");
            draw(changedInclude,1);
            files.write("includes/nested/value.hlsli",blue); Files::poll();
            require(reload->GetGeneration()>generation && reload->GetLastError()[0]==0,
                "fixing a watched include automatically recovers");
            ClawMaterialDrawRecords recovered; capture(recovered); draw(recovered,2);

            generation=reload->GetGeneration();
            files.write("includes/color.hlsli","#include \"nested/created-later.hlsli\"\n"); Files::poll();
            require(reload->GetGeneration()==generation && reload->GetLastError()[0],
                "missing include leaves the last-good file generation active");
            draw(recovered,2);
            files.write("includes/nested/created-later.hlsli",red); Files::poll();
            require(reload->GetGeneration()>generation && reload->GetLastError()[0]==0,
                "creating a previously missing include automatically retries the failed graph");
            ClawMaterialDrawRecords createdInclude; capture(createdInclude); draw(createdInclude,0);

            require(MaterialProgramReloadDX11::QueueWatching(id,false),"Editor can queue watching pause");
            require(MaterialProgramReloadDX11::EditorStatus(id)->getPropertyAsBool("pending"),
                "watch control reports queued rather than completed work");
            MaterialProgramReloadDX11::UpdateAll();
            require(!MaterialProgramReloadDX11::EditorStatus(id)->getPropertyAsBool("watching"),"owner applies watch pause");
            generation=reload->GetGeneration(); const auto attempts=reload->GetAttemptCount();
            files.write("includes/nested/created-later.hlsli",green); Files::poll();
            require(reload->GetGeneration()==generation && reload->GetAttemptCount()==attempts,
                "paused file watching does not compile changed files");
            draw(createdInclude,0);
            require(MaterialProgramReloadDX11::QueueReload(id),"Editor queues manual reload while watching is paused");
            status=MaterialProgramReloadDX11::EditorStatus(id);
            require(status->getPropertyAsBool("pending") && reload->GetGeneration()==generation,
                "reading a queued status does not invoke the compiler");
            MaterialProgramReloadDX11::UpdateAll();
            require(reload->GetGeneration()>generation && !MaterialProgramReloadDX11::EditorStatus(id)->getPropertyAsBool("watching"),
                "manual reload works while watching remains paused");
            ClawMaterialDrawRecords manual; capture(manual); draw(manual,1);

            generation=reload->GetGeneration();
            files.write("pixel.hlsl",scalarPixel); files.write("vertex.hlsl",shiftedVertex);
            require(MaterialProgramReloadDX11::QueueWatching(id,true),"Editor queues watcher resumption");
            Files::poll();
            require(reload->GetGeneration()>generation && reload->GetLastError()[0]==0,
                "changed shader roots and CPU schema publish together after watching resumes");
            require(values.GetLayout()!=initialLayout && !values.IsHandleValid(initialTint,MaterialShaderParameterType::Colour,sizeof(ColourF)),
                "file schema migration invalidates prior runtime parameter handles");
            const auto migratedTint=values.GetColour(values.FindParameter("Tint"));
            require(migratedTint.r==tint.r && migratedTint.g==tint.g && migratedTint.b==tint.b && migratedTint.a==tint.a,
                "compatible material values survive reflected file schema migration");
            const auto gain=values.FindParameter("Gain");
            require(values.IsHandleValid(gain,MaterialShaderParameterType::Scalar,sizeof(f32)) && values.GetScalar(gain)==0,
                "a newly reflected scalar material field starts from its canonical default");
            ClawMaterialDrawRecords migrated; capture(migrated); draw(migrated,1,true);
            draw(original,0); draw(manual,1);

            files.write("variants/pixel.hlsl",variantPixel);
            generation=reload->GetGeneration();
            require(reload->SetStageFiles(files.path("vertex.hlsl"),files.path("variants/pixel.hlsl"),error),
                error.empty()?"shader root paths can be replaced":error.c_str());
            MaterialProgramReloadDX11::UpdateAll();
            require(reload->GetGeneration()>generation && reload->GetLastError()[0]==0,
                "changing shader root paths rebuilds relative include resolution and packing maps");
            ClawMaterialDrawRecords movedRoot; capture(movedRoot); draw(movedRoot,2,true);
            status=MaterialProgramReloadDX11::EditorStatus(id);
            require(status->getProperty("pixelFile").find("variants")!=String::npos,
                "Editor status identifies the currently watched root file");
            generation=reload->GetGeneration();
            files.write("pixel.hlsl","This old root is no longer watched"); Files::poll();
            require(reload->GetGeneration()==generation && reload->GetLastError()[0]==0,
                "retired root files do not trigger the replacement dependency graph");
            draw(movedRoot,2,true); draw(original,0);
            {
                auto clone=make_ptr<ClawMaterial>();material->copyRuntimeShaderTo(*clone);
                const auto saved=clone->getProperties();
                require(saved->getProperty("runtimeShaderPixelFile").find("variants")!=String::npos &&
                    saved->getProperty("runtimeShaderVertexFile")==files.path("vertex.hlsl") &&
                    saved->getPropertyAsBool("runtimeShaderWatching"),
                    "cloned file material saves the active roots and shared watcher policy");
                require(clone->getRuntimePublication()!=nullptr && !clone->getRuntimeShaderReloadStatus()->getPropertyAsBool("pending"),
                    "cloning shares the ready shader without queueing a redundant compile");
            }
            std::fprintf(stderr,"Shader file watching: nested include changes, missing/error recovery, pause/manual control, reflected migration, root replacement and retained generations passed\n");
            return true;
        }
        catch(const std::exception &error)
        {
            std::fprintf(stderr,"FAIL: shader file watch contracts: %s\n",error.what());
            return false;
        }
    }
}
