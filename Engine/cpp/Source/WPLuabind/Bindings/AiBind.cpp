#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/AiBind.hpp"
#include <luabind/luabind.hpp>
#include "WPLuabind/SmartPtrConverter.hpp"
#include <Workphone/Workphone.hpp>
#include "WPLuabind/WPLuabindTypes.hpp"
#include <luabind/operator.hpp>
#include "WPLuabind/ParamConverter.hpp"

namespace workphone
{
    void bindAi( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<IAi, ISharedObject, SmartPtr<IAi>>( "IAi" )
                        .def( "setFlag", &IAi::setFlag )
                        .def( "getFlag", &IAi::getFlag )
                        .def( "setDebugFlag", &IAi::setDebugFlag )
                        .def( "getDebugFlag", &IAi::getDebugFlag )
                        .def( "setLogFlag", &IAi::setLogFlag )
                        .def( "getLogFlag", &IAi::getLogFlag )];

        module( L )[class_<IAiGoal, ISharedObject, SmartPtr<IAiGoal>>( "IAiGoal" )
                        .def( "start", &IAiGoal::start )
                        .def( "finish", &IAiGoal::finish )
                        .def( "setState", &IAiGoal::setState )
                        .def( "getState", &IAiGoal::getState )
                        .def( "getType", &IAiGoal::getType )
                        .def( "setType", &IAiGoal::setType )
                        .def( "getFSM", &IAiGoal::getFSM )
                        .def( "setFSM", &IAiGoal::setFSM )];

        module( L )[class_<IAiCompositeGoal, IAiGoal, SmartPtr<IAiCompositeGoal>>( "IAiCompositeGoal" )
                        .def( "addGoal", &IAiCompositeGoal::addGoal )
                        .def( "removeGoal", &IAiCompositeGoal::removeGoal )
                        .def( "getGoals", &IAiCompositeGoal::getGoals )
                        .def( "removeAllGoals", &IAiCompositeGoal::removeAllGoals )
                        .def( "removeGoalsByType", &IAiCompositeGoal::removeGoalsByType )
                        .def( "hasGoalType", &IAiCompositeGoal::hasGoalType )
                        .def( "addGoalEvaluator", &IAiCompositeGoal::addGoalEvaluator )
                        .def( "removeGoalEvaluator", &IAiCompositeGoal::removeGoalEvaluator )
                        .def( "getEvaluators", &IAiCompositeGoal::getEvaluators )
                        .def( "hasGoals", &IAiCompositeGoal::hasGoals )];

        module(
            L )[class_<IAiGoalEvaluator, ISharedObject, SmartPtr<IAiGoalEvaluator>>( "IAiGoalEvaluator" )
                    .def( "activateGoal", &IAiGoalEvaluator::activateGoal )
                    .def( "getRating", &IAiGoalEvaluator::getRating )
                    .def( "getOwner", &IAiGoalEvaluator::getOwner )
                    .def( "setOwner", &IAiGoalEvaluator::setOwner )
                    .def( "getBias", &IAiGoalEvaluator::getBias )
                    .def( "setBias", &IAiGoalEvaluator::setBias )];

        module( L )[class_<IAiManager, ISharedObject, SmartPtr<IAiManager>>( "IAiManager" )
                        .def( "getPathfinder2", &IAiManager::getPathfinder2 )
                        .def( "setPathfinder2", &IAiManager::setPathfinder2 )
                        .def( "query", static_cast<String ( IAiManager::* )( const String & ) const>(
                                           &IAiManager::query ) )
                        .def( "processResponse", &IAiManager::processResponse )];

        module( L )[class_<IAiScene, ISharedObject, SmartPtr<IAiScene>>( "IAiScene" )
                        .def( "getTrack", &IAiScene::getTrack )
                        .def( "reset", &IAiScene::reset )];

