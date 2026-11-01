#include <EditorPCH.hpp>
#if WP_EDITOR_TESTS
#include <Workphone/Workphone.hpp>
#include <EditorApplication.hpp>
#include <boost/test/unit_test.hpp>
#include "../../../../../Tests/cpp/RuntimeMediaComponentContracts.hpp"

BOOST_AUTO_TEST_CASE(runtime_media_component_test)
{
    struct Application {workphone::editor::EditorApplication value;Application(){value.setDebugMode(true);value.setActiveThreads(0);value.load(nullptr);}~Application(){value.unload(nullptr);}} application;
    namespace fs=std::filesystem;const auto parent=fs::temp_directory_path();
    const auto root=parent/fs::u8path(("wp-runtime-media-components-"+workphone::StringUtil::getUUID()).c_str());
    fs::create_directories(root);
    struct Cleanup {fs::path root,parent;~Cleanup(){std::error_code ignored;if(root.parent_path()==parent&&root.filename().u8string().find("wp-runtime-media-components-")==0)fs::remove_all(root,ignored);}} cleanup{root,parent};
    runtime_media_component_contracts::run(root);
}
#endif
