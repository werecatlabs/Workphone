#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/PhysicsBind.hpp"
#include <luabind/luabind.hpp>
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include "WPLuabind/Helpers/PhysicsHelper3.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Physics/INativePhysicsObject2.hpp>

namespace workphone
{
    // PhysicsShape2Ptr _createCollisionShape( IPhysicsManager2 *mgr, const char *pShapeName )
    //{
    //     String shapeName = pShapeName;
    //     if( shapeName == ( "sphere" ) )
    //     {
    //         return mgr->createCollisionShape( physics::PST_SPHERE );
    //     }
    //     else if( shapeName == ( "box" ) )
    //     {
    //         return mgr->createCollisionShape( physics::PST_BOX );
    //     }

    //    return nullptr;
    //}

    // SphereShape2Ptr _createSphere( IPhysicsManager2 *mgr )
    //{
    //     return nullptr;  // mgr->createCollisionShape(PST_SPHERE);
    // }

    // BoxShape2Ptr _createBox( IPhysicsManager2 *mgr )
    //{
    //     return nullptr;  // mgr->createCollisionShape(PST_BOX);
    // }

    // RigidBody2Ptr _createRigidBody( IPhysicsManager2 *mgr )
    //{
    //     return mgr->createRigidBody();
    // }

    // RigidBody2Ptr _createRigidBodyFromShape( IPhysicsManager2 *mgr, PhysicsShape2Ptr shape )
    //{
    //     return mgr->createRigidBody( shape );
    // }

    // RigidBody2Ptr _createRigidBodyFromSphere( IPhysicsManager2 *mgr, SphereShape2Ptr shape )
    //{
    //     return mgr->createRigidBody( shape );
    // }

    // RigidBody2Ptr _createRigidBodyFromBox( IPhysicsManager2 *mgr, BoxShape2Ptr shape )
    //{
    //     return mgr->createRigidBody( shape );
    // }

    // void _setUserData( IPhysicsBody2 *body, IScriptObject *obj )
    //{
    //     body->setUserData( obj );
    // }

    // SmartPtr<IScriptObject> _getUserData( IPhysicsBody2 *body )
    //{
    //     SmartPtr<IScriptObject> pObject( (IScriptObject *)body->getUserData() );
    //     return pObject;
    // }

    // void _setRigidBody( PhysicsContainer2 *container, lua_Integer hash, RigidBody2Ptr rigidBody )
    //{
    //     hash32 id = *reinterpret_cast<hash32 *>( &hash );
    //     container->setObject( id, rigidBody );
    // }

    // void _addBody( PhysicsResponse2 *physicsResponse, IScriptObject *obj )
    //{
    //     //physicsResponse->addBody(obj);
    // }

    // RigidBody2Ptr _getRigidBody( PhysicsContainer2 *container, const char *name )
    //{
    //     auto hash = StringUtil::getHash( name );
    //     return container->getRigidBody( hash );
    // }

    // RigidBody2Ptr _getRigidBodyHash( PhysicsContainer2 *container, lua_Integer hash )
    //{
    //     hash32 id = *reinterpret_cast<hash32 *>( &hash );
    //     return container->getRigidBody( id );
    // }

    // void _setShape( PhysicsContainer2 *container, lua_Integer hash, PhysicsShape2Ptr shape )
    //{
    //     hash32 id = *reinterpret_cast<hash32 *>( &hash );
    //     container->setShape( id, shape );
    // }

    // PhysicsShape2Ptr _getShape( PhysicsContainer2 *container, lua_Integer hash )
    //{
    //     hash32 id = *reinterpret_cast<hash32 *>( &hash );
    //     return container->getShape( id );
    // }

    // void _addVelocity( IPhysicsBody2 *body, const Vector2<real_Num> &vel )
    //{
    //     body->addVelocity( vel );
    // }

    // StateObjectPtr _getStateContext( IPhysicsBody2 *body )
    //{
    //     return nullptr;  // body->getStateContext().get();
    // }

    // void _setBodyPosition( IPhysicsBody2 *body, const Vector2F &position )
    //{
    //     body->setPosition( Vector2<real_Num>( position.X(), position.Y() ) );
    // }

    // void _addBodyForce( IPhysicsBody2 *body, const Vector2F &force )
    //{
    //     body->addForce( Vector2<real_Num>( force.X(), force.Y() ) );
    // }

    // void _setBodyForce( IPhysicsBody2 *body, const Vector2F &force )
    //{
    //     body->setForce( Vector2<real_Num>( force.X(), force.Y() ) );
    // }

    template <class T>
    void _setCollisionType( T *body, lua_Integer mask )
    {
        body->setCollisionType( static_cast<u32>( mask ) );
    }

    template <class T>
    lua_Integer _getCollisionType( T *body )
    {
        return static_cast<lua_Integer>( body->getCollisionType() );
    }

    template <class T>
    void _setCollisionMask( T *body, lua_Integer mask )
    {
        body->setCollisionMask( static_cast<u32>( mask ) );
    }

    template <class T>
    lua_Integer _getCollisionMask( T *body )
    {
        return static_cast<lua_Integer>( body->getCollisionMask() );
    }

    template <class T>
    void _setActorFlag( T *body, lua_Integer flag, bool value )
    {
        body->setActorFlag( static_cast<physics::ActorFlagEnum>( flag ), value );
    }

    template <class T>
    lua_Integer _getActorFlags( T *body )
    {
        return static_cast<lua_Integer>( body->getActorFlags() );
    }

    void _setRigidBodyFlag( physics::IRigidBody3 *body, lua_Integer flag, bool value )
    {
        body->setRigidBodyFlag( static_cast<physics::RigidBodyFlagEnum>( flag ), value );
    }

    lua_Integer _getRigidBodyFlags( const physics::IRigidBody3 *body )
    {
        return static_cast<lua_Integer>( body->getRigidBodyFlags() );
    }

    void _clearForce( physics::IRigidBody3 *body, lua_Integer mode )
    {
        body->clearForce( static_cast<physics::ForceModeEnum>( mode ) );
    }

    void _clearTorque( physics::IRigidBody3 *body, lua_Integer mode )
    {
        body->clearTorque( static_cast<physics::ForceModeEnum>( mode ) );
    }

    void _setConstraintFlag( physics::IPhysicsConstraint3 *constraint, lua_Integer flag, bool value )
    {
        constraint->setConstraintFlag( static_cast<physics::ConstraintFlagEnum>( flag ), value );
    }

    lua_Integer _getConstraintFlags( const physics::IPhysicsConstraint3 *constraint )
    {
        return static_cast<lua_Integer>( constraint->getConstraintFlags() );
    }

    void _setDriveFlags( physics::IConstraintDrive *drive, lua_Integer flags )
    {
        drive->setDriveFlags( static_cast<physics::D6JointDriveFlagEnum>( flags ) );
    }

    lua_Integer _getDriveFlags( const physics::IConstraintDrive *drive )
    {
        return static_cast<lua_Integer>( drive->getDriveFlags() );
    }

    void _setDrive( physics::IConstraintD6 *constraint, lua_Integer index,
                    SmartPtr<physics::IConstraintDrive> drive )
    {
        constraint->setDrive( static_cast<physics::D6DriveEnum>( index ), drive );
    }

    SmartPtr<physics::IConstraintDrive> _getDrive( const physics::IConstraintD6 *constraint,
                                                   lua_Integer                   index )
    {
        return constraint->getDrive( static_cast<physics::D6DriveEnum>( index ) );
    }

    void _setMotion( physics::IConstraintD6 *constraint, lua_Integer axis, lua_Integer motion )
    {
        constraint->setMotion( static_cast<physics::D6AxisEnum>( axis ),
                               static_cast<physics::D6MotionEnum>( motion ) );
    }

    lua_Integer _getMotion( const physics::IConstraintD6 *constraint, lua_Integer axis )
    {
        return static_cast<lua_Integer>(
            constraint->getMotion( static_cast<physics::D6AxisEnum>( axis ) ) );
    }

    lua_Integer _getStateTask( const physics::IPhysicsManager *manager )
    {
        return static_cast<lua_Integer>( manager->getStateTask() );
    }

    lua_Integer _getPhysicsTask( const physics::IPhysicsManager *manager )
    {
        return static_cast<lua_Integer>( manager->getPhysicsTask() );
    }

    // void _setObjectType( IPhysicsBody2 *body, lua_Integer hash )
    //{
    //     body->setObjectType( *reinterpret_cast<hash32 *>( &hash ) );
    // }

