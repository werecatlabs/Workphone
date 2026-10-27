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
    String projectRendererApi(SmartPtr<Properties> profile)
    {
        if(!profile) return "DX11";
#if defined(WP_PLATFORM_OSX)
        const auto selected=profile->getPropertyAsInt("graphics.renderer.mac");
        return selected==2?"Vulkan":selected==3?"OpenGL":"Metal";
#elif defined(WP_PLATFORM_IOS)
        return "Metal";
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
        if(api!="DX11"&&api!="DX12"&&api!="Software")
        {
            const auto provider=getProjectRendererProvider(api);
            if(!provider) {error="No "+api+" renderer provider is registered. Install/build its renderer plugin or select a built-in API.";return false;}
            if(!provider->validate(profile,error)) return false;
        }
        const auto language=profile->getPropertyAsInt("graphics.shaderLanguage");
        const auto model=profile->getPropertyAsInt("graphics.shaderModel");
        const bool fallback=!profile->hasProperty("graphics.allowRendererFallback")||profile->getPropertyAsBool("graphics.allowRendererFallback");
        const auto compiler=getProjectShaderCompilerProvider(language,model);
        if(compiler) return compiler->validate(profile,api,error);
        if(api=="Software")
        {
            if(language!=0) {error="The Software renderer executes CPU material kernels; a shader compiler provider is required for an explicit shader language.";return false;}
            notes="Software rendering uses CPU material kernels.";return true;
        }
        if(api=="DX11"||api=="DX12")
        {
            if(language>1||model==3||(model==2&&!fallback))
            {error="No shader compiler provider is registered for language "+String(language)+", model "+String(model)+" on "+api+". Register a compatible compiler, choose HLSL SM4/SM5, or allow the SM6 compatibility fallback.";return false;}
            if(model==2) notes="The built-in "+api+" compiler uses HLSL SM5 compatibility shaders; native SM6 requires a registered compiler provider.";
        }
        else if(language!=0)
        {error="An explicit shader language requires a registered compiler provider for "+api+".";return false;}
        return true;
    }
}
