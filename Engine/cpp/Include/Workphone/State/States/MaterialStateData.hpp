#ifndef __MaterialStateData_h__
#define __MaterialStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    class Properties;

    class WPCore_API MaterialStateData : public StateData
    {
    public:
        MaterialStateData();
        ~MaterialStateData() override;

        WP_CLASS_REGISTER_DECL;

        FixedArray<SmartPtr<render::ITexture>, (u32)PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES> textures;

        FixedString<WP_MAX_PATH> filePath = "None";
        FixedString<WP_MAX_PATH> materialName;

        f32 metalness = 0.5f;
        f32 roughness = 0.01f;
    };

}  // namespace workphone

#endif  // __MaterialStateData_h__
