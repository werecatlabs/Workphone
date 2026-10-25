#include <EditorPCH.hpp>
#include <EditorApplication.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/CityRoadWindow.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>

#if WP_EDITOR_TESTS
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
using namespace workphone;
using namespace workphone::editor;
namespace
{
    struct RoadEditorDirectory
    {
        std::filesystem::path path=std::filesystem::temp_directory_path()/std::string(("workphone-city-tool-"+StringUtil::getUUID()).c_str());
        ~RoadEditorDirectory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
    };
    struct RoadEditorGuard
    {
        EditorApplication app;
        ~RoadEditorGuard(){if(auto state=core::IApplicationManager::instancePtr()){state->setQuit(true);state->setRunning(false);}app.unload(nullptr);}
    };
    struct RoadTask
    {
        TaskId previous=Thread::getCurrentTask();
        explicit RoadTask(TaskId task){Thread::setCurrentTask(task);}
        ~RoadTask(){Thread::setCurrentTask(previous);}
    };
    struct RoadTaskOwner
    {
        SmartPtr<ITask> task;SmartPtr<ISharedObject> previous;
        explicit RoadTaskOwner(SmartPtr<ITask> value):task(value),previous(value->getOwner()){task->setOwner(nullptr);}
        ~RoadTaskOwner(){task->setOwner(previous);}
    };
    class RoadTaskBarrier : public Job
    {
    public:
        bool *complete=nullptr;TaskId *observed=nullptr;
        void execute() override{*observed=Thread::getCurrentTask();*complete=true;}
    };
    String roadJSON(const procedural::CityRoadSplineSource &source)
    {SourceBudgetText text;String error;BOOST_REQUIRE_MESSAGE(procedural::serializeCityRoadSplineSource(source,text,error),error);return String(text.data(),text.size());}
}

