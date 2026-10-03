#ifndef StatePhysicsVelocity2_h__
#define StatePhysicsVelocity2_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    class WPCore_API StatePhysicsVelocity2 : public StateMessage
    {
    public:
        /// Some commonly used types.
        static const hash_type ADD_VELOCITY_HASH;
        static const hash_type SET_VELOCITY_HASH;

        StatePhysicsVelocity2();
        explicit StatePhysicsVelocity2( const Vector2<real_Num> &velocity );
        ~StatePhysicsVelocity2() override;

        Vector2<real_Num> getVelocity() const;
        void setVelocity( const Vector2<real_Num> &velocity );

        Vector2<real_Num> getRelativePosition() const;
        void setRelativePosition( const Vector2<real_Num> &relativePosition );

        WP_CLASS_REGISTER_DECL;

    protected:
        Vector2<real_Num> m_velocity;
        Vector2<real_Num> m_relativePosition;
    };
}  // namespace workphone

#endif  // StatePhysicsVelocity2_h__
