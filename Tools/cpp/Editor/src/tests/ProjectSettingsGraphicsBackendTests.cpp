#include <EditorPCH.hpp>
#include <editor/ProjectSettingsGraphicsBackend.hpp>
#include <Workphone/Workphone.hpp>

#if WP_EDITOR_TESTS
#include <boost/test/unit_test.hpp>
#include <WorkphoneGraphics/workphone_graphics_hdr.h>
#include <WorkphoneGraphics/workphone_graphics_pipeline.h>
#include <Workphone/Graphics/GraphicsPipeline.hpp>
#include <Workphone/Graphics/ProjectGraphicsProviders.hpp>
#include <array>
#include <vector>
#include <cmath>

using namespace workphone;
using namespace workphone::editor;

BOOST_AUTO_TEST_CASE(project_graphics_output_curves_test)
{
    auto *hdr=wp_hdr_create(2,2);
    BOOST_REQUIRE(hdr);
    const std::array<float,12> source={2,.5f,.25f,2,.5f,.25f,2,.5f,.25f,2,.5f,.25f};
    std::array<float,12> output{};
    std::array<float,4> red{};
    for(int curve=0;curve<4;++curve)
    {
        wp_hdr_set_output_options(hdr,1,0,curve);
        wp_hdr_composite(hdr,source.data(),nullptr,nullptr,2,2,output.data());
        red[curve]=output[9];
        for(auto value:output) BOOST_CHECK(std::isfinite(value) && value>=0 && value<=1);
    }
    BOOST_CHECK_EQUAL(red[0],1.0f);
    BOOST_CHECK_CLOSE(red[2],2.0f/3.0f,.001f);
    BOOST_CHECK(std::abs(red[1]-red[2])>.05f);
    BOOST_CHECK(std::abs(red[1]-red[3])>.05f);
    wp_hdr_set_output_options(hdr,0,0,2);
    wp_hdr_composite(hdr,source.data(),nullptr,nullptr,2,2,output.data());
    BOOST_CHECK_EQUAL(output[9],1.0f);
    BOOST_CHECK_CLOSE(output[10],.5f,.001f);
    wp_hdr_set_output_options(hdr,0,1,2);
    wp_hdr_composite(hdr,source.data(),nullptr,nullptr,2,2,output.data());
    BOOST_CHECK_CLOSE(output[10],wp_hdr_linear_to_srgb(.5f),.001f);
    wp_hdr_destroy(hdr);
}

BOOST_AUTO_TEST_CASE(project_graphics_backend_pipeline_profile_test)
{
    auto settings=workphone::make_ptr<Properties>();
    settings->setProperty("graphics.antiAliasing",1);
    settings->setProperty("graphics.hdr",false);
    settings->setProperty("graphics.colorSpace",0);
    settings->setProperty("graphics.tonemapping",2);
    settings->setProperty("graphics.renderScale",.5f);
    settings->setProperty("graphics.customWidth",1600);
    settings->setProperty("graphics.customHeight",900);
    settings->setProperty("graphics.shadowResolution",3);
    settings->setProperty("quality.preset",0);
    settings->setProperty("graphics.bloom",true);
    settings->setProperty("graphics.postProcessing",true);
    auto profile=projectGraphicsBackendProperties(settings);
    render::GraphicsPipeline pipeline;
    pipeline.setProperties(profile);
    pipeline.setProperties(profile);
    const auto persisted=pipeline.getProperties();
    BOOST_CHECK(!pipeline.isTAAEnabled());
    BOOST_CHECK(pipeline.isBloomEnabled());
    BOOST_CHECK_EQUAL(pipeline.getCsmSettings().m_shadowMapSize,4096);
    BOOST_CHECK_EQUAL(persisted->getPropertyAsInt("graphics.customWidth"),1600);
    BOOST_CHECK_CLOSE(persisted->getPropertyAsFloat("graphics.renderScale"),.5f,.001f);
    BOOST_CHECK_EQUAL(persisted->getPropertyAsInt("graphics.tonemapping"),2);
}


