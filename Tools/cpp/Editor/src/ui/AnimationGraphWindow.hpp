#ifndef AnimationGraphWindow_h__
#define AnimationGraphWindow_h__

#include <ui/EditorWindow.hpp>
#include <Workphone/Animation/AnimationGraph.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <array>
#include <mutex>
#include <optional>

namespace workphone::editor
{
    class AnimationGraphWindow : public EditorWindow
    {
    public:
        ~AnimationGraphWindow() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;

        WP_CLASS_REGISTER_DECL;

    private:
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

        SmartPtr<scene::Animator> findSelectedAnimator() const;
        void loadFromAnimator( SmartPtr<scene::Animator> animator );
        void applyGraph();
        void updatePlayback();

#if WP_BUILD_IMGUI
        void draw();
        void drawToolbar();
        void drawCanvas( f32 width, f32 height );
        void drawInspector( f32 width, f32 height );
        void drawStateInspector();
        void drawTransitionInspector();
        void drawParameters();
        void drawPoseNodes();
        void drawPoseNodeInspector();
        bool chooseClip( const char *label, String &id );
        bool chooseParameter( const char *label, String &name, std::optional<bool> isBoolean );
        bool chooseState( const char *label, String &id, const String &excluded );
        void addState( const String &clipId, const String &poseNodeId, Vector2F position );
        void addTransition( const String &from, const String &to );
        void drawAddStateMenu( Vector2F position );
        void selectState( const String &id );
        void selectTransition( size_t index );
        void fitCanvas( Vector2F size );
        void markEdited();
#endif

        // Scene and playback reads run on the Application task. The renderer only
        // reads snapshots and changes the draft or enqueues an action under this lock.
        std::mutex m_mutex;
        SmartPtr<ui::IUIWindow> m_window;
        SmartPtr<scene::Animator> m_animator;
        animation::AnimationGraphDefinition m_edited;
        Array<animation::AnimationGraphClip> m_availableClips;
        Array<animation::AnimationGraphParameter> m_liveChanges;
        Playback m_playback;
        Action m_action = Action::None;
        String m_targetName;
        String m_status;
        size_t m_ambiguousClipCount = 0;
        bool m_selectionChanged = false;
        bool m_dirty = false;
        bool m_graphLoadFailed = false;

        String m_selectedState;
        String m_selectedPoseNode;
        std::optional<animation::AnimationGraphPoseNodeDefinition> m_poseDraft;
        std::optional<size_t> m_selectedTransition;
        bool m_showInspector = false;
        std::array<char, 128> m_newParameterName{};
        bool m_newParameterIsBoolean = false;
        Vector2F m_camera = Vector2F::zero();
        f32 m_zoom = 1.0f;
        bool m_fitOnDraw = true;
        bool m_panning = false;
        String m_draggedState;
        String m_linkSource;
        Vector2F m_dragOrigin = Vector2F::zero();
        Vector2F m_dragStart = Vector2F::zero();
        Vector2F m_addPosition = Vector2F::zero();
    };
}  // namespace workphone::editor

#endif  // AnimationGraphWindow_h__
