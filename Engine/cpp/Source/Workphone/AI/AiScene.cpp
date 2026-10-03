#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/AI/AiScene.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Ai/IAiAgent.hpp>
#include <Workphone/Interface/Ai/IAiWaypoint.hpp>
#include <Workphone/Interface/Ai/IAiTrack.hpp>
#include <algorithm>
#include <limits>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, AiScene, IAiScene );

    AiScene::AiScene() = default;

    AiScene::~AiScene() = default;

    void AiScene::addAgent( SmartPtr<IAiAgent> agent )
    {
        if( agent && std::find( m_agents.begin(), m_agents.end(), agent ) == m_agents.end() )
        {
            m_agents.push_back( agent );
        }
    }

    void AiScene::removeAgent( SmartPtr<IAiAgent> agent )
    {
        m_agents.erase( std::remove( m_agents.begin(), m_agents.end(), agent ), m_agents.end() );
    }

    Array<SmartPtr<IAiAgent>> AiScene::getAgents() const
    {
        return m_agents;
    }

    SmartPtr<IAiTrack> AiScene::getTrack() const
    {
        return m_track;
    }

    Array<SmartPtr<IAiWaypoint>> AiScene::getWaypoints() const
    {
        return m_waypoints;
    }

    SmartPtr<IAiWaypoint> AiScene::findNearestWaypoint( const Vector3<real_Num> &position ) const
    {
        SmartPtr<IAiWaypoint> nearestWaypoint;
        auto nearestDistance = std::numeric_limits<real_Num>::max();

        for( const auto &waypoint : m_waypoints )
        {
            if( !waypoint )
            {
                continue;
            }

            auto properties = waypoint->getProperties();
            if( !properties )
            {
                continue;
            }

            Vector3<real_Num> waypointPosition;
            if( !properties->getPropertyValue( "position", waypointPosition ) &&
                !properties->getPropertyValue( "Position", waypointPosition ) )
            {
                continue;
            }

            const auto distance = Vector3<real_Num>::distance( position, waypointPosition );
            if( distance < nearestDistance )
            {
                nearestDistance = distance;
                nearestWaypoint = waypoint;
            }
        }

        return nearestWaypoint;
    }

    void AiScene::reset()
    {
        m_agents.clear();
        m_waypoints.clear();

        if( m_track )
        {
            m_track->clear();
        }

        m_track = nullptr;
    }

}  // namespace workphone
