#ifndef ClawScene_h__
#define ClawScene_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsScene.hpp>
#include "workphone_graphics_scene.h"

namespace workphone
{
    namespace render
    {

        // Forward declaration to keep this header lightweight; the full definition
        // lives in <WPGraphics/Jobs/SceneNodeCullJob.hpp> and is pulled in by ClawScene.cpp.
        class SceneNodeCullJob;
        class CameraVisibilitySet;

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

            time_interval m_nextCullTime = 0.0;

            bool m_ownsScene = true;

            /**
             * @brief The most recently submitted SceneNodeCullJob, kept alive so
             *        that render() and other lazy consumers on the job queue can
             *        read its per-node cull flags via
             *        SceneNodeCullJob::isCulled.
             */
            SmartPtr<SceneNodeCullJob> m_cullJob;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawScene_h__
