#ifndef CSkybox6SidedOgreNext_h__
#define CSkybox6SidedOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/Skybox.hpp>
#include <OgreSceneManager.h>
#include <array>

/**
 * @file CSkybox6SidedOgreNext.hpp
 * @brief Six-sided skybox implementation using the Ogre next-generation API.
 *
 * This header declares `CSkybox6SidedOgreNext`, a Skybox implementation that
 * creates six independently textured planes to represent the faces of a cube
 * surrounding the camera. The class keeps the skybox centered on the camera
 * and allows setting a material per face.
 */
namespace workphone
{
    namespace render
    {
        /**
         * @class CSkybox6SidedOgreNext
         * @brief Six-sided Ogre (next) skybox that follows the rendering camera.
         *
         * The class creates six 2D rectangles positioned around the camera to
         * form a cube. Each face can be assigned a different material. The
         * skybox is updated each frame to remain centered on the camera so it
         * appears infinitely far away.
         */
        class CSkybox6SidedOgreNext : public Skybox, private Ogre::SceneManager::Listener
        {
        public:
            using Skybox::setMaterial;
            using Skybox::setVisible;

            /**
             * @enum Face
             * @brief Index of each cube face and its corresponding axis direction.
             *
             * The mapping corresponds to the direction each face is looking at
             * in world space (relative to the camera):
             * - Front  => +Z
             * - Back   => -Z
             * - Left   => -X
             * - Right  => +X
             * - Top    => +Y
             * - Bottom => -Y
             */
            enum Face
            {
                Front = 0,  ///< +Z
                Back,       ///< -Z
                Left,       ///< -X
                Right,      ///< +X
                Top,        ///< +Y
                Bottom,     ///< -Y
                Count
            };

            /**
             * @brief Default constructor.
             *
             * Constructs an uninitialized skybox. Call the other constructor
             * or set up the scene manager and camera before using.
             */
            CSkybox6SidedOgreNext();

            /**
             * @brief Destructor. Cleans up created Ogre objects.
             */
            ~CSkybox6SidedOgreNext() override;

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
             * @brief Update skybox position/orientation. Call once per frame.
             *
             * Keeps the skybox centered on the camera so it appears stationary
             * relative to the viewer.
             */
            void update();

            /**
             * @brief Assign a material to a specific face of the skybox.
             * @param face The face to set the material for.
             * @param materialName The Ogre material name to apply.
             */
            void setMaterial( Face face, const String &materialName );

            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            void setupStateObject();
            void destroyStateObject();

            bool load( Ogre::SceneManager *sceneMgr, Ogre::Camera *camera, f32 size = 5000.0f );
            void destroyOgreObjects();
            void destroyTextureDatablocks();
            void applyMaterial( SmartPtr<IMaterial> material );
            void applyState( SmartPtr<SkyStateData> skyStateData );
            bool applyFaceMaterial( Face face, const SmartPtr<IMaterial> &material );
            bool applyNamedMaterial( Face face, const String &materialName );
            bool applyFaceTexture( Face face, const SmartPtr<ITexture> &texture );
            void refreshMaterials();
            void refreshVisibility();
            void updatePosition( const Ogre::Camera *camera );
            const Ogre::Camera *getActiveCamera() const;
            void registerSceneListener();
            void unregisterSceneListener();
            void preFindVisibleObjects( Ogre::SceneManager *source,
                                        Ogre::SceneManager::IlluminationRenderStage irs,
                                        Ogre::Viewport *viewport ) override;

            /**
             * @brief Create the rectangle plane for a given cube face.
             * @param face The face to create (one of Face enum values).
             */
            bool createPlane( Face face );

            Ogre::SceneManager *mSceneMgr = nullptr;  ///< Scene manager used to create and manage nodes
            Ogre::Camera *mCamera = nullptr;          ///< Camera the skybox follows
            Ogre::SceneNode *mRootNode = nullptr;     ///< Root node for the skybox geometry
            f32 mSize = 0.0f;                         ///< Size (extent) of the skybox cube
            bool mSceneListenerRegistered = false;

            /**
             * @brief Item objects representing each of the six faces.
             *
             * The array order matches the `Face` enum.
             */
            static constexpr size_t NumFaces = static_cast<size_t>( Face::Count );
            std::array<Ogre::Item *, NumFaces> mFaces = {};
            std::array<Ogre::SceneNode *, NumFaces> mFaceNodes = {};
            std::array<String, NumFaces> mFaceMaterialNames = {};
            std::array<String, NumFaces> mFaceMeshNames = {};
            std::array<String, NumFaces> mTextureDatablockNames = {};
            std::array<SmartPtr<ITexture>, NumFaces> mFaceTextures = {};
            std::array<Ogre::TextureGpu *, NumFaces> mBoundFaceTextures = {};
            std::array<bool, NumFaces> mFaceHasRenderableMaterial = {};
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CSkybox6SidedOgreNext_h__
