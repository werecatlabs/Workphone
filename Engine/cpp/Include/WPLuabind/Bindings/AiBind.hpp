#ifndef AiBind_h__
#define AiBind_h__

#include "WPLuabind/WPLuabindPrerequisites.hpp"
#include <Workphone/Interface/Ai/IAi.hpp>
#include <Workphone/Interface/Ai/IAiCompositeGoal.hpp>
#include <Workphone/Interface/Ai/IAiGoal.hpp>
#include <Workphone/Interface/Ai/IAiGoalEvaluator.hpp>
#include <Workphone/Interface/Ai/IAiManager.hpp>
#include <Workphone/Interface/Ai/IAiScene.hpp>
#include <Workphone/Interface/Ai/IAiSensoryMemory.hpp>
#include <Workphone/Interface/Ai/IAiSteering3.hpp>
#include <Workphone/Interface/Ai/IAiTargeting3.hpp>
#include <Workphone/Interface/Ai/IAiTargetingSystem.hpp>
#include <Workphone/Interface/Ai/IAiTrack.hpp>
#include <Workphone/Interface/Ai/IAiTrackElement.hpp>
#include <Workphone/Interface/Ai/ILearning.hpp>
#include <Workphone/Interface/Ai/INeuralNetwork.hpp>
#include <Workphone/Interface/Ai/IPathfinder2.hpp>
#include <Workphone/Interface/Ai/IPathfinder3.hpp>
#include <Workphone/Interface/Ai/IPathNode2.hpp>
#include <Workphone/Interface/Ai/IPathNode3.hpp>
#include <Workphone/Interface/Ai/IVehicleAi.hpp>
#include <Workphone/Interface/Ai/IVehicleAiManager.hpp>

namespace workphone
{
    void bindAi( lua_State *L );
}

#endif // AiBind_h__