BOOST_AUTO_TEST_CASE(project_graphics_provider_preflight_test)
{
    struct Compiler final:IProjectShaderCompilerProvider
    {
        bool validate(SmartPtr<Properties> profile,const String &api,String &) override
        {return profile->getPropertyAsInt("graphics.shaderLanguage")==4&&api=="DX11";}
        bool compile(SmartPtr<Properties>,const String &,const String &,const String &,const String &,Array<u8> &,String &) override {return false;}
    };
    auto previous=getProjectShaderCompilerProvider(4,3);
    struct Restore {std::shared_ptr<IProjectShaderCompilerProvider> provider;~Restore(){registerProjectShaderCompilerProvider(4,3,provider);}} restore{previous};
    registerProjectShaderCompilerProvider(4,3,nullptr);
    auto profile=workphone::make_ptr<Properties>();profile->setProperty("graphics.renderer.pc",1);profile->setProperty("graphics.shaderLanguage",4);profile->setProperty("graphics.shaderModel",3);
    String error,notes;BOOST_CHECK(!validateProjectGraphicsProviders(profile,error,notes));BOOST_CHECK(!error.empty());
    registerProjectShaderCompilerProvider(4,3,std::make_shared<Compiler>());error.clear();BOOST_CHECK(validateProjectGraphicsProviders(profile,error,notes));
    profile->setProperty("graphics.renderer.pc",5);profile->setProperty("graphics.renderPipeline",2);error.clear();
    BOOST_CHECK(!validateProjectGraphicsProviders(profile,error,notes));BOOST_CHECK(error.find("Software")!=String::npos);
    profile->setProperty("graphics.renderer.pc",1);profile->setProperty("graphics.shaderLanguage",0);profile->setProperty("graphics.shaderModel",2);profile->setProperty("graphics.allowRendererFallback",false);error.clear();
    BOOST_CHECK(!validateProjectGraphicsProviders(profile,error,notes));profile->setProperty("graphics.allowRendererFallback",true);error.clear();
    BOOST_CHECK(validateProjectGraphicsProviders(profile,error,notes));BOOST_CHECK(notes.find("SM5")!=String::npos);
}

#if defined(_WIN32) && WP_BUILD_RENDERER_DX11
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <windows.h>
#include <d3d11.h>

