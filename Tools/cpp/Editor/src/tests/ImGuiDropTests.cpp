#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>

#if WP_EDITOR_TESTS
#    include <EditorApplication.hpp>
#    include <WPImGui/ImGuiDropJob.hpp>
#    include <boost/test/unit_test.hpp>
#    include <imgui.h>
#    include <imgui_internal.h>
#    include <cstring>
#    include <utility>

using namespace workphone;
using namespace workphone::editor;

namespace
{
    struct DropQueueEditor
    {
        EditorApplication app;

        ~DropQueueEditor()
        {
            if( auto application = core::IApplicationManager::instance() )
            {
                application->setQuit( true );
                application->setRunning( false );
            }
            app.unload( nullptr );
        }
    };

    struct DropQueueApplicationTask
    {
        TaskId previous = Thread::getCurrentTask();

        DropQueueApplicationTask()
        {
            Thread::setCurrentTask( TaskId::Application );
        }

        ~DropQueueApplicationTask()
        {
            Thread::setCurrentTask( previous );
        }
    };

    struct DropQueueContext
    {
        ImGuiContext *previous = ImGui::GetCurrentContext();
        ImGuiContext *synthetic = nullptr;

        ~DropQueueContext()
        {
            if( synthetic )
            {
                ImGui::DestroyContext( synthetic );
            }
            ImGui::SetCurrentContext( previous );
        }
    };

    struct DropQueueTaskOwner
    {
        SmartPtr<ITask> task;
        SmartPtr<ISharedObject> previous;

        explicit DropQueueTaskOwner( SmartPtr<ITask> applicationTask ) :
            task( applicationTask ), previous( applicationTask->getOwner() )
        {
            task->setOwner( nullptr );
        }

        ~DropQueueTaskOwner()
        {
            task->setOwner( previous );
        }
    };

    class RecordingDropTarget : public ui::IUIDropTarget
    {
    public:
        Array<String> payloads;
        Array<TaskId> tasks;

        Parameter handleEvent( EventType eventType, hash_type eventValue,
                               const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override
        {
            BOOST_CHECK( eventType == EventType::UI );
            BOOST_CHECK_EQUAL( eventValue, IEvent::handleDrop );
            BOOST_REQUIRE_EQUAL( arguments.size(), 1 );
            payloads.push_back( arguments[0].getStr() );
            tasks.push_back( Thread::getCurrentTask() );
            return Parameter();
        }
    };
}  // namespace

BOOST_AUTO_TEST_CASE( im_gui_drop_queue_delivers_distinct_gestures_test )
{
    DropQueueEditor guard;
    guard.app.setDebugMode( true );
    guard.app.setActiveThreads( 0 );
    guard.app.load( nullptr );
    BOOST_REQUIRE( guard.app.isLoaded() );
    auto application = core::IApplicationManager::instance();
    BOOST_REQUIRE( application );
    auto jobs = application->getJobQueue();
    BOOST_REQUIRE( jobs );
    BOOST_REQUIRE( jobs->isRunning() );
    BOOST_REQUIRE( application->isRunning() );
    BOOST_REQUIRE( !application->getQuit() );
    auto tasks = application->getTaskManager();
    BOOST_REQUIRE( tasks );
    auto task = tasks->getTask( TaskId::Application );
    BOOST_REQUIRE( task );
    BOOST_REQUIRE( !task->isStopped() );
    DropQueueApplicationTask applicationTask;
    DropQueueTaskOwner taskOwner( task );
    DropQueueContext context;
    auto target = workphone::make_ptr<RecordingDropTarget>();

    auto flushJobs = [&]() {
        // Task-specific jobs are stored on ITask, independently from JobQueue::hasJobs().
        task->setState( ITask::State::Idle );
        task->setNextUpdateTime( -1.0 );
        task->update();
        for( int i = 0; i < 100 && jobs->hasJobs(); ++i )
        {
            jobs->update();
        }
    };
    auto queue = [&]( const String &payload ) {
        Array<Parameter> arguments( 1 );
        arguments[0].setStr( payload );
        ui::ImGuiDropJob::queue( target, std::move( arguments ), nullptr, nullptr );
    };
    flushJobs();

    // Independent drops must never share a time-based cooldown. Back-to-back dispatches
    // also exercise the copied job payload, before either delivery is processed.
    ImGui::SetCurrentContext( nullptr );
    queue( "first mesh" );
    queue( "second prefab" );
    BOOST_CHECK( target->payloads.empty() );
    flushJobs();
    BOOST_REQUIRE_EQUAL( target->payloads.size(), 2 );
    BOOST_CHECK( target->payloads[0] == "first mesh" );
    BOOST_CHECK( target->payloads[1] == "second prefab" );

    // Model the tree row and its enclosing window observing one release. The test
    // supplies synthetic ImGui release state; delivery still uses the real job queue.
    context.synthetic = ImGui::CreateContext();
    context.synthetic->FrameCount = 1;
    context.synthetic->IO.MouseReleased[ImGuiMouseButton_Left] = true;
    context.synthetic->DragDropActive = true;
    context.synthetic->DragDropPayload.SourceId = 123;
    context.synthetic->DragDropPayload.DataFrameCount = context.synthetic->FrameCount;
    std::memcpy( context.synthetic->DragDropPayload.DataType, "_TREENODE", sizeof( "_TREENODE" ) );
    queue( "one gesture" );
    queue( "duplicate window observation" );
    flushJobs();
    BOOST_REQUIRE_EQUAL( target->payloads.size(), 3 );
    BOOST_CHECK( target->payloads[2] == "one gesture" );

    // The next release is a separate gesture even when it has the same source.
    ++context.synthetic->FrameCount;
    context.synthetic->DragDropPayload.DataFrameCount = context.synthetic->FrameCount;
    queue( "next gesture" );
    flushJobs();
    BOOST_REQUIRE_EQUAL( target->payloads.size(), 4 );
    BOOST_CHECK( target->payloads[3] == "next gesture" );
    for( auto task : target->tasks )
    {
        BOOST_CHECK( task == TaskId::Application );
    }
}

#endif
