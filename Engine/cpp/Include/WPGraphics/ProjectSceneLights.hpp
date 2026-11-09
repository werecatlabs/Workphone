#pragma once
#include <Workphone/Memory/SmartPtr.hpp>
#include <WPGraphics/WPClawHammerPrerequisites.hpp>

namespace workphone::render
{
    struct GraphicsPipelineRenderSettings;
    /** Upload per-camera point-light lists. Caller owns the graphics/context lock. */
    WPGraphics_API bool uploadProjectSceneLights( void *nativeRenderer, SmartPtr<IGraphicsScene> scene,
                                                  SmartPtr<IGraphicsCamera> camera,
                                                  const GraphicsPipelineRenderSettings &settings );
}  // namespace workphone::render
