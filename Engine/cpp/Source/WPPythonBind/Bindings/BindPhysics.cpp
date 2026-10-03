#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <boost/python.hpp>
#include <WPPythonBind/Bindings/BindPhysics.hpp>
#include <WPPythonBind/Helpers/PythonHelper.hpp>
#include <Workphone/Workphone.hpp>
#include <WPApplication/WPApplication.hpp>

namespace fb
{

    SmartPtr<physics::IPhysicsShape2> _createCollisionShape( physics::IPhysicsManager2D *mgr,
                                                             const char *pShapeName )
    {
        //String shapeName = pShapeName;
        //if( shapeName.equals_ignore_case( "sphere" ) )
        //{
        //    return mgr->createCollisionShape( PST_SPHERE );
        //}
        //else if( shapeName.equals_ignore_case( "box" ) )
        //{
        //    return mgr->createCollisionShape( PST_BOX );
        //}

        return nullptr;
    }

    SmartPtr<physics::ISphereShape2> _createSphere( physics::IPhysicsManager2D *mgr )
    {
        return nullptr;  //mgr->createCollisionShape( PST_SPHERE );
    }

    SmartPtr<physics::IBoxShape2> _createBox( physics::IPhysicsManager2D *mgr )
    {
        return nullptr;  //mgr->createCollisionShape( PST_BOX );
    }

    SmartPtr<physics::IRigidBody2> _createRigidBody( physics::IPhysicsManager2D *mgr )
    {
        return mgr->createRigidBody();
    }

    SmartPtr<physics::IRigidBody2> _createRigidBodyFromShape( physics::IPhysicsManager2D *mgr,
                                                              SmartPtr<physics::IPhysicsShape2> shape )
    {
        return mgr->createRigidBody( shape );
    }

    SmartPtr<physics::IRigidBody2> _createRigidBodyFromSphere( physics::IPhysicsManager2D *mgr,
                                                               SmartPtr<physics::ISphereShape2> shape )
    {
        return mgr->createRigidBody( shape );
    }

    SmartPtr<physics::IRigidBody2> _createRigidBodyFromBox( physics::IPhysicsManager2D *mgr,
                                                            SmartPtr<physics::IBoxShape2> shape )
    {
        return mgr->createRigidBody( shape );
    }

    void _setUserData( physics::IPhysicsBody2D *body, IObject *obj )
    {
        body->setUserData( obj );
    }

    Parameter _getUserData( physics::IPhysicsBody2D *body )
    {
        Parameter param;
        param.setPtr( (IObject *)body->getUserData() );
        return param;
    }

    void _setRigidBody( scene::PhysicsContainer2 *container, python_Integer hash,
                        SmartPtr<physics::IRigidBody2> rigidBody )
    {
        u32 id = *reinterpret_cast<u32 *>( &hash );
        container->setObject( id, rigidBody );
    }

    void _addBody( scene::PhysicsContainer2 *physicsResponse, IObject *obj )
    {
        //physicsResponse->addBody( obj );
    }

    SmartPtr<physics::IRigidBody2> _getRigidBody( scene::PhysicsContainer2 *container, const char *name )
    {
        hash32 hash = StringUtil::getHash( name );
        return container->getRigidBody( hash );
    }

    SmartPtr<physics::IRigidBody2> _getRigidBodyHash( scene::PhysicsContainer2 *container,
                                                      python_Integer hash )
    {
        u32 id = *reinterpret_cast<u32 *>( &hash );
        return container->getRigidBody( id );
    }

    void _setShape( scene::PhysicsContainer2 *container, python_Integer hash,
                    SmartPtr<physics::IPhysicsShape2> shape )
    {
        u32 id = *reinterpret_cast<u32 *>( &hash );
        container->setShape( id, shape );
    }

    SmartPtr<physics::IPhysicsShape2> _getShape( scene::PhysicsContainer2 *container,
                                                 python_Integer hash )
    {
        u32 id = *reinterpret_cast<u32 *>( &hash );
        return container->getShape( id );
    }

    void _addVelocity( physics::IPhysicsBody2D *body, const Vector2<physics_Num> &vel )
    {
        body->addVelocity( vel );
    }

