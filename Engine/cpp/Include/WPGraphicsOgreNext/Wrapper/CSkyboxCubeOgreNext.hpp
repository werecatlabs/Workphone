#ifndef CSkyboxCubeOgreNext_h__
#define CSkyboxCubeOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/SkyboxCube.hpp>
#include <OgrePrerequisites.h>
#include <OgreSceneManager.h>

namespace workphone
{
    namespace render
    {
        /**
         * @class CSkyboxCubeOgreNext
         * @brief Ogre (Next) implementation of a cube skybox.
         *
         * This class provides a cubemap skybox rendered with Ogre objects.
         * It inherits from the abstract `Skybox` interface in the Workphone engine
         * and manages Ogre scene resources required to display a static skybox.
         */
        class CSkyboxCubeOgreNext : public SkyboxCube, private Ogre::SceneManager::Listener
        {
        public:
            using SkyboxCube::setMaterial;
            using SkyboxCube::setVisible;

            /**
             * @brief Default constructor.
             *
             * Creates an empty skybox wrapper. Call `setMaterial` and set up the
             * scene and camera separately before using `update` if this ctor is
             * used.
             */
            CSkyboxCubeOgreNext();

            /**
             * @brief Destructor. Releases any Ogre resources owned by this wrapper.
             */
            ~CSkyboxCubeOgreNext();

            /**
             * @copydoc ISharedObject::load
             *
             * Create or re-create the underlying Ogre terrain resources using
             * the provided `data` (typically scene or resource parameters).
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             *
             * Release Ogre resources and detach from the scene. Safe to call
             * multiple times.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Update the skybox state.
             *
             * This should be called every frame so the skybox can keep its
             * orientation and position consistent with the camera (if required).
             */
            void update();

            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_DECL;  ///< Macro used for class registration in Workphone

        private:
            /**
             * @brief Construct and initialize the skybox.
             *
             * @param sceneMgr Pointer to the Ogre scene manager used to create
             *                 scene nodes and items.
             * @param camera   Pointer to the Ogre camera that the skybox should
             *                 follow/orient to.
             * @param size     Size (extent) of the cube used for the skybox.
             */
            void load( Ogre::SceneManager *sceneMgr, Ogre::Camera *camera, f32 size = 5000.0f );

            void setupStateObject();
            void destroyStateObject();
            void applyMaterial( SmartPtr<IMaterial> material );
            void applyState( SmartPtr<SkyStateData> skyStateData );
            void preFindVisibleObjects( Ogre::SceneManager *source,
                                        Ogre::SceneManager::IlluminationRenderStage irs,
                                        Ogre::Viewport *viewport ) override;

            const Ogre::Camera *getActiveCamera() const;
            void updateSkyFrustum();
            void updateSkyFrustum( const Ogre::Camera *camera );
            void destroySky();
            void destroySkyMaterial();
            void registerSceneListener();
            void unregisterSceneListener();
            Ogre::MaterialPtr getOrCreateSkyMaterial();
            bool bindCubeTexture( SmartPtr<ITexture> cubeTexture );

            Array<SmartPtr<ITexture>> m_textures;     ///< Array of textures used for the cubemap faces
            Ogre::SceneManager *mSceneMgr = nullptr;  ///< Non-owning pointer to Ogre scene manager
            Ogre::Camera *mCamera = nullptr;    ///< Non-owning pointer to the camera the skybox follows
            Ogre::Rectangle2D *mSky = nullptr;  ///< Fullscreen rectangle used to render the cubemap sky
            Ogre::MaterialPtr mSkyMaterial;     ///< Cloned Ogre sky material bound to the cubemap
            f32 mSize = 0.0f;
            bool mOwnsSkyMaterial = false;
            bool mHasRenderableMaterial = false;
            bool mSceneListenerRegistered = false;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CSkyboxCubeOgreNext_h__
