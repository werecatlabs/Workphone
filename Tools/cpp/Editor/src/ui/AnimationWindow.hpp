#ifndef AnimationWindow_h__
#define AnimationWindow_h__

#include <EditorPrerequisites.hpp>
#include <ui/EditorWindow.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUILabelSliderPair.hpp>
#include <Workphone/Interface/UI/IUILabelTextInputPair.hpp>
#include <Workphone/Interface/UI/IUILabelTogglePair.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUITreeCtrl.hpp>
#include <Workphone/Interface/UI/IUIVector3.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>

namespace workphone::editor
{
    /** Native animation clip preview and two-bone IK constraint editor. */
    class AnimationWindow : public EditorWindow
    {
    public:
        class WindowListener : public IEventListener
        {
        public:
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments,
                                   SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object,
                                   SmartPtr<IEvent> event ) override;

            SmartPtr<AnimationWindow> getOwner() const;
            void setOwner( SmartPtr<AnimationWindow> owner );

            WP_CLASS_REGISTER_DECL;

        private:
            AtomicWeakPtr<AnimationWindow> m_owner;
        };

        AnimationWindow();
        ~AnimationWindow() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;
        void updateSelection() override;

        void refresh();

        WP_CLASS_REGISTER_DECL;

    protected:
        enum class WidgetId
        {
            None,
            Refresh,
            Play,
            Pause,
            Stop,
            Rewind,
            ClipTree,
            Time,
            Speed,
            Weight,
            Looping,
            ConstraintTree,
            AddConstraint,
            RemoveConstraint,
            ApplyConstraint,
            ConstraintId,
            EffectorBone,
            TargetPosition,
            TargetRotation,
            PoleTarget,
            ConstraintWeight,
            ChainRotationWeight,
            ConstraintEnabled,
            MatchTargetOrientation,
            PoseBlend
        };

        SmartPtr<scene::Animator> findSelectedAnimator() const;
        animation::TwoBoneIKConstraint *getSelectedConstraint();
        const animation::TwoBoneIKConstraint *getSelectedConstraint() const;
        void rebuildClipTree();
        void rebuildConstraintTree();
        void syncPlaybackControls();
        void syncConstraintControls();
        void selectClip();
        void selectConstraint();
        void applyPlaybackControls();
        void addConstraint();
        void removeConstraint();
        void applyConstraint();
        void setStatus( const String &status );

        SmartPtr<scene::Animator> m_animator;
        String m_selectedConstraintId;

        SmartPtr<ui::IUIWindow> m_window;
        SmartPtr<IEventListener> m_windowListener;
        SmartPtr<ui::IUIText> m_statusText;
        SmartPtr<ui::IUITreeCtrl> m_clipTree;
        SmartPtr<ui::IUITreeCtrl> m_constraintTree;

        SmartPtr<ui::IUILabelSliderPair> m_timeSlider;
        SmartPtr<ui::IUILabelSliderPair> m_speedSlider;
        SmartPtr<ui::IUILabelSliderPair> m_weightSlider;
        SmartPtr<ui::IUILabelTogglePair> m_loopingToggle;

        SmartPtr<ui::IUILabelTextInputPair> m_constraintIdText;
        SmartPtr<ui::IUILabelTextInputPair> m_effectorBoneText;
        SmartPtr<ui::IUIVector3> m_targetPosition;
        SmartPtr<ui::IUIVector3> m_targetRotation;
        SmartPtr<ui::IUIVector3> m_poleTarget;
        SmartPtr<ui::IUILabelSliderPair> m_constraintWeightSlider;
        SmartPtr<ui::IUILabelSliderPair> m_chainRotationWeightSlider;
        SmartPtr<ui::IUILabelTogglePair> m_constraintEnabledToggle;
        SmartPtr<ui::IUILabelTogglePair> m_matchTargetOrientationToggle;
        SmartPtr<ui::IUILabelTogglePair> m_poseBlendToggle;
    };
}  // namespace workphone::editor

#endif  // AnimationWindow_h__
