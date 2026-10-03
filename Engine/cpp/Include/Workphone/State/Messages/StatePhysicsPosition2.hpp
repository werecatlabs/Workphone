#ifndef StatePhysicsPosition_h__
#define StatePhysicsPosition_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include "Workphone/Math/Vector2.hpp"

namespace workphone
{

    class WPCore_API StatePhysicsPosition2 : public StateMessage
    {
    public:
        StatePhysicsPosition2();
        explicit StatePhysicsPosition2( u32 subjectId );
        ~StatePhysicsPosition2() override;

        StatePhysicsPosition2( u32 subjectId, const Vector2<real_Num> &position );

        explicit StatePhysicsPosition2( const Vector2<real_Num> &position );

        Vector2<real_Num> getPosition() const;
        void setPosition( const Vector2<real_Num> &value );

        u32 getSubjectId() const;
        void setSubjectId( u32 value );

        static const hash_type TYPE;

        WP_CLASS_REGISTER_DECL;

    protected:
        Vector2<real_Num> m_position;
        u32 m_subjectId;
    };

}  // namespace workphone

#endif  // StatePhysicsPosition_h__
