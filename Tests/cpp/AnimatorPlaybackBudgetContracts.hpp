#pragma once
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Mesh/Bone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/MeshResource.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Mesh/VertexBoneAssignment.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <workphone_graphics_mesh.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <stdexcept>

namespace animator_playback_budget_contracts
{
    using namespace workphone;
    inline void require(bool condition,const char *message)
    { if(!condition) throw std::runtime_error(message); }
    struct Budget
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler=WorldStreamingScheduler::shared();
        static constexpr auto source=size_t(StreamMemoryClass::SourceCpu);
        u64 original=scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set(u64 limit) { String error; require(scheduler->setBudget(StreamMemoryClass::SourceCpu,limit,error),"Animator budget applies"); }
        ~Budget(){set(original);}
    };
    class Timer : public TimerMT { public: f64 getDeltaTime() const override {return .25;} };
    struct TimerScope
    {
        SmartPtr<core::IApplicationManager> app=core::IApplicationManager::instance();
        SmartPtr<ITimer> previous=app->getTimer();
        SmartPtr<Timer> timer=make_ptr<Timer>();
        TimerScope(){timer->load(nullptr);app->setTimer(timer);}
        ~TimerScope(){app->setTimer(previous);timer->unload(nullptr);}
    };
    inline SmartPtr<scene::GameActor> actor(const char *name)
    {
        auto result=dynamic_pointer_cast<scene::GameActor>(core::IApplicationManager::instance()->getGameManager()->createActor());
        require(result!=nullptr,"Actor fixture registers real game-manager handles");
        result->setName(name); return result;
    }
    inline void inventories()
    {
        Budget budget;
        scene::Animator animator;
        auto first=make_ptr<workphone::Animation>();first->setLength(1);
        auto second=make_ptr<workphone::Animation>();second->setLength(1);
        Array<SmartPtr<IAnimation>> raw;raw.reserve(17);raw.push_back(first);
        animator.setAnimations(raw);
        auto snapshot=animator.getAnimations();
        require(snapshot.get_allocator().isSourceBudgeted() && snapshot.capacity()==17,
                "Animator imports and snapshots retain admitted caller spare capacity");
        Array<SmartPtr<IAnimation>> replacement;replacement.reserve(29);replacement.push_back(second);
        const auto resident=budget.used();budget.set(resident);
        bool denied=false;try{animator.setAnimations(replacement);}catch(const std::bad_alloc&){denied=true;}
        require(denied && animator.getSelectedAnimation()==first && animator.getNumAnimationClips()==1 && budget.used()==resident,
                "Denied runtime inventory replacement preserves selection, owners and exact residency");
        budget.set(budget.original);animator.setAnimations(replacement);
        auto next=animator.getAnimations();
        require(next.capacity()==29 && next[0]==second && snapshot[0]==first,
                "Retry imports the new generation without invalidating caller snapshots");
        Array<SmartPtr<scene::Animation>> descriptors;descriptors.reserve(11);descriptors.push_back(make_ptr<scene::Animation>());
        animator.setAnimationClips(descriptors);
        auto retained=animator.getAnimationClips();
        require(retained.get_allocator().isSourceBudgeted() && retained.capacity()==11,"Descriptor spare is admitted");
        const auto before=budget.used();budget.set(before);const auto epoch=sourceBudgetDenialCount();
        animator.clearAnimations();
        require(animator.getNumAnimationClips()==0 && animator.isStopped() && snapshot[0]==first && retained.size()==1 &&
                budget.used()<before && sourceBudgetDenialCount()==epoch,"At-cap clear detaches both inventories without replacement admission");
        budget.set(budget.original);
    }
    inline void cachedPlayback()
    {
        TimerScope timer;
        Budget budget;
        const auto path=std::filesystem::temp_directory_path()/"wp-animator-playback-contract.skeleton.xml";
        struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}} cleanup{path};
        {std::ofstream file(path);file << R"(<skeleton><bones>
<bone id="0" name="Root"><position x="0" y="0" z="0"/><rotation angle="0"><axis x="0" y="1" z="0"/></rotation></bone>
<bone id="1" name="Tip"><position x="0" y="1" z="0"/><rotation angle="0"><axis x="0" y="1" z="0"/></rotation></bone>
</bones><bonehierarchy><boneparent bone="Tip" parent="Root"/></bonehierarchy>
<animations><animation name="Move" length="1"><tracks><track bone="Root"><keyframes>
<keyframe time="0"><translate x="0" y="0" z="0"/></keyframe>
<keyframe time="1"><translate x="2" y="0" z="0"/></keyframe>
</keyframes></track></tracks></animation></animations></skeleton>)";}
        auto root=actor("Root"), tip=actor("Tip");root->addChild(tip);
        auto animator=make_ptr<scene::Animator>();animator->setActor(root);
        animator->setAnimationCachePath(path.u8string());
        const auto before=budget.used();budget.set(before);
        bool denied=false;try{animator->play();}catch(const std::bad_alloc&){denied=true;}
        require(denied && animator->isStopped() && animator->getNumAnimationClips()==0 && budget.used()==before,
                "Cold cache admission denial cannot publish a partial rig or consume Play");
        budget.set(budget.original);
        auto properties=make_ptr<Properties>();properties->setButtonPressed(scene::Animator::playButtonStr,true);
        properties->setProperty(scene::Animator::applyOnPropertyChangeStr,false);
        animator->setProperties(properties);
        require(animator->isPlaying() && animator->getNumAnimationClips()==1 && animator->getSkeleton(),
                "The actual Play property action restores runtime clips on a cached prefab");
        auto rig=dynamic_pointer_cast<MeshSkeleton>(animator->getSkeleton());
        require(rig && rig->getBone("Tip")->getParent()==rig->getBone("Root"),"Cached skeleton parent links are restored");
        animator->update();
        require(std::abs(animator->getAnimationTime()-.25f)<1e-5f && std::abs(rig->getBone("Root")->getPosition().X()-.5f)<1e-4f,
                "Playing advances time and applies actual cached bone keys");
        animator->pause();animator->update();require(std::abs(animator->getAnimationTime()-.25f)<1e-5f,"Paused playback stays fixed");
        animator->play();animator->update();require(std::abs(rig->getBone("Root")->getPosition().X()-1)<1e-4f,"Resume advances the same rig");
        auto other=make_ptr<scene::Animator>();other->setActor(root);other->setAnimationCachePath(path.u8string());other->play();
        require(other->getSkeleton()!=animator->getSkeleton(),"Prefab instances own independent runtime rigs");
        other->setAnimationTime(.75f);
        require(std::abs(rig->getBone("Root")->getPosition().X()-1)<1e-4f,"A second instance cannot overwrite the first pose");
        animator->stop();require(animator->isStopped() && animator->getAnimationTime()==0,"Stop resets time");
        other->unload(nullptr);animator->unload(nullptr);
        core::IApplicationManager::instance()->getGameManager()->destroyActor(root);
    }
    inline SmartPtr<scene::GameActor> prefabShell( const SmartPtr<Properties> &properties )
    {
        String name;properties->getPropertyValue("label",name);
        auto result=actor(name.c_str());
        for(const auto &child:properties->getChildrenByName("child")) result->addChild(prefabShell(child));
        return result;
    }
    inline void skinPublication()
    {
        Budget budget;
        auto manager=dynamic_pointer_cast<MeshManager>(core::IApplicationManager::instance()->getMeshManager());
        require(manager!=nullptr,"Skin contract has the engine mesh manager");
        auto source=make_ptr<workphone::Mesh>();auto section=make_ptr<SubMesh>();
        auto buffer=make_ptr<VertexBuffer>();auto declaration=make_ptr<VertexDeclaration>();
        declaration->addElement(0,0,VertexElementSemantic::VES_POSITION,VertexElementType::VET_FLOAT3);
        declaration->addElement(0,12,VertexElementSemantic::VES_NORMAL,VertexElementType::VET_FLOAT3);
        declaration->addElement(0,24,VertexElementSemantic::VES_TEXTURE_COORDINATES,VertexElementType::VET_FLOAT2);
        buffer->setVertexDeclaration(declaration);buffer->setNumVertices(3);
        const float vertices[]={0,0,0,0,0,1,0,0, 1,0,0,0,0,1,1,0, 0,1,0,0,0,1,0,1};
        std::memcpy(buffer->createVertexData(),vertices,sizeof(vertices));
        auto indices=make_ptr<IndexBuffer>();indices->setNumIndices(3);
        auto indexData=static_cast<u32 *>(indices->createIndexData());indexData[0]=0;indexData[1]=1;indexData[2]=2;
        section->setVertexBuffer(buffer);section->setIndexBuffer(indices);
        for(u32 i=0;i<3;++i){auto assignment=make_ptr<VertexBoneAssignment>();assignment->setVertexIndex(i);assignment->setBoneIndex(0);assignment->setWeight(1);section->addBoneAssignment(assignment);}
        source->addSubMesh(section);
        auto resource=make_ptr<MeshResource>();resource->setFilePath("__procedural/animator-skin-contract");resource->setMesh(source);
        resource->setLoadingState(LoadingState::Loaded);manager->addMeshResource(resource);
        render::ClawMesh mesh;mesh.setMeshName(resource->getFilePath());mesh.load(nullptr);
        require(mesh.isLoaded(),"Skin fixture native mesh loads");
        auto rig=make_ptr<MeshSkeleton>();auto bone=rig->createBone("Root",0);
        bone->setPosition({2,0,0});bone->setBindingPose();bone->setPosition({3,0,0});
        String error;const auto applied=mesh.applySkeletonPose(rig,error);require(applied,error.c_str());
        auto first=[&]{wp_vec3f value;std::memcpy(&value,wp_graphics_mesh_get_vertices(mesh.getNativeMesh()),sizeof(value));return value;};
        require(std::abs(first().x-1)<1e-5f,"Current joint multiplied by inverse binding pose deforms actual native vertices");
        error="Skin pose admission or validation failed; retaining previous geometry.";error.clear();
        const auto resident=budget.used();budget.set(resident);bone->setPosition({4,0,0});
        require(!mesh.applySkeletonPose(rig,error) && std::abs(first().x-1)<1e-5f,
                "At-cap native replacement preserves the last physical skin pose");
        budget.set(budget.original);const auto retry=mesh.applySkeletonPose(rig,error);require(retry,error.c_str());
        require(std::abs(first().x-2)<1e-5f,"Skin retry resamples immutable bind vertices without cumulative deformation");
        budget.set(budget.used());mesh.unload(nullptr);
        require(!mesh.hasSkinningData(),"At-cap render unload retires pose bindings and scratch");
        budget.set(budget.original);manager->destroyResource(resource);
    }
    // Optional project-asset reproduction; the ordinary suite uses the portable
    // analytical cache above. This never rewrites the user's project or caches.
    inline void projectAsset()
    {
        const auto project=std::getenv("WP_ANIMATOR_PROJECT");
        const auto prefab=std::getenv("WP_ANIMATOR_PREFAB");
        const auto meshPath=std::getenv("WP_ANIMATOR_MESH");
        const auto rigPath=std::getenv("WP_ANIMATOR_RIG");
        if(!project || !prefab || !meshPath || !rigPath) return;
        TimerScope timer;
        auto app=core::IApplicationManager::instance();
        const auto previous=app->getProjectPath();
        struct Restore{SmartPtr<core::IApplicationManager> app;String path;~Restore(){app->setProjectPath(path);}} restore{app,previous};
        app->setProjectPath(project);
        std::ifstream input(prefab,std::ios::binary|std::ios::ate);
        require(bool(input),"Project prefab is readable");
        auto bytes=makeSourceBudgetSnapshot<char>();bytes.resize(size_t(input.tellg())+1,0);
        input.seekg(0);input.read(bytes.data(),bytes.size()-1);
        auto properties=make_ptr<Properties>();DataUtil::parse(String(bytes.data()),properties.get(),DataFormat::XML);
        auto root=prefabShell(properties);
        class ImportedMesh : public scene::Mesh {public:void path(const String &value){m_meshPath=value;}};
        auto imported=make_ptr<ImportedMesh>();imported->path(meshPath);root->addComponentInstance(imported);
        auto animator=make_ptr<scene::Animator>();animator->setActor(root);
        auto action=make_ptr<Properties>();action->setButtonPressed(scene::Animator::playButtonStr,true);
        animator->setProperties(action);
        require(animator->isPlaying() && animator->getNumAnimationClips()>0 && animator->getSkeleton(),
                "Project model Play restores a real runtime clip and rig");
        auto rig=dynamic_pointer_cast<MeshSkeleton>(animator->getSkeleton());
        size_t parents=0;for(const auto &bone:rig->getBones()) if(bone->getParent()) ++parents;
        require(parents>0,"Old project cache recovers real bone hierarchy from its prefab instance");
        auto meshOwner=make_ptr<render::ClawMesh>();auto &mesh=*meshOwner;mesh.setMeshName(meshPath);mesh.load(nullptr);
        require(mesh.isLoaded() && mesh.getNativeMesh(),"Project mesh loads its actual native geometry");
        auto renderer=make_ptr<scene::MeshRenderer>();renderer->setGraphicsObject(meshOwner);root->addComponentInstance(renderer);
        String error;
        animator->setAnimationTime(0);
        require(animator->getPlaybackStatus().empty(),"Animator publishes the sampled pose directly to its actual mesh renderer");
        require(mesh.hasSkinningData(),"Animator controller installs the real mesh skin binding");
        auto native=mesh.getNativeMesh();const auto count=wp_graphics_mesh_get_vertex_count(native);
        const auto stride=wp_graphics_mesh_get_vertex_stride(native);
        auto positions=makeSourceBudgetSnapshot<wp_vec3f>();positions.resize(count);
        auto raw=static_cast<const u8 *>(wp_graphics_mesh_get_vertices(native));
        for(u32 i=0;i<count;++i) std::memcpy(&positions[i],raw+size_t(i)*stride,sizeof(wp_vec3f));
        size_t changed=0;double maximum=0;
        // The clip returns to its initial pose at 2.5 seconds. Sample moving
        // and resting portions, rather than equating equal endpoints with a stall.
        for(float time : {.0416667f,.125f,.5f,1.f,2.5f,4.f,5.f})
        {
            animator->setAnimationTime(time);
            require(animator->getPlaybackStatus().empty(),"Animator controller publishes advancing bone poses");
            raw=static_cast<const u8 *>(wp_graphics_mesh_get_vertices(native));
            size_t frameChanged=0;
            for(u32 i=0;i<count;++i)
            {
                wp_vec3f current;std::memcpy(&current,raw+size_t(i)*stride,sizeof(current));
                const auto distance=std::abs(current.x-positions[i].x)+std::abs(current.y-positions[i].y)+std::abs(current.z-positions[i].z);
                require(std::isfinite(distance),"Project animated positions stay finite");
                if(distance>1e-5f) ++frameChanged;maximum=std::max(maximum,double(distance));
            }
            changed=std::max(changed,frameChanged);
        }
        require(changed>0,"Project model Play changes physical rendered vertex positions");
        animator->stop();require(animator->getPlaybackStatus().empty(),"Stop publishes the reset pose");
        raw=static_cast<const u8 *>(wp_graphics_mesh_get_vertices(native));
        for(u32 i=0;i<count;++i)
        {
            wp_vec3f current;std::memcpy(&current,raw+size_t(i)*stride,sizeof(current));
            require(std::abs(current.x-positions[i].x)+std::abs(current.y-positions[i].y)+std::abs(current.z-positions[i].z)<1e-4f,
                    "Project Stop restores the first sampled physical pose without cumulative deformation");
        }
        std::printf("Project Animator: %u vertices, %zu changed, %zu hierarchy links, maximum displacement %.6f\n",count,changed,parents,maximum);
        renderer->setGraphicsObject(nullptr);mesh.unload(nullptr);animator->unload(nullptr);
        app->getGameManager()->destroyActor(root);
    }
    inline void run(){inventories();cachedPlayback();skinPublication();projectAsset();}
}
