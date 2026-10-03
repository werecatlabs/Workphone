#ifndef IGUIAnimator_h__
#define IGUIAnimator_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class WPCore_API IUIAnimator : public IUIElement
        {
        public:
            IUIAnimator();

            IUIAnimator( u32 poolTypeId );

            /** Virtual destructor. */
            ~IUIAnimator() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IGUIAnimator_h__
