#ifndef IPathNode2_h__
#define IPathNode2_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    /** Interface for a 2d path node.
     */
    class WPCore_API IPathNode2 : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IPathNode2() override;

        virtual Vector2I getPosition() const = 0;
        virtual void setPosition( const Vector2I &position ) = 0;

        virtual f32 getCost( SmartPtr<IPathNode2> successor ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IPathNode2_h__
