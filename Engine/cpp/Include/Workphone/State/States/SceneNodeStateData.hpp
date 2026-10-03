#ifndef __SceneNodeState_h__
#define __SceneNodeState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    class WPCore_API SceneNodeStateData : public StateData
    {
    public:
        SceneNodeStateData();
        SceneNodeStateData( const SceneNodeStateData &state );

        bool operator==( SceneNodeStateData *other ) const;

        bool operator!=( SceneNodeStateData *other ) const;

        SmartPtr<IState> clone() const;

        WP_CLASS_REGISTER_DECL;

        Vector3<real_Num> lookAt;
        Vector3<real_Num> yawFixedAxis = Vector3<real_Num>::unitY();

        u32 id = 0;
        u32 flags = 0;
        s32 queueCount = 0;
        s32 tickCount = 0;

        bool yawFixed = false;
    };
}  // namespace workphone

#endif  // SceneNodeState_h__
