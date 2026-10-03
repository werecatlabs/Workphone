#ifndef CProceduralTerrain_h__
#define CProceduralTerrain_h__

#include <Workphone/Interface/Procedural/IProceduralTerrain.hpp>
#include <WPProcedural/CProceduralObject.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API CProceduralTerrain : public CProceduralObject<IProceduralTerrain>
        {
        public:
            CProceduralTerrain();
            ~CProceduralTerrain() override;

            Vector3F getSize() const override;
            void setSize( const Vector3F &size ) override;

            Vector2I getHeightmapResolution() const override;
            void setHeightmapResolution( const Vector2I &resolution ) override;

            Vector2I getAlphamapResolution() const override;
            void setAlphamapResolution( const Vector2I &size ) override;

            Vector2I getDetailResolution() const override;
            void setDetailResolution( const Vector2I &size ) override;

            s32 getDetailResolutionPerPatch() const override;
            void setDetailResolutionPerPatch( s32 resolutionPerPatch ) override;

            Array<f32> getHeightData() const override;
            void setHeightData( const Array<f32> &heightData ) override;

        protected:
            Vector3F m_size = Vector3F::zero();
            Vector2I m_heightmapResolution = Vector2I::zero();
            Vector2I m_alphamapResolution = Vector2I::zero();
            Vector2I m_detailResolution = Vector2I::zero();
            s32 m_detailResolutionPerPatch = 0;
            Array<f32> m_heightData;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // CProceduralTerrain_h__
