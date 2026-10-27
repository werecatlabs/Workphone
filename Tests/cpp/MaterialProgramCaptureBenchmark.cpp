#include <Workphone/Memory/SmartPtr.hpp>
#include <WPGraphics/MaterialProgramDX11.hpp>
#include <Workphone/Graphics/MaterialShader.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "DX11TestEvidence.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace workphone;
using namespace workphone::render;
using Microsoft::WRL::ComPtr;
static void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
static u64 cycles(){ULONG64 result=0;require(QueryThreadCycleTime(GetCurrentThread(),&result)!=FALSE,"Owner cycle query failed");return result;}
int main()
{
    struct Shutdown{~Shutdown(){WorldStreamingScheduler::shutdownShared();}} shutdown;
    try
    {
        ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level{};
        require(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context)),"WARP device creation failed");
        require(recordDX11TestDevice(device.Get()),"Device evidence unavailable");
        const char *source="cbuffer Values:register(b4){float4 Tint;}Texture2D Tex:register(t12);SamplerState Samp:register(s5);"
            "float4 main(float4 p:SV_POSITION,float2 uv:TEXCOORD2):SV_TARGET{return Tint*Tex.Sample(Samp,uv);}";
        ComPtr<ID3DBlob> code,error;
        require(SUCCEEDED(D3DCompile(source,std::strlen(source),"CaptureCpu",nullptr,nullptr,"main","ps_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&error)),"Shader compilation failed");
        std::unique_ptr<wp_shader_program,decltype(&wp_shader_program_destroy)> definition(wp_shader_program_create(),wp_shader_program_destroy);
        require(bool(definition),"Native definition creation failed");
        const auto text=[](const char *s){return reinterpret_cast<const wp_c8 *>(s);};
        wp_shader_program_register_uniform(definition.get(),text("Tint"),WORKPHONE_UNIFORM_TYPE_VEC4);
        for(auto name:{"Tex","Samp"})wp_shader_program_register_param(definition.get(),text(name),WORKPHONE_UNIFORM_TYPE_INT,1,
            WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,WORKPHONE_SHADER_AUTO_PARAM_NONE,
            std::strcmp(name,"Tex")==0?WORKPHONE_SHADER_RESOURCE_TEXTURE:WORKPHONE_SHADER_RESOURCE_SAMPLER);
        MaterialShader schema("CaptureCpu");using T=MaterialShaderParameterType;
        schema.addParameter(T::Colour,"Tint");schema.addParameter(T::Texture,"Tex");schema.addParameter(T::Sampler,"Samp");
        std::array<MaterialShaderParametersInstance,2> values;
        for(size_t i=0;i<values.size();++i)
        {
            schema.buildInstance(values[i]);
            require(values[i].SetColour(values[i].FindParameter("Tint"),ColourF(i?.75f:.25f,.5f,.75f,1)),"Tint preparation failed");
            require(values[i].SetTexture(values[i].FindParameter("Tex"),1) && values[i].SetSampler(values[i].FindParameter("Samp"),2),"Numeric resource preparation failed");
        }
        String diagnostic;
        auto program=MaterialProgramDX11::Create(device.Get(),schema.getLayout(),definition.get(),1,
            {code->GetBufferPointer(),code->GetBufferSize()},{},diagnostic);
        require(bool(program),diagnostic.c_str());
        std::array<MaterialProgramDX11::PublicationPtr,3> retained;
        u64 serial=0;
        const auto capture=[&]{auto publication=program->Capture(values[serial%2],{});require(bool(publication),"Capture failed");retained[serial++%retained.size()]=std::move(publication);};
        const auto warm=std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now()-warm<std::chrono::milliseconds(250))capture();
        std::array<u64,1024> calibration{};
        for(auto &sample:calibration){auto start=cycles();sample=cycles()-start;}
        std::sort(calibration.begin(),calibration.end());
        constexpr u32 operations=5000,samples=7;
        for(u32 sample=0;sample<samples;++sample)
        {
            const auto start=cycles();
            for(u32 i=0;i<operations;++i)capture();
            const auto elapsed=cycles()-start;
            std::cout<<"capture_cpu sample="<<sample<<" operations="<<operations<<" cycles="<<elapsed
                <<" clock_pair_median="<<calibration[calibration.size()/2]<<" retained="<<retained.size()<<'\n';
        }
        std::cout<<"Capture CPU scope: actual immutable packing and three-publication retirement; prepared values/numeric resources, no Bind/Present/GPU work\n";
        return 0;
    }
    catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
