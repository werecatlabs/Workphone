#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/ProjectGraphicsProviders.hpp>
#include <Workphone/Workphone.hpp>
#include <map>
#include <mutex>

namespace workphone
{
    namespace
    {
        std::mutex providerMutex;
        std::map<String,std::shared_ptr<IProjectRendererProvider>> renderers;
        std::map<std::pair<s32,s32>,std::shared_ptr<IProjectShaderCompilerProvider>> compilers;
    }
    void registerProjectRendererProvider(const String &api,std::shared_ptr<IProjectRendererProvider> provider)
    { std::lock_guard<std::mutex> lock(providerMutex); if(provider) renderers[api]=std::move(provider); else renderers.erase(api); }
    std::shared_ptr<IProjectRendererProvider> getProjectRendererProvider(const String &api)
    { std::lock_guard<std::mutex> lock(providerMutex);const auto i=renderers.find(api);return i==renderers.end()?nullptr:i->second; }
    void registerProjectShaderCompilerProvider(s32 language,s32 model,std::shared_ptr<IProjectShaderCompilerProvider> provider)
    { std::lock_guard<std::mutex> lock(providerMutex);const auto key=std::make_pair(language,model);if(provider) compilers[key]=std::move(provider);else compilers.erase(key); }
    std::shared_ptr<IProjectShaderCompilerProvider> getProjectShaderCompilerProvider(s32 language,s32 model)
    { std::lock_guard<std::mutex> lock(providerMutex);const auto i=compilers.find({language,model});return i==compilers.end()?nullptr:i->second; }
    void releaseProjectGraphicsProviders()
    {
        decltype(renderers) retiredRenderers;decltype(compilers) retiredCompilers;
        {std::lock_guard<std::mutex> lock(providerMutex);retiredRenderers.swap(renderers);retiredCompilers.swap(compilers);}
    }
    String projectRendererApi(SmartPtr<Properties> profile)
    {
        if(!profile) return "DX11";
#if defined(WP_PLATFORM_OSX)
        const auto selected=profile->getPropertyAsInt("graphics.renderer.mac");
        return selected==2?"Vulkan":selected==3?"OpenGL":"Metal";
#elif defined(WP_PLATFORM_IOS)
        return profile->getPropertyAsInt("graphics.renderer.ios")==2?"OpenGLES":"Metal";
#elif defined(WP_PLATFORM_ANDROID)
        return profile->getPropertyAsInt("graphics.renderer.android")==2?"OpenGLES":"Vulkan";
#else
        const char *apis[]={"DX11","DX11","DX12","Vulkan","OpenGL","Software"};
        const auto selected=profile->getPropertyAsInt("graphics.renderer.pc");
        return selected>=0&&selected<6?apis[selected]:"DX11";
#endif
    }
    bool validateProjectGraphicsProviders(SmartPtr<Properties> profile,String &error,String &notes)
    {
        if(!profile) {error="Project graphics profile is missing.";return false;}
        const auto api=projectRendererApi(profile);
        if(auto provider=getProjectRendererProvider(api))
        {if(!provider->validate(profile,error)) return false;}
        else if(api!="DX11"&&api!="DX12"&&api!="Software")
        {
            error="No "+api+" renderer provider is registered. Install/build its renderer plugin or select a built-in API.";return false;
        }
        const auto language=profile->getPropertyAsInt("graphics.shaderLanguage");
        const auto model=profile->getPropertyAsInt("graphics.shaderModel");
        const bool fallback=!profile->hasProperty("graphics.allowRendererFallback")||profile->getPropertyAsBool("graphics.allowRendererFallback");
        if(api=="Software")
        {
            if(language!=0&&!getProjectRendererProvider(api)) {error="An explicit shader language on Software requires a full-profile Software renderer provider; the built-in CPU kernel does not execute compiled shaders.";return false;}
            if(!getProjectRendererProvider(api) &&
                (profile->getPropertyAsInt("graphics.renderPipeline")!=0 || profile->getPropertyAsInt("graphics.lightingMode")!=0 ||
                 profile->getPropertyAsInt("graphics.gi")!=0 || profile->getPropertyAsInt("graphics.shadowQuality")!=0))
            {error="The built-in Software renderer supports unlit Forward rasterization. Choose DX11/DX12 for scene lighting, Forward+/Deferred or shadows, or register a full-profile Software renderer provider.";return false;}
            notes="Software rendering uses CPU material kernels.";return true;
        }
        const auto compiler=getProjectShaderCompilerProvider(language,model);
        if(compiler) return compiler->validate(profile,api,error);
        if(api=="DX11"||api=="DX12")
        {
            if(language>1||model==3||(model==2&&!fallback))
            {error="No shader compiler provider is registered for language "+std::to_string(language)+", model "+std::to_string(model)+" on "+api+". Register a compatible compiler, choose HLSL SM4/SM5, or allow the SM6 compatibility fallback.";return false;}
            if(api=="DX12") notes="Direct3D 12 executes the shared scene pipeline through D3D11On12 using compatible DXBC shaders.";
            if(model==2) notes+=(notes.empty()?"":" ")+String("HLSL SM5 compatibility is active; native SM6 requires a registered compiler provider.");
        }
        else if(language!=0)
        {error="An explicit shader language requires a registered compiler provider for "+api+".";return false;}
        return true;
    }
}
