#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>
#include <EditorApplication.hpp>
#include <Workphone/System/UIResource.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/UI/UIManager.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>

#if WP_EDITOR_TESTS
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace workphone;
using namespace workphone::resource;
namespace
{
    namespace fs=std::filesystem;
    // Only the four remaining pure manager services are supplied. The inherited
    // core addElement() deliberately returns null, modeling an unsupported backend.
    class UnsupportedUIResourceManager final : public ui::UIManager
    {
    public:
        bool loadFont(const String &,const String &) override { return false; }
        void unloadFont(const String &,const String &) override {}
        SmartPtr<ISharedObject> getOverlay() const override { return {}; }
        void setOverlay(SmartPtr<ISharedObject>) override {}
    };
    struct UIResourceEditor
    {
        editor::EditorApplication app;
        ~UIResourceEditor()
        {
            if(auto manager=core::IApplicationManager::instancePtr()) {manager->setQuit(true);manager->setRunning(false);}
            app.unload(nullptr);
        }
    };
    struct UIResourceDirectory
    {
        fs::path parent=fs::absolute(fs::temp_directory_path()).lexically_normal();
        fs::path root=parent/fs::u8path(("workphone-ui-resource-"+StringUtil::getUUID()).c_str());
        ~UIResourceDirectory()
        {
            std::error_code ignored;
            if(root.parent_path()==parent&&root.filename().u8string().find("workphone-ui-resource-")==0)
                fs::remove_all(root,ignored);
        }
    };
    void writeUI(const fs::path &file,const char *text)
    {std::ofstream stream(file,std::ios::binary);stream<<text;BOOST_REQUIRE(stream.good());}
    void sealUI(RuntimeResource &resource)
    {resource.header.payloadSize=resource.payload.size();resource.header.payloadHash=hashBytes(resource.payload.data(),resource.payload.size());}
}

