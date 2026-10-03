#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <Workphone/Workphone.hpp>
#include <WPApplication/WPApplication.hpp>
#include <WPPythonBind/Bindings/BindComponents.hpp>
#include <WPPythonBind/Helpers/ComponentContainerHelper.hpp>
#include <WPPythonBind/Helpers/ComponentHelper.hpp>
#include <WPPythonBind/Helpers/PythonHelper.hpp>
#include <boost/python.hpp>

namespace fb
{

    void bindComponents()
    {
        using namespace fb::scene;
        using namespace boost::python;

        //class_<IComponentContainer, ComponentContainerPtr, bases<IObject>, boost::noncopyable>(
        //    "ComponentContainer", no_init )
        //    .def( "addComponent", ComponentContainerHelper::addComponentFactoryNamed )
        //    .def( "addComponent", ComponentContainerHelper::addComponentFactoryInt )
        //    //.def("setComponent", &IComponentContainer::addComponent )
        //    .def( "removeComponent", &IComponentContainer::removeComponent )
        //    .def( "getComponentById", &IComponentContainer::getComponentById )
        //    .def( "clearComponents", &IComponentContainer::clearComponents );

        class_<IComponent, SmartPtr<scene::IComponent>, bases<ISharedObject>, boost::noncopyable>(
            "IComponent", no_init )
            .def( "getOwner", ComponentHelper::getOwner )
            .def( "setOwner", ComponentHelper::setOwner )
            .def( "isEnabled", &IComponent::isEnabled )
            .def( "setEnabled", &IComponent::setEnabled );

        class_<BaseComponent, SmartPtr<BaseComponent>, bases<IComponent>, boost::noncopyable>(
            "BaseComponent", no_init );

        class_<MovementControl2, SmartPtr<MovementControl2>, bases<IComponent>, boost::noncopyable>(
            "MovementControl2" )
            .def( "getMoveSpeed", &MovementControl2::getMoveSpeed )
            .def( "setMoveSpeed", &MovementControl2::setMoveSpeed )

            .def( "getGameInputId", &MovementControl2::getGameInputId )
            .def( "setGameInputId", &MovementControl2::setGameInputId )

            .def( "getMoveVector", &MovementControl2::getMoveVector )
            .def( "setMoveVector", &MovementControl2::setMoveVector )

            .def( "getGameActionLeft", &MovementControl2::getGameActionLeft )
            .def( "setGameActionLeft", &MovementControl2::setGameActionLeft )
            .def( "getGameActionRight", &MovementControl2::getGameActionRight )
            .def( "setGameActionRight", &MovementControl2::setGameActionRight )

            .def( "getGameActionUp", &MovementControl2::getGameActionUp )
            .def( "setGameActionUp", &MovementControl2::setGameActionUp )
            .def( "getGameActionDown", &MovementControl2::getGameActionDown )
            .def( "setGameActionDown", &MovementControl2::setGameActionDown );

        class_<MovementControlFPS, SmartPtr<MovementControlFPS>, bases<IComponent>, boost::noncopyable>(
            "MovementControlFPS" )
            .def( "getGameInputId", &MovementControlFPS::getGameInputId )
            .def( "setGameInputId", &MovementControlFPS::setGameInputId )

            //.def("getMoveSpeed", &MovementControlFPS::getMoveSpeed )
            //.def("setMoveSpeed", &MovementControlFPS::setMoveSpeed )

            .def( "getPosition", &MovementControlFPS::getPosition )
            .def( "setPosition", &MovementControlFPS::setPosition )

            .def( "getOrientation", &MovementControlFPS::getOrientation )
            .def( "getRotation", &MovementControlFPS::getRotation )

            .def( "getRotationOffset", &MovementControlFPS::getRotationOffset )
            .def( "setRotationOffset", &MovementControlFPS::setRotationOffset )

            .def( "getDirection", &MovementControlFPS::getDirection )
            .def( "setDirection", &MovementControlFPS::setDirection )

            .def( "getGameActionLeft", &MovementControlFPS::getGameActionLeft )
            .def( "setGameActionLeft", &MovementControlFPS::setGameActionLeft )
            .def( "getGameActionRight", &MovementControlFPS::getGameActionRight )
            .def( "setGameActionRight", &MovementControlFPS::setGameActionRight )

            .def( "getGameActionUp", &MovementControlFPS::getGameActionUp )
            .def( "setGameActionUp", &MovementControlFPS::setGameActionUp )
            .def( "getGameActionDown", &MovementControlFPS::getGameActionDown )
            .def( "setGameActionDown", &MovementControlFPS::setGameActionDown )

            .def( "getCameraSceneNode", &MovementControlFPS::getCameraSceneNode )
            .def( "setCameraSceneNode", &MovementControlFPS::setCameraSceneNode )

            .def( "getCharacterSceneNode", &MovementControlFPS::getCharacterSceneNode )
            .def( "setCharacterSceneNode", &MovementControlFPS::setCharacterSceneNode )

            .def( "getCharacter", &MovementControlFPS::getCharacter )
            .def( "setCharacter", &MovementControlFPS::setCharacter )

            .def( "getUserControlled", &MovementControlFPS::getUserControlled )
            .def( "setUserControlled", &MovementControlFPS::setUserControlled )

            .def( "getWeapon", &MovementControlFPS::getWeapon )
            .def( "setWeapon", &MovementControlFPS::setWeapon );

        class_<EntityMovement2, SmartPtr<EntityMovement2>, bases<IComponent>, boost::noncopyable>(
            "EntityMovement2" )
            .def( "updatePhysicsBody", &EntityMovement2::updatePhysicsBody )
            //.def("getBody", &EntityMovement2::getBody )
            //.def("setBody", &EntityMovement2::setBody )
            .def( "getMovementDirection", &EntityMovement2::getMovementDirection )
            .def( "setMovementDirection", &EntityMovement2::setMovementDirection )
            .def( "getDampVelocityState", &EntityMovement2::getDampVelocityState )
            .def( "setDampVelocityState", &EntityMovement2::setDampVelocityState )
            .def( "getSpeedMultiplier", &EntityMovement2::getSpeedMultiplier )
            .def( "setSpeedMultiplier", &EntityMovement2::setSpeedMultiplier )
            .def( "getSpeed", &EntityMovement2::getSpeed )
            .def( "setSpeed", &EntityMovement2::setSpeed )
            .def( "getAllowMovement", &EntityMovement2::getAllowMovement )
            .def( "setAllowMovement", &EntityMovement2::setAllowMovement )
            .def( "getStaticDamp", &EntityMovement2::getStaticDamp )
            .def( "setStaticDamp", &EntityMovement2::setStaticDamp )
            .def( "getDynamicDamp", &EntityMovement2::getDynamicDamp )
            .def( "setDynamicDamp", &EntityMovement2::setDynamicDamp );

        class_<SuperFreezeLogic, SmartPtr<SuperFreezeLogic>, bases<IComponent>, boost::noncopyable>(
            "SuperFreezeLogic" )
            .def( "isFrozen", &SuperFreezeLogic::isFrozen )
            .def( "setFrozen", &SuperFreezeLogic::setFrozen );

        class_<InputComponent, SmartPtr<InputComponent>, bases<IComponent>, boost::noncopyable>(
            "InputComponent" )
            .def( "getGameInputId", &InputComponent::getGameInputId )
            .def( "setGameInputId", &InputComponent::setGameInputId );

        //class_<ViewportComponent, ViewportSmartPtr<scene::IComponent>, bases<IComponent>, boost::noncopyable>(
        //    "ViewportConstraint" )
        //    .def( "setViewportId", &ViewportComponent::setViewportId )
        //    .def( "getViewportId", &ViewportComponent::getViewportId );

        //class_<EnergyContainer, EnergyContainerPtr, bases<IComponent>, boost::noncopyable>( "ViewportConstraint" )
        //    .def( "addEnergy", &EnergyContainer::addEnergy )
        //    .def( "setEnergy", &EnergyContainer::setEnergy )
        //    .def( "getEnergy", &EnergyContainer::getEnergy )

        //    .def( "setMaxEnergy", &EnergyContainer::setMaxEnergy )
        //    .def( "getMaxEnergy", &EnergyContainer::getMaxEnergy )
        //    .def( "isAlive", &EnergyContainer::isAlive );

        //class_<Grid2Container, Grid2ContainerPtr, bases<IComponent>, boost::noncopyable>( "Grid2Container" )
        //    .def( "isCellOccupied", &Grid2Container::isCellOccupied )
        //    .def( "setCellOccupied", &Grid2Container::setCellOccupied )
        //    .def( "getCellIndex", &Grid2Container::getCellIndex )
        //    .def( "getCellPosition", &Grid2Container::getCellPosition );

        //class_<MatchLogic, SmartPtr<MatchLogic>, bases<IComponent>, boost::noncopyable>( "MatchLogic" )
        //    .def( "getNumRoundsWon", &MatchLogic::getNumRoundsWon )
        //    .def( "setNumRoundsWon", &MatchLogic::setNumRoundsWon )

        //    .def( "getTotalRounds", &MatchLogic::getTotalRounds )
        //    .def( "setTotalRounds", &MatchLogic::setTotalRounds )

        //    .def( "getCurrentRound", &MatchLogic::getCurrentRound )
        //    .def( "setCurrentRound", &MatchLogic::setCurrentRound )

        //    .def( "setCurrentRoundState", &MatchLogic::setCurrentRoundState )
        //    .def( "getCurrentRoundState", &MatchLogic::getCurrentRoundState )

        //    .def( "getRoundEndState", &MatchLogic::getRoundEndState )
        //    .def( "setRoundEndState", &MatchLogic::setRoundEndState );

        class_<ComboLogic, SmartPtr<ComboLogic>, bases<IComponent>, boost::noncopyable>( "ComboLogic" )
            .def( "addComboCount", &ComboLogic::addComboCount )
            .def( "setComboCount", &ComboLogic::setComboCount )
            .def( "getComboCount", &ComboLogic::getComboCount )
            .def( "setComboFinishTime", &ComboLogic::setComboFinishTime )
            .def( "getComboFinishTime", &ComboLogic::getComboFinishTime );

        //class_<AnimatorContainer, AnimatorContainerPtr, bases<IComponent>>( "AnimatorContainer" )
        //    .def( "setAnimator", &AnimatorContainer::addAnimator );

        //class_<FragCounter, FragCounterPtr, bases<IComponent>>( "FragCounter" )
        //    .def( "addFrags", &FragCounter::addFrags )
        //    .def( "setFrags", &FragCounter::setFrags )
        //    .def( "getFrags", &FragCounter::getFrags );

        //class_<Playable, PlayablePtr, bases<IComponent>>( "Playable" )
        //    .def( "getPlayerNumber", &Playable::getPlayerNumber )
        //    .def( "setPlayerNumber", &Playable::setPlayerNumber )

        //    .def( "isUserControlled", &Playable::isUserControlled )
        //    .def( "setUserControlled", &Playable::setUserControlled );

        //class_<AiGoalBased, AiGoalBasedPtr, bases<IComponent>>( "AiGoalBased" )
        //    .def( "getGoal", &AiGoalBased::getGoal )
        //    .def( "setGoal", &AiGoalBased::setGoal )
        //    //.def("setGoal", _setGoal )

        //    .def( "getSteering", &AiGoalBased::getSteering )
        //    .def( "setSteering", &AiGoalBased::setSteering )

        //    .def( "getCharacter", &AiGoalBased::getCharacter )
        //    .def( "setCharacter", &AiGoalBased::setCharacter )

        //    .def( "getTargeting", &AiGoalBased::getTargeting )
        //    .def( "setTargeting", &AiGoalBased::setTargeting );

        //class_<ScreenSpacePosition, ScreenSpacePositionPtr, bases<IComponent>>(
        //    "ScreenSpacePosition" )
        //    .def( "getScreenPosition", &ScreenSpacePosition::getScreenPosition )
        //    .def( "setScreenPosition", &ScreenSpacePosition::setScreenPosition )

        //    .def( "getScreenPositionOffset", &ScreenSpacePosition::getScreenPositionOffset )
        //    .def( "setScreenPositionOffset", &ScreenSpacePosition::setScreenPositionOffset )

        //    .def( "getWorldPosition", &ScreenSpacePosition::getWorldPosition )

        //    .def( "getScreenDepth", &ScreenSpacePosition::getScreenDepth )
        //    .def( "setScreenDepth", &ScreenSpacePosition::setScreenDepth );

        //class_<ScreenSpaceRotation, ScreenSpaceRotationPtr, bases<IComponent>>(
        //    "ScreenSpaceRotation" );

        class_<TransformComponent, SmartPtr<TransformComponent>, bases<BaseComponent>,
               boost::noncopyable>( "Transform" )
            .def( "getPosition", &TransformComponent::getPosition )
            .def( "setPosition", &TransformComponent::setPosition )
            .def( "getOrientation", &TransformComponent::getOrientation )
            .def( "setOrientation", &TransformComponent::setOrientation );

        //class_<CollisionNode, SmartPtr<CollisionNode>, bases<IComponent>>( "CollisionNode" )
        //    //.def("getRootType", &CollisionNode::getRootType )

        //    //.def("getNodeId", &CollisionNode::getNodeId )
        //    //.def("_setNodeId", &CollisionNode::_setNodeId )

        //    //.def("getNodeType", &CollisionNode::getNodeType )
        //    //.def("setNodeType", &CollisionNode::setNodeType )

        //    //.def("hasParent", &CollisionNode::hasParent )
        //    ////.def("getParent", _getCollisionNodeParent )

        //    //.def("addChild", &CollisionNode::addChild )
        //    ////.def("addChild", _addChild )
        //    //.def("removeChild", &CollisionNode::removeChild )
        //    ////.def("removeChild", _removeChild )
        //    //.def("findChild", &CollisionNode::findChild )
        //    //.def("remove", &CollisionNode::remove )
        //    //.def("removeAllChildren", &CollisionNode::removeAllChildren )
        //    //.def("getChildren", &CollisionNode::getChildren )
        //    ;

        class_<Destructible, SmartPtr<Destructible>, bases<IComponent>, boost::noncopyable>(
            "Destructible" )
            //.def( "getScene", &Destructible::getScene )
            //.def( "setScene", &Destructible::setScene )
            .def( "reset", &Destructible::reset )
            .def( "destroy", &Destructible::destroy )

            .def( "createPhysics", &Destructible::createPhysics )

            .def( "getScale", &Destructible::getScale )
            .def( "setScale", &Destructible::setScale )

            //.def("setCollisionType", DestructibleHelper::setCollisionType )
            //.def("getCollisionType", DestructibleHelper::getCollisionType )
            //.def("setCollisionMask", DestructibleHelper::setCollisionMask )
            //.def("getCollisionMask", DestructibleHelper::getCollisionMask )

            .def( "getMaxLifeTime", &Destructible::getMaxLifeTime )
            .def( "setMaxLifeTime", &Destructible::setMaxLifeTime )

            .def( "isVisible", &Destructible::isVisible )
            .def( "setVisible", &Destructible::setVisible )

            .def( "add", &Destructible::add )
            .def( "remove", &Destructible::remove );

        class_<Collision, SmartPtr<Collision>, bases<IComponent>, boost::noncopyable>( "Collision" )
            .def( "getExtents", &Collision::getExtents )
            .def( "setExtents", &Collision::setExtents )

            .def( "getPosition", &Collision::getPosition )
            .def( "setPosition", &Collision::setPosition )

            .def( "getRadius", &Collision::getRadius )
            .def( "setRadius", &Collision::setRadius )

            .def( "getRigidBody", &Collision::getRigidBody )
            .def( "setRigidBody", &Collision::setRigidBody )

            .def( "getBoundingBox", &Collision::getBoundingBox );

        //class_<GraphicsContainer, GraphicsContainerPtr, bases<BaseComponent>, boost::noncopyable>(
        //    "GraphicsContainer", no_init )
        //    //.def("setObject", _setObjectSceneNode )
        //    //.def("setObject", _setObjectSceneNodeHash )
        //    //.def("setObject", _setObjectGfxObj )
        //    //.def("setObject", _setObjectGfxObjHash )
        //    //.def("setObject", _setObjectGfxMesh )
        //    //.def("setObject", _setObjectGfxMeshHash )
        //    //.def("setObject", _setObjectParticleSystem)
        //    //.def("setObject", _setObjectParticleSystemHash )
        //    //.def("setObject", _setObjectAnimationCtrl )
        //    //.def("setObject", _setObjectAnimationCtrlHash )
        //    //.def("setObject", _setObjectAnimationStateCtrl )
        //    //.def("setObject", _setObjectAnimationStateCtrlHash )
        //    //.def("getSceneNode", _getSceneNode )
        //    .def( "getSceneNode", &GraphicsContainer::getSceneNode )
        //    .def( "getGraphicsObject", &GraphicsContainer::getGraphicsObject )
        //    .def( "getAnimationController", &GraphicsContainer::getAnimationController )
        //    //.def("getAnimationStateControl", _getAnimationStateController )
        //    .def( "getAnimationStateControl", &GraphicsContainer::getAnimationStateController )
        //    //.def("getAnimationController", _getAnimationCtrl )
        //    //.def("getMesh", _getMesh )
        //    //.def("getMesh", _getMeshHash )
        //    ;

        //PythonHelper::registerPointer<IComponentContainer>();
        PythonHelper::registerPointer<IComponent>();
        PythonHelper::registerPointer<MovementControl2>();
        //PythonHelper::registerPointer<GraphicsContainer>();
    }

}  // end namespace fb
