#pragma once
#include "RuntimeSceneMembershipContracts.hpp"
#include <Workphone/Scene/Components/AudioEmitter.hpp>
#include <Workphone/Scene/Components/UI/UIComponent.hpp>
#include <Workphone/Sound/SoundManager.hpp>
#include <Workphone/Sound/Sound.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/UIResource.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUILayoutContainer.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <WPAudio/WPAudioManager.hpp>
#include <WPAudio/AudioResourceCompiler.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <future>
#include <stdexcept>

namespace runtime_media_component_contracts
{
    using namespace workphone;
    using namespace workphone::resource;
    inline void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
    inline void check(bool value,const String &message){if(!value)throw std::runtime_error(message.empty()?"Media component operation failed without a diagnostic":message.c_str());}
    inline void seal(RuntimeResource &value)
    {value.header.compilerVersion=value.header.resourceType==ResourceTypeID("wav")?audio::AudioResourceCompiler().versionFor(value.header.resourceType):ui::UIResourceCompiler().versionFor(value.header.resourceType);
        value.header.sourceHash=1;value.header.payloadSize=value.payload.size();value.header.payloadHash=hashBytes(value.payload.data(),value.payload.size());}
    inline std::shared_ptr<RuntimeResource> wave()
    {
        auto value=std::make_shared<RuntimeResource>();value->header.resourceId=ResourceID("data://tone.wav");value->header.resourceType=ResourceTypeID("wav");
        const unsigned char bytes[]={
            'R','I','F','F',40,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,
            1,0,1,0,0x40,0x1f,0,0,0x80,0x3e,0,0,2,0,16,0,
            'd','a','t','a',4,0,0,0,0,0,0xff,0x7f};
        value->payload.resize(sizeof(bytes));std::memcpy(value->payload.data(),bytes,sizeof(bytes));seal(*value);return value;
    }
    class PinnedSound final : public Sound
    {
    public:
        StreamBudget admission;
        std::shared_ptr<const RuntimeResource> input;
        void unload(SmartPtr<ISharedObject>) override{stop();input.reset();admission.reset();setLoadingState(LoadingState::Unloaded);}
    };
    class OfflineManager final : public SoundManager
    {
    public:
        bool unavailable=false;
        size_t installed=0;
        SmartPtr<ISound> createRuntimeSound(std::shared_ptr<const RuntimeResource> resource,String &error,std::shared_ptr<ResourcePayloadMount> ={}) override
        {
            if(unavailable){error="Explicit offline fixture device unavailable";return nullptr;}
            if(!audio::AudioResourceCompiler::validate(*resource,error))return nullptr;
            auto lease=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,4096,error);if(!lease)return nullptr;
            auto sound=make_ptr<PinnedSound>();sound->admission=std::move(lease);sound->input=std::move(resource);
            sound->setOwner(this);sound->setLoadingState(LoadingState::Loaded);m_sounds.push_back(sound);++installed;return sound;
        }
        size_t voices() const{return m_sounds.size();}
    };
    inline std::shared_ptr<RuntimeResource> uiDocument(const std::filesystem::path &folder,const char *text)
    {
        const auto file=folder/"component.uires";
        {std::ofstream stream(file,std::ios::binary);stream<<text;check(bool(stream),"Cannot write component UI fixture");}
        ui::UIResourceCompiler compiler;CompileContext context;context.sourcePath=file.u8string().c_str();context.resourceId=ResourceID("data://component.uires");
        std::ostringstream encoded;Array<String> messages;
        check(compiler.compile(context,encoded,messages)==CompilationStatus::Success,"Cannot compile component UI fixture");
        const auto bytes=encoded.str();auto result=std::make_shared<RuntimeResource>();result->header.resourceId=context.resourceId;result->header.resourceType=ResourceTypeID("uires");
        result->payload.resize(bytes.size());std::memcpy(result->payload.data(),bytes.data(),bytes.size());seal(*result);return result;
    }
    inline void run(const std::filesystem::path &folder)
    {
        auto app=core::IApplicationManager::instancePtr();check(bool(app),"Media components require an application");
        auto audioInput=wave();String error;check(audio::AudioResourceCompiler::validate(*audioInput,error),error);
        auto corrupt=std::make_shared<RuntimeResource>(*audioInput);corrupt->payload.back()^=1;
        check(!audio::AudioResourceCompiler::validate(*corrupt,error),"Typed audio must reject a modified sealed payload");
        auto selected=std::make_shared<RuntimeResource>(*audioInput);selected->header.resourceId=ResourceID("data://tone.wav:take.wav");
        check(!audio::AudioResourceCompiler::validate(*selected,error),"Typed audio must reject unsupported selectors");
        auto actual=make_ptr<WPAudioManager>();check(!actual->createRuntimeSound(audioInput,error)&&error.find("device")!=String::npos,"Unloaded native audio must explicitly report its unavailable device");actual=nullptr;
        auto previousManager=app->getSoundManager();auto offline=make_ptr<OfflineManager>();offline->setLoadingState(LoadingState::Loaded);app->setSoundManager(offline);
        struct RestoreSound {SmartPtr<core::IApplicationManager> app;SmartPtr<ISoundManager> previous;~RestoreSound(){app->setSoundManager(previous);}} restore{app,previousManager};
        auto uiInput=uiDocument(folder,R"({"version":1,"nodes":[{"type":"container","id":201,"parent":-1,"label":"Runtime component document","position":[0.02,0.04],"size":[0.4,0.2],"visible":true,"enabled":true},{"type":"text","id":202,"parent":0,"label":"First generation","position":[0,0],"size":[0.2,0.04],"visible":true,"enabled":true}]})");
        const String soundId="37b841aa-1226-4fe4-a242-9a048ca73624",uiId="37b841aa-1226-4fe4-a242-9a048ca73625";
        const String nextSoundId="37b841aa-1226-4fe4-a242-9a048ca73626",nextUIId="37b841aa-1226-4fe4-a242-9a048ca73627";
        {auto authored=make_ptr<scene::UIComponent>();auto authoredProperties=make_ptr<Properties>();authoredProperties->setProperty("uiAssetUuid",uiId);authored->setProperties(authoredProperties);
            authored->setProperties(make_ptr<Properties>());String saved;authored->getProperties()->getPropertyValue("uiAssetUuid",saved);
            check(saved==uiId&&!authored->getElement(),"Authoring UI partial updates must persist the asset picker UUID without claiming a runtime preview");}
        auto context=std::make_shared<RuntimeAssetContext>();
        auto nextAudio=std::make_shared<RuntimeResource>(*audioInput);nextAudio->header.sourceHash=2;
        auto nextUI=std::make_shared<RuntimeResource>(*uiInput);const std::string oldLabel="First generation",newLabel="Other generation";
        auto label=std::search(nextUI->payload.begin(),nextUI->payload.end(),oldLabel.begin(),oldLabel.end());check(label!=nextUI->payload.end(),"UI fixture label must exist");
        std::memcpy(&*label,newLabel.data(),newLabel.size());seal(*nextUI);nextUI->header.sourceHash=2;
        context->resolve=[audioInput,uiInput,nextAudio,nextUI,soundId,uiId,nextSoundId,nextUIId](const String &id,String &error)->std::shared_ptr<const RuntimeResource>{
            error.clear();if(id==soundId)return audioInput;if(id==uiId)return uiInput;if(id==nextSoundId)return nextAudio;if(id==nextUIId)return nextUI;error="UUID outside captured media closure";return {};};
        auto first=make_ptr<scene::AudioEmitter>(),second=make_ptr<scene::AudioEmitter>();first->configureRuntimeAssets(context);second->configureRuntimeAssets(context);
        auto audioProperties=make_ptr<Properties>();audioProperties->setProperty("Sound",soundId);
        first->setProperties(audioProperties);second->setProperties(audioProperties);
        check(first->getSound()!=second->getSound()&&offline->voices()==2,"Runtime emitters require independent registered voices");first->play();check(first->getSound()->isPlaying()&&!second->getSound()->isPlaying(),"Runtime emitters must not share playback state");
        const auto previous=first->getSound();offline->unavailable=true;audioProperties->setProperty("Sound",nextSoundId);
        bool refused=false;try{first->setProperties(audioProperties);}catch(const std::exception&){refused=true;}
        check(refused&&first->getSound()==previous&&previous->isPlaying()&&offline->voices()==2,"Unavailable replacement must preserve the last playing voice");offline->unavailable=false;
        refused=false;try{first->setSound(nullptr);}catch(const std::exception&){refused=true;}check(refused,"Runtime emitter cannot enter its authoring setter");
        first->unload(nullptr);second->unload(nullptr);check(offline->voices()==0,"Runtime emitter retirement must unregister its native voice");
        refused=false;try{first->load(nullptr);}catch(const std::exception&){refused=true;}check(refused,"Runtime audio unload latch must reject plain authoring reload");
        first=nullptr;second=nullptr;
        auto manager=app->getRenderUI();check(manager&&manager->isLoaded(),"Runtime component test requires the real native UI manager");const auto before=manager->getChildObjects().size();
        auto game=app->getGameManager();auto ownerScene=game?game->getCurrentScene():nullptr;
        check(game&&ownerScene&&ownerScene->isLoaded(),"Runtime UI membership requires the existing loaded Editor scene");
        auto actor=game->createActor();check(actor!=nullptr,"Runtime UI membership actor must allocate");
        struct RetireActor {SmartPtr<scene::IGameManager> game;SmartPtr<scene::IGameActor> actor;~RetireActor(){if(actor)game->destroyActor(actor);}} retireActor{game,actor};
        auto component=make_ptr<scene::UIComponent>();component->configureRuntimeAssets(context);actor->addComponentInstance(component);
        auto properties=make_ptr<Properties>();properties->setProperty("uiAssetUuid",uiId);properties->setProperty("state",static_cast<s32>(scene::IComponent::State::Edit));component->load(properties);
        auto host=component->getElement();check(host&&host->getChildren().size()==1,"Runtime UI must publish under its registered native host");
        auto document=host->getChildren()[0];check(document->getChildren().size()==1&&!document->isVisible()&&!document->isEnabled(),"Unpublished runtime UI must remain hidden and input-disabled");
        ownerScene->addActor(actor);check(document->isVisible()&&document->isEnabled(),"Actual scene publication must activate runtime UI");
        runtime_scene_membership_contracts::nested(game,actor,[&](bool visible){check(document->isVisible()==visible&&document->isEnabled()==visible,"Nested runtime UI visibility/input must follow its published scene ancestor");});
        actor->setEnabled(false);check(!document->isVisible()&&!document->isEnabled(),"Disabled runtime actors must hide and disable their UI");
        actor->setEnabled(true);check(document->isVisible()&&document->isEnabled(),"Re-enabled scene actors must restore authored UI flags");
        component->setEnabled(false);check(document->isVisible()&&!document->isEnabled(),"Disabled runtime UI components must disable native input");
        component->setEnabled(true);check(document->isVisible()&&document->isEnabled(),"Re-enabled runtime components must restore native input");
        ownerScene->removeActor(actor);check(!document->isVisible()&&!document->isEnabled(),"Removing a loaded actor must hide and disable its retained UI");
        ownerScene->addActor(actor);check(document->isVisible()&&document->isEnabled(),"Republishing the same loaded actor must reactivate its retained UI");
        auto text=dynamic_pointer_cast<ui::IUIText>(document->getChildren()[0]);check(text&&text->getText()=="First generation","Runtime UI must consume typed control data");text=nullptr;
        const auto installed=manager->getChildObjects().size();properties->setProperty("uiAssetUuid",String("missing"));refused=false;
        try{component->setProperties(properties);}catch(const std::exception&){refused=true;}
        check(refused&&component->getElement()==host&&host->getChildren()[0]==document&&manager->getChildObjects().size()==installed,"Missing UI UUID must preserve native controls and registrations");
        document=nullptr;properties->setProperty("uiAssetUuid",nextUIId);component->setProperties(properties);
        check(component->getElement()==host&&host->getChildren().size()==1&&manager->getChildObjects().size()==installed,"UI replacement must retire the old generation under one host");
        document=host->getChildren()[0];text=dynamic_pointer_cast<ui::IUIText>(document->getChildren()[0]);check(text&&text->getText()==newLabel.c_str(),"Typed UI replacement must publish new content");text=nullptr;
        component->setVisible(false);check(!document->isVisible(),"Runtime UI visibility must reach native document controls");component->setVisible(true);check(document->isVisible(),"Runtime UI visibility must restore authored visible controls");
        const auto foreign=std::async(std::launch::async,[component,properties]() mutable {try{component->setProperties(properties);return false;}catch(const std::exception&){return true;}}).get();
        check(foreign&&host->getChildren()[0]==document,"UI replacement on another physical owner must be rejected before native mutation");
        document=nullptr;host=nullptr;game->destroyActor(actor);retireActor.actor=nullptr;actor=nullptr;
        check(!component->getElement()&&manager->getChildObjects().size()==before,"Actor destruction without an external graphics fence must retire the runtime UI document and host");
        refused=false;try{component->load(nullptr);}catch(const std::exception&){refused=true;}check(refused,"Runtime UI unload latch must reject plain authoring reload");component=nullptr;
        auto editorUI=app->getUI();check(editorUI&&editorUI->isLoaded(),"Window publication requires the Editor's native ImGui manager");
        const auto editorBefore=editorUI->getChildObjects().size();auto windowHost=editorUI->addElement(ui::IUIWindow::typeInfo());
        check(bool(windowHost),"Cannot create native window-publication host");
        windowHost->setLabel("Runtime media window publication host");
        auto oldWindow=std::make_shared<RuntimeResource>(*uiInput),newWindow=std::make_shared<RuntimeResource>(*nextUI);
        oldWindow->payload[12]=1;newWindow->payload[12]=1;seal(*oldWindow);seal(*newWindow);
        auto oldInstance=ui::stageUIResource(editorUI,oldWindow,error);check(bool(oldInstance),error);
        check(oldInstance->publish(windowHost,error),error);
        auto newInstance=ui::stageUIResource(editorUI,newWindow,error);check(bool(newInstance),error);
        check(!newInstance->publish(windowHost,error),"Unrelated duplicate native window labels must still be rejected");
        check(newInstance->publish(windowHost,error,oldInstance.get()),error);oldInstance.reset();
        check(windowHost->getChildren().size()==1,"Replacing windows must retire the prior publication generation");newInstance.reset();
        editorUI->removeElement(windowHost);windowHost=nullptr;check(editorUI->getChildObjects().size()==editorBefore,"Native window replacement must remove all registrations");
    }
}
