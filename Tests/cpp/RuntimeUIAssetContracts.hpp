#pragma once
#include <Workphone/System/UIResource.hpp>
#include <Workphone/System/UIAssetResource.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourcePayloadStream.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/UI/IUILayoutContainer.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/FontResourceCompiler.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <filesystem>
#include <fstream>
#include <future>

namespace runtime_ui_asset_contracts
{
    using namespace workphone;using namespace workphone::resource;namespace fs=std::filesystem;
    inline void require(bool value,const String &error){if(!value)throw std::runtime_error(error.c_str());}
    inline String text(const fs::path &path){return path.generic_u8string().c_str();}
    inline void write(const fs::path &path,const String &value){std::ofstream out(path,std::ios::binary);out.write(value.data(),value.size());require(bool(out),"Cannot write UI dependency fixture");}
    inline void run(SmartPtr<core::IApplicationManager> app,const fs::path &base)
    {
        const auto source=base/"source",compiled=base/"compiled",package=base/"package";fs::create_directories(source);String error;
        const String textureId="9215710c-cc4a-444d-851e-000000000001",styleId="9215710c-cc4a-444d-851e-000000000002",localeId="9215710c-cc4a-444d-851e-000000000003",fontId="9215710c-cc4a-444d-851e-000000000004";
        String image(18,'\0');image[2]=2;image[12]=image[14]=16;image[16]=32;image[17]=0x28;for(size_t i=0;i<256;++i){image.push_back(0);image.push_back(char(255));image.push_back(0);image.push_back(char(255));}
        image[18]=image[19]=image[20]=image[21]=char(255);
        write(source/"atlas.tga",image);write(source/"atlas.texres",R"({"version":1,"source":"data://atlas.tga","mipFilter":"none","atlasColumns":1,"alphaCutoff":0.5})");
        write(source/"face.fontres",R"({"version":1,"atlas":"data://atlas.texres","whitePixel":[0,0],"height":16,"ascent":12,"descent":-4,"fallback":63,"glyphs":[{"codepoint":63,"advance":8,"bounds":[0,0,8,16],"uv":[0,0,0.5,1]},{"codepoint":65,"advance":9,"bounds":[0,0,8,16],"uv":[0,0,0.5,1]}]})");
        write(source/"hud.uistyle",R"({"version":1,"styles":[{"name":"hud","colour":[0.25,0.5,0.75,1],"textSize":23}]})");
        write(source/"hud.locres",R"({"version":1,"fallback":"en","locales":{"en":{"greeting":"Hello captured world","confirm":"Confirm","literal":"@{no-authoring-lookup|literal}","fallbackOnly":"Fallback"},"fr":{"greeting":"Bonjour capturé","confirm":"Confirmer"},"dup":{"greeting":"Same","confirm":"Same"}}})");
        auto document=[&](bool game,const char *key){return String("{\"version\":2,\"styleAssetUuid\":\"")+styleId+"\",\"localizationAssetUuid\":\""+localeId+"\",\"locale\":\"en\",\"nodes\":["
            "{\"type\":\""+(game?String("container"):String("window"))+"\",\"id\":401,\"parent\":-1,\"label\":\"Captured dependency UI\",\"position\":[0,0],\"size\":["+(game?String("0.8,0.8"):String("320,200"))+"],\"visible\":true,\"enabled\":true},"
            "{\"type\":\"text\",\"id\":402,\"parent\":0,\"label\":\"Source greeting\",\"localeKey\":\""+key+"\",\"styleClass\":\"hud\",\"fontAssetUuid\":\""+fontId+"\",\"position\":[0,0],\"size\":["+(game?String("0.8,0.1"):String("280,30"))+"],\"visible\":true,\"enabled\":true},"
            "{\"type\":\"image\",\"id\":403,\"parent\":0,\"label\":\"Captured image\",\"textureAssetUuid\":\""+textureId+"\",\"position\":[0,0],\"size\":["+(game?String("0.2,0.2"):String("64,64"))+"],\"visible\":true,\"enabled\":true},"
            "{\"type\":\"button\",\"id\":404,\"parent\":0,\"label\":\"Source confirm\",\"localeKey\":\"confirm\",\"fontAssetUuid\":\""+fontId+"\",\"position\":[0,0],\"size\":["+(game?String("0.2,0.1"):String("100,30"))+"],\"visible\":true,\"enabled\":true}]}";};
        write(source/"desktop.uires",document(false,"greeting"));write(source/"game.uires",document(true,"greeting"));write(source/"missing.uires",document(false,"absent-key"));
        auto duplicate=document(false,"greeting");const auto textType=duplicate.find("\"type\":\"text\"");require(textType!=String::npos,"UI duplicate-label fixture text node");duplicate.replace(textType,13,"\"type\":\"button\"");write(source/"duplicate.uires",duplicate);
        ResourceSystemConfig config;config.target="pc";config.sourceRoot=text(source);config.compiledRoot=text(compiled);config.assetBindings={{textureId,ResourceID("data://atlas.texres")},{styleId,ResourceID("data://hud.uistyle")},{localeId,ResourceID("data://hud.locres")},{fontId,ResourceID("data://face.fontres")}};
        auto registry=std::make_shared<ResourceCompilerRegistry>();for(const auto &compiler:Array<std::shared_ptr<IResourceCompiler>>{std::make_shared<render::GraphicsResourceCompiler>(),std::make_shared<ui::UIResourceCompiler>(),std::make_shared<ui::UIAssetResourceCompiler>(),std::make_shared<render::FontResourceCompiler>()})require(registry->registerCompiler(compiler,&error),error);
        {ui::UIResourceCompiler compiler;CompileContext probe;probe.sourcePath=text(source/"desktop.uires");probe.resourceId=ResourceID("data://desktop.uires");DependencySet dependencies;
            require(!compiler.getDependencies(probe,dependencies,error),"UI dependencies must reject missing captured UUID maps");probe.assetBindings=std::make_shared<const std::map<String,ResourceID>>(std::map<String,ResourceID>{{textureId,ResourceID("data://wrong.locres")}});require(!compiler.getDependencies(probe,dependencies,error),"UI dependencies must reject wrong/missing captured types");}
        {ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());require(author.initialize(config,error),error);CompilationOptions options;options.packagedBuild=true;
            for(const auto id:{ResourceID("data://desktop.uires"),ResourceID("data://game.uires"),ResourceID("data://missing.uires"),ResourceID("data://duplicate.uires")}){const auto report=author.compile(id,options);String diagnostic;for(const auto &message:report.messages)diagnostic+=message+"; ";require(report.succeeded(),diagnostic);}
            require(author.writeRuntimePackage(text(package),{ResourceID("data://desktop.uires"),ResourceID("data://game.uires"),ResourceID("data://missing.uires"),ResourceID("data://duplicate.uires")},{},error),error);author.shutdown();}
        fs::remove_all(source);fs::remove_all(compiled);require(!fs::exists(source)&&!fs::exists(compiled),"UI native dependencies must execute without source/build storage");
        RuntimeResourceConfig runtime;runtime.target="pc";runtime.compiledRoot=text(package);runtime.manifestPath=text(package/"runtime.wprm");ResourceSystem reader(nullptr,nullptr);require(reader.initializeRuntime(runtime,error),error);
        auto mount=ResourcePayloadMount::create(runtime,WorldStreamingScheduler::shared(),{},error);require(bool(mount),error);
        std::map<fs::path,std::pair<u64,u64>> hashes;for(const auto &file:fs::recursive_directory_iterator(package))if(file.is_regular_file()){u64 hash=0,size=0;require(CompiledResourceIO::hashFile(text(file.path()),hash,size,error),error);hashes.emplace(file.path(),std::make_pair(hash,size));fs::permissions(file.path(),fs::perms::owner_write|fs::perms::group_write|fs::perms::others_write,fs::perm_options::remove);}
        struct RestoreFiles {std::map<fs::path,std::pair<u64,u64>> &files;~RestoreFiles(){for(const auto &item:files){std::error_code ignored;fs::permissions(item.first,fs::perms::owner_write,fs::perm_options::add,ignored);}}} restoreFiles{hashes};
        {
            auto scheduler=std::make_shared<WorldStreamingScheduler>();auto captured=ResourcePayloadMount::create(runtime,scheduler,{},error);require(bool(captured),error);
            auto raw=reader.load(ResourceID("data://desktop.uires"),error);require(bool(raw),error);ui::UIResourceInstallOptions options;options.mount=captured;
            auto manager=app->getUI();const auto baseline=manager->getChildObjects().size();
            for(auto category:{StreamMemoryClass::WorkingCpu,StreamMemoryClass::SourceCpu}){
                const auto prior=scheduler->stats();require(scheduler->setBudget(category,prior.used[size_t(category)],error),error);
                auto denied=ui::stageUIResource(manager,raw,error,options);const bool diagnosed=!error.empty();
                require(scheduler->setBudget(category,prior.limits[size_t(category)],error),error);
                require(!denied&&diagnosed&&manager->getChildObjects().size()==baseline,"UI descriptor and control admission must obey the captured mount without leaking registrations");
            }
        }
        for(bool game:{false,true}){
            auto manager=game?app->getRenderUI():app->getUI();require(manager&&manager->isLoaded(),"UI asset fixture requires both existing native UI managers");const auto baseline=manager->getChildObjects().size();
            auto host=game?static_pointer_cast<ui::IUIElement>(manager->addElementByType<ui::IUILayoutContainer>()):static_pointer_cast<ui::IUIElement>(manager->addElementByType<ui::IUIWindow>());require(bool(host),"Cannot create supported native UI dependency host");host->setLabel(game?"Game dependency host":"Editor dependency host");
            struct Host {SmartPtr<ui::IUIManager> manager;SmartPtr<ui::IUIElement> host;~Host(){manager->removeElement(host);}} cleanup{manager,host};
            auto raw=reader.load(ResourceID(game?"data://game.uires":"data://desktop.uires"),error);require(bool(raw),error);ui::UIResourceInstallOptions options;options.mount=mount;
            auto instance=ui::stageUIResource(manager,raw,error,options);require(bool(instance),error);require(instance->publish(host,error),error);auto root=instance->root();require(root->getChildren().size()==3,"Captured UI must publish all native children");
            auto textControl=dynamic_pointer_cast<ui::IUIText>(root->getChildren()[0]);auto imageControl=dynamic_pointer_cast<ui::IUIImage>(root->getChildren()[1]);auto button=dynamic_pointer_cast<ui::IUIButton>(root->getChildren()[2]);
            require(textControl&&imageControl&&button&&textControl->getText()=="Hello captured world"&&textControl->getTextSize()==23,"Style/translation must reach native text");require(textControl->getColour()==ColourF(.25f,.5f,.75f,1),"Captured style colour must reach native controls");
            require(textControl->getRuntimeFont()&&textControl->getRuntimeFont()==button->getRuntimeFont(),"Native controls must share the captured font owner");void *native=nullptr;imageControl->getTexture()->getTextureFinal(&native);require(native!=nullptr,"Captured image must expose a real native texture view");
            const auto installed=manager->getChildObjects().size();auto bad=std::make_shared<RuntimeResource>(*raw);bad->dependencies.clear();require(!ui::stageUIResource(manager,bad,error,options)&&manager->getChildObjects().size()==installed&&host->getChildren()[0]==root,"Missing dependency must preserve native registrations and old tree");
            auto missing=reader.load(ResourceID("data://missing.uires"),error);if(!game)require(!ui::stageUIResource(manager,missing,error,options)&&error.find("Missing translation")!=String::npos,"Missing captured fallback must reject before publication");
            if(!game){auto duplicate=reader.load(ResourceID("data://duplicate.uires"),error);auto translated=options;translated.locale="dup";require(!ui::stageUIResource(manager,duplicate,error,translated)&&error.find("sibling labels")!=String::npos&&manager->getChildObjects().size()==installed,"Translated duplicate buttons must reject before native publication and preserve the ready tree");}
            require(!instance->setLocale("../../fr",error)&&instance->root()==root,"Invalid language cannot replace the ready UI");
            auto scheduler=WorldStreamingScheduler::shared();const auto limit=scheduler->stats().limits[size_t(StreamMemoryClass::Gpu)];require(scheduler->setBudget(StreamMemoryClass::Gpu,scheduler->stats().used[size_t(StreamMemoryClass::Gpu)],error),error);
            const bool refused=!instance->setLocale("fr",error);require(scheduler->setBudget(StreamMemoryClass::Gpu,limit,error),error);require(refused&&instance->root()==root,"GPU denial must preserve the complete native UI generation");
            textControl=nullptr;imageControl=nullptr;button=nullptr;root=nullptr;require(instance->setActive(false,false,error),error);require(instance->setLocale("fr",error),error);root=instance->root();require(!root->isVisible(),"Language replacement must preserve explicit visibility");require(instance->setActive(true,true,error),error);
            textControl=dynamic_pointer_cast<ui::IUIText>(root->getChildren()[0]);require(textControl&&textControl->getText()=="Bonjour capturé"&&manager->getChildObjects().size()==installed,"Language switch must replace captured text without registration leaks");
            require(std::async(std::launch::async,[instance]{String error;return !instance->setLocale("en",error);}).get(),"Language replacement must reject foreign physical owners");
            textControl=nullptr;root=nullptr;instance.reset();require(host->getChildren().empty()&&manager->getChildObjects().size()==baseline+1,"UI asset retirement must remove every native control and its assets");
        }
        for(const auto &file:hashes){u64 hash=0,size=0;require(CompiledResourceIO::hashFile(text(file.first),hash,size,error)&&hash==file.second.first&&size==file.second.second,"UI consumers must preserve every read-only package byte");}
    }
}
