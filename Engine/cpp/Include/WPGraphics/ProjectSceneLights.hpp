#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
namespace workphone::render
{
    /** Upload per-camera point-light lists. Caller owns the graphics/context lock. */
    WPGraphics_API bool uploadProjectSceneLights( void *nativeRenderer, SmartPtr<IGraphicsScene> scene,
        SmartPtr<IGraphicsCamera> camera, SmartPtr<Properties> settings );
}
