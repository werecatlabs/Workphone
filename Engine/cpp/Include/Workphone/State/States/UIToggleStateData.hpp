#ifndef UIToggleStateData_h__
#define UIToggleStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>

namespace workphone
{

    /**
     * @brief State data for UI toggle elements.
     *
     * Contains the state for toggle buttons, checkboxes, and radio buttons.
     */
    class WPCore_API UIToggleStateData : public StateData
    {
    public:
        UIToggleStateData();
        ~UIToggleStateData() override;

        WP_CLASS_REGISTER_DECL;

        FixedString<128> label;  ///< The label text for the toggle.
        f32 textSize = 1.0f;     ///< The size of the label text.
        bool checked = false;    ///< Whether the toggle is checked/toggled on.
        bool showLabel = true;   ///< Whether to show the label.
        u8 toggleType = 0u;      ///< Type of toggle (checkbox, radio, toggle button).
        u8 toggleState = 0u;     ///< State of toggle (off, on, indeterminate).
    };

}  // namespace workphone

#endif  // UIToggleStateData_h__
