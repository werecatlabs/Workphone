#ifndef IPathfinder2_h__
#define IPathfinder2_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    /** Interface for a 2d pathfinder.
     */
    class WPCore_API IPathfinder2 : public ISharedObject
    {
    public:
        enum class State
        {
            SEARCH_STATE_NOT_INITIALISED,
            SEARCH_STATE_SEARCHING,
            SEARCH_STATE_SUCCEEDED,
            SEARCH_STATE_FAILED,
            SEARCH_STATE_OUT_OF_MEMORY,
            SEARCH_STATE_INVALID
        };

        /** Virtual destructor. */
        ~IPathfinder2() override;

        /** */
        virtual void setGoal( const Vector2I &start, const Vector2I &goal ) = 0;

        /** */
        virtual u32 searchStep() = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IPathfinder2_h__
