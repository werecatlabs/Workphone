#ifndef IUIAnimationGraph_h__
#define IUIAnimationGraph_h__

#include <Workphone/Animation/AnimationGraph.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <mutex>

namespace workphone
{
    namespace ui
    {

        class WPCore_API IUIAnimationGraph : public IUIElement
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

            // Both tasks hold mutex while using the model. The renderer edits the draft
            // and queues requests. The Application task applies them to the Animator.
            struct Model
            {
                std::mutex mutex;
                animation::AnimationGraphDefinition edited;
                Array<animation::AnimationGraphClip> availableClips;
                Array<animation::AnimationGraphParameter> liveChanges;
                Playback playback;
                Action action = Action::None;
                String targetName;
                String status;
                size_t ambiguousClipCount = 0;
                bool selectionChanged = false;
                bool dirty = false;
                bool resetView = true;  // Consumed by the widget after loading a different draft.
            };

            IUIAnimationGraph();

            IUIAnimationGraph( u32 poolTypeId );

            ~IUIAnimationGraph() override;

            Model &getModel();

            WP_CLASS_REGISTER_DECL;

        private:
            Model m_model;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIAnimationGraph_h__
