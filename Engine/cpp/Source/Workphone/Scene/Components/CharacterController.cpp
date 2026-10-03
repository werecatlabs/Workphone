#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CharacterController.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>

namespace workphone
{
    namespace scene
    {
        WP_CLASS_REGISTER_DERIVED( workphone::scene, CharacterController, Component );

        CharacterController::CharacterController() = default;
        CharacterController::~CharacterController() = default;

        SmartPtr<physics::IPhysicsScene3> CharacterController::getScene() const
        {
            return m_scene;
        }

        void CharacterController::setScene( SmartPtr<physics::IPhysicsScene3> scene )
        {
            m_scene = scene;
        }

        void CharacterController::setTransform( const Transform3<real_Num> &transform )
        {
            m_transform = transform;
        }
        Transform3<real_Num> CharacterController::getTransform() const
        {
            return m_transform;
        }

        void CharacterController::setActorFlag( physics::ActorFlagEnum flag, bool value )
        {
            //if( value )
            //    m_actorFlags = static_cast<physics::ActorFlagEnum>( m_actorFlags | flag );
            //else
            //    m_actorFlags = static_cast<physics::ActorFlagEnum>( m_actorFlags & ~flag );
        }
        physics::ActorFlagEnum CharacterController::getActorFlags() const
        {
            return m_actorFlags;
        }

        real_Num CharacterController::getMass() const
        {
            return m_mass;
        }
        void CharacterController::setMass( real_Num mass )
        {
            m_mass = mass;
        }

        void CharacterController::setCollisionType( u32 type )
        {
            m_collisionType = type;
        }
        u32 CharacterController::getCollisionType() const
        {
            return m_collisionType;
        }

        void CharacterController::setCollisionMask( u32 mask )
        {
            m_collisionMask = mask;
        }
        u32 CharacterController::getCollisionMask() const
        {
            return m_collisionMask;
        }

        void *CharacterController::getUserDataById( u32 id ) const
        {
            auto it = m_userDataById.find( id );
            return it != m_userDataById.end() ? it->second : nullptr;
        }
        void CharacterController::setUserDataById( u32 id, void *userData )
        {
            m_userDataById[id] = userData;
        }

        void *CharacterController::getUserData() const
        {
            return m_userData;
        }
        void CharacterController::setUserData( void *userData )
        {
            m_userData = userData;
        }

        bool CharacterController::getKinematicMode() const
        {
            return m_kinematicMode;
        }
        void CharacterController::setKinematicMode( bool kinematicMode )
        {
            m_kinematicMode = kinematicMode;
        }

        void CharacterController::wakeUp()
        { /* Implementation depends on physics engine */
        }

        void CharacterController::setPosition( const Vector3F &position )
        {
            m_position = position;
        }
        Vector3F CharacterController::getPosition() const
        {
            return m_position;
        }

        void CharacterController::setOrientation( const Quaternion<real_Num> &orientation )
        {
            m_orientation = orientation;
        }
        Quaternion<real_Num> CharacterController::getOrientation() const
        {
            return m_orientation;
        }

        f32 CharacterController::getMoveSpeed() const
        {
            return m_moveSpeed;
        }
        void CharacterController::setMoveSpeed( f32 moveSpeed )
        {
            m_moveSpeed = moveSpeed;
        }

        bool CharacterController::getJump() const
        {
            return m_jump;
        }
        void CharacterController::setJump( bool jump )
        {
            m_jump = jump;
        }
        bool CharacterController::isGrounded() const
        {
            return m_grounded;
        }

        void CharacterController::setWalkVector( const Vector3<real_Num> &vector )
        {
            m_walkVector = vector;
        }
        void CharacterController::stop()
        {
            m_walkVector = Vector3<real_Num>();
        }
    }  // namespace scene
}  // namespace workphone
