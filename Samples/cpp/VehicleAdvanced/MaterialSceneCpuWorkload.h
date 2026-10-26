#pragma once
#if defined(_WIN32) && WP_GRAPHICS_SYSTEM_CLAW
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/MaterialProgramDX11.hpp>
#include <Workphone/Graphics/MaterialShader.hpp>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdlib>
#include <cstring>
#include <set>

namespace workphone::advanced
{
    // Explicit profiling variant of the existing scene, using its real textures,
    // geometry, visibility, LOD and material ownership. Compilation is pre-warm.
    inline void installMaterialCpuPrograms(SceneAssets &assets, render::ClawRendererDX11 &renderer)
    {
        if(!std::getenv("WP_MATERIAL_SCENE_CUSTOM")) return;
        using namespace render;
        using Microsoft::WRL::ComPtr;
        const char *source="cbuffer Values:register(b4){float4 Tint;}"
            "Texture2D Tex:register(t12);SamplerState Samp:register(s5);"
            "float4 main(float4 p:SV_POSITION,float2 uv:TEXCOORD2):SV_TARGET{return Tint*Tex.Sample(Samp,uv);}";
        ComPtr<ID3DBlob> code,diagnostic;
        if(FAILED(D3DCompile(source,std::strlen(source),"MaterialSceneCpu",nullptr,nullptr,"main","ps_5_0",
                            D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&diagnostic)))
            throw std::runtime_error("Material CPU scene shader compilation failed");
        auto native=wp_renderer_get_dx11(renderer.getNativeRenderer());
        auto device=static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));
        D3D11_SAMPLER_DESC desc{};desc.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU=desc.AddressV=desc.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;desc.MaxLOD=D3D11_FLOAT32_MAX;
        ComPtr<ID3D11SamplerState> sampler;
        if(FAILED(device->CreateSamplerState(&desc,&sampler))) throw std::runtime_error("Scene sampler creation failed");
        std::unique_ptr<wp_shader_program,decltype(&wp_shader_program_destroy)> definition(wp_shader_program_create(),wp_shader_program_destroy);
        const auto text=[](const char *s){return reinterpret_cast<const wp_c8 *>(s);};
        wp_shader_program_register_uniform(definition.get(),text("Tint"),WORKPHONE_UNIFORM_TYPE_VEC4);
        for(auto name:{"Tex","Samp"}) wp_shader_program_register_param(definition.get(),text(name),WORKPHONE_UNIFORM_TYPE_INT,1,
            WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,WORKPHONE_SHADER_AUTO_PARAM_NONE,
            std::strcmp(name,"Tex")==0?WORKPHONE_SHADER_RESOURCE_TEXTURE:WORKPHONE_SHADER_RESOURCE_SAMPLER);
        using T=MaterialShaderParameterType;
        MaterialShader schema("SceneCpuCustom");
        schema.addParameter(T::Colour,"Tint");schema.addParameter(T::Texture,"Tex");schema.addParameter(T::Sampler,"Samp");
        MaterialShaderRegistry::instance().registerShader(schema);
        std::set<ClawMaterial *> unique; MaterialProgramDX11::Ptr program;unsigned installed=0;
        for(auto &owner:assets.materials)
        {
            auto material=dynamic_pointer_cast<ClawMaterial>(owner);
            if(!material || !unique.insert(material.get()).second) continue;
            auto texture=material->getTexture(0);if(!texture) continue;
            const auto diffuse=material->getDiffuse();
            material->setShaderParameters(schema.getParameterInfo(),schema.getShaderID());
            auto &values=material->getShaderParameters();
            values.SetColour(values.FindParameter("Tint"),diffuse);
            values.SetTexture(values.FindParameter("Tex"),1);values.SetSampler(values.FindParameter("Samp"),2);
            if(!program)
            {
                String error;program=MaterialProgramDX11::Create(device,values.GetLayout(),definition.get(),1,
                    {code->GetBufferPointer(),code->GetBufferSize()},{},error);
                if(!program) { std::cerr << "Material scene program creation: " << error << '\n';throw std::runtime_error(error.c_str()); }
            }
            auto resources=std::make_shared<MaterialProgramResourceTableDX11>();
            resources->SetTexture(1,texture);resources->SetSampler(2,sampler.Get());
            material->setRuntimeProgram(program,resources);++installed;
        }
        if(!installed) throw std::runtime_error("Material CPU custom scene requires real textured materials");
        std::cout << "Material CPU scene custom textured materials=" << installed << '\n';
    }
}
#endif
