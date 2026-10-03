#ifndef UIDropdownStateData_h__
#define UIDropdownStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief State data for UI dropdown elements.
     *
     * Stores the list of selectable options and the currently selected index.
     */
    class WPCore_API UIDropdownStateData : public StateData
    {
    public:
        UIDropdownStateData();
        ~UIDropdownStateData() override;

        WP_CLASS_REGISTER_DECL;

        Array<FixedString<256>> options;  ///< The list of available options.
        u32 selectedOption = 0u;          ///< The index of the currently selected option.
    };

}  // namespace workphone

#endif  // UIDropdownStateData_h__
