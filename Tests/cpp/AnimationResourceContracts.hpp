#pragma once
#include <Workphone/System/AnimationResourceCompiler.hpp>
#include <Workphone/System/SkeletonResourceCompiler.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <cstring>

namespace animation_resource_contracts
{
    using namespace workphone;using namespace workphone::resource;
    inline const char *rigText()
    {
        return R"(<skeleton><bones><bone id="0" name="Root"/><bone id="1" name="Child"><position x="0" y="1" z="0"/></bone></bones>
<bonehierarchy><boneparent bone="Child" parent="Root"/></bonehierarchy><animations>
<animation name="Idle" length="1"><tracks>
<track bone="Root" handle="0"><keyframes><keyframe time="0"><translate x="0" y="0" z="0"/></keyframe><keyframe time="1"><translate x="0" y="0" z="0"/></keyframe></keyframes></track>
<track bone="Child" handle="1"><keyframes><keyframe time="0"><translate x="0" y="1" z="0"/></keyframe><keyframe time="1"><translate x="0" y="1" z="0"/></keyframe></keyframes></track></tracks></animation>
<animation name="Move" length="1"><tracks>
<track bone="Root" handle="0"><keyframes><keyframe time="0"><translate x="0" y="0" z="0"/></keyframe><keyframe time="1"><translate x="4" y="0" z="0"/></keyframe></keyframes></track>
<track bone="Child" handle="1"><keyframes><keyframe time="0"><translate x="0" y="1" z="0"/></keyframe><keyframe time="1"><translate x="8" y="1" z="0"/></keyframe></keyframes></track></tracks></animation>
</animations></skeleton>)";
    }
    inline const char *graphText()
    {
        return R"({"version":1,"rig":"data://rig.skelres","mask":"data://upper.amask","initialState":"Idle",
"clips":[{"id":"idle","clip":"Idle","speed":1,"loop":true},{"id":"move","clip":"Move","speed":1,"loop":true}],
"parameters":[{"name":"enabled","type":"bool","value":false},{"name":"blend","type":"float","value":1},{"name":"select","type":"float","value":0}],
"nodes":[{"id":"idlePose","type":"clip","clip":"idle","speed":1,"loop":true},{"id":"movePose","type":"clip","clip":"move","speed":1,"loop":true},
{"id":"blendPose","type":"blend1d","parameter":"blend","children":["idlePose","movePose"],"ranges":[{"input0":0,"input1":1,"low":0,"high":1}]},
{"id":"selectPose","type":"selector","parameter":"select","children":["idlePose","movePose"]}],
"states":[{"id":"Idle","clip":"idle","speed":1},{"id":"Move","node":"blendPose","speed":1},{"id":"Select","node":"selectPose","speed":1}],
"transitions":[{"from":"Idle","to":"Move","duration":0,"exitTime":-1,"priority":1,"synchronize":false,"conditions":[{"parameter":"enabled","op":"equal","value":true}]},
{"from":"Move","to":"Select","duration":0,"exitTime":-1,"priority":1,"synchronize":false,"conditions":[{"parameter":"select","op":"greater","value":0}]},
{"from":"Select","to":"Idle","duration":0.1,"exitTime":-1,"priority":0,"synchronize":false,"conditions":[{"parameter":"enabled","op":"equal","value":false}]}]})";
    }
    inline void save(const std::filesystem::path &path,const String &value)
    {std::ofstream file(path,std::ios::binary);file.write(value.data(),value.size());BOOST_REQUIRE(file.good());}
    inline void seal(RuntimeResource &value)
    {value.header.payloadSize=value.payload.size();value.header.payloadHash=hashBytes(value.payload.data(),value.payload.size());}
}

