#ifndef WPVehiclePhysicsHelicopterStep_h__
#define WPVehiclePhysicsHelicopterStep_h__

#include <Workphone/Workphone.hpp>
#include <cmath>

namespace workphone::vehicle::helicopter_step
{
    inline bool frame( double &time, double &dt )
    {
        auto app = core::IApplicationManager::instancePtr();
        if( !app || !app->isPlaying() || app->isPaused() || Thread::getCurrentTask() == TaskId::Render )
            return false;
        const auto manager = app->getGameManager();
        const auto task = Thread::getCurrentTask();
        if( task != TaskId::Physics && task != ( manager ? manager->getSceneTask() : TaskId::Application ) )
            return false;
        auto timer = app->getTimerPtr();
        if( !timer ) return false;
        time = timer->getTime(); dt = timer->getDeltaTime();
        if( !std::isfinite( time ) || !std::isfinite( dt ) || dt <= 0 ) return false;
        dt = Math<double>::min( dt, .05 );
        return true;
    }
}
#endif
