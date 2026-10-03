#ifndef AnimationGraphWindow_h__
#define AnimationGraphWindow_h__

#include <EditorPrerequisites.hpp>
#include <ui/EditorWindow.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Animation/AnimationGraph.hpp>
#include <Workphone/Animation/AnimationGraphNodes.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUITreeCtrl.hpp>
#include <Workphone/Interface/UI/IUILabelSliderPair.hpp>
#include <Workphone/Interface/UI/IUILabelTextInputPair.hpp>
#include <Workphone/Interface/UI/IUILabelTogglePair.hpp>
#include <Workphone/Interface/UI/IUILabelDropdownPair.hpp>

namespace workphone::editor
{
    /**
     * @brief Data-driven animation graph editor window.
     *
     * Surfaces the full AnimationGraphDefinition (parameters, states, transitions and
     * Esoterica-derived pose nodes such as 1D blend spaces and selectors) to the editor with
     * live preview against the selected Animator. Edits are made against a local copy and
     * committed to the Animator with Apply, keeping authoring fully data-driven.
     */
    class AnimationGraphWindow : public EditorWindow
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

            SmartPtr<AnimationGraphWindow> getOwner() const;
            void setOwner( SmartPtr<AnimationGraphWindow> owner );

            WP_CLASS_REGISTER_DECL;

        private:
            AtomicWeakPtr<AnimationGraphWindow> m_owner;
        };

        AnimationGraphWindow();
        ~AnimationGraphWindow() override;

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
            Apply,
            ResetGraph,
            Play,
            Pause,
            Stop,
            StateTree,
            TransitionTree,
            PoseNodeTree,
            ParameterTree,
            AddState,
            RemoveState,
            SetInitial,
            AddTransition,
            RemoveTransition,
            AddCondition,
            RemoveCondition,
            AddPoseNode,
            RemovePoseNode,
            AddParameter,
            RemoveParameter,
            SelectedId,
            SelectedClipId,
            SelectedPoseNodeId,
            SelectedSpeed,
            SelectedDuration,
            SelectedExitTime,
            SelectedPriority,
            SelectedSync,
            SelectedFromState,
            SelectedToState,
            SelectedParameter,
            SelectedIsBool,
            SelectedBoolValue,
            SelectedThreshold,
            SelectedComparison,
            SelectedNodeType,
            SelectedChildren,
            SelectedBlendRanges,
            LiveParamValue,
            LiveParamBool
        };

        enum class SelectionKind
        {
            None,
            State,
            Transition,
            Condition,
            PoseNode,
            Parameter
        };

        struct AuthoredParameter
        {
            String name;
            bool isBool = false;
            f32 floatValue = 0.0f;
            bool boolValue = false;
        };

        SmartPtr<scene::Animator> findSelectedAnimator() const;
        void loadFromAnimator();
        void rebuildTrees();
        void syncEditor();
        void applyEditedGraph();
        void setStatus( const String &status );

        void addState();
        void removeState();
        void addTransition();
        void removeTransition();
        void addCondition();
        void removeCondition();
        void addPoseNode();
        void removePoseNode();
        void addParameter();
        void removeParameter();

        void onSelectionChanged( WidgetId source );
        void applyEditorFields();
        void applyLiveParameter();

        animation::AnimationGraphState *getSelectedState();
        animation::AnimationGraphTransition *getSelectedTransition();
        animation::AnimationGraphCondition *getSelectedCondition();
        animation::AnimationGraphPoseNodeDefinition *getSelectedPoseNode();
        AuthoredParameter *getSelectedParameter();

        const String comparisonLabel( animation::AnimationGraphComparison comparison ) const;
        const String nodeTypeLabel( animation::AnimationGraphPoseNodeType type ) const;

        SmartPtr<scene::Animator> m_animator;
        animation::AnimationGraphDefinition m_edited;
        Array<AuthoredParameter> m_parameters;
        SelectionKind m_selectionKind = SelectionKind::None;
        String m_selectedId;
        u32 m_selectedTransitionIndex = 0;
        u32 m_selectedConditionIndex = 0;

        SmartPtr<ui::IUIWindow> m_window;
        SmartPtr<IEventListener> m_windowListener;
        SmartPtr<ui::IUIText> m_statusText;
        SmartPtr<ui::IUITreeCtrl> m_stateTree;
        SmartPtr<ui::IUITreeCtrl> m_transitionTree;
        SmartPtr<ui::IUITreeCtrl> m_poseNodeTree;
        SmartPtr<ui::IUITreeCtrl> m_parameterTree;

        SmartPtr<ui::IUILabelTextInputPair> m_idText;
        SmartPtr<ui::IUILabelTextInputPair> m_clipIdText;
        SmartPtr<ui::IUILabelTextInputPair> m_poseNodeIdText;
        SmartPtr<ui::IUILabelSliderPair> m_speedSlider;
        SmartPtr<ui::IUILabelSliderPair> m_durationSlider;
        SmartPtr<ui::IUILabelSliderPair> m_exitTimeSlider;
        SmartPtr<ui::IUILabelSliderPair> m_prioritySlider;
        SmartPtr<ui::IUILabelSliderPair> m_thresholdSlider;
        SmartPtr<ui::IUILabelTextInputPair> m_parameterText;
        SmartPtr<ui::IUILabelTextInputPair> m_fromStateText;
        SmartPtr<ui::IUILabelTextInputPair> m_toStateText;
        SmartPtr<ui::IUILabelTogglePair> m_syncToggle;
        SmartPtr<ui::IUILabelTogglePair> m_isBoolToggle;
        SmartPtr<ui::IUILabelTogglePair> m_boolValueToggle;
        SmartPtr<ui::IUILabelDropdownPair> m_comparisonDropdown;
        SmartPtr<ui::IUILabelDropdownPair> m_nodeTypeDropdown;
        SmartPtr<ui::IUILabelTextInputPair> m_childrenText;
        SmartPtr<ui::IUILabelTextInputPair> m_blendRangesText;
        SmartPtr<ui::IUILabelSliderPair> m_liveParamValueSlider;
        SmartPtr<ui::IUILabelTogglePair> m_liveParamBoolToggle;
    };
}  // namespace workphone::editor

#endif  // AnimationGraphWindow_h__