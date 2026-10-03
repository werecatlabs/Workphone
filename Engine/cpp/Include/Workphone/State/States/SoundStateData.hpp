#ifndef SoundState_h__
#define SoundState_h__

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{

    class SoundStateData : public StateData
    {
    public:
        SoundStateData();
        ~SoundStateData() override;

        WP_CLASS_REGISTER_DECL;

        u32 flags = 0;
        f32 volume = 1.0f;
        f32 pan = 0.0f;
    };

}  // namespace workphone

#endif  // SoundState_h__
