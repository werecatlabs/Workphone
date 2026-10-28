#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#include <WPProcedural/WPTextureForge.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CTextureForge : public ITextureForge, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;

        void setSeed( u32 seed ) override;
        SurfaceBakeResult bakeSurface( SurfaceTag tag, const SurfaceBakeParams &params ) override;
        void bakeConcrete( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakePlaster( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeBrick( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeWood( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeMetal( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeAsphalt( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeSand( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeFabric( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeFoliage( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeGlass( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakePaint( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeRubber( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeDirt( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void bakeStone( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override;
        void heightToNormal( const HeightBuffer &height, TextureBuffer &normalOut,
                             real_Num strength ) override;
        void computeORM( const HeightBuffer &height, TextureBuffer &ormOut, SurfaceTag tag ) override;
        void encodeSRGB( TextureBuffer &albedo ) override;
        u32 getSeed() const override;
        WP_CLASS_REGISTER_DECL;

    private:
        WPTextureForge mImplementation;
    };
}  // namespace workphone::procedural
