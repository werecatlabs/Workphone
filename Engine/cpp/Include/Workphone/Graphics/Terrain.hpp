#ifndef __CTerrain_h__
#define __CTerrain_h__

#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/Graphics/ITerrainBlendMap.hpp>
#include <Workphone/Interface/Graphics/ITerrainRayResult.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Graphics/FoliageData.hpp>
#include <mutex>
#include <functional>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Renderer-agnostic implementation of IGraphicsTerrain.
         *
         * Terrain owns a retained immutable rectangular heightfield. CPU sampling,
         * picking and mesh extraction use the same triangles as renderer backends,
         * including in headless tools. Legacy state entries mirror the source for
         * existing listeners; renderer backends own their native GPU resources.
         */
        class WPCore_API Terrain : public SharedGraphicsObject<IGraphicsTerrain>
        {
        public:
            /** Serialisation property key for the heightmap dimensions (Vector2I). */
            static const String HeightMapSizeStr;
            /** Serialisation property key for the vertical height scale (f32). */
            static const String HeightScaleStr;
            /** Serialisation property key for the wireframe toggle (bool). */
            static const String ShowWireframeStr;
            /** Serialisation property key for the material name (String). */
            static const String MaterialNameStr;
            /** Serialisation property key for the visibility toggle (bool). */
            static const String VisibleStr;
            /** Serialisation property key for the heightmap texture resource name. */
            static const String HeightMapStr;

            /**
             * @brief Default constructor.
             *
             * Assigns a unique name/id and configures the render task flag so state
             * updates are processed on the graphics thread.
             */
            Terrain();

            /**
             * @brief Destructor.
             *
             * Releases base state. Derived renderers release their native resources
             * in their own destructor/unload implementation.
             */
            ~Terrain() override;

            /** Retained immutable CPU data; safe to keep while a later revision is published. */
            TerrainSnapshot getTerrainSnapshot() const;
            u64 getTerrainRevision() const;
            /** Complete foliage authoring generations; renderer consumers retain
             * snapshots across page preparation and both colour/shadow passes. */
            bool setFoliageSnapshot( FoliageSnapshot snapshot, String &error );
            FoliageSnapshot getFoliageSnapshot() const;
            struct ActiveFoliage
            {
                FoliageSnapshot snapshot;
                Transform3<real_Num> placement;
            };
            using FoliagePublicationGate =
                std::function<bool( FoliageSnapshot, const Transform3<real_Num> &,
                                    const std::function<void()> &, String & )>;
            void setFoliagePublicationGate( FoliagePublicationGate );
            bool publishFoliageSnapshot( FoliageSnapshot, const Transform3<real_Num> &,
                                         const std::function<void()> &commitRenderer, String &error );
            ActiveFoliage getActiveFoliage() const;
            struct FoliageDiagnostics
            {
                bool currentGenerationReady = false;
                u64 sourceRevision = 0, viewKey = 0;
                u32 residentPages = 0, pendingPages = 0, failedPages = 0;
                u64 residentInstances = 0, testedInstances = 0, culledInstances = 0;
                std::array<u64, 3> lodInstances{};
                u64 groups = 0, predictedDraws = 0, actualDraws = 0, submittedInstances = 0;
                u64 geometryBytes = 0, uploadBytes = 0;
                String error;
            };
            void setFoliageDiagnostics( const FoliageDiagnostics &diagnostics );
            FoliageDiagnostics getFoliageDiagnostics() const;
            bool setFoliageForcedLod( s32 level, String &error );
            s32 getFoliageForcedLod() const;
            /** Validate/copy before publication. Zero expectedRevision means unconditional;
             * a nonzero token rejects stale edits. Failed edits retain the current snapshot.
             */
            bool applyTerrainData( const TerrainData &data, String &error, u64 expectedRevision = 0 );

            /** Preserve component-owned identity/revision instead of assigning a second token. */
            bool requestTerrainSnapshot( TerrainSnapshot snapshot, String &error,
                                         u64 expectedRevision = 0 );
            TerrainSnapshot getActiveTerrainSnapshot() const;
            Transform3<real_Num> getRequestedTerrainWorldTransform() const;
            void setDeferredTerrainPublication( bool deferred );
            bool isTerrainPublicationDeferred() const;
            bool hasPendingTerrainPublication() const;
            using TerrainPublication = std::function<bool( const std::function<void()> &, String & )>;
            using TerrainPublicationGate =
                std::function<bool( TerrainSnapshot, const TerrainPublication &, String & )>;
            void setTerrainPublicationGate( TerrainPublicationGate gate );
            /** Renderer owner calls only after all fallible GPU/material preparation.
             * commitRenderer must contain no-fail swaps. Gate supplies synchronized physics publication.
             */
            bool publishPendingTerrain( TerrainSnapshot expected,
                                        const std::function<void()> &commitRenderer, String &error );

            /**
             * @brief Register the terrain state data in the state context.
             *
             * Creates TerrainStateData, TransformStateData and GraphicsObjectData
             * entries keyed by getId() when a state context is present and seeds them
             * with the current local values. Safe to call multiple times.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Release state and clear cached resources.
             *
             * Removes the terrain-owned state entries and clears the CPU-side blend
             * map cache. Derived renderers should call this from their own unload().
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IGraphicsTerrain::getWorldTransform */
            Transform3<real_Num> getWorldTransform() const override;

            /** @copydoc IGraphicsTerrain::setWorldTransform */
            void setWorldTransform( const Transform3<real_Num> &worldTransform ) override;

            /** @copydoc IGraphicsTerrain::getPosition */
            Vector3<real_Num> getPosition() const override;

            /** @copydoc IGraphicsTerrain::setPosition */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Sample the terrain height at an arbitrary world position.
             *
             * Maps world-space (x,z) to the heightfield and interpolates the rendered
             * triangle. Applies sample height scale and the supported world transform.
             *
             * @param position World-space 3D position to sample.
             * @return f32 Interpolated world-space height (0 outside the footprint).
             */
            f32 getHeightAtWorldPosition( const Vector3<real_Num> &position ) const override;

            /** @copydoc IGraphicsTerrain::getSize */
            u16 getSize() const override;

            /** @copydoc IGraphicsTerrain::getTerrainSpacePosition */
            Vector3<real_Num> getTerrainSpacePosition(
                const Vector3<real_Num> &worldSpace ) const override;

            /**
             * @brief Get a copy of the raw height data array.
             *
             * Row-major: index = z * width + x, where width/depth come from
             * getHeightMapSize(). Values are unscaled height samples.
             */
            Array<f32> getHeightData() const override;

            /**
             * @brief Replace the terrain's internal height data.
             *
             * The supplied array must contain width*depth samples (as defined by
             * getHeightMapSize()); otherwise the call is ignored. Valid data publishes
             * a new snapshot and invalidates cached meshes. Override
             * _onHeightDataChanged() in a backend to trigger a GPU mesh rebuild.
             */
            void setHeightData( const Array<f32> &heightData ) override;

            /** @copydoc IGraphicsTerrain::isVisible */
            bool isVisible() const override;

            /** @copydoc IGraphicsTerrain::setVisible */
            void setVisible( bool visible ) override;

            /** @copydoc IGraphicsTerrain::getShowWireframe */
            bool getShowWireframe() const override;

            /** @copydoc IGraphicsTerrain::setShowWireframe */
            void setShowWireframe( bool showWireframe ) override;

            /** @copydoc IGraphicsTerrain::getMaterialName */
            String getMaterialName() const override;

            /** @copydoc IGraphicsTerrain::setMaterialName */
            void setMaterialName( const String &materialName ) override;

            /**
             * @brief Get the terrain blend map for the specified layer.
             *
             * The base class maintains a small CPU-side blend map registry so that
             * renderer plugins which do not implement their own blend maps still get a
             * working ITerrainBlendMap. Renderer plugins may override this to return a
             * GPU-backed blend map instead.
             *
             * @param index Layer/blend-map index (0-based).
             * @return SmartPtr<ITerrainBlendMap> or nullptr if index is out of range.
             */
            SmartPtr<ITerrainBlendMap> getBlendMap( u32 index ) override;

            /**
             * @brief Return the size (resolution) of a layer blend map.
             *
             * Defaults to a fraction of the heightmap size (clamped to u16). Renderer
             * plugins with native blend maps should override this.
             */
            u16 getLayerBlendMapSize() const override;

            /**
             * @brief Ray/terrain intersection test.
             *
             * Traverses intersected grid cells and tests their actual triangles. The
             * first nonnegative hit supports vertical, upward and non-unit rays.
             *
             * @param ray World-space ray to test.
             * @return SmartPtr<ITerrainRayResult> with the hit (or hasIntersected()==false).
             */
            SmartPtr<ITerrainRayResult> intersects( const Ray3F &ray ) const override;

            /**
             * @brief Return the full-resolution CPU collision/export mesh.
             *
             * Lazily builds and retains a mesh for the current source revision. Returns
             * nullptr if mesh validation or allocation fails.
             */
            SmartPtr<IMesh> getMesh() const override;

            /**
             * @brief Get the heightmap dimensions (width in x, depth in z) as texel counts.
             */
            Vector2I getHeightMapSize() const override;

            /**
             * @brief Set the heightmap dimensions.
             *
             * Both axes are clamped to a minimum of 2. When the size changes the cached
             * height data is resized to width*depth (preserving existing samples where
             * possible) and the state is invalidated. An unchanged size is a true no-op
             * that preserves explicit spacing, origin and samples.
             */
            void setHeightMapSize( const Vector2I &heightMapSize ) override;

            /** @copydoc IGraphicsTerrain::getHeightScale */
            f32 getHeightScale() const override;

            /** @copydoc IGraphicsTerrain::setHeightScale */
            void setHeightScale( f32 heightScale ) override;

            /** @copydoc IGraphicsTerrain::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IGraphicsTerrain::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc IGraphicsTerrain::getSceneManager */
            SmartPtr<IGraphicsScene> getSceneManager() const override;

            /** @copydoc IGraphicsTerrain::setSceneManager */
            void setSceneManager( SmartPtr<IGraphicsScene> sceneManager ) override;

            /**
             * @brief Retrieve the underlying renderer-specific object pointer.
             *
             * The base implementation writes nullptr. Renderer plugins override this to
             * expose their native terrain handle.
             */
            void _getObject( void **ppObject ) const override;

            /** @copydoc IGraphicsTerrain::setTextureLayer */
            void setTextureLayer( s32 layer, const String &textureName ) override;

            /** @copydoc IGraphicsTerrain::getHeightMap */
            SmartPtr<ITexture> getHeightMap() const override;

            /** @copydoc IGraphicsTerrain::setHeightMap */
            void setHeightMap( SmartPtr<ITexture> heightMap ) override;

            /** @copydoc IGraphicsTerrain::getTextures */
            Array<SmartPtr<ITexture>> getTextures() const override;

            /** @copydoc IGraphicsTerrain::setTextures */
            void setTextures( const Array<SmartPtr<ITexture>> &textures ) override;

            /**
             * @brief Get the texture at the requested material layer index.
             *
             * Bounds-checked: returns nullptr when index >= the texture array size.
             */
            SmartPtr<ITexture> getTexture( u32 index ) const override;

            /**
             * @brief Set the texture at the requested material layer index.
             *
             * Grows the texture array as needed so any layer index is valid.
             */
            void setTexture( u32 index, SmartPtr<ITexture> texture ) override;

            /**
             * @brief Called to rebuild/update the terrain material state.
             *
             * The base implementation only invalidates the material-name state field.
             * Renderer plugins override this to refresh GPU material bindings.
             */
            void updateMaterial() override;

            /**
             * @brief Handle an incoming state message (IStateListener).
             *
             * Reacts to load/reload messages so the terrain state can be (re)seeded
             * from editor or gameplay code. Returns true when the message is consumed.
             */
            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief React to a changed state instance (IStateListener).
             *
             * When TerrainStateData changes the relevant fields are pulled back into the
             * local cache so query methods stay consistent without repeatedly touching
             * the state system.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state );

            /** @brief Acquire the graphics system lock (see SharedGraphicsObject). */
            void lock() override;
            /** @brief Release the graphics system lock (see SharedGraphicsObject). */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Renderer lifecycle: no query surface exists until its first complete publication. */
            void resetActiveTerrainPublication();
            mutable std::mutex m_terrainDataMutex;
            mutable std::recursive_mutex m_foliagePublicationMutex;
            FoliageSnapshot m_foliageSnapshot;
            ActiveFoliage m_activeFoliage;
            FoliagePublicationGate m_foliagePublicationGate;
            mutable std::mutex m_foliageDiagnosticsMutex;
            FoliageDiagnostics m_foliageDiagnostics;
            s32 m_foliageForcedLod = -1;
            TerrainSnapshot m_terrainSnapshot;
            TerrainSnapshot m_activeTerrainSnapshot;
            Transform3<real_Num> m_terrainWorldTransform;
            Transform3<real_Num> m_activeTerrainWorldTransform;
            bool m_deferredTerrainPublication = false;
            TerrainPublicationGate m_terrainPublicationGate;
            bool m_terrainVisible = true;
            mutable SmartPtr<IMesh> m_cpuTerrainMesh;
            mutable u64 m_cpuTerrainMeshRevision = 0;
            /** Maximum number of material layers/texture slots expected by the interface. */
            static constexpr u32 MaxTextureLayers =
                static_cast<u32>( IGraphicsTerrain::TextureTypes::COUNT );

            AtomicWeakPtr<IGraphicsScene> m_sceneManager;

            Vector2I m_heightMapSize = Vector2I( 256, 256 );
            f32 m_heightScale = 50.0f;
            String m_materialName;
            bool m_showWireframe = false;

            /** Cached CPU-side blend maps, lazily created by getBlendMap(). */
            Array<SmartPtr<ITerrainBlendMap>> m_blendMaps;

            /** Per-instance counter used to generate unique terrain names. */
            static u32 m_idExt;

            /**
             * @brief Hook invoked whenever the height data or heightmap size changes.
             *
             * The base implementation is a no-op. Renderer plugins override this to mark
             * their GPU mesh dirty (for example ClawTerrain sets m_renderMeshDirty).
             */
            virtual void _onHeightDataChanged();

            /**
             * @brief Hook invoked whenever the height scale changes.
             *
             * The base implementation is a no-op. Renderer plugins override this to mark
             * their GPU mesh dirty.
             */
            virtual void _onHeightScaleChanged();

            /**
             * @brief Ensure the state context has TerrainStateData/TransformStateData/
             * GraphicsObjectData entries for this terrain, seeded from local values.
             */
            void _ensureStateData();

            /**
             * @brief Build a ray-terrain result object for intersects().
             * @return A new ITerrainRayResult with this terrain set as owner.
             */
            SmartPtr<ITerrainRayResult> _makeRayResult( bool hit, const Vector3<real_Num> &pos ) const;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CTerrain_h__
