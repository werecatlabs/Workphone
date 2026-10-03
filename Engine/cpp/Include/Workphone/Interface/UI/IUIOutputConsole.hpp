#ifndef IUIOutputConsole_h__
#define IUIOutputConsole_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        class WPCore_API IUIOutputConsole : public IUIElement
        {
        public:
            ~IUIOutputConsole() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIOutputConsole_h__
