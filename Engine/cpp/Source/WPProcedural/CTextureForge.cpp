#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CTextureForge.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CTextureForge, ITextureForge );

    void CTextureForge::setSeed( u32 seed )
    {
        mImplementation = WPTextureForge( seed );
    }

    SurfaceBakeResult CTextureForge::bakeSurface( SurfaceTag tag, const SurfaceBakeParams &params )
    {
        return mImplementation.bakeSurface( tag, params );
    }

    void CTextureForge::bakeConcrete( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeConcrete( out, params );
    }

    void CTextureForge::bakePlaster( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakePlaster( out, params );
    }

    void CTextureForge::bakeBrick( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeBrick( out, params );
    }

    void CTextureForge::bakeWood( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeWood( out, params );
    }

    void CTextureForge::bakeMetal( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeMetal( out, params );
    }

    void CTextureForge::bakeAsphalt( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeAsphalt( out, params );
    }

    void CTextureForge::bakeSand( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeSand( out, params );
    }

    void CTextureForge::bakeFabric( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeFabric( out, params );
    }

    void CTextureForge::bakeFoliage( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeFoliage( out, params );
    }

    void CTextureForge::bakeGlass( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeGlass( out, params );
    }

    void CTextureForge::bakePaint( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakePaint( out, params );
    }

    void CTextureForge::bakeRubber( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeRubber( out, params );
    }

    void CTextureForge::bakeDirt( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeDirt( out, params );
    }

    void CTextureForge::bakeStone( SurfaceBakeResult &out, const SurfaceBakeParams &params )
    {
        mImplementation.bakeStone( out, params );
    }

    void CTextureForge::heightToNormal( const HeightBuffer &height, TextureBuffer &normalOut,
                                        real_Num strength )
    {
        WPTextureForge::heightToNormal( height, normalOut, strength );
    }

    void CTextureForge::computeORM( const HeightBuffer &height, TextureBuffer &ormOut, SurfaceTag tag )
    {
        WPTextureForge::computeORM( height, ormOut, tag );
    }

    void CTextureForge::encodeSRGB( TextureBuffer &albedo )
    {
        WPTextureForge::encodeSRGB( albedo );
    }

    u32 CTextureForge::getSeed() const
    {
        return mImplementation.getSeed();
    }
}  // namespace workphone::procedural
