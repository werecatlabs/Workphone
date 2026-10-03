#ifndef IAiTargeting3_h__
#define IAiTargeting3_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /**
     * @brief Interface for an AI targeting system. Inherits from ISharedObject.
     */
    class WPCore_API IAiTargeting3 : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IAiTargeting3() override;

        virtual Vector3<real_Num> getTargetPosition() const = 0;
        virtual void setTargetPosition( const Vector3<real_Num> &targetPosition ) = 0;

        virtual SmartPtr<scene::IGameActor> getOwner() const = 0;
        virtual void setOwner( SmartPtr<scene::IGameActor> owner ) = 0;

        virtual SmartPtr<scene::IGameActor> getTarget() const = 0;
        virtual void setTarget( SmartPtr<scene::IGameActor> target ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IAiTargeting3_h__
