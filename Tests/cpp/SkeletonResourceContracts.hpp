#pragma once
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/System/SkeletonResourceCompiler.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstring>

namespace skeleton_resource_contracts
{
    using namespace workphone;
    inline String source()
    {
        return "<skeleton><bones>"
            "<bone id=\"0\" name=\"Root\"><position x=\"0\" y=\"0\" z=\"0\"/><rotation qw=\"1\" qx=\"0\" qy=\"0\" qz=\"0\"/></bone>"
            "<bone id=\"1\" name=\"Child\"><position x=\"0\" y=\"1\" z=\"0\"/></bone>"
            "</bones><bonehierarchy><boneparent bone=\"Child\" parent=\"Root\"/></bonehierarchy>"
            "<animations><animation name=\"Move\" length=\"1\"><tracks><track bone=\"Root\" handle=\"0\"><keyframes>"
            "<keyframe time=\"0\"><translate x=\"0\" y=\"0\" z=\"0\"/></keyframe>"
            "<keyframe time=\"1\"><translate x=\"2\" y=\"0\" z=\"0\"/></keyframe>"
            "</keyframes></track></tracks></animation></animations></skeleton>";
    }
}

BOOST_AUTO_TEST_CASE( typed_skeleton_compiler_and_owner_install_contract )
{
    using namespace workphone;
    using namespace workphone::resource;
    String error; const auto text=skeleton_resource_contracts::source();
    BOOST_REQUIRE(SkeletonResourceCompiler::validateSource(text,error));
    const auto path=std::filesystem::temp_directory_path()/std::filesystem::u8path(("wp-typed-rig-"+StringUtil::getUUID()+".skelres").c_str());
    struct Cleanup { std::filesystem::path path; ~Cleanup(){std::error_code ignored; std::filesystem::remove(path,ignored);} } cleanup{path};
    { std::ofstream file(path,std::ios::binary); file.write(text.data(),text.size()); BOOST_REQUIRE(file.good()); }
    SkeletonResourceCompiler compiler; CompileContext context; context.sourcePath=path.u8string().c_str(); context.resourceId=ResourceID("data://rig.skelres");
    std::ostringstream output; Array<String> messages;
    BOOST_REQUIRE(compiler.compile(context,output,messages)==CompilationStatus::Success);
    std::filesystem::remove(path); // The consumer must install without the source file.
    auto resource=std::make_shared<RuntimeResource>(); resource->header.resourceId=context.resourceId;
    resource->header.resourceType=ResourceTypeID("skelres"); resource->header.compilerVersion=compiler.versionFor(ResourceTypeID("skelres"));
    const auto payload=output.str(); resource->payload.resize(payload.size());
    std::memcpy(resource->payload.data(),payload.data(),payload.size());
    resource->header.payloadSize=resource->payload.size();
    resource->header.payloadHash=hashBytes(resource->payload.data(),resource->payload.size());
    resource->header.sourceHash=1;
    auto first=SkeletonResourceCompiler::decode(*resource,error),second=SkeletonResourceCompiler::decode(*resource,error);
    BOOST_REQUIRE(first && second); BOOST_REQUIRE(first->getBone("Root") && second->getBone("Root"));
    BOOST_CHECK(first!=second); first->getAnimation("Move")->apply(.5f);
    BOOST_CHECK_CLOSE(first->getBone("Root")->getPosition().x,1.0f,.001f);
    BOOST_CHECK_SMALL(second->getBone("Root")->getPosition().x,.0001f);
    BOOST_CHECK(first->getBone("Child")->getParent()==first->getBone("Root"));
    scene::Animator animator; BOOST_REQUIRE(animator.loadResource(resource,error)); const auto previous=animator.getSkeleton();
    animator.setAnimationTime(.5f);
    BOOST_CHECK_CLOSE(animator.getSkeleton()->getBone("Root")->getPosition().x,1.0f,.001f);
    auto broken=std::make_shared<RuntimeResource>(*resource); broken->header.compilerVersion=0;
    BOOST_CHECK(!animator.loadResource(broken,error)); BOOST_CHECK(animator.getSkeleton()==previous);
    BOOST_CHECK_EQUAL(animator.getAnimations().size(),1u);
    const String rigUuid="12b43688-9d32-4e85-b5dc-f8bac29dc309",meshUuid="12b43688-9d32-4e85-b5dc-f8bac29dc310";
    auto meshResource=std::make_shared<RuntimeResource>();meshResource->header.resourceType=ResourceTypeID("mesh");
    meshResource->header.resourceId=ResourceID("data://native.mesh");
    auto runtimeContext=std::make_shared<RuntimeAssetContext>();
    runtimeContext->resolve=[resource,meshResource,rigUuid,meshUuid](const String &uuid,String &error)->std::shared_ptr<const RuntimeResource>{
        if(uuid==rigUuid)return resource;if(uuid==meshUuid)return meshResource;error="Unbound fixture UUID";return {};
    };
    scene::Animator packaged;packaged.configureRuntimeAssets(runtimeContext);
    auto animationProperties=make_ptr<Properties>();animationProperties->setProperty("animationAssetUuid",rigUuid);
    animationProperties->setProperty("animationTime",.25f);packaged.setProperties(animationProperties);
    BOOST_REQUIRE(packaged.getSkeleton());BOOST_CHECK_CLOSE(packaged.getSkeleton()->getBone("Root")->getPosition().x,.5f,.001f);
    const auto packagedRig=packaged.getSkeleton();auto missing=make_ptr<Properties>();missing->setProperty("animationAssetUuid","unbound");
    BOOST_CHECK_THROW(packaged.setProperties(missing),std::runtime_error);BOOST_CHECK(packaged.getSkeleton()==packagedRig);
    auto cache=make_ptr<Properties>();cache->setProperty("animationCachePath","authoring.skeleton.xml");
    BOOST_CHECK_THROW(packaged.setProperties(cache),std::runtime_error);BOOST_CHECK(packaged.getSkeleton()==packagedRig);
    scene::Mesh meshComponent;meshComponent.configureRuntimeAssets(runtimeContext);
    auto meshProperties=make_ptr<Properties>();meshProperties->setProperty("Mesh Resource",meshUuid);meshProperties->setProperty("Skeleton",rigUuid);
    meshComponent.setProperties(meshProperties);
    BOOST_CHECK(meshComponent.getRuntimeResource()==meshResource);BOOST_CHECK(meshComponent.getRuntimeSkeletonResource()==resource);
    BOOST_CHECK(!meshComponent.getMeshResource());
    auto wrongMesh=make_ptr<Properties>();wrongMesh->setProperty("Mesh Resource",rigUuid);
    BOOST_CHECK_THROW(meshComponent.setProperties(wrongMesh),std::runtime_error);BOOST_CHECK(meshComponent.getRuntimeResource()==meshResource);
    BOOST_CHECK_THROW(meshComponent.setMeshPath("authoring.mesh"),std::runtime_error);
    meshComponent.unload(nullptr);packaged.unload(nullptr);
    BOOST_CHECK(!meshComponent.getRuntimeResource() && !meshComponent.getRuntimeSkeletonResource() && !packaged.getSkeleton());
    BOOST_CHECK_THROW(packaged.setProperties(animationProperties),std::runtime_error);
    auto corrupt=std::make_shared<RuntimeResource>(*resource); corrupt->payload[0]^=1;
    BOOST_CHECK(!SkeletonResourceCompiler::decode(*corrupt,error));
    corrupt=std::make_shared<RuntimeResource>(*resource); corrupt->header.formatVersion=0;
    BOOST_CHECK(!SkeletonResourceCompiler::decode(*corrupt,error));
    corrupt=std::make_shared<RuntimeResource>(*resource); corrupt->header.sourceHash=0;
    BOOST_CHECK(!SkeletonResourceCompiler::decode(*corrupt,error));
    corrupt=std::make_shared<RuntimeResource>(*resource); ++corrupt->header.payloadSize;
    BOOST_CHECK(!SkeletonResourceCompiler::decode(*corrupt,error));
    corrupt=std::make_shared<RuntimeResource>(*resource); corrupt->header.resourceId=ResourceID("data://rig.skelres:clip.skelres");
    BOOST_CHECK(!SkeletonResourceCompiler::decode(*corrupt,error));
    for(const String invalid:{"<skeleton><bones/></skeleton>",
        "<!DOCTYPE skeleton><skeleton><bones/></skeleton>",
        "<skeleton><bones><bone id=\"100\" name=\"Root\"/></bones></skeleton>",
        "<skeleton><bones><bone id=\"0\" name=\"Root\"><scale x=\"2\" y=\"1\" z=\"1\"/></bone></bones></skeleton>",
        "<skeleton><bones><bone id=\"0\" name=\"Root\"/></bones><animationlinks/></skeleton>"})
        BOOST_CHECK(!SkeletonResourceCompiler::validateSource(invalid,error));
    auto cycle=text; const String link="<boneparent bone=\"Root\" parent=\"Child\"/>";
    cycle.insert(cycle.find("</bonehierarchy>"),link); BOOST_CHECK(!SkeletonResourceCompiler::validateSource(cycle,error));
    auto badNumber=text; const auto position=badNumber.find("length=\"1\""); badNumber.replace(position,10,"length=\"1junk\"");
    BOOST_CHECK(!SkeletonResourceCompiler::validateSource(badNumber,error));
    auto mergedKeys=text; const auto secondKey=mergedKeys.find("<keyframe time=\"1\">");
    mergedKeys.replace(secondKey,String("<keyframe time=\"1\">").size(),"<keyframe time=\"0.00000005\">");
    BOOST_CHECK(!SkeletonResourceCompiler::validateSource(mergedKeys,error));
}
