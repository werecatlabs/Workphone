#ifndef IUIInputManager_h__
#define IUIInputManager_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a input manager. */
        class WPCore_API IUIInputManager : public IUIElement
        {
        public:
            IUIInputManager();

            IUIInputManager( u32 poolTypeId );

            /** Destructor. */
            ~IUIInputManager() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIInputManager_h__
