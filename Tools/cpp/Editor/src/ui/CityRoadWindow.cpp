#include <EditorPCH.hpp>
#include <ui/CityRoadWindow.hpp>
#include <ui/UIManager.hpp>
#include <editor/EditorManager.hpp>
#include <jobs/SaveSceneJob.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED(workphone::editor,CityRoadWindow,EditorWindow);
    WP_CLASS_REGISTER_DERIVED(workphone::editor,CityRoadWindow::Listener,IEventListener);
    WP_CLASS_REGISTER_DERIVED(workphone::editor,CityRoadWindow::ViewportJob,Job);
    WP_CLASS_REGISTER_DERIVED(workphone::editor,CityRoadEditCommand,Command);
    namespace
    {
        SourceBudgetText newId()
        {
            const auto value=StringUtil::getUUID();SourceBudgetText id(value.data(),value.size());
            std::transform(id.begin(),id.end(),id.begin(),[](unsigned char c){return char(std::tolower(c));});return id;
        }
        SmartPtr<scene::IGameScene> activeScene()
        {auto app=core::IApplicationManager::instancePtr();auto manager=app?app->getGameManager():nullptr;return manager?manager->getCurrentScene():nullptr;}
        String json(const procedural::CityRoadSplineSource &source,String &error)
        {SourceBudgetText bytes;if(!procedural::serializeCityRoadSplineSource(source,bytes,error))return {};return String(bytes.data(),bytes.size());}
        bool sameAuthoredState(const String &current,const SourceBudgetText &expected,String &error)
        {
            procedural::CityRoadSplineSource a,b;
            if(!procedural::deserializeCityRoadSplineSource(std::string_view(current.data(),current.size()),a,error) ||
               !procedural::deserializeCityRoadSplineSource(std::string_view(expected.data(),expected.size()),b,error))return false;
            a.revision=b.revision=0;a.terrainRevision=b.terrainRevision=0;SourceBudgetText left,right;
            if(!procedural::serializeCityRoadSplineSource(a,left,error) || !procedural::serializeCityRoadSplineSource(b,right,error))return false;
            if(left!=right){error="Road source changed outside this history step; undo/redo was rejected.";return false;}return true;
        }
    }
    CityRoadEditCommand::CityRoadEditCommand(){setPrimary(true);}
    bool CityRoadEditCommand::configure(SmartPtr<scene::CityRoadAuthoring> target,const String &after,String &error)
    {
        auto app=core::IApplicationManager::instancePtr();auto scene=activeScene();
        if(!target || !scene || !target->getActor() || target->getActor()->getScene()!=scene)
        {error="Road command target is not in the active scene.";return false;}
        m_target=target;m_scene=scene;const auto before=target->getSourceJSON();m_before.assign(before.data(),before.size());m_after.assign(after.data(),after.size());
        m_project=app->getProjectPath();error.clear();return true;
    }
    bool CityRoadEditCommand::apply(const SourceBudgetText &value,const SourceBudgetText &expected)
    {
        if(Thread::getCurrentTask()!=TaskId::Application){m_error="Road publication requires the Application task.";return false;}
        auto app=core::IApplicationManager::instancePtr();auto target=m_target.load().lock();auto scene=m_scene.load().lock();
        if(!app || app->getProjectPath()!=m_project || !scene || activeScene()!=scene || !target ||
           !target->getActor() || target->getActor()->getScene()!=scene)
        {m_error="Road history belongs to a closed scene or project.";return false;}
        // Each successful history step increments the source revision. Compare authored
        // state, then CAS the current token so A/B edits can undo and redo in sequence.
        const auto revision=target->getSourceRevision();
        if(!sameAuthoredState(target->getSourceJSON(),expected,m_error)){WP_LOG_ERROR(m_error);return false;}
        if(!target->setSourceJSON(String(value.data(),value.size()),revision,m_error)){WP_LOG_ERROR(m_error);return false;}
        target->getActor()->setDirty(true);return true;
    }
    void CityRoadEditCommand::execute(){if(!m_applied)m_applied=apply(m_after,m_before);setState(State::Finished);}
    void CityRoadEditCommand::undo(){if(m_applied && apply(m_before,m_after))m_applied=false;}
    void CityRoadEditCommand::redo(){execute();}
    bool CityRoadEditCommand::dispatchHistory(SmartPtr<ICommandManager> manager,bool redo,String &error)
    {
        if(Thread::getCurrentTask()!=TaskId::Application)
        {
            class HistoryJob : public Job
            {
            public:
                SmartPtr<ICommandManager> manager;bool redo=false;
                void execute() override{String error;CityRoadEditCommand::dispatchHistory(manager,redo,error);if(!error.empty())WP_LOG_ERROR(error);}
            };
            auto app=core::IApplicationManager::instancePtr();auto tasks=app?app->getTaskManager():nullptr;auto owner=tasks?tasks->getTask(TaskId::Application):nullptr;
            if(!manager || !owner || !owner->isLoaded() || owner->getState()==ITask::State::Shutdown){error="Application history task is unavailable.";return false;}
            auto job=make_ptr<HistoryJob>();job->manager=manager;job->redo=redo;job->setPrimary(true);owner->addJob(job);error.clear();return true;
        }
        error.clear();if(!manager)return false;auto command=redo?manager->getNextCommand():manager->getPreviousCommand();if(!command)return false;
        auto road=dynamic_pointer_cast<CityRoadEditCommand>(command);const bool before=road && road->applied();
        if(redo)command->redo();else command->undo();
        if(road && road->applied()==before)
        {
            error=road->error();auto restored=redo?manager->getPreviousCommand():manager->getNextCommand();
            if(restored!=command){if(redo)manager->getNextCommand();else manager->getPreviousCommand();error+=" Command history changed during rejection.";}
            if(error.empty())error="The road history step was rejected without changing the source.";return false;
        }
        return true;
    }

    CityRoadWindow::CityRoadWindow()=default;
    CityRoadWindow::~CityRoadWindow(){unload(nullptr);}
    void CityRoadWindow::load(SmartPtr<ISharedObject>)
    {
        setLoadingState(LoadingState::Loading);auto app=core::IApplicationManager::instancePtr();auto ui=app?app->getUI():nullptr;
        if(!ui){setLoadingState(LoadingState::Unloaded);return;}
        m_listener=make_ptr<Listener>();m_listener->setOwner(this);
        auto window=ui->addElementByType<ui::IUIWindow>();window->setLabel("City Roads");window->setSize({440,520});setParentWindow(window);
        const auto button=[&](const String &label,Widget id,bool same=false){auto b=ui->addElementByType<ui::IUIButton>();b->setLabel(label);b->setElementId(id);b->setSameLine(same);b->addObjectListener(m_listener);window->addChild(b);};
        const auto text=[&](const String &label){auto t=ui->addElementByType<ui::IUIText>();t->setText(label);window->addChild(t);return t;};
        text("Select terrain, then Bind. Source is saved with the scene.");button("Bind Selected Terrain",Bind);
        button("Draw Road",Draw);button("Move Knot",Move,true);button("In Handle",InHandle,true);button("Out Handle",OutHandle,true);
        text("Click adds a knot; drag sets handles. Enter commits; Escape cancels.");
        const auto input=[&](const String &label,Widget id,const String &value){auto p=ui->addElementByType<ui::IUILabelTextInputPair>();p->setLabel(label);p->setValue(value);p->setElementId(id);window->addChild(p);return p;};
        m_width=input("Road width (m)",Width,"12");m_grade=input("Maximum grade (rise/run)",Grade,"0.08");
        m_closed=ui->addElementByType<ui::IUILabelTogglePair>();m_closed->setLabel("Closed spline");m_closed->setValue(false);window->addChild(m_closed);
        m_outerRadius=input("Roundabout outer radius (m)",OuterRadius,"14");m_islandRadius=input("Island radius (m)",IslandRadius,"8");
        m_clockwise=ui->addElementByType<ui::IUILabelTogglePair>();m_clockwise->setLabel("Clockwise circulation");m_clockwise->setValue(true);window->addChild(m_clockwise);
        button("Place Roundabout (4 approaches)",Roundabout);
        button("Delete Selected Roundabout",RemoveRoundabout);
        button("Apply Road Fields",ApplyFields);button("Commit Road",Finish,true);button("Cancel Draft",Cancel,true);
        button("Delete Selected Road",RemoveRoad);button("Undo",Undo);button("Redo",Redo,true);button("Save Scene",Save,true);
        m_status=text("Select a terrain or city road component to begin.");attachViewport();setLoadingState(LoadingState::Loaded);
    }
    void CityRoadWindow::attachViewport()
    {
        auto app=core::IApplicationManager::instancePtr();auto viewport=app?app->getSceneRenderWindow():nullptr;
        auto previous=m_viewport.load().lock();if(previous==viewport)return;
        cancelDraft();if(previous && m_listener)previous->removeObjectListener(m_listener);m_viewport=viewport;
        if(viewport && m_listener)viewport->addObjectListener(m_listener);
    }
    void CityRoadWindow::unload(SmartPtr<ISharedObject> data)
    {
        {RecursiveMutex::ScopedLock lock(m_toolMutex);++m_toolEpoch;m_inputs.clear();m_dispatchPending=false;m_cachedEnabled=false;m_cachedPreview={};}
        if(!m_listener && !getParentWindow())return;cancelDraft();m_enabled=false;
        if(auto viewport=m_viewport.load().lock())if(m_listener)viewport->removeObjectListener(m_listener);
        m_viewport=nullptr;if(m_listener)m_listener->setOwner(nullptr);m_listener=nullptr;
        auto app=core::IApplicationManager::instancePtr();auto ui=app?app->getUI():nullptr;
        if(auto window=getParentWindow()){if(ui){for(auto child:window->getChildren())ui->removeElement(child);ui->removeElement(window);}setParentWindow(nullptr);}
        m_width=nullptr;m_grade=nullptr;m_closed=nullptr;m_outerRadius=nullptr;m_islandRadius=nullptr;m_clockwise=nullptr;m_status=nullptr;m_authoring=nullptr;m_scene=nullptr;EditorWindow::unload(data);
    }
    void CityRoadWindow::setWindowVisible(bool visible)
    {if(!visible){{RecursiveMutex::ScopedLock lock(m_toolMutex);++m_toolEpoch;m_cachedEnabled=false;m_cachedPreview={};}cancelDraft();m_enabled=false;}else attachViewport();EditorWindow::setWindowVisible(visible);}
    void CityRoadWindow::status(const String &message){if(m_status)m_status->setText(message);}
    SmartPtr<scene::CityRoadAuthoring> CityRoadWindow::current() const
    {
        auto scene=activeScene();auto target=m_authoring.load().lock();
        return scene && scene==m_scene.load().lock() && target && target->getActor() && target->getActor()->getScene()==scene?target:nullptr;
    }
    void CityRoadWindow::updateSelection()
    {
        attachViewport();auto app=core::IApplicationManager::instancePtr();auto selection=app?app->getSelectionManager():nullptr;
        if(!selection)return;
        const auto selected=selection->getSelection();u64 signature=1469598103934665603ull;
        for(const auto &object:selected){signature^=u64(reinterpret_cast<uintptr_t>(object.get()));signature*=1099511628211ull;}
        signature^=u64(reinterpret_cast<uintptr_t>(activeScene().get()));
        if(!m_hasSelectionSignature || signature!=m_selectionSignature)
        {{RecursiveMutex::ScopedLock lock(m_toolMutex);++m_toolEpoch;m_cachedEnabled=false;m_cachedPreview={};}cancelDraft();m_selected=false;}
        m_selectionSignature=signature;m_hasSelectionSignature=true;
        for(auto object:selected)
        {
            auto target=dynamic_pointer_cast<scene::CityRoadAuthoring>(object);
            if(!target)if(auto actor=dynamic_pointer_cast<scene::IGameActor>(object))target=actor->getComponent<scene::CityRoadAuthoring>();
            if(target){m_authoring=target;m_scene=activeScene();status("City road source selected. Draw or edit knots in the scene.");return;}
        }
        if(!current()){m_authoring=nullptr;m_scene=nullptr;m_enabled=false;}
    }
    bool CityRoadWindow::bindSelection()
    {
        auto app=core::IApplicationManager::instancePtr();auto selection=app?app->getSelectionManager():nullptr;auto scene=activeScene();
        SmartPtr<scene::TerrainSystem> terrain;
        if(selection)for(auto object:selection->getSelection())
        {terrain=dynamic_pointer_cast<scene::TerrainSystem>(object);if(!terrain)if(auto actor=dynamic_pointer_cast<scene::IGameActor>(object))terrain=actor->getComponent<scene::TerrainSystem>();if(terrain)break;}
        if(!terrain || !scene){status("Select a terrain component or its actor first.");return false;}
        auto target=current();if(!target)target=scene->getComponent<scene::CityRoadAuthoring>();
        if(!target)
        {
            auto actor=app->getGameManager()->createActor();actor->setName("City Road Authoring");
            actor->setPosition(Vector3F::zero());actor->setOrientation(QuaternionF::identity());actor->setScale(Vector3F::unit());
            target=actor->addComponent<scene::CityRoadAuthoring>();scene->addActor(actor);scene->registerAllUpdates(actor);
            if(auto editor=EditorManager::getSingletonPtr())if(auto ui=editor->getUI())ui->rebuildSceneTree();
        }
        String error;if(!target || !target->bindTerrain(terrain,error)){status(error);return false;}
        m_authoring=target;m_scene=scene;m_enabled=true;status("Terrain bound. Draw Road starts one editable Bezier spline.");return true;
    }
    void CityRoadWindow::cancelDraft(){m_draft={};m_manualHandles.clear();m_drafting=m_dragging=false;}
    bool CityRoadWindow::beginDraft()
    {
        auto target=current();if(!target){status("Bind a terrain or select a City Road Authoring component first.");return false;}
        m_draft=target->getSource();m_draftRevision=target->getSourceRevision();m_drafting=true;return true;
    }
    bool CityRoadWindow::beginDraw()
    {
        cancelDraft();if(!current() && !bindSelection())return false;if(!beginDraft())return false;auto target=current();
        if(m_draft.id.empty())m_draft.id=newId();const auto terrain=target->getTerrainBindingUUID();m_draft.terrainResourceUuid.assign(terrain.data(),terrain.size());
        procedural::CityRoadSpline road;road.id=newId();road.heightOffset=.025f;
        if(!fields(road)){cancelDraft();return false;}m_draft.roads.push_back(std::move(road));
        m_road=m_draft.roads.size()-1;m_mode=Mode::Draw;m_enabled=true;m_selected=false;status("Draw: click terrain for knots, drag for tangent handles. Enter commits; Escape cancels.");return true;
    }
    Vector3F CityRoadWindow::controlPoint(Vector3F p,const procedural::CityRoadSpline &road) const
    {
        auto target=current();if(!target)return p;p=target->localToWorld(p);
        if(road.heightMode==procedural::CityRoadHeightMode::Drape)
        {f32 height=0;const auto snapshot=target->getTerrainSnapshot();if(!snapshot || !render::sampleTerrainHeight(*snapshot,target->getTerrainPlacement(),p,height))p.y=std::numeric_limits<float>::quiet_NaN();else p.y=height+road.heightOffset;}
        return p;
    }
    bool CityRoadWindow::fields(procedural::CityRoadSpline &road)
    {
        try{size_t widthEnd=0,gradeEnd=0;const auto width=m_width->getValue(),grade=m_grade->getValue();
            const float w=std::stof(width,&widthEnd),g=std::stof(grade,&gradeEnd);
            if(widthEnd!=width.size() || gradeEnd!=grade.size() || !std::isfinite(w) || !std::isfinite(g) || w<.5f || w>60 || g<=0 || g>.35f)throw std::runtime_error("range");
            road.width=w;road.maxGrade=g;road.closed=m_closed->getValue();return true;
        }catch(const std::exception &){status("Width must be 0.5..60 m; grade must be greater than 0 and at most 0.35.");return false;}
    }
    bool CityRoadWindow::commitDraft()
    {
        auto target=current();if(!target || !m_drafting)return false;
        if(target->getSourceRevision()!=m_draftRevision){status("Source changed during this edit. Cancel the draft and reload before committing.");return false;}
        String error;const auto after=json(m_draft,error);if(after.empty()){status(error);return false;}
        String comparisonError;const SourceBudgetText intended(after.data(),after.size());
        if(sameAuthoredState(target->getSourceJSON(),intended,comparisonError)){cancelDraft();status("Selection retained; no road fields changed.");return true;}
        auto app=core::IApplicationManager::instancePtr();auto history=app?app->getCommandManager():nullptr;
        if(!history){status("Command history is unavailable; draft retained.");return false;}
        auto command=make_ptr<CityRoadEditCommand>();if(!command->configure(target,after,error)){status(error);return false;}
        command->execute();if(!command->applied()){status(command->error());return false;}history->addCommand(command);
        cancelDraft();status("Road edit committed. Ctrl+Z/Redo use scene command history; Save Scene persists source and terrain binding.");return true;
    }
    bool CityRoadWindow::addRoundabout(Vector3F center)
    {
        if(!beginDraft())return false;center.y+=.025f;auto target=current();procedural::CityRoadRoundaboutSpec spec;spec.junctionId=newId();spec.center=center;
        try
        {
            size_t a=0,b=0;const auto outer=m_outerRadius->getValue(),inner=m_islandRadius->getValue();
            spec.outerRadius=std::stof(outer,&a);spec.islandRadius=std::stof(inner,&b);
            if(a!=outer.size() || b!=inner.size() || !std::isfinite(spec.outerRadius) || !std::isfinite(spec.islandRadius) || spec.outerRadius<6 || spec.outerRadius>128 || spec.islandRadius<1 || spec.outerRadius-spec.islandRadius<3 || spec.outerRadius-spec.islandRadius>7)throw std::runtime_error("range");
        }
        catch(const std::exception &){status("Roundabout requires outer radius 6..128 m and a circulating lane 3..7 m wide around an island of at least 1 m.");cancelDraft();return false;}
        procedural::CityRoadSpline settings;if(!fields(settings)){cancelDraft();return false;}
        if(settings.width<4 || settings.width>12 || settings.width>=spec.outerRadius){status("Roundabout approaches require road width 4..12 m, smaller than the outer radius.");cancelDraft();return false;}
        spec.clockwise=m_clockwise->getValue();spec.maxGrade=settings.maxGrade;
        const auto snapshot=target->getTerrainSnapshot();const auto placement=target->getTerrainPlacement();const float step=spec.outerRadius+18;
        float xp=0,xm=0,zp=0,zm=0;
        if(!snapshot || !render::sampleTerrainHeight(*snapshot,placement,center+Vector3F(step,0,0),xp) || !render::sampleTerrainHeight(*snapshot,placement,center-Vector3F(step,0,0),xm) ||
           !render::sampleTerrainHeight(*snapshot,placement,center+Vector3F(0,0,step),zp) || !render::sampleTerrainHeight(*snapshot,placement,center-Vector3F(0,0,step),zm))
        {status("Roundabout approaches leave the terrain or cross a hole. Choose a covered location.");cancelDraft();return false;}
        spec.gradeX=(xp-xm)/(2*step);spec.gradeZ=(zp-zm)/(2*step);
        for(const auto direction:{Vector3F(1,0,0),Vector3F(0,0,1),Vector3F(-1,0,0),Vector3F(0,0,-1)})
        {
            procedural::CityRoadSpline road;road.id=newId();road.width=settings.width;road.maxGrade=settings.maxGrade;road.heightMode=procedural::CityRoadHeightMode::Drape;road.heightOffset=.025f;
            procedural::CityRoadRoundaboutPort port;port.approachId=road.id;port.outward=direction;port.width=road.width;port.length=18;spec.ports.push_back(port);
            auto start=center+direction*(spec.outerRadius+port.length),end=start+direction*40.f;
            start.y=center.y+spec.gradeX*(start.x-center.x)+spec.gradeZ*(start.z-center.z);
            end.y=center.y+spec.gradeX*(end.x-center.x)+spec.gradeZ*(end.z-center.z);
            start.y-=road.heightOffset;end.y-=road.heightOffset;
            const auto tangent=(end-start)/3.f;
            road.knots.push_back({newId(),start,{},tangent});road.knots.push_back({newId(),end,-tangent,{}});m_draft.roads.push_back(std::move(road));
        }
        m_draft.version=2;const auto binding=target->getTerrainBindingUUID();m_draft.terrainResourceUuid.assign(binding.data(),binding.size());
        m_draft.roundabouts.push_back(std::move(spec));return commitDraft();
    }
    void CityRoadWindow::history(bool redo)
    {
        if(m_drafting && m_mode==Mode::Draw && !redo)
        {if(m_road<m_draft.roads.size() && !m_draft.roads[m_road].knots.empty())m_draft.roads[m_road].knots.pop_back();m_dragging=false;status("Removed the last draft knot.");return;}
        cancelDraft();auto app=core::IApplicationManager::instancePtr();auto manager=app?app->getCommandManager():nullptr;
        String error;if(CityRoadEditCommand::dispatchHistory(manager,redo,error))status("History applied.");else if(!error.empty())status(error);
    }
    void CityRoadWindow::action(Widget id,bool invalidateInput)
    {
        if(Thread::getCurrentTask()!=TaskId::Application){status("Road controls require the Application task.");return;}
        if(invalidateInput && (id==Draw || id==Move || id==InHandle || id==OutHandle || id==Roundabout || id==Cancel || id==Bind)){RecursiveMutex::ScopedLock lock(m_toolMutex);++m_toolEpoch;}
        if(id==Bind){cancelDraft();bindSelection();return;}
        if(id==Cancel){cancelDraft();m_enabled=false;status("Road tool cancelled.");return;}
        if(id==Undo || id==Redo){history(id==Redo);return;}
        if(id==Save){if(m_drafting){status("Commit or cancel the draft before saving.");return;}auto app=core::IApplicationManager::instancePtr();if(app && activeScene()){auto job=make_ptr<SaveSceneJob>();job->setPrimary(true);app->getJobQueue()->addJob(job);status("Scene save requested with canonical road source and terrain binding.");}return;}
        if(id==Finish)
        {
            if(m_drafting && m_road<m_draft.roads.size() && fields(m_draft.roads[m_road]))
            {
                auto &road=m_draft.roads[m_road];
                if(m_mode==Mode::Draw && road.closed && road.knots.size()>=3)
                    for(const size_t i:{size_t(0),road.knots.size()-1})if(!m_manualHandles.count(road.knots[i].id))
                    {const auto tangent=(road.knots[(i+1)%road.knots.size()].position-road.knots[(i+road.knots.size()-1)%road.knots.size()].position)/6.f;road.knots[i].inHandle=-tangent;road.knots[i].outHandle=tangent;}
                commitDraft();
            }
            return;
        }
        if(id==Draw)
        {
            beginDraw();return;
        }
        if(id==Move || id==InHandle || id==OutHandle)
        {cancelDraft();m_mode=id==Move?Mode::Move:id==InHandle?Mode::InHandle:Mode::OutHandle;m_enabled=true;status("Drag a knot or handle on the terrain. Each completed drag is one undoable edit.");return;}
        if(id==Roundabout){cancelDraft();if(!current() && !bindSelection())return;m_mode=Mode::Roundabout;m_enabled=true;status("Click terrain to place a roundabout and four connected approaches. Terrain fit failures retain the current city.");return;}
        if(id==RemoveRoundabout)
        {
            if(!m_selected || !beginDraft()){status("Select an approach road knot with Move Knot first.");return;}
            const auto junction=std::find_if(m_draft.roundabouts.begin(),m_draft.roundabouts.end(),[&](const auto &item){return std::any_of(item.ports.begin(),item.ports.end(),[&](const auto &port){return port.approachId==m_selectedRoadId;});});
            if(junction==m_draft.roundabouts.end()){cancelDraft();status("The selected road is not a roundabout approach.");return;}
            SourceBudgetSet<SourceBudgetText> removed,retained;
            for(const auto &port:junction->ports)removed.insert(port.approachId);
            for(const auto &other:m_draft.roundabouts)if(&other!=&*junction)for(const auto &port:other.ports)retained.insert(port.approachId);
            m_draft.roads.erase(std::remove_if(m_draft.roads.begin(),m_draft.roads.end(),[&](const auto &road){return removed.count(road.id) && !retained.count(road.id);}),m_draft.roads.end());
            m_draft.roundabouts.erase(junction);if(commitDraft())m_selected=false;return;
        }
        if(id==ApplyFields || id==RemoveRoad)
        {
            if(!m_selected || !beginDraft()){status("Select a road knot first with Move Knot.");return;}
            const auto found=std::find_if(m_draft.roads.begin(),m_draft.roads.end(),[&](const auto &road){return road.id==m_selectedRoadId;});
            if(found==m_draft.roads.end()){cancelDraft();status("The selected road was removed or changed. Select it again.");return;}m_road=size_t(found-m_draft.roads.begin());
            if(id==RemoveRoad)
            {
                for(const auto &junction:m_draft.roundabouts)for(const auto &port:junction.ports)if(port.approachId==m_selectedRoadId)
                {cancelDraft();status("This road belongs to a roundabout. Delete Selected Roundabout removes its junction and approaches together.");return;}
                m_draft.roads.erase(m_draft.roads.begin()+m_road);
            }
            else if(!fields(m_draft.roads[m_road]))return;
            if(commitDraft() && id==RemoveRoad)m_selected=false;
        }
    }
    Parameter CityRoadWindow::preview() const
    {
        Parameters lines,points;auto target=current();if(!target)return Parameter(Parameters{Parameter(false)});
        const auto &source=m_drafting?m_draft:target->getSource();size_t budget=0;
        const auto point=[](Vector3F p){Parameter v;v.setVector3(p);return v;};
        const auto world=[&](Vector3F p,bool drape=false)
        {
            p=target->localToWorld(p);
            if(drape){f32 height=0;const auto snapshot=target->getTerrainSnapshot();if(!snapshot || !render::sampleTerrainHeight(*snapshot,target->getTerrainPlacement(),p,height))p.y=std::numeric_limits<float>::quiet_NaN();else p.y=height;}
            return p+Vector3F(0,.035f,0);
        };
        for(const auto &road:source.roads)
        {
            for(const auto &k:road.knots)
            {if(++budget>4096)break;const auto center=controlPoint(k.position,road)+Vector3F(0,.035f,0),in=controlPoint(k.position+k.inHandle,road)+Vector3F(0,.035f,0),out=controlPoint(k.position+k.outHandle,road)+Vector3F(0,.035f,0);points.push_back(point(center));lines.push_back(point(in));lines.push_back(point(center));lines.push_back(point(center));lines.push_back(point(out));points.push_back(point(in));points.push_back(point(out));}
            const size_t spans=road.knots.size()>1?(road.closed?road.knots.size():road.knots.size()-1):0;
            for(size_t i=0;i<spans && budget<4096;++i)
            {
                const auto &a=road.knots[i],&b=road.knots[(i+1)%road.knots.size()];auto previous=a.position;
                for(unsigned j=1;j<=16 && budget<4096;++j,++budget)
                {const float t=j/16.f,u=1-t;const auto p=a.position*(u*u*u)+(a.position+a.outHandle)*(3*u*u*t)+(b.position+b.inHandle)*(3*u*t*t)+b.position*(t*t*t);const bool drape=road.heightMode==procedural::CityRoadHeightMode::Drape;lines.push_back(point(world(previous,drape)));lines.push_back(point(world(p,drape)));previous=p;}
            }
        }
        for(const auto &junction:source.roundabouts)
            for(const float radius:{junction.outerRadius,junction.islandRadius})
                for(unsigned i=0;i<64 && budget<4096;++i,++budget)
                {const float a=i*6.28318530718f/64,b=(i+1)*6.28318530718f/64;auto p=junction.center+Vector3F(std::cos(a)*radius,0,std::sin(a)*radius),q=junction.center+Vector3F(std::cos(b)*radius,0,std::sin(b)*radius);p.y=junction.center.y+junction.gradeX*(p.x-junction.center.x)+junction.gradeZ*(p.z-junction.center.z);q.y=junction.center.y+junction.gradeX*(q.x-junction.center.x)+junction.gradeZ*(q.z-junction.center.z);lines.push_back(point(world(p)));lines.push_back(point(world(q)));}
        return Parameter(Parameters{Parameter(m_enabled),Parameter(lines),Parameter(points)});
    }
    Parameter CityRoadWindow::processViewport(const Array<Parameter> &args,bool queued)
    {
        if(args.size()!=7)return {};
        const auto phase=args.front().getStr();
        if(phase=="button"){action(static_cast<Widget>(s32(args[1].getF64())),!queued);return preview();}
        if(!m_enabled || !current())return {};
        if(phase=="cancel"){cancelDraft();m_enabled=false;return preview();}
        if(phase=="finish"){action(Finish);return preview();}
        if(phase=="undo" || phase=="redo"){history(phase=="redo");return preview();}
        if(phase=="save"){action(Save);return preview();}
        if(phase=="remove")
        {if(m_drafting && m_mode==Mode::Draw)history(false);else action(RemoveRoad);return preview();}
        if(phase=="commit")
        {if(m_dragging && m_mode!=Mode::Draw)commitDraft();m_dragging=false;return preview();}
        if(phase!="begin" && phase!="update")return preview();
        const Ray3F ray({f32(args[1].getF64()),f32(args[2].getF64()),f32(args[3].getF64())},{f32(args[4].getF64()),f32(args[5].getF64()),f32(args[6].getF64())});
        Vector3F hit;String error;auto target=current();if(!target->intersectTerrain(ray,hit,error)){status(error);return preview();}
        if(m_mode==Mode::Roundabout){if(phase=="begin")addRoundabout(hit);return preview();}
        if(phase=="begin")
        {
            if(m_mode==Mode::Draw)
            {
                if(!m_drafting && !beginDraw())return preview();auto &knots=m_draft.roads[m_road].knots;
                if(knots.size()>=256){status("A road supports at most 256 knots.");return preview();}
                if(!knots.empty() && (knots.back().position-hit).lengthSquared()<.01f)return preview();
                procedural::CityRoadSplineKnot knot;knot.id=newId();knot.position=hit;
                if(!knots.empty())
                {
                    auto &previous=knots.back();const auto span=hit-previous.position;knot.inHandle=-span/3.f;
                    if(!m_manualHandles.count(previous.id))
                    {const auto tangent=knots.size()>1?(hit-knots[knots.size()-2].position)/6.f:span/3.f;previous.outHandle=tangent;if(knots.size()>1)previous.inHandle=-tangent;}
                }
                knots.push_back(std::move(knot));m_knot=knots.size()-1;m_dragging=true;status("Draft knots: "+StringUtil::toString(u32(knots.size()))+". Enter commits the road.");
            }
            else
            {
                if(!beginDraft())return preview();float closest=4.f;bool found=false;
                for(size_t r=0;r<m_draft.roads.size();++r)for(size_t k=0;k<m_draft.roads[r].knots.size();++k)
                {const auto &road=m_draft.roads[r];const auto &knot=road.knots[k];const auto p=controlPoint(knot.position+(m_mode==Mode::InHandle?knot.inHandle:m_mode==Mode::OutHandle?knot.outHandle:Vector3F::zero()),road);const auto distance=(p-hit).lengthSquared();if(distance<closest){closest=distance;m_road=r;m_knot=k;found=true;}}
                if(!found){cancelDraft();status("No knot/handle within 2 metres of the terrain pick.");return preview();}
                m_dragging=m_selected=true;const auto &road=m_draft.roads[m_road];m_selectedRoadId=road.id;m_selectedKnotId=road.knots[m_knot].id;m_width->setValue(StringUtil::toString(road.width));m_grade->setValue(StringUtil::toString(road.maxGrade));m_closed->setValue(road.closed);
            }
        }
        else if(m_dragging && m_drafting && m_road<m_draft.roads.size() && m_knot<m_draft.roads[m_road].knots.size())
        {
            auto &knot=m_draft.roads[m_road].knots[m_knot];if(m_mode==Mode::Move)knot.position=hit;
            else if(m_mode==Mode::InHandle)knot.inHandle=hit-knot.position;
            else {knot.outHandle=hit-knot.position;if(m_mode==Mode::Draw){knot.inHandle=-knot.outHandle;m_manualHandles.insert(knot.id);}}
        }
        return preview();
    }
    Parameter CityRoadWindow::handleViewport(const Array<Parameter> &args)
    {
        if(args.size()!=7)return {};
        if(Thread::getCurrentTask()==TaskId::Application)
        {const auto reply=processViewport(args);RecursiveMutex::ScopedLock lock(m_toolMutex);m_cachedPreview=reply;const auto &parts=reply.getArray();m_cachedEnabled=!parts.empty() && parts.front().getBool();return reply;}
        const auto phase=args[0].getStr();RecursiveMutex::ScopedLock lock(m_toolMutex);if(!m_cachedEnabled && phase!="button")return {};
        auto app=core::IApplicationManager::instancePtr();auto tasks=app?app->getTaskManager():nullptr;auto ownerTask=tasks?tasks->getTask(TaskId::Application):nullptr;
        if(!ownerTask || !ownerTask->isLoaded() || ownerTask->getState()==ITask::State::Shutdown)return {};
        Input input;input.phase.assign(phase.data(),phase.size());input.epoch=m_toolEpoch;
        input.origin={f32(args[1].getF64()),f32(args[2].getF64()),f32(args[3].getF64())};input.direction={f32(args[4].getF64()),f32(args[5].getF64()),f32(args[6].getF64())};
        const bool motion=phase=="hover" || phase=="update";
        if(motion && !m_inputs.empty() && m_inputs.back().phase==input.phase)m_inputs.back()=std::move(input);
        else if(m_inputs.size()<128)m_inputs.push_back(std::move(input));
        else{m_inputs.clear();input.phase="cancel";m_inputs.push_back(std::move(input));}
        if(!m_dispatchPending)
        {
            auto job=make_ptr<ViewportJob>();job->setOwner(this);job->setPrimary(true);m_dispatchPending=true;
            try{ownerTask->addJob(job);}catch(...){m_dispatchPending=false;m_inputs.clear();throw;}
        }
        return m_cachedPreview.getArray().empty()?Parameter(Parameters{Parameter(true)}):m_cachedPreview;
    }
    void CityRoadWindow::drainViewport()
    {
        SourceBudgetVector<Input> pending;
        {RecursiveMutex::ScopedLock lock(m_toolMutex);m_dispatchPending=false;pending=std::move(m_inputs);m_inputs={};}
        if(Thread::getCurrentTask()!=TaskId::Application){status("Road viewport input was not delivered on the Application task; no source was changed.");WP_LOG_ERROR("CityRoadWindow: wrong viewport input task.");return;}
        for(const auto &input:pending)
        {
            {RecursiveMutex::ScopedLock lock(m_toolMutex);if(input.epoch!=m_toolEpoch)continue;}
            if(!isLoaded() || !isWindowVisible() || (!m_enabled && input.phase!="button"))continue;
            const Parameters args{Parameter(String(input.phase.data(),input.phase.size())),Parameter(f64(input.origin.x)),Parameter(f64(input.origin.y)),Parameter(f64(input.origin.z)),Parameter(f64(input.direction.x)),Parameter(f64(input.direction.y)),Parameter(f64(input.direction.z))};
            const auto reply=processViewport(args,true);
            {RecursiveMutex::ScopedLock lock(m_toolMutex);m_cachedPreview=reply;const auto &parts=reply.getArray();m_cachedEnabled=!parts.empty() && parts.front().getBool();}
        }
    }
    void CityRoadWindow::ViewportJob::execute(){if(auto owner=m_owner.load().lock())owner->drainViewport();}
    Parameter CityRoadWindow::Listener::handleEvent(EventType,hash_type value,const Array<Parameter> &args,SmartPtr<ISharedObject> sender,SmartPtr<ISharedObject>,SmartPtr<IEvent>)
    {
        auto owner=m_owner.load().lock();if(!owner || !owner->isLoaded() || !owner->isWindowVisible())return {};
        if(!owner->getParentWindow() || !owner->getParentWindow()->isVisible())
        {if(value==StringUtil::getHash("cityRoadViewportTool") && args.size()==7){auto cancel=args;cancel[0]=Parameter(String("cancel"));owner->handleViewport(cancel);}return {};}
        if(value==StringUtil::getHash("cityRoadViewportTool"))return owner->handleViewport(args);
        if(value==IEvent::handleSelection)if(auto element=dynamic_pointer_cast<ui::IUIElement>(sender))
        {owner->handleViewport({Parameter(String("button")),Parameter(f64(element->getElementId())),Parameter(0.0),Parameter(0.0),Parameter(0.0),Parameter(0.0),Parameter(0.0)});}
        return {};
    }
}
