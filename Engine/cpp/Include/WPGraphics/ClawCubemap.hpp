#ifndef ClawCubemap_h__
#define ClawCubemap_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <array>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;
struct wp_dx11_cubemap_owner;

namespace workphone::render
{
    /** Linear HDR environment with GGX-filtered roughness levels.
     * Faces use the sky's front/back/left/right/up/down order and UV orientation.
     * Accepts encoded RGBA/BGRA8 UNORM/sRGB and linear RGBA16F/32F 2D views,
     * up to 2048 pixels per dimension at the selected mip. Nonfinite/negative
     * radiance and unsupported views fail without replacing the previous cube.
     * An empty face set builds a neutral studio environment for material previews. */
    class WPGraphics_API ClawCubemap
    {
    public:
        ClawCubemap() = default;
        ~ClawCubemap();
        ClawCubemap( const ClawCubemap & ) = delete;
        ClawCubemap &operator=( const ClawCubemap & ) = delete;
        bool update( ID3D11Device *device, ID3D11DeviceContext *context,
                     const std::array<ID3D11ShaderResourceView *, 6> &faces );
        void reset();

        ID3D11ShaderResourceView *getView() const;
        ID3D11ShaderResourceView *getViewForDevice( ID3D11Device *device ) const;

        float getMaxLod() const
        {
            return m_owner ? 7.0f : 0.0f;
        }

    private:
        wp_dx11_cubemap_owner *m_owner = nullptr;
    };
}  // namespace workphone::render
#endif