BOOST_AUTO_TEST_CASE(project_graphics_dx11_msaa_scale_resolve_test)
{
    struct NativeWindow
    {
        HWND window=CreateWindowExW(0,L"STATIC",L"Project graphics settings validation",WS_OVERLAPPEDWINDOW,
                                    0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        ~NativeWindow(){if(window) DestroyWindow(window);}
    } window;
    BOOST_REQUIRE(window.window);
    for(int backend:{0,1}) for(int driver:{0,3})
    {
        wp_renderer_dx11_set_d3d12_execution(backend);
        // Cover both native compiler targets without doubling device lifetimes.
        wp_renderer_dx11_set_startup_options(driver,backend?0:1,0,1);
        auto *renderer=wp_renderer_dx11_create(window.window,32,16);
        BOOST_REQUIRE_MESSAGE(renderer,"DX11 device creation failed");
        BOOST_CHECK_EQUAL(wp_renderer_dx11_get_d3d12_device(renderer)!=nullptr,backend!=0);
        const auto mask=wp_renderer_dx11_supported_scene_samples(renderer);
        BOOST_CHECK(mask & 1u);
        unsigned frames=0;
        for(int samples:{1,2,4,8})
        {
            if(!(mask & samples)) continue;
            wp_renderer_dx11_begin_frame(renderer);
            wp_renderer_dx11_set_scene_sampling(renderer,samples,.5f);
            wp_renderer_dx11_set_output_options(renderer,1,1,2);
            BOOST_REQUIRE(wp_renderer_dx11_begin_hdr_scene(renderer,1));
            auto *depth=static_cast<ID3D11Texture2D*>(wp_renderer_dx11_get_scene_depth_texture(renderer));
            BOOST_REQUIRE(depth);
            D3D11_TEXTURE2D_DESC description{};
            depth->GetDesc(&description);
            BOOST_CHECK_EQUAL(description.Width,16u);
            BOOST_CHECK_EQUAL(description.Height,8u);
            BOOST_CHECK_EQUAL(description.SampleDesc.Count,unsigned(samples));
            BOOST_CHECK(description.BindFlags & D3D11_BIND_SHADER_RESOURCE);
            wp_renderer_dx11_set_clear_color(renderer,2,.5f,.25f,1);
            wp_renderer_dx11_clear(renderer,WORKPHONE_CLEAR_FLAG_ALL);
            wp_renderer_dx11_end_hdr_scene(renderer);
            std::array<unsigned char,32*16*4> pixels{};
            BOOST_REQUIRE(wp_renderer_dx11_copy_back_buffer_rgba8(renderer,window.window,pixels.data(),pixels.size()));
            const auto middle=(8*32+16)*4;
            BOOST_CHECK(pixels[middle]>pixels[middle+1]);
            BOOST_CHECK(pixels[middle+1]>pixels[middle+2]);
            BOOST_CHECK(pixels[middle]>150u);
            wp_renderer_dx11_end_frame(renderer);
            ++frames;
        }
        BOOST_CHECK_EQUAL(wp_renderer_dx11_get_hdr_completed_frames(renderer),frames);
        std::array<unsigned char,32*16*4> plain{},smooth{};
        for(int aa:{0,1})
        {
            wp_renderer_dx11_begin_frame(renderer);
            wp_renderer_dx11_set_scene_sampling(renderer,1,1);
            wp_renderer_dx11_set_post_options(renderer,aa,0,0,1);
            wp_renderer_dx11_set_output_options(renderer,0,0,0);
            BOOST_REQUIRE(wp_renderer_dx11_begin_hdr_scene(renderer,1));
            wp_renderer_dx11_set_clear_color(renderer,0,0,0,1);
            wp_renderer_dx11_clear(renderer,WORKPHONE_CLEAR_FLAG_ALL);
            wp_renderer_dx11_set_cull_mode(renderer,WORKPHONE_CULL_MODE_NONE);
            const wp_vertex_pc vertices[]={{{-1,-1,0},0xffffffff},{{-1,1,0},0xffffffff},{{1,-1,0},0xffffffff}};
            wp_renderer_dx11_draw_triangles_pc(renderer,vertices,3);
            wp_renderer_dx11_end_hdr_scene(renderer);
            auto &out=aa ? smooth : plain;
            BOOST_REQUIRE(wp_renderer_dx11_copy_back_buffer_rgba8(renderer,window.window,out.data(),out.size()));
            wp_renderer_dx11_end_frame(renderer);
        }
        unsigned changed=0;
        for(size_t i=0;i<plain.size();i+=4) if(plain[i]!=smooth[i]) ++changed;
        BOOST_CHECK_GT(changed,0u);

        // A local blue point light must reach all actual scene algorithms, including
        // the separate Deferred MRT light pass and multisample attachment resolves.
        for(int pipeline:{0,1,2,3}) for(int samples:{1,4})
        {
            if(!(mask & samples)) continue;
            wp_renderer_dx11_begin_frame(renderer);wp_renderer_dx11_set_scene_sampling(renderer,samples,1);
            wp_renderer_dx11_set_pipeline_mode(renderer,pipeline,1);wp_renderer_dx11_set_post_options(renderer,0,0,0,1);wp_renderer_dx11_set_output_options(renderer,0,0,0);
            wp_renderer_dx11_set_irradiance_probes(renderer,nullptr,0,0);
            const float lights[]={0,0,-1,4,0,0,8,0},forward[]={0,0,-1};const unsigned cells[]={0,1},list[]={0};
            BOOST_REQUIRE(wp_renderer_dx11_set_local_lights(renderer,lights,1,cells,list,1,1,1,1,.1f,100,forward));
            BOOST_REQUIRE(wp_renderer_dx11_begin_hdr_scene(renderer,1));
            wp_renderer_dx11_set_clear_color(renderer,0,0,0,1);wp_renderer_dx11_clear(renderer,WORKPHONE_CLEAR_FLAG_ALL);
            wp_renderer_dx11_set_cull_mode(renderer,WORKPHONE_CULL_MODE_NONE);wp_renderer_dx11_set_blend_mode(renderer,WORKPHONE_BLEND_MODE_NONE);
            wp_material_dx11 material{};material.base_color={1,1,1,1};material.surface={0,.5f,1,1};material.uv_transform.z=1;
            material.camera_position={0,0,3,0};material.ambient_color.w=1;wp_renderer_dx11_set_material(renderer,&material);
            const wp_vertex_pntc vertices[]={{{-1,-1,0},{0,0,1},{0,0},0xffffffff},{{-1,1,0},{0,0,1},{0,0},0xffffffff},{{1,-1,0},{0,0,1},{0,0},0xffffffff}};
            const unsigned short indices[]={0,1,2};auto *geometry=wp_renderer_dx11_create_indexed_geometry_pntc(renderer,vertices,3,indices,3,0);
            BOOST_REQUIRE(geometry);wp_renderer_dx11_draw_geometry_pntc(renderer,geometry,0,3,0);wp_renderer_dx11_end_hdr_scene(renderer);
            BOOST_REQUIRE(wp_renderer_dx11_copy_back_buffer_rgba8(renderer,window.window,plain.data(),plain.size()));
            BOOST_CHECK_GT(plain[(8*32+4)*4+2],20u);BOOST_CHECK_LT(plain[(8*32+4)*4],5u);
            wp_renderer_dx11_destroy_geometry(geometry);wp_renderer_dx11_end_frame(renderer);
        }
        // Probe radiance is sampled by the native PBR shader, not only uploaded.
        unsigned red[2]{};
        for(int gi:{0,1})
        {
            wp_renderer_dx11_begin_frame(renderer);wp_renderer_dx11_set_scene_sampling(renderer,1,1);wp_renderer_dx11_set_pipeline_mode(renderer,0,0);
            const float forward[]={0,0,-1};wp_renderer_dx11_set_local_lights(renderer,nullptr,0,nullptr,nullptr,0,1,1,1,.1f,100,forward);
            float probe[40]{};probe[4]=10;BOOST_REQUIRE(wp_renderer_dx11_set_irradiance_probes(renderer,probe,gi?1:0,gi));
            BOOST_REQUIRE(wp_renderer_dx11_begin_hdr_scene(renderer,1));wp_renderer_dx11_set_clear_color(renderer,0,0,0,1);wp_renderer_dx11_clear(renderer,WORKPHONE_CLEAR_FLAG_ALL);
            const wp_vertex_pntc vertices[]={{{-1,-1,0},{0,0,1},{0,0},0xffffffff},{{-1,1,0},{0,0,1},{0,0},0xffffffff},{{1,-1,0},{0,0,1},{0,0},0xffffffff}};
            const unsigned short indices[]={0,1,2};auto *geometry=wp_renderer_dx11_create_indexed_geometry_pntc(renderer,vertices,3,indices,3,0);BOOST_REQUIRE(geometry);
            wp_renderer_dx11_draw_geometry_pntc(renderer,geometry,0,3,0);wp_renderer_dx11_end_hdr_scene(renderer);
            BOOST_REQUIRE(wp_renderer_dx11_copy_back_buffer_rgba8(renderer,window.window,plain.data(),plain.size()));red[gi]=plain[(8*32+4)*4];
            wp_renderer_dx11_destroy_geometry(geometry);wp_renderer_dx11_end_frame(renderer);
        }
        BOOST_CHECK_GT(red[1],red[0]+20u);
        for(int samples:{1,4})
        {
            if(!(mask & samples)) continue;
            for(int ao:{0,1})
            {
                wp_renderer_dx11_begin_frame(renderer);wp_renderer_dx11_set_scene_sampling(renderer,samples,1);wp_renderer_dx11_set_pipeline_mode(renderer,0,ao);
                BOOST_REQUIRE(wp_renderer_dx11_begin_hdr_scene(renderer,1));wp_renderer_dx11_set_clear_color(renderer,0,0,0,1);wp_renderer_dx11_clear(renderer,WORKPHONE_CLEAR_FLAG_ALL);
                const wp_vertex_pc vertices[]={{{-1,-1,.2f},0xffffffff},{{-1,1,.2f},0xffffffff},{{1,-1,.2f},0xffffffff},{{1,-1,.2f},0xffffffff},{{-1,1,.2f},0xffffffff},{{1,1,.2f},0xffffffff},
                    {{-.3f,-.6f,-.2f},0xffffffff},{{-.3f,.6f,-.2f},0xffffffff},{{.3f,-.6f,-.2f},0xffffffff},{{.3f,-.6f,-.2f},0xffffffff},{{-.3f,.6f,-.2f},0xffffffff},{{.3f,.6f,-.2f},0xffffffff}};
                wp_renderer_dx11_draw_triangles_pc(renderer,vertices,12);wp_renderer_dx11_end_hdr_scene(renderer);auto &out=ao?smooth:plain;
                BOOST_REQUIRE(wp_renderer_dx11_copy_back_buffer_rgba8(renderer,window.window,out.data(),out.size()));wp_renderer_dx11_end_frame(renderer);
            }
            unsigned darker=0;for(size_t i=0;i<plain.size();i+=4) if(smooth[i]<plain[i]) ++darker;BOOST_CHECK_GT(darker,0u);
        }
        for(int frame=0;frame<3;++frame)
        {
            const int width=frame==1?40:32,height=frame==1?20:16;BOOST_REQUIRE(wp_renderer_dx11_resize(renderer,width,height));
            wp_renderer_dx11_begin_frame(renderer);wp_renderer_dx11_set_scene_sampling(renderer,4,.5f);wp_renderer_dx11_set_pipeline_mode(renderer,2,0);
            BOOST_REQUIRE(wp_renderer_dx11_begin_hdr_scene(renderer,1));wp_renderer_dx11_set_clear_color(renderer,.25f,.5f,.75f,1);wp_renderer_dx11_clear(renderer,WORKPHONE_CLEAR_FLAG_ALL);wp_renderer_dx11_end_hdr_scene(renderer);
            std::vector<unsigned char> pixels(width*height*4);BOOST_REQUIRE(wp_renderer_dx11_copy_back_buffer_rgba8(renderer,window.window,pixels.data(),pixels.size()));
            const auto offset=(height/2*width+width/2)*4;BOOST_CHECK_GT(pixels[offset+2],100u);BOOST_CHECK_LT(pixels[offset],100u);
            wp_renderer_dx11_end_frame(renderer);BOOST_CHECK_GE(wp_renderer_dx11_try_present(renderer,window.window,0),0);
        }
        wp_renderer_dx11_destroy(renderer);
    }
    wp_renderer_dx11_set_d3d12_execution(0);
    wp_renderer_dx11_set_startup_options(0,0,0,1);
}
#endif
#endif