        module(
            L )[class_<IAiSensoryMemory, ISharedObject, SmartPtr<IAiSensoryMemory>>( "IAiSensoryMemory" )
                    .def( "updateMemory", &IAiSensoryMemory::updateMemory )
                    .def( "addMemory", &IAiSensoryMemory::addMemory )
                    .def( "removeMemory", &IAiSensoryMemory::removeMemory )];

        module( L )[class_<IAiSteering3, ISharedObject, SmartPtr<IAiSteering3>>( "IAiSteering3" )
                        .def( "calculate", &IAiSteering3::calculate )
                        .def( "forwardComponent", &IAiSteering3::forwardComponent )
                        .def( "sideComponent", &IAiSteering3::sideComponent )
                        .def( "getTarget", &IAiSteering3::getTarget )
                        .def( "setTarget", &IAiSteering3::setTarget )
                        .def( "getDirection", &IAiSteering3::getDirection )
                        .def( "getPosition", &IAiSteering3::getPosition )
                        .def( "setPosition", &IAiSteering3::setPosition )
                        .def( "setTargetAgent1", &IAiSteering3::setTargetAgent1 )
                        .def( "setTargetAgent2", &IAiSteering3::setTargetAgent2 )
                        .def( "getForce", &IAiSteering3::getForce )
                        .def( "setSummingMethod", &IAiSteering3::setSummingMethod )
                        .def( "seekOn", &IAiSteering3::seekOn )
                        .def( "arriveOn", &IAiSteering3::arriveOn )
                        .def( "wanderOn", &IAiSteering3::wanderOn )
                        .def( "separationOn", &IAiSteering3::separationOn )
                        .def( "wallAvoidanceOn", &IAiSteering3::wallAvoidanceOn )
                        .def( "seekOff", &IAiSteering3::seekOff )
                        .def( "arriveOff", &IAiSteering3::arriveOff )
                        .def( "wanderOff", &IAiSteering3::wanderOff )
                        .def( "separationOff", &IAiSteering3::separationOff )
                        .def( "wallAvoidanceOff", &IAiSteering3::wallAvoidanceOff )
                        .def( "getSeekIsOn", &IAiSteering3::getSeekIsOn )
                        .def( "getArriveIsOn", &IAiSteering3::getArriveIsOn )
                        .def( "getWanderIsOn", &IAiSteering3::getWanderIsOn )
                        .def( "getSeparationIsOn", &IAiSteering3::getSeparationIsOn )
                        .def( "getWallAvoidanceIsOn", &IAiSteering3::getWallAvoidanceIsOn )
                        .def( "getFeelers", &IAiSteering3::getFeelers )
                        .def( "getWanderJitter", &IAiSteering3::getWanderJitter )
                        .def( "getWanderDistance", &IAiSteering3::getWanderDistance )
                        .def( "getWanderRadius", &IAiSteering3::getWanderRadius )
                        .def( "getSeparationWeight", &IAiSteering3::getSeparationWeight )];

        module( L )[class_<IAiTargeting3, ISharedObject, SmartPtr<IAiTargeting3>>( "IAiTargeting3" )
                        .def( "getTargetPosition", &IAiTargeting3::getTargetPosition )
                        .def( "setTargetPosition", &IAiTargeting3::setTargetPosition )
                        .def( "getOwner", &IAiTargeting3::getOwner )
                        .def( "setOwner", &IAiTargeting3::setOwner )
                        .def( "getTarget", &IAiTargeting3::getTarget )
                        .def( "setTarget", &IAiTargeting3::setTarget )];

        module( L )[class_<IAiTargetingSystem, ISharedObject, SmartPtr<IAiTargetingSystem>>(
            "IAiTargetingSystem" )];

        module( L )[class_<IAiTrack, ISharedObject, SmartPtr<IAiTrack>>( "IAiTrack" )
                        .def( "addElement", &IAiTrack::addElement )
                        .def( "removeElement", &IAiTrack::removeElement )
                        .def( "getElement", &IAiTrack::getElement )
                        .def( "getNumElements", &IAiTrack::getNumElements )
                        .def( "clear", &IAiTrack::clear )
                        .def( "load", &IAiTrack::load )
                        .def( "save", &IAiTrack::save )];

