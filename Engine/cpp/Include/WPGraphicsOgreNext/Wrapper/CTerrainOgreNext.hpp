#ifndef CTerrainOgreNext_h__
#define CTerrainOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <OgreVector3.h>

namespace workphone
{
    namespace render
    {

        /**
         * @file CTerrainOgreNext.hpp
         * @brief OgreNext-backed terrain implementation interface.
         *
         * This header declares `CTerrainOgreNext`, a concrete implementation of
         * the engine `Terrain` interface using OgreNext terrain primitives.
         */

        /**
         * @brief Terrain implementation for OgreNext.
         *
         * `CTerrainOgreNext` wraps OgreNext terrain functionality and adapts it
         * to the engine's `Terrain` interface. Responsibilities include:
         * - creating and managing the Ogre terrain instance,
         * - providing height queries and conversions between world and terrain space,
         * - maintaining terrain height/texture data and exposing blend maps,
         * - integrating terrain rendering with Ogre's PBS lighting/shadowing via listeners.
         */
        class CTerrainOgreNext : public Terrain
        {
        public:
            // Default value constants
            static const Ogre::Vector3 DEFAULT_DIMENSIONS;
            static const Ogre::Vector3 DEFAULT_LIGHT_DIRECTION;
            static const time_interval DEFAULT_UPDATE_INTERVAL;
            static const time_interval DEFAULT_INITIAL_UPDATE_DELAY;
            static const f32 DEFAULT_LIGHT_EPSILON;
            static const u32 DEFAULT_RENDER_QUEUE_ID;
            static const bool DEFAULT_CAST_SHADOWS;

            static const String TerrainDimensionsStr;
            static const String LightDirectionStr;
            static const String LightEpsilonStr;
            static const String UpdateIntervalStr;
            static const String InitialUpdateDelayStr;
            static const String RenderQueueIdStr;
            static const String CastShadowsStr;
            static const String ShowWireframeStr;
            static const String PositionStr;
            static const String NextUpdateTimeStr;
            static const String HeightMapStr;
            static const String HeightMapPathStr;
            static const String TexturesStr;
            static const String DiffuseTextureStr;
            static const String DetailWeightTextureStr;
            static const String DetailTexture0Str;
            static const String DetailTexture1Str;
            static const String DetailTexture2Str;
            static const String DetailTexture3Str;
            static const String DetailTexture0NMStr;
            static const String DetailTexture1NMStr;
            static const String DetailTexture2NMStr;
            static const String DetailTexture3NMStr;
            static const String DetailRoughness0Str;
            static const String DetailRoughness1Str;
            static const String DetailRoughness2Str;
            static const String DetailRoughness3Str;
            static const String DetailMetalness0Str;
            static const String DetailMetalness1Str;
            static const String DetailMetalness2Str;
            static const String DetailMetalness3Str;
            static const String ReflectionTextureStr;

            /**
             * @brief State listener used to adapt engine state messages to the terrain.
             *
             * The nested `StateListener` class listens for state messages and
             * state changes and forwards relevant events to its owner terrain.
             * It holds a weak reference to the owning `CTerrainOgreNext`.
             */
            class StateListener : public IStateListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                StateListener();

                /**
                 * @brief Destructor.
                 */
                ~StateListener() override;

                /**
                 * @brief Handle an incoming state message.
                 * @param message Message to handle.
                 * @return True if the message was handled, otherwise false.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle a state change notification.
                 * @param state The state that changed.
                 * @return True if handled, otherwise false.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the owner terrain instance (strong pointer).
                 * @return SmartPtr to the owning `CTerrainOgreNext`.
                 */
                SmartPtr<CTerrainOgreNext> getOwner() const;

                /**
                 * @brief Set the owner terrain instance.
                 * @param owner SmartPtr to the owning `CTerrainOgreNext`.
                 */
                void setOwner( SmartPtr<CTerrainOgreNext> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /// Weak pointer to the owning terrain to avoid reference cycles.
                AtomicWeakPtr<CTerrainOgreNext> m_owner;
            };

            /**
             * @brief Default constructor.
             *
             * Initializes members to default values. Does not create Ogre objects.
             */
            CTerrainOgreNext();

