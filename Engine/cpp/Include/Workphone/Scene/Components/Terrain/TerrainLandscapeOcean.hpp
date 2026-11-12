#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone::scene
{
    class TerrainSystem;

    /// Configures the marked ocean child only. Coordinates and margin are terrain-local metres.
    WPCore_API bool configureLandscapeOcean( TerrainSystem *terrain, bool enabled, f32 localSeaLevel,
                                             f32 margin, String &error );
}  // namespace workphone::scene
