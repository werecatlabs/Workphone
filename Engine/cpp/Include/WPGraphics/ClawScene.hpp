#ifndef ClawScene_h__
#define ClawScene_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsScene.hpp>
#include "workphone_graphics_scene.h"
#include <memory>
#include <mutex>
struct wp_transform_job_stats;
struct wp_graphics_cull_bounds_stats;
struct wp_graphics_cull_sort_stats;
struct wp_graphics_cull_storage_stats;
struct wp_graphics_cull_timing_stats;

namespace workphone
{
    namespace render
    {

        // Forward declaration to keep this header lightweight; the full definition
        // lives in <WPGraphics/Jobs/SceneNodeCullJob.hpp> and is pulled in by ClawScene.cpp.
        class SceneNodeCullJob;
        class CameraVisibilitySet;
        class ClawFoliageBatch;
        class ClawFoliageWorld;
        class ClawRendererDX11;
        struct ClawFoliageInteraction;

        /**
         * @class ClawScene
         * @brief C++ wrapper around the Workphone C graphics scene API.
         *
         * ClawScene derives from GraphicsScene and uses a wp_graphics_scene as the
         * native scene backing.  Where the C scene API exposes a matching property
         * (ambient light, hemisphere lighting, fog, skybox, shadows, envmap scale)
         * the setter updates the C scene and then forwards to the base class so that
         * the C++ state context stays in sync.
         *
         * The scene owns a per-instance FactoryManager so that graphics objects and
         * scene nodes created through the scene are instantiated as Claw-backed types.
         */
        class WPGraphics_API ClawScene : public GraphicsScene
        {
        public:
            ClawScene();
            ClawScene( wp_graphics_scene *scene, bool ownsScene );
            ~ClawScene() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @brief Returns the underlying C graphics scene. */
            wp_graphics_scene *getNativeScene() const;

            /** Detaches and returns the native scene without destroying it. */
            wp_graphics_scene *releaseNativeScene();

            void clear() override;

            /** Select a scene-owned sun without changing other lights. Null restores fallback. */
            bool setPrimaryDirectionalLight( SmartPtr<IGraphicsLight> light );
            /** Visible preferred member, otherwise the first visible Claw directional light. */
            SmartPtr<IGraphicsLight> getPrimaryDirectionalLight() const;

            SmartPtr<ISharedObject> addGraphicsObjectByTypeId( hash_type id ) override;
            bool removeGraphicsObject( SmartPtr<ISharedObject> object ) override;
            SmartPtr<IGraphicsSceneNode> addSceneNode( const String &name ) override;
            SmartPtr<IGraphicsSceneNode> addSceneNode() override;

            void setAmbientLight( const ColourF &colour ) override;
            ColourF getAmbientLight() const override;

            ColourF getUpperHemisphere() const override;
            void setUpperHemisphere( const ColourF &upperHemisphere ) override;

            ColourF getLowerHemisphere() const override;
            void setLowerHemisphere( const ColourF &lowerHemisphere ) override;

            Vector3<real_Num> getHemisphereDir() const override;
            void setHemisphereDir( const Vector3<real_Num> &hemisphereDir ) override;

            f32 getEnvmapScale() const override;
            void setEnvmapScale( f32 envmapScale ) override;

            void setFog( u32 fogMode, const ColourF &colour = ColourF::White, f32 expDensity = 0.001f,
                         f32 linearStart = 0.0f, f32 linearEnd = 1.0f ) override;

            void setSkyBox( bool enable, SmartPtr<IMaterial> material, f32 distance = 5000,
                            bool drawFirst = true ) override;
            void setSkyBox( bool enable, SmartPtr<ITexture> texture, f32 distance = 5000,
                            bool drawFirst = true ) override;

            bool getEnableSkybox() const override;
            bool getEnableShadows() const override;
            void setEnableShadows( bool enableShadows, bool depthShadows = true ) override;

            void _getObject( void **ppObject ) const override;

            /** Submits Claw-backed skies and meshes for the current camera pass. */
            void render( void *renderer );
            /** Render owner: prepare CPU jobs before binding an offscreen target. */
            bool prepareRenderJobs( SmartPtr<IGraphicsCamera> camera );
            bool prepareRenderJobs( SmartPtr<IGraphicsCamera> camera, bool profile,
                                    ClawRendererDX11 *materialRenderer = nullptr );

            /** Owner elapsed CPU time for an explicitly profiled successful
             * preparation. Main includes capture/transforms/culling/sorting and
             * worker waits; shadow includes validation/derivation/sorting.
             * Material includes snapshot capture and cached CPU preparation;
             * total also includes retirement/setup. These are not GPU times. */
            struct PreparationTiming
            {
                u64 total_ns = 0, main_prepare_ns = 0, shadow_derive_ns = 0;
                u64 material_prepare_ns = 0;
            };