            /**
             * @brief Destructor.
             *
             * Ensures Ogre resources are released when the object is destroyed.
             */
            ~CTerrainOgreNext() override;

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
             * @copydoc ISharedObject::reload
             *
             * Reloads terrain resources from `data`. Typically calls `unload`
             * followed by `load`.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Called after the main update step.
             *
             * Performs any deferred updates required after scene update,
             * such as material or shadow listener adjustments that must occur
             * on the render thread or after scene changes.
             */
            void postUpdate() override;

            SmartPtr<Properties> getProperties() const override;

            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get current world-space position of the terrain node.
             * @return Terrain position in world space.
             */
            Vector3F getPosition() const override;

            /**
             * @brief Set world-space position for the terrain node.
             * @param position New world-space terrain position.
             */
            void setPosition( const Vector3F &position ) override;

            /**
             * @brief Query height at a world-space position.
             * @param position World-space position to sample.
             * @return Height (Y) at the given world position.
             *
             * The implementation converts the provided world-space position
             * to terrain local coordinates and samples the terrain height.
             */
            f32 getHeightAtWorldPosition( const Vector3F &position ) const override;

            /**
             * @brief Get the terrain grid size (number of vertices per side).
             * @return Terrain size (u16).
             */
            u16 getSize() const override;

            /**
             * @brief Convert a world-space position into terrain-space coordinates.
             * @param worldSpace Position in world-space to convert.
             * @return Position in terrain local/terrain-space coordinates.
             *
             * Terrain-space is the coordinate system used by the underlying
             * terrain heightmap and sampling functions.
             */
            Vector3F getTerrainSpacePosition( const Vector3F &worldSpace ) const override;

            /**
             * @brief Replace the terrain height data and update the Ogre terrain.
             * @param heightData Array of heights in row-major order matching terrain size.
             *
             * Calling this will typically recreate the internal height image and
             * refresh the Ogre terrain instance to reflect the new heights.
             */
            void _setHeightData( const Array<f32> &heightData );

            /**
             * @brief Get a terrain layer blend map by index.
             * @param index Layer index to query.
             * @return SmartPtr to `ITerrainBlendMap` or null if not available.
             */
            SmartPtr<ITerrainBlendMap> getBlendMap( u32 index ) override;

            /**
             * @brief Get the size (width/height) of the layer blend maps.
             * @return Blend map resolution as u16.
             */
            u16 getLayerBlendMapSize() const override;

            /**
             * @brief Intersect a ray with the terrain geometry.
             * @param ray Ray to test (world space).
             * @return SmartPtr to `ITerrainRayResult` containing intersection details,
             *         or null if there is no intersection.
             */
            SmartPtr<ITerrainRayResult> intersects( const Ray3F &ray ) const override;

            /**
             * @brief Get an engine `IMesh` representation of the terrain.
             * @return SmartPtr to `IMesh` describing the terrain geometry.
             *
             * Useful for physics, exporting, or other systems that operate on mesh data.
             */
            SmartPtr<IMesh> getMesh() const override;

            /**
             * @brief Internal helper to obtain an opaque engine object pointer.
             * @param ppObject Output pointer location to receive the underlying object pointer.
             *
             * Implementation writes the underlying native Ogre object pointer into
             * `*ppObject`. Used by systems that need direct native access.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Set heightmap texture used to generate terrain heights.
             * @param heightMap SmartPtr to a texture containing height values.
             *
             * The texture will be converted into an internal image and used to
             * populate the terrain height data.
             */
            void setHeightMap( SmartPtr<ITexture> heightMap ) override;

            /**
             * @brief Get the Ogre `SceneNode` that contains the terrain.
             * @return Pointer to Ogre::SceneNode or nullptr if not set.
             */
            Ogre::SceneNode *getSceneNode() const;

            /**
             * @brief Set the Ogre `SceneNode` that will contain the terrain.
             * @param sceneNode Pointer to an existing Ogre::SceneNode (not owned).
             */
            void setSceneNode( Ogre::SceneNode *sceneNode );

            Ogre::SceneNode *getTerrainCameraNode() const;

            void setTerrainCameraNode( Ogre::SceneNode *terrainCameraNode );

            Ogre::Camera *getTerrainCamera() const;

            void setTerrainCamera( Ogre::Camera *terrainCamera );

            void setTextureLayer( s32 layer, const String &textureName ) override;

            void setTextures( const Array<SmartPtr<ITexture>> &textures ) override;

