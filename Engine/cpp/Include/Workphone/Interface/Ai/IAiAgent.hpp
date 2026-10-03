#ifndef IAiAgent_h__
#define IAiAgent_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** A base ai class.
     */
    class WPCore_API IAiAgent : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IAiAgent() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAiAgent_h__
