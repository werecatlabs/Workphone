#pragma once
#include "ClawAnimationContracts.hpp"
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <WPGraphics/Resources/SkinnedMeshResource.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include "ClawFoliageContracts.hpp"

namespace claw_skeleton_resource_contracts
{
    inline void consumeSkin(std::shared_ptr<const workphone::resource::RuntimeResource> resource)
    {
        using namespace workphone; using namespace workphone::render; using namespace claw_animation_contracts;
        require(resource!=nullptr,"Skin package consumer requires a loaded root resource");
        String error; CookedSkinnedMeshData skin;
        const auto decoded=SkinnedMeshResourceCompiler::decode(*resource,skin,error); require(decoded,error.c_str());
        std::shared_ptr<const workphone::resource::RuntimeResource> skeleton;
        for(const auto &child:resource->dependencies) if(child && child->header.resourceId==skin.skeleton) skeleton=child;
        require(skeleton!=nullptr,"Skin package includes its pinned skeleton child");
        scene::Animator first,second;
        const auto rigFirst=first.loadResource(skeleton,error); require(rigFirst,error.c_str());
        const auto rigSecond=second.loadResource(skeleton,error); require(rigSecond,error.c_str());
        auto mesh=make_ptr<ClawMesh>(),other=make_ptr<ClawMesh>();
        const auto meshFirst=mesh->loadResource(resource,error,skin.target,skin.packagedBuild); require(meshFirst,error.c_str());
        const auto meshSecond=other->loadResource(resource,error,skin.target,skin.packagedBuild); require(meshSecond,error.c_str());
        require(mesh->hasSkinningData() && other->hasSkinningData() && mesh->getNativeMesh()!=other->getNativeMesh(),"Skin package installs independent native geometry and bind data");
        const auto initial=firstX(*mesh);
        first.setAnimationTime(.25f);
        const auto posed=mesh->applySkeletonPose(first.getSkeleton(),error); require(posed,error.c_str());
        const auto rest=other->applySkeletonPose(second.getSkeleton(),error); require(rest,error.c_str());
        require(std::abs(firstX(*mesh)-initial-.5f)<.0001f && std::abs(firstX(*other)-initial)<.0001f,
            "Packaged mesh weights and skeleton clips deform independent native vertices");
        auto application=core::IApplicationManager::instancePtr();
        auto graphics=application?application->getGraphicsSystemPtr():nullptr;
        auto renderer=graphics?dynamic_pointer_cast<ClawRendererDX11>(graphics->getRenderer()):nullptr;
        require(renderer!=nullptr,"Skin consumer requires an active DX11 renderer");
        auto target=make_ptr<ClawRenderTarget>(); target->setSize({64,64});
        struct Scope { SmartPtr<ClawRendererDX11> renderer; ~Scope(){renderer->setCamera(nullptr);renderer->setRenderTarget(nullptr);} } scope{renderer};
        renderer->setRenderTarget(target); renderer->setCamera(nullptr); renderer->setViewport(nullptr); renderer->disableShadows();
        renderer->setSceneLighting(ColourF::White,{0,-1,0},ColourF::White,0);
        auto native=wp_renderer_get_dx11(renderer->getNativeRenderer()); claw_foliage_contracts::setup(native);
        const auto draw=[&](ClawMesh &value) {
            renderer->clear(ColourF::Black); renderer->renderMesh(&value,Matrix4F::identity());
            return claw_foliage_contracts::pixels(native);
        };
        const auto before=draw(*other),after=draw(*mesh); size_t restPixels=0,posedPixels=0;
        for(size_t index=0;index<before.size();index+=4) {
            restPixels+=unsigned(before[index])+before[index+1]+before[index+2]>30;
            posedPixels+=unsigned(after[index])+after[index+1]+after[index+2]>30;
        }
        require(restPixels>10 && posedPixels>10 && before!=after,"Packaged skin deformation changes real DX11 pixels");
        const auto previous=mesh->getNativeMesh(); const auto position=firstX(*mesh);
        auto missing=std::make_shared<workphone::resource::RuntimeResource>(*resource); missing->dependencies.clear();
        const auto missingInstalled=mesh->loadResource(missing,error,skin.target,skin.packagedBuild);
        require(!missingInstalled && mesh->getNativeMesh()==previous && firstX(*mesh)==position,"Missing skin children retain complete old owner set");
        auto invalid=std::make_shared<workphone::resource::RuntimeResource>(*resource); const u32 nan=0x7fc00000;
        std::memcpy(invalid->payload.data()+invalid->payload.size()-4,&nan,4);
        invalid->header.payloadHash=workphone::resource::hashBytes(invalid->payload.data(),invalid->payload.size());
        const auto invalidInstalled=mesh->loadResource(invalid,error,skin.target,skin.packagedBuild);
        require(!invalidInstalled && mesh->getNativeMesh()==previous && firstX(*mesh)==position,"Invalid skin weights retain complete old owner set");
        auto selector=std::make_shared<workphone::resource::RuntimeResource>(*resource);
        selector->header.resourceId=workphone::resource::ResourceID(resource->header.resourceId.str()+":part.skinres");
        const auto selected=mesh->loadResource(selector,error,skin.target,skin.packagedBuild);
        require(!selected && mesh->getNativeMesh()==previous,"Skin root selectors must not install the entire parent payload");
        mesh->unload(nullptr); other->unload(nullptr);
        std::puts("PASS: source-free typed skin mesh/rig/weights, independent deformation, DX11 pixels and failed replacement retention");
    }

