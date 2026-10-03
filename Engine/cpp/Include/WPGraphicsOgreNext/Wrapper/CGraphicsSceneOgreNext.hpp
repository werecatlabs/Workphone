#ifndef __CSceneManagerOgreNext_H_
#define __CSceneManagerOgreNext_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsScene.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Pool.hpp>
#include <Workphone/Core/Set.hpp>
#include <OgreVector3.h>
#include <WPGraphicsOgreNext/Wrapper/CSceneNodeOgreNext.hpp>

namespace workphone
{
    namespace render
    {
        class CCubemapOgreNext;

        /**
         * @brief Scene manager implementation using OgreNext.
         *
         * This class implements the GraphicsScene interface and adapts
         * functionality to Ogre's SceneManager. It owns and manages scene
         * resources such as cameras, lights, meshes, particle systems and
         * instance managers. It also provides utilities for ray casting and
         * scene updates.
         */
        class CGraphicsSceneOgreNext : public GraphicsScene
        {
        public:
            /**
             * @brief Special root scene node used by the scene manager.
             *
             * The RootSceneNode acts as the top-level node for all scene
             * objects created by this scene manager. It overrides load/unload
             * to perform any root-level initialization/cleanup required by
             * OgreNext integration.
             */
            class RootSceneNode : public CSceneNodeOgreNext
            {
            public:
                RootSceneNode();
                RootSceneNode( CGraphicsSceneOgreNext *scene );
                ~RootSceneNode() override;

                /**
                 * @brief Load resources or state for the root node.
                 * @param data Optional data used during loading.
                 */
                void load( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Unload resources or state for the root node.
                 * @param data Optional data used during unloading.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;
            };

            /**
             * @brief Default constructor. Initializes an empty scene wrapper.
             */
            CGraphicsSceneOgreNext();

            /**
             * @brief Construct the scene wrapper using an existing Ogre SceneManager.
             * @param sceneManager Pointer to an already created Ogre::SceneManager.
             */
            CGraphicsSceneOgreNext( Ogre::SceneManager *sceneManager );

            /**
             * @brief Destructor, releases owned resources.
             */
            ~CGraphicsSceneOgreNext() override;

            /**
             * @brief Load scene resources or state.
             * @param data Optional shared data required during load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload scene resources and perform cleanup.
             * @param data Optional data passed when unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Perform per-frame update for the scene.
             */
            void update() override;

            /**
             * @brief Register an OgreNext cubemap for automatic material binding.
             */
            void registerCubemap( CCubemapOgreNext *cubemap );

            /**
             * @brief Remove an OgreNext cubemap from automatic material binding.
             */
            void unregisterCubemap( CCubemapOgreNext *cubemap );

            /**
             * @brief Find the highest priority cubemap that affects a world position.
             */
            CCubemapOgreNext *findBestCubemap( const Vector3F &position, u32 visibilityMask ) const;

            /**
             * @brief Re-evaluate automatic cubemap assignment for meshes in this scene.
             */
            void refreshAutomaticCubemaps();

            /**
             * @brief Perform post-update tasks after the main update loop.
             */
            void postUpdate() override;

#ifdef _DEBUG
            s32 addReference();
            bool removeReference();
#endif

            /**
             * @brief Remove all objects from the scene and release associated resources.
             */
            void clear() override;

            /**
             * @brief Check whether an animation with the given name exists.
             * @param animationName Name of the animation to query.
             * @return true if the animation exists, false otherwise.
             */
            bool hasAnimation( const String &animationName ) override;

            /**
             * @brief Destroy an animation by name.
             * @param animationName Name of the animation to destroy.
             * @return true if the animation was destroyed, false otherwise.
             */
            bool destroyAnimation( const String &animationName ) override;

            /**
             * @brief Adds a graphics object of the specified type to the scene.
             * @param id Hash identifier for the type of graphics object to create.
             * @return A smart pointer to the created graphics object.
             */
            SmartPtr<ISharedObject> addGraphicsObjectByTypeId( hash_type id ) override;

            /**
             * @brief Adds a scene node with the given name.
             * @param name The name of the scene node.
             * @return A smart pointer to the created scene node.
             */
            SmartPtr<IGraphicsSceneNode> addSceneNode( const String &name ) override;

            /**
             * @brief Create and add a camera to the scene.
             * @param name The unique name for the new camera.
             * @return Smart pointer to the created camera.
             */
            SmartPtr<IGraphicsCamera> addCamera( const String &name );

            /**
             * @brief Retrieve a camera by name.
             * @param name Name of the camera.
             * @return Smart pointer to the camera or nullptr if not found.
             */
            SmartPtr<IGraphicsCamera> getCamera( const String &name );

