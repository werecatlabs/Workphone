#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone::scene
{
    class TerrainSystem;

    struct LandscapeFoliageSettings
    {
        bool enabled = true;
        u32 seed = 42;
        f32 seaLevel = 0, candidateSpacing = 8;
        f32 palmDensity = .004f, bushDensity = .006f, grassDensity = .012f;
        String assetDirectory = "Assets/Landscape/Foliage";
    };

    // Stage persistent procedural species and native scatter, then commit one
    // complete authoring document. Failure retains the previous population.
    // Sea level and spacing are expressed in terrain-local metres.
    WPCore_API bool configureLandscapeFoliage( TerrainSystem *terrain,
                                               const LandscapeFoliageSettings &settings, String &error );
    WPCore_API bool getLandscapeFoliageSettings( TerrainSystem *terrain,
                                                 LandscapeFoliageSettings &settings );
}  // namespace workphone::scene
