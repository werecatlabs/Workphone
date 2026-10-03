#ifndef StatePhysicsForce2_h__
#define StatePhysicsForce2_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    class WPCore_API StatePhysicsForce2 : public StateMessage
    {
    public:
        /// Some commonly used types.
        static const hash_type ADD_FORCE_HASH;
        static const hash_type SET_FORCE_HASH;

        StatePhysicsForce2();

        Vector2<real_Num> getForce() const;
        void setForce( const Vector2<real_Num> &value );

        Vector2<real_Num> getRelativePosition() const;
        void setRelativePosition( const Vector2<real_Num> &value );

        WP_CLASS_REGISTER_DECL;

    protected:
        Vector2<real_Num> m_force;
        Vector2<real_Num> m_relativePosition;
    };
}  // namespace workphone

#endif  // StatePhysicsForce2_h__
