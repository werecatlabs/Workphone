#ifndef IPathfinder3_h__
#define IPathfinder3_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    /** Interface for a 3d pathfinder.
     */
    class WPCore_API IPathfinder3 : public ISharedObject
    {
    public:
        ~IPathfinder3() override;

        virtual Array<IPathNode3> getNodes() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
