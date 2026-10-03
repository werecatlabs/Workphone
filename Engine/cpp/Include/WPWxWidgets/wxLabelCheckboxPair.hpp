#ifndef wxLabelCheckboxPair_h__
#define wxLabelCheckboxPair_h__

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <WPWxWidgets/WPWxApplicationWindow.hpp>

namespace workphone
{
    namespace ui
    {

        class wxLabelCheckboxPair : public wxApplicationWindow
        {
        public:
            wxLabelCheckboxPair();
            ~wxLabelCheckboxPair();

            void load( SmartPtr<ISharedObject> data ) override;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // wxLabelCheckboxPair_h__
