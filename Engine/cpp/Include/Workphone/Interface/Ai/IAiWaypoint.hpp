#ifndef IAiWaypoint_h__
#define IAiWaypoint_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    class IAiWaypoint : public ISharedObject
    {
    public:
        ~IAiWaypoint() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAiWaypoint_h__
