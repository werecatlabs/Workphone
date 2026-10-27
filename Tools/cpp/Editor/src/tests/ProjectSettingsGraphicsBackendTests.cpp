#include <EditorPCH.hpp>
#include <editor/ProjectSettingsGraphicsBackend.hpp>
#include <Workphone/Workphone.hpp>

#if WP_EDITOR_TESTS
#include <boost/test/unit_test.hpp>
#include <WorkphoneGraphics/workphone_graphics_hdr.h>
#include <WorkphoneGraphics/workphone_graphics_pipeline.h>
#include <Workphone/Graphics/GraphicsPipeline.hpp>
#include <array>
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
    for(int driver:{0,3})
    {
        wp_renderer_dx11_set_startup_options(driver,1,0,1);
        auto *renderer=wp_renderer_dx11_create(window.window,32,16);
        BOOST_REQUIRE_MESSAGE(renderer,"DX11 device creation failed");
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
        wp_renderer_dx11_destroy(renderer);
    }
    wp_renderer_dx11_set_startup_options(0,0,0,1);
}
#endif
#endif
