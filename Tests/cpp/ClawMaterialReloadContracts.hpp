#pragma once

#include "ClawFoliageContracts.hpp"
#include "ClawFoliageSceneContracts.hpp"
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialDrawRecords.hpp>
#include <WPGraphics/MaterialProgramReloadDX11.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <workphone_graphics_mesh.h>
#include <cstdio>
#include <cstring>
#include <memory>

namespace claw_material_reload_contracts
{
    using namespace workphone;
    using namespace workphone::render;

    inline void require(bool condition, const char *message)
    {
        if(!condition) throw std::runtime_error(message);
    }
    inline const wp_c8 *text(const char *value)
    { return reinterpret_cast<const wp_c8 *>(value); }

    class Mesh : public ClawMesh
    {
    public:
        Mesh()
        {
            m_mesh = wp_graphics_mesh_create();
            require(m_mesh != nullptr,"reload fixture native mesh allocates");
            const wp_graphics_mesh_vertex_pnt vertices[] = {
                {{-.2f,-.2f,.5f},{0,0,-1},{0,0}},
                {{.2f,-.2f,.5f},{0,0,-1},{1,0}},
                {{0,.2f,.5f},{0,0,-1},{.5f,1}}};
            const wp_u16 indices[] = {0,1,2};
            wp_graphics_mesh_set_vertices(m_mesh,WORKPHONE_VERTEX_FORMAT_PNT,vertices,3);
            wp_graphics_mesh_set_indices_u16(m_mesh,indices,3);
        }
    };

    struct BudgetLimit
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        StreamMemoryClass kind;
        u64 original;
        bool active = true;
        BudgetLimit(StreamMemoryClass memory, u64 limit) : kind(memory),
            original(scheduler->stats().limits[size_t(memory)])
        {
            String error;
            require(scheduler->setBudget(kind,limit,error),"reload fixture budget limit applies");
        }
        void restore()
        {
            if(active)
            {
                String ignored;
                scheduler->setBudget(kind,original,ignored);
                active = false;
            }
        }
        ~BudgetLimit() { restore(); }
        BudgetLimit(const BudgetLimit &) = delete;
        BudgetLimit &operator=(const BudgetLimit &) = delete;
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
            claw_foliage_scene_contracts::Fixture fixture;
            auto target = make_ptr<ClawRenderTarget>(); target->setSize({64,64});
            renderer.setRenderTarget(target); renderer.setViewport(nullptr); renderer.setCamera(nullptr);
            renderer.disableShadows();
            auto native = wp_renderer_get_dx11(renderer.getNativeRenderer());
            auto device = wp_renderer_dx11_get_device(native);
            using Type = MaterialShaderParameterType;
            const auto material = [] {
                auto value = make_ptr<ClawMaterial>();
                auto technique = make_ptr<ClawMaterialTechnique>();
                technique->setMaterial(value.get()); technique->createPass();
                value->setTechniques({technique}); value->setCullMode(1); value->setBlendMode(0);
                value->setMaterialType(MaterialType::UI);
                value->setShaderParameters({{Type::Colour,"Tint"}},"ReloadContract");
                return value;
            };
            auto first = material(), second = material();
            const auto layout = first->getShaderParameters().GetLayout();
            second->getShaderParameters().Reset(layout);
            first->getShaderParameters().SetColour(first->getShaderParameters().FindParameter("Tint"),ColourF::White);
            second->getShaderParameters().SetColour(second->getShaderParameters().FindParameter("Tint"),ColourF(.5f,.5f,.5f,1));
            require(layout && second->getShaderParameters().GetLayout() == layout,"reload subscribers share one fixed CPU schema");

            const char *vertexA =
                "cbuffer Transform:register(b0){row_major float4x4 Mvp;}"
                "float4 main(float3 p:POSITION):SV_POSITION{return mul(Mvp,float4(p,1));}";
            const char *vertexB =
                "cbuffer Transform:register(b0){row_major float4x4 Mvp;}"
                "float4 main(float3 p:POSITION):SV_POSITION{return mul(Mvp,float4(p+float3(.5,0,0),1));}";
            const char *pixelA =
                "cbuffer Values:register(b4){float4 Tint;}"
                "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*float4(1,0,0,1);}";
            const char *pixelB =
                "cbuffer Values:register(b9){float4 Tint:packoffset(c1);}"
                "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*float4(0,1,0,1);}";
            const char *pixelWrongSchema =
                "cbuffer Values:register(b9){float4 Alien;}"
                "float4 main(float4 p:SV_POSITION):SV_TARGET{return Alien;}";
            using Shader = std::unique_ptr<wp_shader,decltype(&wp_shader_destroy)>;
            using Program = std::unique_ptr<wp_shader_program,decltype(&wp_shader_program_destroy)>;
            Shader vertex(wp_shader_create(WORKPHONE_SHADER_TYPE_VERTEX,WORKPHONE_SHADER_LANGUAGE_HLSL,
                text(vertexA),text("main")),&wp_shader_destroy);
            Shader pixel(wp_shader_create(WORKPHONE_SHADER_TYPE_PIXEL,WORKPHONE_SHADER_LANGUAGE_HLSL,
                text(pixelA),text("main")),&wp_shader_destroy);
            Program authoring(wp_shader_program_create(),&wp_shader_program_destroy);
            require(vertex && pixel && authoring,"reload fixture authoring descriptors allocate");
            wp_shader_set_profile(vertex.get(),text("vs_5_0"));
            wp_shader_set_profile(pixel.get(),text("ps_5_0"));
            wp_shader_program_attach(authoring.get(),vertex.get());
            wp_shader_program_attach(authoring.get(),pixel.get());
            require(wp_shader_program_register_uniform(authoring.get(),text("Tint"),WORKPHONE_UNIFORM_TYPE_VEC4) >= 0,
                "reload fixture registers canonical Tint");
            MaterialProgramReflectionSelection vertexSelection; // b0 is geometry-owned.
            String error;
            auto reload = MaterialProgramReloadDX11::Create(device,layout,authoring.get(),1,error,nullptr,&vertexSelection);
            if(!reload) throw std::runtime_error(error.c_str());
            second->setRuntimeReload(reload); first->setRuntimeReload(reload);
            Mesh firstMesh, secondMesh; firstMesh.setMaterial(first); secondMesh.setMaterial(second);
            const auto capture = [&](ClawMaterialDrawRecords &records) {
                records.capture(&firstMesh); records.capture(&secondMesh); renderer.prepareMaterialRecords(records);
            };
            const auto expect = [&](u32 x, int channel, bool dim, const char *message) {
                const auto pixels = claw_foliage_contracts::pixels(native);
                const auto offset = (32u*64u+x)*4u;
                bool matches = true;
                for(int i=0;i<3;++i)
                    matches &= i == channel ? (dim ? pixels[offset+i] > 90 && pixels[offset+i] < 170 : pixels[offset+i] > 220)
                                             : pixels[offset+i] < 10;
                require(matches,message);
            };
            const auto draw = [&](Mesh &mesh, const ClawMaterialDrawRecords &records, bool shifted, bool dim) {
                renderer.clear(ColourF::Black);
                require(renderer.renderMeshRecords(&mesh,Matrix4F::identity(),records,*records.find(&mesh)),
                    "retained reload publication remains valid");
                expect(shifted ? 48 : 32,shifted ? 1 : 0,dim,"active reload program uses coherent shader pair and material packing");
                expect(shifted ? 32 : 48,-1,false,"other shader-generation footprint is absent");
            };

            ClawMaterialDrawRecords original; capture(original);
            auto originalFirst = first->getRuntimePublication(), originalSecond = second->getRuntimePublication();
            require(originalFirst && originalSecond && reload->GetGeneration() > 0,"initial reload generation publishes both subscribers");
            draw(firstMesh,original,false,false); draw(secondMesh,original,false,true);

            // A valid new VS must not appear if the accompanying PS fails.
            const auto generationA = reload->GetGeneration(), attemptsA = reload->GetAttemptCount();
            wp_shader_set_source(vertex.get(),text(vertexB));
            wp_shader_set_source(pixel.get(),text("This is not valid HLSL"));
            ClawMaterialDrawRecords invalidSource; capture(invalidSource);
            require(reload->GetGeneration() == generationA && reload->GetAttemptCount() == attemptsA+1 &&
                first->getRuntimePublication() == originalFirst && second->getRuntimePublication() == originalSecond &&
                reload->GetLastError()[0],"syntax failure keeps complete last-good shader/maps/publications and reports diagnostics");
            const auto memoizedAttempts = reload->GetAttemptCount(), memoizedCompiles = reload->GetCompileCount();
            for(int i=0;i<3;++i) { first->getRuntimePublication(); second->getRuntimePublication(); }
            require(reload->GetAttemptCount() == memoizedAttempts && reload->GetCompileCount() == memoizedCompiles,
                "unchanged failed revision does not recompile on material capture");
            draw(firstMesh,invalidSource,false,false); draw(secondMesh,invalidSource,false,true);
            reload->RequestRetry();
            require(first->getRuntimePublication() == originalFirst &&
                reload->GetAttemptCount() == memoizedAttempts+1 && reload->GetGeneration() == generationA &&
                reload->GetLastError()[0],"explicit retry reattempts a failed revision without disturbing last-good output");

            wp_shader_set_source(pixel.get(),text(pixelB));
            ClawMaterialDrawRecords shifted; capture(shifted);
            require(reload->GetGeneration() == generationA+1 && reload->GetLastError()[0] == 0 &&
                first->getRuntimePublication() != originalFirst && second->getRuntimePublication() != originalSecond,
                "fixed source automatically commits all subscribers without manual update");
            draw(firstMesh,original,false,false); draw(firstMesh,shifted,true,false); draw(firstMesh,original,false,false);
            draw(secondMesh,shifted,true,true);

            const auto generationB = reload->GetGeneration();
            auto shiftedFirst = first->getRuntimePublication(), shiftedSecond = second->getRuntimePublication();
            wp_shader_set_source(pixel.get(),text(pixelWrongSchema));
            require(first->getRuntimePublication() == shiftedFirst && second->getRuntimePublication() == shiftedSecond &&
                reload->GetGeneration() == generationB && reload->GetLastError()[0],
                "reflected parameter mismatch preserves complete last-good generation");
            draw(firstMesh,shifted,true,false);

            // Subscriber A can pack the candidate; incompatible B prevents every
            // subscriber commit. B defers its live capture, but retained B draws.
            second->getShaderParameters().Reset({{Type::Scalar,"Incompatible"}});
            wp_shader_set_source(vertex.get(),text(vertexA)); wp_shader_set_source(pixel.get(),text(pixelA));
            require(first->getRuntimePublication() == shiftedFirst && !second->getRuntimePublication() &&
                reload->GetGeneration() == generationB && reload->GetLastError()[0],
                "one incompatible subscriber blocks publication for the whole cohort");
            draw(secondMesh,shifted,true,true);
            second->getShaderParameters().Reset(layout);
            second->getShaderParameters().SetColour(second->getShaderParameters().FindParameter("Tint"),ColourF(.5f,.5f,.5f,1));
            ClawMaterialDrawRecords restored; capture(restored);
            require(reload->GetGeneration() == generationB+1 && reload->GetLastError()[0] == 0,
                "restored subscriber automatically retries the pending source revision");
            draw(firstMesh,restored,false,false); draw(secondMesh,restored,false,true);

            // Required-stage detachment is another authoring edit, and keeps the
            // old generation until the complete stage set is present again.
            const auto beforeDetach = reload->GetGeneration();
            auto beforeDetachFirst = first->getRuntimePublication();
            wp_shader_program_detach(authoring.get(),WORKPHONE_SHADER_TYPE_PIXEL);
            require(first->getRuntimePublication() == beforeDetachFirst && reload->GetGeneration() == beforeDetach &&
                reload->GetLastError()[0],"missing required pixel stage retains last-good runtime");
            wp_shader_program_attach(authoring.get(),pixel.get());
            first->getRuntimePublication();
            require(reload->GetGeneration() == beforeDetach+1 && reload->GetLastError()[0] == 0,
                "restoring stage attachment automatically retries the complete program");

            auto scheduler = WorldStreamingScheduler::shared();
            const auto source = size_t(StreamMemoryClass::SourceCpu), gpu = size_t(StreamMemoryClass::Gpu);
            auto previousFirst = first->getRuntimePublication(), previousSecond = second->getRuntimePublication();
            wp_shader_set_source(vertex.get(),text(vertexB)); wp_shader_set_source(pixel.get(),text(pixelB));
            auto baseline = scheduler->stats();
            const auto beforeCpuGeneration = reload->GetGeneration(), beforeCpuAttempts = reload->GetAttemptCount();
            {
                BudgetLimit limit(StreamMemoryClass::SourceCpu,baseline.used[source]);
                require(first->getRuntimePublication() == previousFirst && second->getRuntimePublication() == previousSecond &&
                    reload->GetGeneration() == beforeCpuGeneration,"CPU admission failure preserves every last-good subscriber");
                const auto denied = scheduler->stats();
                require(denied.used[source] == baseline.used[source] && denied.used[gpu] == baseline.used[gpu] &&
                    denied.rejectedReservations[source] > baseline.rejectedReservations[source],
                    "failed CPU candidate leaves no source/GPU lease growth");
                require(reload->GetAttemptCount() == beforeCpuAttempts+1,"CPU denial attempts a revision once at unchanged headroom");
            }
            first->getRuntimePublication();
            require(reload->GetGeneration() == beforeCpuGeneration+1 && reload->GetLastError()[0] == 0,
                "restored CPU headroom automatically retries a denied candidate");
            ClawMaterialDrawRecords cpuRecovered; capture(cpuRecovered);
            draw(firstMesh,cpuRecovered,true,false); draw(secondMesh,cpuRecovered,true,true);

            previousFirst = first->getRuntimePublication(); previousSecond = second->getRuntimePublication();
            wp_shader_set_source(vertex.get(),text(vertexA)); wp_shader_set_source(pixel.get(),text(pixelA));
            baseline = scheduler->stats();
            const auto beforeGpuGeneration = reload->GetGeneration(), beforeGpuAttempts = reload->GetAttemptCount();
            {
                BudgetLimit limit(StreamMemoryClass::Gpu,baseline.used[gpu]);
                require(first->getRuntimePublication() == previousFirst && second->getRuntimePublication() == previousSecond &&
                    reload->GetGeneration() == beforeGpuGeneration,"GPU admission failure preserves every last-good subscriber");
                const auto denied = scheduler->stats();
                require(denied.used[source] == baseline.used[source] && denied.used[gpu] == baseline.used[gpu] &&
                    denied.rejectedReservations[gpu] > baseline.rejectedReservations[gpu],
                    "failed GPU candidate unwinds all staged CPU/GPU ownership");
                require(reload->GetAttemptCount() == beforeGpuAttempts+1,"GPU denial attempts a revision once at unchanged headroom");
            }
            first->getRuntimePublication();
            require(reload->GetGeneration() == beforeGpuGeneration+1 && reload->GetLastError()[0] == 0,
                "restored GPU headroom automatically retries a denied candidate");
            ClawMaterialDrawRecords gpuRecovered; capture(gpuRecovered);
            draw(firstMesh,gpuRecovered,false,false); draw(secondMesh,gpuRecovered,false,true);