BOOST_AUTO_TEST_CASE(ui_resource_package_test)
{
    UIResourceEditor application;application.app.setActiveThreads(0);application.app.load(nullptr);
    auto app=core::IApplicationManager::instancePtr();BOOST_REQUIRE(app);auto manager=app->getUI();BOOST_REQUIRE(manager);
    UIResourceDirectory folder;const auto source=folder.root/"source",cooked=folder.root/"cooked",package=folder.root/"package";
    fs::create_directories(source);
    writeUI(source/"hud.uires",R"({"version":1,"nodes":[
      {"type":"window","id":101,"parent":-1,"label":"Typed UI package panel","position":[12,24],"size":[320,180],"visible":true,"enabled":true},
      {"type":"text","id":102,"parent":0,"label":"Source-free UI text","position":[4,8],"size":[200,20],"visible":true,"enabled":true,"textSize":18},
      {"type":"button","id":103,"parent":0,"label":"Packaged button","position":[4,40],"size":[120,28],"visible":true,"enabled":false,"textSize":16}]})");
    ui::UIResourceCompiler compiler;CompileContext invalid;invalid.resourceId=ResourceID("data://bad.uires");
    invalid.sourcePath=(source/"bad.uires").u8string().c_str();Array<String> messages;
    for(const char *bad : {R"({"version":1,"nodes":[],"script":"ignored.lua"})",
        R"({"version":1,"nodes":[{"type\u0000ignored":"window","id":1,"parent":-1,"label":"NUL field","position":[0,0],"size":[1,1],"visible":true,"enabled":true}]})",
        R"({"version":1,"nodes":[{"type":"window","id":1,"parent":0,"label":"Cycle","position":[0,0],"size":[1,1],"visible":true,"enabled":true}]})"}) {
        writeUI(source/"bad.uires",bad);std::ostringstream output;
        BOOST_CHECK(compiler.compile(invalid,output,messages)==CompilationStatus::Failure);BOOST_CHECK(!messages.empty());
    }
    // ImGui window names identify global native state even under different
    // parents. Source and sealed payload admission must reject that alias.
    const std::string repeatedWindows=R"({"version":1,"nodes":[
      {"type":"window","id":1,"parent":-1,"label":"Repeated window","position":[0,0],"size":[1,1],"visible":true,"enabled":true},
      {"type":"container","id":2,"parent":0,"label":"Content","position":[0,0],"size":[1,1],"visible":true,"enabled":true},
      {"type":"window","id":3,"parent":1,"label":"Repeated window","position":[0,0],"size":[1,1],"visible":true,"enabled":true}]})";
    messages.clear();writeUI(source/"bad.uires",repeatedWindows.c_str());std::ostringstream repeatedOutput;
    BOOST_CHECK(compiler.compile(invalid,repeatedOutput,messages)==CompilationStatus::Failure);
    BOOST_REQUIRE(!messages.empty());BOOST_CHECK(messages.back().find("window labels")!=String::npos);
    const std::string repeatedLabel="Repeated window",distinctLabel="Distinct window";
    auto distinctWindows=repeatedWindows;distinctWindows.replace(distinctWindows.rfind(repeatedLabel),repeatedLabel.size(),distinctLabel);
    writeUI(source/"bad.uires",distinctWindows.c_str());std::ostringstream distinctOutput;
    BOOST_REQUIRE(compiler.compile(invalid,distinctOutput,messages)==CompilationStatus::Success);
    const auto nestedWindowPayload=distinctOutput.str();
    auto registry=std::make_shared<ResourceCompilerRegistry>();String error;
    BOOST_REQUIRE(registry->registerCompiler(std::make_shared<ui::UIResourceCompiler>(),&error));
    ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());ResourceSystemConfig config;
    config.sourceRoot=source.u8string().c_str();config.compiledRoot=cooked.u8string().c_str();config.target="pc";
    BOOST_REQUIRE_MESSAGE(author.initialize(config,error),error.c_str());CompilationOptions options;options.packagedBuild=true;
    const ResourceID id("data://hud.uires");const auto report=author.compile(id,options);BOOST_REQUIRE(report.succeeded());
    const String uuid="478c01a9-439a-4884-9dda-0ea981db7e11";
    BOOST_REQUIRE_MESSAGE(author.writeRuntimePackage(package.u8string().c_str(),{id},{{uuid,id}},error),error.c_str());author.shutdown();
    // These fresh fixture-owned paths are immediate children of the verified root.
    BOOST_REQUIRE(source.parent_path()==folder.root&&cooked.parent_path()==folder.root);
    fs::remove_all(source);fs::remove_all(cooked);
    ResourceSystem runtime(nullptr,nullptr);RuntimeResourceConfig mount;
    mount.compiledRoot=package.u8string().c_str();mount.manifestPath=(package/"runtime.wprm").u8string().c_str();mount.target="pc";
    BOOST_REQUIRE_MESSAGE(runtime.initializeRuntime(mount,error),error.c_str());auto resource=runtime.loadUUID(uuid,error);BOOST_REQUIRE_MESSAGE(resource,error.c_str());
    const auto baseline=manager->getChildObjects().size();auto host=manager->addElement(ui::IUIWindow::typeInfo());BOOST_REQUIRE(host);host->setLabel("Typed UI package host");
    auto instance=ui::stageUIResource(manager,resource,error);BOOST_REQUIRE_MESSAGE(instance,error.c_str());
    BOOST_REQUIRE(instance->root());BOOST_CHECK(!instance->root()->isVisible());BOOST_CHECK_EQUAL(instance->root()->getChildren().size(),2);
    auto foreign=manager->addElement(ui::IUIWindow::typeInfo());BOOST_REQUIRE(foreign);manager->removeElement(foreign);
    BOOST_CHECK(!instance->publish(foreign,error));BOOST_CHECK(!error.empty());BOOST_CHECK(!instance->root()->isVisible());
    BOOST_CHECK(foreign->getChildren().empty());foreign=nullptr;
    BOOST_CHECK(!instance->publish(instance->root(),error));BOOST_CHECK(!instance->root()->isVisible());
    BOOST_REQUIRE_MESSAGE(instance->publish(host,error),error.c_str());BOOST_CHECK_EQUAL(host->getChildren().size(),1);
    BOOST_CHECK(instance->root()->isVisible());BOOST_CHECK_EQUAL(instance->root()->getElementId(),101);
    auto children=instance->root()->getChildren();auto text=workphone::dynamic_pointer_cast<ui::IUIText>(children[0]);BOOST_REQUIRE(text);
    BOOST_CHECK_EQUAL(text->getText(),String("Source-free UI text"));BOOST_CHECK_EQUAL(text->getTextSize(),18);
    BOOST_CHECK_EQUAL(children[1]->getLabel(),String("Packaged button"));BOOST_CHECK(!children[1]->isEnabled());
    BOOST_CHECK_EQUAL(instance->root()->getPosition().X(),12);BOOST_CHECK_EQUAL(instance->root()->getSize().Y(),180);
    const auto installedCount=manager->getChildObjects().size();auto corrupt=std::make_shared<RuntimeResource>(*resource);corrupt->payload.pop_back();sealUI(*corrupt);
    {
        auto duplicate=ui::stageUIResource(manager,resource,error);BOOST_REQUIRE_MESSAGE(duplicate,error.c_str());
        BOOST_CHECK(!duplicate->publish(host,error));BOOST_CHECK(!error.empty());BOOST_CHECK(!duplicate->root()->isVisible());
        BOOST_CHECK_EQUAL(host->getChildren().size(),1);duplicate.reset();BOOST_CHECK_EQUAL(manager->getChildObjects().size(),installedCount);
        auto removed=ui::stageUIResource(manager,resource,error);BOOST_REQUIRE_MESSAGE(removed,error.c_str());
        manager->removeElement(removed->root());BOOST_CHECK(!removed->publish(host,error));BOOST_CHECK(error.find("removed")!=String::npos);
        BOOST_CHECK_EQUAL(host->getChildren().size(),1);BOOST_CHECK(instance->root()->isVisible());removed.reset();
        BOOST_CHECK_EQUAL(manager->getChildObjects().size(),installedCount);
    }
    BOOST_CHECK(!ui::stageUIResource(manager,corrupt,error));BOOST_CHECK_EQUAL(manager->getChildObjects().size(),installedCount);
    auto duplicateWindows=std::make_shared<RuntimeResource>(*resource);duplicateWindows->payload.clear();
    for(unsigned char byte:nestedWindowPayload)duplicateWindows->payload.push_back(byte);
    auto nestedLabel=std::search(duplicateWindows->payload.begin(),duplicateWindows->payload.end(),distinctLabel.begin(),distinctLabel.end());
    BOOST_REQUIRE(nestedLabel!=duplicateWindows->payload.end());BOOST_REQUIRE_EQUAL(repeatedLabel.size(),distinctLabel.size());
    std::copy(repeatedLabel.begin(),repeatedLabel.end(),nestedLabel);sealUI(*duplicateWindows);
    BOOST_CHECK(!ui::stageUIResource(manager,duplicateWindows,error));BOOST_CHECK(error.find("window labels")!=String::npos);
    BOOST_CHECK_EQUAL(manager->getChildObjects().size(),installedCount);
    auto staleHash=std::make_shared<RuntimeResource>(*resource);BOOST_REQUIRE(staleHash->payload.size()>27);staleHash->payload[27]^=1;
    BOOST_CHECK(!ui::stageUIResource(manager,staleHash,error));BOOST_CHECK_EQUAL(manager->getChildObjects().size(),installedCount);
    auto unsupportedGraph=std::make_shared<RuntimeResource>(*resource);unsupportedGraph->header.installDependencies.push_back(ResourceID("data://ignored.lua"));
    BOOST_CHECK(!ui::stageUIResource(manager,unsupportedGraph,error));BOOST_CHECK_EQUAL(manager->getChildObjects().size(),installedCount);
    for(u8 invalidByte : {u8(0),u8(0xff)}) {
        auto invalidLabel=std::make_shared<RuntimeResource>(*resource);BOOST_REQUIRE(invalidLabel->payload.size()>27);
        invalidLabel->payload[27]=invalidByte;sealUI(*invalidLabel);BOOST_CHECK(!ui::stageUIResource(manager,invalidLabel,error));
        BOOST_CHECK_EQUAL(manager->getChildObjects().size(),installedCount);
    }
    BOOST_CHECK_EQUAL(host->getChildren().size(),1);BOOST_CHECK(instance->root()->isVisible());
    auto unsupported=make_ptr<UnsupportedUIResourceManager>();BOOST_CHECK(!ui::stageUIResource(unsupported,resource,error));BOOST_CHECK(!error.empty());
    unsupported->setLoadingState(LoadingState::Loaded);BOOST_CHECK(!ui::stageUIResource(unsupported,resource,error));BOOST_CHECK(!error.empty());
    unsupported->setLoadingState(LoadingState::Unloaded);unsupported=nullptr;
    text=nullptr;children.clear();instance.reset();BOOST_CHECK(host->getChildren().empty());
    manager->removeElement(host);host=nullptr;BOOST_CHECK_EQUAL(manager->getChildObjects().size(),baseline);
}
#endif
