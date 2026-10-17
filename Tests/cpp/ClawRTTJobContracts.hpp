#pragma once
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawTextureManager.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WorkphoneGraphics/workphone_graphics_cull_job.h>
#include <WorkphoneGraphics/workphone_graphics_transform_job.h>
#include <WorkphoneGraphics/workphone_graphics_scenenode.h>
#include <Workphone/Core/Properties.hpp>
#include <WorkphoneGraphics/workphone_graphics_system.h>
#include <thread>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <memory>
#include <wrl/client.h>

namespace claw_rtt_contracts {
using namespace workphone;
using namespace workphone::render;
inline void require(bool value, const char *message) { if(!value) throw std::runtime_error(message); }
struct Draw {
    wp_renderer_dx11 *r;
    wp_render_texture_dx11 *target;
    wp_graphics_rtt_desc desc;
    HWND window = nullptr;
    bool fail = false, stale = false, pending = false, nested = false;
    bool sample_destination = false, transient_sample = false;
    unsigned validations = 0, draws = 0;
};
inline void triangle(wp_renderer_dx11 *r) {
    wp_mat4f identity{};
    for(int i = 0; i < 4; ++i) identity.m[i][i] = 1;
    wp_renderer_dx11_set_world_matrix(r, &identity);
    wp_renderer_dx11_set_view_matrix(r, &identity);
    wp_renderer_dx11_set_projection_matrix(r, &identity);
    wp_renderer_dx11_set_cull_mode(r, WORKPHONE_CULL_MODE_NONE);
    wp_renderer_dx11_set_blend_mode(r, WORKPHONE_BLEND_MODE_NONE);
    wp_renderer_dx11_set_fill_mode(r, WORKPHONE_FILL_MODE_SOLID);
    wp_renderer_dx11_set_depth_test_enabled(r, 0);
    wp_renderer_dx11_set_scissor_enabled(r, 0);
    const wp_vertex_pc vertices[] = {{{-.75f,-.75f,.5f},0xff0000ffu},
        {{.75f,-.75f,.5f},0xff0000ffu},{{0,.75f,.5f},0xff0000ffu}};
    wp_renderer_dx11_draw_triangles_pc(r, vertices, 3);
}
inline wp_rtt_result dependency(void *p, const wp_graphics_rtt_desc *) {
    return static_cast<Draw *>(p)->pending ? WP_RTT_NOT_READY : WP_RTT_OK;
}
inline wp_rtt_result validate(void *p, const wp_graphics_rtt_desc *d) {
    auto &draw = *static_cast<Draw *>(p);
    ++draw.validations;
    return d->scene_generation == 13 && d->view_generation == 19 &&
           !(draw.stale && draw.validations == 2) ? WP_RTT_OK : WP_RTT_STALE;
}
inline wp_rtt_result draw(void *p, const wp_graphics_rtt_desc *) {
    auto &d = *static_cast<Draw *>(p); ++d.draws;
    try {
    if(d.nested) {
        require(wp_renderer_dx11_execute_rtt_job(d.r, d.target, &d.desc, nullptr, nullptr, draw, p) == WP_RTT_BUSY,
                "nested RTT must reject");
        require(!wp_renderer_dx11_resize(d.r, 64, 64) && !wp_renderer_dx11_try_present(d.r,
                    d.window, 0), "RTT must block resize and presentation");
    }
    triangle(d.r);
    if(d.transient_sample) {
        wp_renderer_dx11_set_texture_native(d.r, wp_renderer_dx11_get_render_texture_view(d.target));
        wp_renderer_dx11_set_texture_native(d.r, nullptr);
    }
    if(d.sample_destination) {
        auto *ctx = static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(d.r));
        auto *view = static_cast<ID3D11ShaderResourceView *>(wp_renderer_dx11_get_render_texture_view(d.target));
        ctx->PSSetShaderResources(7, 1, &view);
    }
    return d.fail ? WP_RTT_BACKEND_FAILED : WP_RTT_OK;
    } catch(...) { return WP_RTT_BACKEND_FAILED; }
}
inline std::vector<unsigned char> pixels(wp_render_texture_dx11 *target) {
    std::vector<unsigned char> value(32 * 32 * 4);
    require(wp_renderer_dx11_copy_render_texture_rgba8(target, value.data(), value.size()), "RTT readback");
    return value;
}
inline bool native(wp_renderer_dx11 *r) {
    auto *context = static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(r));
    auto *target = wp_renderer_dx11_create_render_texture(r, 32, 32);
    require(target != nullptr, "RTT destination allocation");
    std::unique_ptr<wp_render_texture_dx11, decltype(&wp_renderer_dx11_destroy_render_texture)>
        owned(target, &wp_renderer_dx11_destroy_render_texture);
    Draw d{r, target, {}};
    DXGI_SWAP_CHAIN_DESC swap{};
    require(SUCCEEDED(static_cast<IDXGISwapChain *>(wp_renderer_dx11_get_swap_chain(r))->GetDesc(&swap)), "RTT swap chain window");
    d.window = swap.OutputWindow;
    d.desc.renderer_generation = wp_renderer_dx11_get_rtt_generation(r);
    d.desc.target_generation = wp_renderer_dx11_get_render_texture_generation(target);
    d.desc.scene_generation = 13; d.desc.view_generation = 19;
    d.desc.width = d.desc.height = 32; d.desc.viewport = {0,0,32,32};
    d.desc.clear_colour = d.desc.clear_depth = 1;
    d.desc.colour[0] = .25f; d.desc.colour[1] = .5f; d.desc.colour[2] = .75f; d.desc.colour[3] = 1;
    d.desc.depth = 1;
    wp_renderer_dx11_set_render_texture(r, target);
    wp_renderer_dx11_set_viewport(r, {0,0,32,32});
    wp_renderer_dx11_set_clear_color(r,.25f,.5f,.75f,1);
    wp_renderer_dx11_clear(r,WORKPHONE_CLEAR_FLAG_ALL);
    triangle(r);
    auto reference = pixels(target);
    wp_renderer_dx11_set_render_texture(r, nullptr);
    wp_renderer_dx11_set_viewport(r, {3,5,64,48});
    wp_renderer_dx11_set_scissor_enabled(r, 1);
    wp_renderer_dx11_set_scissor_rect(r, {7,9,21,25});
    auto *device = static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(r));
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend;
    D3D11_BLEND_DESC blendDescription{};
    blendDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_GREEN;
    require(SUCCEEDED(device->CreateBlendState(&blendDescription, blend.GetAddressOf())), "RTT previous blend state");
    const float factors[] = {.2f,.4f,.6f,.8f};
    context->OMSetBlendState(blend.Get(), factors, 0x55555555);
    const float sourcePixel[] = {.5f,.25f,.125f,1};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> source;
    source.Attach(static_cast<ID3D11ShaderResourceView *>(wp_renderer_dx11_create_texture_rgba32f_native(
        r, sourcePixel, 1, 1, 4)));
    require(source != nullptr, "RTT previous shader resource");
    auto *sourceView = source.Get();
    context->PSSetShaderResources(9, 1, &sourceView);
    context->VSSetShaderResources(11, 1, &sourceView);
    context->CSSetShaderResources(4, 1, &sourceView);
    ID3D11RenderTargetView *original = nullptr;
    ID3D11DepthStencilView *originalDepth = nullptr;
    context->OMGetRenderTargets(1, &original, &originalDepth);
    if(original) original->Release(); if(originalDepth) originalDepth->Release();
    D3D11_VIEWPORT previous{}; UINT viewportCount = 1; context->RSGetViewports(&viewportCount, &previous);
    d.nested = true;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, dependency, validate, draw, &d) == WP_RTT_OK,
            "RTT scene must publish successfully");
    auto lastGood = pixels(target);
    require(std::abs(int(lastGood[0]) - 64) <= 1 && std::abs(int(lastGood[1]) - 128) <= 1 &&
                std::abs(int(lastGood[2]) - 191) <= 1 && lastGood[3] == 255 &&
                std::memcmp(lastGood.data(), &lastGood[(16 * 32 + 16) * 4], 4) != 0,
            "RTT clear and real triangle draw must reach destination pixels");
    require(lastGood == reference, "native RTT output must match the complete synchronous reference image");
    const auto restored = wp_renderer_dx11_get_viewport(r);
    ID3D11RenderTargetView *after = nullptr; ID3D11DepthStencilView *afterDepth = nullptr;
    context->OMGetRenderTargets(1, &after, &afterDepth);
    D3D11_VIEWPORT afterViewport{}; viewportCount = 1; context->RSGetViewports(&viewportCount, &afterViewport);
    require(after == original && afterDepth == originalDepth && restored.x == 3 && restored.y == 5 &&
                restored.width == 64 && restored.height == 48 &&
                std::memcmp(&previous, &afterViewport, sizeof(previous)) == 0,
            "RTT must restore native and actual context outputs and viewport");
    if(after) after->Release(); if(afterDepth) afterDepth->Release();
    Microsoft::WRL::ComPtr<ID3D11BlendState> restoredBlend;
    float restoredFactors[4]{}; UINT restoredMask = 0;
    context->OMGetBlendState(restoredBlend.GetAddressOf(), restoredFactors, &restoredMask);
    require(restoredBlend == blend && restoredMask == 0x55555555 &&
                !std::memcmp(factors, restoredFactors, sizeof(factors)), "RTT must restore blend state, factors and sample mask");
    for(unsigned stage = 0; stage < 3; ++stage) {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> restoredSource;
        if(stage == 0) context->PSGetShaderResources(9, 1, restoredSource.GetAddressOf());
        else if(stage == 1) context->VSGetShaderResources(11, 1, restoredSource.GetAddressOf());
        else context->CSGetShaderResources(4, 1, restoredSource.GetAddressOf());
        require(restoredSource == source, "RTT must restore shader resource bindings across stages");
    }
    sourceView = nullptr;
    context->PSSetShaderResources(9, 1, &sourceView);
    context->VSSetShaderResources(11, 1, &sourceView);
    context->CSSetShaderResources(4, 1, &sourceView);
    d.fail = true; d.nested = false; d.validations = 0; d.desc.colour[0] = 1;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, dependency, validate, draw, &d) == WP_RTT_BACKEND_FAILED &&
                pixels(target) == lastGood, "failed RTT must retain every last-good output pixel");
    d.fail = false; d.stale = true; d.validations = 0;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, dependency, validate, draw, &d) == WP_RTT_STALE &&
                pixels(target) == lastGood, "stale post-draw scene result must not publish");
    d.stale = false; d.sample_destination = true;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, dependency, validate, draw, &d) == WP_RTT_HAZARD &&
                pixels(target) == lastGood, "sampling destination during draw must reject publication and restore state");
    d.sample_destination = false; d.transient_sample = true;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, dependency, validate, draw, &d) == WP_RTT_HAZARD &&
                pixels(target) == lastGood, "transient destination sampling must remain rejected after unbinding");
    d.transient_sample = false; d.pending = true;
    const auto draws = d.draws;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, dependency, validate, draw, &d) == WP_RTT_NOT_READY &&
                d.draws == draws && pixels(target) == lastGood, "pending cull dependency must not bind or draw");
    d.pending = false; ++d.desc.target_generation;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, nullptr, nullptr, draw, &d) == WP_RTT_STALE,
            "stale target generation must reject");
    --d.desc.target_generation;
    ++d.desc.renderer_generation;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, nullptr, nullptr, draw, &d) == WP_RTT_STALE,
            "stale renderer generation must reject");
    --d.desc.renderer_generation;
    d.desc.format = WP_RTT_RGBA16_FLOAT; d.desc.colour_space = WP_RTT_SCENE_LINEAR;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, nullptr, nullptr, draw, &d) == WP_RTT_UNSUPPORTED,
            "unsupported HDR output must report its capability limit");
    d.desc.format = WP_RTT_RGBA8_UNORM; d.desc.colour_space = WP_RTT_LEGACY_LDR;
    auto *view = static_cast<ID3D11ShaderResourceView *>(wp_renderer_dx11_get_render_texture_view(target));
    context->PSSetShaderResources(7, 1, &view);
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, nullptr, nullptr, draw, &d) == WP_RTT_HAZARD &&
                pixels(target) == lastGood, "sampled destination outside slot zero must reject without writes");
    view = nullptr; context->PSSetShaderResources(7, 1, &view);
    wp_rtt_result threadResult = WP_RTT_OK;
    std::thread worker([&] { threadResult = wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, nullptr, nullptr, draw, &d); });
    worker.join();
    require(threadResult == WP_RTT_WRONG_THREAD, "RTT GPU execution must reject another thread");
    wp_renderer_dx11_set_scissor_enabled(r, 0);
    std::puts("DX11 RTT pixels, output/viewport restoration, rollback, dependencies, stale generations, hazards, recursion and render-owner execution passed.");
    return true;
}

