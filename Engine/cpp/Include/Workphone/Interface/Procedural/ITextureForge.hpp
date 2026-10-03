#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/TextureForgeTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API ITextureForge : public ISharedObject
    {
    public:
        ~ITextureForge() override;
        /// Rebuild the seeded baker before generating a new surface.
        virtual void setSeed( u32 seed ) = 0;
        virtual SurfaceBakeResult bakeSurface( SurfaceTag tag, const SurfaceBakeParams &params ) = 0;
        virtual void bakeConcrete( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakePlaster( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeBrick( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeWood( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeMetal( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeAsphalt( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeSand( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeFabric( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeFoliage( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeGlass( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakePaint( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeRubber( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeDirt( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void bakeStone( SurfaceBakeResult &out, const SurfaceBakeParams &params ) = 0;
        virtual void heightToNormal( const HeightBuffer &height, TextureBuffer &normalOut,
                                     real_Num strength = 2.0f ) = 0;
        virtual void computeORM( const HeightBuffer &height, TextureBuffer &ormOut, SurfaceTag tag ) = 0;
        virtual void encodeSRGB( TextureBuffer &albedo ) = 0;
        virtual u32 getSeed() const = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
