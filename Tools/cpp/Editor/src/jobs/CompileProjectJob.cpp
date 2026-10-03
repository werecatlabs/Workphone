#include <EditorPCH.hpp>
#include <jobs/CompileProjectJob.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    CompileProjectJob::CompileProjectJob() = default;

    CompileProjectJob::~CompileProjectJob() = default;

    void CompileProjectJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto editorManager = EditorManager::getSingletonPtr();
        auto project = editorManager->getProject();

        project->compile();
    }
}  // namespace workphone::editor
