#ifndef TerrainSystem_h__
#define TerrainSystem_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <mutex>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Component responsible for managing terrain within a scene.
         *
         * The TerrainSystem stores terrain resources such as height maps,
         * layers, textures and tree/grass generation settings. It owns the
         * runtime terrain render object and associated scene node used to
         * display terrain geometry. The component is responsible for loading
         * and applying serialized properties, creating the render terrain,
         * and synchronizing runtime state with editable properties.
         */
        class WPCore_API TerrainSystem : public Component
        {
        public:
            // Property key strings
            static const String UpdateMaterialStr;
            static const String LayersStr;
            static const String HeightMapStr;
            static const String HeightMapSizeStr;
            static const String HeightScaleStr;
            static const String ShowWireframeStr;
            static const String GenerateStr;
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
            static const String MetalnessStr;
            static const String DetailMetalnessValue0Str;
            static const String DetailMetalnessValue1Str;
            static const String DetailMetalnessValue2Str;
            static const String DetailMetalnessValue3Str;
            static const String DefaultMetalnessValueStr;
            static const String TreeSettingsStr;
            static const String TreesEnableStr;
            static const String TreeDensityStr;
            static const String GrassSettingsStr;
            static const String GrassEnableStr;
            static const String GrassDensityStr;
            static const String TreesStr;
            static const String GenerateTreesStr;
            static const String AddTreesStr;
            static const String RemoveTreesStr;
            static const String TreeLayerStr;
            static const String TreeTextureStr;
            static const String GeneratedHeightMapWidthStr;
            static const String GeneratedHeightMapHeightStr;
            static const String GeneratedHeightMapValueScaleStr;
            static const String GeneratedHeightMapTypeStr;
            static const String PreviewTreeCountStr;
            static const String GeneratedTreeCountStr;
            static const String TreePrefabStr;

            static const Array<String> terrainTypes;

            /**
             * @brief Default constructor.
             *
             * Initializes member variables to sensible defaults. The actual
             * GPU-side terrain object is created later by createTerrain() when
             * the rendering subsystem and required resources are available.
             */
            TerrainSystem();

            /**
             * @brief Deleted copy constructor to prevent copying of component
             * state and internal resource handles.
             */
            TerrainSystem( const TerrainSystem &other ) = delete;

            /**
             * @brief Destructor. Releases owned resources and unregisters any
             * listeners from render objects.
             */
            ~TerrainSystem() override;

            /**
             * @brief Load component state from serialized data.
             *
             * @copydoc Component::load
             * @param data Shared object that contains serialized component
             * properties (usually a Properties instance).
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload component state and release runtime resources.
             *
             * @copydoc Component::unload
             * @param data Optional shared object containing unload context.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Called when component flags change.
             *
             * Update internal behavior based on the new flags.
             * @copydoc Component::updateFlags
             * @param flags The new flags value.
             * @param oldFlags The previous flags value.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @brief Export the component's editable properties.
             *
             * The returned Properties object is used by editors and serializers
             * to inspect and persist the component configuration.
             * @copydoc Component::getProperties
             * @return SmartPtr<Properties> containing current property values.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply properties previously created by getProperties().
             *
             * @copydoc Component::setProperties
             * @param properties Properties object containing values to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Called when the component's transform changes.
             *
             * Updates the attached terrain scene node to reflect the new
             * transform. @copydoc IComponent::updateTransform
             */
            void updateTransform() override;

            /**
             * @brief Calculate the current number of terrain layers based on
             * internal layer and texture arrays.
             *
             * This does not modify state; it inspects configured arrays and
             * returns how many layers should be active.
             * @return Number of layers (may be zero).
             */
            s32 calculateNumLayers() const;

            /**
             * @brief Get the configured number of terrain layers.
             * @return Number of layers.
             */
            s32 getNumLayers() const;

            /**
             * @brief Add a new terrain layer and return its descriptor.
             * @return SmartPtr<TerrainLayer> to the newly created layer.
             */
            SmartPtr<TerrainLayer> addLayer();

            /**
             * @brief Remove a terrain layer by index.
             * @param index Index of the layer to remove.
             */
            void removeLayer( s32 index );

            /**
             * @brief Remove the specified terrain layer.
             * @param layer SmartPtr to the TerrainLayer to remove.
             */
            void removeLayer( SmartPtr<TerrainLayer> layer );

            /**
             * @brief Resize the layer array to contain numLayers entries.
             * @param numLayers Desired number of layers.
             */
            void setNumLayers( s32 numLayers );

            /**
             * @brief Create and add a tree layer to the terrain.
             * @return SmartPtr<TerrainTreeLayer> to the newly added layer.
             */
            SmartPtr<TerrainTreeLayer> addTreeLayer();

            /**
             * @brief Create and add a grass layer to the terrain.
             * @return SmartPtr<TerrainGrassLayer> to the newly added layer.
             */
            SmartPtr<TerrainGrassLayer> addGrassLayer();

            /**
             * @brief Retrieve the height map texture used by the terrain.
             * @return SmartPtr to the ITexture representing the height map.
             */
            SmartPtr<render::ITexture> getHeightMap() const;

            /**
             * @brief Set the height map texture for the terrain.
             * @param heightMap Texture containing height values (grayscale).
             */
            void setHeightMap( SmartPtr<render::ITexture> heightMap );

            /**
             * @brief Get the vertical scale applied to height map values.
             * @return Height scale multiplier.
             */
            f32 getHeightScale() const;

            /**
             * @brief Set the vertical scale applied to raw height map values.
             * @param heightScale Scale factor in world units.
             */
            void setHeightScale( f32 heightScale );

            Vector2I getHeightMapSize() const;
            void setHeightMapSize( const Vector2I &heightMapSize );

            bool getShowWireframe() const;
            void setShowWireframe( bool showWireframe );

            u32 getGeneratedHeightMapWidth() const;
            void setGeneratedHeightMapWidth( u32 width );

            u32 getGeneratedHeightMapHeight() const;
            void setGeneratedHeightMapHeight( u32 height );

            f32 getGeneratedHeightMapValueScale() const;
            void setGeneratedHeightMapValueScale( f32 valueScale );

            String getGeneratedHeightMapType() const;
            void setGeneratedHeightMapType( const String &heightMapType );

            /**
             * @brief Generate procedural height data using the configured
             * editor generation parameters.
             */
            void generateHeightMap();

            /**
             * @brief Rebuild and synchronize the runtime terrain object after
             * editor-side shape, heightmap, or layer changes.
             */
            void rebuild();

            f32 getDefaultMetalnessValue() const;
            void setDefaultMetalnessValue( f32 defaultMetalnessValue );

            bool getTreesEnabled() const;
            void setTreesEnabled( bool enabled );

            f32 getTreeDensity() const;
            void setTreeDensity( f32 density );

            bool getGrassEnabled() const;
            void setGrassEnabled( bool enabled );

            f32 getGrassDensity() const;
            void setGrassDensity( f32 density );

            u32 getPreviewTreeCount() const;
            void setPreviewTreeCount( u32 count );

            u32 getGeneratedTreeCount() const;
            void setGeneratedTreeCount( u32 count );

            String getTreePrefabName() const;
            void setTreePrefabName( const String &treePrefabName );

            /**
             * @brief Returns the runtime graphics terrain object used for
             * rendering.
             * @return SmartPtr to the IGraphicsTerrain instance, or null if
             * none exists.
             */
            SmartPtr<render::IGraphicsTerrain> getTerrain() const;

            /** Authoritative samples survive renderer recreation and scene serialization.
             * Mutations are owner-thread commits; retained snapshots are safe to read.
             */
            render::TerrainSnapshot getTerrainSnapshot() const;
            u64 getTerrainRevision() const;
            bool applyTerrainData( const render::TerrainData &data, String &error,
                                   u64 expectedRevision = 0 );
            String exportTerrainData() const;
            bool importTerrainData( const String &json, String &error );
            SmartPtr<ISharedObject> toData() const override;

            /**
             * @brief Attach a graphics terrain instance to this component.
             *
             * The provided terrain object will be used for rendering and will
             * be managed by this component while attached.
             * @param terrain SmartPtr to an IGraphicsTerrain implementation.
             */
            void setTerrain( SmartPtr<render::IGraphicsTerrain> terrain );

            /**
             * @brief Synchronize the component's layer descriptors with the
             * underlying render terrain so that textures, weights and other
             * material parameters are up to date.
             */
            void updateLayers();

            /**
             * @brief Resize internal layout/weight maps to match the current
             * heightmap or layer count. Called when resolution or layer
             * configuration changes.
             */
            void resizeLayermap();

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Listener class used to receive events from the terrain
             * graphics object.
             *
             * The listener forwards relevant notifications to the owning
             * TerrainSystem instance and observes loading state changes so
             * that the component can update its internal pointers.
             */
            class TerrainObjectListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor for the listener.
                 */
                TerrainObjectListener();

                /**
                 * @brief Destructor. Automatically unregisters from observed
                 * objects if necessary.
                 */
                ~TerrainObjectListener() override;

                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle an incoming event from the observed object.
                 * @copydoc IEventListener::handleEvent
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Called when the observed object's loading state
                 * changes.
                 * @param sharedObject The object whose state changed.
                 * @param oldState Previous loading state.
                 * @param newState New loading state.
                 */
                void loadingStateChanged( ISharedObject *sharedObject, LoadingState oldState,
                                          LoadingState newState );

                /**
                 * @brief Called to destroy the listener object when the
                 * runtime requires it. Implements ISharedObject-style destroy
                 * semantics.
                 * @param ptr Optional pointer passed by manager code.
                 * @return true if destruction was successful.
                 */
                bool destroy( void *ptr );

                /**
                 * @brief Retrieve the TerrainSystem that owns this listener.
                 * @return SmartPtr to the owning TerrainSystem, may be null.
                 */
                SmartPtr<TerrainSystem> getOwner() const;

                /**
                 * @brief Assign the owning TerrainSystem for this listener.
                 * @param owner SmartPtr to the TerrainSystem that should be
                 * considered the owner.
                 */
                void setOwner( SmartPtr<TerrainSystem> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /** Weak pointer to the owning TerrainSystem instance. */
                AtomicWeakPtr<TerrainSystem> m_owner;
            };

            /**
             * @brief Create and attach the runtime graphics terrain object.
             *
             * This sets up m_terrain and m_node so the terrain can be rendered.
             * The function assumes required resources and renderer are ready.
             */
            void createTerrain();

            /**
             * @brief Update the component's visibility state.
             *
             * Synchronizes the scene node's visibility with the component's
             * flags and any editor preview state. @copydoc Component::updateVisibility
             */
            void updateVisibility() override;

            /**
             * @brief Handle internal FSM events for the component.
             * @param state Current FSM state.
             * @param eventType The event that occurred.
             * @return FSMReturnType indicating how the FSM should proceed.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Retrieve a tree prefab by index.
             * @param idx Index of the prefab to retrieve.
             * @return SmartPtr to the IGamePrefab, or null if not found.
             */
            SmartPtr<IGamePrefab> getTreePrefab( s32 idx ) const;

            /** Listener instance used to observe the graphics terrain object. */
            SmartPtr<TerrainObjectListener> m_terrainListener;

            /** Height map texture for the terrain (may be generated procedurally). */
            SmartPtr<render::ITexture> m_heightMap;

            /**
             * Runtime graphics terrain instance used to render terrain
             * geometry and materials.
             */
            SmartPtr<render::IGraphicsTerrain> m_terrain;

            mutable std::mutex m_terrainDataMutex;
            render::TerrainSnapshot m_terrainData;

            /** Scene node the terrain is attached to. */
            SmartPtr<render::IGraphicsSceneNode> m_node;

            /** Vertical scale applied to raw heightmap values when constructing geometry. */
            f32 m_heightScale = 50.0f;

            /** Height map dimensions used by generated terrain data and the renderer. */
            Vector2I m_heightMapSize = Vector2I( 256, 256 );

            /** Whether the runtime terrain should render as wireframe. */
            bool m_showWireframe = false;

            /** Number of active texture/material layers on the terrain. */
            s32 m_numLayers = 0;

            /** Width of a procedurally generated heightmap (pixels). */
            u32 m_generatedHeightMapWidth = 256;

            /** Height of a procedurally generated heightmap (pixels). */
            u32 m_generatedHeightMapHeight = 256;

            /** Scale applied to generated height values to convert them to world units. */
            f32 m_generatedHeightMapValueScale = 255.0f;

            /** Procedural heightmap type: gradient, island, or mountains. */
            String m_generatedHeightMapType = "gradient";

            /** Default metalness value applied to layers that do not specify one. */
            f32 m_defaultMetalnessValue = 0.5f;

            /** Whether tree rendering and placement is enabled for this terrain. */
            bool m_treesEnabled = false;

            /** Density factor controlling how many trees are generated per unit area. */
            f32 m_treeDensity = 0.0f;

            /** Whether grass rendering and placement is enabled for this terrain. */
            bool m_grassEnabled = false;

            /** Density factor controlling how much grass is generated per unit area. */
            f32 m_grassDensity = 0.0f;

            /** Number of trees shown in editor preview modes. */
            u32 m_previewTreeCount = 10;

            /** Target number of trees to generate for runtime/export. */
            u32 m_generatedTreeCount = 1000;

            /** Name of the prefab used when spawning trees. */
            String m_treePrefabName = "tree.prefab";

            /** Array of layer weight / layout textures used by the renderer. */
            Array<SmartPtr<render::ITexture>> m_layers;

            /** Array of diffuse/detail textures used by terrain layers. */
            Array<SmartPtr<render::ITexture>> m_textures;

            /** Metalness values for each detail layer (parallel to m_layers). */
            Array<f32> m_metalnessValues;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // TerrainSystem_h__
