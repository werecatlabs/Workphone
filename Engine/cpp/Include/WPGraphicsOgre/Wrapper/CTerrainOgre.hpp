#ifndef __CTerrainOgre_h__
#define __CTerrainOgre_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <OgreVector3.h>
#include <OgreRenderTargetListener.h>
#include <OgreTexture.h>

namespace workphone
{
    namespace render
    {
        /**
         * @class CTerrainOgre
         * @brief Ogre-based implementation of the engine's Terrain interface.
         *
         * This class wraps Ogre terrain functionality and exposes the engine-agnostic
         * Terrain interface defined in @c Workphone::Graphics. It manages loading and
         * unloading of terrain data, height queries, blend maps, trees (paged geometry),
         * and various Ogre-specific helpers such as the terrain group and camera used
         * for terrain operations.
         *
         * Responsibilities:
         * - Create/define terrains via Ogre::TerrainGroup.
         * - Provide height and coordinate conversion utilities.
         * - Maintain blend maps and material names for terrain layers.
         * - Optionally manage paged geometry trees and debug overlays.
         *
         * Threading / lifecycle:
         * Instances integrate with the engine's state system through a nested
         * TerrainStateListener that ensures proper unload handling when state changes.
         *
         * @ingroup WPGraphicsOgre_Wrapper
         */
        class CTerrainOgre : public Terrain
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes internal members to default values. Does not create Ogre
             * resources � call @c load or the appropriate init method to allocate
             * Ogre objects.
             */
            CTerrainOgre();

            /**
             * @brief Destructor.
             *
             * Releases owned Ogre resources and unregisters listeners. Ensure that the
             * terrain has been properly unloaded via @c unload before destruction if
             * Ogre manager lifetime is uncertain.
             */
            ~CTerrainOgre() override;

            /**
             * @brief Per-frame update hook.
             *
             * Called by the engine update loop to perform periodic updates such as
             * deferred heightmap updates, paging, and tree updates.
             */
            void update() override;

            /**
             * @brief Load terrain-related data.
             *
             * The provided shared object can contain configuration or resource
             * references required for terrain initialization (heightmaps, layer
             * textures, paging settings, etc.). The concrete interpretation of
             * @p data is implementation-dependent.
             *
             * @param data Shared object that contains load-time data or parameters.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload terrain-related data and free Ogre resources.
             *
             * This should remove terrains from Ogre::TerrainGroup, destroy related
             * textures/meshes, and detach any debug overlays or listeners.
             *
             * @param data Optional shared object used during unload. May be null.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Retrieve raw height data.
             *
             * Returns the height buffer used by the terrain. The representation is
             * engine-defined (typically a row-major array of float heights).
             *
             * @return Array of heights.
             */
            Array<f32> getHeightData() const override;

            /**
             * @brief Get the material name applied to terrain surfaces.
             * @return Material name string.
             */
            String getMaterialName() const override;

            /**
             * @brief Set the material name used by the terrain.
             * @param materialName Name of the material to apply.
             */
            void setMaterialName( const String &materialName ) override;

            /**
             * @brief Get a blend map for the specified layer index.
             *
             * Blend maps control how different texture layers mix across the
             * terrain surface.
             *
             * @param index Layer index of the requested blend map.
             * @return Smart pointer to an ITerrainBlendMap for the layer or null if none.
             */
            SmartPtr<ITerrainBlendMap> getBlendMap( u32 index ) override;

            /**
             * @brief Cast a ray against the terrain and return hit information.
             *
             * Useful for picking, physics height sampling, or line-of-sight tests.
             *
             * @param ray Ray in world space to test against the terrain.
             * @return Terrain ray result with hit distance/position/normal if hit,
             *         or null if no intersection.
             */
            SmartPtr<ITerrainRayResult> intersects( const Ray3F &ray ) const override;

            /**
             * @brief Get the size (width/height) used for layer blend maps.
             * @return Blend map resolution (samples per side).
             */
            u16 getLayerBlendMapSize() const override;

            /**
             * @brief Get a mesh representation of the terrain.
             *
             * Returns an engine mesh object representing the terrain geometry.
             * The mesh may be generated on-demand from the heightmap.
             *
             * @return SmartPtr to IMesh describing the terrain geometry.
             */
            SmartPtr<IMesh> getMesh() const override;

            /**
             * @brief Get the scale applied to raw height values.
             * @return Height scale factor.
             */
            f32 getHeightScale() const override;

            /**
             * @brief Set the scale applied to raw height values.
             * @param heightScale New height scale factor.
             */
            void setHeightScale( f32 heightScale ) override;

            /**
             * @brief Configure Ogre terrain defaults for a terrain light.
             *
             * Sets up default lighting, shader generation settings, and any global
             * parameters required by Ogre::Terrain before creating individual terrains.
             *
             * @param l Pointer to the Ogre light used for terrain illumination.
             */
            void configureTerrainDefaults( Ogre::Light *l );

