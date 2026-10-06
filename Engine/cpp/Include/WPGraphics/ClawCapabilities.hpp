#ifndef CLAW_CAPABILITIES_HPP
#define CLAW_CAPABILITIES_HPP
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone::render
{
    enum class ClawFeatureStatus { Unavailable, Experimental, Implemented };
    struct ClawFeatureCapability
    {
        const char *name;
        ClawFeatureStatus status;
        const char *limitation;
    };
    struct ClawCapabilities
    {
        IGraphicsSystem::RenderApi backend;
        bool productionCertified = false;
        u32 maxSkinJoints = 0;
        u32 maxParticleCapacity = 0;
        Array<ClawFeatureCapability> features;
    };
    /** Implementation inventory, not an assertion that a device is available.
     * Certification requires the independent release gates in the production plan. */
    WPGraphics_API ClawCapabilities getClawCapabilities( IGraphicsSystem::RenderApi backend );
}
#endif
