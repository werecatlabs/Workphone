#ifndef AiScene_h__
#define AiScene_h__

#include <Workphone/Interface/Ai/IAiScene.hpp>

namespace workphone
{
    class WPCore_API AiScene : public IAiScene
    {
    public:
        AiScene();
        ~AiScene() override;

        void addAgent( SmartPtr<IAiAgent> agent ) override;

        void removeAgent( SmartPtr<IAiAgent> agent ) override;

        Array<SmartPtr<IAiAgent>> getAgents() const override;

        SmartPtr<IAiTrack> getTrack() const override;

        Array<SmartPtr<IAiWaypoint>> getWaypoints() const override;

        SmartPtr<IAiWaypoint> findNearestWaypoint( const Vector3<real_Num> &position ) const override;

        void reset() override;

        WP_CLASS_REGISTER_DECL;

    private:
        Array<SmartPtr<IAiAgent>> m_agents;
        Array<SmartPtr<IAiWaypoint>> m_waypoints;
        SmartPtr<IAiTrack> m_track;
    };
}  // namespace workphone

#endif  // AiScene_h__
