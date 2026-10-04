#ifndef TerrainBlendMapImpl_h__
#define TerrainBlendMapImpl_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/Graphics/ITerrainBlendMap.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Renderer-agnostic, CPU-side implementation of ITerrainBlendMap.
         *
         * This class stores a square float blend buffer in system memory and exposes it
         * through the ITerrainBlendMap interface. It is used by the base Terrain class to
         * provide working blend maps for renderer plugins that do not ship their own
         * native blend map implementation. Renderer plugins that have a GPU-backed blend
         * map (such as CTerrainOgreBlendMap) are free to return their own type instead.
         *
         * The buffer is row-major with index = y * size + x and stores normalised blend
         * weights in the range [0,1]. loadImage()/saveImage() are intentionally no-ops at
         * this layer (image loading is a renderer/resource-system concern); they can be
         * overridden by a renderer-specific blend map.
         */
        class WPCore_API TerrainBlendMapImpl : public ITerrainBlendMap
        {
        public:
            /**
             * @brief Construct an empty blend map owned by the given terrain.
             * @param owner Terrain that owns this blend map.
             * @param size  Resolution of the (square) blend map in texels.
             * @param layer Layer index this blend map corresponds to.
             */
            TerrainBlendMapImpl( SmartPtr<IGraphicsTerrain> owner, u32 size, u32 layer );

            /** Destructor. */
            ~TerrainBlendMapImpl() override;

            /** @copydoc ITerrainBlendMap::loadImage */
            void loadImage( const String &fileName,
                            const String &path = StringUtil::EmptyString ) override;

            /** @copydoc ITerrainBlendMap::saveImage */
            void saveImage( const String &fileName,
                            const String &path = StringUtil::EmptyString ) override;

            /** @copydoc ITerrainBlendMap::getBlendValue */
            f32 getBlendValue( u32 x, u32 y ) override;

            /** @copydoc ITerrainBlendMap::setBlendValue */
            void setBlendValue( u32 x, u32 y, f32 blendValue ) override;

            /** @copydoc ITerrainBlendMap::getSize */
            u32 getSize() const override;

            /** @copydoc ITerrainBlendMap::getOwner */
            SmartPtr<IGraphicsTerrain> getOwner() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Weak reference to the owning terrain (avoids reference cycles). */
            AtomicWeakPtr<IGraphicsTerrain> m_owner;
            /** Square blend buffer (size * size entries, row-major). */
            Array<f32> m_data;
            /** Resolution (width == height) of the blend map. */
            u32 m_size = 0;
            /** Layer index this blend map corresponds to. */
            u32 m_layer = 0;
        };
    }  // namespace render
}  // namespace workphone

#endif  // TerrainBlendMapImpl_h__
