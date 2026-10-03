#ifndef CompositorState_h__
#define CompositorState_h__

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{

    class WPCore_API CompositorStateData : public StateData
    {
    public:
        CompositorStateData();
        ~CompositorStateData() override;

        WP_CLASS_REGISTER_DECL;

        bool enabled = false;
    };

}  // namespace workphone

#endif  // CompositorState_h__
