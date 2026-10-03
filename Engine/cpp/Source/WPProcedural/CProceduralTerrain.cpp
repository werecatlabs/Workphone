#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace procedural
    {
        CProceduralTerrain::CProceduralTerrain() = default;

        CProceduralTerrain::~CProceduralTerrain() = default;

        Vector3F CProceduralTerrain::getSize() const
        {
            return m_size;
        }

        void CProceduralTerrain::setSize( const Vector3F &size )
        {
            m_size = size;
        }

        Vector2I CProceduralTerrain::getHeightmapResolution() const
        {
            return m_heightmapResolution;
        }

        void CProceduralTerrain::setHeightmapResolution( const Vector2I &resolution )
        {
            m_heightmapResolution = resolution;
        }

        Vector2I CProceduralTerrain::getAlphamapResolution() const
        {
            return m_alphamapResolution;
        }

        void CProceduralTerrain::setAlphamapResolution( const Vector2I &size )
        {
            m_alphamapResolution = size;
        }

        Vector2I CProceduralTerrain::getDetailResolution() const
        {
            return m_detailResolution;
        }

        void CProceduralTerrain::setDetailResolution( const Vector2I &size )
        {
            m_detailResolution = size;
        }

        s32 CProceduralTerrain::getDetailResolutionPerPatch() const
        {
            return m_detailResolutionPerPatch;
        }

        void CProceduralTerrain::setDetailResolutionPerPatch( s32 resolutionPerPatch )
        {
            m_detailResolutionPerPatch = resolutionPerPatch;
        }

        Array<f32> CProceduralTerrain::getHeightData() const
        {
            return m_heightData;
        }

        void CProceduralTerrain::setHeightData( const Array<f32> &heightData )
        {
            m_heightData = heightData;
        }
    }  // namespace procedural
}  // namespace workphone
