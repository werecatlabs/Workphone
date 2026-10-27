#include <EditorPCH.hpp>
#include <editor/ProjectSettingsGraphicsBackend.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Graphics/ProjectIrradiance.hpp>
#include <Workphone/Graphics/ProjectGraphicsProviders.hpp>
#include <cmath>

namespace workphone::editor
{
    namespace
    {
        const char *keys[] = {
            "graphics.resolution", "graphics.customWidth", "graphics.customHeight", "graphics.fullscreen", "graphics.vsync", "graphics.presentMode", "graphics.frameRateLimit",
            "graphics.renderScale", "graphics.dynamicResolution", "graphics.hdr", "graphics.colorSpace",
            "graphics.textureFiltering", "graphics.anisotropy", "graphics.renderPipeline", "graphics.antiAliasing",
            "graphics.shadowQuality", "graphics.shadowDistance", "graphics.shadowResolution", "graphics.lightingMode",
            "graphics.gi", "graphics.reflectionProbes", "graphics.postProcessing", "graphics.bloom",
            "graphics.motionBlur", "graphics.ambientOcclusion", "graphics.tonemapping", "quality.preset",
            "quality.texture", "quality.maxTextureSize", "quality.mesh", "quality.lodBias", "quality.particles",
            "quality.terrain", "quality.streamingBudgetMb", "performance.gpuParticles", "performance.occlusionCulling",
            "performance.staticBatching", "performance.dynamicBatching", "performance.memoryBudgetMb",
            "performance.gpuMemoryBudgetMb", "diagnostics.gpuProfiler", "graphics.renderer.pc", "graphics.renderer.mac",
            "graphics.renderer.ios", "graphics.renderer.android", "graphics.renderer.mode", "graphics.shaderModel",
            "graphics.shaderLanguage", "graphics.validationLayers", "graphics.allowRendererFallback"
        };

        String listKeys( const Properties &settings )
        {
            String list;
            for( auto key : keys ) if( settings.hasProperty( key ) )
            { if( !list.empty() ) list += "; "; list += key; }
            return list;
        }

        String requirements( const Properties &settings )
        {
            String result;
            const auto add = [&]( const String &message ) { if( !result.empty() ) result += " "; result += message; };
            if( settings.getPropertyAsInt( "graphics.renderPipeline" ) == 2 )
                add( "Deferred shading requires a geometry G-buffer renderer; the current scene renderer is forward." );
            return result;
        }

        bool budgetFits( const Properties &settings, const char *key, StreamMemoryClass category, String &error )
        {
            if( !settings.hasProperty( key ) ) return true;
            s32 megabytes = 0;
            if( !settings.getPropertyValue( key, megabytes ) || megabytes <= 0 )
            { error = String("Invalid memory budget: ") + key; return false; }
            const auto stats = WorldStreamingScheduler::shared()->stats();
            if( static_cast<u64>( megabytes ) * (1ull << 20) < stats.used[static_cast<size_t>( category )] )
            { error = String("Budget is below live allocations: ") + key; return false; }
            return true;
        }
    }

    SmartPtr<Properties> validateProjectSettingsGraphicsBackend( SmartPtr<Properties> settings )
    {
        auto result = workphone::make_ptr<Properties>();
        String error;
        if( !settings ) error = "Project graphics settings are missing.";
        if( settings )
        {
            String startupNotes;
            validateProjectGraphicsProviders(settings,error,startupNotes);
            result->setProperty("startupNotes",startupNotes);
            const auto app=core::IApplicationManager::instancePtr();
            const auto graphics=app ? app->getGraphicsSystem() : nullptr;
            const s32 aa=settings->getPropertyAsInt("graphics.antiAliasing");
            const s32 samples=aa==3 ? 2 : aa==4 ? 4 : aa==5 ? 8 : 1;
            if(graphics && aa>2)
            {
                const auto properties=graphics->getProperties();
                s32 mask=0;
                if(properties->getPropertyValue("msaa_supported_mask",mask) && !(mask & samples))
                    error="This graphics device does not support the requested MSAA sample count.";
            }
            if( error.empty() && settings->getPropertyAsInt( "graphics.resolution" ) == 5 &&
                (settings->getPropertyAsInt("graphics.customWidth") <= 0 || settings->getPropertyAsInt("graphics.customHeight") <= 0) )
                error = "Custom resolution requires explicit graphics.customWidth and graphics.customHeight values.";
            if( error.empty() && !budgetFits( *settings, "quality.streamingBudgetMb", StreamMemoryClass::WorkingCpu, error ) ) {}
            if( error.empty() && !budgetFits( *settings, "performance.memoryBudgetMb", StreamMemoryClass::SourceCpu, error ) ) {}
            if( error.empty() && !budgetFits( *settings, "performance.gpuMemoryBudgetMb", StreamMemoryClass::Gpu, error ) ) {}
        }
        result->setProperty( "success", error.empty() );
        result->setProperty( "status", error.empty() ? String("Graphics settings validated.") : error );
        if( settings ) { result->setProperty( "handledKeys", listKeys(*settings) ); result->setProperty("externalRequirements", requirements(*settings)); }
        return result;
    }

