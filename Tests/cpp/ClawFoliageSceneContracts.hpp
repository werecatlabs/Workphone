#pragma once

#include "ClawFoliageContracts.hpp"
#include "ClawMaterialPassLifetimeContracts.hpp"
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawFoliageBatch.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawLight.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialManager.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <Workphone/Graphics/Viewport.hpp>
#include <Workphone/State/States/ViewportStateData.hpp>
#include <WorkphoneGraphics/workphone_graphics_mesh.h>
#include <WorkphoneGraphics/workphone_graphics_camera.h>
#include <WorkphoneGraphics/workphone_graphics_scene.h>

namespace claw_foliage_scene_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using claw_foliage_contracts::require;
    using claw_foliage_contracts::statistics;
    using claw_foliage_contracts::pixels;

    class FixtureGraphics : public ClawHammerSystem
    {
    public:
        void setMaterials(SmartPtr<IMaterialManager> value) { m_materialManager=value; }
    };

    class FixtureMaterialManager : public ClawMaterialManager
    {
    public:
        void remember(SmartPtr<IMaterial> value) { m_materials.push_back(value); }
    };

    struct Fixture
    {
        claw_material_pass_lifetime_contracts::Fixture services;
        SmartPtr<IGraphicsSystem> previousGraphics=services.application->getGraphicsSystem();
        SmartPtr<FixtureGraphics> graphics=make_ptr<FixtureGraphics>();
        SmartPtr<FixtureMaterialManager> materials;
        Fixture()
        {
            services.application->setGraphicsSystem(graphics);
            materials=make_ptr<FixtureMaterialManager>();
            materials->load(nullptr);
            graphics->setMaterials(materials);
        }
        ~Fixture()
        {
            materials->unload(nullptr);
            graphics->setMaterials(nullptr);
            materials=nullptr;
            services.application->setGraphicsSystem(previousGraphics);
        }
    };

    class SectionMesh : public ClawMesh
    {
    public:
        explicit SectionMesh(std::shared_ptr<bool> retired) : m_retired(std::move(retired))
        {
            m_mesh=wp_graphics_mesh_create();
            require(m_mesh != nullptr,"scene foliage mesh must allocate");
            const wp_graphics_mesh_vertex_pnt vertices[]={
                {{-0.15f,-0.15f,0.5f},{0,0,1},{0,0}},
                {{0.15f,-0.15f,0.5f},{0,0,1},{1,0}},
                {{0.15f,0.15f,0.5f},{0,0,1},{1,1}},
                {{-0.15f,0.15f,0.5f},{0,0,1},{0,1}}};
            const wp_u16 indices[]={0,1,2,0,2,3};
            wp_graphics_mesh_set_vertices(m_mesh,WORKPHONE_VERTEX_FORMAT_PNT,vertices,4);
            wp_graphics_mesh_set_indices_u16(m_mesh,indices,6);
            require(wp_graphics_mesh_add_submesh(m_mesh,0,3,0)==0 &&
                    wp_graphics_mesh_add_submesh(m_mesh,3,3,1)==1,
                    "scene foliage fixture must expose two real material sections");
        }
        ~SectionMesh() override { *m_retired=true; }
    private:
        std::shared_ptr<bool> m_retired;
    };

    class IdentityCamera : public ClawCamera
    {
    public:
        Matrix4<real_Num> getViewMatrix() const override { return Matrix4<real_Num>::identity(); }
        Matrix4<real_Num> getProjectionMatrix() const override { return Matrix4<real_Num>::identity(); }
    };

    class FixtureScene : public ClawScene
    {
    public:
        void addLight(SmartPtr<ClawLight> light) { m_lights.push_back(light); }
    };

    class FixtureViewport : public Viewport
    {
    public:
        void _getObject(void **object) const override { if(object) *object=nullptr; }
    };

    inline ClawFoliageInstance instance(float x, ColourF tint)
    {
        ClawFoliageInstance value;
        value.transform[0][3]=x;
        value.tint=tint;
        return value;
    }

    inline void sceneContracts(ClawRendererDX11 &renderer)
    {
        Fixture fixture;
        auto scene=make_ptr<FixtureScene>();
        scene->setAmbientLight(ColourF::White);
        scene->setEnableShadows(false);
        auto target=make_ptr<ClawRenderTarget>(); target->setSize({128,128});
        renderer.setRenderTarget(target); renderer.setViewport(nullptr); renderer.setCamera(nullptr);
        auto *native=wp_renderer_get_dx11(renderer.getNativeRenderer());
        auto retired=std::make_shared<bool>(false);
        auto mesh=make_ptr<SectionMesh>(retired);
        Array<ClawFoliageInstance> authored={instance(-0.5f,ColourF::Red),instance(0.5f,ColourF::Green)};
        String error;
        auto invalidMesh=make_ptr<SectionMesh>(std::make_shared<bool>(false));
        const wp_u16 invalidIndices[]={0,1,9,0,2,3};
        wp_graphics_mesh_set_indices_u16(invalidMesh->getNativeMesh(),invalidIndices,6);
        require(!ClawFoliageBatch::create(invalidMesh,authored,error) && !error.empty(),
                "out-of-range geometry must fail before scene publication");
        const wp_u16 validIndices[]={0,1,2,0,2,3};
        wp_graphics_mesh_set_indices_u16(invalidMesh->getNativeMesh(),validIndices,6);
        wp_graphics_mesh_clear_submeshes(invalidMesh->getNativeMesh());
        wp_graphics_mesh_add_submesh(invalidMesh->getNativeMesh(),4,3,0);
        require(!ClawFoliageBatch::create(invalidMesh,authored,error) && !error.empty(),
                "invalid section range must fail before scene publication");
        invalidMesh=nullptr;
        auto first=ClawFoliageBatch::create(mesh,authored,error,1,true);
        require(first != nullptr && error.empty() && first->instanceCount()==2,
                "valid scene foliage must create an immutable two-instance batch");
        authored[0].transform[0][3]=0; authored[0].tint=ColourF::Blue;
        require(scene->replaceFoliageBatch(nullptr,first,error),"scene must attach its first foliage batch");
        auto oldSnapshot=scene->getFoliageBatches();
        require(oldSnapshot && oldSnapshot->size()==1 && oldSnapshot->front()==first &&
                scene->getNativeScene()->object_count==0,
                "scene batch ownership must not create native actors per plant");
        require(!scene->replaceFoliageBatch(nullptr,first,error) && !error.empty() &&
                scene->getFoliageBatches()==oldSnapshot,"duplicate attachment must retain the published snapshot");

        auto unsupported=make_ptr<ClawMaterial>();
        unsupported->setName("late-foliage-transparent");
        auto technique=make_ptr<ClawMaterialTechnique>();
        technique->setMaterial(unsupported.get());
        technique->createPass();
        unsupported->setTechniques({technique});
        unsupported->setTransparent(true);
        require(unsupported->isTransparent(),"material fixture must have a real transparent pass state");
        auto boundMesh=make_ptr<SectionMesh>(std::make_shared<bool>(false));
        boundMesh->setMaterial(unsupported,1);
        require(!ClawFoliageBatch::create(boundMesh,authored,error) && !error.empty() &&
                scene->getFoliageBatches()==oldSnapshot,
                "bound transparent material candidate must reject before retiring last-good foliage");
        unsupported->setTransparent(false); unsupported->setBlendMode(3);
        require(!ClawFoliageBatch::create(boundMesh,authored,error) && !error.empty(),
                "nontransparent additive material must also reject before publication");
        unsupported->setBlendMode(0); unsupported->setTransparent(true);
        auto namedMesh=make_ptr<SectionMesh>(std::make_shared<bool>(false));
        namedMesh->setMaterialName(unsupported->getName(),1);
        require(!namedMesh->getMaterial(1),"late named material fixture must have no bound section pointer");
        fixture.materials->remember(unsupported);
        require(!ClawFoliageBatch::create(namedMesh,authored,error) && !error.empty() &&
                scene->getFoliageBatches()==oldSnapshot,
                "named transparent material must resolve and reject before scene publication");
        unsupported->setTransparent(false);
        auto materialCandidate=ClawFoliageBatch::create(namedMesh,authored,error);
        require(materialCandidate != nullptr,"opaque named material must remain supported");
        unsupported->setTransparent(true);
        const auto materialBefore=statistics(native);
        require(materialCandidate->render(renderer)==0 && statistics(native).draws==materialBefore.draws,
                "later shared-material transparency edits must retain the draw-time guard");
        materialCandidate.reset(); boundMesh=nullptr; namedMesh=nullptr;

        const auto before=statistics(native);
        renderer.clear(ColourF::Black); scene->render(static_cast<IRenderer *>(&renderer));
        const auto after=statistics(native);
        const auto visible=pixels(native);
        const auto ambient=scene->getAmbientLight();
        std::printf("Scene foliage pixels: left=(%u,%u,%u,%u) right=(%u,%u,%u,%u), ambient=(%.3f,%.3f,%.3f).\n",
            visible[(64*128+32)*4],visible[(64*128+32)*4+1],visible[(64*128+32)*4+2],visible[(64*128+32)*4+3],
            visible[(64*128+96)*4],visible[(64*128+96)*4+1],visible[(64*128+96)*4+2],visible[(64*128+96)*4+3],
            ambient.r,ambient.g,ambient.b);
        require(after.draws-before.draws==2 && after.instanced_draws-before.instanced_draws==2 &&
                after.instances-before.instances==4 && after.triangles-before.triangles==4,
                "ClawScene must submit two shared sections for two instances through DX11 instancing");
        // The real scene PBR path includes untinted dielectric/environment
        // reflections. Require strong intended tint dominance and coverage,
        // rather than the zero secondary channels of an unlit shader.
        require(visible[(64*128+32)*4]>80 &&
                visible[(64*128+32)*4]>std::max(visible[(64*128+32)*4+1],visible[(64*128+32)*4+2])+50 &&
                visible[(64*128+96)*4+1]>80 &&
                visible[(64*128+96)*4+1]>std::max(visible[(64*128+96)*4],visible[(64*128+96)*4+2])+50,
                "scene render must preserve original batch transforms/tints after authoring arrays change");

        wp_graphics_scene_set_visibility_mask(scene->getNativeScene(),2);
        const auto maskBefore=statistics(native);
        renderer.clear(ColourF::Black); scene->render(static_cast<IRenderer *>(&renderer));
        require(statistics(native).draws==maskBefore.draws,"scene mask must exclude foliage without GPU draws");
        wp_graphics_scene_set_visibility_mask(scene->getNativeScene(),~wp_u32(0));
        auto camera=make_ptr<IdentityCamera>();
        wp_camera_set_visibility_mask(camera->getNativeCamera(),2);
        renderer.setCamera(camera);
        scene->render(static_cast<IRenderer *>(&renderer));
        require(statistics(native).draws==maskBefore.draws,"camera mask must exclude foliage independently");
        wp_camera_set_visibility_mask(camera->getNativeCamera(),1);
        scene->render(static_cast<IRenderer *>(&renderer));
        require(statistics(native).draws==maskBefore.draws+2 && scene->getFoliageBatches()==oldSnapshot,
                "a second view must recover foliage without mutating shared visibility or population");
        auto viewport=make_ptr<FixtureViewport>();
        auto viewportContext=fixture.services.states->addStateContext();
        auto viewportState=make_ptr<State>(); viewportState->setId(viewport->getId());
        viewportState->setData(make_ptr<ViewportStateData>()); viewportContext->addState(viewportState);
        viewport->setStateContext(viewportContext); viewport->setVisibilityMask(2);
        require(viewport->getVisibilityMask()==2,"viewport fixture must have a real editable state record");
        renderer.setViewport(viewport);
        const auto viewportBefore=statistics(native);
        scene->render(static_cast<IRenderer *>(&renderer));
        require(statistics(native).draws==viewportBefore.draws,"viewport mask must exclude foliage");
        viewport->setVisibilityMask(1);
        scene->render(static_cast<IRenderer *>(&renderer));
        require(statistics(native).draws==viewportBefore.draws+2,"viewport mask must restore compatible foliage");
        renderer.setViewport(nullptr);
        viewport->setStateContext(nullptr); fixture.services.states->removeStateContext(viewportContext);

        auto sun=make_ptr<ClawLight>(); sun->load(nullptr);
        sun->setType(LightTypes::LT_DIRECTIONAL); sun->setDirection(Vector3<real_Num>(0,-1,-1));
        sun->setPowerScale(1); sun->setCastShadows(true); scene->addLight(sun);
        scene->setEnableShadows(true);
        const auto shadowBefore=statistics(native);
        renderer.clear(ColourF::Black); scene->render(static_cast<IRenderer *>(&renderer));
        require(statistics(native).draws-shadowBefore.draws==4 &&
                statistics(native).instances-shadowBefore.instances==8,
                "scene shadow and colour passes must each submit both instanced sections");
        scene->setEnableShadows(false);

        authored={instance(0,ColourF(1,1,0,1))};
        auto replacement=ClawFoliageBatch::create(mesh,authored,error,1,false);
        require(replacement && scene->replaceFoliageBatch(first,replacement,error),
                "a valid foliage replacement must publish atomically");
        auto retained=scene->getFoliageBatches();
        require(retained && retained!=oldSnapshot && retained->size()==1 &&
                oldSnapshot->front()==first && first->instanceCount()==2,
                "retained readers must keep the prior immutable population");
        require(!scene->replaceFoliageBatch(first,first,error) && !error.empty() &&
                scene->getFoliageBatches()==retained,"stale replacement must preserve the current population");
        authored[0].transform[0][0]=0;
        require(!ClawFoliageBatch::create(mesh,authored,error) && !error.empty() &&
                scene->getFoliageBatches()==retained,"invalid instance candidate must preserve scene content");
        renderer.clear(ColourF::Black); scene->render(static_cast<IRenderer *>(&renderer));
        const auto changed=pixels(native);
        require(changed[(64*128+64)*4]>80 && changed[(64*128+64)*4+1]>80 &&
                changed[(64*128+32)*4]==0 && changed[(64*128+96)*4+1]==0,
                "published replacement must change actual scene pixels without leaving old instances");
        scene->setEnableShadows(true);
        const auto noCastBefore=statistics(native);
        scene->render(static_cast<IRenderer *>(&renderer));
        require(statistics(native).draws-noCastBefore.draws==2,
                "a batch with castShadows disabled must retain only its colour submissions");
        scene->setEnableShadows(false);

        mesh=nullptr; first.reset(); replacement.reset(); oldSnapshot.reset();
        scene->clear();
        require(!scene->getFoliageBatches() && !*retired,
                "scene clear must detach foliage while retained render snapshots keep its shared mesh alive");
        const auto clearBefore=statistics(native);
        renderer.clear(ColourF::Black); scene->render(static_cast<IRenderer *>(&renderer));
        require(statistics(native).draws==clearBefore.draws,"cleared scene must submit no retired foliage");
        retained.reset();
        require(*retired,"releasing the final population snapshot must release the shared mesh and GPU cache");
        sun->unload(nullptr);
        scene=nullptr;
    }

    inline bool run(ClawRendererDX11 &renderer)
    {
        struct Scope { ClawRendererDX11 &renderer; ~Scope() { renderer.disableShadows();
            renderer.setViewport(nullptr); renderer.setCamera(nullptr); renderer.setRenderTarget(nullptr); } } scope{renderer};
        try
        {
            sceneContracts(renderer);
            std::puts("ClawScene foliage publication, real draws/pixels, three visibility masks, shadows and retirement passed.");
            return true;
        }
        catch(const std::exception &error)
        {
            std::fprintf(stderr,"ClawScene foliage contract failed: %s\n",error.what()); return false;
        }
    }
}
