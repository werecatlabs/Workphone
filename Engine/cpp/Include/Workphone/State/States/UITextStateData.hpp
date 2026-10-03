#ifndef UITextStateData_h__
#define UITextStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{

    class WPCore_API UITextStateData : public StateData
    {
    public:
        UITextStateData();
        ~UITextStateData() override;

        WP_CLASS_REGISTER_DECL;

        FixedString<128> text;        ///< The text content to display.
        f32 textSize = 1.0f;          ///< The size of the text.
        u8 verticalAlignment = 3u;    ///< Vertical alignment value (implementation-specific).
        u8 horizontalAlignment = 3u;  ///< Horizontal alignment value (implementation-specific).
    };

}  // namespace workphone

#endif  // UITextStateData_h__
