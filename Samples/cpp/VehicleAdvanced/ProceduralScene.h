#pragma once
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>

// Compatibility names for the sample's command-line validation and capture harness.
namespace workphone::advanced
{
    using scene::procedural_scene::CircuitSample;
    using scene::procedural_scene::Circuit;
    using scene::procedural_scene::SceneAssets;
    using scene::procedural_scene::generateCircuit;
    using scene::procedural_scene::validateCircuit;
    using scene::procedural_scene::validateReflection;
    using scene::procedural_scene::buildScene;
    using scene::procedural_scene::configurePhysics;
}