            void setTexture( u32 index, SmartPtr<ITexture> texture ) override;

            Ogre::Image2 *getHeightImage() const;

            void setHeightImage( Ogre::Image2 *heightImage );

            Ogre::HlmsPbsTerraShadows *getHlmsPbsTerraShadows() const;

            void setHlmsPbsTerraShadows( Ogre::HlmsPbsTerraShadows *hlmsPbsTerraShadows );

            /**
             * @brief Get the underlying Ogre terrain instance.
             * @return Pointer to `Ogre::Terra` or nullptr if not created.
             */
            Ogre::Terra *getTerra() const;

            /**
             * @brief Set the underlying Ogre terrain instance.
             * @param terra Pointer to an existing `Ogre::Terra` (not owned).
             */
            void setTerra( Ogre::Terra *terra );

            Vector3F getTerrainDimensions() const;

            void setTerrainDimensions( const Vector3F &dimensions );

            Vector3F getLightDirection() const;

            void setLightDirection( const Vector3F &lightDirection );

            f32 getLightEpsilon() const;

            void setLightEpsilon( f32 lightEpsilon );

            time_interval getUpdateInterval() const;

            void setUpdateInterval( time_interval updateInterval );

            time_interval getInitialUpdateDelay() const;

            void setInitialUpdateDelay( time_interval initialUpdateDelay );

            time_interval getNextUpdateTime() const;

            void setNextUpdateTime( time_interval nextUpdateTime );

            u32 getRenderQueueId() const;

            void setRenderQueueId( u32 renderQueueId );

            bool getCastShadows() const;

            void setCastShadows( bool castShadows );

            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Update the terrain material to match current settings.
             *
             * Called internally when terrain textures, lighting or blend maps change.
             */
            void updateMaterial() override;

            /**
             * @brief Create or initialize the underlying Ogre `Terra` instance.
             *
             * Allocates and configures `m_terra` and attaches it to `m_sceneNode`.
             */
            void createTerraInstance();

            /**
             * @brief Convert the current `m_heightData` into an `Ogre::Image2`.
             *
             * This image is used by Ogre as the source for the terrain heights.
             */
            void createImageDataFromHeightData();

            // To mirror active camera properties for correct updates.
            Ogre::SceneNode *m_terrainCameraNode = nullptr;

            // Camera used by the terrain for correct updates.
            Ogre::Camera *m_terrainCamera = nullptr;

            /// Internal image used to upload heightmap data to Ogre (owned).
            Ogre::Image2 *m_heightImage = nullptr;

            /// Scene node which hosts the terrain (not owned).
            Ogre::SceneNode *m_sceneNode = nullptr;

            /// Underlying Ogre terrain instance (not owned).
            Ogre::Terra *m_terra = nullptr;

            /// Listener to make PBS materials be affected by terrain shadows (not owned).
            Ogre::HlmsPbsTerraShadows *m_hlmsPbsTerraShadows = nullptr;

            /// Cached terrain center in world-space (Ogre coordinate system).
            Ogre::Vector3 m_center = Ogre::Vector3::ZERO;

            /// Terrain dimensions (width, height-range, depth).
            Ogre::Vector3 m_dimensions = DEFAULT_DIMENSIONS;

            /// Directional light vector used for terrain lighting/shadow computations.
            Ogre::Vector3 m_lightDirection = DEFAULT_LIGHT_DIRECTION;

            /// Next time (engine time) when the terrain will perform a scheduled update.
            time_interval m_nextUpdateTime = 0.0;

            /// Time between terrain render updates.
            time_interval m_updateInterval = DEFAULT_UPDATE_INTERVAL;

            /// Initial delay before the first terrain render update after load.
            time_interval m_initialUpdateDelay = DEFAULT_INITIAL_UPDATE_DELAY;

            /// Small epsilon used in lighting comparisons to avoid division by zero.
            f32 m_lightEpsilon = DEFAULT_LIGHT_EPSILON;

            /// Ogre render queue used by Terra.
            u32 m_renderQueueId = DEFAULT_RENDER_QUEUE_ID;

            /// Whether the Terra object casts shadows.
            bool m_castShadows = DEFAULT_CAST_SHADOWS;

            static u32 m_cameraNameExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CTerrainOgreNext_h__
