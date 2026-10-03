#ifndef IUIAbout_h__
#define IUIAbout_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** About dialog interface. */
        class WPCore_API IUIAbout : public IUIElement
        {
        public:
            IUIAbout();

            IUIAbout( u32 poolTypeId );

            /** Virtual destructor. */
            ~IUIAbout() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIAbout_h__
