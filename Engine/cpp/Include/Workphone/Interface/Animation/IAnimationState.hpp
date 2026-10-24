#ifndef IAnimationState_h__
#define IAnimationState_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>

namespace workphone
{

    /** Interface for an IAnimationState. */
    class WPCore_API IAnimationState : public ISharedObject
    {
    public:
        /// Typedef for an array of float values used as a bone blend mask
        using BoneBlendMask = std::vector<float, SourceBudgetAllocator<float>>;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAnimationState_h__
