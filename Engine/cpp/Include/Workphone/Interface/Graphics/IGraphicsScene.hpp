#ifndef _WP_ISceneManager_H_
#define _WP_ISceneManager_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Ray3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class IGraphicsScene
         * @brief Renderer-agnostic interface for managing a renderable scene.
         *
         * This interface defines the responsibilities of a scene manager inside the
         * engine. A concrete implementation is expected to be backend specific
         * (for example an Ogre, DirectX or OpenGL based implementation) and is
         * responsible for creating and owning renderer resources, scene nodes and
         * graphics objects. The interface intentionally exposes operations for:
         *  - creating, querying and removing graphics objects and scene nodes,
         *  - configuring global scene-wide rendering parameters (ambient light,
         *    hemisphere lighting, fog, skybox and shadowing),
         *  - performing spatial queries (ray casts) and
         *  - exposing the underlying native renderer object for backend-specific
         *    operations.
         *
         * Ownership and threading:
         *  - Returned objects are wrapped in the project's SmartPtr/ISharedObject
         *    abstractions and follow their shared ownership semantics.
         *  - Concrete implementations should document any thread-safety
         *    guarantees (which typically are limited to the render thread).
         */
        class WPCore_API IGraphicsScene : public ISharedObject
        {
        public:
            /**
             * @enum FrameBufferType
             * @brief Bitmask used to identify framebuffer attachments.
             *
             * These values represent common framebuffer attachments and can be
             * combined with bitwise OR when an operation targets multiple
             * attachments (for example clearing colour and depth at once).
             */
            enum FrameBufferType
            {
                FBT_COLOUR = 0x1,  ///< Color (colour) buffer.
                FBT_DEPTH = 0x2,   ///< Depth buffer.
                FBT_STENCIL = 0x4  ///< Stencil buffer.
            };

            /**
             * @enum FogMode
             * @brief Supported fog calculation methods.
             *
             * The modes represent common fog equations used by renderers. The
             * interpretation of parameters such as density, start and end
             * distances is implementation-defined but follows the conventional
             * meanings described below.
             */
            enum FogMode
            {
                FOG_NONE,   ///< No fog applied.
                FOG_EXP,    ///< Exponential fog.
                FOG_EXP2,   ///< Exponential squared fog.
                FOG_LINEAR  ///< Linear fog between start and end.
            };

            /** Constants used when naming or searching for default scene nodes. */
            static const String sceneNodeStr;
            static const String sceneNodePrefix;

            /** Predefined viewport mask values used to filter objects per-viewport. */
            static const u32 VIEWPORT_MASK_TERRAIN;   ///< Terrain objects mask.
            static const u32 VIEWPORT_MASK_OCCLUDER;  ///< Occluder objects mask.
            static const u32 VIEWPORT_MASK_USER;      ///< User-defined objects mask.
            static const u32 VIEWPORT_MASK_SHADOW;    ///< Shadow caster objects mask.

            /** Bitmask flags used to control shadowing and buffer clearing. */
            static const u32 enableShadowsFlag;  ///< Flag to enable shadows.
            static const u32 depthShadowsFlag;   ///< Flag to enable depth-based shadows.
            static const u32 clearingFlag;       ///< Flag used for buffer clearing operations.

            /**
             * @brief Virtual destructor.
             *
             * Ensure derived destructors release renderer resources and detach
             * the scene from external systems. Declared virtual since this is
             * used polymorphically.
             */
            ~IGraphicsScene() override;

            /**
             * @brief Return a human readable type identifier for this scene.
             *
             * Used by factories and runtime introspection. The exact string is
             * backend dependent (for example "OgreScene" or "GLScene").
             */
            virtual String getType() const = 0;

            /**
             * @brief Set the scene's type identifier.
             * @param type New type identifier string.
             *
             * This value is primarily informational and may be used by factory
             * code or debugging tools.
             */
            virtual void setType( const String &type ) = 0;

            /**
             * @brief Completely reset the scene and release associated resources.
             *
             * Implementations must remove all scene nodes, graphics objects and
             * free renderer resources. After calling this method previously
             * returned SmartPtr references may become invalid.
             */
            virtual void clear() = 0;

            /**
             * @brief Check whether an animation resource exists in the scene.
             * @param animationName Name of the animation to query.
             * @return True if the animation exists, false otherwise.
             *
             * Animations may be referenced by multiple objects; this queries the
             * scene's animation registry rather than per-object animation lists.
             */
            virtual bool hasAnimation( const String &animationName ) = 0;

            /**
             * @brief Remove an animation resource from the scene by name.
             * @param animationName Name of the animation to remove.
             * @return True if the animation was found and removed; false if not found.
             *
             * Implementations should also ensure that objects referencing the
             * animation are notified or updated to avoid dangling references.
             */
            virtual bool destroyAnimation( const String &animationName ) = 0;

            /**
             * @brief Create and register a graphics object in the scene.
             * @param name Requested name assigned to the object. May be used to
             *             look up the object later.
             * @param type String factory identifier describing the concrete type.
             * @return SmartPtr<ISharedObject> to the created object, or nullptr on failure
             *         when exceptions are disabled.
             *
             * The created object will normally implement `IGraphicsObject` or a
             * compatible interface. Concrete factories may perform initialization
             * and resource allocation; failures can either throw or return nullptr
             * depending on project-wide exception configuration.
             */
            virtual SmartPtr<ISharedObject> addGraphicsObject( const String &name,
                                                               const String &type ) = 0;

            /**
             * @brief Create and add a graphics object with an implementation-generated name.
             * @param type String factory identifier describing the concrete type.
             * @return SmartPtr<ISharedObject> to the created object or nullptr on failure.
             *
             * The scene implementation will generate a unique name for the object.
             */
            virtual SmartPtr<ISharedObject> addGraphicsObject( const String &type ) = 0;

            /**
             * @brief Create a graphics object using a hashed type identifier.
             * @param id Hashed type identifier used to resolve the factory.
             * @return SmartPtr<ISharedObject> to the created object or nullptr on failure.
             */
            virtual SmartPtr<ISharedObject> addGraphicsObjectByTypeId( hash_type id ) = 0;

            /**
             * @brief Remove and unregister a graphics object from the scene.
             * @param graphicsObject Smart pointer referencing the object to remove.
             * @return True if the object was removed successfully, false otherwise.
             *
             * Implementations must clear any internal references so that the
             * object's resources can be released. The caller may reset its
             * SmartPtr after calling this method.
             */
            virtual bool removeGraphicsObject( SmartPtr<ISharedObject> graphicsObject ) = 0;

            /**
             * @brief Retrieve the set of top-level graphics objects registered with the scene.
             * @return Array of SmartPtr<IGraphicsObject> representing registered objects.
             *
             * Returned elements represent the objects currently managed by the scene.
             * Depending on the implementation and threading model elements may be
             * null if objects were removed concurrently.
             */
            virtual Array<SmartPtr<IGraphicsObject>> getGraphicsObjects() const = 0;

            /**
             * @brief Return all graphics objects that are derived from the compile-time type T.
             * @tparam T Target type derived from IGraphicsObject.
             * @return Array of SmartPtr<T> containing matching casted objects.
             *
             * Uses runtime type checks to select objects compatible with T. This
             * is a convenience wrapper around `getGraphicsObjects()` and runtime
             * downcasts the results.
             */
            template <class T>
            Array<SmartPtr<T>> getGraphicsObjectsByType() const;

            /**
             * @brief Return the first registered graphics object of type T.
             * @tparam T Target type derived from IGraphicsObject.
             * @return SmartPtr<T> to the first matching object, or nullptr if none exist.
             *
             * Useful when only one instance of a specific graphics object type is
             * expected or when any instance will do.
             */
            template <class T>
            SmartPtr<T> getGraphicsObjectByType() const;

            /**
             * @brief Set the global ambient light colour used for basic lighting.
             * @param colour Ambient light colour to apply.
             *
             * The ambient colour is applied to materials that sample or combine
             * ambient lighting. Exact material behaviour is implementation
             * dependent.
             */
            virtual void setAmbientLight( const ColourF &colour ) = 0;

            /**
             * @brief Retrieve the current ambient light colour.
             * @return ColourF representing the ambient lighting contribution.
             */
            virtual ColourF getAmbientLight() const = 0;

            /**
             * @brief Get the upper hemisphere ambient colour for environment lighting.
             * @return ColourF used for the upper hemisphere contribution.
             */
            virtual ColourF getUpperHemisphere() const = 0;

            /**
             * @brief Set the colour used by the upper hemisphere in image-based lighting.
             * @param upperHemisphere Colour applied to the upper hemisphere.
             */
            virtual void setUpperHemisphere( const ColourF &upperHemisphere ) = 0;

            /**
             * @brief Get the lower hemisphere ambient colour for environment lighting.
             * @return ColourF used for the lower hemisphere contribution.
             */
            virtual ColourF getLowerHemisphere() const = 0;

            /**
             * @brief Set the colour used by the lower hemisphere in image-based lighting.
             * @param lowerHemisphere Colour applied to the lower hemisphere.
             */
            virtual void setLowerHemisphere( const ColourF &lowerHemisphere ) = 0;

            /**
             * @brief Get the hemisphere lighting direction.
             * @return Vector3<real_Num> representing the hemisphere light direction.
             *
             * This vector describes the orientation of the hemisphere lighting
             * used to blend upper and lower hemisphere colours.
             */
            virtual Vector3<real_Num> getHemisphereDir() const = 0;

            /**
             * @brief Set the hemisphere lighting direction.
             * @param hemisphereDir Direction vector describing the "up" direction
             *                      for hemisphere blending.
             */
            virtual void setHemisphereDir( const Vector3<real_Num> &hemisphereDir ) = 0;

            /**
             * @brief Get the environment map contribution scale.
             * @return Environment map intensity as a floating point scale.
             */
            virtual f32 getEnvmapScale() const = 0;

            /**
             * @brief Set the intensity/scale applied to environment map lighting.
             * @param envmapScale Scale factor for environment map contribution.
             */
            virtual void setEnvmapScale( f32 envmapScale ) = 0;

            /**
             * @brief Get the camera currently used to render the scene.
             * @return SmartPtr<IGraphicsCamera> to the active camera, or nullptr if none set.
             */
            virtual SmartPtr<IGraphicsCamera> getActiveCamera() const = 0;

            /**
             * @brief Set the active camera for rendering.
             * @param camera SmartPtr<IGraphicsCamera> to set as the active camera.
             */
            virtual void setActiveCamera( SmartPtr<IGraphicsCamera> camera ) = 0;

            /**
             * @brief Get the default camera associated with the scene.
             * @return SmartPtr<IGraphicsCamera> to the default camera, or nullptr if none.
             */
            virtual SmartPtr<IGraphicsCamera> getDefaultCamera() const = 0;

            /**
             * @brief Set the default camera used by the scene.
             * @param camera SmartPtr<IGraphicsCamera> to assign as the default camera.
             */
            virtual void setDefaultCamera( SmartPtr<IGraphicsCamera> camera ) = 0;

            /**
             * @brief Lookup a scene node by its name.
             * @param name Name of the node to find.
             * @return SmartPtr<IGraphicsSceneNode> to the node, or nullptr if not found.
             */
            virtual SmartPtr<IGraphicsSceneNode> getSceneNode( const String &name ) const = 0;

            /**
             * @brief Retrieve a scene node by its hashed identifier.
             * @param id Hashed id of the node to locate.
             * @return SmartPtr<IGraphicsSceneNode> to the node, or nullptr if not found.
             */
            virtual SmartPtr<IGraphicsSceneNode> getSceneNodeById( hash_type id ) const = 0;

            /**
             * @brief Get the root node of the scene graph.
             * @return SmartPtr<IGraphicsSceneNode> referencing the root scene node.
             */
            virtual SmartPtr<IGraphicsSceneNode> getRootSceneNode() const = 0;

            /**
             * @brief Create a new unnamed child scene node under the root node.
             * @return SmartPtr<IGraphicsSceneNode> referencing the newly created node.
             */
            virtual SmartPtr<IGraphicsSceneNode> addSceneNode() = 0;

            /**
             * @brief Create a new named child scene node under the root node.
             * @param name Name to assign to the node.
             * @return SmartPtr<IGraphicsSceneNode> referencing the newly created node.
             */
            virtual SmartPtr<IGraphicsSceneNode> addSceneNode( const String &name ) = 0;

            /**
             * @brief Remove a scene node and its children from the scene graph.
             * @param sceneNode SmartPtr<IGraphicsSceneNode> referencing the node to remove.
             * @return True if removal succeeded, false otherwise.
             */
            virtual bool removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) = 0;

            /**
             * @brief Enable/disable and configure the scene skybox.
             * @param enable True to enable the skybox, false to disable it.
             * @param material Material used for the skybox faces.
             * @param distance Distance from the camera at which the skybox is rendered.
             *                 (default 5000). Interpretation may be backend-specific.
             * @param drawFirst If true the skybox is rendered before other objects.
             */
            virtual void setSkyBox( bool enable, SmartPtr<IMaterial> material, f32 distance = 5000,
                                    bool drawFirst = true ) = 0;

            virtual void setSkyBox( bool enable, SmartPtr<ITexture> texture, f32 distance = 5000,
                                    bool drawFirst = true ) = 0;

            /**
             * @brief Configure global fog for the scene.
             * @param fogMode Fog equation to use (one of FogMode).
             * @param colour Fog colour (default is white).
             * @param expDensity Density parameter for exponential fog modes.
             * @param linearStart Start distance for linear fog.
             * @param linearEnd End distance for linear fog.
             *
             * Behaviour and parameter interpretation are renderer-specific; callers
             * should follow the semantics expected by the concrete implementation.
             */
            virtual void setFog( u32 fogMode, const ColourF &colour = ColourF::White,
                                 f32 expDensity = 0.001f, f32 linearStart = 0.0f,
                                 f32 linearEnd = 1.0f ) = 0;

            /**
             * @brief Check whether a skybox is currently enabled for the scene.
             * @return True when a skybox is active, false otherwise.
             */
            virtual bool getEnableSkybox() const = 0;

            /**
             * @brief Query whether shadow rendering is enabled for the scene.
             * @return True when shadows are enabled, false otherwise.
             */
            virtual bool getEnableShadows() const = 0;

            /**
             * @brief Enable or disable shadows and select shadowing technique.
             * @param enableShadows True to enable shadows, false to disable.
             * @param depthShadows When true prefer depth-based shadow methods.
             */
            virtual void setEnableShadows( bool enableShadows, bool depthShadows = true ) = 0;

            /**
             * @brief Perform a ray cast against the scene and return the nearest hit.
             * @param ray Ray (origin + direction) to test.
             * @param result Output vector populated with the world-space hit point on success.
             * @return True if the ray intersected an object and `result` contains a valid position.
             *
             * The set of objects used for intersection tests (visual geometry,
             * collision proxies, etc.) is implementation specific and should be
             * documented by the concrete scene implementation.
             */
            virtual bool castRay( const Ray3<real_Num> &ray, Vector3<real_Num> &result ) = 0;

            /**
             * @brief Get the raw pointer to the factory manager used by the scene.
             * @return Non-owning pointer to the IFactoryManager instance.
             */
            virtual IFactoryManager *getFactoryManagerPtr() const = 0;

            /**
             * @brief Get a SmartPtr to the factory manager used by the scene.
             * @return SmartPtr<IFactoryManager> owning or referencing the factory manager.
             */
            virtual SmartPtr<IFactoryManager> getFactoryManager() const = 0;

            /**
             * @brief Set the factory manager used to create scene and graphics objects.
             * @param factoryManager SmartPtr<IFactoryManager> to assign.
             */
            virtual void setFactoryManager( SmartPtr<IFactoryManager> factoryManager ) = 0;

            /**
             * @brief Handle an incoming state message (observer/callback entry).
             * @param message SmartPtr<IStateMessage> containing the message to process.
             * @return True if the message was handled successfully, false otherwise.
             */
            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

            /**
             * @brief Notification that a state object has changed.
             * @param state SmartPtr<IState> reference to the new/changed state.
             * @return True if the scene handled the state change successfully.
             *
             * This is intended for listeners that must update resources or
             * internal caches when a resource or material state transitions.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

            /**
             * @brief Obtain the backend-specific native renderer object.
             * @param ppObject Pointer to a void* which will be set to point to the native object.
             *
             * The exact concrete type stored in `*ppObject` depends on the
             * implementation (for example `Ogre::SceneManager*`). Callers must
             * cast the returned pointer to the appropriate type.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            /**
             * @name Scene and Graphics Object State Contexts
             * Helpers for getting/setting IStateContext instances that are used
             * by scene nodes and graphics objects to manage resource state.
             */
            //@{
            virtual IStateContext *getSceneNodeContextPtr() const = 0;
            virtual SmartPtr<IStateContext> getSceneNodeContext() const = 0;
            virtual void setSceneNodeContext( SmartPtr<IStateContext> context ) = 0;

            virtual IStateContext *getGraphicsObjectContextPtr( u32 typeId ) const = 0;
            virtual SmartPtr<IStateContext> getGraphicsObjectContext( u32 typeId ) const = 0;
            virtual void setGraphicsObjectContext( u32 typeId, SmartPtr<IStateContext> context ) = 0;
            //@}

            /**
             * @brief Create a graphics object using the compile-time type T.
             * @tparam T Type that exposes a static `typeInfo()` used to resolve the factory id.
             * @return SmartPtr<T> pointing to the created object or nullptr on failure.
             *
             * Convenience wrapper that forwards to `addGraphicsObjectByTypeId`.
             */
            template <class T>
            SmartPtr<T> addGraphicsObjectByType();

            /**
             * @brief Retrieve objects managed by the scene that are of type T.
             * @tparam T Desired object type.
             * @return Array of SmartPtr<T> containing objects that match T.
             *
             * Unlike `getGraphicsObjectsByType` this helper performs a type
             * check against the broader scene object set and is useful for
             * non-IGraphicsObject types managed by the scene.
             */
            template <class T>
            Array<SmartPtr<T>> getObjectByType() const;

            WP_CLASS_REGISTER_DECL;
        };

        template <class T>
        SmartPtr<T> IGraphicsScene::addGraphicsObjectByType()
        {
            auto typeInfo = T::typeInfo();
            return addGraphicsObjectByTypeId( typeInfo );
        }

        template <class T>
        Array<SmartPtr<T>> IGraphicsScene::getGraphicsObjectsByType() const
        {
            auto objectsByType = Array<SmartPtr<T>>();
            objectsByType.reserve( 8 );

            auto objects = getGraphicsObjects();
            for( auto obj : objects )
            {
                if( obj )
                {
                    if( obj->isDerived<T>() )
                    {
                        objectsByType.push_back( obj );
                    }
                }
            }

            return objectsByType;
        }

        template <class T>
        SmartPtr<T> IGraphicsScene::getGraphicsObjectByType() const
        {
            auto objects = getGraphicsObjects();
            for( auto obj : objects )
            {
                if( obj->isDerived<T>() )
                {
                    return obj;
                }
            }

            return nullptr;
        }

        template <class T>
        Array<SmartPtr<T>> IGraphicsScene::getObjectByType() const
        {
            Array<SmartPtr<T>> objects;
            objects.reserve( 4 );

            auto graphicsObjects = getGraphicsObjects();
            for( auto &obj : graphicsObjects )
            {
                if( obj )
                {
                    if( obj->isDerived<T>() )
                    {
                        auto castedObj = workphone::static_pointer_cast<T>( obj );
                        objects.push_back( castedObj );
                    }
                }
            }

            return objects;
        }

    }  // end namespace render
}  // namespace workphone

#endif