    void describeProjectSettingsGraphicsBackend( SmartPtr<Properties> response )
    {
        if( !response ) return;
        for( auto key : keys ) response->setProperty( String("capability.") + key, true );
        response->setProperty("availability.graphics.fullscreen", "Win32 borderless fullscreen preserves and restores the window position and style.");
        response->setProperty("availability.graphics.tonemapping", "Native software and DX11 output resolve consume the selected tone curve.");
        response->setProperty("availability.quality.texture", "Texture mip upload limits affect new and refreshed GPU views.");
        response->setProperty("availability.performance.memoryBudgetMb", "Admission budget for participating CPU asset storage; cannot be reduced below live allocations.");
        response->setProperty("availability.performance.gpuMemoryBudgetMb", "Admission budget for participating GPU resource ownership.");
        response->setProperty("availability.diagnostics.gpuProfiler", "DX11 GPU timestamps are collected separately from CPU timing when supported by the device.");
    }

    SmartPtr<Properties> projectGraphicsBackendProperties( SmartPtr<Properties> settings )
    {
        auto result = workphone::make_ptr<Properties>();
        if( !settings ) return result;
        for( auto key : keys ) if( settings->hasProperty(key) ) result->addProperty(settings->getPropertyObject(key));
        s32 value;
        if( settings->getPropertyValue("quality.preset", value) && value < 4 ) result->setProperty("Quality",value);
        if( settings->getPropertyValue("graphics.renderer.pc",value) && (value == 1 || value == 2 || value == 5) )
            result->setProperty("Render API", value == 5 ? 0 : value);
        if( settings->getPropertyValue("graphics.antiAliasing",value) ) result->setProperty("Temporal anti-aliasing",value == 2);
        if( settings->getPropertyValue("graphics.shadowQuality",value) ) result->setProperty("Cascaded shadows",value > 0);
        if( settings->getPropertyValue("graphics.shadowResolution",value) && value >= 0 && value <= 3 )
        { constexpr s32 sizes[] = {512,1024,2048,4096}; result->setProperty("Shadow map size",sizes[value]); }
        bool post = true; settings->getPropertyValue("graphics.postProcessing",post);
        const char *effects[][2] = {{"graphics.bloom","Bloom"},{"graphics.motionBlur","Motion blur"},{"graphics.ambientOcclusion","Ambient occlusion"}};
        for( const auto &effect : effects ) { bool enabled; if(settings->getPropertyValue(effect[0],enabled)) result->setProperty(effect[1],post && enabled); }
        return result;
    }

    SmartPtr<Properties> applyProjectSettingsGraphicsBackend( SmartPtr<Properties> settings )
    {
        auto result = validateProjectSettingsGraphicsBackend(settings);
        if( !result->getPropertyAsBool("success") ) return result;
        const auto app = core::IApplicationManager::instancePtr();
        String error;
        if(!applyProjectIrradianceSettings(settings,app ? app->getProjectPath() : String(),error))
        { result->setProperty("success",false);result->setProperty("status",error);return result; }
        const struct { const char *key; StreamMemoryClass category; } budgets[] = {
            {"quality.streamingBudgetMb",StreamMemoryClass::WorkingCpu},
            {"performance.memoryBudgetMb",StreamMemoryClass::SourceCpu},
            {"performance.gpuMemoryBudgetMb",StreamMemoryClass::Gpu}
        };
        for( const auto &budget : budgets ) if(settings->hasProperty(budget.key))
            if( !WorldStreamingScheduler::shared()->setBudget(budget.category,
                  static_cast<u64>(settings->getPropertyAsInt(budget.key)) * (1ull << 20),error) )
            { result->setProperty("success",false); result->setProperty("status",error); return result; }
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        if( graphics )
        {
            auto properties = projectGraphicsBackendProperties(settings);
            bool vsync = true; settings->getPropertyValue("graphics.vsync",vsync);
            const s32 present = settings->getPropertyAsInt("graphics.presentMode");
            if(present == 1) vsync = true;
            if(present == 3) vsync = false;
            properties->setProperty("vsync",vsync);
            graphics->setProperties(properties);
            if(auto pipeline = graphics->getGraphicsPipeline()) { pipeline->setProperties(properties); pipeline->setProperties(properties); }
            if(auto window = graphics->getDefaultWindow())
            {
                bool fullscreen;
                if(settings->getPropertyValue("graphics.fullscreen",fullscreen)) window->setFullscreen(fullscreen);
                s32 resolution;
                if(settings->getPropertyValue("graphics.resolution",resolution) && !window->isFullScreen())
                {
                    constexpr s32 sizes[][2] = {{1920,1080},{1280,720},{2560,1440},{3840,2160}};
                    if(resolution >= 0 && resolution < 4) window->resize(sizes[resolution][0],sizes[resolution][1]);
                    else if(resolution == 4) window->maximize();
                    else if(resolution == 5) window->resize(settings->getPropertyAsInt("graphics.customWidth"),settings->getPropertyAsInt("graphics.customHeight"));
                }
            }
        }
        result->setProperty("appliedKeys",listKeys(*settings));
        result->setProperty("status",graphics ? "Graphics settings applied to the active renderer and resource policies." : "Graphics policies stored; the renderer applies them when started.");
        return result;
    }
}
