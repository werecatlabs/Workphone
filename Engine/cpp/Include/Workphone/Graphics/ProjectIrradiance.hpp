#pragma once
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <WorkphoneGraphics/workphone_graphics_irradiance.h>
#include <memory>

namespace workphone
{
    /** Providers may use lightmaps, ray-tracing services or the built-in physics/scene sampler. */
    class WPCore_API IProjectIrradianceProvider
    {
    public:
        virtual ~IProjectIrradianceProvider() = default;
        virtual bool sample( const float position[3], u32 rays, s32 lightSelection,
                             bool includeEnvironment, wp_irradiance_probe &probe, String &error ) = 0;
    };
    WPCore_API void setProjectIrradianceProvider( std::shared_ptr<IProjectIrradianceProvider> provider );
    WPCore_API bool applyProjectIrradianceSettings( SmartPtr<Properties> settings,
                                                   const String &projectDirectory, String &error );
    /** Realtime/Hybrid refresh one bounded local probe per call. Baked data remains immutable. */
    WPCore_API void updateProjectIrradiance();
    WPCore_API Array<wp_irradiance_probe> getProjectIrradianceProbes();
    WPCore_API s32 getProjectIrradianceMode();
    WPCore_API void releaseProjectIrradiance();
}
