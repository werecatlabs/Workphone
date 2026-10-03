#ifndef SoundManagerState_h__
#define SoundManagerState_h__

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{
    class SoundManagerStateData : public StateData
    {
    public:
        SoundManagerStateData();
        ~SoundManagerStateData() override;

        WP_CLASS_REGISTER_DECL;

        u32 flags = 0;
        f32 volume = 1.0f;
    };
}  // namespace workphone

#endif  // SoundManagerState_h__
