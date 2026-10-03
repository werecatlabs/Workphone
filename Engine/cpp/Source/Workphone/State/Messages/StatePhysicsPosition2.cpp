#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StatePhysicsPosition2.hpp"
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StatePhysicsPosition2, StateMessage );
    const hash_type StatePhysicsPosition2::TYPE = StringUtil::getHash( "StatePhysicsPosition2" );

    StatePhysicsPosition2::StatePhysicsPosition2( const Vector2<real_Num> &position ) :
        m_position( position ),
        m_subjectId( 0 )
    {
    }

    StatePhysicsPosition2::StatePhysicsPosition2( u32 subjectId, const Vector2<real_Num> &position ) :
        m_position( position ),
        m_subjectId( subjectId )
    {
    }

    StatePhysicsPosition2::StatePhysicsPosition2( u32 subjectId ) : m_subjectId( subjectId )
    {
    }

    StatePhysicsPosition2::StatePhysicsPosition2() = default;
    StatePhysicsPosition2::~StatePhysicsPosition2() = default;

    auto StatePhysicsPosition2::getPosition() const -> Vector2<real_Num>
    {
        return m_position;
    }

    void StatePhysicsPosition2::setPosition( const Vector2<real_Num> &value )
    {
        m_position = value;
    }

    auto StatePhysicsPosition2::getSubjectId() const -> u32
    {
        return m_subjectId;
    }

    void StatePhysicsPosition2::setSubjectId( u32 value )
    {
        m_subjectId = value;
    }
}  // namespace workphone
