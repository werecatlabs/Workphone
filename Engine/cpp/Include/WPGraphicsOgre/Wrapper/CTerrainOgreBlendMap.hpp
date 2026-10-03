#ifndef CTerrainBlendMap_h__
#define CTerrainBlendMap_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/ITerrainBlendMap.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace render
    {

        /** Concrete implementation of ITerrainBlendMap for use with Ogre terrain.
         *  Holds and manages a single Ogre TerrainLayerBlendMap. */
        class CTerrainOgreBlendMap : public ITerrainBlendMap
        {
        public:
            /** Constructor. */
            CTerrainOgreBlendMap();

            /** Destructor. */
            ~CTerrainOgreBlendMap() override;

            /** Initialise the blendmap. This overload creates an empty blendmap with the specified
             * size.
             *  @param terrain The terrain the blendmap belongs to.
             *  @param index The index of the layer in the terrain. */
            void initialise( SmartPtr<IGraphicsTerrain> terrain, u32 index );

            /** Initialise the blendmap with an existing Ogre blendmap.
             *  @param blendMap The existing blendmap. */
            void initialise( Ogre::TerrainLayerBlendMap *blendMap );

            /** Update modifications. */
            void updateModifications();

            /** Load an image and apply it to the blendmap.
             *  @param fileName The filename of the image to load.
             *  @param path The path to search for the image. Defaults to an empty string.
             */
            void loadImage( const String &fileName,
                            const String &path = StringUtil::EmptyString ) override;

            /** Save the blendmap to an image.
             *  @param fileName The filename of the image to save.
             *  @param path The path to save the image. Defaults to an empty string.
             */
            void saveImage( const String &fileName,
                            const String &path = StringUtil::EmptyString ) override;

            /** Get a single value of blend information.
             *  @param x The x coordinate of the point of data to get.
             *  @param y The y coordinate of the point of data to get.
             *  @return The blend data.
             */
            f32 getBlendValue( u32 x, u32 y ) override;

            /** Set a single value of blend information.
             *  @param x The x coordinate of the point of data to set.
             *  @param y The y coordinate of the point of data to set.
             *  @param blendValue The blend value to set (0..1).
             */
            void setBlendValue( u32 x, u32 y, f32 blendValue ) override;

            /** Get the size of the blendmap.
             *  @return The size of the blendmap.
             */
            u32 getSize() const override;

            /** Get the terrain that owns this blendmap.
             *  @return The owner of the blendmap.
             */
            SmartPtr<IGraphicsTerrain> getOwner() const override;

            /** Get the underlying Ogre blendmap.
             *  @return The underlying Ogre blendmap.
             */
            Ogre::TerrainLayerBlendMap *getBlendMap() const;

            /** Set the underlying Ogre blendmap.
             *  @param blendMap The Ogre blendmap to set. */
            void setBlendMap( Ogre::TerrainLayerBlendMap *blendMap );

        protected:
            /**
             *  This class listens to state changes for a terrain blend map and responds accordingly.
             */
            class TerrainBlendMapStateListener : public IStateListener
            {
            public:
                /**
                 * Constructor.
                 * @param terrainBlendMap The blendmap to listen to.
                 */
                TerrainBlendMapStateListener( CTerrainOgreBlendMap *terrainBlendMap );

                /**
                 * Destructor.
                 */
                ~TerrainBlendMapStateListener() override;

                /**
                 * Handle state changes.
                 * @param message The state message.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * Handle state changes.
                 * @param state The state to handle.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

            protected:
                /** Pointer to the terrain blend map that this listener is listening to. */
                CTerrainOgreBlendMap *m_terrainBlendMap = nullptr;
            };

            /** The state object for tracking the state of this terrain blend map. */
            SmartPtr<IStateContext> m_stateContext;

            /** The state listener for handling state changes for this terrain blend map. */
            SmartPtr<IStateListener> m_stateListener;

            /** The terrain to which this blend map belongs. */
            CTerrainOgre *m_terrain = nullptr;

            /** The Ogre terrain layer blend map object that this blend map wraps. */
            Ogre::TerrainLayerBlendMap *m_blendMap = nullptr;

            /** The index of this blend map within its owning terrain. */
            s32 m_index = 0;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CTerrainBlendMap_h__
