#pragma once
#include <ui/EditorWindow.hpp>
#include <commands/Command.hpp>
#include <Workphone/Interface/Procedural/CityRoadSplineSource.hpp>
#include <Workphone/Interface/UI/IUILabelTextInputPair.hpp>
#include <Workphone/Interface/UI/IUILabelTogglePair.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone::scene { class CityRoadAuthoring; class TerrainSystem; }
namespace workphone::editor
{
    // One source transaction per completed draw/drag. History never stores preview actors.
    class CityRoadEditCommand : public Command
    {
    public:
        CityRoadEditCommand();
        bool configure(SmartPtr<scene::CityRoadAuthoring>, const String &after, String &error);
        void execute() override;
        void undo() override;
        void redo() override;
        bool applied() const { return m_applied; }
        const String &error() const { return m_error; }
        static bool dispatchHistory(SmartPtr<ICommandManager>,bool redo,String &error);
        WP_CLASS_REGISTER_DECL;
    private:
        bool apply(const SourceBudgetText &,const SourceBudgetText &expected);
        AtomicWeakPtr<scene::CityRoadAuthoring> m_target;
        AtomicWeakPtr<scene::IGameScene> m_scene;
        SourceBudgetText m_before,m_after;
        String m_project,m_error;
        bool m_applied=false;
    };

    class CityRoadWindow : public EditorWindow
    {
    public:
        class Listener : public IEventListener
        {
        public:
            Parameter handleEvent(EventType,hash_type,const Array<Parameter>&,SmartPtr<ISharedObject>,SmartPtr<ISharedObject>,SmartPtr<IEvent>) override;
            void setOwner(SmartPtr<CityRoadWindow> owner) { m_owner=owner; }
            WP_CLASS_REGISTER_DECL;
        private: AtomicWeakPtr<CityRoadWindow> m_owner;
        };
        class ViewportJob : public Job
        {
        public:
            void execute() override;
            void setOwner(SmartPtr<CityRoadWindow> owner){m_owner=owner;}
            WP_CLASS_REGISTER_DECL;
        private: AtomicWeakPtr<CityRoadWindow> m_owner;
        };
        CityRoadWindow();
        ~CityRoadWindow() override;
        void load(SmartPtr<ISharedObject>) override;
        void unload(SmartPtr<ISharedObject>) override;
        void setWindowVisible(bool) override;
        void updateSelection() override;
        Parameter handleViewport(const Array<Parameter>&);
        WP_CLASS_REGISTER_DECL;
    private:
        enum Widget { Bind=43001,Draw,Move,InHandle,OutHandle,Finish,Cancel,ApplyFields,RemoveRoad,Undo,Redo,Save,Width,Grade,Closed,Roundabout,OuterRadius,IslandRadius,RemoveRoundabout };
        enum class Mode { Draw,Move,InHandle,OutHandle,Roundabout };
        void attachViewport();
        void action(Widget,bool invalidateInput=true);
        void status(const String &);
        void cancelDraft();
        bool beginDraft();
        bool beginDraw();
        Vector3F controlPoint(Vector3F,const procedural::CityRoadSpline &) const;
        bool fields(procedural::CityRoadSpline &);
        bool commitDraft();
        bool bindSelection();
        bool addRoundabout(Vector3F center);
        void history(bool redo);
        void drainViewport();
        Parameter processViewport(const Array<Parameter>&,bool queued=false);
        Parameter preview() const;
        SmartPtr<scene::CityRoadAuthoring> current() const;
        SmartPtr<Listener> m_listener;
        AtomicWeakPtr<ui::IUIRenderWindow> m_viewport;
        AtomicWeakPtr<scene::CityRoadAuthoring> m_authoring;
        AtomicWeakPtr<scene::IGameScene> m_scene;
        SmartPtr<ui::IUILabelTextInputPair> m_width,m_grade;
        SmartPtr<ui::IUILabelTextInputPair> m_outerRadius,m_islandRadius;
        SmartPtr<ui::IUILabelTogglePair> m_closed,m_clockwise;
        SmartPtr<ui::IUIText> m_status;
        procedural::CityRoadSplineSource m_draft;
        SourceBudgetText m_selectedRoadId,m_selectedKnotId;
        SourceBudgetSet<SourceBudgetText> m_manualHandles;
        u64 m_draftRevision=0;
        size_t m_road=0,m_knot=0;
        Mode m_mode=Mode::Move;
        bool m_enabled=false,m_drafting=false,m_dragging=false,m_selected=false;
        struct Input { SourceBudgetText phase; Vector3F origin{},direction{};u64 epoch=0; };
        mutable RecursiveMutex m_toolMutex;
        SourceBudgetVector<Input> m_inputs;
        Parameter m_cachedPreview;
        u64 m_toolEpoch=0;
        bool m_dispatchPending=false,m_cachedEnabled=false;
        u64 m_selectionSignature=0;
        bool m_hasSelectionSignature=false;
    };
}