    SmartPtr<ISharedObject> _getStateObject( physics::IPhysicsBody2D *body )
    {
        return nullptr;  // body->getStateObject().getPtr();
    }

    void _setBodyPosition( physics::IPhysicsBody2D *body, const Vector2F &position )
    {
        body->setPosition( Vector2<physics_Num>( position.X(), position.Y() ) );
    }

    void _addBodyForce( physics::IPhysicsBody2D *body, const Vector2F &force )
    {
        body->addForce( Vector2<physics_Num>( force.X(), force.Y() ) );
    }

    void _setBodyForce( physics::IPhysicsBody2D *body, const Vector2F &force )
    {
        body->setForce( Vector2<physics_Num>( force.X(), force.Y() ) );
    }

    template <class T>
    void _setCollisionType( T *body, python_Integer mask )
    {
        body->setCollisionType( *reinterpret_cast<u32 *>( &mask ) );
    }

    template <class T>
    python_Integer _getCollisionType( T *body )
    {
        u32 mask = body->getCollisionType();
        return *reinterpret_cast<python_Integer *>( &mask );
    }

    template <class T>
    void _setCollisionMask( T *body, python_Integer mask )
    {
        body->setCollisionMask( *reinterpret_cast<u32 *>( &mask ) );
    }

    template <class T>
    python_Integer _getCollisionMask( T *body )
    {
        u32 mask = body->getCollisionMask();
        return *reinterpret_cast<python_Integer *>( &mask );
    }

    void _setObjectType( physics::IPhysicsBody2D *body, python_Integer hash )
    {
        body->setObjectType( *reinterpret_cast<hash32 *>( &hash ) );
    }

    python_Integer _getObjectType( physics::IPhysicsBody2D *body )
    {
        hash32 hash = body->getObjectType();
        return *reinterpret_cast<python_Integer *>( &hash );
    }

    SmartPtr<physics::ITerrainShape> _createTerrain( physics::IPhysicsManager *physicsMgr,
                                                     SmartPtr<IObjectDirector> objectTemplate )
    {
        return nullptr;  // physicsMgr->createTerrain( objectTemplate );
    }

