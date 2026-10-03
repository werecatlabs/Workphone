#ifndef UIAnchorState_h__
#define UIAnchorState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{

    class WPCore_API UIAnchorStateData : public StateData
    {
    public:
        UIAnchorStateData();
        ~UIAnchorStateData() override;

        Vector2<real_Num> anchor = Vector2<real_Num>( 0.0f, 1.0f );
        Vector2<real_Num> anchorMin = Vector2<real_Num>( 0.0f, 1.0f );
        Vector2<real_Num> anchorMax = Vector2<real_Num>( 0.0f, 1.0f );

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // UIAnchorState_h__
