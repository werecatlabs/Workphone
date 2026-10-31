#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>
#if WP_EDITOR_TESTS
#include <EditorApplication.hpp>
#include <Workphone/System/SceneResourceCompiler.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/SkeletonResourceCompiler.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/MeshResourceCompiler.hpp>
#include <WPGraphics/Resources/SkinnedMeshResource.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPLua/LuaScriptCompiler.hpp>
#include <workphone_graphics_mesh.h>
#include <workphone_graphics_renderer.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
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
#include <cstring>
#include <map>
#include <vector>

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
    struct Cleanup { fs::path root; ~Cleanup(){std::error_code error;for(fs::recursive_directory_iterator it(root,error),end;!error&&it!=end;it.increment(error)){std::error_code ignored;if(it->is_regular_file(ignored))fs::permissions(it->path(),fs::perms::owner_write,fs::perm_options::add,ignored);}fs::remove_all(root,error);} } cleanup{root};
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
    auto legacyVersion=[](const ResourceTypeID &type){return combineHash(combineHash(combineHash(14695981039346656037ull,1),1),type.value());};
    {
        auto legacy=std::make_shared<RuntimeResource>(*sceneResource);legacy->header.compilerVersion=legacyVersion(ResourceTypeID("fbscene"));
        auto graph=decodeActorGraphResource(legacy,error);BOOST_REQUIRE_MESSAGE(graph,error);BOOST_REQUIRE_MESSAGE(installCookedSceneGraph(graph,target,error),error);
        auto actor=target->findActorByName("CookedGraphRoot");BOOST_REQUIRE(actor);BOOST_CHECK(actor->getLocalPosition()==Vector3<real_Num>(2,3,4));manager->destroyActor(actor);
    }
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
    {
        auto source=runtime.loadUUID("357261b5-3e87-48d2-a3c1-2ba952c53df2",error);BOOST_REQUIRE_MESSAGE(source,error);auto legacy=std::make_shared<RuntimeResource>(*source);legacy->header.compilerVersion=legacyVersion(ResourceTypeID("prefab"));
        auto graph=decodeActorGraphResource(legacy,error);BOOST_REQUIRE_MESSAGE(graph,error);auto prefab=installCookedPrefabGraph(graph,prefabs,error);BOOST_REQUIRE_MESSAGE(prefab,error);
        auto actor=prefab->createActor();BOOST_REQUIRE(actor);BOOST_REQUIRE(actor->findChildByName("CookedGraphChild"));manager->destroyActor(actor);prefab->unload(nullptr);
    }
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

    // One actual mixed graph, with the same authored component/property schema
    // consumed by GameActorUtil. Every family is cooked before authoring storage
    // is removed; actor construction, skin sampling and Script updates run offline.
    {
        using namespace workphone::render;
        const auto mixed=root/"mixed";const auto source=mixed/"authoring"/"source";
        fs::create_directories(source);
        auto writeMixed=[&](const char *name,const String &value){std::ofstream file(source/name,std::ios::binary);file.write(value.data(),value.size());BOOST_REQUIRE(file.good());};
        auto word=[](String &bytes,u32 value){for(unsigned i=0;i<4;++i)bytes.push_back(char(value>>(i*8)));};
        auto number=[&](String &bytes,f32 value){u32 bits=0;std::memcpy(&bits,&value,4);word(bytes,bits);};
        String geometry="WPM1";
        for(u32 value:{1u,3u,1u,0u,3u,3u,1u})word(geometry,value);
        for(f32 value:{-.2f,-.2f,.5f,.2f,.2f,.5f})number(geometry,value);
        for(auto point:Array<Vector2F>{{-.2f,-.2f},{.2f,-.2f},{0,.2f}})
            for(f32 value:{point.x,point.y,.5f,0.f,0.f,-1.f,.5f,.5f})number(geometry,value);
        for(u32 value:{0u,1u,2u,0u,3u,42u})word(geometry,value);
        writeMixed("character.mesh",geometry);writeMixed("character.mesh.materials","[\"data://surface.matres\"]");
        writeMixed("character.skinres",R"({"version":1,"mesh":"data://character.mesh","skeleton":"data://rig.skelres","vertices":[{"joints":[0,0,0,0],"weights":[1,0,0,0]},{"joints":[0,0,0,0],"weights":[1,0,0,0]},{"joints":[0,0,0,0],"weights":[1,0,0,0]}]})");
        writeMixed("rig.skelres","<skeleton><bones><bone id=\"0\" name=\"Root\"><position x=\"0\" y=\"0\" z=\"0\"/></bone></bones><animations><animation name=\"Move\" length=\"1\"><tracks><track bone=\"Root\" handle=\"0\"><keyframes><keyframe time=\"0\"><translate x=\"0\" y=\"0\" z=\"0\"/></keyframe><keyframe time=\"1\"><translate x=\"2\" y=\"0\" z=\"0\"/></keyframe></keyframes></track></tracks></animation></animations></skeleton>");
        String image(18,'\0');image[2]=2;image[12]=image[14]=2;image[16]=32;image[17]=0x28;
        for(size_t pixel=0;pixel<4;++pixel){image.push_back(0);image.push_back(char(255));image.push_back(0);image.push_back(char(255));}
        writeMixed("leaf.tga",image);writeMixed("leaf.texres",R"({"version":1,"source":"data://leaf.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5})");
        writeMixed("surface.matres",R"({"version":1,"baseColour":[1,1,1,1],"metalness":0,"roughness":1,"baseColourTexture":"data://leaf.texres"})");
        writeMixed("Helper.lua","return {amount=42}");writeMixed("Actor.lua.deps","data://Helper.lua\n");
        writeMixed("Actor.lua","local helper=require('Helper'); mixedLoads=(mixedLoads or 0)+1; class 'MixedCookedActor'; function MixedCookedActor:__init(owner) self.owner=owner end; function MixedCookedActor:update() mixedTicks=(mixedTicks or 0)+helper.amount end");
        const String skinUuid="82d11978-21eb-4618-b0cb-000000000001",rigUuid="82d11978-21eb-4618-b0cb-000000000002",luaUuid="82d11978-21eb-4618-b0cb-000000000003";
        auto actorData=make_ptr<Properties>();actorData->setName("actors");actorData->setProperty("uuid",StringUtil::getUUID());actorData->setProperty("name","MixedCookedActor");
        auto component=[&](const String &type){auto value=make_ptr<Properties>();value->setName("component");value->setProperty("componentType","workphone::scene::"+type);value->setProperty("uuid",StringUtil::getUUID());actorData->addChild(value);return value;};
        auto meshData=component("Mesh");meshData->setProperty("Mesh Resource",skinUuid);meshData->setProperty("Skeleton",rigUuid);
        component("MeshRenderer");auto animatorData=component("Animator");animatorData->setProperty("autoPlay",false);animatorData->setProperty("animationTime",.25f);
        auto scriptData=component("Script");scriptData->setProperty("scriptAssetUuid",luaUuid);scriptData->setProperty("className","MixedCookedActor");scriptData->setProperty("updateInEditMode",true);
        auto document=make_ptr<Properties>();document->addChild(actorData);writeMixed("mixed.fbscene",DataUtil::toString(document.get(),true));writeMixed("mixed.prefab",DataUtil::toString(actorData.get(),true));
        {
            SceneResourceCompiler compiler;CompileContext probe;probe.resourceId=ResourceID("data://mixed.fbscene");probe.sourcePath=(source/"mixed.fbscene").u8string().c_str();DependencySet dependencies;
            BOOST_CHECK(!compiler.getDependencies(probe,dependencies,error));
            probe.assetBindings=std::make_shared<const std::map<String,ResourceID>>(std::map<String,ResourceID>{{skinUuid,ResourceID("data://wrong.wav")}});
            BOOST_CHECK(!compiler.getDependencies(probe,dependencies,error));
        }
        auto registry=std::make_shared<ResourceCompilerRegistry>();
        for(const auto &compiler:Array<std::shared_ptr<IResourceCompiler>>{std::make_shared<SceneResourceCompiler>(),std::make_shared<GraphicsResourceCompiler>(),std::make_shared<MeshResourceCompiler>(),std::make_shared<SkinnedMeshResourceCompiler>(),std::make_shared<SkeletonResourceCompiler>(),std::make_shared<LuaScriptCompiler>()})
            BOOST_REQUIRE_MESSAGE(registry->registerCompiler(compiler,&error),error);
        const ResourceID mixedId("data://mixed.fbscene"),mixedPrefabId("data://mixed.prefab");
        ResourceSystemConfig config;config.sourceRoot=source.u8string().c_str();config.compiledRoot=(mixed/"authoring"/"compiled").u8string().c_str();config.target="pc";
        config.assetBindings={{skinUuid,ResourceID("data://character.skinres")},{rigUuid,ResourceID("data://rig.skelres")},{luaUuid,ResourceID("data://Actor.lua")}};
        {
            ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());BOOST_REQUIRE_MESSAGE(author.initialize(config,error),error);
            CompilationOptions options;options.packagedBuild=true;
            const auto cooked=author.compile(mixedId,options);for(const auto &message:cooked.messages)BOOST_TEST_MESSAGE(message);BOOST_REQUIRE(cooked.succeeded());
            BOOST_REQUIRE(author.compile(mixedPrefabId,options).succeeded());
            BOOST_REQUIRE_MESSAGE(author.writeRuntimePackage((mixed/"package").u8string().c_str(),{mixedId,mixedPrefabId},{{"82d11978-21eb-4618-b0cb-000000000010",mixedId},{"82d11978-21eb-4618-b0cb-000000000011",mixedPrefabId}},error),error);author.shutdown();
            auto catalog=make_ptr<AssetDatabaseManager>();BOOST_REQUIRE(catalog->setProjectRoot(source.u8string().c_str()));catalog->loadFromFile((mixed/"authoring"/"catalog.db").u8string().c_str());BOOST_REQUIRE(catalog->getDatabase()&&catalog->getDatabase()->isLoaded());catalog->unload(nullptr);
        }
        fs::remove_all(mixed/"authoring");BOOST_REQUIRE(!fs::exists(source)&&!fs::exists(mixed/"authoring"/"catalog.db"));
        std::map<fs::path,u64> packageHashes;for(const auto &entry:fs::recursive_directory_iterator(mixed/"package"))if(entry.is_regular_file()) {u64 hash=0,size=0;BOOST_REQUIRE(CompiledResourceIO::hashFile(entry.path().u8string().c_str(),hash,size,error));packageHashes.emplace(entry.path(),hash);fs::permissions(entry.path(),fs::perms::owner_write|fs::perms::group_write|fs::perms::others_write,fs::perm_options::remove);}
        ResourceSystem offline(nullptr,nullptr);RuntimeResourceConfig runtimeConfig;runtimeConfig.target="pc";runtimeConfig.compiledRoot=(mixed/"package").u8string().c_str();runtimeConfig.manifestPath=(mixed/"package"/"runtime.wprm").u8string().c_str();BOOST_REQUIRE_MESSAGE(offline.initializeRuntime(runtimeConfig,error),error);
        auto cooked=offline.loadUUID("82d11978-21eb-4618-b0cb-000000000010",error);auto mixedGraph=decodeActorGraphResource(cooked,error);BOOST_REQUIRE_MESSAGE(mixedGraph,error);
        {
            auto legacy=std::make_shared<RuntimeResource>(*cooked);legacy->header.compilerVersion=legacyVersion(ResourceTypeID("fbscene"));BOOST_CHECK(!decodeActorGraphResource(legacy,error));
            legacy->header.installDependencies.clear();legacy->header.installArtifacts.clear();legacy->dependencies.clear();BOOST_CHECK(!decodeActorGraphResource(legacy,error));BOOST_CHECK(error.find("Legacy")!=String::npos);
        }
        auto context=createActorGraphRuntimeContext(mixedGraph,"pc",true,error);BOOST_REQUIRE_MESSAGE(context,error);BOOST_REQUIRE(context->scripts);auto authoringVM=app->getScriptManager();
        class DatabaseSpy final : public ResourceDatabase {public:size_t lookups=0;SmartPtr<ISharedObject> getObject(const workphone::UUID &id) override {++lookups;return ResourceDatabase::getObject(id);}};
        auto database=app->getResourceDatabase();auto spy=make_ptr<DatabaseSpy>();app->setResourceDatabase(spy);
        struct DatabaseScope { SmartPtr<core::IApplicationManager> app;SmartPtr<IResourceDatabase> previous;~DatabaseScope(){app->setResourceDatabase(previous);} } databaseScope{app,database};
        BOOST_REQUIRE_MESSAGE(installCookedSceneGraph(mixedGraph,target,context,error),error);BOOST_CHECK(app->getScriptManager()==authoringVM);
        auto actor=target->findActorByName("MixedCookedActor");BOOST_REQUIRE(actor);
        auto rendererComponent=actor->getComponent<scene::MeshRenderer>();auto animator=actor->getComponent<scene::Animator>();auto script=actor->getComponent<scene::Script>();BOOST_REQUIRE(rendererComponent&&animator&&script);
        auto nativeMesh=dynamic_pointer_cast<ClawMesh>(rendererComponent->getGraphicsObject());BOOST_REQUIRE(nativeMesh&&nativeMesh->hasSkinningData()&&nativeMesh->getMaterial(0));
        auto vertices=[&](){return static_cast<const wp_graphics_mesh_vertex_pnt *>(wp_graphics_mesh_get_vertices(nativeMesh->getNativeMesh()));};
        BOOST_CHECK_CLOSE(animator->getSkeleton()->getBone("Root")->getPosition().x,.5f,.001f);BOOST_CHECK_CLOSE(vertices()[0].position.x,.3f,.001f);
        script->update();
        context->scripts->loadScriptFromString("assert(mixedLoads==1 and mixedTicks==42); class 'MixedSceneProof'; function MixedSceneProof:__init(owner) owner:setName('mixed-scene-proof-passed') end");
        auto proof=make_ptr<Properties>();context->scripts->createObject("MixedSceneProof",proof);BOOST_CHECK_EQUAL(proof->getName(),"mixed-scene-proof-passed");context->scripts->destroyObject(proof);
        {
            auto properties=make_ptr<Properties>();properties->setProperty("probe",actor->getHandle()->getUUIDAsString());
            struct ResolutionScope { Properties::ObjectResolver previous;bool fallback;~ResolutionScope(){Properties::exchangeObjectResolver(std::move(previous));Properties::exchangeObjectResolverFallback(fallback);} } resolution{
                Properties::exchangeObjectResolver([actor](const String &identity)->SmartPtr<ISharedObject>{return identity==actor->getHandle()->getUUIDAsString()?actor:nullptr;}),Properties::exchangeObjectResolverFallback(false)};
            SmartPtr<scene::IGameActor> resolved;BOOST_REQUIRE(properties->getPropertyValue("probe",resolved));BOOST_CHECK(resolved==actor);
            SmartPtr<render::IMaterial> wrongType;BOOST_CHECK_THROW(properties->getPropertyValue("probe",wrongType),std::runtime_error);
            properties->setProperty("probe",StringUtil::getUUID());BOOST_CHECK_THROW(properties->getPropertyValue("probe",resolved),std::runtime_error);
        }
        const auto retainedActors=target->getActors().size();BOOST_CHECK(!installCookedSceneGraph(mixedGraph,target,context,error));BOOST_CHECK(target->getSceneLoadingState()==scene::IGameScene::SceneLoadingState::Loaded);BOOST_CHECK_EQUAL(target->getActors().size(),retainedActors);
        auto incomplete=std::make_shared<RuntimeResource>(*cooked);incomplete->dependencies.clear();BOOST_CHECK(!decodeActorGraphResource(incomplete,error));
        {
            auto properties=make_ptr<Properties>();BOOST_REQUIRE(PropertiesBinarySerializer::deserialize(cooked->payload,*properties,&error));
            auto roots=properties->getChildrenByName("actors");BOOST_REQUIRE_EQUAL(roots.size(),1);auto replacement=scene::GameActorUtil::createInstanceData(roots.front());
            for(auto component:replacement->getChildren())if(component->getProperty("componentType")=="workphone::scene::Script")component->setProperty("className","MissingCookedClass");
            auto failedData=make_ptr<Properties>();failedData->addChild(replacement);auto failed=std::make_shared<RuntimeResource>(*cooked);failed->payload=PropertiesBinarySerializer::serialize(*failedData);failed->header.payloadSize=failed->payload.size();failed->header.payloadHash=hashBytes(failed->payload.data(),failed->payload.size());
            auto failedGraph=decodeActorGraphResource(failed,error);BOOST_REQUIRE_MESSAGE(failedGraph,error);auto failedContext=createActorGraphRuntimeContext(failedGraph,"pc",true,error);BOOST_REQUIRE_MESSAGE(failedContext,error);
            BOOST_CHECK(!installCookedSceneGraph(failedGraph,target,failedContext,error));BOOST_CHECK(target->getSceneLoadingState()==scene::IGameScene::SceneLoadingState::Loaded);BOOST_CHECK_EQUAL(target->getActors().size(),retainedActors);BOOST_CHECK(target->findActorByName("MixedCookedActor")==actor);
        }
        auto prefabGraph=decodeActorGraphResource(offline.loadUUID("82d11978-21eb-4618-b0cb-000000000011",error),error);BOOST_REQUIRE_MESSAGE(prefabGraph,error);
        auto instancePrefab=installCookedPrefabGraph(prefabGraph,prefabs,context,error);BOOST_REQUIRE_MESSAGE(instancePrefab,error);
        auto first=instancePrefab->createActor(),second=instancePrefab->createActor();BOOST_REQUIRE(first&&second);BOOST_CHECK(first->getHandle()->getUUID()!=second->getHandle()->getUUID());
        BOOST_CHECK_EQUAL(first->getComponent<scene::Mesh>()->getProperties()->getProperty("Mesh Resource"),skinUuid);BOOST_CHECK_EQUAL(second->getComponent<scene::Mesh>()->getProperties()->getProperty("Mesh Resource"),skinUuid);
        BOOST_CHECK_EQUAL(first->getComponent<scene::Script>()->getScriptAssetUuid(),luaUuid);BOOST_CHECK_EQUAL(second->getComponent<scene::Script>()->getScriptAssetUuid(),luaUuid);
        {
            auto data=dynamic_pointer_cast<Properties>(instancePrefab->getData());BOOST_REQUIRE(data);auto mutated=data->getChild("component");BOOST_REQUIRE(mutated);const auto previous=mutated->getProperty("componentType");
            mutated->setProperty("componentType","AudioEmitter");BOOST_CHECK_THROW(instancePrefab->createActor(),std::runtime_error);mutated->setProperty("componentType",previous);
        }
        auto secondAnimator=second->getComponent<scene::Animator>();BOOST_REQUIRE(secondAnimator);animator->setAnimationTime(.5f);BOOST_CHECK_CLOSE(vertices()[0].position.x,.8f,.001f);BOOST_CHECK_CLOSE(secondAnimator->getSkeleton()->getBone("Root")->getPosition().x,.5f,.001f);
        {
            auto renderer=dynamic_pointer_cast<ClawRendererDX11>(app->getGraphicsSystem()->getRenderer());BOOST_REQUIRE(renderer);
            auto previousTarget=renderer->getRenderTarget();auto previousViewport=renderer->getViewport();auto previousCamera=renderer->getCamera();
            struct RenderScope {SmartPtr<ClawRendererDX11> renderer;SmartPtr<render::IRenderTarget> target;SmartPtr<render::IViewport> viewport;SmartPtr<render::IGraphicsCamera> camera;~RenderScope(){renderer->setCamera(camera);renderer->setViewport(viewport);renderer->setRenderTarget(target);}} renderScope{renderer,previousTarget,previousViewport,previousCamera};
            auto surface=make_ptr<ClawRenderTarget>();surface->setSize({64,64});renderer->setRenderTarget(surface);renderer->setViewport(nullptr);renderer->setCamera(nullptr);renderer->disableShadows();renderer->setSceneLighting(ColourF::White,{0,-1,0},ColourF::White,0);
            auto native=wp_renderer_get_dx11(renderer->getNativeRenderer());BOOST_REQUIRE(native);wp_mat4f unit{};for(unsigned axis=0;axis<4;++axis)unit.m[axis][axis]=1;
            wp_renderer_dx11_set_world_matrix(native,&unit);wp_renderer_dx11_set_view_matrix(native,&unit);wp_renderer_dx11_set_projection_matrix(native,&unit);wp_renderer_dx11_set_cull_mode(native,WORKPHONE_CULL_MODE_NONE);wp_renderer_dx11_set_depth_test_enabled(native,0);wp_renderer_dx11_set_depth_write_enabled(native,0);wp_renderer_dx11_set_blend_mode(native,WORKPHONE_BLEND_MODE_NONE);
            auto draw=[&](){
                renderer->clear(ColourF::Black);renderer->renderMesh(nativeMesh.get(),Matrix4F::identity());
                using Microsoft::WRL::ComPtr;auto device=static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));auto commands=static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(native));
                ComPtr<ID3D11RenderTargetView> view;commands->OMGetRenderTargets(1,view.GetAddressOf(),nullptr);BOOST_REQUIRE(view);ComPtr<ID3D11Resource> resource;view->GetResource(resource.GetAddressOf());ComPtr<ID3D11Texture2D> colour;BOOST_REQUIRE(SUCCEEDED(resource.As(&colour)));
                D3D11_TEXTURE2D_DESC descriptor{};colour->GetDesc(&descriptor);BOOST_REQUIRE(descriptor.Format==DXGI_FORMAT_R8G8B8A8_UNORM);descriptor.Usage=D3D11_USAGE_STAGING;descriptor.BindFlags=0;descriptor.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;BOOST_REQUIRE(SUCCEEDED(device->CreateTexture2D(&descriptor,nullptr,staging.GetAddressOf())));commands->CopyResource(staging.Get(),colour.Get());D3D11_MAPPED_SUBRESOURCE pixels{};BOOST_REQUIRE(SUCCEEDED(commands->Map(staging.Get(),0,D3D11_MAP_READ,0,&pixels)));
                std::vector<u8> result(size_t(descriptor.Width)*descriptor.Height*4);for(UINT row=0;row<descriptor.Height;++row)std::memcpy(result.data()+row*descriptor.Width*4,static_cast<const u8 *>(pixels.pData)+row*pixels.RowPitch,descriptor.Width*4);commands->Unmap(staging.Get(),0);return result;
            };
            animator->setAnimationTime(.25f);const auto before=draw();animator->setAnimationTime(.5f);const auto after=draw();size_t green=0;for(size_t pixel=0;pixel<before.size();pixel+=4)green+=before[pixel+1]>40&&before[pixel+1]>before[pixel]*2;BOOST_CHECK(green>10);BOOST_CHECK(before!=after);
        }
        BOOST_CHECK_EQUAL(spy->lookups,0);manager->destroyActor(first);manager->destroyActor(second);manager->destroyActor(actor);first=nullptr;second=nullptr;actor=nullptr;instancePrefab->unload(nullptr);
        for(const auto &entry:packageHashes){u64 hash=0,size=0;BOOST_REQUIRE(CompiledResourceIO::hashFile(entry.first.u8string().c_str(),hash,size,error));BOOST_CHECK_EQUAL(hash,entry.second);}
    }
}
#endif