            /** False and zeroed when unprofiled, failed or retired. Stale jobs
             * still describe elapsed work, without certifying draw validity. */
            bool getPreparedTiming( PreparationTiming & ) const;
            /** Disjoint native main preparation phases; false/zero when unavailable. */
            bool getPreparedPhaseTiming( wp_graphics_cull_timing_stats * ) const;
            bool validateRenderJobs() const;
            /** Actual transform dispatches for the prepared pass; derived views add none. */
            u32 getPreparedTransformDispatchCount() const;
            /** Owner-only copied main-view preparation work. Transform counters
             * describe the shared publication once, excluding the derived shadow
             * view. Optional outputs are zeroed after retirement; stale prepared
             * jobs still report work performed, without certifying draw validity. */
            bool getPreparedJobStatistics( wp_transform_job_stats *, wp_graphics_cull_bounds_stats *,
                                           wp_u32 *candidates, wp_u32 *visible,
                                           wp_graphics_cull_sort_stats *sort = nullptr ) const;
            /** Copied main-view cull array bytes; shared publication counted once.
             * False and zeroed after retirement, without certifying draw validity. */
            bool getPreparedStorageStatistics( wp_graphics_cull_storage_stats * ) const;
            struct MaterialPreparationStatistics
            {
                u64 records = 0, bindings = 0, meshes = 0, preparations = 0, dense_capacity_bytes = 0;
            };
            /** Copied scene material publication counts; false/zero after retirement. */
            bool getPreparedMaterialStatistics( MaterialPreparationStatistics & ) const;
            void discardRenderJobs();

            using FoliageBatches = Array<std::shared_ptr<const ClawFoliageBatch>>;
            /** Fixed-LOD batches; mutation/retirement uses the render owner thread.
             * A null previous adds; a null replacement removes. Invalid updates
             * preserve the current immutable population. */
            bool replaceFoliageBatch( std::shared_ptr<const ClawFoliageBatch> previous,
                                      std::shared_ptr<const ClawFoliageBatch> replacement,
                                      String &error );
            std::shared_ptr<const FoliageBatches> getFoliageBatches() const;
            void clearFoliageBatches();
            /** Render-owner standalone cooked worlds; terrain worlds attach automatically. */
            bool addFoliageWorld( std::shared_ptr<ClawFoliageWorld>, String &error );
            bool removeFoliageWorld( const std::shared_ptr<ClawFoliageWorld> & );
            void clearFoliageWorlds();
            bool setFoliageInteractions( const Array<ClawFoliageInteraction> &, String &error );
            String getFoliageError() const;

            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            bool handleStateChanged( SmartPtr<IState> &state );

            /**
             * @brief Build and submit a SceneNodeCullJob that culls this scene's
             *        nodes against the supplied camera's frustum.
             *
             * The job is configured with this scene as the traversal root and
             * the camera's frustum as the cull volume, submitted to the
             * application manager's job queue, and cached on the scene so the
             * render() pass and any other lazy consumers on the job queue can
             * read its per-node cull flags via SceneNodeCullJob::isCulled.
             *
             * @param camera The camera whose frustum is used as the cull
             *               volume. A null pointer is accepted but no culling
             *               will be performed (the cached job will simply clear
             *               the cull flag on every visited node).
             * @return The submitted SceneNodeCullJob, or an empty pointer if
             *         the job could not be created or no job queue is
             *         available.
             */
            SmartPtr<SceneNodeCullJob> dispatchCullingJob( SmartPtr<IGraphicsCamera> camera );

            /**
             * @brief Get the per-camera visibility set for the most recent cull.
             *
             * The visibility set is populated asynchronously by the cached
             * cull job (see dispatchCullingJob) and contains the scene nodes
             * that were found visible against the supplied camera's frustum.
             * The render() pass iterates this set when available, falling back to
             * the per-node cull flag (and ultimately the inline frustum test)
             * when no set exists yet.
             *
             * @return SmartPtr<CameraVisibilitySet> The cached visibility set,
             *         or an empty pointer when no culling job has been dispatched.
             */
            SmartPtr<CameraVisibilitySet> getVisibilitySet() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            void createStateContext();
            void unbindNativeRenderObjects();

            /** @brief Native C graphics scene owned by this object. */
            wp_graphics_scene *m_scene;

            ConcurrentArray<SmartPtr<IGraphicsLight>> m_lights;
            WeakPtr<IGraphicsLight> m_primaryDirectionalLight;
            mutable std::mutex m_primaryLightMutex;

            time_interval m_nextCullTime = 0.0;

            bool m_ownsScene = true;

            /**
             * @brief The most recently submitted SceneNodeCullJob, kept alive so
             *        that render() and other lazy consumers on the job queue can
             *        read its per-node cull flags via
             *        SceneNodeCullJob::isCulled.
             */
            SmartPtr<SceneNodeCullJob> m_cullJob;
            std::shared_ptr<const FoliageBatches> m_foliageBatches;
            mutable std::mutex m_foliageMutex;
            struct NativeRenderJobs;
            std::shared_ptr<NativeRenderJobs> m_nativeRenderJobs;
            struct FoliageWorldState;
            std::unique_ptr<FoliageWorldState> m_foliageWorlds;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawScene_h__