        module(
            L )[class_<IAiTrackElement, ISharedObject, SmartPtr<IAiTrackElement>>( "IAiTrackElement" )
                    .def( "getCenter", &IAiTrackElement::getCenter )
                    .def( "setCenter", &IAiTrackElement::setCenter )
                    .def( "getStart", &IAiTrackElement::getStart )
                    .def( "setStart", &IAiTrackElement::setStart )
                    .def( "getEnd", &IAiTrackElement::getEnd )
                    .def( "setEnd", &IAiTrackElement::setEnd )
                    .def( "getDirection", &IAiTrackElement::getDirection )
                    .def( "setDirection", &IAiTrackElement::setDirection )
                    .def( "getExtents", &IAiTrackElement::getExtents )
                    .def( "setExtents", &IAiTrackElement::setExtents )];

        module( L )[class_<ILearning, ISharedObject, SmartPtr<ILearning>>( "ILearning" )
                        .def( "save", &ILearning::save )
                        .def( "load", &ILearning::load )
                        .def( "reset", &ILearning::reset )];

        module( L )[class_<INeuralNetwork, ISharedObject, SmartPtr<INeuralNetwork>>( "INeuralNetwork" )
                        .def( "save", &INeuralNetwork::save )
                        .def( "load", &INeuralNetwork::load )];

        module( L )[class_<IPathfinder2, ISharedObject, SmartPtr<IPathfinder2>>( "IPathfinder2" )
                        .def( "setGoal", &IPathfinder2::setGoal )
                        .def( "searchStep", &IPathfinder2::searchStep )];

        module( L )[class_<IPathfinder3, ISharedObject, SmartPtr<IPathfinder3>>( "IPathfinder3" )];

        module( L )[class_<IPathNode2, ISharedObject, SmartPtr<IPathNode2>>( "IPathNode2" )
                        .def( "getPosition", &IPathNode2::getPosition )
                        .def( "setPosition", &IPathNode2::setPosition )
                        .def( "getCost", &IPathNode2::getCost )];

        module( L )[class_<IPathNode3, ISharedObject, SmartPtr<IPathNode3>>( "IPathNode3" )];

        module( L )[class_<IVehicleAi, ISharedObject, SmartPtr<IVehicleAi>>( "IVehicleAi" )
                        .def( "getVehicleController", &IVehicleAi::getVehicleController )
                        .def( "setVehicleController", &IVehicleAi::setVehicleController )
                        .def( "getVehicleManager", &IVehicleAi::getVehicleManager )
                        .def( "setVehicleManager", &IVehicleAi::setVehicleManager )];

        module( L )[class_<IVehicleAiManager, ISharedObject, SmartPtr<IVehicleAiManager>>(
                        "IVehicleAiManager" )
                        .def( "addVehicle", &IVehicleAiManager::addVehicle )
                        .def( "removeVehicle", &IVehicleAiManager::removeVehicle )
                        .def( "getVehicles", &IVehicleAiManager::getVehicles )
                        .def( "setVehicles", &IVehicleAiManager::setVehicles )];

        {
            object g = globals( L );
            object table = g["IAiGoal"];
            table["None"] = static_cast<u32>( IAiGoal::State::None );
            table["Ready"] = static_cast<u32>( IAiGoal::State::Ready );
            table["Executing"] = static_cast<u32>( IAiGoal::State::Executing );
            table["Finished"] = static_cast<u32>( IAiGoal::State::Finished );
            table["Failed"] = static_cast<u32>( IAiGoal::State::Failed );
            table["Count"] = static_cast<u32>( IAiGoal::State::Count );
        }
    }
} // namespace workphone