            /**
             * @brief Get the default or active camera for the scene.
             * @return Smart pointer to the active camera.
             */
            SmartPtr<IGraphicsCamera> getCamera() const;

            /**
             * @brief Query whether a camera with the given name exists.
             */
            bool hasCamera( const String &name );

            /**
             * @brief Create and add a light to the scene.
             * @param name Unique name of the light.
             * @return Smart pointer to the created light.
             */
            SmartPtr<IGraphicsLight> addLight( const String &name );

            /**
             * @brief Retrieve a light by name.
             * @param name Name of the light.
             * @return Smart pointer to the light or nullptr if not found.
             */
            SmartPtr<IGraphicsLight> getLight( const String &name ) const;

            /**
             * @brief Add a mesh to the scene with a specific instance name.
             * @param name The unique name to register the mesh under.
             * @param meshName The resource name of the mesh to load.
             */
            SmartPtr<IGraphicsMesh> addMesh( const String &name, const String &meshName );

            /**
             * @brief Add a mesh to the scene using only the mesh resource name.
             * @param meshName The resource name of the mesh to load.
             */
            SmartPtr<IGraphicsMesh> addMesh( const String &meshName );

            /**
             * @brief Create a legacy (v1) mesh representation.
             * @param meshName Name of the mesh resource.
             */
            SmartPtr<IGraphicsMesh> createV1Mesh( const String &meshName );

            /**
             * @brief Retrieve a mesh by its registered name.
             * @param name Registered name of the mesh.
             */
            SmartPtr<IGraphicsMesh> getMesh( const String &name ) const;

            /**
             * @brief Create and add a particle system using a template.
             * @param name Name for the particle system instance.
             * @param templateName Template resource to use when creating the system.
             */
            SmartPtr<IParticleSystem> addParticleSystem( const String &name,
                                                         const String &templateName );

            /**
             * @brief Get a particle system instance by name.
             */
            SmartPtr<IParticleSystem> getParticleSystem( const String &name ) const;

            /**
             * @brief Create an animation state controller for driven animations.
             */
            SmartPtr<IAnimationStateController> createAnimationStateController();

            /**
             * @brief Create an animation texture controller for animated textures.
             * @param textureUnit The material texture unit to control.
             * @param clone If true, the material will be cloned before being modified.
             * @param clonedMaterialName Optional name for the cloned material.
             */
            SmartPtr<IAnimationTextureControl> createAnimationTextureCtrl(
                SmartPtr<IMaterialTexture> textureUnit, bool clone = false,
                const String &clonedMaterialName = StringUtil::EmptyString );

            /**
             * @brief Enable or disable a skybox for the scene.
             * @param enable Whether to enable the skybox.
             * @param material Material used to render the skybox.
             * @param distance Distance from the camera to the skybox.
             * @param drawFirst Whether the skybox should be drawn before all other geometry.
             */
            void setSkyBox( bool enable, SmartPtr<IMaterial> material, f32 distance = 5000,
                            bool drawFirst = true ) override;

            /**
             * @brief Configure scene fog parameters.
             */
            void setFog( u32 fogMode, const ColourF &colour = ColourF::White, f32 expDensity = 0.001f,
                         f32 linearStart = 0.0f, f32 linearEnd = 1.0f ) override;

            /**
             * @brief Register a scene node to receive per-frame updates.
             */
            void registerSceneNodeForUpdates( SmartPtr<IGraphicsSceneNode> sceneNode );

            /**
             * @brief Remove a scene node from the update list by smart pointer.
             * @return true if the node was unregistered, false if it was not found.
             */
            bool unregisteredForUpdates( SmartPtr<IGraphicsSceneNode> sceneNode );

            /**
             * @brief Remove a scene node from the update list by raw pointer.
             */
            bool unregisteredForUpdates( IGraphicsSceneNode *sceneNode );

            /**
             * @brief Register a graphics object to receive updates.
             */
            void registerForUpdates( SmartPtr<IGraphicsObject> gfxObject );

            /**
             * @brief Unregister a graphics object using a smart pointer.
             */
            bool unregisteredForUpdates( SmartPtr<IGraphicsObject> gfxObject );

            /**
             * @brief Unregister a graphics object using a raw pointer.
             */
            bool unregisteredForUpdates( IGraphicsObject *gfxObject );

            /**
             * @brief Get the prefix applied to animation names managed by this scene.
             */
            String getAnimationNamePrefix() const;
            /**
             * @brief Set a prefix that will be applied to created animation names.
             */
            void setAnimationNamePrefix( const String &prefix );