    // lua_Integer _getObjectType( IPhysicsBody2 *body )
    //{
    //     hash32 hash = body->getObjectType();
    //     return *reinterpret_cast<lua_Integer *>( &hash );
    // }

    // PhysicsTerrainPtr _createTerrain( IPhysicsManager3 *physicsMgr, TerrainTemplatePtr objectTemplate
    // )
    //{
    //     return physicsMgr->createTerrain( objectTemplate );
    // }

    void bindPhysics( lua_State *L )
    {
        using namespace luabind;
        using namespace workphone;
        using namespace physics;

        module( L )[class_<IPhysicsShape, ISharedObject, SmartPtr<IPhysicsShape>>( "IPhysicsShape" )
                        .def( "isAttached", &IPhysicsShape::isAttached )
                        .def( "setEnabled", &IPhysicsShape::setEnabled )
                        .def( "isEnabled", &IPhysicsShape::isEnabled )
                        .def( "setTrigger", &IPhysicsShape::setTrigger )
                        .def( "isTrigger", &IPhysicsShape::isTrigger )
                        .def( "setStateContext", &IPhysicsShape::setStateContext )
                        .def( "getStateContext", &IPhysicsShape::getStateContext )
                        .def( "setStateListener", &IPhysicsShape::setStateListener )
                        .def( "getStateListener", &IPhysicsShape::getStateListener )
                        .def( "setCollisionType", &IPhysicsShape::setCollisionType )
                        .def( "getCollisionType", &IPhysicsShape::getCollisionType )
                        .def( "setCollisionMask", &IPhysicsShape::setCollisionMask )
                        .def( "getCollisionMask", &IPhysicsShape::getCollisionMask )
                        .def( "handleStateChanged",
                              static_cast<bool ( IPhysicsShape::* )( const SmartPtr<IStateMessage> & )>(
                                  &IPhysicsShape::handleStateChanged ) )
                        .def( "handleStateChanged",
                              static_cast<bool ( IPhysicsShape::* )( SmartPtr<IState> & )>(
                                  &IPhysicsShape::handleStateChanged ) )
                        .scope[def( "typeInfo", IPhysicsShape::typeInfo )]];

        module( L )[class_<IPhysicsShape2, IPhysicsShape, SmartPtr<IPhysicsShape2>>( "IPhysicsShape2" )
                        .def( "getSphere", &IPhysicsShape2::getSphere )
                        .def( "getAABB", &IPhysicsShape2::getAABB )
                        .def( "getPoints", &IPhysicsShape2::getPoints )
                        .def( "computeMass", &IPhysicsShape2::computeMass )
                        .def( "getType", &IPhysicsShape2::getType )
                        .scope[def( "typeInfo", IPhysicsShape2::typeInfo )]];

        module( L )[class_<IBoxShape2, IPhysicsShape2, SmartPtr<IBoxShape2>>( "IBoxShape2" )
                        .def( "setAABB", &IBoxShape2::setAABB )
                        .scope[def( "typeInfo", IBoxShape2::typeInfo )]];
        module( L )[class_<ISphereShape2, IPhysicsShape2, SmartPtr<ISphereShape2>>( "ISphereShape2" )
                        .def( "setRadius", &ISphereShape2::setRadius )
                        .def( "getRadius", &ISphereShape2::getRadius )
                        .scope[def( "typeInfo", ISphereShape2::typeInfo )]];

        module( L )[class_<FilterData>( "FilterData" )
                        .def( constructor<>() )
                        .def( constructor<u32, u32, u32, u32>() )
                        .def( "setToDefault", &FilterData::setToDefault )
                        .def_readwrite( "word0", &FilterData::word0 )
                        .def_readwrite( "word1", &FilterData::word1 )
                        .def_readwrite( "word2", &FilterData::word2 )
                        .def_readwrite( "word3", &FilterData::word3 )];

        module( L )[class_<IPhysicsShape3, IPhysicsShape, SmartPtr<IPhysicsShape3>>( "IPhysicsShape3" )
                        .def( "getMaterial", &IPhysicsShape3::getMaterial )
                        .def( "setMaterial", &IPhysicsShape3::setMaterial )
                        .def( "setLocalPose", &IPhysicsShape3::setLocalPose )
                        .def( "getLocalPose", &IPhysicsShape3::getLocalPose )
                        .def( "setSimulationFilterData", &IPhysicsShape3::setSimulationFilterData )
                        .def( "getSimulationFilterData", &IPhysicsShape3::getSimulationFilterData )
                        .def( "setActor", &IPhysicsShape3::setActor )
                        .def( "getActor", &IPhysicsShape3::getActor )
                        .def( "hasShapeData", &IPhysicsShape3::hasShapeData )
                        .def( "clone", &IPhysicsShape3::clone )
                        .scope[def( "typeInfo", IPhysicsShape3::typeInfo )]];

        module( L )[class_<IBoxShape3, IPhysicsShape3, SmartPtr<IBoxShape3>>( "IBoxShape3" )
                        .def( "getExtents", &IBoxShape3::getExtents )
                        .def( "setExtents", &IBoxShape3::setExtents )
                        .def( "getAABB", &IBoxShape3::getAABB )
                        .def( "setAABB", &IBoxShape3::setAABB )
                        .scope[def( "typeInfo", IBoxShape3::typeInfo )]];

        module( L )[class_<IPhysicsBody2D, ISharedObject, SmartPtr<IPhysicsBody2D>>( "IPhysicsBody2D" )
                        .def( "setPosition", &IPhysicsBody2D::setPosition )
                        .def( "getPosition", &IPhysicsBody2D::getPosition )
                        .def( "setTargetPosition", &IPhysicsBody2D::setTargetPosition )
                        .def( "getTargetPosition", &IPhysicsBody2D::getTargetPosition )
                        .def( "setOrientation", &IPhysicsBody2D::setOrientation )
                        .def( "getOrientation", &IPhysicsBody2D::getOrientation )
                        .def( "getAngularVelocity", &IPhysicsBody2D::getAngularVelocity )
                        .def( "addForce", &IPhysicsBody2D::addForce )
                        .def( "setForce", &IPhysicsBody2D::setForce )
                        .def( "getForce", &IPhysicsBody2D::getForce )
                        .def( "addTorque", &IPhysicsBody2D::addTorque )
                        .def( "setTorque", &IPhysicsBody2D::setTorque )
                        .def( "getTorque", &IPhysicsBody2D::getTorque )
                        .def( "addVelocity", &IPhysicsBody2D::addVelocity )
                        .def( "setVelocity", &IPhysicsBody2D::setVelocity )
                        .def( "getVelocity", &IPhysicsBody2D::getVelocity )
                        .def( "setMaxVelocity", &IPhysicsBody2D::setMaxVelocity )
                        .def( "getMaxVelocity", &IPhysicsBody2D::getMaxVelocity )
                        .def( "setLinearDampValue", &IPhysicsBody2D::setLinearDampValue )
                        .def( "getLinearDampValue", &IPhysicsBody2D::getLinearDampValue )
                        .def( "setAngularDampValue", &IPhysicsBody2D::setAngularDampValue )
                        .def( "getAngularDampValue", &IPhysicsBody2D::getAngularDampValue )
                        .def( "setAirResistance", &IPhysicsBody2D::setAirResistance )
                        .def( "getAirResistance", &IPhysicsBody2D::getAirResistance )
                        .def( "setRestitution", &IPhysicsBody2D::setRestitution )
                        .def( "getRestitution", &IPhysicsBody2D::getRestitution )
                        .def( "setMass", &IPhysicsBody2D::setMass )
                        .def( "getMass", &IPhysicsBody2D::getMass )
                        .def( "getMassInv", &IPhysicsBody2D::getMassInv )
                        .def( "setFlag", &IPhysicsBody2D::setFlag )
                        .def( "getFlag", &IPhysicsBody2D::getFlag )
                        .def( "getBodyType", &IPhysicsBody2D::getBodyType )
                        .def( "setObjectType", &IPhysicsBody2D::setObjectType )
                        .def( "getObjectType", &IPhysicsBody2D::getObjectType )
                        .def( "setWorldId", &IPhysicsBody2D::setWorldId )
                        .def( "getWorldId", &IPhysicsBody2D::getWorldId )
                        .def( "setEnabled", &IPhysicsBody2D::setEnabled )
                        .def( "isEnabled", &IPhysicsBody2D::isEnabled )
                        .def( "getLocalAABB", &IPhysicsBody2D::getLocalAABB )
                        .def( "getWorldAABB", &IPhysicsBody2D::getWorldAABB )
                        .def( "setMaterialId", &IPhysicsBody2D::setMaterialId )
                        .def( "getMaterialId", &IPhysicsBody2D::getMaterialId )
                        .def( "setCollisionType", &IPhysicsBody2D::setCollisionType )
                        .def( "getCollisionType", &IPhysicsBody2D::getCollisionType )
                        .def( "setCollisionMask", &IPhysicsBody2D::setCollisionMask )
                        .def( "getCollisionMask", &IPhysicsBody2D::getCollisionMask )
                        .def( "setSleep", &IPhysicsBody2D::setSleep )
                        .def( "isSleeping", &IPhysicsBody2D::isSleeping )
                        .def( "setContraintAABB", &IPhysicsBody2D::setContraintAABB )
                        .def( "getContraintAABB", &IPhysicsBody2D::getContraintAABB )
                        .def( "getKinematicMode", &IPhysicsBody2D::getKinematicMode )
                        .def( "setKinematicMode", &IPhysicsBody2D::setKinematicMode )
                        .def( "getGravity", &IPhysicsBody2D::getGravity )
                        .def( "setGravity", &IPhysicsBody2D::setGravity )
                        .def( "getEnableGravity", &IPhysicsBody2D::getEnableGravity )
                        .def( "setEnableGravity", &IPhysicsBody2D::setEnableGravity )
                        .def( "addEffect", &IPhysicsBody2D::addEffect )
                        .def( "removeEffect", &IPhysicsBody2D::removeEffect )
                        .def( "getConstraints", &IPhysicsBody2D::getConstraints )
                        .def( "removeConstraints", &IPhysicsBody2D::removeConstraints )
                        .def( "removeConstraint", &IPhysicsBody2D::removeConstraint )
                        .def( "addConstraint", &IPhysicsBody2D::addConstraint )
                        .scope[def( "typeInfo", IPhysicsBody2D::typeInfo )]];
        module( L )[class_<IPhysicsMaterial2, ISharedObject, SmartPtr<IPhysicsMaterial2>>(
                        "IPhysicsMaterial2" )
                        .def( "setFriction", &IPhysicsMaterial2::setFriction )
                        .def( "getFriction", &IPhysicsMaterial2::getFriction )
                        .def( "setRestitution", &IPhysicsMaterial2::setRestitution )
                        .def( "getRestitution", &IPhysicsMaterial2::getRestitution )
                        .def( "setContactPosition", &IPhysicsMaterial2::setContactPosition )
                        .def( "getContactPosition", &IPhysicsMaterial2::getContactPosition )
                        .def( "setContactNormal", &IPhysicsMaterial2::setContactNormal )
                        .def( "getContactNormal", &IPhysicsMaterial2::getContactNormal )
                        .def( "setPhysicsBodyA", &IPhysicsMaterial2::setPhysicsBodyA )
                        .def( "getPhysicsBodyA", &IPhysicsMaterial2::getPhysicsBodyA )
                        .def( "setPhysicsBodyB", &IPhysicsMaterial2::setPhysicsBodyB )
                        .def( "getPhysicsBodyB", &IPhysicsMaterial2::getPhysicsBodyB )
                        .scope[def( "typeInfo", IPhysicsMaterial2::typeInfo )]];
        module( L )[class_<IRigidBody2, IPhysicsBody2D, SmartPtr<IRigidBody2>>( "IRigidBody2" )
                        .def( "setCollisionShape", &IRigidBody2::setCollisionShape )
                        .def( "getCollisionShape", &IRigidBody2::getCollisionShape )
                        .scope[def( "typeInfo", IRigidBody2::typeInfo )]];
        module( L )[class_<IPhysicsParticle2, IPhysicsBody2D, SmartPtr<IPhysicsParticle2>>(
                        "IPhysicsParticle2" )
                        .def( "setCollisionShape", &IPhysicsParticle2::setCollisionShape )
                        .def( "getCollisionShape", &IPhysicsParticle2::getCollisionShape )
                        .scope[def( "typeInfo", IPhysicsParticle2::typeInfo )]];

        module( L )[class_<INativePhysicsObject2>( "INativePhysicsObject2" )
                        .def( "getNativeObject", &INativePhysicsObject2::getNativeObject )];

        module( L )[class_<IPhysicsBody3, ISharedObject, SmartPtr<IPhysicsBody3>>( "IPhysicsBody3" )
                        .def( "getScene", &IPhysicsBody3::getScene )
                        .def( "setScene", &IPhysicsBody3::setScene )
                        .def( "setTransform", &IPhysicsBody3::setTransform )
                        .def( "getTransform", &IPhysicsBody3::getTransform )

                        .def( "setActorFlag", _setActorFlag<IPhysicsBody3> )
                        .def( "getActorFlags", _getActorFlags<IPhysicsBody3> )

                        .def( "getMass", &IPhysicsBody3::getMass )
                        .def( "setMass", &IPhysicsBody3::setMass )

                        .def( "setCollisionType", _setCollisionType<IPhysicsBody3> )
                        .def( "getCollisionType", _getCollisionType<IPhysicsBody3> )

                        .def( "setCollisionMask", _setCollisionMask<IPhysicsBody3> )
                        .def( "getCollisionMask", _getCollisionMask<IPhysicsBody3> )

                        .def( "setEnabled", &IPhysicsBody3::setEnabled )
                        .def( "isEnabled", &IPhysicsBody3::isEnabled )

                        .def( "getUserDataById", &IPhysicsBody3::getUserDataById )
                        .def( "setUserDataById", &IPhysicsBody3::setUserDataById )

                        .def( "setKinematicMode", &IPhysicsBody3::setKinematicMode )
                        .def( "getKinematicMode", &IPhysicsBody3::getKinematicMode )

                        .def( "wakeUp", &IPhysicsBody3::wakeUp )

                        .def( "setStateContext", &IPhysicsBody3::setStateContext )
                        .def( "getStateContext", &IPhysicsBody3::getStateContext )
                        .def( "clone", &IPhysicsBody3::clone )
                        .scope[def( "typeInfo", IPhysicsBody3::typeInfo )]];

        module(
            L )[class_<IRigidBody3, IPhysicsBody3, SmartPtr<IRigidBody3>>( "IRigidBody3" )
                    .def( "setRigidBodyFlag", _setRigidBodyFlag )
                    .def( "getRigidBodyFlags", _getRigidBodyFlags )
                    .def( "addShape", &IRigidBody3::addShape )
                    .def( "removeShape", &IRigidBody3::removeShape )
                    .def( "getShapes", &IRigidBody3::getShapes )
                    .def( "getNumShapes", &IRigidBody3::getNumShapes )
                    .def( "setLinearVelocity", &IRigidBody3::setLinearVelocity )
                    .def( "getLinearVelocity", &IRigidBody3::getLinearVelocity )
                    .def( "setAngularVelocity", &IRigidBody3::setAngularVelocity )
                    .def( "getAngularVelocity", &IRigidBody3::getAngularVelocity )
                    .def( "addForce", &IRigidBody3::addForce )
                    .def( "clearForce", _clearForce )
                    .def( "addTorque", &IRigidBody3::addTorque )
                    .def( "clearTorque", _clearTorque )
                    .def( "getLocalAABB", &IRigidBody3::getLocalAABB )
                    .def( "getWorldAABB", &IRigidBody3::getWorldAABB )
                    .def( "setCMassLocalPose", &IRigidBody3::setCMassLocalPose )
                    .def( "getCMassLocalPose", &IRigidBody3::getCMassLocalPose )
                    .def( "setMassSpaceInertiaTensor", &IRigidBody3::setMassSpaceInertiaTensor )
                    .def( "getMassSpaceInertiaTensor", &IRigidBody3::getMassSpaceInertiaTensor )
                    .def( "getMassSpaceInvInertiaTensor", &IRigidBody3::getMassSpaceInvInertiaTensor )
                    .scope[def( "typeInfo", IRigidBody3::typeInfo )]];
        module( L )[class_<IRigidStatic3, IRigidBody3, SmartPtr<IRigidStatic3>>( "IRigidStatic3" )
                        .scope[def( "typeInfo", IRigidStatic3::typeInfo )]];
        module( L )[class_<ICharacterController3, IPhysicsBody3, SmartPtr<ICharacterController3>>(
                        "ICharacterController3" )
                        .def( "setPosition", &ICharacterController3::setPosition )
                        .def( "getPosition", &ICharacterController3::getPosition )
                        .def( "setOrientation", &ICharacterController3::setOrientation )
                        .def( "getOrientation", &ICharacterController3::getOrientation )
                        .def( "getMoveSpeed", &ICharacterController3::getMoveSpeed )
                        .def( "setMoveSpeed", &ICharacterController3::setMoveSpeed )
                        .def( "getJump", &ICharacterController3::getJump )
                        .def( "setJump", &ICharacterController3::setJump )
                        .def( "isGrounded", &ICharacterController3::isGrounded )
                        .def( "setWalkVector", &ICharacterController3::setWalkVector )
                        .def( "stop", &ICharacterController3::stop )
                        .scope[def( "typeInfo", ICharacterController3::typeInfo )]];

        // module( L )[class_<IPhysicsMaterial2, IScriptObject, boost::shared_ptr<IObject>>(
        //                 "IPhysicsMaterial2" )
        //                 .def( "setFriction", &IPhysicsMaterial2::setFriction )
        //                 .def( "getFriction", &IPhysicsMaterial2::getFriction )

        //                .def( "setRestitution", &IPhysicsMaterial2::setRestitution )
        //                .def( "getRestitution", &IPhysicsMaterial2::getRestitution )

        //                .def( "setContactPosition", &IPhysicsMaterial2::setContactPosition )
        //                .def( "getContactPosition", &IPhysicsMaterial2::getContactPosition )

        //                .def( "setContactNormal", &IPhysicsMaterial2::setContactNormal )
        //                .def( "getContactNormal", &IPhysicsMaterial2::getContactNormal )

        //                .def( "setPhysicsBodyA", &IPhysicsMaterial2::setPhysicsBodyA )
        //                .def( "getPhysicsBodyA", &IPhysicsMaterial2::getPhysicsBodyA )

        //                .def( "setPhysicsBodyB", &IPhysicsMaterial2::setPhysicsBodyB )
        //                .def( "getPhysicsBodyB", &IPhysicsMaterial2::getPhysicsBodyB )];

        // module( L )[class_<IRigidBody2, IPhysicsBody2, boost::shared_ptr<IObject>>( "IRigidBody2" )];

        // module( L )[class_<WPPhysicsRigidBody2, IRigidBody2, boost::shared_ptr<IObject>>( "WPPhysicsRigidBody2" )];

        // module( L )[class_<IPhysicsParticle2, IPhysicsBody2, boost::shared_ptr<IObject>>(
        //     "IPhysicsParticle2" )];

        // module( L )[class_<WPPhysicsParticle2, IPhysicsParticle2, boost::shared_ptr<IObject>>( "WPPhysicsParticle2" )];

        // module(
        //     L )[class_<IPhysicsShape2, IScriptObject, boost::shared_ptr<IObject>>( "PhysicsShape2" )];

        // module( L )[class_<ISphereShape2, IPhysicsShape2, boost::shared_ptr<IObject>>( "WPPhysicsSphereShape2"
        // )
        //                 .def( "getRadius", &ISphereShape2::getRadius )
        //                 .def( "setRadius", &ISphereShape2::setRadius )];

        // module( L )[class_<IBoxShape2, IPhysicsShape2, boost::shared_ptr<IObject>>( "BoxShape2" )
        //                 .def( "setAABB", &IBoxShape2::setAABB )
        //                 .def( "getAABB", &IBoxShape2::getAABB )];

        // module(
        //     L )[class_<IPhysicsWorld2, IScriptObject, boost::shared_ptr<IObject>>( "PhysicsWorld2" )
        //             .def( "removeRigidBody", &IPhysicsWorld2::removeRigidBody )
        //             .def( "addRigidBody", &IPhysicsWorld2::addRigidBody )];

        // module( L )[class_<IPhysicsManager2, IScriptObject, boost::shared_ptr<IObject>>(
        //                 "PhysicsManager2" )
        //                 .def( "createCollisionShape", _createCollisionShape )
        //                 .def( "createSphere", _createSphere )
        //                 .def( "createBox", _createBox )
        //                 .def( "createRigidBody", _createRigidBody )
        //                 .def( "createRigidBody", _createRigidBodyFromShape )
        //                 //.def("createRigidBody", _createRigidBodyFromSphere )
        //                 //.def("createRigidBody", _createRigidBodyFromBox )

        //                .def( "addWorld", &IPhysicsManager2::addWorld )
        //                .def( "removeWorld", &IPhysicsManager2::removeWorld )
        //                .def( "findWorld", &IPhysicsManager2::findWorld )];

        // module(
        //     L )[class_<PhysicsContainer2, IComponent, boost::shared_ptr<IObject>>( "PhysicsContainer2"
        //     )
        //             .def( "getRigidBody", _getRigidBody )
        //             .def( "getRigidBody", _getRigidBodyHash )
        //             .def( "setRigidBody", _setRigidBody )

        //            .def( "getShape", _getShape )
        //            .def( "setShape", _setShape )];

        // module(
        //     L )[class_<PhysicsResponse2, IComponent, boost::shared_ptr<IObject>>( "PhysicsResponse2" )
        //             .def( "addBody", &PhysicsResponse2::addBody )
        //             .def( "addBody", _addBody )
        //             .def( "getUseMessages", &PhysicsResponse2::getUseMessages )
        //             .def( "setUseMessages", &PhysicsResponse2::setUseMessages )];

        ////
        //// 3d physics
        ////

        // module(
        //     L )[class_<IPhysicsShape3, IScriptObject, boost::shared_ptr<IObject>>( "IPhysicsShape3"
        //     )];

        // module( L )[class_<IBoxShape3, IPhysicsShape3, boost::shared_ptr<IObject>>( "IBoxShape3" )
        //                 .def( "getExtents", &IBoxShape3::getExtents )
        //                 .def( "setExtents", &IBoxShape3::setExtents )

        //                .def( "getAABB", &IBoxShape3::getAABB )
        //                .def( "setAABB", &IBoxShape3::setAABB )];

        // module( L )[class_<IPhysicsBody3, IScriptObject, boost::shared_ptr<IObject>>( "IPhysicsBody3"
        // )
        //                 //.def("setPosition", &IPhysicsBody3::setPosition )
        //                 //.def("getPosition", &IPhysicsBody3::getPosition )

        //                .def( "getMass", &IPhysicsBody3::getMass )
        //                .def( "setMass", &IPhysicsBody3::setMass )

        //                .def( "setCollisionType", _setCollisionType<IPhysicsBody3> )
        //                .def( "getCollisionType", _getCollisionType<IPhysicsBody3> )

        //                .def( "setCollisionMask", _setCollisionMask<IPhysicsBody3> )
        //                .def( "getCollisionMask", _getCollisionMask<IPhysicsBody3> )];

        // module( L )[class_<IRigidBody3, IPhysicsBody3, boost::shared_ptr<IObject>>( "IRigidBody3" )];

        // module( L )[class_<IRigidStatic3, IPhysicsBody3, boost::shared_ptr<IObject>>( "IRigidStatic3"
        // )];

        // module( L )[class_<ICharacterController3, IPhysicsBody3, boost::shared_ptr<IObject>>(
        //                 "CharacterController3" )
        //                 //.def("setPosition", &ICharacterController3::setPosition )
        //                 //.def("getPosition", &ICharacterController3::getPosition )

        //                .def( "setOrientation", &ICharacterController3::setOrientation )
        //                .def( "getOrientation", &ICharacterController3::getOrientation )];

        // module(
        //     L )[class_<IPhysicsTerrain, IScriptObject, boost::shared_ptr<IObject>>( "PhysicsTerrain" )
        //             .def( "setCollisionType", _setCollisionType<IPhysicsTerrain> )
        //             .def( "getCollisionType", _getCollisionType<IPhysicsTerrain> )

        //            .def( "setCollisionMask", _setCollisionMask<IPhysicsTerrain> )
        //            .def( "getCollisionMask", _getCollisionMask<IPhysicsTerrain> )];

        module(
            L )[class_<IPhysicsScene3, ISharedObject, SmartPtr<IPhysicsScene3>>( "IPhysicsScene3" )
                    .def( "clear", &IPhysicsScene3::clear )
                    .def( "addActor", &IPhysicsScene3::addActor )
                    .def( "removeActor", &IPhysicsScene3::removeActor )
                    .def( "getActors", &IPhysicsScene3::getActors )
                    .def( "hasActor", &IPhysicsScene3::hasActor )
                    .def( "numDynamicActors", &IPhysicsScene3::numDynamicActors )
                    .def( "numStaticActors", &IPhysicsScene3::numStaticActors )
                    .def( "setSize", &IPhysicsScene3::setSize )
                    .def( "getSize", &IPhysicsScene3::getSize )
                    .def( "rayTest", &IPhysicsScene3::rayTest )
                    .def( "intersects", &IPhysicsScene3::intersects )
                    .def( "castRay", static_cast<bool ( IPhysicsScene3::* )(
                                         const Vector3<real_Num> &, const Vector3<real_Num> &,
                                         Array<SmartPtr<IRaycastHit>> & )>( &IPhysicsScene3::castRay ) )
                    .def( "castRay", static_cast<bool ( IPhysicsScene3::* )( const Ray3<real_Num> &,
                                                                             SmartPtr<IRaycastHit> )>(
                                         &IPhysicsScene3::castRay ) )
                    .def( "castRayDynamic", &IPhysicsScene3::castRayDynamic )
                    .def( "simulate", &IPhysicsScene3::simulate )
                    .def( "fetchResults", &IPhysicsScene3::fetchResults )
                    .def( "setGravity", &IPhysicsScene3::setGravity )
                    .def( "getGravity", &IPhysicsScene3::getGravity )
                    .def( "setMinThreads", &IPhysicsScene3::setMinThreads )
                    .def( "getMinThreads", &IPhysicsScene3::getMinThreads )
                    .def( "setMaxThreads", &IPhysicsScene3::setMaxThreads )
                    .def( "getMaxThreads", &IPhysicsScene3::getMaxThreads )
                    .scope[def( "typeInfo", IPhysicsScene3::typeInfo )]];

        module(
            L )[class_<IPhysicsManager, ISharedObject, SmartPtr<IPhysicsManager>>( "IPhysicsManager" )
                    .def( "getEnableDebugDraw", &IPhysicsManager::getEnableDebugDraw )
                    .def( "setEnableDebugDraw", &IPhysicsManager::setEnableDebugDraw )
                    .def( "addMaterial", &IPhysicsManager::addMaterial )
                    .def( "removeMaterial", &IPhysicsManager::removeMaterial )
                    .def( "addScene", &IPhysicsManager::addScene )
                    .def( "removeScene", &IPhysicsManager::removeScene )
                    .def( "addCollisionShapeByType", &IPhysicsManager::addCollisionShapeByType )
                    .def( "removeCollisionShape", &IPhysicsManager::removeCollisionShape )
                    .def( "removePhysicsBody", &IPhysicsManager::removePhysicsBody )
                    .def( "addCharacter", &IPhysicsManager::addCharacter )
                    .def( "addRigidStatic",
                          static_cast<SmartPtr<IRigidStatic3> ( IPhysicsManager::* )(
                              const Transform3<real_Num> & )>( &IPhysicsManager::addRigidStatic ) )
                    .def( "addRigidDynamic", &IPhysicsManager::addRigidDynamic )
                    .def( "addRigidStatic",
                          static_cast<SmartPtr<IRigidStatic3> ( IPhysicsManager::* )(
                              SmartPtr<IPhysicsShape3> )>( &IPhysicsManager::addRigidStatic ) )
                    .def( "addRigidStatic", static_cast<SmartPtr<IRigidStatic3> ( IPhysicsManager::* )(
                                                SmartPtr<IPhysicsShape3>, SmartPtr<Properties> )>(
                                                &IPhysicsManager::addRigidStatic ) )
                    .def( "addVehicle", static_cast<SmartPtr<IPhysicsVehicle3> ( IPhysicsManager::* )(
                                            SmartPtr<IRigidBody3> )>( &IPhysicsManager::addVehicle ) )
                    .def( "addVehicle", static_cast<SmartPtr<IPhysicsVehicle3> ( IPhysicsManager::* )(
                                            SmartPtr<IRigidBody3>, const SmartPtr<Properties> & )>(
                                            &IPhysicsManager::addVehicle ) )
                    .def( "removeVehicle", &IPhysicsManager::removeVehicle )
                    .def( "rayTest", &IPhysicsManager::rayTest )
                    .def( "intersects", &IPhysicsManager::intersects )
                    .def( "addConstraintD6", &IPhysicsManager::addConstraintD6 )
                    .def( "addFixedConstraint", &IPhysicsManager::addFixedConstraint )
                    .def( "removeConstraint", &IPhysicsManager::removeConstraint )
                    .def( "addConstraintDrive", &IPhysicsManager::addConstraintDrive )
                    .def( "addConstraintLinearLimit", &IPhysicsManager::addConstraintLinearLimit )
                    .def( "addRaycastHitData", &IPhysicsManager::addRaycastHitData )
                    .def( "removeRaycastHitData", &IPhysicsManager::removeRaycastHitData )
                    .def( "getStateTask", _getStateTask )
                    .def( "getPhysicsTask", _getPhysicsTask )
                    .def( "loadObject", &IPhysicsManager::loadObject )
                    .def( "unloadObject", &IPhysicsManager::unloadObject )
                    .def( "getPhysicsScene", &IPhysicsManager::getPhysicsScene )
                    .def( "setPhysicsScene", &IPhysicsManager::setPhysicsScene )
                    .def( "getObjectsScene", &IPhysicsManager::getObjectsScene )
                    .def( "setObjectsScene", &IPhysicsManager::setObjectsScene )
                    .def( "getRaycastScene", &IPhysicsManager::getRaycastScene )
                    .def( "setRaycastScene", &IPhysicsManager::setRaycastScene )
                    .def( "getControlsScene", &IPhysicsManager::getControlsScene )
                    .def( "setControlsScene", &IPhysicsManager::setControlsScene )
                    .scope[def( "typeInfo", IPhysicsManager::typeInfo )]];

        module( L )[class_<IPhysicsConstraint, ISharedObject, SmartPtr<IPhysicsConstraint>>(
                        "IPhysicsConstraint" )
                        .scope[def( "typeInfo", IPhysicsConstraint::typeInfo )]];

        module( L )[class_<IPhysicsConstraint2, IPhysicsConstraint, SmartPtr<IPhysicsConstraint2>>(
                        "IPhysicsConstraint2" )
                        .def( "getBodyA", &IPhysicsConstraint2::getBodyA )
                        .def( "setBodyA", &IPhysicsConstraint2::setBodyA )
                        .def( "getBodyB", &IPhysicsConstraint2::getBodyB )
                        .def( "setBodyB", &IPhysicsConstraint2::setBodyB )
                        .scope[def( "typeInfo", IPhysicsConstraint2::typeInfo )]];

        module( L )[class_<IPhysicsConstraint3, IPhysicsConstraint, SmartPtr<IPhysicsConstraint3>>(
                        "IPhysicsConstraint3" )
                        .def( "getBodyA", &IPhysicsConstraint3::getBodyA )
                        .def( "setBodyA", &IPhysicsConstraint3::setBodyA )
                        .def( "getBodyB", &IPhysicsConstraint3::getBodyB )
                        .def( "setBodyB", &IPhysicsConstraint3::setBodyB )
                        .def( "setLocalPose", &IPhysicsConstraint3::setLocalPose )
                        .def( "getLocalPose", &IPhysicsConstraint3::getLocalPose )
                        .def( "setConstraintFlag", _setConstraintFlag )
                        .def( "getConstraintFlags", _getConstraintFlags )
                        .def( "setBreakForce", &IPhysicsConstraint3::setBreakForce )
                        .def( "getBreakForce", &IPhysicsConstraint3::getBreakForce )
                        .def( "setProjectionLinearTolerance",
                              &IPhysicsConstraint3::setProjectionLinearTolerance )
                        .def( "getProjectionLinearTolerance",
                              &IPhysicsConstraint3::getProjectionLinearTolerance )
                        .def( "setProjectionAngularTolerance",
                              &IPhysicsConstraint3::setProjectionAngularTolerance )
                        .def( "getProjectionAngularTolerance",
                              &IPhysicsConstraint3::getProjectionAngularTolerance )
                        .scope[def( "typeInfo", IPhysicsConstraint3::typeInfo )]];

        module( L )[class_<IPhysicsSpring, ISharedObject, SmartPtr<IPhysicsSpring>>( "IPhysicsSpring" )
                        .def( "getStiffness", &IPhysicsSpring::getStiffness )
                        .def( "setStiffness", &IPhysicsSpring::setStiffness )
                        .def( "getDamping", &IPhysicsSpring::getDamping )
                        .def( "setDamping", &IPhysicsSpring::setDamping )
                        .scope[def( "typeInfo", IPhysicsSpring::typeInfo )]];

        module( L )[class_<IConstraintDrive, IPhysicsSpring, SmartPtr<IConstraintDrive>>(
                        "IConstraintDrive" )
                        .def( "getForceLimit", &IConstraintDrive::getForceLimit )
                        .def( "setForceLimit", &IConstraintDrive::setForceLimit )
                        .def( "getDriveFlags", _getDriveFlags )
                        .def( "setDriveFlags", _setDriveFlags )
                        .def( "setIsAcceleration", &IConstraintDrive::setIsAcceleration )
                        .def( "isAcceleration", &IConstraintDrive::isAcceleration )
                        .scope[def( "typeInfo", IConstraintDrive::typeInfo )]];

        module(
            L )[class_<IConstraintLimit, ISharedObject, SmartPtr<IConstraintLimit>>( "IConstraintLimit" )
                    .def( "getRestitution", &IConstraintLimit::getRestitution )
                    .def( "setRestitution", &IConstraintLimit::setRestitution )
                    .def( "getBounceThreshold", &IConstraintLimit::getBounceThreshold )
                    .def( "setBounceThreshold", &IConstraintLimit::setBounceThreshold )
                    .def( "getStiffness", &IConstraintLimit::getStiffness )
                    .def( "setStiffness", &IConstraintLimit::setStiffness )
                    .def( "getDamping", &IConstraintLimit::getDamping )
                    .def( "setDamping", &IConstraintLimit::setDamping )
                    .def( "getContactDistance", &IConstraintLimit::getContactDistance )
                    .def( "setContactDistance", &IConstraintLimit::setContactDistance )
                    .scope[def( "typeInfo", IConstraintLimit::typeInfo )]];

        module( L )[class_<IConstraintLinearLimit, IConstraintLimit, SmartPtr<IConstraintLinearLimit>>(
                        "IConstraintLinearLimit" )
                        .def( "getValue", &IConstraintLinearLimit::getValue )
                        .def( "setValue", &IConstraintLinearLimit::setValue )
                        .scope[def( "typeInfo", IConstraintLinearLimit::typeInfo )]];

        module(
            L )[class_<IConstraintD6, IPhysicsConstraint3, SmartPtr<IConstraintD6>>( "IConstraintD6" )
                    .def( "setDrivePosition", &IConstraintD6::setDrivePosition )
                    .def( "getDrivePosition", &IConstraintD6::getDrivePosition )
                    .def( "setDrive", _setDrive )
                    .def( "getDrive", _getDrive )
                    .def( "setLinearLimit", &IConstraintD6::setLinearLimit )
                    .def( "getLinearLimit", &IConstraintD6::getLinearLimit )
                    .def( "setMotion", _setMotion )
                    .def( "getMotion", _getMotion )
                    .scope[def( "typeInfo", IConstraintD6::typeInfo )]];

        module( L )[class_<IConstraintFixed2, ISharedObject, SmartPtr<IConstraintFixed2>>(
                        "IConstraintFixed2" )
                        .scope[def( "typeInfo", IConstraintFixed2::typeInfo )]];

        module( L )[class_<IConstraintFixed3, IPhysicsConstraint3, SmartPtr<IConstraintFixed3>>(
                        "IConstraintFixed3" )
                        .scope[def( "typeInfo", IConstraintFixed3::typeInfo )]];

        module(
            L )[class_<IPhysicsEffect2, ISharedObject, SmartPtr<IPhysicsEffect2>>( "IPhysicsEffect2" )
                    .def( "isEnabled", &IPhysicsEffect2::isEnabled )
                    .def( "setEnabled", &IPhysicsEffect2::setEnabled )
                    .def( "getStrength", &IPhysicsEffect2::getStrength )
                    .def( "setStrength", &IPhysicsEffect2::setStrength )
                    .def( "handleEvent", &IPhysicsEffect2::handleEvent )
                    .scope[def( "typeInfo", IPhysicsEffect2::typeInfo )]];

        module( L )[class_<IPhysicsBodyEffect2, IPhysicsEffect2, SmartPtr<IPhysicsBodyEffect2>>(
                        "IPhysicsBodyEffect2" )
                        .def( "getOwner", &IPhysicsBodyEffect2::getOwner )
                        .def( "setOwner", &IPhysicsBodyEffect2::setOwner )
                        .scope[def( "typeInfo", IPhysicsBodyEffect2::typeInfo )]];

        module(
            L )[class_<IPhysicsBodyEffectSnap2, IPhysicsBodyEffect2, SmartPtr<IPhysicsBodyEffectSnap2>>(
                    "IPhysicsBodyEffectSnap2" )
                    .def( "getTarget", &IPhysicsBodyEffectSnap2::getTarget )
                    .def( "setTarget", &IPhysicsBodyEffectSnap2::setTarget )
                    .def( "getUseAxis", &IPhysicsBodyEffectSnap2::getUseAxis )
                    .def( "setUseAxis", &IPhysicsBodyEffectSnap2::setUseAxis )
                    .scope[def( "typeInfo", IPhysicsBodyEffectSnap2::typeInfo )]];

        module( L )[class_<IPhysicsCompositeShape3, IPhysicsShape3, SmartPtr<IPhysicsCompositeShape3>>(
                        "IPhysicsCompositeShape3" )
                        .def( "getShapes", &IPhysicsCompositeShape3::getShapes )
                        .def( "setShapes", &IPhysicsCompositeShape3::setShapes )
                        .scope[def( "typeInfo", IPhysicsCompositeShape3::typeInfo )]];

        module( L )[class_<IPhysicsDebug, ISharedObject, SmartPtr<IPhysicsDebug>>( "IPhysicsDebug" )
                        .def( "drawLine", &IPhysicsDebug::drawLine )
                        .scope[def( "typeInfo", IPhysicsDebug::typeInfo )]];

        module( L )
            [class_<IPhysicsManager2D, ISharedObject, SmartPtr<IPhysicsManager2D>>( "IPhysicsManager2D" )
                 .def( "updateRigidBodies", &IPhysicsManager2D::updateRigidBodies )
                 .def( "updateParticles", &IPhysicsManager2D::updateParticles )
                 .def( "addWorld", &IPhysicsManager2D::addWorld )
                 .def( "removeWorld", &IPhysicsManager2D::removeWorld )
                 .def( "findWorld", &IPhysicsManager2D::findWorld )
                 .def( "getWorlds", &IPhysicsManager2D::getWorlds )
                 .def( "createCollisionShapeByType", &IPhysicsManager2D::createCollisionShapeByType )
                 .def( "createRigidBody", static_cast<SmartPtr<IRigidBody2> ( IPhysicsManager2D::* )()>(
                                              &IPhysicsManager2D::createRigidBody ) )
                 .def( "createRigidBody",
                       static_cast<SmartPtr<IRigidBody2> ( IPhysicsManager2D::* )(
                           SmartPtr<IPhysicsShape2> )>( &IPhysicsManager2D::createRigidBody ) )
                 .def( "createSoftBody", &IPhysicsManager2D::createSoftBody )
                 .def( "createParticle", &IPhysicsManager2D::createParticle )
                 .def( "clear", &IPhysicsManager2D::clear )
                 .def( "getParticle", &IPhysicsManager2D::getParticle )
                 .def( "OnChangeFlags", &IPhysicsManager2D::OnChangeFlags )
                 .def( "isColliding", &IPhysicsManager2D::isColliding )
                 .scope[def( "typeInfo", IPhysicsManager2D::typeInfo )]];

        module(
            L )[class_<IPhysicsMaterial3, IResource, SmartPtr<IPhysicsMaterial3>>( "IPhysicsMaterial3" )
                    .def( "getFriction", &IPhysicsMaterial3::getFriction )
                    .def( "setFriction", &IPhysicsMaterial3::setFriction )
                    .def( "getDynamicFriction", &IPhysicsMaterial3::getDynamicFriction )
                    .def( "setDynamicFriction", &IPhysicsMaterial3::setDynamicFriction )
                    .def( "getStaticFriction", &IPhysicsMaterial3::getStaticFriction )
                    .def( "setStaticFriction", &IPhysicsMaterial3::setStaticFriction )
                    .def( "getRestitution", &IPhysicsMaterial3::getRestitution )
                    .def( "setRestitution", &IPhysicsMaterial3::setRestitution )
                    .def( "getContactPosition", &IPhysicsMaterial3::getContactPosition )
                    .def( "getContactNormal", &IPhysicsMaterial3::getContactNormal )
                    .def( "getPhysicsBodyA", &IPhysicsMaterial3::getPhysicsBodyA )
                    .def( "getPhysicsBodyB", &IPhysicsMaterial3::getPhysicsBodyB )
                    .scope[def( "typeInfo", IPhysicsMaterial3::typeInfo )]];

        module(
            L )[class_<IPhysicsParticle3, IPhysicsBody3, SmartPtr<IPhysicsParticle3>>(
                    "IPhysicsParticle3" )
                    .def( "getCollisionShape",
                          static_cast<const SmartPtr<IPhysicsShape3> &(IPhysicsParticle3::*)() const>(
                              &IPhysicsParticle3::getCollisionShape ) )
                    .def( "setCollisionShape", &IPhysicsParticle3::setCollisionShape )
                    .scope[def( "typeInfo", IPhysicsParticle3::typeInfo )]];

        module( L )[class_<IPhysicsScene2, ISharedObject, SmartPtr<IPhysicsScene2>>( "IPhysicsScene2" )
                        .def( "updateRigidBodies", &IPhysicsScene2::updateRigidBodies )
                        .def( "updateParticles", &IPhysicsScene2::updateParticles )
                        .def( "addRigidBody", &IPhysicsScene2::addRigidBody )
                        .def( "removeRigidBody", &IPhysicsScene2::removeRigidBody )
                        .def( "addParticle", &IPhysicsScene2::addParticle )
                        .def( "removeParticle", &IPhysicsScene2::removeParticle )
                        .def( "setSize", &IPhysicsScene2::setSize )
                        .def( "getSize", &IPhysicsScene2::getSize )
                        .def( "setGravity", &IPhysicsScene2::setGravity )
                        .def( "getGravity", &IPhysicsScene2::getGravity )
                        .scope[def( "typeInfo", IPhysicsScene2::typeInfo )]];

        module( L )[class_<IPhysicsSoftBody2, ISharedObject, SmartPtr<IPhysicsSoftBody2>>(
                        "IPhysicsSoftBody2" )
                        .def( "setPosition", &IPhysicsSoftBody2::setPosition )
                        .def( "getPosition", &IPhysicsSoftBody2::getPosition )
                        .scope[def( "typeInfo", IPhysicsSoftBody2::typeInfo )]];

        module( L )[class_<IPhysicsSoftBody3, ISharedObject, SmartPtr<IPhysicsSoftBody3>>(
                        "IPhysicsSoftBody3" )
                        .def( "setPosition", &IPhysicsSoftBody3::setPosition )
                        .def( "getPosition", &IPhysicsSoftBody3::getPosition )
                        .scope[def( "typeInfo", IPhysicsSoftBody3::typeInfo )]];

        module(
            L )[class_<IPhysicsVehicle3, ISharedObject, SmartPtr<IPhysicsVehicle3>>( "IPhysicsVehicle3" )
                    .def( "addWheel", &IPhysicsVehicle3::addWheel )
                    .def( "getWheel", &IPhysicsVehicle3::getWheel )
                    .def( "getNumWheels", &IPhysicsVehicle3::getNumWheels )
                    .def( "finalize", &IPhysicsVehicle3::finalize )
                    .def( "applyEngineForce", &IPhysicsVehicle3::applyEngineForce )
                    .def( "setBrake", &IPhysicsVehicle3::setBrake )
                    .def( "setSteeringValue", &IPhysicsVehicle3::setSteeringValue )
                    .def( "setPosition", &IPhysicsVehicle3::setPosition )
                    .def( "getPosition", &IPhysicsVehicle3::getPosition )
                    .def( "setOrientation", &IPhysicsVehicle3::setOrientation )
                    .def( "getOrientation", &IPhysicsVehicle3::getOrientation )
                    .def( "setVelocity", &IPhysicsVehicle3::setVelocity )
                    .def( "getVelocity", &IPhysicsVehicle3::getVelocity )
                    .def( "setMaterialId", &IPhysicsVehicle3::setMaterialId )
                    .def( "getMaterialId", &IPhysicsVehicle3::getMaterialId )
                    .def( "getLocalAABB", &IPhysicsVehicle3::getLocalAABB )
                    .def( "getWorldAABB", &IPhysicsVehicle3::getWorldAABB )
                    .def( "setEnabled", &IPhysicsVehicle3::setEnabled )
                    .def( "isEnabled", &IPhysicsVehicle3::isEnabled )
                    .def( "getVehicleInput",
                          static_cast<const SmartPtr<IPhysicsVehicleInput3> &(IPhysicsVehicle3::*)()
                                          const>( &IPhysicsVehicle3::getVehicleInput ) )
                    .def( "getWheelTransformations", &IPhysicsVehicle3::getWheelTransformations )
                    .scope[def( "typeInfo", IPhysicsVehicle3::typeInfo )]];

        module( L )[class_<IPhysicsVehicleInput3, ISharedObject, SmartPtr<IPhysicsVehicleInput3>>(
                        "IPhysicsVehicleInput3" )
                        .def( "setDigitalAccel", &IPhysicsVehicleInput3::setDigitalAccel )
                        .def( "setDigitalBrake", &IPhysicsVehicleInput3::setDigitalBrake )
                        .def( "setDigitalHandbrake", &IPhysicsVehicleInput3::setDigitalHandbrake )
                        .def( "setDigitalSteerLeft", &IPhysicsVehicleInput3::setDigitalSteerLeft )
                        .def( "setDigitalSteerRight", &IPhysicsVehicleInput3::setDigitalSteerRight )
                        .def( "getDigitalAccel", &IPhysicsVehicleInput3::getDigitalAccel )
                        .def( "getDigitalBrake", &IPhysicsVehicleInput3::getDigitalBrake )
                        .def( "getDigitalHandbrake", &IPhysicsVehicleInput3::getDigitalHandbrake )
                        .def( "getDigitalSteerLeft", &IPhysicsVehicleInput3::getDigitalSteerLeft )
                        .def( "getDigitalSteerRight", &IPhysicsVehicleInput3::getDigitalSteerRight )
                        .def( "setAnalogAccel", &IPhysicsVehicleInput3::setAnalogAccel )
                        .def( "setAnalogBrake", &IPhysicsVehicleInput3::setAnalogBrake )
                        .def( "setAnalogHandbrake", &IPhysicsVehicleInput3::setAnalogHandbrake )
                        .def( "setAnalogSteer", &IPhysicsVehicleInput3::setAnalogSteer )
                        .def( "getAnalogAccel", &IPhysicsVehicleInput3::getAnalogAccel )
                        .def( "getAnalogBrake", &IPhysicsVehicleInput3::getAnalogBrake )
                        .def( "getAnalogHandbrake", &IPhysicsVehicleInput3::getAnalogHandbrake )
                        .def( "getAnalogSteer", &IPhysicsVehicleInput3::getAnalogSteer )
                        .def( "setGearUp", &IPhysicsVehicleInput3::setGearUp )
                        .def( "setGearDown", &IPhysicsVehicleInput3::setGearDown )
                        .def( "getGearUp", &IPhysicsVehicleInput3::getGearUp )
                        .def( "getGearDown", &IPhysicsVehicleInput3::getGearDown )
                        .scope[def( "typeInfo", IPhysicsVehicleInput3::typeInfo )]];

        module(
            L )[class_<IPhysicsVehicleWheel3, ISharedObject, SmartPtr<IPhysicsVehicleWheel3>>(
                    "IPhysicsVehicleWheel3" )
                    .def( "getRadius", &IPhysicsVehicleWheel3::getRadius )
                    .def( "setRadius", &IPhysicsVehicleWheel3::setRadius )
                    .def( "getWidth", &IPhysicsVehicleWheel3::getWidth )
                    .def( "setWidth", &IPhysicsVehicleWheel3::setWidth )
                    .def( "getMaxSuspensionTravelCm", &IPhysicsVehicleWheel3::getMaxSuspensionTravelCm )
                    .def( "setMaxSuspensionTravelCm", &IPhysicsVehicleWheel3::setMaxSuspensionTravelCm )
                    .def( "getMaxSuspensionForce", &IPhysicsVehicleWheel3::getMaxSuspensionForce )
                    .def( "setMaxSuspensionForce", &IPhysicsVehicleWheel3::setMaxSuspensionForce )
                    .def( "getSuspensionStiffness", &IPhysicsVehicleWheel3::getSuspensionStiffness )
                    .def( "setSuspensionStiffness", &IPhysicsVehicleWheel3::setSuspensionStiffness )
                    .def( "getSuspensionDamping", &IPhysicsVehicleWheel3::getSuspensionDamping )
                    .def( "setSuspensionDamping", &IPhysicsVehicleWheel3::setSuspensionDamping )
                    .def( "getFrictionSlip", &IPhysicsVehicleWheel3::getFrictionSlip )
                    .def( "setFrictionSlip", &IPhysicsVehicleWheel3::setFrictionSlip )
                    .def( "getSteering", &IPhysicsVehicleWheel3::getSteering )
                    .def( "setSteering", &IPhysicsVehicleWheel3::setSteering )
                    .def( "getEngineForce", &IPhysicsVehicleWheel3::getEngineForce )
                    .def( "setEngineForce", &IPhysicsVehicleWheel3::setEngineForce )
                    .def( "getBrake", &IPhysicsVehicleWheel3::getBrake )
                    .def( "setBrake", &IPhysicsVehicleWheel3::setBrake )
                    .def( "isInContact", &IPhysicsVehicleWheel3::isInContact )
                    .scope[def( "typeInfo", IPhysicsVehicleWheel3::typeInfo )]];

        module( L )[class_<IPlaneShape3, IPhysicsShape3, SmartPtr<IPlaneShape3>>( "IPlaneShape3" )
                        .def( "getDistance", &IPlaneShape3::getDistance )
                        .def( "setDistance", &IPlaneShape3::setDistance )
                        .def( "getNormal", &IPlaneShape3::getNormal )
                        .def( "setNormal", &IPlaneShape3::setNormal )
                        .def( "getPlane", &IPlaneShape3::getPlane )
                        .scope[def( "typeInfo", IPlaneShape3::typeInfo )]];

        module( L )[class_<IMassData2, ISharedObject, SmartPtr<IMassData2>>( "IMassData2" )
                        .def( "setMass", &IMassData2::setMass )
                        .def( "getMass", &IMassData2::getMass )
                        .def( "setCenter", &IMassData2::setCenter )
                        .def( "getCenter", &IMassData2::getCenter )
                        .def( "setInertia", &IMassData2::setInertia )
                        .def( "getInertia", &IMassData2::getInertia )
                        .scope[def( "typeInfo", IMassData2::typeInfo )]];

        module( L )[class_<IMassData3, ISharedObject, SmartPtr<IMassData3>>( "IMassData3" )
                        .scope[def( "typeInfo", IMassData3::typeInfo )]];

        module( L )[class_<ICharacterController2, ISharedObject, SmartPtr<ICharacterController2>>(
                        "ICharacterController2" )
                        .scope[def( "typeInfo", ICharacterController2::typeInfo )]];

        module( L )[class_<IRigidDynamic3, IRigidBody3, SmartPtr<IRigidDynamic3>>( "IRigidDynamic3" )
                        .def( "setKinematicTarget", &IRigidDynamic3::setKinematicTarget )
                        .def( "getKinematicTarget", &IRigidDynamic3::getKinematicTarget )
                        .def( "isKinematic", &IRigidDynamic3::isKinematic )
                        .def( "setKinematic", &IRigidDynamic3::setKinematic )
                        .def( "setLinearDamping", &IRigidDynamic3::setLinearDamping )
                        .def( "getLinearDamping", &IRigidDynamic3::getLinearDamping )
                        .def( "setAngularDamping", &IRigidDynamic3::setAngularDamping )
                        .def( "getAngularDamping", &IRigidDynamic3::getAngularDamping )
                        .def( "setMaxAngularVelocity", &IRigidDynamic3::setMaxAngularVelocity )
                        .def( "getMaxAngularVelocity", &IRigidDynamic3::getMaxAngularVelocity )
                        .def( "isSleeping", &IRigidDynamic3::isSleeping )
                        .def( "setSleepThreshold", &IRigidDynamic3::setSleepThreshold )
                        .def( "getSleepThreshold", &IRigidDynamic3::getSleepThreshold )
                        .def( "setStabilizationThreshold", &IRigidDynamic3::setStabilizationThreshold )
                        .def( "getStabilizationThreshold", &IRigidDynamic3::getStabilizationThreshold )
                        .def( "setWakeCounter", &IRigidDynamic3::setWakeCounter )
                        .def( "getWakeCounter", &IRigidDynamic3::getWakeCounter )
                        .def( "wakeUp", &IRigidDynamic3::wakeUp )
                        .def( "putToSleep", &IRigidDynamic3::putToSleep )
                        .def( "setSolverIterationCounts", &IRigidDynamic3::setSolverIterationCounts )
                        .def( "getSolverIterationCounts", &IRigidDynamic3::getSolverIterationCounts )
                        .def( "getContactReportThreshold", &IRigidDynamic3::getContactReportThreshold )
                        .def( "setContactReportThreshold", &IRigidDynamic3::setContactReportThreshold )
                        .scope[def( "typeInfo", IRigidDynamic3::typeInfo )]];

        module( L )[class_<ISphereShape3, IPhysicsShape3, SmartPtr<ISphereShape3>>( "ISphereShape3" )
                        .def( "setRadius", &ISphereShape3::setRadius )
                        .def( "getRadius", &ISphereShape3::getRadius )
                        .scope[def( "typeInfo", ISphereShape3::typeInfo )]];

        module( L )[class_<ITerrainShape, IPhysicsShape3, SmartPtr<ITerrainShape>>( "ITerrainShape" )
                        .scope[def( "typeInfo", ITerrainShape::typeInfo )]];

        module( L )[class_<IMeshShape, IPhysicsShape3, SmartPtr<IMeshShape>>( "IMeshShape" )
                        .def( "getMesh", &IMeshShape::getMesh )
                        .def( "getMeshResource", &IMeshShape::getMeshResource )
                        .def( "setMeshResource", &IMeshShape::setMeshResource )
                        .def( "getCleanMesh", &IMeshShape::getCleanMesh )
                        .def( "setCleanMesh", &IMeshShape::setCleanMesh )
                        .def( "isConvex", &IMeshShape::isConvex )
                        .def( "setConvex", &IMeshShape::setConvex )
                        .scope[def( "typeInfo", IMeshShape::typeInfo )]];

        module( L )[class_<IRaycastHit, ISharedObject, SmartPtr<IRaycastHit>>( "IRaycastHit" )
                        .def( "getCollider", &IRaycastHit::getCollider )
                        .def( "setCollider", &IRaycastHit::setCollider )
                        .def( "getRigidBody", &IRaycastHit::getRigidBody )
                        .def( "setRigidBody", &IRaycastHit::setRigidBody )
                        .def( "getBarycentricCoordinate", &IRaycastHit::getBarycentricCoordinate )
                        .def( "setBarycentricCoordinate", &IRaycastHit::setBarycentricCoordinate )
                        .def( "getLightmapCoord", &IRaycastHit::getLightmapCoord )
                        .def( "setLightmapCoord", &IRaycastHit::setLightmapCoord )
                        .def( "getNormal", &IRaycastHit::getNormal )
                        .def( "setNormal", &IRaycastHit::setNormal )
                        .def( "getPoint", &IRaycastHit::getPoint )
                        .def( "setPoint", &IRaycastHit::setPoint )
                        .def( "getTextureCoord", &IRaycastHit::getTextureCoord )
                        .def( "setTextureCoord", &IRaycastHit::setTextureCoord )
                        .def( "getTextureCoord2", &IRaycastHit::getTextureCoord2 )
                        .def( "setTextureCoord2", &IRaycastHit::setTextureCoord2 )
                        .def( "getDistance", &IRaycastHit::getDistance )
                        .def( "setDistance", &IRaycastHit::setDistance )
                        .def( "getTriangleIndex", &IRaycastHit::getTriangleIndex )
                        .def( "setTriangleIndex", &IRaycastHit::setTriangleIndex )
                        .def( "getCollisionMask", &IRaycastHit::getCollisionMask )
                        .def( "setCollisionMask", &IRaycastHit::setCollisionMask )
                        .def( "getCheckStatic", &IRaycastHit::getCheckStatic )
                        .def( "setCheckStatic", &IRaycastHit::setCheckStatic )
                        .def( "getCheckDynamic", &IRaycastHit::getCheckDynamic )
                        .def( "setCheckDynamic", &IRaycastHit::setCheckDynamic )
                        .scope[def( "typeInfo", IRaycastHit::typeInfo )]];
    }
} // namespace workphone