    void bindPhysics()
    {
        using namespace boost::python;

        class_<physics::IPhysicsBody2D, SmartPtr<physics::IPhysicsBody2D>, bases<ISharedObject>,
               boost::noncopyable>( "PhysicsBody2", no_init )
            .def( "setPosition", &physics::IPhysicsBody2D::setPosition )
            //.def("setPosition", _setBodyPosition )
            .def( "getPosition", &physics::IPhysicsBody2D::getPosition )

            //.def("setTargetPosition", &IPhysicsBody2::setTargetPosition )
            //.def("getTargetPosition", &IPhysicsBody2::getTargetPosition )

            //.def("addVelocity", _addVelocity )
            //.def("addVelocity", &IPhysicsBody2::addVelocity )
            //.def("setVelocity", &IPhysicsBody2::setVelocity )
            //.def("getVelocity", &IPhysicsBody2::getVelocity )

            ////.def("addForce", _addBodyForce )
            //.def("addForce", &IPhysicsBody2::addForce )
            ////.def("setForce", _setBodyForce )
            //.def("setForce", &IPhysicsBody2::setForce )
            //.def("getForce", &IPhysicsBody2::getForce )

            //.def("addTorque", &IPhysicsBody2::addTorque )
            //.def("setTorque", &IPhysicsBody2::setTorque )
            //.def("getTorque", &IPhysicsBody2::getTorque )

            //.def("setLinearDampValue", &IPhysicsBody2::setLinearDampValue )
            //.def("getLinearDampValue", &IPhysicsBody2::getLinearDampValue )

            //.def("setAngularDampValue", &IPhysicsBody2::setAngularDampValue )
            //.def("getAngularDampValue", &IPhysicsBody2::getAngularDampValue )

            //.def("setAirResistance", &IPhysicsBody2::setAirResistance )
            //.def("getAirResistance", &IPhysicsBody2::getAirResistance )

            //.def("setRestitution", &IPhysicsBody2::setRestitution )
            //.def("getRestitution", &IPhysicsBody2::getRestitution )

            //.def("setMass", &IPhysicsBody2::setMass )
            //.def("getMass", &IPhysicsBody2::getMass )
            //.def("getMassInv", &IPhysicsBody2::getMassInv )

            //.def("setFlag", &IPhysicsBody2::setFlag )
            //.def("getFlag", &IPhysicsBody2::getFlag )

            //.def("setObjectType", _setObjectType )
            //.def("getObjectType", _getObjectType )

            //.def("setWorldId", &IPhysicsBody2::setWorldId )
            //.def("getWorldId", &IPhysicsBody2::getWorldId )

            //.def("setEnabled", &IPhysicsBody2::setEnabled )
            //.def("isEnabled", &IPhysicsBody2::isEnabled )

            //.def("getLocalAABB", &IPhysicsBody2::getLocalAABB )
            //.def("getWorldAABB", &IPhysicsBody2::getWorldAABB )

            //.def("setMaterialId", &IPhysicsBody2::setMaterialId )
            //.def("getMaterialId", &IPhysicsBody2::getMaterialId )

            //.def("setCollisionType", _setCollisionType<IPhysicsBody2> )
            //.def("getCollisionType", _getCollisionType<IPhysicsBody2> )

            //.def("setCollisionMask", _setCollisionMask<IPhysicsBody2> )
            //.def("getCollisionMask", _getCollisionMask<IPhysicsBody2> )

            //.def("setSleep", &IPhysicsBody2::setSleep )
            //.def("isSleeping", &IPhysicsBody2::isSleeping )

            //.def("setContraintAABB", &IPhysicsBody2::setContraintAABB )
            //.def("getContraintAABB", &IPhysicsBody2::getContraintAABB )

            //.def("setUserData", _setUserData )
            //.def("getUserData", _getUserData )

            //.def("getStateObject", _getStateObject )
            ;

        class_<physics::IPhysicsMaterial2, SmartPtr<physics::IPhysicsMaterial2>, bases<ISharedObject>,
               boost::noncopyable>( "IPhysicsMaterial2", no_init )
            .def( "setFriction", &physics::IPhysicsMaterial2::setFriction )
            .def( "getFriction", &physics::IPhysicsMaterial2::getFriction )

            .def( "setRestitution", &physics::IPhysicsMaterial2::setRestitution )
            .def( "getRestitution", &physics::IPhysicsMaterial2::getRestitution )

            .def( "setContactPosition", &physics::IPhysicsMaterial2::setContactPosition )
            .def( "getContactPosition", &physics::IPhysicsMaterial2::getContactPosition )

            .def( "setContactNormal", &physics::IPhysicsMaterial2::setContactNormal )
            .def( "getContactNormal", &physics::IPhysicsMaterial2::getContactNormal )

            //.def("setPhysicsBodyA", &IPhysicsMaterial2::setPhysicsBodyA )
            //.def("getPhysicsBodyA", &IPhysicsMaterial2::getPhysicsBodyA )

            //.def("setPhysicsBodyB", &IPhysicsMaterial2::setPhysicsBodyB )
            //.def("getPhysicsBodyB", &IPhysicsMaterial2::getPhysicsBodyB )
            ;

        /*
        class_<IRigidBody2, SmartPtr<physics::IRigidBody2>, bases<IObject>, boost::noncopyable>( "RigidBody2", no_init );

        class_<IPhysicsShape2, SmartPtr<physics::IPhysicsShape2>, bases<IObject>, boost::noncopyable>( "PhysicsShape2",
                                                                                      no_init );

        class_<ISphereShape2, SmartPtr<physics::ISphereShape2>, bases<IObject>, boost::noncopyable>( "SphereShape2",
                                                                                    no_init )
            .def( "getRadius", &ISphereShape2::getRadius )
            .def( "setRadius", &ISphereShape2::setRadius );

        class_<IBoxShape2, SmartPtr<physics::IBoxShape2>, bases<IObject>, boost::noncopyable>( "BoxShape2", no_init )
            .def( "setAABB", &IBoxShape2::setAABB )
            .def( "getAABB", &IBoxShape2::getAABB );

        class_<IPhysicsWorld2, PhysicsWorld2Ptr, bases<IObject>, boost::noncopyable>( "PhysicsWorld2",
                                                                                      no_init )
            .def( "removeRigidBody", &IPhysicsWorld2::removeRigidBody )
            .def( "addRigidBody", &IPhysicsWorld2::addRigidBody );

        class_<IPhysicsManager2, PhysicsManager2Ptr, bases<IObject>, boost::noncopyable>(
            "PhysicsManager2", no_init )
            .def( "createCollisionShape", _createCollisionShape )
            .def( "createSphere", _createSphere )
            .def( "createBox", _createBox )
            .def( "createRigidBody", _createRigidBody )
            .def( "createRigidBody", _createRigidBodyFromShape )
            .def( "createRigidBody", _createRigidBodyFromSphere )
            .def( "createRigidBody", _createRigidBodyFromBox )

            .def( "addWorld", &IPhysicsManager2::addWorld )
            .def( "removeWorld", &IPhysicsManager2::removeWorld )
            .def( "findWorld", &IPhysicsManager2::findWorld );

        class_<PhysicsContainer2, PhysicsContainer2Ptr, bases<IObject>, boost::noncopyable>(
            "PhysicsContainer2", no_init )
            .def( "getRigidBody", _getRigidBody )
            .def( "getRigidBody", _getRigidBodyHash )
            .def( "setRigidBody", _setRigidBody )

            .def( "getShape", _getShape )
            .def( "setShape", _setShape );

        class_<PhysicsResponse2, PhysicsResponse2Ptr, bases<IObject>, boost::noncopyable>(
            "PhysicsResponse2", no_init )
            .def( "addBody", &PhysicsResponse2::addBody )
            .def( "addBody", _addBody )
            .def( "getUseMessages", &PhysicsResponse2::getUseMessages )
            .def( "setUseMessages", &PhysicsResponse2::setUseMessages );

        class_<IPhysicsBody3, PhysicsBody3Ptr, bases<IObject>, boost::noncopyable>( "PhysicsBody3",
                                                                                    no_init )
            .def( "setCollisionType", _setCollisionType<IPhysicsBody3> )
            .def( "getCollisionType", _getCollisionType<IPhysicsBody3> )

            .def( "setCollisionMask", _setCollisionMask<IPhysicsBody3> )
            .def( "getCollisionMask", _getCollisionMask<IPhysicsBody3> );

        class_<IRigidBody3, RigidBody3Ptr, bases<IObject>, boost::noncopyable>( "RigidBody3", no_init );

        class_<ICharacterController3, CharacterController3Ptr, bases<IObject>, boost::noncopyable>(
            "CharacterController3", no_init )
            .def( "setPosition", &ICharacterController3::setPosition )
            .def( "getPosition", &ICharacterController3::getPosition )

            .def( "setOrientation", &ICharacterController3::setOrientation )
            .def( "getOrientation", &ICharacterController3::getOrientation );

        class_<IPhysicsTerrain, PhysicsTerrainPtr, bases<IObject>, boost::noncopyable>( "PhysicsTerrain",
                                                                                        no_init )
            .def( "setCollisionType", _setCollisionType<IPhysicsTerrain> )
            .def( "getCollisionType", _getCollisionType<IPhysicsTerrain> )

            .def( "setCollisionMask", _setCollisionMask<IPhysicsTerrain> )
            .def( "getCollisionMask", _getCollisionMask<IPhysicsTerrain> );

        class_<IPhysicsManager3, PhysicsManager3Ptr, bases<IObject>, boost::noncopyable>(
            "PhysicsManager3", no_init )
            .def( "addCharacter", &IPhysicsManager3::addCharacter )
            .def( "createTerrain", &IPhysicsManager3::createTerrain )
            .def( "createTerrain", _createTerrain )

            .def( "getEnableDebugDraw", &IPhysicsManager3::getEnableDebugDraw )
            .def( "setEnableDebugDraw", &IPhysicsManager3::setEnableDebugDraw );

        PythonHelper::registerPointer<IPhysicsTerrain>();
        PythonHelper::registerPointer<IPhysicsManager3>();
        */
    }

}  // namespace fb
