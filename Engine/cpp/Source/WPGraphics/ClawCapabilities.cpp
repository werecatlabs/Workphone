#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawCapabilities.hpp>
#include <workphone_graphics_skinning.h>
#include <workphone_graphics_particle_simulation.h>

namespace workphone::render
{
    ClawCapabilities getClawCapabilities( IGraphicsSystem::RenderApi backend )
    {
        ClawCapabilities result;
        result.backend = backend;
        const bool dx11 = backend == IGraphicsSystem::RenderApi::DX11;
        const bool software = backend == IGraphicsSystem::RenderApi::Software;
        const bool dx12 = backend == IGraphicsSystem::RenderApi::DX12;
        const bool hardware = dx11 || dx12;
        const bool native = hardware || software;
        result.maxSkinJoints = native ? WP_SKIN_MAX_JOINTS : 0;
        result.maxParticleCapacity = native ? WP_PARTICLE_SIMULATION_MAX_CAPACITY : 0;
        using Status = ClawFeatureStatus;
        result.features = {
            { "window-presentation",
              native ? Status::Implemented
                     : Status::Unavailable,
              "DX11 and D3D12 via D3D11On12 share native presentation and scene passes." },
            { "static-meshes",
              native ? Status::Implemented
                     : Status::Unavailable,
              "Native mesh submission; GPU backends share material and geometry ownership." },
            { "pbr-materials", hardware ? Status::Implemented : Status::Unavailable,
              "Forward, tiled Forward+, Deferred MRT and Mobile paths; local light list budgets apply." },
            { "cpu-skinning", native ? Status::Experimental : Status::Unavailable,
              "Imported bind weights and Animator pose publication; 4 influences, 256 joints, "
              "rigid/uniform-scale affine transforms. GPU skinning remains unavailable." },
            { "gpu-skinning", Status::Unavailable, "GPU palettes and skinned vertex shader pending." },
            { "animation-controller", native ? Status::Experimental : Status::Unavailable,
              "Cached clip restoration and CPU skeletal pose publication; representative "
              "asset/interactive validation is required." },
            { "particle-simulation", native ? Status::Experimental : Status::Unavailable,
              "One point emitter; deterministic 120 Hz CPU simulation; no template parser or timed "
              "pause." },
            { "particle-rendering", hardware ? Status::Experimental : Status::Unavailable,
              "Batched camera-facing billboards; no soft depth, trails, flipbooks or global transparent "
              "sorting yet." },
            { "water", Status::Unavailable, "Water rendering passes pending." },
            { "gpu-post-processing", hardware ? Status::Implemented : Status::Unavailable,
              "Native MSAA resolve, scale, FXAA/temporal history, bloom, depth AO and output transfer." },
            { "cpu-post-processing", software ? Status::Implemented : Status::Unavailable,
              "Default-window CPU framebuffer path; not a GPU frame pipeline." },
            { "runtime-ui", native ? Status::Implemented : Status::Unavailable,
              "Native UI draws after the scene output resolve." },
            { "device-recovery", Status::Unavailable,
              "Full resource reconstruction and release certification pending." },
            { "runtime-renderer-switch", Status::Unavailable, "Selection takes effect after restart." }
        };
        return result;
    }
}  // namespace workphone::render