            /**
             * @brief Define a single terrain tile in the terrain group.
             *
             * This function typically either loads terrain data from disk or creates
             * a flat/empty terrain depending on @p flat.
             *
             * @param x Tile X index.
             * @param y Tile Y index.
             * @param flat If true, generates a flat (zero-height) terrain tile.
             */
            void defineTerrain( long x, long y, bool flat = false );

            /**
             * @brief Helper to load terrain height image into an Ogre::Image.
             *
             * @param flipX If true, flip the image horizontally during load.
             * @param flipY If true, flip the image vertically during load.
             * @param img Out parameter populated with the image data on success.
             * @return True if an image was retrieved and loaded successfully.
             */
            bool getTerrainImage( bool flipX, bool flipY, Ogre::Image &img );

            /**
             * @brief Initialize blend maps for a given Ogre::Terrain.
             *
             * Sets up the layer blend textures and initial values based on the
             * terrain geometry/materials.
             *
             * @param terrain Pointer to the Ogre terrain to initialize.
             */
            void initBlendMaps( Ogre::Terrain *terrain );

            /**
             * @brief Get the underlying Ogre::TerrainGroup for direct Ogre operations.
             * @return Pointer to the Ogre terrain group or null if not created.
             */
            Ogre::TerrainGroup *getTerrainGroup() const;

            /**
             * @brief Set the underlying Ogre::TerrainGroup.
             *
             * Allows external code to assign a pre-created Ogre::TerrainGroup to
             * this wrapper instance.
             *
             * @param terrainGroup Pointer to an existing Ogre::TerrainGroup.
             */
            void setTerrainGroup( Ogre::TerrainGroup *terrainGroup );

            /**
             * @brief Add a debug overlay showing a shadow texture.
             *
             * @param loc Location index of the overlay.
             * @param num Numerical id used to compose unique overlay names.
             */
            void addTextureShadowDebugOverlay( int loc, size_t num );

            /**
             * @brief Add overlay to debug a texture by name.
             *
             * @param loc Overlay location index.
             * @param texname Name of the texture resource to display.
             * @param i Index used for unique overlay identification.
             */
            void addTextureDebugOverlay( int loc, const Ogre::String &texname, size_t i );

            /**
             * @brief Get internal engine object pointer.
             *
             * Used by bindings or RTTI helpers to obtain a raw pointer to the
             * native object (Ogre-side) this wrapper represents.
             *
             * @param ppObject Out parameter set to pointer to underlying object.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get the associated graphics scene manager wrapper.
             * @return Smart pointer to the IGraphicsScene instance used by this terrain.
             */
            SmartPtr<IGraphicsScene> getSceneManager() const override;

            /**
             * @brief Set the graphics scene manager wrapper that owns this terrain.
             * @param sceneManager Smart pointer to the scene manager.
             */
            void setSceneManager( SmartPtr<IGraphicsScene> sceneManager ) override;

            /**
             * @brief Get the raw Ogre SceneManager used by this terrain.
             * @return Ogre::SceneManager pointer or nullptr.
             */
            Ogre::SceneManager *getOgreSceneManager() const;

            /**
             * @brief Get the Ogre camera used for terrain operations (paging, shadows).
             * @return Pointer to the terrain camera or nullptr.
             */
            Ogre::Camera *getTerrainCamera() const;

            /**
             * @brief Set the Ogre camera used for terrain operations.
             * @param terrainCamera Pointer to an existing Ogre::Camera instance.
             */
            void setTerrainCamera( Ogre::Camera *terrainCamera );

            /**
             * @brief Get the scene node that contains the terrain camera.
             * @return Ogre::SceneNode pointer or nullptr.
             */
            Ogre::SceneNode *getTerrainCameraSceneNode() const;

            /**
             * @brief Set the scene node that contains the terrain camera.
             * @param terrainCameraSceneNode Pointer to an Ogre::SceneNode.
             */
            void setTerrainCameraSceneNode( Ogre::SceneNode *terrainCameraSceneNode );

            /**
             * @brief Access the PagedGeometry trees manager used for vegetation.
             * @return Pointer to Forests::PagedGeometry or nullptr.
             */
            Forests::PagedGeometry *getTrees() const;

            /**
             * @brief Assign the paged-geometry trees manager used for vegetation.
             * @param trees Pointer to Forests::PagedGeometry instance.
             */
            void setTrees( Forests::PagedGeometry *trees );

            /**
             * @brief Get terrain tile size (heightmap resolution).
             * @return Tile size in samples (e.g. 129).
             */
            u16 getTerrainSize() const;

            /**
             * @brief Set terrain tile size (heightmap resolution).
             * @param terrainSize Number of samples per side.
             */
            void setTerrainSize( u16 terrainSize );

            /**
             * @brief Get the world size covered by a single terrain tile.
             * @return World size in world units.
             */
            f32 getTerrainWorldSize() const;

