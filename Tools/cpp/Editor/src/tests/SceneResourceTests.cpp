#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>
#if WP_EDITOR_TESTS
#include <EditorApplication.hpp>
#include <Workphone/System/SceneResourceCompiler.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>

using namespace workphone;
using namespace workphone::resource;

BOOST_AUTO_TEST_CASE(cooked_scene_prefab_resource_test)
{
    struct Application {
        editor::EditorApplication value;
        Application(){value.setDebugMode(true);value.setActiveThreads(0);value.load(nullptr);}
        ~Application(){value.unload(nullptr);}
    } application;
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/fs::u8path(("wp-actor-graph-"+StringUtil::getUUID()).c_str());
    BOOST_REQUIRE(!fs::exists(root));fs::create_directories(root/"source");
    struct Cleanup { fs::path root; ~Cleanup(){std::error_code error;fs::remove_all(root,error);} } cleanup{root};
    auto app=core::IApplicationManager::instancePtr();
    auto manager=app->getGameManager(); auto target=manager->getCurrentScene();
    BOOST_REQUIRE(target&&target->isLoaded());
    auto authored=manager->createActor(), child=manager->createActor();
    BOOST_REQUIRE(authored&&child);authored->setName("CookedGraphRoot");child->setName("CookedGraphChild");
    authored->setLocalPosition({2,3,4});child->setLocalPosition({1,0,0});authored->addChild(child);
    authored->getTransform()->updateWorldFromLocal();child->getTransform()->updateWorldFromLocal();
    const String identity="abcdefab-cdef-4abc-8def-abcdefabcdef";
    authored->getHandle()->setUUID(identity);
    auto prefabData=dynamic_pointer_cast<Properties>(authored->toData());
    BOOST_REQUIRE(prefabData);auto sceneData=make_ptr<Properties>();
    prefabData->setProperty("uuid",StringUtil::make_upper(identity));
    prefabData->setName("actors");sceneData->addChild(prefabData);
    auto write=[&](const char *name,const String &bytes){std::ofstream file(root/"source"/name,std::ios::binary);file.write(bytes.data(),bytes.size());BOOST_REQUIRE(file.good());};
    write("fixture.fbscene",DataUtil::toString(sceneData.get(),true));
    write("fixture.prefab",DataUtil::toString(prefabData.get(),true));
    write("bad.prefab","{\"uuid\":\"357261b5-3e87-48d2-a3c1-2ba952c53df2\",\"children\":[{\"handle_name\":\"component\",\"children\":[],\"componentType\":\"Mesh\"}]}");
    {
        SceneResourceCompiler compiler;CompileContext probe;probe.resourceId=ResourceID("data://invalid.prefab");
        probe.sourcePath=(root/"source"/"invalid.prefab").u8string().c_str();DependencySet dependencies;String error;
        const String id="357261b5-3e87-48d2-a3c1-2ba952c53df2";
        for(const auto &invalid:Array<String>{
            "{\"uuid\":\""+id+"\",\"uuid\":\""+id+"\",\"children\":[]}",
            "{\"uuid\":\""+id+"\",\"children\":[{\"handle_name\":\"child\",\"uuid\":\""+id+"\",\"children\":[]}]}",
            "{\"uuid\":\""+id+"\",\"children\":[{\"handle_name\":\"localTransform\",\"Position\":\"nan 0 0\",\"children\":[]}]}",
            "{\"uuid\":\""+id+"\",\"children\":[{\"handle_name\":\"localTransform\",\"Position\":\"1e300 0 0\",\"children\":[]}]}"})
        { write("invalid.prefab",invalid);BOOST_CHECK(!compiler.getDependencies(probe,dependencies,error)); }
        String invalidUtf8="{\"uuid\":\""+id+"\",\"label\":\"";
        invalidUtf8.push_back(static_cast<char>(0xff));invalidUtf8+="\",\"children\":[]}";
        write("invalid.prefab",invalidUtf8);BOOST_CHECK(!compiler.getDependencies(probe,dependencies,error));
        String deep="{\"uuid\":\""+StringUtil::getUUID()+"\",\"children\":[]}";
        for(size_t depth=0;depth<66;++depth)
            deep="{\"handle_name\":\"child\",\"uuid\":\""+StringUtil::getUUID()+"\",\"children\":["+deep+"]}";
        write("invalid.prefab",deep);BOOST_CHECK(!compiler.getDependencies(probe,dependencies,error));
        probe.sourcePath=(root/"source"/"fixture.prefab").u8string().c_str();
        probe.resourceId=ResourceID("data://fixture.prefab:selection.prefab");
        BOOST_CHECK(!compiler.getDependencies(probe,dependencies,error));
    }
    manager->destroyActor(authored);authored=nullptr;child=nullptr;prefabData=nullptr;sceneData=nullptr;
    String error; const ResourceID sceneId("data://fixture.fbscene"),prefabId("data://fixture.prefab");
    {
        auto registry=std::make_shared<ResourceCompilerRegistry>();
        BOOST_REQUIRE(registry->registerCompiler(std::make_shared<SceneResourceCompiler>(),&error));
        ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());ResourceSystemConfig config;
        config.sourceRoot=(root/"source").u8string().c_str();config.compiledRoot=(root/"cooked").u8string().c_str();config.target="actor-graph-contract";
        BOOST_REQUIRE_MESSAGE(author.initialize(config,error),error);
        CompilationOptions options;options.packagedBuild=true;
        const auto sceneReport=author.compile(sceneId,options);
        for(const auto &message:sceneReport.messages)BOOST_TEST_MESSAGE(message);
        BOOST_REQUIRE(sceneReport.succeeded());BOOST_REQUIRE(author.compile(prefabId,options).succeeded());
        BOOST_CHECK(!author.compile(ResourceID("data://bad.prefab"),options).succeeded());
        BOOST_REQUIRE_MESSAGE(author.writeRuntimePackage((root/"package").u8string().c_str(),{sceneId,prefabId},
            {{"357261b5-3e87-48d2-a3c1-2ba952c53df1",sceneId},{"357261b5-3e87-48d2-a3c1-2ba952c53df2",prefabId}},error),error);
        author.shutdown();
    }
    fs::rename(root/"source",root/"source-offline");fs::rename(root/"cooked",root/"cooked-offline");
    ResourceSystem runtime(nullptr,nullptr);RuntimeResourceConfig mount;mount.target="actor-graph-contract";
    mount.compiledRoot=(root/"package").u8string().c_str();mount.manifestPath=(root/"package"/"runtime.wprm").u8string().c_str();
    BOOST_REQUIRE_MESSAGE(runtime.initializeRuntime(mount,error),error);
    auto sceneResource=runtime.loadUUID("357261b5-3e87-48d2-a3c1-2ba952c53df1",error);
    BOOST_REQUIRE(sceneResource);
    {
        auto binary=make_ptr<Properties>();
        BOOST_REQUIRE(PropertiesBinarySerializer::deserialize(sceneResource->payload,*binary,&error));
        auto roots=binary->getChildrenByName("actors");BOOST_REQUIRE_EQUAL(roots.size(),1);
        String stored;BOOST_REQUIRE(roots.front()->getPropertyValue("uuid",stored));
        BOOST_CHECK_EQUAL(stored,identity);
    }
    auto graph=decodeActorGraphResource(sceneResource,error);BOOST_REQUIRE_MESSAGE(graph,error);
    BOOST_REQUIRE_MESSAGE(installCookedSceneGraph(graph,target,error),error);
    auto installed=target->findActorByName("CookedGraphRoot");BOOST_REQUIRE(installed);
    BOOST_CHECK_EQUAL(installed->getHandle()->getUUIDAsString(),identity);
    BOOST_CHECK(installed->getLocalPosition()==Vector3<real_Num>(2,3,4));
    BOOST_REQUIRE(installed->findChildByName("CookedGraphChild"));
    const auto count=target->getActors().size();
    BOOST_CHECK(!installCookedSceneGraph(graph,target,error));
    BOOST_CHECK(target->getSceneLoadingState()==scene::IGameScene::SceneLoadingState::Loaded);
    BOOST_CHECK_EQUAL(target->getActors().size(),count);
    BOOST_CHECK(target->findActorByName("CookedGraphRoot")==installed);
    BOOST_CHECK(installed->isLoaded());
    {
        auto concrete=dynamic_pointer_cast<scene::GameScene>(target);BOOST_REQUIRE(concrete);
        const auto previous=concrete->getLoadGeneration();const auto current=concrete->beginSceneLoad();
        target->setSceneLoadingState(scene::IGameScene::SceneLoadingState::Failed);
        BOOST_CHECK(!concrete->restoreReadySceneAfterFailedAppend(previous));
        BOOST_CHECK(target->getSceneLoadingState()==scene::IGameScene::SceneLoadingState::Failed);
        BOOST_CHECK(concrete->restoreReadySceneAfterFailedAppend(current));
        BOOST_CHECK(target->getSceneLoadingState()==scene::IGameScene::SceneLoadingState::Loaded);
    }
    auto invalid=std::make_shared<RuntimeResource>(*sceneResource);
    invalid->header.formatVersion=0;
    BOOST_CHECK(!decodeActorGraphResource(invalid,error));
    invalid=std::make_shared<RuntimeResource>(*sceneResource);invalid->header.sourceHash=0;
    BOOST_CHECK(!decodeActorGraphResource(invalid,error));
    invalid=std::make_shared<RuntimeResource>(*sceneResource);
    const String marker="CookedGraphRoot";
    auto label=std::search(invalid->payload.begin(),invalid->payload.end(),marker.begin(),marker.end());
    BOOST_REQUIRE(label!=invalid->payload.end());*label='B';
    BOOST_CHECK(!decodeActorGraphResource(invalid,error));
    BOOST_CHECK(error.find("integrity")!=String::npos);
    invalid=std::make_shared<RuntimeResource>(*sceneResource);++invalid->header.payloadSize;
    BOOST_CHECK(!decodeActorGraphResource(invalid,error));
    invalid=std::make_shared<RuntimeResource>(*sceneResource);invalid->payload[8]=0;
    invalid->header.payloadHash=hashBytes(invalid->payload.data(),invalid->payload.size());
    BOOST_CHECK(!decodeActorGraphResource(invalid,error));
    invalid=std::make_shared<RuntimeResource>(*sceneResource);BOOST_REQUIRE(invalid->payload.size()>32);
    for(size_t i=28;i<32;++i)invalid->payload[i]=0xff;
    invalid->header.payloadHash=hashBytes(invalid->payload.data(),invalid->payload.size());
    BOOST_CHECK(!decodeActorGraphResource(invalid,error));
    BOOST_CHECK(error.find("limit")!=String::npos);
    invalid=std::make_shared<RuntimeResource>(*sceneResource);
    label=std::search(invalid->payload.begin(),invalid->payload.end(),marker.begin(),marker.end());
    BOOST_REQUIRE(label!=invalid->payload.end());*label=0xff;
    invalid->header.payloadHash=hashBytes(invalid->payload.data(),invalid->payload.size());
    BOOST_CHECK(!decodeActorGraphResource(invalid,error));
    BOOST_CHECK(error.find("UTF-8")!=String::npos);
    invalid=std::make_shared<RuntimeResource>(*sceneResource);
    invalid->header.resourceId=ResourceID("data://fixture.fbscene:selection.fbscene");
    BOOST_CHECK(!decodeActorGraphResource(invalid,error));
    auto prefabGraph=decodeActorGraphResource(runtime.loadUUID("357261b5-3e87-48d2-a3c1-2ba952c53df2",error),error);
    BOOST_REQUIRE_MESSAGE(prefabGraph,error);
    auto prefabs=dynamic_pointer_cast<scene::GamePrefabManager>(app->getPrefabManager());BOOST_REQUIRE(prefabs);
    auto prefab=installCookedPrefabGraph(prefabGraph,prefabs,error);BOOST_REQUIRE_MESSAGE(prefab,error);
    auto first=prefab->createActor(),second=prefab->createActor();BOOST_REQUIRE(first&&second);
    BOOST_CHECK(first->getHandle()->getUUIDAsString()!=second->getHandle()->getUUIDAsString());
    BOOST_REQUIRE(first->findChildByName("CookedGraphChild"));
    manager->destroyActor(first);manager->destroyActor(second);first=nullptr;second=nullptr;
    SmartPtr<Properties> escaped;
    {
        auto data=dynamic_pointer_cast<Properties>(prefab->getData());BOOST_REQUIRE(data);
        auto children=data->getChildren();BOOST_REQUIRE(!children.empty());escaped=children.front();
    }
    prefab->unload(nullptr);
    const auto held=WorldStreamingScheduler::shared()->stats().used[size_t(StreamMemoryClass::SourceCpu)];
    escaped=nullptr;
    BOOST_CHECK(WorldStreamingScheduler::shared()->stats().used[size_t(StreamMemoryClass::SourceCpu)]<held);
}
#endif
