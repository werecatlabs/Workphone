#ifndef CTerrainRayResult_h__
#define CTerrainRayResult_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/ITerrainRayResult.hpp>

namespace workphone
{
    namespace render
    {
        class CTerrainRayResult : public ITerrainRayResult
        {
        public:
            CTerrainRayResult();
            ~CTerrainRayResult() override;

            bool hasIntersected() const override;
            void setIntersected( bool intersected ) override;

            SmartPtr<IGraphicsTerrain> getTerrain() const override;
            void setTerrain( SmartPtr<IGraphicsTerrain> terrain ) override;

            Vector3F getPosition() const override;
            void setPosition( const Vector3F &position ) override;

        protected:
            Vector3F m_position;
            SmartPtr<IGraphicsTerrain> m_terrain;
            bool m_isIntersected;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CTerrainRayResult_h__