            /**
             * @brief Set the world size covered by a single terrain tile.
             * @param terrainWorldSize Size in world units.
             */
            void setTerrainWorldSize( f32 terrainWorldSize );

            /**
             * @brief Set a texture name for a specific texture layer index.
             * @param layer Zero-based layer index.
             * @param textureName Name of the texture resource to use.
             */
            void setTextureLayer( s32 layer, const String &textureName ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Listener subclass that integrates terrain with the engine state system.
             *
             * This listener ensures the terrain is properly unloaded when the owning
             * state is unloaded, and can respond to state messages as required.
             */
            class TerrainStateListener : public IStateListener
            {
            public:
                TerrainStateListener();
                ~TerrainStateListener() override;

                /**
                 * @brief Called when the state is being unloaded.
                 * @param data Optional data passed during unload.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle an incoming state message.
                 * @param message The state message to handle.
                 * @return True if the message was handled, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Callback invoked when the tracked state has changed.
                 * @param state The new state.
                 * @return True if handled.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the owner terrain wrapper.
                 * @return Smart pointer to the owning CTerrainOgre.
                 */
                SmartPtr<CTerrainOgre> getOwner() const;

                /**
                 * @brief Set the owner terrain wrapper.
                 * @param owner Smart pointer to the owning CTerrainOgre.
                 */
                void setOwner( SmartPtr<CTerrainOgre> owner );

            private:
                AtomicWeakPtr<CTerrainOgre> m_owner; /**< Weak reference to the CTerrainOgre owner. */
            };

            /**
             * @brief Small helper that listens for Ogre render target updates to capture
             *        shadow/texture data for debug or readback purposes.
             *
             * This class attaches to an Ogre::Texture render target and copies its
             * pixel data in pre-render callbacks.
             */
            class ShadowRenderTargetListener : public Ogre::RenderTargetListener
            {
            public:
                /**
                 * @brief Construct the listener for a given texture.
                 * @param tex Texture to monitor (Ogre texture pointer).
                 */
                ShadowRenderTargetListener( Ogre::TexturePtr tex );
                ~ShadowRenderTargetListener() override;

                /**
                 * @brief Called before the render target is updated.
                 * @param evt Render target event.
                 */
                void preRenderTargetUpdate( const Ogre::RenderTargetEvent &evt ) override;

                /**
                 * @brief Called before each viewport attached to the render target is updated.
                 * @param evt Render target viewport event.
                 */
                void preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt ) override;

            protected:
                Ogre::TexturePtr m_tex;        /**< Texture being listened to. */
                Ogre::PixelBox *box = nullptr; /**< PixelBox used for readback (owned/managed). */
                float *data = nullptr;         /**< Raw float buffer used for pixel readback. */
                u32 m_nextUpdate = 0;          /**< Frame counter for throttling updates. */
            };

            /** Load/create all terrain tiles, groups and related resources. */
            void loadTerrain();

            /** Load paged-geometry trees and vegetation data. */
            void loadTrees();

            /** Helper to set tree visibility mask on a scene node (used by PagedGeometry). */
            void setTreeMask( Ogre::SceneNode *node );

            Ogre::Camera *m_terrainCamera = nullptr; /**< Camera used for terrain operations. */
            Ogre::SceneNode *m_terrainCameraSceneNode =
                nullptr; /**< Scene node that holds the terrain camera. */

            // Pointers to PagedGeometry class instances:
            Forests::PagedGeometry *m_trees = nullptr; /**< Vegetation/paged geometry manager. */

            Ogre::TerrainGroup *mTerrainGroup = nullptr;   /**< Ogre terrain group instance. */
            Ogre::TerrainPaging *mTerrainPaging = nullptr; /**< Ogre terrain paging helper (if used). */
            Ogre::PageManager *mPageManager = nullptr;     /**< Ogre page manager for paged terrain. */

            AtomicSmartPtr<IGraphicsScene> m_sceneManager; /**< Backing graphics scene wrapper. */

            f32 mHeightUpdateCountDown = 0.0f; /**< Countdown timer until next height update. */
            f32 mHeightUpdateRate = 0.0f;      /**< Frequency (seconds) between height updates. */
            Ogre::Vector3 mTerrainPos =
                Ogre::Vector3::ZERO; /**< Cached world position of terrain origin. */

            u16 m_terrainSize = 129;         /**< Heightmap/sample size per tile (default 129). */
            f32 m_terrainWorldSize = 512.0f; /**< World-space size covered by one terrain tile. */

            bool mTerrainsImported = false; /**< True if terrains were imported from files. */
            bool mPaging = false;           /**< True if terrain paging is enabled. */

            bool m_isVisible = false; /**< Visibility flag for the terrain. */

            Array<SmartPtr<ITerrainBlendMap>> m_blendLayers; /**< Blend layers used by the terrain. */

            static u32 m_ext; /**< Extension / internal use field. */
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CTerrain_h__
