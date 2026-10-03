#ifndef IAiTargetingSystem_h__
#define IAiTargetingSystem_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Class to manage AI targeting.
     */
    class WPCore_API IAiTargetingSystem : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IAiTargetingSystem() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAiTargetingSystem_h__