            /**
             * @brief Get the suffix applied to animation names managed by this scene.
             */
            String getAnimationNameSuffix() const;
            /**
             * @brief Set a suffix that will be applied to created animation names.
             */
            void setAnimationNameSuffix( const String &suffix );

            /**
             * @brief Create a terrain object for the scene.
             */
            SmartPtr<IGraphicsTerrain> createTerrain();

            /**
             * @brief Destroy a previously created terrain object.
             */
            void destroyTerrain( SmartPtr<IGraphicsTerrain> terrain );

            /**
             * @brief Retrieve a terrain instance by id.
             */
            SmartPtr<IGraphicsTerrain> getTerrain( u32 id ) const;

            /**
             * @brief Add a decal cursor for projected decals on terrain surfaces.
             */
            SmartPtr<IDecalCursor> addDecalCursor( const String &terrainMaterial,
                                                   const String &decalTextureName,
                                                   const Vector2F &size );

            /**
             * @brief Cast a ray into the scene and retrieve the first hit position.
             * @param ray Ray to cast.
             * @param result Output position of the intersection (if any).
             * @return true if the ray hit something, false otherwise.
             */
            bool castRay( const Ray3F &ray, Vector3F &result ) override;

            /**
             * @brief Split a mesh into smaller meshes according to properties.
             */
            Array<SmartPtr<IGraphicsMesh>> splitMesh( SmartPtr<IGraphicsMesh> mesh,
                                                      const SmartPtr<Properties> &properties ) override;

            /**
             * @brief Access the underlying Ogre SceneManager pointer.
             */
            Ogre::SceneManager *getSceneManager() const;

            /**
             * @brief Retrieve the underlying native object pointer.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Register an existing graphics object that was created externally.
             */
            void addExistingGraphicsObject( SmartPtr<IGraphicsObject> graphicsObject );

            /**
             * @brief Register an existing scene node created outside of this scene wrapper.
             */
            void addExistingSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode );

            /**
             * @brief Create an instance manager used for hardware instancing.
             */
            SmartPtr<IInstanceManager> createInstanceManager( const String &customName,
                                                              const String &meshName,
                                                              const String &groupName, u32 technique,
                                                              u32 numInstancesPerBatch, u16 flags = 0,
                                                              u16 subMeshIdx = 0 ) override;

            /**
             * @brief Create an instanced object using an existing instance manager.
             */
            SmartPtr<IInstancedObject> createInstancedObject( const String &materialName,
                                                              const String &managerName ) override;
            /**
             * @brief Destroy an instanced object previously created.
             */
            void destroyInstancedObject( SmartPtr<IInstancedObject> instancedObject ) override;

            /**
             * @brief Get the factory manager used to create graphics objects.
             */
            SmartPtr<IFactoryManager> getFactoryManager() const;

            /**
             * @brief Set a custom factory manager for creating graphics objects.
             */
            void setFactoryManager( SmartPtr<IFactoryManager> factoryManager );

            /**
             * @brief Handle a state message from the application state machine.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handle changes to the application state.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            void createStateContext();

            void destroyStateContext();

            void clearQueues();

            bool validateGfxObjName( const String &name ) const;

            String getUniqueName( const String &baseName ) const;

            const ConcurrentArray<SmartPtr<IGraphicsSceneNode>> &getSceneNodes() const;
            void setSceneNodes( const ConcurrentArray<SmartPtr<IGraphicsSceneNode>> &sceneNodes );

            const ConcurrentArray<SmartPtr<IGraphicsSceneNode>> &getRegisteredSceneNodes() const;
            void setRegisteredSceneNodes(
                const ConcurrentArray<SmartPtr<IGraphicsSceneNode>> &sceneNodes );

            Ogre::SceneManager *m_sceneManager = nullptr;

            Ogre::RaySceneQuery *m_pRaySceneQuery = nullptr;

            AssimpLoader *m_loader = nullptr;

            using EntityDeleteQueue = Array<Ogre::Entity *>;
            EntityDeleteQueue m_entityDeleteQueue;

            using SceneNodeDeleteQueue = Array<Ogre::SceneNode *>;
            SceneNodeDeleteQueue m_sceneNodeDeleteQueue;

            using ParticleSysDeleteQueue = Array<Ogre::ParticleSystem *>;
            ParticleSysDeleteQueue m_particleSysDeleteQueue;

            String m_animationNamePrefix;
            String m_animationNameSuffix;

            bool m_enableShadows = false;
            bool m_depthShadows = false;

            Array<SmartPtr<IInstanceManager>> m_instanceManagers;
            Array<CCubemapOgreNext *> m_cubemaps;

            /// Value used to generate a unique entity name.
            static u32 m_nextGeneratedNameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif
