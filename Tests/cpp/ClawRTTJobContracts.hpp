#pragma once
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawTextureManager.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WorkphoneGraphics/workphone_graphics_system.h>
#include <thread>
#include <stdexcept>
#include <vector>
#include <cstring>

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
    wp_renderer_dx11_set_render_texture(r, nullptr);
    wp_renderer_dx11_set_viewport(r, {3,5,64,48});
    wp_renderer_dx11_set_scissor_enabled(r, 1);
    wp_renderer_dx11_set_scissor_rect(r, {7,9,21,25});
    ID3D11RenderTargetView *original = nullptr;
    ID3D11DepthStencilView *originalDepth = nullptr;
    context->OMGetRenderTargets(1, &original, &originalDepth);
    D3D11_VIEWPORT previous{}; UINT viewportCount = 1; context->RSGetViewports(&viewportCount, &previous);
    d.nested = true;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, dependency, validate, draw, &d) == WP_RTT_OK,
            "RTT scene must publish successfully");
    auto lastGood = pixels(target);
    require(std::abs(int(lastGood[0]) - 64) <= 1 && std::abs(int(lastGood[1]) - 128) <= 1 &&
                std::abs(int(lastGood[2]) - 191) <= 1 && lastGood[3] == 255 &&
                std::memcmp(lastGood.data(), &lastGood[(16 * 32 + 16) * 4], 4) != 0,
            "RTT clear and real triangle draw must reach destination pixels");
    const auto restored = wp_renderer_dx11_get_viewport(r);
    ID3D11RenderTargetView *after = nullptr; ID3D11DepthStencilView *afterDepth = nullptr;
    context->OMGetRenderTargets(1, &after, &afterDepth);
    D3D11_VIEWPORT afterViewport{}; viewportCount = 1; context->RSGetViewports(&viewportCount, &afterViewport);
    require(after == original && afterDepth == originalDepth && restored.x == 3 && restored.y == 5 &&
                restored.width == 64 && restored.height == 48 &&
                std::memcmp(&previous, &afterViewport, sizeof(previous)) == 0,
            "RTT must restore native and actual context outputs and viewport");
    if(after) after->Release(); if(afterDepth) afterDepth->Release();
    if(original) original->Release(); if(originalDepth) originalDepth->Release();
    d.fail = true; d.nested = false; d.validations = 0; d.desc.colour[0] = 1;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, dependency, validate, draw, &d) == WP_RTT_BACKEND_FAILED &&
                pixels(target) == lastGood, "failed RTT must retain every last-good output pixel");
    d.fail = false; d.stale = true; d.validations = 0;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, dependency, validate, draw, &d) == WP_RTT_STALE &&
                pixels(target) == lastGood, "stale post-draw scene result must not publish");
    d.stale = false; d.pending = true;
    const auto draws = d.draws;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, dependency, validate, draw, &d) == WP_RTT_NOT_READY &&
                d.draws == draws && pixels(target) == lastGood, "pending cull dependency must not bind or draw");
    d.pending = false; ++d.desc.target_generation;
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, nullptr, nullptr, draw, &d) == WP_RTT_STALE,
            "stale target generation must reject");
    --d.desc.target_generation;
    auto *view = static_cast<ID3D11ShaderResourceView *>(wp_renderer_dx11_get_render_texture_view(target));
    context->PSSetShaderResources(7, 1, &view);
    require(wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, nullptr, nullptr, draw, &d) == WP_RTT_HAZARD &&
                pixels(target) == lastGood, "sampled destination outside slot zero must reject without writes");
    view = nullptr; context->PSSetShaderResources(7, 1, &view);
    wp_rtt_result threadResult = WP_RTT_OK;
    std::thread worker([&] { threadResult = wp_renderer_dx11_execute_rtt_job(r, target, &d.desc, nullptr, nullptr, draw, &d); });
    worker.join();
    require(threadResult == WP_RTT_WRONG_THREAD, "RTT GPU execution must reject another thread");
    wp_renderer_dx11_destroy_render_texture(target);
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
class Graphics : public ClawHammerSystem {
public:
    bool fail = false;
    unsigned draws = 0;
    void initialise(SmartPtr<ClawRendererDX11> renderer, SmartPtr<Textures> textures) {
        m_sys = wp_graphics_system_create(); m_renderer = renderer; m_textureManager = textures;
    }
    void frame() { prepareFramePasses(.016f); wp_graphics_system_render(m_sys); }
    void render() override {
        ++draws;
        auto renderer = dynamic_pointer_cast<ClawRendererDX11>(getRenderer());
        triangle(wp_renderer_get_dx11(renderer->getNativeRenderer()));
        if(fail) throw std::runtime_error("injected Claw RTT scene failure");
    }
    void detach() { m_renderer = nullptr; m_textureManager = nullptr; }
};
inline bool integration(SmartPtr<IGraphicsWindow> window) {
    auto renderer = make_ptr<ClawRendererDX11>(); renderer->load(window);
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
    const auto oldGeneration = wp_renderer_dx11_get_render_texture_generation(output);
    target->setSize({INT_MAX,INT_MAX});
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
