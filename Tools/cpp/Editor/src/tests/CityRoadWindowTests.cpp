#include <EditorPCH.hpp>
#include <EditorApplication.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/CityRoadWindow.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <WPImGui/SceneViewportToolInput.hpp>

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
    struct RoadEditorRunning
    {
        core::IApplicationManager &app;
        bool previous;
        explicit RoadEditorRunning( core::IApplicationManager &value )
            : app( value ), previous( value.isRunning() )
        {
            app.setRunning( true );
        }
        ~RoadEditorRunning() { app.setRunning( previous ); }
    };
    class RoadTaskBarrier : public Job
    {
    public:
        bool *complete=nullptr;TaskId *observed=nullptr;
        void execute() override{*observed=Thread::getCurrentTask();*complete=true;}
    };
    struct RoadViewportCamera
    {
        Matrix4<real_Num> view, projection;
        const Matrix4<real_Num> &getViewMatrix() const { return view; }
        const Matrix4<real_Num> &getProjectionMatrix() const { return projection; }
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
    const auto viewport = app->getSceneRenderWindow();
    BOOST_REQUIRE( viewport );
    BOOST_REQUIRE( !viewport->getObjectListeners().empty() );
    RoadViewportCamera camera;
    const Vector3<real_Num> eye( 0, 80, 100 );
    const auto orientation = MathUtil<real_Num>::getOrientationFromDirection(
        ( -eye ).normaliseCopy(), -Vector3<real_Num>::unitZ(), true, Vector3<real_Num>::unitY() );
    camera.view.makeViewMatrix( eye, orientation, nullptr );
    camera.projection.makePerspective( real_Num( 1 ), real_Num( 1.5 ), real_Num( 1 ), real_Num( 1000 ) );
    const auto clip = camera.projection * camera.view;
    const auto viewportGesture = [&]( const char *phase, float x, float z,
                                       Vector2F origin, Vector2F size ) {
        const auto y = 2.f + .01f * x;
        const auto project = [&]( u32 row ) {
            return clip[row][0] * x + clip[row][1] * y + clip[row][2] * z + clip[row][3];
        };
        const auto w = project( 3 );
        const Vector2F point( f32( project( 0 ) / w * .5 + .5 ), f32( .5 - project( 1 ) / w * .5 ) );
        BOOST_REQUIRE( point.x > 0 && point.x < 1 && point.y > 0 && point.y < 1 );
        Parameters arguments;
        BOOST_REQUIRE( ui::makeSceneViewportToolArguments( camera, origin, size, origin + point * size,
                                                            String( phase ), arguments ) );
        for( auto listener : viewport->getObjectListeners() )
        {
            listener->handleEvent( EventType::UI,
                StringUtil::getHash( "cityRoadViewportTool" ), arguments, viewport, nullptr, nullptr );
        }
    };
    const auto diagnostic=[&]()
    {
        String result=city->getError();
        for(auto element:window->getParentWindow()->getChildren())if(auto text=dynamic_pointer_cast<ui::IUIText>(element))result=text->getText();
        return String("CityRoadAuthoring: ")+city->getError()+"; tool: "+result;
    };
    {RoadTask render(TaskId::Render);activate("Bind Selected Terrain");}BOOST_CHECK(!city->getTerrain());flush();BOOST_CHECK(city->getTerrain()==terrain);commands->clearAll();
    const auto beforeRender=publications;
    {
        RoadTask render( TaskId::Render );
        activate( "Draw Road" );
        const Vector2F origin( 123.75f, 91.5f ), size( 866.5f, 517.25f );
        viewportGesture( "begin", -30, -10, origin, size );
        viewportGesture( "commit", -30, -10, origin, size );
        viewportGesture( "begin", 30, -10, origin, size );
        viewportGesture( "commit", 30, -10, origin, size );
        viewportGesture( "finish", 30, -10, origin, size );
    }
    BOOST_CHECK(city->getSource().roads.empty());BOOST_CHECK_EQUAL(publications,beforeRender);flush();
    BOOST_REQUIRE_MESSAGE(city->getSource().roads.size()==1u,diagnostic());BOOST_REQUIRE_EQUAL(city->getSource().roads[0].knots.size(),2u);BOOST_CHECK(applicationOnly);
    BOOST_CHECK_SMALL(city->getSource().roads[0].heightOffset-.025f,.0001f);
    BOOST_CHECK_SMALL(city->getBake().roads[0].points.front().position.y-(2-.3f+.025f),.001f);
    BOOST_CHECK_SMALL( city->getSource().roads[0].knots.front().position.x + 30.f, .001f );
    BOOST_CHECK_SMALL( city->getSource().roads[0].knots.back().position.x - 30.f, .001f );
    BOOST_CHECK_SMALL( city->getSource().roads[0].knots.front().position.z + 10.f, .001f );
    BOOST_CHECK_SMALL( city->getSource().roads[0].knots.back().position.z + 10.f, .001f );
    const auto roadId=city->getSource().roads[0].id,knotId=city->getSource().roads[0].knots[0].id;
    const auto initialKnotPosition = city->getSource().roads[0].knots[0].position;
    // Give the scratch scene a filename before testing the normal Save action.
    const auto saved = temporary.path / "Assets" / "CityRoadTool.scene";
    const auto savedPath = saved.generic_u8string();
    scene->saveScene( savedPath );
    BOOST_REQUIRE( std::filesystem::is_regular_file( saved ) );
    // A second gesture re-arms Draw and records a manually dragged tangent.
    const Vector2F secondOrigin( 8, 80 ), secondSize( 384, 300 );
    const auto secondGesture = [&]( const char *phase, float x, float z ) {
        viewportGesture( phase, x, z, secondOrigin, secondSize );
    };
    {
        RoadTask render( TaskId::Render );
        secondGesture( "begin", -40, 25 );
        secondGesture( "update", -33, 27 );
        secondGesture( "commit", -33, 27 );
        secondGesture( "begin", -10, 25 );
        secondGesture( "commit", -10, 25 );
        secondGesture( "finish", -10, 25 );
    }
    flush();
    BOOST_REQUIRE_MESSAGE( city->getSource().roads.size() == 2u, diagnostic() );
    const auto &dragged = city->getSource().roads.back().knots.front();
    BOOST_CHECK_SMALL( ( dragged.outHandle - Vector3F( 7, .07f, 2 ) ).length(), .001f );
    BOOST_CHECK( dragged.inHandle == -dragged.outHandle );
    { RoadTask render( TaskId::Render ); secondGesture( "undo", -10, 25 ); }
    flush();
    BOOST_REQUIRE_EQUAL( city->getSource().roads.size(), 1u );
    for(auto &height:data.heights)height+=5;BOOST_REQUIRE_MESSAGE(terrain->applyTerrainData(data,error),error);BOOST_REQUIRE_MESSAGE(city->rebuild(error),error);
    const auto overlay=ray("hover",-30,-10).getArray();BOOST_REQUIRE_EQUAL(overlay.size(),3u);const auto &controls=overlay[2].getArray();BOOST_REQUIRE(!controls.empty());
    BOOST_CHECK_SMALL(controls.front().getVector3().y-(7-.3f+.025f+.035f),.001f);
    activate("Move Knot");ray("begin",-30,-10);ray("update",-25,-15);ray("commit",-25,-15);
    BOOST_CHECK_SMALL(city->getSource().roads[0].knots[0].position.x+25,.0001f);
    {RoadTask render(TaskId::Render);BOOST_REQUIRE_MESSAGE(CityRoadEditCommand::dispatchHistory(commands,false,error),error);}
    BOOST_CHECK_SMALL(city->getSource().roads[0].knots[0].position.x+25,.0001f);
    flush();
    BOOST_CHECK( city->getSource().roads[0].knots[0].position == initialKnotPosition );
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
    const auto approachId = city->getSource().roundabouts[0].ports.front().approachId;
    const auto approach = [&]() -> const procedural::CityRoadSpline &
    {
        const auto &roads = city->getSource().roads;
        const auto found = std::find_if(roads.begin(),roads.end(),[&](const auto &road){return road.id==approachId;});
        BOOST_REQUIRE(found!=roads.end());
        return *found;
    };
    const auto originalApproach = approach();
    const auto originalJunctionId = city->getSource().roundabouts.front().junctionId;
    const auto approachMidpoint = (originalApproach.knots.front().position+originalApproach.knots.back().position)*.5f;
    activate("Select Road");ray("begin",approachMidpoint.x,approachMidpoint.z);
    SmartPtr<ui::IUILabelTextInputPair> roadWidthInput;
    for(auto element:window->getParentWindow()->getChildren())
        if(auto input=dynamic_pointer_cast<ui::IUILabelTextInputPair>(element))
            if(input->getLabel()=="Road width (m)")roadWidthInput=input;
    BOOST_REQUIRE(roadWidthInput);
    const auto checkApproachWidth = [&](f32 width)
    {
        BOOST_REQUIRE_EQUAL(city->getSource().roundabouts.size(),1u);
        BOOST_CHECK(city->getSource().roundabouts.front().junctionId==originalJunctionId);
        BOOST_CHECK_SMALL(approach().width-width,.0001f);
        BOOST_REQUIRE_EQUAL(approach().knots.size(),originalApproach.knots.size());
        for(size_t i=0;i<originalApproach.knots.size();++i)
        {
            BOOST_CHECK(approach().knots[i].id==originalApproach.knots[i].id);
            BOOST_CHECK(approach().knots[i].position==originalApproach.knots[i].position);
            BOOST_CHECK(approach().knots[i].inHandle==originalApproach.knots[i].inHandle);
            BOOST_CHECK(approach().knots[i].outHandle==originalApproach.knots[i].outHandle);
        }
        unsigned linkedPorts=0;
        for(const auto &junction:city->getSource().roundabouts)for(const auto &port:junction.ports)
            if(port.approachId==approachId){++linkedPorts;BOOST_CHECK_SMALL(port.width-width,.0001f);}
        BOOST_CHECK_EQUAL(linkedPorts,1u);
    };
    const auto beforeWidthRevision=city->getSourceRevision();
    const auto beforeWidthPublications=publications;
    roadWidthInput->setValue("7");
    {RoadTask render(TaskId::Render);activate("Apply Road Fields");}
    BOOST_CHECK_EQUAL(city->getSourceRevision(),beforeWidthRevision);
    BOOST_CHECK_EQUAL(publications,beforeWidthPublications);
    flush();
    BOOST_REQUIRE_MESSAGE(city->getSourceRevision()==beforeWidthRevision+1,diagnostic());
    BOOST_CHECK_EQUAL(publications,beforeWidthPublications+1);
    checkApproachWidth(7);
    activate("Undo");checkApproachWidth(originalApproach.width);
    activate("Redo");checkApproachWidth(7);
    const auto beforeRejectedWidth=roadJSON(city->getSource());
    const auto beforeRejectedPublications=publications;
    roadWidthInput->setValue("13"); // Valid ordinary-road width, invalid roundabout approach.
    activate("Apply Road Fields");
    BOOST_CHECK(roadJSON(city->getSource())==beforeRejectedWidth);
    BOOST_CHECK_EQUAL(publications,beforeRejectedPublications);
    checkApproachWidth(7);
    // Commit Road can also repair and publish a retained draft after rejection.
    roadWidthInput->setValue("8");
    activate("Commit Road");
    BOOST_REQUIRE_MESSAGE(publications==beforeRejectedPublications+1,diagnostic());
    checkApproachWidth(8);
    activate("Undo");checkApproachWidth(7);
    // The rejected draft did not create history: Undo still restores this edit.
    activate("Undo");checkApproachWidth(originalApproach.width);
    activate("Select Road");ray("begin",approachMidpoint.x,approachMidpoint.z);
    activate("Insert Knot");ray("begin",approachMidpoint.x,approachMidpoint.z);
    BOOST_REQUIRE_MESSAGE(approach().knots.size()==3u,diagnostic());
    BOOST_CHECK(city->getSource().roundabouts[0].ports.front().approachId==approachId);
    BOOST_CHECK(approach().knots.front().id==originalApproach.knots.front().id);
    BOOST_CHECK(approach().knots.back().id==originalApproach.knots.back().id);
    BOOST_CHECK_SMALL((approach().knots.front().outHandle.normaliseCopy()-originalApproach.knots.front().outHandle.normaliseCopy()).length(),.0001f);
    activate("Undo");BOOST_REQUIRE_EQUAL(approach().knots.size(),2u);
    activate("Move Knot");ray("begin",32,40);ray("commit",32,40);const auto beforeGuard=city->getSourceRevision();activate("Delete Selected Road");
    BOOST_CHECK_EQUAL(city->getSourceRevision(),beforeGuard);BOOST_CHECK_EQUAL(city->getSource().roads.size(),5u);
    activate("Delete Selected Roundabout");BOOST_CHECK(city->getSource().roundabouts.empty());BOOST_REQUIRE_EQUAL(city->getSource().roads.size(),1u);
    activate("Undo");BOOST_CHECK_EQUAL(city->getSource().roundabouts.size(),1u);BOOST_CHECK_EQUAL(city->getSource().roads.size(),5u);
    activate("Undo");BOOST_CHECK(city->getSource().roundabouts.empty());BOOST_REQUIRE_EQUAL(city->getSource().roads.size(),1u);

    const auto evaluate = [](const procedural::CityRoadSplineKnot &start,
                             const procedural::CityRoadSplineKnot &end, f32 t)
    {
        const f32 u = 1.f - t;
        return start.position * (u * u * u) + (start.position + start.outHandle) * (3.f * u * u * t) +
               (end.position + end.inHandle) * (3.f * u * t * t) + end.position * (t * t * t);
    };
    auto straightSource = city->getSource();
    auto &straight = straightSource.roads[0].knots;
    straight.front().outHandle = Vector3F::zero();
    straight.back().inHandle = Vector3F::zero();
    BOOST_REQUIRE_MESSAGE(city->setSourceJSON(roadJSON(straightSource),city->getSourceRevision(),error),error);
    const auto straightPick = evaluate(straight.front(),straight.back(),.25f);
    activate("Select Road");ray("begin",straightPick.x,straightPick.z);
    activate("Insert Knot");ray("begin",straightPick.x,straightPick.z);
    const auto &straightSplit = city->getSource().roads[0].knots;
    BOOST_REQUIRE_MESSAGE(straightSplit.size()==3u,diagnostic());
    BOOST_CHECK_SMALL((straightSplit[1].position-straightPick).length(),.002f);
    BOOST_CHECK(straightSplit.front().outHandle==Vector3F::zero());
    BOOST_CHECK(straightSplit.back().inHandle==Vector3F::zero());
    activate("Undo");BOOST_REQUIRE_EQUAL(city->getSource().roads[0].knots.size(),2u);

    auto curvedSource = city->getSource();
    curvedSource.roads[0].knots[0].outHandle = {15, 0, 10};
    curvedSource.roads[0].knots[1].inHandle = {-18, 0, 12};
    BOOST_REQUIRE_MESSAGE(city->setSourceJSON(roadJSON(curvedSource),city->getSourceRevision(),error),error);
    const auto &original = curvedSource.roads[0].knots;
    const auto pick = evaluate(original.front(), original.back(), .4f);
    const auto beforeInsert = city->getSourceRevision();
    const auto beforeInsertPublications = publications;
    activate("Select Road");
    const auto selectedOverlay = ray("begin",pick.x,pick.z).getArray();
    BOOST_REQUIRE_EQUAL(selectedOverlay.size(),3u);
    BOOST_CHECK_EQUAL(selectedOverlay[2].getArray().size(),original.size()*3u); // Centre and two handles per knot.
    BOOST_CHECK_EQUAL(city->getSourceRevision(),beforeInsert);
    {
        RoadTask render(TaskId::Render);
        activate("Insert Knot");
        ray("begin",pick.x,pick.z);
        ray("commit",pick.x,pick.z);
    }
    BOOST_CHECK_EQUAL(city->getSourceRevision(),beforeInsert);
    flush();
    const auto &split = city->getSource().roads[0].knots;
    BOOST_REQUIRE_MESSAGE(split.size()==3u,diagnostic());
    BOOST_CHECK_EQUAL(city->getSourceRevision(),beforeInsert+1);
    BOOST_CHECK_EQUAL(publications,beforeInsertPublications+1);
    BOOST_CHECK(city->getSource().roads[0].id==roadId);
    BOOST_CHECK(split.front().id==original.front().id && split.back().id==original.back().id);
    const auto insertedId = split[1].id;
    BOOST_CHECK(insertedId!=original.front().id && insertedId!=original.back().id);
    const f32 splitParameter = split.front().outHandle.x / original.front().outHandle.x;
    BOOST_CHECK_SMALL(splitParameter-.4f,.001f);
    BOOST_CHECK(split.front().inHandle==original.front().inHandle);
    BOOST_CHECK(split.back().outHandle==original.back().outHandle);
    for (const f32 t : {0.f,.25f,.5f,.75f,1.f})
    {
        BOOST_CHECK_SMALL((evaluate(split[0],split[1],t)-evaluate(original[0],original[1],splitParameter*t)).length(),.002f);
        BOOST_CHECK_SMALL((evaluate(split[1],split[2],t)-evaluate(original[0],original[1],splitParameter+(1.f-splitParameter)*t)).length(),.002f);
    }
    const auto afterInsert = city->getSourceRevision();
    ray("begin",100,100);
    ray("begin",original.front().position.x,original.front().position.z);
    BOOST_CHECK_EQUAL(city->getSourceRevision(),afterInsert);
    activate("Select Road");
    const auto missedOverlay = ray("begin",100,100).getArray();
    BOOST_REQUIRE_EQUAL(missedOverlay.size(),3u);
    BOOST_CHECK(missedOverlay[2].getArray().empty());
    activate("Delete Selected Road");
    BOOST_CHECK_EQUAL(city->getSourceRevision(),afterInsert);
    activate("Undo");
    BOOST_REQUIRE_EQUAL(city->getSource().roads[0].knots.size(),2u);
    BOOST_CHECK(city->getSource().roads[0].knots[0].outHandle==original[0].outHandle);
    BOOST_CHECK(city->getSource().roads[0].knots[1].inHandle==original[1].inHandle);
    activate("Redo");
    BOOST_REQUIRE_EQUAL(city->getSource().roads[0].knots.size(),3u);
    BOOST_CHECK(city->getSource().roads[0].knots[1].id==insertedId);

    const auto openSource = city->getSource();
    auto closedSource = openSource;
    auto &closedRoad = closedSource.roads[0];
    closedRoad.closed = true;
    closedRoad.width = 6;
    closedRoad.knots[0].position = {-55,7,-40};
    closedRoad.knots[1].position = {55,7,-40};
    closedRoad.knots[2].position = {0,7,55};
    for (size_t i=0;i<closedRoad.knots.size();++i)
    {
        const auto tangent = (closedRoad.knots[(i+1)%3].position-closedRoad.knots[(i+2)%3].position)/6.f;
        closedRoad.knots[i].inHandle = -tangent;
        closedRoad.knots[i].outHandle = tangent;
    }
    BOOST_REQUIRE_MESSAGE(city->setSourceJSON(roadJSON(closedSource),city->getSourceRevision(),error),error);
    const auto seamPick = evaluate(closedRoad.knots.back(),closedRoad.knots.front(),.5f);
    activate("Select Road");ray("begin",seamPick.x,seamPick.z);
    activate("Insert Knot");ray("begin",seamPick.x,seamPick.z);
    const auto &seam = city->getSource().roads[0].knots;
    BOOST_REQUIRE_MESSAGE(seam.size()==4u,diagnostic());
    BOOST_CHECK(city->getSource().roads[0].closed);
    BOOST_CHECK(seam.front().id==closedRoad.knots.front().id && seam[2].id==closedRoad.knots.back().id);
    BOOST_CHECK_SMALL((seam.back().position-seamPick).length(),.002f);
    BOOST_REQUIRE_MESSAGE(city->setSourceJSON(roadJSON(openSource),city->getSourceRevision(),error),error);

    const auto beforeHide=city->getSourceRevision();activate("Draw Road");
    {RoadTask render(TaskId::Render);ray("begin",-40,25);ray("commit",-40,25);ray("begin",-10,25);ray("commit",-10,25);ray("finish",-10,25);}
    window->setWindowVisible(false);{RoadTask render(TaskId::Render);BOOST_CHECK(ray("begin",0,0).getArray().empty());}flush();BOOST_CHECK_EQUAL(city->getSourceRevision(),beforeHide);

    window->setWindowVisible( true );
    BOOST_REQUIRE( !editor->isEditingPrefab() );
    BOOST_REQUIRE( !app->getQuit() );
    BOOST_REQUIRE( app->getJobQueue()->isRunning() );
    BOOST_REQUIRE( app->getFileSystem()->isExistingFile( scene->getFilePath() ) );
    {
        RoadEditorRunning running( *app );
        { RoadTask render( TaskId::Render ); activate( "Save Scene" ); }
        flush();
    }
    std::ifstream file(saved,std::ios::binary);const std::string serialized((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    file.close();
    BOOST_CHECK(serialized.find(std::string(roadId.data(),roadId.size()))!=std::string::npos);
    BOOST_REQUIRE_MESSAGE( serialized.find( std::string( insertedId.data(), insertedId.size() ) ) !=
        std::string::npos, "City Roads Save Scene must write the inserted knot to disk" );
    const auto expected = city->getSource().roads.front();
    const auto expectedBinding = city->getTerrainBindingUUID();
    BOOST_CHECK( applicationOnly );
    commands->clearAll();
    selection->clearSelection();
    window->setWindowVisible( false );
    window->updateSelection();
    scene->clear( true );
    auto reopened = app->getFactoryManager()->make_ptr<scene::GameScene>();
    BOOST_REQUIRE( reopened );
    reopened->load( nullptr );
    manager->setCurrentScene( reopened );
    reopened->loadScene( savedPath, false );
    BOOST_REQUIRE( reopened->getSceneLoadingState() == scene::IGameScene::SceneLoadingState::Loaded );
    auto restoredActor = reopened->findActorByName( "City tool test source" );
    auto restoredTerrainActor = reopened->findActorByName( "City tool test terrain" );
    BOOST_REQUIRE( restoredActor );
    BOOST_REQUIRE( restoredTerrainActor );
    auto restored = restoredActor->getComponent<scene::CityRoadAuthoring>();
    auto restoredTerrain = restoredTerrainActor->getComponent<scene::TerrainSystem>();
    BOOST_REQUIRE( restored );
    BOOST_REQUIRE( restoredTerrain );
    restored->setPublishCallback( []( const auto &, const auto &, auto, const auto &, String & ) {
        return true;
    } );
    if( auto renderer = dynamic_pointer_cast<render::Terrain>( restoredTerrain->getTerrain() ) )
        renderer->setDeferredTerrainPublication( false );
    flush();
    restored->update();
    BOOST_REQUIRE_MESSAGE( restored->rebuild( error ), error );
    BOOST_REQUIRE_EQUAL( restored->getSource().roads.size(), 1u );
    BOOST_CHECK_EQUAL( restored->getTerrainBindingUUID(), expectedBinding );
    BOOST_CHECK( restored->getTerrain() == restoredTerrain );
    const auto &actual = restored->getSource().roads.front();
    BOOST_CHECK( actual.id == roadId );
    BOOST_REQUIRE_EQUAL( actual.knots.size(), expected.knots.size() );
    for( size_t i = 0; i < actual.knots.size(); ++i )
    {
        BOOST_CHECK( actual.knots[i].id == expected.knots[i].id );
        BOOST_CHECK( actual.knots[i].position == expected.knots[i].position );
        BOOST_CHECK( actual.knots[i].inHandle == expected.knots[i].inHandle );
        BOOST_CHECK( actual.knots[i].outHandle == expected.knots[i].outHandle );
    }
    reopened->clear( true );
}
#endif
