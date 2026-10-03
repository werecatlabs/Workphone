#ifndef IPathNode3_h__
#define IPathNode3_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /** Interface for a 3d path node.
     */
    class WPCore_API IPathNode3 : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IPathNode3() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IPathNode2_h__
