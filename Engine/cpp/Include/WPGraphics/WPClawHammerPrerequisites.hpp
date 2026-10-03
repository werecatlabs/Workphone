#ifndef __WPGraphicsPrequisites_h__
#define __WPGraphicsPrequisites_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPGraphics_EXPORTS
#            define WPGraphics_API __declspec( dllexport )
#        else
#            define WPGraphics_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPGraphics_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPGraphics_API
#endif

struct wp_graphics_system;
struct wp_graphics_material;
struct wp_renderable;
struct wp_render_pipeline;
struct wp_renderer;

namespace workphone
{
    namespace render
    {
        class CParticleSystem;
        class ParticleData;
        class ClawUIToggleButton;
    }  // namespace render

    namespace ui
    {
        class ClawUIImage;
        class ClawUIToggleButton;
    }  // namespace ui
}  // namespace workphone

#endif  // __WPGraphicsPrequisites_h__
