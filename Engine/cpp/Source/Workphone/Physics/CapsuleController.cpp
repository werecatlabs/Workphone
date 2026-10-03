#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/CapsuleController.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone
{
    namespace physics
    {
        WP_CLASS_REGISTER_DERIVED( workphone::physics, CapsuleController, ICharacterController3 );

        CapsuleController::CapsuleController() :
            m_scene( nullptr ),
            m_stateContext( nullptr ),
            m_transform( Transform3<real_Num>::identity() ),
            m_position( Vector3<real_Num>::zero() ),
            m_orientation( Quaternion<real_Num>::identity() ),
            m_walkVector( Vector3<real_Num>::zero() ),
            m_mass( static_cast<real_Num>( 80.0 ) ),  // Default character mass
            m_moveSpeed( 5.0f ),
            m_collisionType( 0 ),
            m_collisionMask( 0xFFFFFFFF ),
            m_actorFlags( static_cast<ActorFlagEnum>( 0 ) ),
            m_userData( nullptr ),
            m_isEnabled( true ),
            m_isKinematic( false ),
            m_isJumping( false ),
            m_isGrounded( false ),
            m_jumpVelocity( 10.0f ),
            m_gravity( static_cast<real_Num>( -9.81 ) ),
            m_verticalVelocity( static_cast<real_Num>( 0.0 ) )
        {
            // Initialize user data array
            for( u32 i = 0; i < MAX_USER_DATA_SLOTS; ++i )
            {
                m_userDataById[i] = nullptr;
            }
        }

        CapsuleController::~CapsuleController() = default;

        void CapsuleController::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                // Initialize the character controller here
                // This would typically create the underlying physics representation

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CapsuleController::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &loadingState = getLoadingState();
                if( loadingState != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    // Clean up the character controller
                    m_scene = nullptr;
                    m_stateContext = nullptr;
                    m_userData = nullptr;

                    for( u32 i = 0; i < MAX_USER_DATA_SLOTS; ++i )
                    {
                        m_userDataById[i] = nullptr;
                    }

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<IPhysicsScene3> CapsuleController::getScene() const
        {
            return m_scene;
        }

        void CapsuleController::setScene( SmartPtr<IPhysicsScene3> scene )
        {
            m_scene = scene;
        }

        void CapsuleController::setTransform( const Transform3<real_Num> &transform )
        {
            m_transform = transform;
            m_position = transform.getPosition();
            m_orientation = transform.getOrientation();
        }

        Transform3<real_Num> CapsuleController::getTransform() const
        {
            return Transform3<real_Num>( m_position, m_orientation );
        }

        void CapsuleController::setActorFlag( ActorFlagEnum flag, bool value )
        {
            auto iFlags = (u32)m_actorFlags;
            auto iFlag = (u32)flag;

            if( value )
            {
                m_actorFlags = static_cast<ActorFlagEnum>( iFlags | iFlag );
            }
            else
            {
                m_actorFlags = static_cast<ActorFlagEnum>( iFlags & ~iFlag );
            }
        }

        ActorFlagEnum CapsuleController::getActorFlags() const
        {
            return m_actorFlags;
        }

        real_Num CapsuleController::getMass() const
        {
            return m_mass;
        }

        void CapsuleController::setMass( real_Num mass )
        {
            m_mass =
                Math<real_Num>::max( mass, static_cast<real_Num>( 0.001 ) );  // Ensure positive mass
        }

        void CapsuleController::setCollisionType( u32 type )
        {
            m_collisionType = type;
        }

        u32 CapsuleController::getCollisionType() const
        {
            return m_collisionType;
        }

        void CapsuleController::setCollisionMask( u32 mask )
        {
            m_collisionMask = mask;
        }

        u32 CapsuleController::getCollisionMask() const
        {
            return m_collisionMask;
        }

        void CapsuleController::setEnabled( bool enabled )
        {
            m_isEnabled = enabled;
        }

        bool CapsuleController::isEnabled() const
        {
            return m_isEnabled;
        }

        void *CapsuleController::getUserDataById( u32 id ) const
        {
            if( id < MAX_USER_DATA_SLOTS )
            {
                return m_userDataById[id];
            }
            return nullptr;
        }

        void CapsuleController::setUserDataById( u32 id, void *userData )
        {
            if( id < MAX_USER_DATA_SLOTS )
            {
                m_userDataById[id] = userData;
            }
        }

        void *CapsuleController::getUserData() const
        {
            return m_userData;
        }

        void CapsuleController::setUserData( void *userData )
        {
            m_userData = userData;
        }

        bool CapsuleController::getKinematicMode() const
        {
            return m_isKinematic;
        }

        void CapsuleController::setKinematicMode( bool kinematicMode )
        {
            m_isKinematic = kinematicMode;
        }

        SmartPtr<IPhysicsBody3> CapsuleController::clone()
        {
            auto clone = workphone::make_ptr<CapsuleController>();

            clone->setScene( m_scene );
            clone->setTransform( m_transform );
            clone->setMass( m_mass );
            clone->setMoveSpeed( m_moveSpeed );
            clone->setCollisionType( m_collisionType );
            clone->setCollisionMask( m_collisionMask );
            clone->setEnabled( m_isEnabled );
            clone->setKinematicMode( m_isKinematic );

            return clone;
        }

        void CapsuleController::wakeUp()
        {
            // Wake up the character controller
            // This would typically wake up the underlying physics body
        }

        SmartPtr<IStateContext> CapsuleController::getStateContext() const
        {
            return m_stateContext;
        }

        void CapsuleController::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        void CapsuleController::setPosition( const Vector3<real_Num> &position )
        {
            m_position = Vector3<physics_Num>( position.X(), position.Y(), position.Z() );
            m_transform.setPosition( m_position );
        }

        Vector3<real_Num> CapsuleController::getPosition() const
        {
            return Vector3<physics_Num>( static_cast<physics_Num>( m_position.X() ),
                                         static_cast<physics_Num>( m_position.Y() ),
                                         static_cast<physics_Num>( m_position.Z() ) );
        }

        void CapsuleController::setOrientation( const Quaternion<real_Num> &orientation )
        {
            m_orientation = orientation;
            m_transform.setOrientation( m_orientation );
        }

        Quaternion<real_Num> CapsuleController::getOrientation() const
        {
            return m_orientation;
        }

        f32 CapsuleController::getMoveSpeed() const
        {
            return m_moveSpeed;
        }

        void CapsuleController::setMoveSpeed( f32 moveSpeed )
        {
            m_moveSpeed = Math<f32>::max( moveSpeed, 0.0f );  // Ensure non-negative speed
        }

        bool CapsuleController::getJump() const
        {
            return m_isJumping;
        }

        void CapsuleController::setJump( bool jump )
        {
            if( jump && m_isGrounded && !m_isJumping )
            {
                m_isJumping = true;
                m_verticalVelocity = m_jumpVelocity;
                m_isGrounded = false;
            }
            else if( !jump )
            {
                m_isJumping = false;
            }
        }

        bool CapsuleController::isGrounded() const
        {
            return m_isGrounded;
        }

        void CapsuleController::_getObject( void **ppObject )
        {
            *ppObject = this;
        }

        void CapsuleController::_getObject( void **object ) const
        {
            *object = const_cast<CapsuleController *>( this );
        }

        void CapsuleController::setWalkVector( const Vector3<real_Num> &vector )
        {
            m_walkVector = vector;

            // Normalize the walk vector if it's not zero
            if( m_walkVector.lengthSquared() > Math<real_Num>::epsilon() )
            {
                m_walkVector = m_walkVector.normaliseCopy();
            }
        }

        void CapsuleController::stop()
        {
            m_walkVector = Vector3<real_Num>::zero();
            m_verticalVelocity = static_cast<real_Num>( 0.0 );
            m_isJumping = false;
        }

        void CapsuleController::update( real_Num deltaTime )
        {
            if( !m_isEnabled )
            {
                return;
            }

            // Apply gravity if not grounded
            if( !m_isGrounded )
            {
                m_verticalVelocity += m_gravity * deltaTime;
            }

            // Calculate movement
            Vector3<real_Num> movement = m_walkVector * static_cast<real_Num>( m_moveSpeed ) * deltaTime;
            movement.Y() += m_verticalVelocity * deltaTime;

            // Update position
            m_position += movement;

            // Simple ground check (would need proper physics raycast in real implementation)
            if( m_position.Y() <= static_cast<real_Num>( 0.0 ) &&
                m_verticalVelocity <= static_cast<real_Num>( 0.0 ) )
            {
                m_position.Y() = static_cast<real_Num>( 0.0 );
                m_verticalVelocity = static_cast<real_Num>( 0.0 );
                m_isGrounded = true;
                m_isJumping = false;
            }
            else
            {
                m_isGrounded = false;
            }

            // Update transform
            m_transform.setPosition( m_position );
        }

        void CapsuleController::setRadius( real_Num radius )
        {
            m_radius = Math<real_Num>::max( radius, static_cast<real_Num>( 0.001 ) );
        }

        real_Num CapsuleController::getRadius() const
        {
            return m_radius;
        }

        void CapsuleController::setHeight( real_Num height )
        {
            m_height = Math<real_Num>::max( height, static_cast<real_Num>( 0.001 ) );
        }

        real_Num CapsuleController::getHeight() const
        {
            return m_height;
        }

        void CapsuleController::setJumpVelocity( real_Num jumpVelocity )
        {
            m_jumpVelocity = Math<real_Num>::max( jumpVelocity, static_cast<real_Num>( 0.0 ) );
        }

        real_Num CapsuleController::getJumpVelocity() const
        {
            return m_jumpVelocity;
        }

        void CapsuleController::setGravity( real_Num gravity )
        {
            m_gravity = gravity;
        }

        real_Num CapsuleController::getGravity() const
        {
            return m_gravity;
        }

        Vector3<real_Num> CapsuleController::getWalkVector() const
        {
            return m_walkVector;
        }

        real_Num CapsuleController::getVerticalVelocity() const
        {
            return m_verticalVelocity;
        }

        void CapsuleController::setVerticalVelocity( real_Num velocity )
        {
            m_verticalVelocity = velocity;
        }

    }  // namespace physics
}  // namespace workphone
