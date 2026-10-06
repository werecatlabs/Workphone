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
        const bool native = dx11 || software;
        result.maxSkinJoints = native ? WP_SKIN_MAX_JOINTS : 0;
        result.maxParticleCapacity = native ? WP_PARTICLE_SIMULATION_MAX_CAPACITY : 0;
        using Status = ClawFeatureStatus;
        result.features = {
            { "window-presentation", native ? Status::Implemented : dx12 ? Status::Experimental : Status::Unavailable, "DX12 presentation is incomplete; other backends are unavailable." },
            { "static-meshes", native ? Status::Implemented : dx12 ? Status::Experimental : Status::Unavailable, "DX12 mesh/material paths are incomplete." },
            { "pbr-materials", dx11 ? Status::Implemented : Status::Unavailable, "DX11 forward material path; light and shader limits apply." },
            { "cpu-skinning", native ? Status::Experimental : Status::Unavailable, "Explicit bind vertices and joint palette; 4 influences, 256 joints, uniform scale. Import/controller bridge pending." },
            { "gpu-skinning", Status::Unavailable, "GPU palettes and skinned vertex shader pending." },
            { "animation-controller", Status::Unavailable, "Graph and IK tests do not certify asset-to-screen animation." },
            { "particle-simulation", native ? Status::Experimental : Status::Unavailable, "One point emitter; deterministic 120 Hz CPU simulation; no template parser or timed pause." },
            { "particle-rendering", dx11 ? Status::Experimental : Status::Unavailable, "Batched camera-facing billboards; no soft depth, trails, flipbooks or global transparent sorting yet." },
            { "water", Status::Unavailable, "Water rendering passes pending." },
            { "gpu-post-processing", Status::Unavailable, "Existing CPU post-processing is software-only." },
            { "cpu-post-processing", software ? Status::Implemented : Status::Unavailable, "Default-window CPU framebuffer path; not a GPU frame pipeline." },
            { "runtime-ui", native ? Status::Implemented : Status::Unavailable, "DX12 native renderer bridge is missing." },
            { "device-recovery", Status::Unavailable, "Full resource reconstruction and release certification pending." },
            { "runtime-renderer-switch", Status::Unavailable, "Selection takes effect after restart." }
        };
        return result;
    }
}
