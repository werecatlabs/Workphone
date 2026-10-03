#ifndef TerrainState_h__
#define TerrainState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{

    class WPCore_API TerrainStateData : public StateData
    {
    public:
        TerrainStateData();

        ~TerrainStateData() override;

        WP_CLASS_REGISTER_DECL;

        SmartPtr<render::ITexture> heightMap;

        Vector2I heightMapSize = Vector2I( 256, 256 );
        f32 heightScale = 50.0f;

        String materialName;

        /// When true, the terrain is rendered in wireframe mode.
        bool showWireframe = false;

        Array<SmartPtr<render::ITexture>> textures;
        Array<f32> metalnessValues;

        /// Raw height samples for the terrain (row-major: z*width + x).
        Array<f32> heightData;
    };
}  // namespace workphone

#endif  // TerrainState_h__