class Texture : public ClawTexture {
public:
    SmartPtr<IRenderTarget> output;
    SmartPtr<IRenderTarget> getRenderTarget() const override { return output; }
    u32 getUsageFlags() const override { return static_cast<u32>(TextureUsage::TU_RENDERTARGET); }
};
class Textures : public ClawTextureManager {
public:
    Array<SmartPtr<ITexture>> values;
    Array<SmartPtr<ITexture>> getTextures() const override { return values; }
};
class SnapshotMesh : public ClawMesh {
public:
    SnapshotMesh() {
        m_mesh = wp_graphics_mesh_create();
        const wp_vertex_pntc vertices[] = {
            {{-.5f,-.5f,-2.f},{0,0,-1},{0,0},0xFFFFFFFFu},
            {{.5f,-.5f,-2.f},{0,0,-1},{1,0},0xFFFFFFFFu},
            {{0,.5f,-2.f},{0,0,-1},{.5f,1},0xFFFFFFFFu}};
        require(m_mesh && wp_graphics_mesh_set_vertices(m_mesh, WORKPHONE_VERTEX_FORMAT_PNTC, vertices, 3),
            "Claw snapshot mesh fixture");
    }
};
class Graphics : public ClawHammerSystem {
public:
    ~Graphics() override { removeScenes(); }
    bool fail = false;
    bool retireSceneAfterDraw = false;
    SmartPtr<ClawScene> nativeScene;
    SmartPtr<ClawMesh> nativeMesh;
    unsigned draws = 0;
    void attachScene(SmartPtr<ClawScene> scene) { nativeScene = scene; m_scenes.push_back(scene); }
    void removeScenes() {
        if(nativeMesh) nativeMesh->bindNativeRenderObject(nullptr);
        nativeMesh = nullptr; m_scenes.clear(); nativeScene = nullptr;
    }
    void initialise(SmartPtr<ClawRendererDX11> renderer, SmartPtr<Textures> textures) {
        m_sys = wp_graphics_system_create(); m_renderer = renderer; m_textureManager = textures;
    }
    void frame() { prepareFramePasses(.016f); wp_graphics_system_render(m_sys); publishRenderStatistics(); }
    void render() override {
        ++draws;
        auto renderer = dynamic_pointer_cast<ClawRendererDX11>(getRenderer());
        if(nativeScene) {
            require(nativeScene->validateRenderJobs(), "frame must publish transform/cull dependency before draw");
            require(nativeScene->getPreparedTransformDispatchCount()==1,
                "active colour/shadow pass must share one transform dispatch");
            wp_transform_job_stats transforms{}; wp_graphics_cull_bounds_stats bounds{};
            wp_u32 candidates=0,visible=0;
            require(nativeScene->getPreparedJobStatistics(&transforms,&bounds,&candidates,&visible) &&
                    candidates==1 && visible==1 && transforms.output_count==1,
                    "active preparation exposes one shared candidate publication");
            const auto flags=wp_graphics_scene_get_cull_flags(nativeScene->getNativeScene());
            require((flags&WP_GRAPHICS_CULL_SPHERE_TEST) || bounds.sphere_tests==0,
                    "active scene honours disabled sphere policy");
            require((flags&WP_GRAPHICS_CULL_SAT_TEST) || bounds.sat_tests==0,
                    "active scene honours disabled SAT policy");
            ClawHammerSystem::render();
            if(retireSceneAfterDraw) {
                if(nativeMesh) nativeMesh->bindNativeRenderObject(nullptr);
                wp_graphics_scene_clear(nativeScene->getNativeScene());
            }
        } else triangle(wp_renderer_get_dx11(renderer->getNativeRenderer()));
        if(fail) throw std::runtime_error("injected Claw RTT scene failure");
    }
    void detach() { m_renderer = nullptr; m_textureManager = nullptr; }
};
class Renderer : public ClawRendererDX11 {
public:
    bool rejectViewport = false;
    void setViewport(SmartPtr<IViewport> value) override {
        if(rejectViewport && value) throw std::runtime_error("injected RTT preparation failure");
        ClawRendererDX11::setViewport(value);
    }
};
inline bool integration(SmartPtr<IGraphicsWindow> window, bool withNativeScene = false) {
    auto renderer = make_ptr<Renderer>(); renderer->load(window);
    require(renderer->isLoaded(), "Claw RTT integration renderer");
    auto target = make_ptr<ClawRenderTarget>(); target->setSize({32,32}); target->setActive(true);
    auto camera = make_ptr<ClawCamera>();
    auto viewport = target->addViewport(19, camera);
    require(viewport != nullptr, "Claw RTT viewport");
    viewport->setActive(true); viewport->setEnableSceneRender(true); viewport->setEnableUI(false);
    viewport->setClearEveryFrame(true, WORKPHONE_CLEAR_FLAG_ALL);
    viewport->setBackgroundColour(ColourF(.25f,.5f,.75f,1));
    auto texture = make_ptr<Texture>(); texture->output = target;
    auto textures = make_ptr<Textures>(); textures->values.push_back(texture);
    Graphics graphics; graphics.initialise(renderer, textures);
    graphics.frame();
    auto *output = target->getNativeRenderTexture(renderer->getNativeRenderer());
    auto good = pixels(output);
    require(graphics.draws == 1 && good[(16 * 32 + 16) * 4] != good[0] && renderer->getRenderTarget() == window,
            "active Claw offscreen pass must execute native job, draw and restore window adapter");
    graphics.fail = true;
    viewport->setBackgroundColour(ColourF(0,1,0,1));
    graphics.frame();
    require(graphics.draws == 2 && pixels(output) == good && renderer->getRenderTarget() == window,
            "active Claw scene exception must preserve last complete texture and restore window");
    renderer->rejectViewport = true;
    graphics.frame();
    require(graphics.draws == 2 && pixels(output) == good && renderer->getRenderTarget() == window,
            "RTT preparation failure before begin must restore the window and preserve output");
    renderer->rejectViewport = false;
    if(withNativeScene) {
    // Exercise real ClawScene consumption within the active RTT frame, then make
    // the completed draw stale before publication. Last complete output survives.
    auto nativeScene = make_ptr<ClawScene>();
    auto object = wp_graphics_scene_create_object(nativeScene->getNativeScene());
    auto nativeNode=wp_graphics_scene_create_node(nativeScene->getNativeScene());
    wp_scenenode_attach_object(nativeNode,object);
    auto snapshotMesh = make_ptr<SnapshotMesh>();
    wp_graphics_mesh_compute_aabb(snapshotMesh->getNativeMesh());
    snapshotMesh->bindNativeRenderObject(object);
    snapshotMesh->setVisibilityFlags(~u32(0));
    nativeScene->setEnableShadows(false);
    graphics.nativeMesh = snapshotMesh;
    graphics.attachScene(nativeScene);
    graphics.fail = false;
    auto request=make_ptr<Properties>(); request->setProperty("request_render_statistics",true);
    graphics.setProperties(request);
    graphics.frame();
    String counters;
    require(graphics.getProperties()->getPropertyValue("render_counters",counters) &&
            counters.find("Preparation last frame: views=1 candidates=1 main-visible=1 transform dispatches=1")!=String::npos,
            "frame telemetry counts colour/shadow publication once after retirement");
    require(counters.find("reference scratch allocs=1 reuses=0")!=String::npos,
            "first active capture allocates reference scratch once");
    require(counters.find("path records allocs=1 reuses=0")!=String::npos,
            "first active capture allocates path records once");
    require(counters.find("index records allocs=1 reuses=0")!=String::npos &&
            counters.find("batch records allocs=1 reuses=0")!=String::npos,
            "active capture reports index and batch storage allocation or reuse");
    require(counters.find("local matrices allocs=1 reuses=0")!=String::npos &&
            counters.find("prefix cache allocs=0 reuses=0")!=String::npos,
            "active single-output capture reports local storage and no optional prefix cache");
    require(counters.find("main sort checks=0 passes=0 scratch allocs=0 bytes=0")!=String::npos,
            "single-output active view skips merge sorting and scratch allocation");
    good = pixels(output);
    require(good[(16*32+16)*4] != good[0] && !nativeScene->validateRenderJobs(),
        "real ClawScene cull result must draw and be retired at frame end");
    wp_transform_job_stats retiredTransforms{}; wp_graphics_cull_bounds_stats retiredBounds{};
    wp_graphics_cull_sort_stats retiredSort{}; retiredSort.scratch_allocations=99;
    wp_u32 retiredCandidates=99,retiredVisible=99;
    require(!nativeScene->getPreparedJobStatistics(&retiredTransforms,&retiredBounds,&retiredCandidates,&retiredVisible,&retiredSort) &&
            !retiredCandidates && !retiredVisible && !retiredTransforms.output_count && !retiredBounds.sphere_tests && !retiredSort.scratch_allocations,
            "retired preparation metadata is zeroed");
    require(wp_graphics_scene_set_cull_flags(nativeScene->getNativeScene(),0)!=0,"disable scene optional cull stages");
    graphics.setProperties(request); graphics.frame();
    require(pixels(output)==good,"cull A/B policy preserves visible RTT pixels");
    require(graphics.getProperties()->getPropertyValue("render_counters",counters) &&
            counters.find("sphere tests=0 rejects=0")!=String::npos &&
            counters.find("SAT tests=0 rejects=0 axes=0")!=String::npos,
            "render statistics expose disabled stage work");
    require(counters.find("reference scratch allocs=0 reuses=1")!=String::npos,
            "warm active scene capture reuses reference scratch without allocation");
    require(counters.find("path records allocs=0 reuses=1")!=String::npos,
            "warm active scene capture also reuses path records");
    require(counters.find("index records allocs=0 reuses=1")!=String::npos &&
            counters.find("batch records allocs=0 reuses=1")!=String::npos,
            "active capture reports index and batch storage allocation or reuse");
    require(counters.find("local matrices allocs=0 reuses=1")!=String::npos &&
            counters.find("prefix cache allocs=0 reuses=0")!=String::npos,
            "active single-output capture reports local storage and no optional prefix cache");
    graphics.retireSceneAfterDraw = true;
    viewport->setBackgroundColour(ColourF(1,0,1,1));
    graphics.frame();
    require(pixels(output) == good && renderer->getRenderTarget() == window,
        "scene retirement after draw must invalidate cull dependency and retain prior RTT output");
    graphics.removeScenes(); nativeScene = nullptr;
    viewport->setActive(false); graphics.setProperties(request); graphics.frame();
    require(graphics.getProperties()->getPropertyValue("render_counters",counters) &&
            counters.find("Preparation last frame: views=0 candidates=0 main-visible=0 transform dispatches=0")!=String::npos,
            "skipped frame does not carry prior preparation counters");
    viewport->setActive(true);
    }
    const auto oldGeneration = wp_renderer_dx11_get_render_texture_generation(output);
    target->setSize({20000,20000});
    require(!target->getNativeRenderTexture(renderer->getNativeRenderer()) &&
                target->getNativeTextureView() != nullptr && pixels(output) == good,
            "failed target replacement must retain the previous complete output");
    target->setSize({48,48});
    output = target->getNativeRenderTexture(renderer->getNativeRenderer());
    require(output && wp_renderer_dx11_get_render_texture_generation(output) != oldGeneration,
            "successful target replacement must invalidate the former generation");
    graphics.detach();
    viewport->setRenderTarget(nullptr); target->removeAllViewports();
    textures->values.clear(); texture->output = nullptr;
    renderer->unload(nullptr);
    std::puts("Active Claw RTT job consumption, scene-exception rollback, target restoration and resize generation replacement passed.");
    return true;
}
}