BOOST_AUTO_TEST_CASE(typed_animation_authoring_reference_roundtrip_contract)
{
    using namespace animation_resource_contracts;
    const String uuid="2a0b1530-9879-4d59-9c51-b29a434d0dcc";
    scene::Animator authored;auto initial=make_ptr<Properties>();initial->setProperty("animationAssetUuid",uuid,Properties::resourceStr);initial->setProperty("animationSpeed",1.5f);authored.setProperties(initial);
    auto saved=dynamic_pointer_cast<Properties>(authored.toData());BOOST_REQUIRE(saved);
    BOOST_CHECK_EQUAL(saved->getProperty("animationAssetUuid"),uuid);BOOST_CHECK_EQUAL(saved->getPropertyObject("animationAssetUuid").getTypeName(),Properties::resourceStr);
    const auto encoded=PropertiesBinarySerializer::serialize(*saved);auto decoded=make_ptr<Properties>();String error;const bool restored=PropertiesBinarySerializer::deserialize(encoded,*decoded,&error);BOOST_REQUIRE_MESSAGE(restored,error.c_str());
    scene::Animator reopened;reopened.setProperties(decoded);BOOST_CHECK_EQUAL(reopened.getProperties()->getProperty("animationAssetUuid"),uuid);BOOST_CHECK_CLOSE(reopened.getAnimationSpeed(),1.5f,.001f);
    auto partial=make_ptr<Properties>();partial->setProperty("animationSpeed",.75f);reopened.setProperties(partial);auto updated=reopened.getProperties();BOOST_CHECK_EQUAL(updated->getProperty("animationAssetUuid"),uuid);BOOST_CHECK_EQUAL(updated->getPropertyObject("animationAssetUuid").getTypeName(),Properties::resourceStr);BOOST_CHECK_CLOSE(reopened.getAnimationSpeed(),.75f,.001f);
    BOOST_CHECK(!reopened.getSkeleton()&&!reopened.hasAnimationGraph()&&reopened.getAnimations().empty()); // Persistence does not activate typed preview.
    scene::Animator legacy;legacy.setProperties(partial);BOOST_CHECK(legacy.getProperties()->getProperty("animationAssetUuid").empty());
    auto clear=make_ptr<Properties>();clear->setProperty("animationAssetUuid",String(),Properties::resourceStr);reopened.setProperties(clear);BOOST_CHECK(reopened.getProperties()->getProperty("animationAssetUuid").empty());
    unsigned resolutions=0;auto context=std::make_shared<RuntimeAssetContext>();context->resolve=[&](const String &,String &failure)->std::shared_ptr<const RuntimeResource>{++resolutions;failure="UUID absent from captured authoring-persistence test";return {};};
    scene::Animator runtime;runtime.configureRuntimeAssets(context);BOOST_CHECK_THROW(runtime.setProperties(initial),std::runtime_error);BOOST_CHECK_EQUAL(resolutions,1u);BOOST_CHECK(runtime.getProperties()->getProperty("animationAssetUuid").empty());
    auto forbidden=make_ptr<Properties>();forbidden->setProperty("animationAssetUuid",uuid);forbidden->setProperty("animationCachePath","authoring/rig.skeleton.xml");BOOST_CHECK_THROW(runtime.setProperties(forbidden),std::runtime_error);BOOST_CHECK_EQUAL(resolutions,1u);BOOST_CHECK(runtime.getProperties()->getProperty("animationAssetUuid").empty());
}

