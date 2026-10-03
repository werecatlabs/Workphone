#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxCharacterController.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include <WPPhysx/PhysxUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <characterkinematic/PxControllerManager.h>
#include <characterkinematic/PxBoxController.h>
#include <characterkinematic/PxCapsuleController.h>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxCharacterController, CapsuleController );

    PhysxCharacterController::PhysxCharacterController() : m_kinematicMode( false )
    {
    }

    PhysxCharacterController::~PhysxCharacterController() = default;

    void PhysxCharacterController::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto timer = applicationManager->getTimer();

        auto task = Thread::getCurrentTask();
        auto t = timer->getTime();
        auto dt = timer->getDeltaTime();

        physx::PxVec3      moveVec( m_moveVector.X(), m_moveVector.Y(), m_moveVector.Z() );
        const physx::PxU32 moveFlags =
            m_controller->move( moveVec, 0.01f, static_cast<f32>( dt ), physx::PxControllerFilters() );

        physx::PxVec3      gravityVec( 0.0f, -10.0f * static_cast<f32>( dt ), 0.0f );
        const physx::PxU32 gravityFlags = m_controller->move( gravityVec, 0.01f, static_cast<f32>( dt ),
                                                              physx::PxControllerFilters() );

        m_controller->getState( m_controllerState );
    }

    void PhysxCharacterController::load( SmartPtr<ISharedObject> data )
    {
        auto                   engine = core::IApplicationManager::instance();
        auto                   pPhysicsManager = engine->getPhysicsManager();
        SmartPtr<PhysxManager> physicsManager =
            workphone::static_pointer_cast<PhysxManager>( pPhysicsManager );

        using namespace physx;

        auto physics = physicsManager->getPhysics();

        // mControllerManager =
        //  PxCreateControllerManager(physicsManager->getPhysics()->getFoundation());

        //// use ray casting to position the CCT on the terrain
        // PxScene& scene = *physicsManager->getScene();
        // PxRaycastHit hit;
        // scene.raycastSingle(PxVec3(0,100,0), PxVec3(0,-1,0), 500.0f,
        // PxSceneQueryFlags(0xffffffff), hit);
        ////mControllerInitialPosition = hit.impact + PxVec3(0.0f, 1.8f, 0.0f);

        //// create and fill in the descriptor for the capsule controller
        // physx::PxCapsuleControllerDesc cDesc;

        // cDesc.height				= 2.0f;
        // cDesc.radius				= 0.5f;
        // cDesc.material				= physicsManager->getDefaultMaterial();
        // cDesc.position				= PxExtendedVec3(mControllerInitialPosition.x,
        // mControllerInitialPosition.y, mControllerInitialPosition.z); cDesc.slopeLimit			=
        // 0.1f; cDesc.contactOffset			= 0.01f; cDesc.stepOffset			= 0.1f;
        // cDesc.invisibleWallHeight	= 0.0f;
        // cDesc.maxJumpHeight			= 2.0f;
        // cDesc.scaleCoeff            = 0.9f;
        // cDesc.climbingMode          = PxCapsuleClimbingMode::eEASY;
        ////cDesc.callback              = this;

        // mController = static_cast<PxCapsuleController*>(
        //	mControllerManager->createController(*physicsManager->getPhysics(),
        // physicsManager->getScene(), cDesc));

        //
    }

    void PhysxCharacterController::unload( SmartPtr<ISharedObject> data )
    {
    }

    void PhysxCharacterController::setPosition( const Vector3F &position )
    {
        m_controller->setPosition( physx::PxExtendedVec3( position.X(), position.Y(), position.Z() ) );
    }

    auto PhysxCharacterController::getPosition() const -> Vector3F
    {
        auto vec = m_controller->getPosition();
        return { static_cast<physics_Num>( vec.x ), static_cast<physics_Num>( vec.y ),
                 static_cast<physics_Num>( vec.z ) };
    }

    void PhysxCharacterController::setOrientation( const QuaternionF &orientation )
    {
        auto up = orientation * Vector3F::up();
        auto pxUP = PhysxUtil::toPx( up );
        m_controller->setUpDirection( pxUP );
    }

    auto PhysxCharacterController::getOrientation() const -> QuaternionF
    {
        return QuaternionF::identity();
    }

    auto PhysxCharacterController::getMoveSpeed() const -> f32
    {
        return 0.f;
    }

    void PhysxCharacterController::setMoveSpeed( f32 moveSpeed )
    {
        m_controller->setStepOffset( moveSpeed );
    }

    auto PhysxCharacterController::getJump() const -> bool
    {
        return false;
    }

    void PhysxCharacterController::setJump( bool jump )
    {
    }

    auto PhysxCharacterController::isGrounded() const -> bool
    {
        return true;
    }

    void PhysxCharacterController::_getObject( void **ppObject )
    {
    }

    void PhysxCharacterController::setWalkVector( const Vector3F &vector )
    {
        m_moveVector = vector;
    }

    void PhysxCharacterController::stop()
    {
        m_moveVector = Vector3F::zero();
    }

    auto PhysxCharacterController::getMass() const -> physics_Num
    {
        return 0.f;
    }

    void PhysxCharacterController::setMass( physics_Num mass )
    {
    }

    void PhysxCharacterController::setCollisionType( u32 type )
    {
    }

    auto PhysxCharacterController::getCollisionType() const -> u32
    {
        return 0;
    }

    void PhysxCharacterController::setCollisionMask( u32 mask )
    {
    }

    auto PhysxCharacterController::getCollisionMask() const -> u32
    {
        return 0;
    }

    void PhysxCharacterController::setEnabled( bool enabled )
    {
    }

    auto PhysxCharacterController::isEnabled() const -> bool
    {
        return true;
    }

    auto PhysxCharacterController::getUserData() const -> void *
    {
        return nullptr;
    }

    void PhysxCharacterController::setUserData( void *userData )
    {
    }

    auto PhysxCharacterController::getKinematicMode() const -> bool
    {
        return m_kinematicMode;
    }

    void PhysxCharacterController::setKinematicMode( bool kinematicMode )
    {
        m_kinematicMode = kinematicMode;
    }

    void PhysxCharacterController::setPxScene( physx::PxScene *scene )
    {
        m_scene = scene;
    }

    physx::PxScene *PhysxCharacterController::getPxScene() const
    {
        return m_scene;
    }

} // namespace workphone::physics
