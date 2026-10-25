#pragma once
#include "ClawMaterialReloadContracts.hpp"
#include <Workphone/Graphics/GraphicsWindow.hpp>
namespace claw_shader_device_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    inline bool run(ClawRendererDX11 &renderer)
    {
        struct Window:GraphicsWindow
        {
            HWND handle=nullptr;
            Vector2I getSize() const override {return {128,128};}
            void getWindowHandle(void *data) override {*static_cast<HWND *>(data)=handle;}
            void _getObject(void **data) const override {*data=nullptr;}
        };
        try
        {
            using claw_material_reload_contracts::require;
            claw_foliage_scene_contracts::Fixture fixture;
            auto material=make_ptr<ClawMaterial>(),clone=make_ptr<ClawMaterial>();
            for(auto value:{material,clone})
            {
                auto technique=make_ptr<ClawMaterialTechnique>();technique->setMaterial(value.get());technique->createPass();
                value->setTechniques({technique});value->setMaterialType(MaterialType::UI);value->setCullMode(1);
            }
            material->setShaderParameters({{MaterialShaderParameterType::Colour,"Tint"}},"DeviceReload");
            auto &values=material->getShaderParameters();values.SetColour(values.FindParameter("Tint"),ColourF(1,0,0,1));
            const auto c=[](const char *p){return reinterpret_cast<const wp_c8 *>(p);};
            std::unique_ptr<wp_shader_program,decltype(&wp_shader_program_destroy)> source(wp_shader_program_create(),&wp_shader_program_destroy);
            auto pixel=wp_shader_create(WORKPHONE_SHADER_TYPE_PIXEL,WORKPHONE_SHADER_LANGUAGE_HLSL,
                c("cbuffer Values:register(b4){float4 Tint;}float4 main(float4 p:SV_POSITION):SV_TARGET{return Tint;}"),c("main"));
            wp_shader_program_attach(source.get(),pixel);wp_shader_destroy(pixel);
            wp_shader_program_register_uniform(source.get(),c("Tint"),WORKPHONE_UNIFORM_TYPE_VEC4);
            auto native=wp_renderer_get_dx11(renderer.getNativeRenderer());String error;
            auto reload=MaterialProgramReloadDX11::Create(wp_renderer_dx11_get_device(native),values.GetLayout(),source.get(),1,error);
            if(!reload) throw std::runtime_error(error.c_str());
            material->setRuntimeReload(reload);material->copyRuntimeShaderTo(*clone);
            clone->getShaderParameters().SetColour(clone->getShaderParameters().FindParameter("Tint"),ColourF(0,1,0,1));
            require(values.GetColour(values.FindParameter("Tint"))==ColourF(1,0,0,1),"shader clones share program but detach values");
            claw_material_reload_contracts::Mesh firstMesh,secondMesh;firstMesh.setMaterial(material);secondMesh.setMaterial(clone);
            auto target=make_ptr<ClawRenderTarget>();target->setSize({64,64});
            renderer.setRenderTarget(target);renderer.setViewport(nullptr);renderer.setCamera(nullptr);
            ClawMaterialDrawRecords old;old.capture(&firstMesh);old.capture(&secondMesh);renderer.prepareMaterialRecords(old);
            auto window=make_ptr<Window>();window->handle=CreateWindowExW(0,L"STATIC",L"Shader recreation contract",WS_OVERLAPPEDWINDOW,
                0,0,128,128,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
            require(window->handle!=nullptr,"device recreation test window exists");
            ClawRendererDX11 other;other.load(window);
            struct Cleanup {ClawRendererDX11 &a,&b;HWND h;~Cleanup(){a.setRenderTarget(nullptr);b.setRenderTarget(nullptr);b.unload(nullptr);DestroyWindow(h);}} cleanup{renderer,other,window->handle};
            require(other.isLoaded(),"second renderer initializes for device recreation");
            auto otherNative=wp_renderer_get_dx11(other.getNativeRenderer());
            require(wp_renderer_dx11_get_device(native)!=wp_renderer_dx11_get_device(otherNative),"two distinct DX11 device identities");
            auto secondTarget=make_ptr<ClawRenderTarget>();secondTarget->setSize({64,64});other.setRenderTarget(secondTarget);
            other.setViewport(nullptr);other.setCamera(nullptr);
            const auto before=reload->GetGeneration();reload->RecreateDevice(wp_renderer_dx11_get_device(otherNative),2);
            auto migrated=material->getRuntimePublication();
            require(migrated && reload->GetGeneration()==before+1,"device rebuild atomically publishes a fresh native generation");
            require(!migrated->Bind(native,false),"new generation rejects previous device context");
            ClawMaterialDrawRecords next;next.capture(&firstMesh);next.capture(&secondMesh);other.prepareMaterialRecords(next);
            const auto draw=[&](ClawRendererDX11 &r,wp_renderer_dx11 *n,claw_material_reload_contracts::Mesh &mesh,
                                const ClawMaterialDrawRecords &records,int channel) {
                r.clear(ColourF::Black);require(r.renderMeshRecords(&mesh,Matrix4F::identity(),records,*records.find(&mesh)),"device generation record remains valid");
                const auto pixels=claw_foliage_contracts::pixels(n);const auto offset=(32u*64u+32u)*4u;
                require(pixels[offset+channel]>220 && pixels[offset+(1-channel)]<10,"correct device generation renders its material values");
            };
            draw(renderer,native,firstMesh,old,0);draw(other,otherNative,secondMesh,next,1);
            MaterialProgramReloadDX11::UpdateAll(wp_renderer_dx11_get_device(otherNative),&other);
            MaterialProgramReloadDX11::UpdateAll(wp_renderer_dx11_get_device(native),&other);
            require(reload->GetGeneration()==before+2,"graphics-owner device replacement triggers automatic native rebuild");
            ClawMaterialDrawRecords recovered;recovered.capture(&firstMesh);recovered.capture(&secondMesh);renderer.prepareMaterialRecords(recovered);
            draw(renderer,native,secondMesh,recovered,1);draw(other,otherNative,firstMesh,next,0);
            const auto sharedLayout=clone->getShaderParameters().GetLayout();
            clone->setShaderParameters(sharedLayout->GetParameterInfo(),"OtherDeviceShader");
            require(clone->getShaderParameters().GetLayout()==sharedLayout,"logical shader switch reuses matching CPU layout");
            require(!clone->getRuntimePublication() && !clone->hasRuntimeProgram(),"same-layout shader switch retires previous runtime attachment");
            bool available=true;clone->getRuntimeShaderReloadStatus()->getPropertyValue("available",available);
            require(!available,"same-layout shader switch clears previous Editor reload status");
            std::fprintf(stderr,"Shader device recreation and independent cloned values: PASS\n");return true;
        }
        catch(const std::exception &e) {renderer.setRenderTarget(nullptr);std::fprintf(stderr,"FAIL: shader device recreation: %s\n",e.what());return false;}
    }
}
