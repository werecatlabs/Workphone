#ifndef IAnimationState_h__
#define IAnimationState_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Interface for an IAnimationState. */
    class WPCore_API IAnimationState : public ISharedObject
    {
    public:
        /// Typedef for an array of float values used as a bone blend mask
        typedef std::vector<float> BoneBlendMask;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAnimationState_h__
