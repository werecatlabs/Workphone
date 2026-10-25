#pragma once
#include "ClawMaterialReloadContracts.hpp"
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
