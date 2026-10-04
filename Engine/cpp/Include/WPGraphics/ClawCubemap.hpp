#ifndef ClawCubemap_h__
#define ClawCubemap_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <array>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;

namespace workphone::render
{
    /** Linear HDR environment with GGX-filtered roughness levels.
     * Faces use the sky's front/back/left/right/up/down order and UV orientation.
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
        ID3D11ShaderResourceView *getView() const
        {
            return m_view;
        }
        float getMaxLod() const
        {
            return m_view ? 7.0f : 0.0f;
        }

    private:
        ID3D11ShaderResourceView *m_view = nullptr;
        std::array<ID3D11ShaderResourceView *, 6> m_faces{};
    };
}  // namespace workphone::render
#endif
