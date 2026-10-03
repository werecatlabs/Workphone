#ifndef TerrainRayResult_h__
#define TerrainRayResult_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Graphics/ITerrainRayResult.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Renderer-agnostic, CPU-side implementation of ITerrainRayResult.
         *
         * Used by the base Terrain::intersects() implementation so that a renderer
         * plugin gets a working ray-hit result without having to provide its own type.
         * Renderer plugins with a native ray result (such as CTerrainRayResult) may
         * return their own type instead.
         *
         * @note This type intentionally does not use WP_CLASS_REGISTER_DECL/DERIVED so it
         * can be instantiated directly with new() without requiring RTTI registration,
         * matching the existing CTerrainRayResult pattern.
         */
        class WPCore_API TerrainRayResult : public ITerrainRayResult
        {
        public:
            /** Default constructor; initialises an empty (non-intersected) result. */
            TerrainRayResult();

            /** Destructor. */
            ~TerrainRayResult() override;

            /** @copydoc ITerrainRayResult::hasIntersected */
            bool hasIntersected() const override;

            /** @copydoc ITerrainRayResult::setIntersected */
            void setIntersected( bool intersected ) override;

            /** @copydoc ITerrainRayResult::getTerrain */
            SmartPtr<IGraphicsTerrain> getTerrain() const override;

            /** @copydoc ITerrainRayResult::setTerrain */
            void setTerrain( SmartPtr<IGraphicsTerrain> terrain ) override;

            /** @copydoc ITerrainRayResult::getPosition */
            Vector3<real_Num> getPosition() const override;

            /** @copydoc ITerrainRayResult::setPosition */
            void setPosition( const Vector3<real_Num> &position ) override;

        protected:
            Vector3<real_Num> m_position = Vector3<real_Num>::zero();
            SmartPtr<IGraphicsTerrain> m_terrain;
            bool m_intersected = false;
        };
    }  // namespace render
}  // namespace workphone

#endif  // TerrainRayResult_h__
