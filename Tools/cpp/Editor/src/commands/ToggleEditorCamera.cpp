#include <EditorPCH.hpp>
#include "ToggleEditorCamera.hpp"
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, ToggleEditorCamera, Command );

    ToggleEditorCamera::ToggleEditorCamera() = default;

    ToggleEditorCamera::~ToggleEditorCamera() = default;

    void ToggleEditorCamera::undo()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        applicationManager->setEditorCamera( !getToggleValue() );

        auto taskManager = applicationManager->getTaskManager();
        auto applicationTask = taskManager->getTask( TaskId::Application );
        auto job = workphone::make_ptr<CameraManagerReset>();
        applicationTask->addJob( job );
    }

    void ToggleEditorCamera::redo()
    {
        execute();
    }

    void ToggleEditorCamera::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto jobQueue = applicationManager->getJobQueue();
        auto taskManager = applicationManager->getTaskManager();
        auto applicationTask = taskManager->getTask( TaskId::Application );

        auto editorCamera = getToggleValue();
        applicationManager->setEditorCamera( editorCamera );

        auto job = workphone::make_ptr<CameraManagerReset>();
        applicationTask->addJob( job );
    }

    bool ToggleEditorCamera::getToggleValue() const
    {
        return m_toggleValue;
    }

    void ToggleEditorCamera::setToggleValue( bool toggleValue )
    {
        m_toggleValue = toggleValue;
    }
}  // namespace workphone::editor
