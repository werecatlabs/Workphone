#ifndef WP_CLAW_DEPTH_READBACK_HPP
#define WP_CLAW_DEPTH_READBACK_HPP
#include <array>
#include <memory>
#include <vector>
struct wp_renderer;
namespace workphone::render
{
    /** Render-owner, bounded 256x256 conventional-Z snapshot. GPU multisampling
     * resolves the farthest sample before conservative pixel-block reduction. */
    class ClawDepthReadback
    {
    public:
        using Surface = std::array<unsigned, 7>;
        ClawDepthReadback();
        ~ClawDepthReadback();
        Surface surface( wp_renderer *renderer ) const;
        bool read( wp_renderer *renderer, std::vector<float> &depth, unsigned &width, unsigned &height );
    private:
        struct Native;
        std::unique_ptr<Native> m_native;
    };
}
#endif
