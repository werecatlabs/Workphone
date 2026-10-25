#pragma once
#include <d3dcompiler.h>
#include <limits>

// Included after the existing cooked-resource Fixture and draw helpers.
namespace cooked_material_shader_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using namespace workphone::resource;
    using Microsoft::WRL::ComPtr;
    namespace fs=std::filesystem;
    inline void run(Fixture &fixture)
    {
        const auto source=fixture.folder/"shader-source", compiled=fixture.folder/"shader-compiled";
        fs::create_directories(source);
        const auto compile=[&](const char *body,const char *profile,const fs::path &path) {
            ComPtr<ID3DBlob> code,diagnostic;
            const auto result=D3DCompile(body,std::strlen(body),"CookedMaterialShaderContracts",nullptr,nullptr,"main",profile,
                D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&diagnostic);
            if(FAILED(result) && diagnostic)std::cerr<<static_cast<const char *>(diagnostic->GetBufferPointer());
            require(SUCCEEDED(result),"Cooked shader HLSL fixture compiles");
            std::ofstream output(path,std::ios::binary|std::ios::trunc);
            output.write(static_cast<const char *>(code->GetBufferPointer()),code->GetBufferSize());
            require(output.good(),"Cooked shader DXBC fixture writes");
        };
        compile("cbuffer Transform:register(b0){row_major float4x4 Mvp;}"
            "cbuffer VertexValues:register(b8){float Scale;}"
            "float4 main(float3 p:POSITION):SV_POSITION{return mul(Mvp,float4(p*Scale,1));}","vs_5_0",source/"vertex.dxbc");
        const char *pixel="cbuffer Values:register(b4){float4 Tint;} Texture2D Tex:register(t12); SamplerState Samp:register(s5);"
            "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*Tex.Sample(Samp,float2(.5,.5));}";
        compile(pixel,"ps_5_0",source/"pixel.dxbc");
        const char *hlsl="#include \"shade.hlsli\"\n"
            "cbuffer Values:register(b4){float4 Tint;} Texture2D Tex:register(t12); SamplerState Samp:register(s5);"
            "float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint*Tex.Sample(Samp,float2(.5,.5))*float4(RED_SCALE,1,1,1);}";
        writeText(source/"pixel.hlsl",hlsl);writeText(source/"shade.hlsli","#define RED_SCALE 1.0f\n");
        std::array<u8,22> white{};white[2]=2;white[12]=white[14]=1;white[16]=32;white[17]=0x28;
        white[18]=white[19]=white[20]=white[21]=255;
        {std::ofstream output(source/"white.tga",std::ios::binary);output.write(reinterpret_cast<const char *>(white.data()),white.size());}
        writeText(source/"white.texres",R"({"version":1,"source":"data://white.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5})");
        const std::string descriptor=R"({"version":1,"baseColour":[1,1,1,1],"metalness":0,"roughness":1,"baseColourTexture":"data://white.texres",
            "shader":{"id":"Cooked.Contract","schemaVersion":7,"vertex":"data://vertex.dxbc","pixelSource":"data://pixel.hlsl",
            "vertexBuffers":["VertexValues"],"vertexResources":[],"pixelBuffers":["Values"],"pixelResources":["Tex","Samp"],
            "parameters":[{"name":"Tint","type":"colour","value":[0,1,0,1]},
            {"name":"Scale","type":"float","value":1},{"name":"Tex","type":"texture","value":"data://white.texres"},
            {"name":"Samp","type":"sampler","value":[1,1,0,1]},
            {"name":"Signed","type":"int","value":-17},{"name":"Mask","type":"uint","value":4294967295},
            {"name":"Enabled","type":"bool","value":true},{"name":"Pair","type":"int2","value":[-2,5]},
            {"name":"Transform","type":"matrix4","value":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]}]}})";
        writeText(source/"shader.matres",descriptor.c_str());
        auto resources=std::make_shared<ResourceSystem>(fixture.resources->registry(),std::make_shared<ResourceCompilationDatabase>());
        ResourceSystemConfig config;config.sourceRoot=pathText(source);config.compiledRoot=pathText(compiled);config.target="cooked-shader-contract";
        String error;require(resources->initialize(config,error),error);
        const ResourceID id("data://shader.matres");CompilationOptions options;options.packagedBuild=true;
        const auto report=resources->compile(id,options);
        if(!report.succeeded())for(const auto &message:report.messages)std::cerr<<message<<'\n';
        require(report.succeeded(),"Versioned typed shader material compiles through ResourceSystem");
        auto loaded=resources->load(id,error);CookedMaterialData data;
        require(loaded && decodeCookedMaterial(*loaded,data,error),error);
        require(loaded->header.compilerVersion==GraphicsResourceCompiler().versionFor(id.type()) &&
            data.shader.shaderID=="Cooked.Contract" && data.shader.schemaVersion==7 &&
            data.shader.parameters.size()==9 && loaded->header.installDependencies.size()==1 && loaded->dependencies.size()==1,
            "v3 shader schema preserves identity/version and deduplicates durable texture dependencies");
        const auto bytes=data.shader.pixelDXBC;
        // Includes are captured before reading during dependency scanning, and
        // become explicit build-key inputs for device-free HLSL compilation.
        writeText(source/"shade.hlsli","#define RED_SCALE .5f\n");
        const auto changed=resources->compileAffected("data://shade.hlsli",options);
        require(!changed.empty() && changed.back().succeeded(),"Shader include is a tracked compile dependency");
        loaded=resources->load(id,error);
        require(loaded && decodeCookedMaterial(*loaded,data,error) && data.shader.pixelDXBC!=bytes,
            "Changed HLSL include publishes changed source-free program bytes");
        GraphicsResourceCompiler compiler;CompileContext probe;probe.resourceId=id;
        probe.sourceRoot=config.sourceRoot;probe.sourcePath=pathText(source/"shader.matres");
        probe.compiledRoot=config.compiledRoot;probe.target=config.target;DependencySet dependencies;
        auto conflicting=descriptor;const auto field=conflicting.find("\"pixelSource\"");
        conflicting.insert(field,"\"pixel\":\"data://pixel.dxbc\",");
        writeText(source/"shader.matres",conflicting.c_str());
        require(!compiler.getDependencies(probe,dependencies,error) && !error.empty(),
            "Ambiguous DXBC/HLSL stage source rejects instead of choosing silently");
        writeText(source/"shader.matres",descriptor.c_str());
        writeText(fixture.folder/"outside.hlsli","#define RED_SCALE 1.0f\n");
        std::string escaping=hlsl;
        escaping.replace(escaping.find("shade.hlsli"),std::strlen("shade.hlsli"),"../outside.hlsli");
        writeText(source/"pixel.hlsl",escaping.c_str());
        require(!compiler.getDependencies(probe,dependencies,error) && !error.empty(),
            "Shader include cannot escape the canonical asset source root");
        writeText(source/"pixel.hlsl",hlsl);
        const auto textureReport=resources->compile(ResourceID("data://white.texres"),options);
        require(textureReport.succeeded(),"Cook validation fixture retains its pinned texture artifact");
        probe.packagedBuild=true;probe.dependencyArtifacts["data://white.texres"]=textureReport.outputPath;
        auto wrongType=descriptor;const auto tintType=wrongType.find("\"type\":\"colour\"");
        require(tintType!=std::string::npos,"Cook validation fixture locates Tint type");
        wrongType.replace(tintType,std::strlen("\"type\":\"colour\""),"\"type\":\"int4\"");
        writeText(source/"shader.matres",wrongType.c_str());
        std::ostringstream invalidShader(std::ios::binary);Array<String> messages;
        require(compiler.compile(probe,invalidShader,messages)==CompilationStatus::Failure &&
            invalidShader.str().empty() && !messages.empty(),
            "Pure-reflection cook gate rejects HLSL float4 versus declared int4 before output publication");
        writeText(source/"shader.matres",descriptor.c_str());
        const auto reject=[&](RuntimeResource broken) {
            graphics_compiler_contracts::refreshIntegrity(broken);CookedMaterialData output;
            require(!decodeCookedMaterial(broken,output,error) && !error.empty() && output.target.empty(),
                "Malformed cooked shader payload rejects without output publication");
        };
        // Locate the endian/schema marker through the bounded sequential legacy prefix.
        const auto word=[&](size_t offset) { require(offset+4<=loaded->payload.size(),"Shader mutation offset bounds");
            return u32(loaded->payload[offset])|u32(loaded->payload[offset+1])<<8|u32(loaded->payload[offset+2])<<16|u32(loaded->payload[offset+3])<<24; };
        size_t marker=8;marker+=4+word(marker)+4+24;marker+=4+word(marker)+24;
        require(word(marker)==0,"Shader fixture has no optional PBR semantic slots");marker+=4;
        require(word(marker)==0x01020304u,"Shader schema marker follows the legacy PBR prefix");
        for(const auto mutation:{std::pair<size_t,u32>{marker,0x04030201u},{marker+4,99u}})
        {auto broken=*loaded;graphics_compiler_contracts::overwrite32(broken,mutation.first,mutation.second);reject(std::move(broken));}
        const auto countOffset=marker+8+4+word(marker+8)+4;
        {auto broken=*loaded;graphics_compiler_contracts::overwrite32(broken,countOffset,0xffffffffu);reject(std::move(broken));}
        {auto broken=*loaded;broken.payload.push_back(0);reject(std::move(broken));}
        {auto broken=*loaded;broken.header.compilerVersion=3;reject(std::move(broken));}
        auto unsupported=data;unsupported.shader.parameters[0].type=MaterialShaderParameterType::Buffer;
        std::ostringstream rejected(std::ios::binary);
        require(!writeCookedMaterial(unsupported,rejected,error) && rejected.str().empty(),"Unsupported durable buffer assets reject explicitly");
        unsupported=data;unsupported.shader.parameters[3].words[2]=4;
        require(!writeCookedMaterial(unsupported,rejected,error),"Invalid portable sampler policy does not silently clamp");
        unsupported=data;++unsupported.shader.parameters[2].texture.payloadHash;
        std::ostringstream wrongPin(std::ios::binary);require(writeCookedMaterial(unsupported,wrongPin,error),error);
        auto broken=graphics_compiler_contracts::wrap(id,wrongPin.str());broken.header.installDependencies=loaded->header.installDependencies;
        broken.dependencies=loaded->dependencies;reject(std::move(broken));
        // Device-independent public writers must not produce envelopes larger
        // than their reader accepts, even with programmatically supplied metadata.
        auto large=data;
        for(auto code:{&large.shader.vertexDXBC,&large.shader.pixelDXBC})
        {
            const auto limit=graphicsMaterialShaderByteLimit/2;
            std::vector<u8> privateData(limit-code->size()-64,0);ComPtr<ID3DBlob> expanded;
            require(SUCCEEDED(D3DSetBlobPart(code->data(),code->size(),D3D_BLOB_PRIVATE_DATA,0,
                privateData.data(),privateData.size(),&expanded)),"Large boundary fixture preserves valid DXBC with private data");
            const auto begin=static_cast<const u8 *>(expanded->GetBufferPointer());
            code->assign(begin,begin+expanded->GetBufferSize());
            require(code->size()<=limit && code->size()>limit-128,"Large boundary shader is near its per-stage limit");
        }
        for(u32 i=0;i<55;++i)
        {
            CookedMaterialShaderParameter p;p.type=MaterialShaderParameterType::Texture;
            p.name=String("Unused")+std::to_string(i).c_str()+String(240,'q');p.texture=data.shader.parameters[2].texture;
            const String path="data://"+String(240,'a')+"/"+String(240,'b')+"/"+String(240,'c')+"/"+
                String(190,'d')+std::to_string(i).c_str()+".texres";
            require(p.texture.id.set(path),"Large metadata fixture has valid durable identities");
            large.shader.parameters.push_back(std::move(p));
        }
        std::ostringstream tooLarge(std::ios::binary);
        require(!writeCookedMaterial(large,tooLarge,error) && tooLarge.str().empty(),
            "Aggregate shader bytecode and metadata reject before writing an unreadable envelope");
        large.shader.parameters.resize(data.shader.parameters.size()+45);
        std::ostringstream bounded(std::ios::binary);require(writeCookedMaterial(large,bounded,error),error);
        require(bounded.str().size()<=graphicsMaterialShaderByteLimit+65536u,"Nearby large payload obeys the reader bound");
        auto largeResource=graphics_compiler_contracts::wrap(id,bounded.str());
        largeResource.header.installDependencies=loaded->header.installDependencies;largeResource.dependencies=loaded->dependencies;
        for(size_t i=data.shader.parameters.size();i<large.shader.parameters.size();++i)
        {
            const auto &identity=large.shader.parameters[i].texture.id;
            auto dependency=std::make_shared<RuntimeResource>(*loaded->dependencies.front());dependency->header.resourceId=identity;
            largeResource.header.installDependencies.push_back(identity);largeResource.dependencies.push_back(std::move(dependency));
        }
        CookedMaterialData nearby;require(decodeCookedMaterial(largeResource,nearby,error),error);
        require(nearby.shader.parameters.size()==large.shader.parameters.size(),"Nearby large valid payload round-trips without native handle serialization");
        // Binary typed values reconstruct canonical CPU types, not native offsets.
        std::unique_ptr<wp_shader_program,decltype(&wp_shader_program_destroy)> definition(wp_shader_program_create(),&wp_shader_program_destroy);
        MaterialShaderParametersInstance values;
        require(populateCookedMaterialShader(data.shader,values,definition.get(),error),error);
        require(values.GetInt(values.FindParameter("Signed"))==-17 && values.GetUInt(values.FindParameter("Mask"))==0xffffffffu &&
            values.GetBool(values.FindParameter("Enabled")) && values.GetMatrix(values.FindParameter("Transform"))[2][1]==10,
            "Portable typed values preserve signedness, bool and all sixteen matrix words");
        const auto manifest=fixture.folder/"shader-runtime.wprm";
        require(resources->writeRuntimeManifest(pathText(manifest),{id},error),error);resources->shutdown();
        const auto hidden=fixture.folder/"shader-source-hidden";fs::rename(source,hidden);
        struct Restore { fs::path source,hidden;~Restore(){std::error_code ignored;if(fs::exists(hidden))fs::rename(hidden,source,ignored);} } restore{source,hidden};
        ResourceSystem shipping(nullptr,nullptr);RuntimeResourceConfig runtime;
        runtime.compiledRoot=config.compiledRoot;runtime.manifestPath=pathText(manifest);runtime.target=config.target;
        require(shipping.initializeRuntime(runtime,error),error);
        auto shipped=shipping.load(id,error);require(bool(shipped),error);
        ClawMaterialResource publisher(nullptr,nullptr,"",{},fixture.graphics.renderer,config.target,true);
        auto candidate=publisher.stageRuntime(shipped,error);
        require(candidate && publisher.publish(candidate,error),error);
        require(candidate->material()->hasRuntimeProgram() && candidate->material()->getRuntimePublication(),
            "Source-free cooked material automatically installs the immutable runtime shader program");
        requireDraw(fixture,candidate,true);
        auto malformed=std::make_shared<RuntimeResource>(*shipped);malformed->payload.push_back(0);graphics_compiler_contracts::refreshIntegrity(*malformed);
        require(!publisher.stageRuntime(malformed,error) && publisher.current()==candidate,
            "Malformed shader replacement preserves the previously published source-free draw");
        requireDraw(fixture,publisher.current(),true);
        // Direct immutable programs stay on their original device. The cooked
        // resource owner reinstalls from the retained shipped payload on a new
        // renderer without source/compiler services, while old views still draw.
        {
            auto firstRenderer=fixture.graphics.renderer;
            auto secondWindow=make_ptr<claw_texture_mip_contracts::Window>();
            secondWindow->handle=CreateWindowExW(0,L"STATIC",L"Cooked shader second device",WS_OVERLAPPED,
                0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
            require(secondWindow->handle!=nullptr,"Second cooked renderer window allocates");
            auto secondRenderer=make_ptr<ClawRendererDX11>();
            struct RestoreRenderer
            {
                Fixture &fixture;SmartPtr<ClawRendererDX11> first,second;
                SmartPtr<claw_texture_mip_contracts::Window> window;
                ~RestoreRenderer()
                {
                    fixture.graphics.renderer=first;fixture.graphics.graphics->attachRenderer(first);
                    second->unload(nullptr);
                    if(window->handle){DestroyWindow(window->handle);window->handle=nullptr;}
                }
            } restoreRenderer{fixture,firstRenderer,secondRenderer,secondWindow};
            secondRenderer->load(secondWindow);require(secondRenderer->isLoaded(),"Second cooked renderer creates a real device");
            auto firstNative=wp_renderer_get_dx11(firstRenderer->getNativeRenderer());
            auto secondNative=wp_renderer_get_dx11(secondRenderer->getNativeRenderer());
            require(wp_renderer_dx11_get_device(firstNative)!=wp_renderer_dx11_get_device(secondNative),
                "Cooked shader recreation fixture uses distinct simultaneously retained devices");
            const auto oldPublication=candidate->material()->getRuntimePublication();
            require(oldPublication && !oldPublication->Bind(secondNative,false),
                "Old immutable shader publication explicitly rejects a different device");
            fixture.graphics.renderer=secondRenderer;fixture.graphics.graphics->attachRenderer(secondRenderer);
            require(!publisher.current(),"Cooked owner hides its incompatible old visible bundle");
            ClawMaterialResource reinstalled(nullptr,nullptr,"",{},secondRenderer,config.target,true);
            auto secondCandidate=reinstalled.stageRuntime(shipped,error);
            require(secondCandidate && reinstalled.publish(secondCandidate,error),error);
            requireDraw(fixture,secondCandidate,true);
            fixture.graphics.renderer=firstRenderer;fixture.graphics.graphics->attachRenderer(firstRenderer);
            require(publisher.current()==candidate,"Returning to the retained original device restores its visible identity");
            requireDraw(fixture,candidate,true);
            require(reinstalled.unload(error),error);
        }
        require(publisher.unload(error),error);shipping.shutdown();
        std::cout<<"PASS: v3 typed material schema, HLSL/DXBC/include build keys, malformed payload rejection, source-free shader/texture/sampler installation and retained visible generation\n";
    }
}
