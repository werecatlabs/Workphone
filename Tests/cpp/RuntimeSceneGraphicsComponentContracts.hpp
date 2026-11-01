#pragma once
#include "RuntimeSceneMembershipContracts.hpp"
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Scene/Components/Skybox.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Scene/Components/Light.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsLight.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <WPGraphics/ClawSkybox.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <Workphone/State/States/CameraStateData.hpp>
#include <Workphone/State/States/GraphicsObjectData.hpp>
#include <limits>
#include <cmath>
#include <atomic>
#include <chrono>
#include <thread>
#include <stdexcept>

namespace runtime_scene_graphics_component_contracts
{
    using namespace workphone;
    inline void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
    template<class F> bool rejects(F operation){try{operation();}catch(const std::exception &){return true;}return false;}
    class PrimaryBarrier final : public Job
    {
    public:
        std::atomic<bool> complete=false;
        void execute() override{complete=true;}
    };

    // Call after the Editor fixture has removed source/catalog/index storage and
    // replaced its database with the lookup spy. These are existing native
    // component lifecycles using only the pinned graph closure passed in.
    inline void run(SmartPtr<core::IApplicationManager> app,
                    std::shared_ptr<const resource::RuntimeAssetContext> context,
                    const String &materialUuid,const String &textureUuid,const String &scriptUuid={},const String &meshUuid={})
    {
        require(app&&context&&context->resolve,"Graphics component contracts require a pinned Editor context");
        auto manager=app->getGameManager();require(manager!=nullptr,"Graphics component contracts require the existing game manager");
        if(!meshUuid.empty())
        {
            auto authored=make_ptr<scene::MeshRenderer>();auto authoredData=make_ptr<Properties>();
            authoredData->setProperty("meshAssetUuid",meshUuid,Properties::resourceStr);authoredData->setProperty("streamMesh",true);
            authored->setProperties(authoredData);
            auto saved=dynamic_pointer_cast<Properties>(authored->toData());
            require(saved&&saved->getProperty("meshAssetUuid")==meshUuid&&saved->getPropertyAsBool("streamMesh")&&
                saved->getPropertyObject("meshAssetUuid").getTypeName()==Properties::resourceStr,
                "Authoring MeshRenderer serialization must retain captured UUID and streaming metadata");
            auto restoredData=make_ptr<Properties>();String persistenceError;
            require(PropertiesBinarySerializer::deserialize(PropertiesBinarySerializer::serialize(*saved),*restoredData,&persistenceError),
                "Authoring MeshRenderer metadata must survive the actual binary scene serializer");
            auto restored=make_ptr<scene::MeshRenderer>();restored->setProperties(restoredData);
            require(restored->getProperties()->getProperty("meshAssetUuid")==meshUuid&&restored->getProperties()->getPropertyAsBool("streamMesh"),
                "A fresh authoring MeshRenderer must consume persisted UUID and streamMesh values");
            auto partial=make_ptr<Properties>();partial->setProperty("zOrder",17u);restored->setProperties(partial);
            require(restored->getZOrder()==17u&&restored->getProperties()->getProperty("meshAssetUuid")==meshUuid&&restored->getProperties()->getPropertyAsBool("streamMesh"),
                "Unrelated authoring property changes must preserve both captured mesh fields");
            require(!restored->isRuntimeLODStreamed()&&!restored->prepareRuntimeLOD(persistenceError)&&persistenceError.empty()&&!restored->getGraphicsObject(),
                "Persisted authoring streamMesh must not activate runtime residency or allocate native geometry");
            restored->releaseRuntimeLOD();
            partial=make_ptr<Properties>();partial->setProperty("streamMesh",false);restored->setProperties(partial);
            require(restored->getProperties()->getProperty("meshAssetUuid")==meshUuid&&!restored->getProperties()->getPropertyAsBool("streamMesh"),
                "Explicit streamMesh=false must preserve an omitted authoring UUID");
            partial=make_ptr<Properties>();partial->setProperty("meshAssetUuid",String());restored->setProperties(partial);
            require(restored->getProperties()->getProperty("meshAssetUuid").empty()&&!restored->getProperties()->getPropertyAsBool("streamMesh"),
                "An explicit empty authoring mesh UUID must clear independently of omitted streaming metadata");
        }
        const auto drainPrimary=[&]{for(int pass=0;pass<2;++pass){auto barrier=make_ptr<PrimaryBarrier>();auto task=app->getTaskManager()->getTask(TaskId::Application);require(task&&task->isLoaded(),"Graphics membership must pump the real Application task");task->addJob(barrier);
            const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(!barrier->complete&&std::chrono::steady_clock::now()<until){app->getJobQueue()->update();app->getTaskManager()->update();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
            require(barrier->complete,"Graphics membership Primary barrier did not drain");}};
        auto actor=manager->createActor();require(actor!=nullptr,"Cannot create graphics component contract actor");
        struct Cleanup {SmartPtr<scene::IGameManager> manager;SmartPtr<scene::IGameActor> actor;~Cleanup(){if(actor)manager->destroyActor(actor);}} cleanup{manager,actor};
        actor->setName("PinnedGraphicsComponentContract");actor->setState(scene::IGameActor::State::Edit);
        auto material=make_ptr<scene::Material>();auto skybox=make_ptr<scene::Skybox>();
        auto camera=make_ptr<scene::Camera>();auto light=make_ptr<scene::Light>();
        material->configureRuntimeAssets(context);skybox->configureRuntimeAssets(context);camera->configureRuntimeAssets(context);light->configureRuntimeAssets(context);
        actor->addComponentInstance(material);actor->addComponentInstance(skybox);actor->addComponentInstance(camera);actor->addComponentInstance(light);
        auto materialData=make_ptr<Properties>();materialData->setProperty("Material",materialUuid);materialData->setProperty("index",0u);
        material->load(materialData);
        auto nativeMaterial=dynamic_pointer_cast<render::ClawMaterial>(material->getMaterial());
        require(nativeMaterial&&nativeMaterial->getNativeMaterial()&&nativeMaterial->getTexture(0),"Pinned material must create native material and texture objects");
        auto skyData=make_ptr<Properties>();for(const auto *key:{"Front","Back","Left","Right","Up","Down"})skyData->setProperty(key,textureUuid);
        skyData->setProperty("distance",750.f);skyData->setProperty("swapLeftRight",true);skybox->load(skyData);
        auto nativeSky=dynamic_pointer_cast<render::ClawSkybox>(skybox->getSkybox());
        require(nativeSky&&nativeSky->getNativeSkybox()&&nativeSky->getMaterial()==skybox->getMaterial()&&nativeSky->getDistance()==750.f,"Pinned skybox must reach its existing native object");
        require(!nativeSky->isVisible(),"An unpublished captured skybox must remain hidden");
        auto faces=skybox->getMaterial()->getCubicTextures();require(faces.size()==6,"Pinned skybox must retain every captured face");
        for(auto face:faces)require(face&&face->isLoaded()&&face->getSize().x>0,"Skybox faces must contain loaded cooked texture data");
        auto cameraData=make_ptr<Properties>();cameraData->setProperty("fov",61.f);cameraData->setProperty("nearClip",.3f);cameraData->setProperty("farClip",300.f);
        cameraData->setProperty("postProcessing.enabled",true);cameraData->setProperty("postProcessing.gamma",2.f);camera->load(cameraData);
        void *nativeCamera=nullptr;require(camera->getCamera()!=nullptr,"Pinned camera must allocate its existing renderer camera");camera->getCamera()->_getObject(&nativeCamera);
        require(nativeCamera&&std::abs(camera->getCamera()->getFOVy()-61.f)<.001f&&camera->getCamera()->getNearClipDistance()==.3f&&camera->getCamera()->getFarClipDistance()==300.f,
            "Camera projection must reach the native renderer");
        auto clawCamera=dynamic_pointer_cast<render::ClawCamera>(camera->getCamera());auto cameraContext=clawCamera?clawCamera->getStateContext():nullptr;
        require(cameraContext&&cameraContext->getStateDataById<GraphicsObjectData>(clawCamera->getId())&&cameraContext->getStateDataById<CameraStateData>(clawCamera->getId()),
            "Native camera factory must publish both existing graphics visibility and typed camera records for its own ID");
        {
            ScopedLock graphicsLock(app->getGraphicsSystem());
            auto clone=dynamic_pointer_cast<render::ClawCamera>(clawCamera->clone("PinnedCameraClone"));
            require(clone&&clone->getId()!=clawCamera->getId()&&clone->getStateContext()==cameraContext,
                "Scene-owned native camera clone must use its creator factory and independent existing state records");
            struct RetireClone {SmartPtr<render::IGraphicsScene> scene;SmartPtr<render::ClawCamera> camera;
                ~RetireClone(){if(camera){camera->setVisible(false);camera->unload(nullptr);scene->removeGraphicsObject(camera);}}} retireClone{app->getGraphicsSystem()->getGraphicsScene(),clone};
            require(std::abs(clone->getFOVy()-61.f)<.001f&&clone->getNearClipDistance()==.3f&&clone->getFarClipDistance()==300.f&&clone->getPostProcessSettings().gamma==2.f,
                "Native clone preserves direct projection and typed post-processing settings");
            clone->setVisible(true);auto cloneSettings=clone->getPostProcessSettings();cloneSettings.gamma=3.f;clone->setPostProcessSettings(cloneSettings);
            require(clone->isVisible()&&!clawCamera->isVisible()&&clawCamera->getPostProcessSettings().gamma==2.f,
                "Cloned visibility and typed camera settings must not mutate the original camera's records");
            const auto cloneId=clone->getId();clone->unload(nullptr);
            require(!cameraContext->getStateDataById<GraphicsObjectData>(cloneId)&&!cameraContext->getStateDataById<CameraStateData>(cloneId)&&
                cameraContext->getStateDataById<GraphicsObjectData>(clawCamera->getId())&&cameraContext->getStateDataById<CameraStateData>(clawCamera->getId()),
                "Unloading one native clone removes only its own records and preserves the original camera");
        }
        auto lightData=make_ptr<Properties>();lightData->setProperty("lightType",1u);lightData->setProperty("intensity",2.f);lightData->setProperty("range",50.f);
        lightData->setProperty("diffuseColour",ColourF(.2f,.4f,.6f,1.f));light->load(lightData);
        void *nativeLight=nullptr;require(light->getLight()!=nullptr,"Pinned light must allocate its existing renderer light");light->getLight()->_getObject(&nativeLight);
        require(nativeLight&&light->getLight()->getPowerScale()==2.f&&light->getLight()->getAttenuationRange()==50.f&&light->getLight()->getDiffuseColour()==ColourF(.2f,.4f,.6f,1.f),
            "Light colour, intensity and attenuation must reach the native renderer");
        require(!light->getLight()->isVisible(),"An unpublished captured light must remain hidden");
        const auto previousMaterial=material->getMaterial(),previousSky=skybox->getMaterial();
        auto invalid=make_ptr<Properties>();invalid->setProperty("Material",textureUuid);
        require(rejects([&]{material->setProperties(invalid);})&&material->getMaterial()==previousMaterial,"Wrong material type must preserve the installed native material");
        invalid->setProperty("Material",materialUuid);invalid->setProperty("Material Path","must-not-open.mat");
        require(rejects([&]{material->setProperties(invalid);})&&material->getMaterial()==previousMaterial,"Runtime material authoring paths must reject before replacement");
        invalid=make_ptr<Properties>();invalid->setProperty("Front",materialUuid);
        require(rejects([&]{skybox->setProperties(invalid);})&&skybox->getMaterial()==previousSky,"Wrong skybox face type must preserve the complete installed skybox");
        require(rejects([&]{skybox->setTextureByName("must-not-open.png",0);})&&skybox->getMaterial()==previousSky,"Runtime skybox name loading must never fall back to authoring");
        invalid=make_ptr<Properties>();invalid->setProperty("fov",std::numeric_limits<f32>::infinity());
        require(rejects([&]{camera->setProperties(invalid);})&&std::abs(camera->getCamera()->getFOVy()-61.f)<.001f,"Invalid camera settings must preserve the native camera");
        invalid=make_ptr<Properties>();invalid->setProperty("intensity",-1.f);
        require(rejects([&]{light->setProperties(invalid);})&&light->getLight()->getPowerScale()==2.f,"Invalid light settings must preserve the native light");
        material->unload(nullptr);skybox->unload(nullptr);camera->unload(nullptr);light->unload(nullptr);
        // Component removal may enqueue the graphics unload on its owner task.
        // Exercise that existing native lifecycle directly under the same fence.
        {ScopedLock graphicsLock(app->getGraphicsSystem());clawCamera->unload(nullptr);}
        require(!cameraContext->getStateDataById<GraphicsObjectData>(clawCamera->getId())&&!cameraContext->getStateDataById<CameraStateData>(clawCamera->getId()),
            "Native camera unload must remove its own visibility and camera records from the shared scene context");
        require(rejects([&]{material->load(materialData);})&&rejects([&]{skybox->load(skyData);})&&rejects([&]{camera->load(cameraData);})&&rejects([&]{light->load(lightData);}),
            "Plain reload after unload must retain source-free mode and require explicit context reconfiguration");
        // Component::unload detaches its actor association. These instances
        // remain in this actor's component inventory, so restore the existing
        // association before explicitly recreating their native state.
        material->setActor(actor);skybox->setActor(actor);camera->setActor(actor);light->setActor(actor);
        material->configureRuntimeAssets(context);skybox->configureRuntimeAssets(context);camera->configureRuntimeAssets(context);light->configureRuntimeAssets(context);
        material->load(materialData);skybox->load(skyData);camera->load(cameraData);light->load(lightData);
        require(material->getMaterial()&&skybox->getSkybox()&&camera->getCamera()&&light->getLight(),"Explicit pinned context reconfiguration must recreate native consumers");
        SmartPtr<scene::MeshRenderer> mesh;
        if(!meshUuid.empty()){
            mesh=make_ptr<scene::MeshRenderer>();mesh->configureRuntimeAssets(context);actor->addComponentInstance(mesh);
            auto data=make_ptr<Properties>();data->setProperty("meshAssetUuid",meshUuid);data->setProperty("state",static_cast<s32>(scene::IComponent::State::Edit));mesh->load(data);
            require(mesh->getGraphicsObject()&&!mesh->getGraphicsObject()->isVisible(),"An unpublished captured mesh must remain hidden after native allocation");
        }
        for(auto component:{static_pointer_cast<scene::IComponent>(skybox),static_pointer_cast<scene::IComponent>(light),static_pointer_cast<scene::IComponent>(camera)})component->setState(scene::IComponent::State::Edit);
        drainPrimary();
        if(camera->getState()!=scene::IComponent::State::Edit&&camera->getState()!=scene::IComponent::State::Play)
            throw std::runtime_error("Runtime camera membership requires its real Application FSM to enter Edit/Play before activation (componentState="+
                std::to_string(static_cast<s32>(camera->getState()))+", actorState="+std::to_string(static_cast<s32>(actor->getState()))+")");
        manager->getCurrentScene()->addActor(actor);
        runtime_scene_membership_contracts::nested(manager,actor,[&](bool visible){
            drainPrimary();
            require(light->getLight()->isVisible()==visible,"Captured light must follow real ancestor publication, enabled state and removal");
            require(skybox->getSkybox()->isVisible()==visible,"Captured skybox must follow real ancestor publication, enabled state and removal");
            if(mesh)require(mesh->getGraphicsObject()->isVisible()==visible,"Captured mesh must follow real ancestor publication, enabled state and removal");
            // Refreshing ordinary properties and material callbacks must not
            // bypass the same native publication boundary.
            light->setVisible(true);skybox->updateMaterials();
            require(light->getLight()->isVisible()==visible&&skybox->getSkybox()->isVisible()==visible,"Direct light visibility and sky material refresh must respect membership");
            camera->setActive(true);
            if(camera->getCamera()->isVisible()!=visible){auto parent=actor->getParent();
                throw std::runtime_error("Captured camera activation must respect nested publication and removal (expected="+std::to_string(visible)+
                    ", native="+std::to_string(camera->getCamera()->isVisible())+", componentState="+std::to_string(static_cast<s32>(camera->getState()))+
                    ", actorState="+std::to_string(static_cast<s32>(actor->getState()))+", actorLoaded="+std::to_string(actor->isLoaded())+
                    ", rootInScene="+std::to_string(actor->getFlag(scene::IGameActor::ActorFlagInScene))+
                    ", parentInScene="+std::to_string(parent&&parent->getFlag(scene::IGameActor::ActorFlagInScene))+
                    ", actorEnabled="+std::to_string(actor->isEnabledInScene())+")");}
            if(visible){
                light->setEnabled(false);skybox->setEnabled(false);if(mesh)mesh->setEnabled(false);
                require(!light->getLight()->isVisible()&&!skybox->getSkybox()->isVisible()&&(!mesh||!mesh->getGraphicsObject()->isVisible()),"Disabling runtime graphics components must hide their native objects");
                light->setEnabled(true);skybox->setEnabled(true);if(mesh)mesh->setEnabled(true);
                require(light->getLight()->isVisible()&&skybox->getSkybox()->isVisible()&&(!mesh||mesh->getGraphicsObject()->isVisible()),"Enabled runtime graphics components must restore a published ancestor's visibility");
            }
        });
        if(!scriptUuid.empty())
        {
            auto cameras=app->getCameraManager();require(cameras!=nullptr,"Camera rollback requires the existing manager");
            const auto managerEnabled=cameras->isEnabled();cameras->setEnabled(true);
            struct RestoreEnabled {SmartPtr<scene::ICameraManager> manager;bool enabled;~RestoreEnabled(){manager->setEnabled(enabled);}} restoreEnabled{cameras,managerEnabled};
            auto previous=manager->createActor();require(previous!=nullptr,"Camera rollback fixture actor must allocate");
            struct Retire {SmartPtr<scene::IGameManager> manager;SmartPtr<scene::IGameActor> actor;~Retire(){manager->destroyActor(actor);}} retire{manager,previous};
            auto readyCamera=make_ptr<scene::Camera>();readyCamera->configureRuntimeAssets(context);previous->addComponentInstance(readyCamera);readyCamera->load(nullptr);
            auto priorNative=dynamic_pointer_cast<render::ClawCamera>(camera->getCamera());auto readyNative=dynamic_pointer_cast<render::ClawCamera>(readyCamera->getCamera());
            require(priorNative&&readyNative&&priorNative->getId()!=readyNative->getId()&&priorNative->getStateContext()==readyNative->getStateContext(),
                "Independent native cameras must share the existing scene context under distinct stable IDs");
            manager->getCurrentScene()->addActor(previous);drainPrimary();readyCamera->setActive(true);
            readyCamera->setTargetTexture(cameras->getEditorRTT());
            const auto previousTarget=readyCamera->getTargetTexture(),previousNativeTarget=readyCamera->getCamera()->getTargetTexture();
            const auto wasActive=readyCamera->isActive();const auto nativeVisible=readyCamera->getCamera()->isVisible();
            require(wasActive,"Camera rollback baseline must be a published active camera");
            auto staged=make_ptr<Properties>();staged->setName("actors");staged->setProperty("uuid",StringUtil::getUUID());staged->setProperty("name","FailedCameraPublication");
            auto cameraProperties=make_ptr<Properties>();cameraProperties->setName("component");cameraProperties->setProperty("uuid",StringUtil::getUUID());cameraProperties->setProperty("componentType","workphone::scene::Camera");cameraProperties->setProperty("isActive",true);cameraProperties->setProperty("state",static_cast<s32>(scene::IComponent::State::Edit));staged->addChild(cameraProperties);
            auto scriptProperties=make_ptr<Properties>();scriptProperties->setName("component");scriptProperties->setProperty("uuid",StringUtil::getUUID());scriptProperties->setProperty("componentType","workphone::scene::Script");scriptProperties->setProperty("scriptAssetUuid",scriptUuid);scriptProperties->setProperty("className","MissingCameraRollbackClass");staged->addChild(scriptProperties);
            {ScopedLock managerLock(manager);ScopedLock graphicsLock(app->getGraphicsSystem());
                require(rejects([&]{scene::GameActorUtil::loadSceneActors({staged},manager->getCurrentScene(),context);}),"A later missing Script constructor must reject the staged Camera batch");}
            drainPrimary();
            require(readyCamera->isActive()==wasActive&&readyCamera->getCamera()->isVisible()==nativeVisible&&readyCamera->getTargetTexture()==previousTarget&&readyCamera->getCamera()->getTargetTexture()==previousNativeTarget,
                "Failed active Camera append must preserve prior published camera flags, visibility and targets");
        }
    }
}
