#pragma once
#include "AnimationResourceContracts.hpp"
#include <Workphone/System/RetargetResourceCompiler.hpp>

namespace retarget_resource_contracts
{
    using namespace workphone;using namespace workphone::resource;
    inline const char *targetText()
    {return R"(<skeleton><bones><bone id="0" name="Base"><position x="10" y="0" z="0"/><rotation qw="0.70710678" qx="0" qy="0" qz="0.70710678"/></bone><bone id="1" name="Tip"><position x="0" y="3" z="0"/></bone></bones><bonehierarchy><boneparent bone="Tip" parent="Base"/></bonehierarchy></skeleton>)";}
    inline const char *definition()
    {return R"({"version":1,"source":"data://source.skelres","target":"data://target.skelres","clips":[{"source":"Move","target":"Stride"}],"bones":[{"source":"Root","target":"Base","translationScale":2},{"source":"Child","target":"Tip","translationScale":0.5}]})";}
}

BOOST_AUTO_TEST_CASE(typed_local_bind_retarget_source_free_pose_contract)
{
    using namespace retarget_resource_contracts;namespace fs=std::filesystem;
    const auto parent=fs::temp_directory_path(),folder=parent/fs::u8path(("wp-retarget-family-"+StringUtil::getUUID()).c_str());fs::create_directories(folder/"source");fs::create_directories(folder/"package");
    struct Cleanup {fs::path parent,folder;~Cleanup(){std::error_code ignored;if(folder.parent_path()==parent&&folder.filename().u8string().find("wp-retarget-family-")==0)fs::remove_all(folder,ignored);}} cleanup{parent,folder};
    auto changed=[](String value,const String &from,const String &to){const auto at=value.find(from);BOOST_REQUIRE(at!=String::npos);value.replace(at,from.size(),to);return value;};
    const String sourceText=changed(animation_resource_contracts::rigText(),"<translate x=\"4\" y=\"0\" z=\"0\"/>","<translate x=\"4\" y=\"0\" z=\"0\"/><rotation qw=\"0.70710678\" qx=\"0\" qy=\"0\" qz=\"0.70710678\"/>");
    animation_resource_contracts::save(folder/"source"/"source.skelres",sourceText);animation_resource_contracts::save(folder/"source"/"target.skelres",targetText());animation_resource_contracts::save(folder/"source"/"move.aretgt",definition());
    ResourceCompilerRegistry registry;String error;BOOST_REQUIRE(registry.registerCompiler(std::make_shared<SkeletonResourceCompiler>(),&error));BOOST_REQUIRE(registry.registerCompiler(std::make_shared<RetargetResourceCompiler>(),&error));BOOST_REQUIRE(registry.hasCompiler(ResourceTypeID("aretgt")));
    std::map<String,String> artifacts;
    const String sourceId="data://source.skelres",targetId="data://target.skelres",retargetId="data://move.aretgt";
    auto cook=[&](const String &id,const char *leaf){CompileContext context;context.resourceId=ResourceID(id);context.sourcePath=(folder/"source"/leaf).u8string().c_str();context.dependencyArtifacts=artifacts;context.target="pc";context.packagedBuild=true;
        const auto compiler=registry.getCompiler(context.resourceId.type());BOOST_REQUIRE(compiler);DependencySet dependencies;const bool scanned=compiler->getDependencies(context,dependencies,error);BOOST_REQUIRE_MESSAGE(scanned,error.c_str());
        std::ostringstream output;Array<String> messages;const auto status=compiler->compile(context,output,messages);BOOST_REQUIRE_MESSAGE(status==CompilationStatus::Success,(messages.empty()?"Retarget fixture cook failed":messages.front().c_str()));const auto bytes=output.str();
        const auto payload=folder/"package"/(std::string(leaf)+".payload");{std::ofstream file(payload,std::ios::binary);file.write(bytes.data(),bytes.size());BOOST_REQUIRE(file.good());}
        CompiledResourceHeader header;header.resourceId=context.resourceId;header.resourceType=context.resourceId.type();header.compilerVersion=compiler->versionFor(header.resourceType);header.sourceHash=1;header.payloadSize=bytes.size();header.payloadHash=hashBytes(bytes.data(),bytes.size());header.installDependencies=dependencies.installDependencies;
        const auto artifact=folder/"package"/(std::string(leaf)+".wprc");const bool written=CompiledResourceIO::writeAtomic(artifact.u8string().c_str(),header,payload.u8string().c_str(),error);BOOST_REQUIRE_MESSAGE(written,error.c_str());fs::remove(payload);artifacts[id]=artifact.u8string().c_str();};
    cook(sourceId,"source.skelres");cook(targetId,"target.skelres");cook(retargetId,"move.aretgt");
    RetargetResourceCompiler compiler;CompileContext bad;bad.resourceId=ResourceID("data://bad.aretgt");bad.sourcePath=(folder/"source"/"bad.aretgt").u8string().c_str();bad.dependencyArtifacts=artifacts;
    auto reject=[&](const String &text,bool scan){animation_resource_contracts::save(fs::u8path(bad.sourcePath.c_str()),text);DependencySet dependencies;if(scan)BOOST_CHECK(!compiler.getDependencies(bad,dependencies,error));else{Array<String> messages;std::ostringstream output;BOOST_CHECK(compiler.compile(bad,output,messages)==CompilationStatus::Failure);}};
    reject(changed(definition(),"\"version\":1","\"version\":1,\"solver\":\"humanoid\""),true);
    reject(changed(definition(),"\"translationScale\":2","\"translationScale\":-1"),true);
    reject(changed(definition(),"\"target\":\"Tip\"","\"target\":\"Base\""),true);
    reject(changed(definition(),"\"source\":\"Move\"","\"source\":\"Missing\""),false);
    reject(changed(definition(),"\"source\":\"Root\"","\"source\":\"Missing\""),false);
    reject(changed(definition(),"\"target\":\"Base\",\"translationScale\":2","\"target\":\"Tip\",\"translationScale\":2"),false);
    reject(R"({"version":1,"source":"data://source.skelres","target":"data://target.skelres","clips":[{"source":"Move","target":"Stride"}],"bones":[{"source":"Child","target":"Tip","translationScale":1}]})",false);
    // Both source rigs are individually valid, but their transferred key would
    // exceed the native finite-position policy; cooking must fail before install.
    const auto normalTarget=artifacts.at(targetId);animation_resource_contracts::save(folder/"source"/"large.skelres",changed(targetText(),"x=\"10\"","x=\"999999\""));cook(targetId,"large.skelres");bad.dependencyArtifacts=artifacts;reject(definition(),false);artifacts[targetId]=normalTarget;
    fs::remove_all(folder/"source");
    auto load=[&](const String &id){auto value=std::make_shared<RuntimeResource>();const bool loaded=CompiledResourceIO::read(artifacts.at(id),*value,error,2u<<20);BOOST_REQUIRE_MESSAGE(loaded,error.c_str());return value;};
    auto from=load(sourceId),to=load(targetId),resource=load(retargetId);resource->dependencies={from,to};BOOST_CHECK_EQUAL(std::memcmp(resource->payload.data(),"WPRT",4),0);
    ResourceID targetIdentity;BOOST_REQUIRE(RetargetResourceCompiler::targetRigIdentity(*resource,targetIdentity,error));BOOST_CHECK(targetIdentity==ResourceID(targetId));
    const String uuid="29e76fa2-99ce-4c89-a1b8-c0432b6cc1bc";auto context=std::make_shared<RuntimeAssetContext>();context->resolve=[resource,uuid](const String &id,String &error)->std::shared_ptr<const RuntimeResource>{if(id==uuid)return resource;error="UUID outside captured retarget package";return {};};
    scene::Animator first,second;first.configureRuntimeAssets(context);second.configureRuntimeAssets(context);auto a=make_ptr<Properties>();a->setProperty("animationAssetUuid",uuid);a->setProperty("animationTime",.5f);first.setProperties(a);auto b=make_ptr<Properties>();b->setProperty("animationAssetUuid",uuid);second.setProperties(b);
    BOOST_REQUIRE(first.getSkeleton()&&second.getSkeleton());BOOST_CHECK(first.getSkeleton()!=second.getSkeleton());BOOST_REQUIRE_EQUAL(first.getAnimations().size(),1u);BOOST_CHECK(!first.hasAnimationGraph());
    BOOST_CHECK_CLOSE(first.getSkeleton()->getBone("Base")->getPosition().x,14.0f,.001f);BOOST_CHECK_CLOSE(first.getSkeleton()->getBone("Tip")->getPosition().x,2.0f,.001f);BOOST_CHECK_CLOSE(first.getSkeleton()->getBone("Tip")->getPosition().y,3.0f,.001f);
    const auto rotation=first.getSkeleton()->getBone("Base")->getOrientation();BOOST_CHECK_CLOSE(std::abs(rotation.w),.382683432,.001);BOOST_CHECK_CLOSE(std::abs(rotation.z),.923879533,.001);
    BOOST_CHECK_CLOSE(second.getSkeleton()->getBone("Base")->getPosition().x,10.0f,.001f);BOOST_CHECK_SMALL(second.getSkeleton()->getBone("Tip")->getPosition().x,.0001f);
    first.setAnimationTime(.75f);BOOST_CHECK_CLOSE(first.getSkeleton()->getBone("Base")->getPosition().x,16.0f,.001f);BOOST_CHECK_CLOSE(second.getSkeleton()->getBone("Base")->getPosition().x,10.0f,.001f);
    const auto previous=first.getSkeleton();auto corrupt=std::make_shared<RuntimeResource>(*resource);corrupt->payload.back()^=1;BOOST_CHECK(!first.loadResource(corrupt,error));BOOST_CHECK(first.getSkeleton()==previous);
    corrupt=std::make_shared<RuntimeResource>(*resource);const u32 nan=0x7fc00000;std::memcpy(corrupt->payload.data()+corrupt->payload.size()-4,&nan,4);animation_resource_contracts::seal(*corrupt);BOOST_CHECK(!first.loadResource(corrupt,error));BOOST_CHECK(first.getSkeleton()==previous);
    corrupt=std::make_shared<RuntimeResource>(*resource);corrupt->header.formatVersion=2;corrupt->header.installArtifacts={{"artifacts/v2/source.bin",from->header.sourceHash,from->header.payloadHash},{"artifacts/v2/target.bin",to->header.sourceHash,to->header.payloadHash^1}};BOOST_CHECK(!first.loadResource(corrupt,error));BOOST_CHECK(first.getSkeleton()==previous);
    auto scheduler=WorldStreamingScheduler::shared();const auto category=size_t(StreamMemoryClass::SourceCpu),limit=scheduler->stats().limits[category];struct Restore {std::shared_ptr<WorldStreamingScheduler> scheduler;u64 limit;~Restore(){String ignored;scheduler->setBudget(StreamMemoryClass::SourceCpu,limit,ignored);}} restore{scheduler,limit};
    BOOST_REQUIRE(scheduler->setBudget(StreamMemoryClass::SourceCpu,scheduler->stats().used[category],error));BOOST_CHECK(!first.loadResource(resource,error));BOOST_CHECK(first.getSkeleton()==previous);BOOST_REQUIRE(scheduler->setBudget(StreamMemoryClass::SourceCpu,limit,error));
    first.unload(nullptr);second.unload(nullptr);BOOST_CHECK(!first.getSkeleton()&&!second.getSkeleton());
}
