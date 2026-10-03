#ifndef _CSceneManager_H_
#define _CSceneManager_H_

#include <Workphone/Graphics/GraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Pool.hpp>
#include <WPGraphicsOgre/WPOgreTypes.hpp>
#include <WPGraphicsOgre/Wrapper/CSceneNodeOgre.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/Set.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class CGraphicsSceneOgre
         * @brief Ogre-based implementation of a graphics scene manager.
         *
         * This class wraps the Ogre::SceneManager and provides a higher-level interface
         * for managing 3D scenes, including meshes, particle systems, terrain, animations,
         * and various graphical objects. It handles object lifecycle, updates, and provides
         * thread-safe operations for scene manipulation.
         *
         * @note This class is thread-safe and uses internal locking mechanisms.
         * @see GraphicsScene, Ogre::SceneManager
         */
        class CGraphicsSceneOgre : public GraphicsScene
        {
        public:
            /**
             * @class MaterialSharedListener
             * @brief Event listener for material-related events in the graphics scene.
             *
             * Handles material loading state changes and destruction events.
             */
            class MaterialSharedListener : public IEventListener
            {
            public:
                /** @brief Default constructor. */
                MaterialSharedListener();

                /** @brief Virtual destructor. */
                ~MaterialSharedListener() override;

                /**
                 * @brief Unloads the associated data.
                 * @param data The shared object to unload.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handles incoming events.
                 * @param eventType The type of event.
                 * @param eventValue Hash value of the event.
                 * @param arguments Array of parameters associated with the event.
                 * @param sender The object that sent the event.
                 * @param object The object associated with the event.
                 * @param event The event object itself.
                 * @return Parameter containing the result of the event handling.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Called when a shared object's loading state changes.
                 * @param sharedObject Pointer to the object that changed state.
                 * @param oldState The previous loading state.
                 * @param newState The new loading state.
                 */
                void loadingStateChanged( ISharedObject *sharedObject, LoadingState oldState,
                                          LoadingState newState );

                /**
                 * @brief Destroys the given pointer.
                 * @param ptr Pointer to destroy.
                 * @return True if destruction was successful, false otherwise.
                 */
                bool destroy( void *ptr );

                /**
                 * @brief Gets the owning graphics scene.
                 * @return Smart pointer to the owning CGraphicsSceneOgre instance.
                 */
                SmartPtr<CGraphicsSceneOgre> getOwner() const;

                /**
                 * @brief Sets the owning graphics scene.
                 * @param owner Smart pointer to the CGraphicsSceneOgre owner.
                 */
                void setOwner( SmartPtr<CGraphicsSceneOgre> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<CGraphicsSceneOgre> m_owner;  ///< Weak pointer to the owning scene.
            };

            /**
             * @class SceneManagerStateListener
             * @brief State listener for scene manager state changes.
             *
             * Handles state messages and state changes for the scene manager.
             */
            class SceneManagerStateListener : public IStateListener
            {
            public:
                /** @brief Default constructor. */
                SceneManagerStateListener();

                /** @brief Virtual destructor. */
                ~SceneManagerStateListener() override;

                /**
                 * @brief Handles state messages.
                 * @param message The state message to handle.
                 * @return True if the message was handled successfully, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handles state changes.
                 * @param state Reference to the new state.
                 * @return True if the state change was handled successfully, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Gets the owning graphics scene.
                 * @return Smart pointer to the owning CGraphicsSceneOgre instance.
                 */
                SmartPtr<CGraphicsSceneOgre> getOwner() const;

                /**
                 * @brief Sets the owning graphics scene.
                 * @param owner Smart pointer to the CGraphicsSceneOgre owner.
                 */
                void setOwner( SmartPtr<CGraphicsSceneOgre> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                AtomicWeakPtr<CGraphicsSceneOgre> m_owner;  ///< Weak pointer to the owning scene.
            };

            /**
             * @class SkyboxStateListener
             * @brief State listener for skybox configuration changes.
             *
             * Monitors and responds to skybox-related state messages and changes.
             */
            class SkyboxStateListener : public IStateListener
            {
            public:
                /** @brief Default constructor. */
                SkyboxStateListener();

                /** @brief Virtual destructor. */
                ~SkyboxStateListener() override;

                /**
                 * @brief Handles skybox state messages.
                 * @param message The state message to handle.
                 * @return True if the message was handled successfully, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handles skybox state changes.
                 * @param state Reference to the new state.
                 * @return True if the state change was handled successfully, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Gets the owning graphics scene.
                 * @return Smart pointer to the owning CGraphicsSceneOgre instance.
                 */
                SmartPtr<CGraphicsSceneOgre> getOwner() const;

                /**
                 * @brief Sets the owning graphics scene.
                 * @param owner Smart pointer to the CGraphicsSceneOgre owner.
                 */
                void setOwner( SmartPtr<CGraphicsSceneOgre> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                AtomicWeakPtr<CGraphicsSceneOgre> m_owner;  ///< Weak pointer to the owning scene.
            };

            /**
             * @class RootSceneNode
             * @brief Root scene node implementation for the graphics scene.
             *
             * Represents the top-level node in the scene graph hierarchy.
             */
            class RootSceneNode : public CSceneNodeOgre
            {
            public:
                /** @brief Default constructor. */
                RootSceneNode();

                /** @brief Virtual destructor. */
                ~RootSceneNode() override;

                /**
                 * @brief Loads the root scene node.
                 * @param data Additional data for loading.
                 */
                void load( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Unloads the root scene node.
                 * @param data Additional data for unloading.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                WP_CLASS_REGISTER_DECL;
            };

            /** @brief Default constructor. */
            CGraphicsSceneOgre();

            /**
             * @brief Constructs a graphics scene with an existing Ogre scene manager.
             * @param sceneManager Pointer to the Ogre scene manager to wrap.
             */
            CGraphicsSceneOgre( Ogre::SceneManager *sceneManager );

            /** @brief Virtual destructor. */
            ~CGraphicsSceneOgre() override;

            /**
             * @brief Loads the graphics scene.
             * @param data Additional data for loading.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the graphics scene.
             * @param data Additional data for unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the graphics scene for the current frame.
             *
             * Performs per-frame updates including object updates, queue processing,
             * and render queue management.
             */
            void update() override;

            /**
             * @brief Adds a graphics object to the scene by name and type.
             * @param name The name for the graphics object.
             * @param type The type of graphics object to create.
             * @return Smart pointer to the created graphics object.
             */
            SmartPtr<ISharedObject> addGraphicsObject( const String &name, const String &type ) override;

            /**
             * @brief Adds a graphics object to the scene by type.
             * @param type The type of graphics object to create.
             * @return Smart pointer to the created graphics object.
             */
            SmartPtr<ISharedObject> addGraphicsObject( const String &type ) override;

            /**
             * @brief Adds a graphics object to the scene by type ID.
             * @param id Hash type ID of the graphics object type.
             * @return Smart pointer to the created graphics object.
             */
            SmartPtr<ISharedObject> addGraphicsObjectByTypeId( hash_type id ) override;

            /**
             * @brief Removes a graphics object from the scene.
             * @param graphicsObject The graphics object to remove.
             * @return True if the object was successfully removed, false otherwise.
             */
            bool removeGraphicsObject( SmartPtr<ISharedObject> graphicsObject ) override;

            /**
             * @brief Gets all graphics objects in the scene.
             * @return Array of smart pointers to all graphics objects.
             */
            Array<SmartPtr<IGraphicsObject>> getGraphicsObjects() const override;

            /**
             * @brief Clears all objects from the scene.
             *
             * Removes all meshes, particle systems, and other graphics objects.
             * @warning This operation cannot be undone.
             */
            void clear() override;

            /**
             * @brief Checks if an animation with the given name exists.
             * @param animationName The name of the animation to check.
             * @return True if the animation exists, false otherwise.
             */
            bool hasAnimation( const String &animationName ) override;

            /**
             * @brief Destroys an animation by name.
             * @param animationName The name of the animation to destroy.
             * @return True if the animation was destroyed, false otherwise.
             */
            bool destroyAnimation( const String &animationName ) override;

            /**
             * @brief Gets the root scene node.
             * @return Smart pointer to the root scene node.
             */
            SmartPtr<IGraphicsSceneNode> getRootSceneNode() const override;

            /**
             * @brief Adds a mesh to the scene with a specific name.
             * @param name The name for the mesh object.
             * @param meshName The name of the mesh resource to load.
             * @return Smart pointer to the created graphics mesh.
             */
            SmartPtr<IGraphicsMesh> addMesh( const String &name, const String &meshName );

            /**
             * @brief Adds a mesh to the scene with an auto-generated name.
             * @param meshName The name of the mesh resource to load.
             * @return Smart pointer to the created graphics mesh.
             */
            SmartPtr<IGraphicsMesh> addMesh( const String &meshName );

            /**
             * @brief Gets a mesh by name.
             * @param name The name of the mesh to retrieve.
             * @return Smart pointer to the graphics mesh, or nullptr if not found.
             */
            SmartPtr<IGraphicsMesh> getMesh( const String &name ) const;

            /**
             * @brief Adds a particle system to the scene.
             * @param name The name for the particle system.
             * @param templateName The name of the particle template to use.
             * @return Smart pointer to the created particle system.
             */
            SmartPtr<IParticleSystem> addParticleSystem( const String &name,
                                                         const String &templateName );

            /**
             * @brief Gets a particle system by name.
             * @param name The name of the particle system to retrieve.
             * @return Smart pointer to the particle system, or nullptr if not found.
             */
            SmartPtr<IParticleSystem> getParticleSystem( const String &name ) const;

            /**
             * @brief Creates an animation state controller.
             * @return Smart pointer to the created animation state controller.
             */
            SmartPtr<IAnimationStateController> createAnimationStateController();

            /**
             * @brief Creates an animation texture controller.
             * @param textureUnit The material texture unit to animate.
             * @param clone Whether to clone the material.
             * @param clonedMaterialName The name for the cloned material (if clone is true).
             * @return Smart pointer to the created animation texture controller.
             */
            SmartPtr<IAnimationTextureControl> createAnimationTextureCtrl(
                SmartPtr<IMaterialTexture> textureUnit, bool clone = false,
                const String &clonedMaterialName = StringUtil::EmptyString );

            /**
             * @brief Sets the skybox for the scene.
             * @param enable Whether to enable the skybox.
             * @param material The material to use for the skybox.
             * @param distance The distance of the skybox from the camera (default: 5000).
             * @param drawFirst Whether to draw the skybox before other objects (default: true).
             */
            void setSkyBox( bool enable, SmartPtr<IMaterial> material, f32 distance = 5000,
                            bool drawFirst = true ) override;

            /**
             * @brief Sets fog parameters for the scene.
             * @param fogMode The fog mode (linear, exponential, etc.).
             * @param colour The color of the fog (default: white).
             * @param expDensity The density for exponential fog (default: 0.001).
             * @param linearStart The start distance for linear fog (default: 0.0).
             * @param linearEnd The end distance for linear fog (default: 1.0).
             */
            void setFog( u32 fogMode, const ColourF &colour = ColourF::White, f32 expDensity = 0.001f,
                         f32 linearStart = 0.0f, f32 linearEnd = 1.0f ) override;

            /**
             * @brief Registers a scene node for per-frame updates.
             * @param sceneNode The scene node to register.
             */
            void registerSceneNodeForUpdates( SmartPtr<IGraphicsSceneNode> sceneNode );

            /**
             * @brief Unregisters a scene node from per-frame updates.
             * @param sceneNode The scene node to unregister.
             * @return True if successfully unregistered, false otherwise.
             */
            bool unregisteredForUpdates( SmartPtr<IGraphicsSceneNode> sceneNode );

            /**
             * @brief Unregisters a scene node from per-frame updates.
             * @param sceneNode Raw pointer to the scene node to unregister.
             * @return True if successfully unregistered, false otherwise.
             */
            bool unregisteredForUpdates( IGraphicsSceneNode *sceneNode );

            /**
             * @brief Registers a graphics object for per-frame updates.
             * @param gfxObject The graphics object to register.
             */
            void registerForUpdates( SmartPtr<IGraphicsObject> gfxObject );

            /**
             * @brief Unregisters a graphics object from per-frame updates.
             * @param gfxObject The graphics object to unregister.
             * @return True if successfully unregistered, false otherwise.
             */
            bool unregisteredForUpdates( SmartPtr<IGraphicsObject> gfxObject );

            /**
             * @brief Unregisters a graphics object from per-frame updates.
             * @param gfxObject Raw pointer to the graphics object to unregister.
             * @return True if successfully unregistered, false otherwise.
             */
            bool unregisteredForUpdates( IGraphicsObject *gfxObject );

            /**
             * @brief Gets the animation name prefix.
             * @return The current animation name prefix.
             */
            String getAnimationNamePrefix() const;

            /**
             * @brief Sets the animation name prefix.
             * @param animationNamePrefix The prefix to use for animation names.
             */
            void setAnimationNamePrefix( const String &animationNamePrefix );

            /**
             * @brief Gets the animation name suffix.
             * @return The current animation name suffix.
             */
            String getAnimationNameSuffix() const;

            /**
             * @brief Sets the animation name suffix.
             * @param animationNameSuffix The suffix to use for animation names.
             */
            void setAnimationNameSuffix( const String &animationNameSuffix );

            /**
             * @brief Creates a terrain object.
             * @return Smart pointer to the created terrain.
             */
            SmartPtr<IGraphicsTerrain> createTerrain();

            /**
             * @brief Destroys a terrain object.
             * @param terrain The terrain to destroy.
             */
            void destroyTerrain( SmartPtr<IGraphicsTerrain> terrain );

            /**
             * @brief Gets a terrain by ID.
             * @param id The ID of the terrain.
             * @return Smart pointer to the terrain, or nullptr if not found.
             */
            SmartPtr<IGraphicsTerrain> getTerrain( u32 id ) const;

            /**
             * @brief Adds a decal cursor to the scene.
             * @param terrainMaterial The material for the terrain.
             * @param decalTextureName The name of the decal texture.
             * @param size The size of the decal cursor.
             * @return Smart pointer to the created decal cursor.
             */
            SmartPtr<IDecalCursor> addDecalCursor( const String &terrainMaterial,
                                                   const String &decalTextureName,
                                                   const Vector2F &size );

            /**
             * @brief Casts a ray into the scene and returns the intersection point.
             * @param ray The ray to cast.
             * @param result Output parameter containing the intersection point.
             * @return True if the ray hit something, false otherwise.
             */
            bool castRay( const Ray3F &ray, Vector3F &result ) override;

            /**
             * @brief Splits a mesh into multiple meshes based on properties.
             * @param mesh The mesh to split.
             * @param properties Properties defining how to split the mesh.
             * @return Array of smart pointers to the resulting mesh fragments.
             */
            Array<SmartPtr<IGraphicsMesh>> splitMesh( SmartPtr<IGraphicsMesh> mesh,
                                                      const SmartPtr<Properties> &properties ) override;

            /**
             * @brief Gets the underlying Ogre scene manager.
             * @return Pointer to the Ogre::SceneManager.
             */
            Ogre::SceneManager *getSceneManager() const;

            /**
             * @brief Gets the internal object pointer.
             * @param ppObject Output pointer to receive the internal object.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Adds an existing graphics object to the scene.
             * @param graphicsObject The graphics object to add.
             */
            void addExistingGraphicsObject( SmartPtr<IGraphicsObject> graphicsObject );

            /**
             * @brief Adds an existing scene node to the scene.
             * @param sceneNode The scene node to add.
             */
            void addExistingSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode );

            /**
             * @brief Creates an instance manager for instanced rendering.
             * @param customName Custom name for the instance manager.
             * @param meshName Name of the mesh to instance.
             * @param groupName Resource group name.
             * @param technique Instancing technique to use.
             * @param numInstancesPerBatch Number of instances per batch.
             * @param flags Additional flags (default: 0).
             * @param subMeshIdx Submesh index to instance (default: 0).
             * @return Smart pointer to the created instance manager.
             */
            SmartPtr<IInstanceManager> createInstanceManager( const String &customName,
                                                              const String &meshName,
                                                              const String &groupName, u32 technique,
                                                              u32 numInstancesPerBatch, u16 flags = 0,
                                                              u16 subMeshIdx = 0 ) override;

            /**
             * @brief Creates an instanced object from an instance manager.
             * @param materialName The material name to use.
             * @param managerName The name of the instance manager.
             * @return Smart pointer to the created instanced object.
             */
            SmartPtr<IInstancedObject> createInstancedObject( const String &materialName,
                                                              const String &managerName ) override;

            /**
             * @brief Destroys an instanced object.
             * @param instancedObject The instanced object to destroy.
             */
            void destroyInstancedObject( SmartPtr<IInstancedObject> instancedObject ) override;

            /**
             * @brief Gets the skybox material.
             * @return Smart pointer to the skybox material.
             */
            SmartPtr<IMaterial> getSkyboxMaterial() const;

            /**
             * @brief Sets the skybox material.
             * @param skyboxMaterial The material to use for the skybox.
             */
            void setSkyboxMaterial( SmartPtr<IMaterial> skyboxMaterial );

            /**
             * @brief Locks the scene for thread-safe access.
             */
            void lock() override;

            /**
             * @brief Attempts to lock the scene without blocking.
             * @return True if the lock was acquired, false otherwise.
             */
            bool try_lock() override;

            /**
             * @brief Unlocks the scene.
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Internal method to add a graphics object.
             * @param graphicsObject The graphics object to add.
             */
            void _addGraphicsObject( SmartPtr<IGraphicsObject> graphicsObject );

            /**
             * @brief Internal method to get all graphics objects.
             * @return Array of smart pointers to all graphics objects.
             */
            Array<SmartPtr<IGraphicsObject>> _getGraphicsObjects() const;

            /**
             * @brief Clears internal delete queues.
             */
            void clearQueues();

            /**
             * @brief Validates a graphics object name.
             * @param name The name to validate.
             * @return True if the name is valid, false otherwise.
             */
            bool validateGfxObjName( const String &name ) const;

            /**
             * @brief Generates a unique name based on a base name.
             * @param baseName The base name to use.
             * @return A unique name string.
             */
            String getUniqueName( const String &baseName ) const;

            /**
             * @brief Extracts mesh information from an Ogre mesh.
             * @param mesh The Ogre mesh pointer.
             * @param vertex_count Output: number of vertices.
             * @param vertices Output: vertex position array.
             * @param index_count Output: number of indices.
             * @param indices Output: index array.
             * @param position Position transformation to apply.
             * @param orient Orientation transformation to apply.
             * @param scale Scale transformation to apply.
             */
            void getMeshInformation( Ogre::MeshPtr mesh, size_t &vertex_count, Ogre::Vector3 *&vertices,
                                     size_t &index_count, unsigned long *&indices,
                                     const Ogre::Vector3 &position, const Ogre::Quaternion &orient,
                                     const Ogre::Vector3 &scale );

            /**
             * @brief Extracts mesh information from an Ogre entity.
             * @param entity The Ogre entity pointer.
             * @param vertex_count Output: number of vertices.
             * @param vertices Output: vertex position array.
             * @param index_count Output: number of indices.
             * @param indices Output: index array.
             * @param position Position transformation to apply.
             * @param orient Orientation transformation to apply.
             * @param scale Scale transformation to apply.
             */
            void getMeshInformation( const Ogre::Entity *entity, size_t &vertex_count,
                                     Ogre::Vector3 *&vertices, size_t &index_count,
                                     unsigned long *&indices, const Ogre::Vector3 &position,
                                     const Ogre::Quaternion &orient, const Ogre::Vector3 &scale );

            Ogre::SceneManager *m_sceneManager =
                nullptr;  ///< Pointer to the underlying Ogre scene manager.

            Ogre::RaySceneQuery *m_pRaySceneQuery = nullptr;  ///< Ray scene query object for raycasting.

            AssimpLoader *m_loader = nullptr;  ///< Assimp loader for importing external model formats.

            using EntityDeleteQueue = Array<Ogre::Entity *>;
            EntityDeleteQueue m_entityDeleteQueue;  ///< Queue of entities pending deletion.

            using SceneNodeDeleteQueue = Array<Ogre::SceneNode *>;
            SceneNodeDeleteQueue m_sceneNodeDeleteQueue;  ///< Queue of scene nodes pending deletion.

            using ParticleSysDeleteQueue = Array<Ogre::ParticleSystem *>;
            ParticleSysDeleteQueue
                m_particleSysDeleteQueue;  ///< Queue of particle systems pending deletion.

            SmartPtr<IGraphicsSceneNode> m_rootSceneNode;  ///< Root scene node of the scene graph.

            String m_animationNamePrefix;  ///< Prefix applied to animation names.
            String m_animationNameSuffix;  ///< Suffix applied to animation names.

            String m_type;  ///< Type identifier for the scene.
            String m_name;  ///< Name of the scene.

            u32 nextRenderQueueUpdate = 0;  ///< Counter for next render queue update.

            std::set<IGraphicsObject *>
                m_registeredGfxObjects;  ///< Set of graphics objects registered for updates.

            Array<SmartPtr<IInstanceManager>>
                m_instanceManagers;  ///< Array of instance managers for instanced rendering.

            Array<SmartPtr<IGraphicsTerrain>> m_terrains;  ///< Array of terrain objects in the scene.

            SmartPtr<IState> m_state;  ///< State object for the scene manager.

            SmartPtr<MaterialSharedListener>
                m_materialSharedListener;                ///< Listener for material events.
            AtomicSmartPtr<IMaterial> m_skyboxMaterial;  ///< Material used for the skybox.
            SmartPtr<SkyboxStateListener>
                m_skyboxMaterialListener;  ///< Listener for skybox state changes.

            atomic_bool m_isClearing = false;  ///< Flag indicating if the scene is being cleared.

            atomic_bool m_enableSkybox = false;  ///< Flag indicating if the skybox is enabled.

            mutable RecursiveMutex m_mutex;  ///< Mutex for thread-safe operations.

            static u32 m_nextGeneratedNameExt;  ///< Static counter for generating unique entity names.
        };
    }  // end namespace render
}  // namespace workphone

#endif