            previousFirst = first->getRuntimePublication(); previousSecond = second->getRuntimePublication();
            baseline = scheduler->stats();
            const auto warmAttempts = reload->GetAttemptCount(), warmCompiles = reload->GetCompileCount();
            const auto warmGeneration = reload->GetGeneration();
            {
                BudgetLimit limit(StreamMemoryClass::SourceCpu,baseline.used[source]);
                for(int i=0;i<4;++i)
                {
                    require(first->getRuntimePublication() == previousFirst && second->getRuntimePublication() == previousSecond,
                        "unchanged warm capture reuses both immutable publications");
                    renderer.clear(ColourF::Black);
                    require(renderer.renderMeshRecords(&firstMesh,Matrix4F::identity(),gpuRecovered,*gpuRecovered.find(&firstMesh)),
                        "warm retained reload draw succeeds at full source budget");
                }
                const auto after = scheduler->stats();
                require(after.used[source] == baseline.used[source] &&
                    after.rejectedReservations[source] == baseline.rejectedReservations[source] &&
                    reload->GetAttemptCount() == warmAttempts && reload->GetCompileCount() == warmCompiles &&
                    reload->GetGeneration() == warmGeneration,"warm polling/draw performs no compile, candidate or SourceCpu allocation");
            }
            expect(32,0,false,"warm full-budget reload draw preserves visible output");
            // A removed subscriber must stop blocking a pending cohort reload,
            // while already captured views keep that subscriber's old program.
            const auto beforeRemoval = reload->GetGeneration();
            previousFirst = first->getRuntimePublication();
            second->getShaderParameters().Reset({{Type::Scalar,"RemovedSubscriberSchema"}});
            wp_shader_set_source(vertex.get(),text(vertexB)); wp_shader_set_source(pixel.get(),text(pixelB));
            require(first->getRuntimePublication() == previousFirst && reload->GetGeneration() == beforeRemoval &&
                reload->GetLastError()[0],"incompatible subscriber blocks a new generation before removal");
            second->setRuntimeReload(nullptr);
            first->getRuntimePublication();
            require(reload->GetGeneration() == beforeRemoval+1 && reload->GetLastError()[0] == 0,
                "removing an incompatible subscriber automatically retries the pending cohort");
            ClawMaterialDrawRecords remaining;
            remaining.capture(&firstMesh); renderer.prepareMaterialRecords(remaining);
            draw(firstMesh,remaining,true,false);
            draw(secondMesh,gpuRecovered,false,true);
            second->unload(nullptr);
            draw(secondMesh,gpuRecovered,false,true);
            // Independent, never-submitted generations must release every
            // metadata/definition/buffer lease after their last publication dies.
            MaterialShaderParametersInstance isolatedValues;
            isolatedValues.Reset(layout);
            isolatedValues.SetColour(isolatedValues.FindParameter("Tint"),ColourF::White);
            const auto retirementBaseline = scheduler->stats();
            {
                auto isolated = MaterialProgramReloadDX11::Create(device,layout,authoring.get(),1,error,nullptr,&vertexSelection);
                if(!isolated) throw std::runtime_error(error.c_str());
                auto binding = isolated->Attach(isolatedValues);
                auto retained = binding->GetPublication();
                require(bool(retained),"isolated reload subscription publishes initial values");
                const auto isolatedGeneration = isolated->GetGeneration();
                wp_shader_set_source(vertex.get(),text(vertexA)); wp_shader_set_source(pixel.get(),text(pixelA));
                auto replacement = binding->GetPublication();
                require(replacement && replacement != retained && isolated->GetGeneration() == isolatedGeneration+1,
                    "isolated reload produces a separately retained generation");
                replacement.reset(); binding.reset(); isolated.reset();
                const auto stillRetained = scheduler->stats();
                require(stillRetained.used[source] > retirementBaseline.used[source] &&
                    stillRetained.used[gpu] > retirementBaseline.used[gpu],
                    "retained publication keeps its private definitions and constant buffers admitted");
                retained.reset();
            }
            wp_shader_set_source(vertex.get(),text(vertexB)); wp_shader_set_source(pixel.get(),text(pixelB));
            const auto retired = scheduler->stats();
            require(retired.used[source] == retirementBaseline.used[source] &&
                retired.used[gpu] == retirementBaseline.used[gpu],
                "last isolated publication retires exact CPU/GPU leases and subscription metadata");
            // The coordinator retains source descriptors, and draw records retain
            // private runtime generations independently of authoring wrappers.
            authoring.reset(); vertex.reset(); pixel.reset();
            previousFirst = first->getRuntimePublication();
            const auto retainedSourceAttempts = reload->GetAttemptCount();
            require(first->getRuntimePublication() == previousFirst &&
                reload->GetAttemptCount() == retainedSourceAttempts,
                "coordinator retains authoring descriptors after caller owners retire");
            draw(firstMesh,remaining,true,false); draw(firstMesh,original,false,false);
            std::fprintf(stderr,"Atomic material shader reload: automatic revision capture, coherent stages/maps, retained generations, cohort rollback, diagnostics, CPU/GPU recovery and warm reuse passed\n");
            return true;
        }
        catch(const std::exception &error)
        {
            std::fprintf(stderr,"FAIL: atomic material shader reload: %s\n",error.what());
            return false;
        }
    }
}
