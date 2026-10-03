#ifndef IGUIAnimatorScale_h__
#define IGUIAnimatorScale_h__

#include <Workphone/Interface/UI/IUIAnimator.hpp>

namespace workphone
{
    namespace ui
    {

        class WPCore_API IUIAnimatorScale : public IUIAnimator
        {
        public:
            /** Virtual destructor. */
            ~IUIAnimatorScale() override = default;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IGUIAnimatorScale_h__
