#ifndef ClawCubemapTexture_h__
#define ClawCubemapTexture_h__

#include <WPGraphics/ClawCubemap.hpp>
#include <Workphone/Graphics/Texture.hpp>

namespace workphone::render
{
    /** A material-bindable, filtered cube texture built from six ordinary textures.
     * Input order and orientation match ISky: front, back, left, right, up, down.
     * Upload and filtering happen lazily on the rendering thread. */
    class WPGraphics_API ClawCubemapTexture : public Texture
    {
    public:
        ClawCubemapTexture();
        void setFaces( const Array<SmartPtr<ITexture>> &faces );
        Array<SmartPtr<ITexture>> getFaces() const;
        Array<SmartPtr<ITexture>> getCubemapFaces() const override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void getTextureFinal( void **texture ) const override;
        void getTextureGPU( void **texture ) const override;
        void _getObject( void **texture ) const override;
        Vector2I getSize() const override;
        Vector2I getActualSize() const override;
        WP_CLASS_REGISTER_DECL;

    private:
        Array<SmartPtr<ITexture>> m_faces;
        mutable ClawCubemap m_cube;
        mutable void *m_renderer = nullptr;
    };
}  // namespace workphone::render
#endif
