#pragma once
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/Resources/MorphResourceCompiler.hpp>
#include <WPGraphics/Resources/MeshResourceCompiler.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <workphone_graphics_mesh_writer.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstring>

namespace claw_morph_resource_contracts
{
    using namespace workphone;using namespace workphone::render;using namespace workphone::resource;namespace fs=std::filesystem;
    inline void require(bool test,const char *message){if(!test)throw std::runtime_error(message);}
    inline void require(bool test,const String &message){if(!test)throw std::runtime_error(message.c_str());}
    inline void text(const fs::path &path,const String &value){std::ofstream file(path,std::ios::binary);file.write(value.data(),value.size());require(bool(file),"Write morph fixture");}
    inline String recipe(){return R"({"version":1,"mesh":"data://base.mesh","clip":"Face","duration":1,"targets":[{"id":"Smile","deltas":[{"vertex":0,"position":[0.4,0.2,0],"normal":[1,0,0]},{"vertex":3,"position":[0,0.4,0],"normal":[0,0,0]}]}],"keys":[{"time":0,"weights":[0]},{"time":1,"weights":[1]}]})";}
    inline String changed(String value,const String &before,const String &after){const auto at=value.find(before);require(at!=String::npos,"Morph mutation token exists");value.replace(at,before.size(),after);return value;}
    inline void seal(RuntimeResource &value){value.header.payloadSize=value.payload.size();value.header.payloadHash=hashBytes(value.payload.data(),value.payload.size());}
    inline float firstX(const ClawMesh &mesh){return static_cast<const wp_graphics_mesh_vertex_pntc*>(wp_graphics_mesh_get_vertices(mesh.getNativeMesh()))[0].position.x;}
    template<class Fixture> void run(Fixture &fixture)
    {
        const auto folder=fixture.folder/"morph-contracts",source=folder/"source",package=folder/"package";fs::create_directories(source);String error;
        auto authored=std::unique_ptr<wp_graphics_mesh,decltype(&wp_graphics_mesh_destroy)>(wp_graphics_mesh_create(),wp_graphics_mesh_destroy);
        const wp_graphics_mesh_vertex_pntc vertices[]={
            {{-.4f,-.2f,.5f},{0,0,-1},{0,0},0xff112233},{{0,-.2f,.5f},{0,0,-1},{1,0},0xff334455},{{-.2f,.2f,.5f},{0,0,-1},{.5f,1},0xff556677},
            {{0,-.2f,.5f},{0,0,-1},{0,0},0xff778899},{{.4f,-.2f,.5f},{0,0,-1},{1,0},0xff99aabb},{{.2f,.2f,.5f},{0,0,-1},{.5f,1},0xffbbccdd}};
        const wp_u16 indices[]={0,1,2,3,4,5};
        require(authored&&wp_graphics_mesh_set_vertices(authored.get(),WORKPHONE_VERTEX_FORMAT_PNTC,vertices,6)&&wp_graphics_mesh_set_indices_u16(authored.get(),indices,6),"Construct real morph geometry");
        require(wp_graphics_mesh_add_submesh(authored.get(),0,3,0)>=0&&wp_graphics_mesh_add_submesh(authored.get(),3,3,0)>=0,"Morph fixture preserves two native material ranges");
        wp_graphics_mesh_compute_aabb(authored.get());require(wp_graphics_mesh_write(authored.get(),(source/"base.mesh").u8string().c_str())!=0,"Write actual native WPM1 mesh");authored.reset();text(source/"face.amorph",recipe());
        auto registry=std::make_shared<ResourceCompilerRegistry>();require(registry->registerCompiler(std::make_shared<MeshResourceCompiler>(),&error)&&registry->registerCompiler(std::make_shared<MorphResourceCompiler>(),&error),error);
        ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());ResourceSystemConfig config;config.sourceRoot=source.u8string().c_str();config.compiledRoot=(folder/"cooked").u8string().c_str();config.target="pc";
        require(author.initialize(config,error),error);CompilationOptions options;options.packagedBuild=true;const ResourceID id("data://face.amorph"),baseId("data://base.mesh");
        require(author.compile(id,options).succeeded(),"Cook native mesh and typed sparse morph closure");
        const String uuid="737e8772-f973-4e26-8ac2-2c3b54e56e3d";
        require(author.writeRuntimePackage(package.u8string().c_str(),{id},{{uuid,id}},error),error);
        MorphResourceCompiler compiler;CompileContext invalid;invalid.resourceId=ResourceID("data://bad.amorph");invalid.sourcePath=(source/"bad.amorph").u8string().c_str();
        auto reject=[&](const String &value){text(source/"bad.amorph",value);DependencySet dependencies;require(!compiler.getDependencies(invalid,dependencies,error)&&!error.empty(),"Invalid morph semantics must fail dependency scan");};
        reject(changed(recipe(),"\"version\":1","\"version\":1,\"untracked\":true"));reject(changed(recipe(),"base.mesh","base.skinres"));
        reject(changed(recipe(),"\"vertex\":3","\"vertex\":0"));reject(changed(recipe(),"\"weights\":[1]","\"weights\":[2]"));reject(changed(recipe(),"\"time\":1","\"time\":0"));
        text(source/"bad.amorph",changed(recipe(),"\"vertex\":3","\"vertex\":999"));require(!author.compile(ResourceID("data://bad.amorph"),options).succeeded(),"Morph compiler validates sparse indices against actual decoded geometry");
        text(source/"bad.amorph",changed(recipe(),"\"normal\":[1,0,0]","\"normal\":[0,0,2]"));require(!author.compile(ResourceID("data://bad.amorph"),options).succeeded(),"Morph compiler rejects normal degeneracy between valid endpoint normals");
        author.shutdown();fs::rename(source,folder/"source-unavailable");fs::rename(folder/"cooked",folder/"cooked-unavailable");
        RuntimeResourceConfig runtime;runtime.compiledRoot=package.u8string().c_str();runtime.manifestPath=(package/"runtime.wprm").u8string().c_str();runtime.target="pc";
        ResourceSystem reader(nullptr,nullptr);require(reader.initializeRuntime(runtime,error),error);auto resource=reader.loadUUID(uuid,error);require(resource!=nullptr,error);
        require(resource->dependencies.size()==1&&resource->dependencies[0]->header.resourceId==baseId&&std::memcmp(resource->payload.data(),"WPMO",4)==0,"Source-free closure contains typed weights and exact native base");
        auto base=resource->dependencies[0];auto first=make_ptr<ClawMesh>(),second=make_ptr<ClawMesh>();require(first->loadResource(base,error),error);require(second->loadResource(base,error),error);
        auto context=std::make_shared<RuntimeAssetContext>();context->resolve=[resource,uuid](const String &requested,String &failure)->std::shared_ptr<const RuntimeResource>{if(requested==uuid)return resource;failure="Unbound morph UUID";return {};};
        auto app=core::IApplicationManager::instance();auto previousGame=app->getGameManager();auto manager=make_ptr<scene::GameManager>();
        const bool addedTransform=!app->getFactoryManager()->hasFactoryById(scene::Transform::typeInfo());
        if(addedTransform)FactoryUtil::addFactory<scene::Transform>();
        struct GameScope {SmartPtr<core::IApplicationManager> app;SmartPtr<scene::IGameManager> previous;SmartPtr<scene::GameManager> manager;bool addedTransform;
            ~GameScope(){manager->unload(nullptr);app->setGameManager(previous);manager=nullptr;if(addedTransform)FactoryUtil::removeFactory<scene::Transform>();}} gameScope{app,previousGame,manager,addedTransform};
        app->setGameManager(manager);manager->load(nullptr);require(manager->isLoaded(),"Morph fixture requires the existing loaded GameManager/FSM lifecycle");
        auto actor=manager->createActor(),other=manager->createActor();require(actor&&other&&actor->isLoaded()&&other->isLoaded()&&actor->getTransform()&&other->getTransform(),"Morph fixture requires registered, fully loaded actors");
        struct Actors {SmartPtr<scene::IGameManager> manager;SmartPtr<scene::IGameActor> first,second;~Actors(){manager->destroyActor(first);manager->destroyActor(second);}} actors{manager,actor,other};
        auto renderer=make_ptr<scene::MeshRenderer>(),otherRenderer=make_ptr<scene::MeshRenderer>();renderer->setGraphicsObject(first);otherRenderer->setGraphicsObject(second);
        auto animator=make_ptr<scene::Animator>(),otherAnimator=make_ptr<scene::Animator>();actor->addComponentInstance(renderer);actor->addComponentInstance(animator);other->addComponentInstance(otherRenderer);other->addComponentInstance(otherAnimator);
        require(actor->getComponent<scene::MeshRenderer>()==renderer&&other->getComponent<scene::Animator>()==otherAnimator,"Morph components must use actual actor/manager registration");
        animator->configureRuntimeAssets(context);otherAnimator->configureRuntimeAssets(context);auto properties=make_ptr<Properties>();properties->setProperty("animationAssetUuid",uuid);animator->setProperties(properties);otherAnimator->setProperties(properties);
        animator->setAnimationTime(.5f);require(std::abs(firstX(*first)+.2f)<.0001f&&std::abs(firstX(*second)+.4f)<.0001f,"Actual UUID Animator sampling deforms only its owned native mesh");
        const auto native=first->getNativeMesh();const auto sampled=static_cast<const wp_graphics_mesh_vertex_pntc*>(wp_graphics_mesh_get_vertices(native));
        require(std::abs(sampled[0].normal.x-.4472136f)<.0001f&&std::abs(sampled[0].normal.z+.8944272f)<.0001f&&std::abs(sampled[3].position.y)<.0001f,"Sparse morph interpolates exact weights and normalizes actual native normals");
        require(sampled[0].color==vertices[0].color&&sampled[0].uv.x==vertices[0].uv.x&&sampled[0].uv.y==vertices[0].uv.y&&wp_graphics_mesh_get_index_format(native)==WORKPHONE_INDEX_FORMAT_UINT16&&wp_graphics_mesh_get_submesh_count(native)==2&&wp_graphics_mesh_get_submesh(native,1)->index_start==3,"Native publication preserves color, UVs, index width and submesh ranges");
        auto cloned=dynamic_pointer_cast<ClawMesh>(first->clone("__morph_clone"));require(cloned!=nullptr&&cloned->getNativeMesh()!=native,error);
        require(cloned->applyMorphAnimation(resource,1,1,1,error),error);require(std::abs(firstX(*cloned))<.0001f&&std::abs(firstX(*first)+.2f)<.0001f,"Cloned native morph shares immutable bind data and owns independent playback state");
        const auto previous=first->getNativeMesh();auto broken=std::make_shared<RuntimeResource>(*resource);broken->payload.back()^=1;
        require(!animator->loadResource(broken,error)&&first->getNativeMesh()==previous,"Corrupt morph replacement preserves native and component owners");
        broken=std::make_shared<RuntimeResource>(*resource);broken->payload[4]=2;seal(*broken);require(!animator->loadResource(broken,error)&&first->getNativeMesh()==previous,"Unknown morph binary version preserves last-good geometry");
        broken=std::make_shared<RuntimeResource>(*resource);++broken->header.installArtifacts[0].payloadHash;require(!animator->loadResource(broken,error)&&first->getNativeMesh()==previous,"Captured base generation pin cannot be replaced");
        require(!first->applyMorphAnimation(broken,.7f,1,1,error)&&first->getNativeMesh()==previous,"Cached native morph retains generation pin validation");
        {
            MorphClipData decoded;require(MorphResourceCodec::decode(*resource,compiler.versionFor(ResourceTypeID("amorph")),decoded,error),error);
            decoded.targets[0].deltas[0].normal={0,0,2};std::ostringstream encoded;require(MorphResourceCodec::write(decoded,encoded,error),error);const auto bytes=encoded.str();
            broken=std::make_shared<RuntimeResource>(*resource);broken->payload.resize(bytes.size());std::memcpy(broken->payload.data(),bytes.data(),bytes.size());seal(*broken);
            require(!animator->loadResource(broken,error)&&first->getNativeMesh()==previous,"Resealed normal trajectories are validated completely before native replacement");
            decoded.targets[0].deltas[0].normal={1,0,0};++decoded.info.topologyHash;std::ostringstream wrong;require(MorphResourceCodec::write(decoded,wrong,error),error);const auto mismatch=wrong.str();broken=std::make_shared<RuntimeResource>(*resource);broken->payload.resize(mismatch.size());std::memcpy(broken->payload.data(),mismatch.data(),mismatch.size());seal(*broken);
            require(!animator->loadResource(broken,error)&&first->getNativeMesh()==previous,"Resealed topology signatures must match the installed native base");
        }
        auto wrongBase=std::make_shared<RuntimeResource>(*base);++wrongBase->header.sourceHash;broken=std::make_shared<RuntimeResource>(*resource);broken->dependencies={wrongBase};require(!first->applyMorphAnimation(broken,.7f,1,1,error)&&first->getNativeMesh()==previous,"Cached morph playback still checks exact installed mesh generation");
        auto scheduler=WorldStreamingScheduler::shared();
        auto sourceReplacement=std::make_shared<RuntimeResource>(*resource);++sourceReplacement->header.sourceHash;
        {const auto stats=scheduler->stats();const auto at=size_t(StreamMemoryClass::SourceCpu);constexpr u64 headroom=8192;
            // The lease's own Source allocation precedes its reservation. Leave
            // bounded bookkeeping room; a new generation still requires more
            // than this for its instance admission, before decoded clip data.
            require(stats.limits[at]-stats.used[at]>headroom,"Morph Source pressure requires headroom");auto pressure=scheduler->reserve(StreamMemoryClass::SourceCpu,stats.limits[at]-stats.used[at]-headroom,error);require(pressure!=nullptr,error);
            const auto admitted=scheduler->stats();require(admitted.limits[at]-admitted.used[at]<8192+u64(sourceReplacement->header.resourceId.str().size())*8,"Pressure must leave less than the new native morph instance requires");
            require(!animator->loadResource(sourceReplacement,error)&&first->getNativeMesh()==previous,"Source pressure rejects a new generation before owned bind/plan allocation");pressure.reset();}
        for(const auto category:{StreamMemoryClass::WorkingCpu,StreamMemoryClass::Gpu}){const auto stats=scheduler->stats();const auto at=size_t(category);auto pressure=scheduler->reserve(category,stats.limits[at]-stats.used[at],error);require(pressure!=nullptr,error);
            require(!first->applyMorphAnimation(resource,.75f,1,1,error)&&first->getNativeMesh()==previous&&std::abs(firstX(*first)+.2f)<.0001f,"Working/GPU peak pressure preserves complete visible morph geometry");pressure.reset();}
        require(first->applyMorphAnimation(resource,.75f,1,1,error),error);require(std::abs(firstX(*first)+.1f)<.0001f,"Rejected morph frame retries successfully from immutable bind vertices");
        Array<wp_skin_vertex> skin(6);const auto current=static_cast<const wp_graphics_mesh_vertex_pntc*>(wp_graphics_mesh_get_vertices(second->getNativeMesh()));
        for(size_t i=0;i<skin.size();++i){std::memset(&skin[i],0,sizeof(skin[i]));skin[i].position=current[i].position;skin[i].normal=current[i].normal;skin[i].weights[0]=1;}
        require(second->setSkinningData(skin),"Construct native skin boundary");const auto skinMesh=second->getNativeMesh();require(!second->applyMorphAnimation(resource,.5f,1,1,error)&&second->getNativeMesh()==skinMesh&&second->hasSkinningData(),"Unsupported skin/morph composition is explicit and preserves binding");
        const auto external=first->getNativeMesh();wp_graphics_mesh_set_primitive_type(external,WORKPHONE_PRIMITIVE_TRIANGLE_STRIP);require(!first->applyMorphAnimation(resource,1,1,1,error)&&first->getNativeMesh()==external,"External native topology edits invalidate morph continuation");
        require(!first->clone("__morph_stale_clone")&&first->getNativeMesh()==external,"Cloning cannot revalidate a stale morph binding after external native topology edits");
        animator->unload(nullptr);otherAnimator->unload(nullptr);renderer->setGraphicsObject(nullptr);otherRenderer->setGraphicsObject(nullptr);actor->removeComponentInstance(animator);actor->removeComponentInstance(renderer);other->removeComponentInstance(otherAnimator);other->removeComponentInstance(otherRenderer);context.reset();resource.reset();base.reset();reader.shutdown();
        cloned->unload(nullptr);first->unload(nullptr);second->unload(nullptr);
        std::puts("PASS: source-free typed static morph, native Animator sampling, independent clones, strict pins and staged pressure rejection");
    }
}
