#ifndef TextureSamplerState_h__
#define TextureSamplerState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{

    class WPCore_API TextureSamplerState : public StateData
    {
    public:
        TextureSamplerState();

        ~TextureSamplerState() override;

        FilterOptions minFilter;
        FilterOptions magFilter;
        FilterOptions mipFilter;

        TextureAddressingMode addressingModeU = TextureAddressingMode::Wrap;
        TextureAddressingMode addressingModeV = TextureAddressingMode::Wrap;
        TextureAddressingMode addressingModeW = TextureAddressingMode::Wrap;

        /// Defaults to NUM_COMPARE_FUNCTIONS which means disabled.
        CompareFunction compareFunction;
        ColourF borderColour;
        f32 minLod;
        f32 maxLod;

        /// This allows you to adjust the mipmap calculation up or down for a
        /// given texture unit. Negative values force a larger mipmap to be used,
        /// positive values force a smaller mipmap to be used. Units are in numbers
        /// of levels, so +1 forces the mipmaps to one smaller level.
        /// @note Only does something if render system has capability RSC_MIPMAP_LOD_BIAS.
        f32 mipLodBias;
        f32 maxAnisotropy;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // TextureSamplerState_h__
