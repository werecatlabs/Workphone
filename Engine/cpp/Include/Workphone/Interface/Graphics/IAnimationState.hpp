#ifndef __WP_IAnimationState_h__
#define __WP_IAnimationState_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {

        class WPCore_API IAnimationState : public ISharedObject
        {
        public:
            ~IAnimationState() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // IAnimationState_h__
