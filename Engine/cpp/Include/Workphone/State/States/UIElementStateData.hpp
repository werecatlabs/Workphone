#ifndef UIElementState_h__
#define UIElementState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{

    class WPCore_API UIElementStateData : public StateData
    {
    public:
        UIElementStateData();
        ~UIElementStateData() override;

        WP_CLASS_REGISTER_DECL;

        SmartPtr<render::IMaterial> material;
        SmartPtr<render::ITexture> texture;

        ColourF colour = ColourF::White;

        hash_type elementId = 0;

        u8 textSize = 12;

        u16 flags = 0;

        String label;
        String name;
        String caption;
    };

}  // namespace workphone

#endif  // UIElementState_h__
