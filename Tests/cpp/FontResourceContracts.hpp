#pragma once
#include <WPGraphics/Resources/FontResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/ClawFont.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneRenderer.hpp>
#include "../../Engine/cpp/Source/WPGraphics/UI/ClawUIFrameRetention.hpp"
#if WP_FONT_IMGUI_CONTRACTS
#include <WPImGui/ImGuiElement.hpp>
#include <WPImGui/ImGuiRuntimeFont.hpp>
#include <WPGraphics/ClawImguiManager.hpp>
#include "../../Engine/cpp/Source/WPGraphics/ClawImGuiFrameRecovery.hpp"
#include <WPImGui/ImGuiText.hpp>
#include <WPImGui/ImGuiButton.hpp>
#include <WPImGui/ImGuiImage.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#endif
#include <Workphone/System/ResourcePayloadStream.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <array>
#include <algorithm>
#include <vector>

namespace font_resource_contracts
{
    using namespace workphone;using namespace workphone::render;using namespace workphone::resource;namespace fs=std::filesystem;
    inline void require(bool valid,const char *message){if(!valid)throw std::runtime_error(message);}
    inline void text(const fs::path &path,const std::string &value){std::ofstream out(path,std::ios::binary);out.write(value.data(),value.size());require(bool(out),"Write bounded font fixture");}
    inline void seal(RuntimeResource &value){value.header.payloadSize=value.payload.size();value.header.payloadHash=hashBytes(value.payload.data(),value.payload.size());}
    inline std::string recipe(){return R"({"version":1,"atlas":"data://atlas.texres","whitePixel":[0,0],"height":8,"ascent":8,"descent":0,"fallback":63,"glyphs":[{"codepoint":63,"advance":5,"bounds":[0,0,4,8],"uv":[0,0,0.5,1]},{"codepoint":65,"advance":7,"bounds":[0,0,4,8],"uv":[0.5,0,1,1]}]})";}
    template<class Fixture>void run(Fixture &fixture)
    {
        const auto root=fixture.folder/"font-contracts",source=root/"source",package=root/"package";fs::create_directories(source);String error;
        std::array<unsigned char,18+8*8*4> image{};image[2]=2;image[12]=image[14]=8;image[16]=32;image[17]=0x28;std::fill(image.begin()+18,image.end(),255);
        {std::ofstream out(source/"atlas.tga",std::ios::binary);out.write(reinterpret_cast<const char *>(image.data()),image.size());require(bool(out),"Write baked font atlas pixels");}
        text(source/"atlas.texres",R"({"version":1,"source":"data://atlas.tga","mipFilter":"none","atlasColumns":1,"alphaCutoff":0.5})");text(source/"face.fontres",recipe());
        FontResourceCompiler compiler;CompileContext context;context.resourceId=ResourceID("data://bad.fontres");context.sourcePath=(source/"bad.fontres").u8string().c_str();context.target="font-contract";
        auto invalid=[&](std::string value){text(source/"bad.fontres",value);DependencySet dependencies;require(!compiler.getDependencies(context,dependencies,error)&&!error.empty(),"Invalid baked font semantics reject before atlas dependency cooking");};
        auto duplicate=recipe();duplicate.replace(duplicate.find("\"codepoint\":65"),14,"\"codepoint\":63");invalid(duplicate);
        auto missing=recipe();missing.replace(missing.find("\"fallback\":63"),13,"\"fallback\":66");invalid(missing);
        auto nonUnicode=recipe();nonUnicode.replace(nonUnicode.find("\"codepoint\":65"),14,"\"codepoint\":55296");invalid(nonUnicode);
        invalid(R"({"version":1,"ttf":"untracked.ttf"})");
        auto registry=std::make_shared<ResourceCompilerRegistry>();require(registry->registerCompiler(std::make_shared<GraphicsResourceCompiler>(),&error)&&registry->registerCompiler(std::make_shared<FontResourceCompiler>(),&error),error.c_str());
        ResourceSystem author(registry,std::make_shared<ResourceCompilationDatabase>());ResourceSystemConfig config;config.sourceRoot=source.u8string().c_str();config.compiledRoot=(root/"cooked").u8string().c_str();config.target="font-contract";
        require(author.initialize(config,error),error.c_str());CompilationOptions options;options.packagedBuild=true;const ResourceID id("data://face.fontres"),atlasId("data://atlas.texres");
        require(author.compile(id,options).succeeded(),"Cook bounded font with its typed atlas dependency");
        require(author.writeRuntimePackage(package.u8string().c_str(),{id},{{"413b3281-d3cf-4d3a-899d-2daaa0b384cf",id},{"513b3281-d3cf-4d3a-899d-2daaa0b384cf",atlasId}},error),error.c_str());author.shutdown();
        fs::rename(source,root/"source-unavailable");fs::rename(root/"cooked",root/"cooked-unavailable");
        RuntimeResourceConfig runtime;runtime.compiledRoot=package.u8string().c_str();runtime.manifestPath=(package/"runtime.wprm").u8string().c_str();runtime.target=config.target;
        ResourceSystem reader(nullptr,nullptr);require(reader.initializeRuntime(runtime,error),error.c_str());auto resource=reader.load(id,error);require(resource!=nullptr,error.c_str());CookedFontData data;
        require(resource->header.compilerVersion==compiler.versionFor(ResourceTypeID("fontres")),"Font container must carry the compiler ABI version, separately from its binary schema version");
        const auto decodedFont=decodeCookedFont(*resource,data,error);
        require(decodedFont,error.empty()?"Source-free font decode failed without a diagnostic":error.c_str());
        require(data.glyphs.size()==2,"Source-free font decode must preserve the exact two glyph records");
        require(data.atlasWidth==8&&data.atlasHeight==8,"Source-free font decode must preserve the exact pinned 8x8 atlas dimensions");
        require(data.atlas==atlasId&&data.atlasPayloadHash==resource->dependencies[0]->header.payloadHash,"Source-free font decode must preserve the exact typed atlas identity and generation pin");
        auto bad=std::make_shared<RuntimeResource>(*resource);bad->payload.back()^=1;require(!decodeCookedFont(*bad,data,error),"Font decoder rejects stale payload hashes before parsing");
        const auto metric=60+config.target.size()+atlasId.str().size();
        bad=std::make_shared<RuntimeResource>(*resource);const u32 nan=0x7fc00000;for(size_t i=0;i<4;++i)bad->payload[metric+i]=u8(nan>>(8*i));seal(*bad);require(!decodeCookedFont(*bad,data,error),"Resealed nonfinite font metrics reject");
        bad=std::make_shared<RuntimeResource>(*resource);bad->payload[12]=0xff;seal(*bad);require(!decodeCookedFont(*bad,data,error),"Resealed invalid UTF-8 font target rejects");
        bad=std::make_shared<RuntimeResource>(*resource);bad->payload.pop_back();seal(*bad);require(!decodeCookedFont(*bad,data,error),"Truncated glyph table rejects before allocation");
        bad=std::make_shared<RuntimeResource>(*resource);++bad->header.installArtifacts[0].payloadHash;require(!decodeCookedFont(*bad,data,error),"Font container atlas pin cannot differ from its payload generation");
        {
            auto changed=std::make_shared<RuntimeResource>(*resource->dependencies[0]);CookedTextureData texture;require(decodeCookedTexture(*changed,texture,error),error.c_str());texture.levels[0].bgra[0]=0;std::ostringstream bytes(std::ios::binary);
            require(writeCookedTexture(texture,bytes,error),error.c_str());const auto encoded=bytes.str();changed->payload.clear();for(const auto byte:encoded)changed->payload.push_back(u8(byte));seal(*changed);
            bad=std::make_shared<RuntimeResource>(*resource);bad->dependencies[0]=changed;bad->header.installArtifacts[0].payloadHash=changed->header.payloadHash;
            const auto pin=28+config.target.size()+atlasId.str().size();for(size_t byte=0;byte<8;++byte)bad->payload[pin+byte]=u8(changed->header.payloadHash>>(8*byte));seal(*bad);
            require(!decodeCookedFont(*bad,data,error)&&error.find("utility texel")!=String::npos,"Resealed atlas with matching pins still rejects a nonwhite utility texel");
        }
        auto graphics=fixture.graphics.graphics;auto font=graphics->createRuntimeFont(resource,error,config.target,true);require(font!=nullptr,error.c_str());auto nativeFont=dynamic_pointer_cast<ClawFont>(font);require(nativeFont&&nativeFont->getNativeFont(),"Typed factory creates the existing native ClawFont/wp_font path");
        require(!graphics->createRuntimeFont(resource,error,"wrong-target",true),"Runtime font factory rejects incompatible target");
        auto *native=nativeFont->getNativeFont();require(native->handle.width(native->handle.userdata,8,reinterpret_cast<const wp_c8 *>("A?"),2)==12&&nativeFont->findGlyph(0x1f642).codepoint==63,"Native width and missing-Unicode lookup use baked metrics/fallback");
        wp_user_font_glyph glyph{};native->handle.query(native->handle.userdata,16,&glyph,65,0);require(glyph.width==8&&glyph.height==16&&glyph.xadvance==14,"Native glyph query scales actual baked bounds and advance");
        require(decodeCookedFont(*resource,data,error),error.c_str());auto malformed=data;malformed.glyphs[0].codepoint=64;
        require(!nativeFont->loadRuntimeFont(malformed,font->getRuntimeAtlas(),error)&&nativeFont->getNativeFont()==native,"Invalid native font replacement preserves the installed handle");
        auto service=WorldStreamingScheduler::shared();const auto category=size_t(StreamMemoryClass::SourceCpu);const auto state=service->stats();
        require(state.limits[category]-state.used[category]>8192,"Font pressure fixture has bounded source headroom");
        auto sourcePressure=service->reserve(StreamMemoryClass::SourceCpu,state.limits[category]-state.used[category]-8192,error);require(sourcePressure!=nullptr,error.c_str());
        require(!nativeFont->loadRuntimeFont(data,font->getRuntimeAtlas(),error)&&nativeFont->getNativeFont()==native,"Source pressure preserves native font fallback before glyph copying");sourcePressure.reset();
        const auto gpu=size_t(StreamMemoryClass::Gpu);const auto gpuState=service->stats();auto gpuPressure=service->reserve(StreamMemoryClass::Gpu,gpuState.limits[gpu]-gpuState.used[gpu],error);require(gpuPressure!=nullptr,error.c_str());
        require(!graphics->createRuntimeFont(resource,error,config.target,true)&&nativeFont->getNativeFont()==native,"GPU pressure cannot replace or retire the existing font atlas");gpuPressure.reset();
        {
            auto unicode=data;auto extra=unicode.glyphs.back();extra.codepoint=0x1f642;unicode.glyphs.push_back(extra);auto wide=make_ptr<ClawFont>();
            require(wide->loadRuntimeFont(unicode,font->getRuntimeAtlas(),error)&&wide->findGlyph(0x1f642).codepoint==0x1f642,"Native bitmap fonts support valid non-BMP Unicode scalars");
#if WP_FONT_IMGUI_CONTRACTS && !defined(IMGUI_USE_WCHAR32)
            auto unsupported=make_ptr<ui::ImGuiText>();require(!unsupported->setRuntimeFont(wide),"16-bit ImGui builds reject unsupported non-BMP fonts explicitly");
#endif
        }
        auto renderer=fixture.graphics.renderer;auto target=make_ptr<ClawRenderTarget>();target->setSize({128,64});renderer->setRenderTarget(target);renderer->setViewport(nullptr);renderer->setCamera(nullptr);renderer->clear(ColourF::Black);
        auto *cRenderer=renderer->getNativeRenderer();auto *output=target->getNativeRenderTexture(cRenderer);
        auto pixels=[&]{std::vector<unsigned char> bytes(128*64*4);require(wp_renderer_dx11_copy_render_texture_rgba8(output,bytes.data(),bytes.size()),"Font native render readback");return bytes;};
        std::weak_ptr<const void> retired;
        {
            ui::ClawUIWorkphoneContext context;require(context.isValid(),"Native C UI font context loads");auto *ctx=context.getContext();ui::ClawUIWorkphoneRenderer converter;converter.setNullTexture(context.getNullTexture());
            auto controls=make_ptr<ui::ClawUIManager>();controls->load(nullptr);require(controls->isLoaded(),"Native font fixture loads the existing Claw UI manager");
            struct RetireControls {SmartPtr<ui::ClawUIManager> manager;~RetireControls(){manager->unload(nullptr);}} retireControls{controls};
            auto owners=makeSourceBudgetSnapshot<SmartPtr<ISharedObject>>();auto snapshots=makeSourceBudgetSnapshot<std::shared_ptr<const void>>();ui::frame_retention::Scope scope(owners,snapshots);
            auto text=dynamic_pointer_cast<ui::IUIText>(controls->addElement(ui::IUIText::typeInfo()));
            auto button=dynamic_pointer_cast<ui::IUIButton>(controls->addElement(ui::IUIButton::typeInfo()));
            auto imageControl=dynamic_pointer_cast<ui::IUIImage>(controls->addElement(ui::IUIImage::typeInfo()));
            require(text&&button&&imageControl,"The existing DLL manager must create typed Claw text, button and image controls");
            text->load(nullptr);button->load(nullptr);imageControl->load(nullptr);
            require(text->setRuntimeFont(font)&&button->setRuntimeFont(font)&&text->getRuntimeFont()==font&&button->getRuntimeFont()==font,"Claw text and button adopt captured per-control font owners");
            text->setText("A?");text->setTextSize(16);text->setColour(ColourF::White);text->setPosition({0,0});text->setSize({64,24});button->setLabel("A");button->setTextSize(16);button->setPosition({0,24});button->setSize({64,24});
            auto atlas=font->getRuntimeAtlas();imageControl->setTexture(atlas);imageControl->setPosition({64,0});imageControl->setSize({32,32});retired=font->retainRuntimeFont();
            const auto draw=[&](SmartPtr<ui::IUIElement> control){auto *widget=dynamic_cast<ui::IWorkphoneWidget *>(control.get());require(widget!=nullptr,"Factory-created control exposes its existing native draw interface");widget->draw(ctx);};
            wp_input_begin(ctx);wp_input_end(ctx);require(wp_begin(ctx,reinterpret_cast<const wp_c8 *>("FontContracts"),wp_make_rect(0,0,128,64),WORKPHONE_WINDOW_NO_SCROLLBAR),"C UI root opens");wp_layout_space_begin(ctx,WORKPHONE_STATIC,64,3);draw(text);draw(button);draw(imageControl);wp_layout_space_end(ctx);wp_end(ctx);
            require(owners.size()>=5&&snapshots.size()>=4,"Host scope must bind the real DLL's retained-frame owners without parallel TLS");
            const wp_command *command=nullptr;u32 fontCommands=0;wp_foreach(command,ctx)if(command->type==WORKPHONE_COMMAND_TEXT){const auto *textCommand=reinterpret_cast<const wp_command_text *>(command);fontCommands+=textCommand->font->userdata.ptr==native;}
            require(fontCommands>=2,"Both Claw text and button commands reference the exact captured native glyph table");
            text->setRuntimeFont(nullptr);button->setRuntimeFont(nullptr);imageControl->setTexture(nullptr);font->unload(nullptr);nativeFont=nullptr;font=nullptr;atlas=nullptr;
            require(!retired.expired(),"C frame retains immutable font data through wrapper/control unload");require(converter.convert(ctx)&&converter.getIndexCount()>0,"Baked native font commands convert after wrapper unload");
            require(converter.submit(ctx,cRenderer,context.getFontPixels(),context.getFontWidth(),context.getFontHeight(),context.getFontTexture()),"Baked font atlas submits through existing native UI renderer");
            const auto rendered=pixels();size_t visible=0;for(size_t y=0;y<28;++y)for(size_t x=0;x<64;++x){const auto i=(y*128+x)*4;visible+=rendered[i]>200&&rendered[i+1]>200&&rendered[i+2]>200;}
            require(visible>20,"Captured Claw text glyphs reach native pixels outside image/button regions");wp_clear(ctx);
        }
        require(retired.expired(),"C frame completion retires old immutable glyph/atlas snapshot");
#if WP_FONT_IMGUI_CONTRACTS
        font=graphics->createRuntimeFont(resource,error,config.target,true);require(font!=nullptr,error.c_str());
        {
            auto *previous=ImGui::GetCurrentContext();ImGui::SetCurrentContext(nullptr);struct Restore {ImGuiContext *previous;~Restore(){ImGui::SetCurrentContext(previous);}} restore{previous};
            ClawImguiManager backend;backend.setRenderer(cRenderer);backend.load(nullptr);require(backend.isInitialised(),"ImGui native font fixture initializes renderer context");ImGui::GetIO().IniFilename=nullptr;ImGui::GetIO().LogFilename=nullptr;backend.setupViewport(128,64);renderer->clear(ColourF::Black);backend.newFrame();
            ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImVec2(128,64));ImGui::Begin("Captured font native",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);
            auto text=make_ptr<ui::ImGuiText>();auto button=make_ptr<ui::ImGuiButton>();auto imageControl=make_ptr<ui::ImGuiImage>();text->load(nullptr);button->load(nullptr);imageControl->load(nullptr);
            require(text->setRuntimeFont(font)&&button->setRuntimeFont(font),"ImGui text and button adopt the pinned bitmap atlas");text->setText("A?");text->setTextSize(16);text->update();button->setLabel("A");button->update();
            auto atlas=font->getRuntimeAtlas();void *view=nullptr;atlas->getTextureFinal(&view);require(view!=nullptr,"ImGui glyph atlas has its exact native SRV");imageControl->setTexture(atlas);imageControl->setSize({8,8});imageControl->update();
            bool usedAtlas=false;for(const auto &command:ImGui::GetWindowDrawList()->CmdBuffer)usedAtlas|=command.TextureId==view&&command.ElemCount>0;require(usedAtlas,"ImGui native draw list contains actual captured atlas glyph/image geometry");
            retired=font->retainRuntimeFont();text->setRuntimeFont(nullptr);button->setRuntimeFont(nullptr);imageControl->setTexture(nullptr);font->unload(nullptr);font=nullptr;atlas=nullptr;require(!retired.expired(),"ImGui frame pins actual old font/texture after retarget/unload");
            ImGui::End();backend.render();const auto rendered=pixels();size_t visible=0;for(size_t i=0;i<rendered.size();i+=4)visible+=rendered[i]>200&&rendered[i+1]>200&&rendered[i+2]>200;require(visible>10,"ImGui captured-font geometry reaches native pixels after owner replacement");
            backend.newFrame();require(retired.expired(),"ImGui next-frame hook retires consumed immutable glyph/atlas owners");
            font=graphics->createRuntimeFont(resource,error,config.target,true);require(font!=nullptr,error.c_str());
            require(text->setRuntimeFont(font),"Pressure frame adopts a new captured font generation");
            ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImVec2(128,64));
            ImGui::Begin("Interrupted captured font",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);
            require(ImGui::BeginTabBar("Interrupted owner tabs"),"Pressure frame opens its real outer tab bar");
            require(ImGui::BeginTabItem("Captured font tab",nullptr,ImGuiTabItemFlags_SetSelected),"Pressure frame opens its selected outer tab item");
            ImGui::BeginChild("Interrupted child",ImVec2(96,48));text->update();
            auto *interruptedWindow=ImGui::GetCurrentWindow();
            require(ImGui::GetCurrentContext()->CurrentTabBar&&interruptedWindow->IDStack.Size==1,
                "Interrupted child has its own ID scope below an outer window's open tab bar");
            require(interruptedWindow->DrawList->VtxBuffer.Size>0,"Pressure frame records real captured-font commands before admission failure");
            retired=font->retainRuntimeFont();text->setRuntimeFont(nullptr);font->unload(nullptr);font=nullptr;
            require(!retired.expired(),"Recorded pressure frame alone pins its retired glyph/atlas generation");
            // Leave ordinary window, tab, style and font stacks open, as a thrown
            // application callback does. The production recovery helper runs
            // after that callback unwinds and before any next-frame pin clear.
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha,.75f);ImGui::PushFont(ImGui::GetFont());
            const auto pressureState=service->stats();
            const auto pressureAvailable=pressureState.limits[category]-pressureState.used[category];
            require(pressureAvailable>8192,"Frame pressure fixture leaves bounded lease-bookkeeping headroom");
            auto blocked=service->reserve(StreamMemoryClass::SourceCpu,pressureAvailable-8192,error);
            require(blocked!=nullptr,error.c_str());bool denied=false;
            try{for(size_t i=0;i<4096;++i)ui::retainImGuiFrameResource(text);}catch(const std::bad_alloc &){denied=true;}
            require(denied,"Recorded ImGui frame must encounter actual retained-owner SourceCpu admission denial");
            workphone::render::detail::discardInterruptedImGuiFrame(backend);
            require(!ImGui::GetCurrentContext()->WithinFrameScope&&!ImGui::GetCurrentContext()->CurrentTabBar&&ImGui::GetCurrentContext()->CurrentTabBarStack.Size==0&&
                interruptedWindow->DrawList->CmdBuffer.Size==0&&interruptedWindow->DrawList->VtxBuffer.Size==0&&
                (!ImGui::GetDrawData()||ImGui::GetDrawData()->CmdListsCount==0)&&!retired.expired(),
                "Recovery discards incomplete borrowed commands while their immutable owners remain pinned, even under SourceCpu pressure");
            blocked.reset();backend.newFrame();require(retired.expired(),"Next normal frame retires pins only after discarded commands are gone");
            renderer->clear(ColourF::Black);ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImVec2(128,64));
            ImGui::Begin("Recovered font frame",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);ImGui::TextUnformatted("frame recovered");ImGui::End();backend.render();
            const auto recovered=pixels();size_t recoveredPixels=0;for(size_t i=0;i<recovered.size();i+=4)recoveredPixels+=recovered[i]>200&&recovered[i+1]>200&&recovered[i+2]>200;
            require(recoveredPixels>10,"An ordinary ImGui frame renders native pixels after recording pressure recovery");backend.shutdown();
        }
#endif
        renderer->setRenderTarget(nullptr);target->unload(nullptr);
    }
}
