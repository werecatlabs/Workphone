#ifndef IUIAnimationGraph_h__
#define IUIAnimationGraph_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        class IUIAnimationGraph : public IUIElement
        {
        public:
            enum class Action
            {
                None,
                Apply,
                Revert,
                Play,
                Pause,
                Stop
            };

            struct Playback
            {
                using Parameter = animation::AnimationGraphParameter;
                String state;
                String destination;
                f32 progress = 0.0f;
                bool available = false;
                bool playing = false;
                bool paused = false;
                Array<Parameter> parameterValues;
                Array<animation::AnimationGraphSample> samples;
            };

            IUIAnimationGraph();

            IUIAnimationGraph( u32 poolTypeId );

            ~IUIAnimationGraph();

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIAnimationGraph_h__