BOOST_AUTO_TEST_CASE(typed_animation_graph_mask_source_free_owner_contract)
{
    using namespace animation_resource_contracts;namespace fs=std::filesystem;
    const auto parent=fs::temp_directory_path(),folder=parent/fs::u8path(("wp-animation-family-"+StringUtil::getUUID()).c_str());fs::create_directories(folder/"source");fs::create_directories(folder/"package");
    struct Cleanup {fs::path parent,folder;~Cleanup(){std::error_code ignored;if(folder.parent_path()==parent&&folder.filename().u8string().find("wp-animation-family-")==0)fs::remove_all(folder,ignored);}} cleanup{parent,folder};
    const String rigId="data://rig.skelres",maskId="data://upper.amask",graphId="data://actor.agraph";
    save(folder/"source"/"rig.skelres",rigText());save(folder/"source"/"upper.amask",R"({"version":1,"rig":"data://rig.skelres","defaultWeight":1,"bones":[{"bone":"Root","weight":0},{"bone":"Child","weight":0.25}]})");save(folder/"source"/"actor.agraph",graphText());
    auto registry=std::make_shared<ResourceCompilerRegistry>();String error;
    BOOST_REQUIRE(registry->registerCompiler(std::make_shared<SkeletonResourceCompiler>(),&error));BOOST_REQUIRE(registry->registerCompiler(std::make_shared<AnimationResourceCompiler>(),&error));
    BOOST_REQUIRE(registry->hasCompiler(ResourceTypeID("agraph"))&&registry->hasCompiler(ResourceTypeID("amask")));
    std::map<String,String> artifacts;
    auto cook=[&](const String &id,const char *leaf){
        CompileContext context;context.resourceId=ResourceID(id);context.sourcePath=(folder/"source"/leaf).u8string().c_str();context.target="pc";context.packagedBuild=true;context.dependencyArtifacts=artifacts;
        const auto compiler=registry->getCompiler(context.resourceId.type());BOOST_REQUIRE(compiler);DependencySet dependencies;
        const bool scanned=compiler->getDependencies(context,dependencies,error);BOOST_REQUIRE_MESSAGE(scanned,error.c_str());
        std::ostringstream encoded;Array<String> messages;const auto status=compiler->compile(context,encoded,messages);
        BOOST_REQUIRE_MESSAGE(status==CompilationStatus::Success,(messages.empty()?"Animation cook failed":messages.front().c_str()));
        const auto payload=encoded.str();const auto dataPath=folder/"package"/(std::string(leaf)+".payload");{std::ofstream file(dataPath,std::ios::binary);file.write(payload.data(),payload.size());BOOST_REQUIRE(file.good());}
        CompiledResourceHeader header;header.resourceId=context.resourceId;header.resourceType=context.resourceId.type();header.compilerVersion=compiler->versionFor(header.resourceType);header.sourceHash=1;header.payloadSize=payload.size();header.payloadHash=hashBytes(payload.data(),payload.size());header.installDependencies=dependencies.installDependencies;
        const auto artifact=folder/"package"/(std::string(leaf)+".wprc");const bool written=CompiledResourceIO::writeAtomic(artifact.u8string().c_str(),header,dataPath.u8string().c_str(),error);BOOST_REQUIRE_MESSAGE(written,error.c_str());fs::remove(dataPath);artifacts[id]=artifact.u8string().c_str();
    };
    cook(rigId,"rig.skelres");cook(maskId,"upper.amask");cook(graphId,"actor.agraph");
    CompileContext invalid;invalid.resourceId=ResourceID("data://bad.agraph");invalid.sourcePath=(folder/"source"/"bad.agraph").u8string().c_str();invalid.dependencyArtifacts=artifacts;AnimationResourceCompiler compiler;
    const auto reject=[&](String value,bool dependencyPhase=true){save(fs::u8path(invalid.sourcePath.c_str()),value);DependencySet dependencies;Array<String> messages;std::ostringstream output;
        if(dependencyPhase)BOOST_CHECK_MESSAGE(!compiler.getDependencies(invalid,dependencies,error),"Unexpected accepted dependency schema: "<<value);else BOOST_CHECK_MESSAGE(compiler.compile(invalid,output,messages)==CompilationStatus::Failure,"Unexpected accepted compiled schema for "<<invalid.resourceId.str()<<": "<<value);};
    auto changed=[](String value,const String &from,const String &to){const auto at=value.find(from);BOOST_REQUIRE(at!=String::npos);value.replace(at,from.size(),to);return value;};
    reject(changed(graphText(),"\"version\":1","\"version\":1,\"unknown\":true"));
    reject(changed(graphText(),"\"children\":[\"idlePose\",\"movePose\"]","\"children\":[\"blendPose\",\"movePose\"]"));
    reject(changed(graphText(),"\"to\":\"Move\"","\"to\":\"Missing\""));
    reject(changed(graphText(),"\"initialState\":\"Idle\"","\"initialState\":\"Missing\""));
    reject(changed(graphText(),"\"op\":\"equal\",\"value\":true","\"op\":\"greater\",\"value\":true"));
    reject(changed(graphText(),"\"clip\":\"Move\"","\"clip\":\"Missing\""),false);
    invalid.resourceId=ResourceID("data://bad.amask");invalid.sourcePath=(folder/"source"/"bad.amask").u8string().c_str();
    reject(R"({"version":1,"rig":"data://rig.skelres","defaultWeight":1,"bones":[{"bone":"Missing","weight":0.5}]})",false);
    reject(R"({"version":1,"rig":"data://rig.skelres","defaultWeight":1,"bones":[{"bone":"Root","weight":1.5}]})");
    fs::remove_all(folder/"source"); // Only sealed compiled containers survive.
    std::map<String,std::shared_ptr<const RuntimeResource>> loaded;
    std::function<std::shared_ptr<const RuntimeResource>(const String&)> read=[&](const String &id){const auto existing=loaded.find(id);if(existing!=loaded.end())return existing->second;
        const auto path=artifacts.find(id);BOOST_REQUIRE(path!=artifacts.end());auto value=std::make_shared<RuntimeResource>();const bool decoded=CompiledResourceIO::read(path->second,*value,error,2u<<20);BOOST_REQUIRE_MESSAGE(decoded,error.c_str());
        for(const auto &dependency:value->header.installDependencies)value->dependencies.push_back(read(dependency.str()));loaded[id]=value;return std::shared_ptr<const RuntimeResource>(value);};
    const auto graph=read(graphId),mask=read(maskId);BOOST_CHECK_EQUAL(std::memcmp(graph->payload.data(),"WPAG",4),0);BOOST_CHECK_EQUAL(std::memcmp(mask->payload.data(),"WPAM",4),0);
    IAnimationState::BoneBlendMask weights;BOOST_REQUIRE(AnimationResourceCompiler::decodeMask(*mask,weights,error));BOOST_REQUIRE_EQUAL(weights.size(),2u);BOOST_CHECK_EQUAL(weights[0],0);BOOST_CHECK_CLOSE(weights[1],.25f,.001f);
    {AnimationGraphResource native;const bool decoded=AnimationResourceCompiler::decodeGraph(*graph,native,error);BOOST_REQUIRE_MESSAGE(decoded,error.c_str());BOOST_REQUIRE_MESSAGE(native.skeleton,"Successful graph decode must publish its native skeleton");BOOST_CHECK_MESSAGE(native.definition.isValid(),"Successful graph decode must publish its validated native evaluator definition");}
    const String uuid="4c721f54-3b86-4b4e-a4ab-1489482b0f88";auto context=std::make_shared<RuntimeAssetContext>();
    context->resolve=[graph,uuid](const String &id,String &error)->std::shared_ptr<const RuntimeResource>{if(id==uuid)return graph;error="UUID absent from captured animation graph";return {};};
    scene::Animator first,second;first.configureRuntimeAssets(context);second.configureRuntimeAssets(context);auto properties=make_ptr<Properties>();properties->setProperty("animationAssetUuid",uuid);
    BOOST_TEST_CHECKPOINT("Install first captured graph Animator");first.setProperties(properties);BOOST_TEST_CHECKPOINT("Install second captured graph Animator");second.setProperties(properties);BOOST_REQUIRE(first.hasAnimationGraph()&&second.hasAnimationGraph());BOOST_CHECK(first.getSkeleton()!=second.getSkeleton());
    auto a=first.getAnimationGraph(),b=second.getAnimationGraph();BOOST_REQUIRE(a&&b);BOOST_CHECK(!b->getBoolParameter("enabled"));a->setBoolParameter("enabled",true);a->update(.01f);BOOST_CHECK_EQUAL(a->getCurrentState(),"Move");BOOST_CHECK_EQUAL(b->getCurrentState(),"Idle");
    BOOST_REQUIRE(a->forceState("Move",.5f));first.onUpdate();BOOST_CHECK_SMALL(first.getSkeleton()->getBone("Root")->getPosition().x,.0001f);BOOST_CHECK_CLOSE(first.getSkeleton()->getBone("Child")->getPosition().x,1.0f,.001f);
    first.onUpdate();BOOST_CHECK_CLOSE(first.getSkeleton()->getBone("Child")->getPosition().x,1.0f,.001f); // No repeated-frame drift.
    a->setFloatParameter("blend",.5f);a->update(0);first.onUpdate();BOOST_CHECK_CLOSE(first.getSkeleton()->getBone("Child")->getPosition().x,.5f,.001f);
    a->setFloatParameter("select",1);BOOST_REQUIRE(a->forceState("Select",.5f));first.onUpdate();BOOST_CHECK_CLOSE(first.getSkeleton()->getBone("Child")->getPosition().x,1.0f,.001f);
    BOOST_CHECK_SMALL(second.getSkeleton()->getBone("Child")->getPosition().x,.0001f);BOOST_CHECK_EQUAL(b->getFloatParameter("blend"),1.0f);
    auto broken=std::make_shared<RuntimeResource>(*graph);broken->payload[4]=2;seal(*broken);const auto previous=first.getSkeleton();BOOST_CHECK(!first.loadResource(broken,error));BOOST_CHECK(first.getSkeleton()==previous&&first.hasAnimationGraph());
    broken=std::make_shared<RuntimeResource>(*graph);broken->payload.back()^=1;BOOST_CHECK(!first.loadResource(broken,error));BOOST_CHECK(first.getSkeleton()==previous);
    auto badMask=std::make_shared<RuntimeResource>(*mask);const u32 nan=0x7fc00000;std::memcpy(badMask->payload.data()+badMask->payload.size()-4,&nan,4);seal(*badMask);BOOST_CHECK(!AnimationResourceCompiler::decodeMask(*badMask,weights,error));
    auto wrongRig=std::make_shared<RuntimeResource>(*loaded.at(rigId));++wrongRig->header.sourceHash;badMask=std::make_shared<RuntimeResource>(*mask);badMask->dependencies={wrongRig};broken=std::make_shared<RuntimeResource>(*graph);broken->dependencies={loaded.at(rigId),badMask};BOOST_CHECK(!first.loadResource(broken,error));BOOST_CHECK(first.getSkeleton()==previous);
    auto scheduler=WorldStreamingScheduler::shared();const auto category=size_t(StreamMemoryClass::WorkingCpu),oldLimit=scheduler->stats().limits[category];
    struct Restore {std::shared_ptr<WorldStreamingScheduler> scheduler;u64 limit;~Restore(){String ignored;scheduler->setBudget(StreamMemoryClass::WorkingCpu,limit,ignored);}} restore{scheduler,oldLimit};
    BOOST_REQUIRE(scheduler->setBudget(StreamMemoryClass::WorkingCpu,scheduler->stats().used[category],error));BOOST_CHECK(!first.loadResource(graph,error));BOOST_CHECK(first.getSkeleton()==previous);BOOST_REQUIRE(scheduler->setBudget(StreamMemoryClass::WorkingCpu,oldLimit,error));
    first.unload(nullptr);second.unload(nullptr);BOOST_CHECK(!first.getSkeleton()&&!second.getSkeleton()&&!first.hasAnimationGraph());
}
