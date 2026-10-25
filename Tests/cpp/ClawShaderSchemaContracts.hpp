#pragma once
#include "ClawFoliageContracts.hpp"
#include "ClawFoliageSceneContracts.hpp"
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialDrawRecords.hpp>
#include <WPGraphics/MaterialProgramReloadDX11.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <workphone_graphics_mesh.h>
#include <cstdio>
#include <memory>
#include <stdexcept>

namespace claw_shader_schema_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    inline void require(bool condition,const char *message)
    { if(!condition) throw std::runtime_error(message); }
    inline const wp_c8 *text(const char *value) { return reinterpret_cast<const wp_c8 *>(value); }
    class Mesh : public ClawMesh
    {
    public:
        Mesh()
        {
            m_mesh=wp_graphics_mesh_create(); require(m_mesh!=nullptr,"schema fixture mesh creates");
            const wp_graphics_mesh_vertex_pnt vertices[]={
                {{-.2f,-.2f,.5f},{0,0,-1},{0,0}},{{.2f,-.2f,.5f},{0,0,-1},{1,0}},{{0,.2f,.5f},{0,0,-1},{.5f,1}}};
            const wp_u16 indices[]={0,1,2};
            wp_graphics_mesh_set_vertices(m_mesh,WORKPHONE_VERTEX_FORMAT_PNT,vertices,3);
            wp_graphics_mesh_set_indices_u16(m_mesh,indices,3);
        }
    };
    struct Limit
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler=WorldStreamingScheduler::shared();
        StreamMemoryClass kind; u64 original;
        Limit(StreamMemoryClass value,u64 bytes) : kind(value),original(scheduler->stats().limits[size_t(value)])
        { String error; require(scheduler->setBudget(kind,bytes,error),"schema budget applies"); }
        ~Limit() { String ignored; scheduler->setBudget(kind,original,ignored); }
    };
    inline bool run(ClawRendererDX11 &renderer)
    {
        struct Restore { ClawRendererDX11 &r; ~Restore() { r.setCamera(nullptr);r.setRenderTarget(nullptr); } } restore{renderer};
        try
        {
            claw_foliage_scene_contracts::Fixture fixture;
            auto target=make_ptr<ClawRenderTarget>();target->setSize({64,64});
            renderer.setRenderTarget(target);renderer.setViewport(nullptr);renderer.setCamera(nullptr);renderer.disableShadows();
            auto native=wp_renderer_get_dx11(renderer.getNativeRenderer());
            using Type=MaterialShaderParameterType;
            const auto makeMaterial=[] {
                auto material=make_ptr<ClawMaterial>();auto technique=make_ptr<ClawMaterialTechnique>();
                technique->setMaterial(material.get());technique->createPass();material->setTechniques({technique});
                material->setCullMode(1);material->setBlendMode(0);material->setMaterialType(MaterialType::UI);
                material->setShaderParameters({{Type::Colour,"Tint"},{Type::Scalar,"Roughness"},{Type::Scalar,"Gain"}},"SchemaContracts");
                return material;
            };
            auto first=makeMaterial(), second=makeMaterial();
            const auto oldLayout=first->getShaderParameters().GetLayout();second->getShaderParameters().Reset(oldLayout);
            first->getShaderParameters().SetColour(first->getShaderParameters().FindParameter("Tint"),ColourF::White);
            second->getShaderParameters().SetColour(second->getShaderParameters().FindParameter("Tint"),ColourF(.5f,.5f,.5f,1));
            first->setRoughness(.65f);second->setRoughness(.35f);
            first->getShaderParameters().SetScalar(first->getShaderParameters().FindParameter("Gain"),.7f);
            second->getShaderParameters().SetScalar(second->getShaderParameters().FindParameter("Gain"),.7f);
            const auto oldGain=first->getShaderParameters().FindParameter("Gain");
            const auto oldTint=first->getShaderParameters().FindParameter("Tint");
            const auto oldFirstState=first->getRenderState(),oldSecondState=second->getRenderState();
            require(oldFirstState && oldSecondState && oldFirstState->roughness==.65f && oldSecondState->roughness==.35f,
                "schema owner begins with canonical roughness in both immutable pass snapshots");
            using Shader=std::unique_ptr<wp_shader,decltype(&wp_shader_destroy)>;
            using Program=std::unique_ptr<wp_shader_program,decltype(&wp_shader_program_destroy)>;
            const char *vs="cbuffer Transform:register(b0){row_major float4x4 Mvp;}"
                "float4 main(float3 p:POSITION):SV_POSITION{return mul(Mvp,float4(p,1));}";
            const char *psOld="cbuffer Values:register(b4){float4 Tint;float Roughness;float Gain;}"
                "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*float4(.5+.5*Gain,.1*Roughness,0,1);}";
            const char *psNew="cbuffer Values:register(b7){uint Mode:packoffset(c0.x);uint Gain:packoffset(c0.y);float4 Tint:packoffset(c1);}"
                "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*(Mode==0 && Gain==0?float4(0,1,0,1):float4(0,0,1,1));}";
            const char *psReadded="cbuffer Values:register(b9){float Roughness:packoffset(c0.x);uint Mode:packoffset(c0.y);"
                "uint Gain:packoffset(c0.z);float4 Tint:packoffset(c1);}"
                "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*(Mode==0 && Gain==0 && Roughness==0?float4(0,0,1,1):float4(1,0,0,1));}";
            Shader vertex(wp_shader_create(WORKPHONE_SHADER_TYPE_VERTEX,WORKPHONE_SHADER_LANGUAGE_HLSL,text(vs),text("main")),&wp_shader_destroy);
            Shader pixel(wp_shader_create(WORKPHONE_SHADER_TYPE_PIXEL,WORKPHONE_SHADER_LANGUAGE_HLSL,text(psOld),text("main")),&wp_shader_destroy);
            Program source(wp_shader_program_create(),&wp_shader_program_destroy);
            require(vertex && pixel && source,"schema authoring descriptors create");
            wp_shader_set_profile(vertex.get(),text("vs_5_0"));wp_shader_set_profile(pixel.get(),text("ps_5_0"));
            wp_shader_program_attach(source.get(),vertex.get());wp_shader_program_attach(source.get(),pixel.get());
            wp_shader_program_register_uniform(source.get(),text("Tint"),WORKPHONE_UNIFORM_TYPE_VEC4);
            wp_shader_program_register_uniform(source.get(),text("Roughness"),WORKPHONE_UNIFORM_TYPE_FLOAT);
            wp_shader_program_register_uniform(source.get(),text("Gain"),WORKPHONE_UNIFORM_TYPE_FLOAT);
            MaterialProgramReflectionSelection vertexSelection;String error;
            auto reload=MaterialProgramReloadDX11::Create(wp_renderer_dx11_get_device(native),oldLayout,source.get(),1,error,nullptr,&vertexSelection);
            if(!reload) throw std::runtime_error(error.c_str());
            second->setRuntimeReload(reload);first->setRuntimeReload(reload);
            Mesh firstMesh,secondMesh;firstMesh.setMaterial(first);secondMesh.setMaterial(second);
            const auto capture=[&](ClawMaterialDrawRecords &records) {
                records.capture(&firstMesh);records.capture(&secondMesh);renderer.prepareMaterialRecords(records);
            };
            const auto draw=[&](Mesh &mesh,const ClawMaterialDrawRecords &records,int channel,bool dim) {
                renderer.clear(ColourF::Black);
                require(renderer.renderMeshRecords(&mesh,Matrix4F::identity(),records,*records.find(&mesh)),"schema retained draw remains valid");
                const auto pixels=claw_foliage_contracts::pixels(native);const auto offset=(32u*64u+32u)*4u;
                for(int i=0;i<3;++i)
                    require(i==channel ? (dim ? pixels[offset+i]>85 && pixels[offset+i]<150 : pixels[offset+i]>180) : pixels[offset+i]<30,
                        "schema CPU values and active GPU packing agree");
            };
            ClawMaterialDrawRecords original;capture(original);draw(firstMesh,original,0,false);draw(secondMesh,original,0,true);
            const auto nextLayout=MaterialShaderParameterLayout::Create({{Type::UInt,"Mode"},{Type::UInt,"Gain"},{Type::Colour,"Tint"}});
            auto originalFirst=first->getRuntimePublication(),originalSecond=second->getRuntimePublication();
            const auto generation=reload->GetGeneration(),attempts=reload->GetAttemptCount(),compiles=reload->GetCompileCount();
            {
                auto edit=first->getShaderParameters().Edit();
                wp_shader_set_source(pixel.get(),text(psNew));reload->RequestSchema(nextLayout);
                for(int i=0;i<3;++i) require(first->getRuntimePublication()==originalFirst && second->getRuntimePublication()==originalSecond,
                    "busy parameter edit retains the complete old schema generation");
                require(reload->GetGeneration()==generation && reload->GetAttemptCount()==attempts && reload->GetCompileCount()==compiles,
                    "busy edit scope does not attempt or compile pending schema reload");
            }
            auto scheduler=WorldStreamingScheduler::shared();const auto cpu=size_t(StreamMemoryClass::SourceCpu),gpu=size_t(StreamMemoryClass::Gpu);
            const auto beforeCpu=scheduler->stats();
            {
                Limit limit(StreamMemoryClass::SourceCpu,beforeCpu.used[cpu]);
                require(first->getRuntimePublication()==originalFirst && second->getRuntimePublication()==originalSecond &&
                    first->getShaderParameters().GetLayout()==oldLayout && second->getShaderParameters().GetLayout()==oldLayout &&
                    first->getRenderState()==oldFirstState && second->getRenderState()==oldSecondState && reload->GetGeneration()==generation,
                    "CPU denial preserves every live schema, state and publication");
                const auto denied=scheduler->stats();
                require(denied.used[cpu]==beforeCpu.used[cpu] && denied.used[gpu]==beforeCpu.used[gpu] &&
                    denied.rejectedReservations[cpu]>beforeCpu.rejectedReservations[cpu],"schema CPU failure rolls back exact usage");
            }
            ClawMaterialDrawRecords migrated;capture(migrated);
            require(reload->GetGeneration()==generation+1 && first->getShaderParameters().GetLayout()==nextLayout &&
                second->getShaderParameters().GetLayout()==nextLayout &&
                !first->getShaderParameters().SetScalar(oldGain,.9f) && !first->getShaderParameters().SetColour(oldTint,ColourF::Black),
                "schema recovery commits all subscribers and invalidates old handles");
            for(auto material:{first,second})
            {
                const auto &values=material->getShaderParameters();
                require(values.GetUInt(values.FindParameter("Mode"))==0 && values.GetUInt(values.FindParameter("Gain"))==0,
                    "new and same-width type-changed fields receive defaults");
            }
            require(first->getShaderParameters().GetColour(first->getShaderParameters().FindParameter("Tint"))==ColourF::White &&
                second->getShaderParameters().GetColour(second->getShaderParameters().FindParameter("Tint"))==ColourF(.5f,.5f,.5f,1) &&
                first->getRoughness()==.65f && second->getRoughness()==.35f && first->getLegacySurfaceStorageBytes()>0,
                "compatible colours survive and removed canonical roughness hands off to fallback");
            draw(firstMesh,migrated,1,false);draw(secondMesh,migrated,1,true);draw(firstMesh,original,0,false);
            const auto readded=MaterialShaderParameterLayout::Create({{Type::Scalar,"Roughness"},{Type::UInt,"Mode"},{Type::UInt,"Gain"},{Type::Colour,"Tint"}});
            const auto generation2=reload->GetGeneration(),attempts2=reload->GetAttemptCount(),compiles2=reload->GetCompileCount();
            const auto priorFirstState=first->getRenderState(),priorSecondState=second->getRenderState();
            auto priorFirst=first->getRuntimePublication(),priorSecond=second->getRuntimePublication();
            {
                MaterialPublicationScope publication;
                wp_shader_set_source(pixel.get(),text(psReadded));reload->RequestSchema(readded);
                for(int i=0;i<3;++i) require(first->getRuntimePublication()==priorFirst && second->getRuntimePublication()==priorSecond,
                    "busy publication scope retains the last-good schema cohort");
                require(reload->GetAttemptCount()==attempts2 && reload->GetCompileCount()==compiles2 && reload->GetGeneration()==generation2,
                    "busy publication scope performs no compilation or attempt");
                publication.commit();
            }
            const auto beforeGpu=scheduler->stats();
            {
                Limit limit(StreamMemoryClass::Gpu,beforeGpu.used[gpu]);
                require(first->getRuntimePublication()==priorFirst && second->getRuntimePublication()==priorSecond &&
                    first->getShaderParameters().GetLayout()==nextLayout && second->getShaderParameters().GetLayout()==nextLayout &&
                    first->getRenderState()==priorFirstState && second->getRenderState()==priorSecondState && reload->GetGeneration()==generation2,
                    "GPU denial preserves complete CPU schema and pass snapshots");
                const auto denied=scheduler->stats();
                require(denied.used[cpu]==beforeGpu.used[cpu] && denied.used[gpu]==beforeGpu.used[gpu] &&
                    denied.rejectedReservations[gpu]>beforeGpu.rejectedReservations[gpu],"schema GPU failure rolls back staged CPU/GPU usage");
            }
            ClawMaterialDrawRecords restored;capture(restored);
            require(reload->GetGeneration()==generation2+1 && first->getShaderParameters().GetLayout()==readded &&
                second->getShaderParameters().GetLayout()==readded && first->getRenderState()->roughness==0 &&
                second->getRenderState()->roughness==0 && first->getLegacySurfaceStorageBytes()==0 &&
                second->getLegacySurfaceStorageBytes()==0 && oldFirstState->roughness==.65f && oldSecondState->roughness==.35f &&
                priorFirstState->roughness==.65f && priorSecondState->roughness==.35f,
                "new semantic defaults commit with both maps while retained CPU states remain immutable");
            draw(firstMesh,restored,2,false);draw(secondMesh,restored,2,true);draw(firstMesh,migrated,1,false);draw(firstMesh,original,0,false);
            std::fprintf(stderr,"Claw atomic CPU schema reload: typed migration/defaults, owner fallback/snapshots, retained GPU generations, busy deferral and CPU/GPU rollback passed\n");
            return true;
        }
        catch(const std::exception &error) { std::fprintf(stderr,"FAIL: Claw shader schema: %s\n",error.what());return false; }
    }
}
