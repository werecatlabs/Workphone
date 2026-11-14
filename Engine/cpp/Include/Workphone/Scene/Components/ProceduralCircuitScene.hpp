#pragma once

#include <Workphone/Scene/Components/ProceduralScene.hpp>

namespace workphone::scene
{
    /// Generates a closed circuit, its scenery and static collision under its owner.
    class WPCore_API ProceduralCircuitScene : public ProceduralScene
    {
    public:
        WP_CLASS_REGISTER_DECL;

    protected:
        void buildGeneratedScene() override;
    };
}  // namespace workphone::scene