BOOST_AUTO_TEST_CASE(city_road_viewport_authoring_test)
{
    RoadEditorDirectory temporary;Project project;project.create(temporary.path.generic_u8string());project.setMediaPaths({});project.save();
    RoadEditorGuard guard;guard.app.setDebugMode(true);guard.app.setActiveThreads(0);guard.app.load(nullptr);BOOST_REQUIRE(guard.app.isLoaded());
    auto app=core::IApplicationManager::instancePtr();auto editor=EditorManager::getSingletonPtr();BOOST_REQUIRE(app);BOOST_REQUIRE(editor);
    editor->loadProject(project.getFilePath());RoadTask application(TaskId::Application);
    auto manager=app->getGameManager();auto scene=manager->getCurrentScene();auto commands=app->getCommandManager();auto selection=app->getSelectionManager();
    auto window=editor->getUI()->getCityRoadWindow();BOOST_REQUIRE(scene);BOOST_REQUIRE(commands);BOOST_REQUIRE(selection);BOOST_REQUIRE(window);BOOST_REQUIRE(window->isLoaded());
    auto applicationTask=app->getTaskManager()->getTask(TaskId::Application);BOOST_REQUIRE(applicationTask);BOOST_REQUIRE(applicationTask->isLoaded());BOOST_REQUIRE(!applicationTask->isStopped());RoadTaskOwner taskOwner(applicationTask);
    const auto flush=[&]()
    {
        // Task-specific owner jobs are independent from JobQueue::hasJobs().
        bool completed=false;TaskId observed=TaskId::None;auto barrier=make_ptr<RoadTaskBarrier>();barrier->complete=&completed;barrier->observed=&observed;applicationTask->addJob(barrier);
        for(unsigned pass=0;pass<8 && !completed;++pass){applicationTask->setState(ITask::State::Idle);applicationTask->setNextUpdateTime(-1);applicationTask->update();}
        BOOST_REQUIRE_MESSAGE(completed,"Application owner queue did not execute its barrier; state="+StringUtil::toString(s32(applicationTask->getState()))+", stopped="+StringUtil::toString(applicationTask->isStopped()));
        BOOST_CHECK(observed==TaskId::Application);
        auto jobs=app->getJobQueue();for(unsigned i=0;i<100 && jobs->hasJobs();++i)jobs->update();
    };flush();
    auto terrainActor=manager->createActor();terrainActor->setName("City tool test terrain");
    auto terrain=terrainActor->addComponent<scene::TerrainSystem>();BOOST_REQUIRE(terrain);scene->addActor(terrainActor);
    // CPU publication adapter below exercises authoring/controls; it does not
    // certify deferred GPU publication (covered by the native showcase capture).
    if(auto renderer=dynamic_pointer_cast<render::Terrain>(terrain->getTerrain())){renderer->setDeferredTerrainPublication(false);BOOST_REQUIRE(!renderer->isTerrainPublicationDeferred());}
    render::TerrainData data;data.dimensions={33,33};data.spacing={8,8};data.origin={-128,-128};data.heights.resize(33*33);
    for(int z=0;z<33;++z)for(int x=0;x<33;++x)data.heights[size_t(z)*33+x]=2+.01f*(-128+x*8);
    String error;BOOST_REQUIRE_MESSAGE(terrain->applyTerrainData(data,error),error);
    auto cityActor=manager->createActor();cityActor->setName("City tool test source");auto city=cityActor->addComponent<scene::CityRoadAuthoring>();BOOST_REQUIRE(city);scene->addActor(cityActor);
    unsigned publications=0;bool applicationOnly=true;
    city->setPublishCallback([&](const auto &,const auto &,auto,const auto &,String &){++publications;applicationOnly=applicationOnly && Thread::getCurrentTask()==TaskId::Application;return true;});
    window->setWindowVisible(true);selection->clearSelection();selection->addSelectedObject(terrainActor);window->updateSelection();
    const auto activate=[&](const String &label)
    {
        for(auto element:window->getParentWindow()->getChildren())
            if(auto button=dynamic_pointer_cast<ui::IUIButton>(element))if(button->getLabel()==label)
            {BOOST_REQUIRE(window->isWindowVisible());BOOST_REQUIRE(window->getParentWindow()->isVisible());BOOST_REQUIRE(!button->getObjectListeners().empty());for(auto listener:button->getObjectListeners())listener->handleEvent(EventType::UI,IEvent::handleSelection,{},button,nullptr,nullptr);return;}
        BOOST_FAIL("Missing native city tool control: "+label);
    };
    const auto ray=[&](const char *phase,float x,float z)
    {return window->handleViewport({Parameter(String(phase)),Parameter(f64(x)),Parameter(40.0),Parameter(f64(z)),Parameter(0.0),Parameter(-1.0),Parameter(0.0)});};
    const auto diagnostic=[&]()
    {
        String result=city->getError();
        for(auto element:window->getParentWindow()->getChildren())if(auto text=dynamic_pointer_cast<ui::IUIText>(element))result=text->getText();
        return String("CityRoadAuthoring: ")+city->getError()+"; tool: "+result;
    };
    {RoadTask render(TaskId::Render);activate("Bind Selected Terrain");}BOOST_CHECK(!city->getTerrain());flush();BOOST_CHECK(city->getTerrain()==terrain);commands->clearAll();
    const auto beforeRender=publications;
    {RoadTask render(TaskId::Render);activate("Draw Road");ray("begin",-30,-10);ray("commit",-30,-10);ray("begin",30,-10);ray("commit",30,-10);ray("finish",30,-10);}
    BOOST_CHECK(city->getSource().roads.empty());BOOST_CHECK_EQUAL(publications,beforeRender);flush();
    BOOST_REQUIRE_MESSAGE(city->getSource().roads.size()==1u,diagnostic());BOOST_REQUIRE_EQUAL(city->getSource().roads[0].knots.size(),2u);BOOST_CHECK(applicationOnly);
    BOOST_CHECK_SMALL(city->getSource().roads[0].heightOffset-.025f,.0001f);
    BOOST_CHECK_SMALL(city->getBake().roads[0].points.front().position.y-(2-.3f+.025f),.001f);
    const auto roadId=city->getSource().roads[0].id,knotId=city->getSource().roads[0].knots[0].id;
    // A second queued gesture re-arms Draw without invalidating its own batch.
    {RoadTask render(TaskId::Render);ray("begin",-40,25);ray("commit",-40,25);ray("begin",-10,25);ray("commit",-10,25);ray("finish",-10,25);}flush();
    BOOST_REQUIRE_MESSAGE(city->getSource().roads.size()==2u,diagnostic());activate("Undo");BOOST_REQUIRE_EQUAL(city->getSource().roads.size(),1u);
    for(auto &height:data.heights)height+=5;BOOST_REQUIRE_MESSAGE(terrain->applyTerrainData(data,error),error);BOOST_REQUIRE_MESSAGE(city->rebuild(error),error);
    const auto overlay=ray("hover",-30,-10).getArray();BOOST_REQUIRE_EQUAL(overlay.size(),3u);const auto &controls=overlay[2].getArray();BOOST_REQUIRE(!controls.empty());
    BOOST_CHECK_SMALL(controls.front().getVector3().y-(7-.3f+.025f+.035f),.001f);
    activate("Move Knot");ray("begin",-30,-10);ray("update",-25,-15);ray("commit",-25,-15);
    BOOST_CHECK_SMALL(city->getSource().roads[0].knots[0].position.x+25,.0001f);
    {RoadTask render(TaskId::Render);BOOST_REQUIRE_MESSAGE(CityRoadEditCommand::dispatchHistory(commands,false,error),error);}
    BOOST_CHECK_SMALL(city->getSource().roads[0].knots[0].position.x+25,.0001f);flush();BOOST_CHECK_SMALL(city->getSource().roads[0].knots[0].position.x+30,.0001f);
    activate("Undo");BOOST_CHECK(city->getSource().roads.empty());
    activate("Redo");activate("Redo");BOOST_REQUIRE_EQUAL(city->getSource().roads.size(),1u);
    BOOST_CHECK(city->getSource().roads[0].id==roadId);BOOST_CHECK(city->getSource().roads[0].knots[0].id==knotId);
    BOOST_CHECK_SMALL(city->getSource().roads[0].knots[0].position.x+25,.0001f);
    auto external=city->getSource();external.roads[0].width=10;
    BOOST_REQUIRE_MESSAGE(city->setSourceJSON(roadJSON(external),city->getSourceRevision(),error),error);
    const auto expectedHistory=commands->getPreviousCommand();commands->getNextCommand();
    const auto externalRevision=city->getSourceRevision();activate("Undo");BOOST_CHECK_EQUAL(city->getSourceRevision(),externalRevision);BOOST_CHECK_EQUAL(city->getSource().roads[0].width,10);
    BOOST_CHECK(commands->getPreviousCommand()==expectedHistory);commands->getNextCommand();
    activate("Draw Road");ray("begin",-40,25);ray("commit",-40,25);ray("cancel",0,0);BOOST_CHECK_EQUAL(city->getSource().roads.size(),1u);
    activate("Place Roundabout (4 approaches)");ray("begin",0,40);
    BOOST_REQUIRE_EQUAL(city->getSource().roundabouts.size(),1u);BOOST_CHECK_EQUAL(city->getSource().roads.size(),5u);
    BOOST_CHECK_EQUAL(city->getSource().roundabouts[0].ports.size(),4u);
    for(const auto &port:city->getSource().roundabouts[0].ports)
        BOOST_CHECK(std::any_of(city->getSource().roads.begin(),city->getSource().roads.end(),[&](const auto &road){return road.id==port.approachId;}));
    activate("Move Knot");ray("begin",32,40);ray("commit",32,40);const auto beforeGuard=city->getSourceRevision();activate("Delete Selected Road");
    BOOST_CHECK_EQUAL(city->getSourceRevision(),beforeGuard);BOOST_CHECK_EQUAL(city->getSource().roads.size(),5u);
    activate("Delete Selected Roundabout");BOOST_CHECK(city->getSource().roundabouts.empty());BOOST_REQUIRE_EQUAL(city->getSource().roads.size(),1u);
    activate("Undo");BOOST_CHECK_EQUAL(city->getSource().roundabouts.size(),1u);BOOST_CHECK_EQUAL(city->getSource().roads.size(),5u);
    activate("Undo");BOOST_CHECK(city->getSource().roundabouts.empty());BOOST_REQUIRE_EQUAL(city->getSource().roads.size(),1u);
    const auto beforeHide=city->getSourceRevision();activate("Draw Road");
    {RoadTask render(TaskId::Render);ray("begin",-40,25);ray("commit",-40,25);ray("begin",-10,25);ray("commit",-10,25);ray("finish",-10,25);}
    window->setWindowVisible(false);{RoadTask render(TaskId::Render);BOOST_CHECK(ray("begin",0,0).getArray().empty());}flush();BOOST_CHECK_EQUAL(city->getSourceRevision(),beforeHide);

    // Scene serialization contains the same canonical source and stable terrain binding.
    const auto saved=temporary.path/"Assets"/"CityRoadTool.scene";scene->saveScene(saved.generic_u8string());BOOST_REQUIRE(std::filesystem::is_regular_file(saved));
    std::ifstream file(saved,std::ios::binary);const std::string serialized((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    BOOST_CHECK(serialized.find(std::string(roadId.data(),roadId.size()))!=std::string::npos);
    auto restored=make_ptr<scene::CityRoadAuthoring>();restored->setPublishCallback([](const auto &,const auto &,auto,const auto &,String &){return true;});
    restored->setProperties(city->getProperties());BOOST_REQUIRE_EQUAL(restored->getSource().roads.size(),1u);
    BOOST_CHECK(restored->getSource().roads[0].id==roadId);BOOST_CHECK_EQUAL(restored->getTerrainBindingUUID(),city->getTerrainBindingUUID());
    BOOST_CHECK(applicationOnly);commands->clearAll();selection->clearSelection();manager->destroyActor(cityActor);manager->destroyActor(terrainActor);
}
#endif