    /** Call from the source-free package consumer with an installed graphics
     * fixture. Uses the existing native skin fixture; it does not claim cooked
     * skin-weight/mesh import support. The packaged rig drives real CPU vertices. */
    inline void consume(std::shared_ptr<const workphone::resource::RuntimeResource> resource)
    {
        using namespace workphone;
        using namespace claw_animation_contracts;
        String error; scene::Animator first,second;
        const auto firstLoaded=first.loadResource(resource,error); require(firstLoaded,error.c_str());
        const auto secondLoaded=second.loadResource(resource,error); require(secondLoaded,error.c_str());
        auto mesh=make_ptr<SkinFixture>(),other=make_ptr<SkinFixture>();
        mesh->load(nullptr); other->load(nullptr);
        first.setAnimationTime(.25f);
        const auto firstApplied=mesh->applySkeletonPose(first.getSkeleton(),error); require(firstApplied,error.c_str());
        const auto secondApplied=other->applySkeletonPose(second.getSkeleton(),error); require(secondApplied,error.c_str());
        require(std::abs(firstX(*mesh)-.3f)<.0001f && std::abs(firstX(*other)+.2f)<.0001f,
            "Packaged skeleton/clip sampling drives independent native CPU skin outputs");
        auto incompatible=make_ptr<MeshSkeleton>(); incompatible->createBone("MissingRoot",1);
        const auto oldX=firstX(*mesh);
        const auto rejected=mesh->applySkeletonPose(incompatible,error);
        require(!rejected && firstX(*mesh)==oldX,"Incompatible supplied skin indices preserve previous geometry");
        const auto restored=mesh->applySkeletonPose(first.getSkeleton(),error);
        require(restored && firstX(*mesh)==oldX,"Incompatible rig rejection preserves the previous palette and rig binding");
        const auto old=first.getSkeleton(); auto corrupt=std::make_shared<workphone::resource::RuntimeResource>(*resource);
        corrupt->header.compilerVersion=0;
        require(!first.loadResource(corrupt,error) && first.getSkeleton()==old,
            "Failed packaged animation replacement retains the previous compatible rig and clips");
        auto rounded=make_ptr<SkinFixture>(); rounded->load(nullptr);
        auto roundedRig=make_ptr<MeshSkeleton>(); auto roundedBone=roundedRig->createBone("Root",0);
        roundedBone->setOrientation(Quaternion<real_Num>(.70712f,.70712f,0,0)); roundedBone->setBindingPose();
        const auto roundedBind=rounded->applySkeletonPose(roundedRig,error); require(roundedBind,error.c_str());
        roundedBone->setOrientation(Quaternion<real_Num>::identity());
        const auto roundedPose=rounded->applySkeletonPose(roundedRig,error); require(roundedPose,error.c_str());
        const auto posedVertices=static_cast<const wp_graphics_mesh_vertex_pnt *>(wp_graphics_mesh_get_vertices(rounded->getNativeMesh()));
        require(std::abs(posedVertices[0].position.y-.5f)<.0001f && std::abs(posedVertices[0].position.z-.2f)<.0001f,
            "Rounded accepted binding quaternions produce rigid native skin deformation");
        const auto roundedY=posedVertices[0].position.y;
        roundedBone->setOrientation(Quaternion<real_Num>(0,0,0,0));
        const auto degenerate=rounded->applySkeletonPose(roundedRig,error);
        require(!degenerate && static_cast<const wp_graphics_mesh_vertex_pnt *>(wp_graphics_mesh_get_vertices(rounded->getNativeMesh()))[0].position.y==roundedY,
            "Degenerate bone quaternions retain the previous native skin geometry");
        rounded->unload(nullptr);
        mesh->unload(nullptr); other->unload(nullptr);
        std::puts("PASS: packaged skeleton clips, independent Animator instances and native CPU skin deformation");
    }
}
