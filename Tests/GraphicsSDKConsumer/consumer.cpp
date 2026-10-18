#include <WorkphoneGraphics/workphone_graphics_system.h>
#include <WorkphoneGraphics/workphone_graphics_transform_job.h>
#include <WorkphoneGraphics/workphone_graphics_cull_job.h>
#include <WorkphoneGraphics/workphone_graphics_rtt_job.h>

int main()
{
    auto *system = wp_graphics_system_create();
    if (!system) return 1;
    wp_graphics_system_update(system);
    wp_graphics_system_render(system);
    wp_graphics_system_frame_stats stats{};
    const bool valid = wp_graphics_system_get_frame_stats(system, &stats) &&
        stats.update_serial == 1 && stats.render_serial == 1;
    wp_graphics_system_destroy(system);
    return valid ? 0 : 2;
}
