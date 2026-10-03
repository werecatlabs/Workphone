#ifndef __WP_ApplicationUtil_h__
#define __WP_ApplicationUtil_h__

#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/UI/IUIMenuItem.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Deque.hpp>
#include <Workphone/Core/List.hpp>
#include <Workphone/Core/Set.hpp>
#include <Workphone/System/FactoryTemplate.hpp>

namespace workphone
{

    /**
     * @class ApplicationUtil
     * @brief A utility class providing common application functionality and helper methods.
     *
     * This class contains static methods for various application operations including:
     * - Scene management and creation (cameras, lights, terrain, objects)
     * - UI element manipulation (menus, tree nodes, overlays)
     * - Resource loading and handling (meshes, materials, textures)
     * - Object type management and factory creation
     * - File format validation for supported asset types
     * - Linked list and collection utilities for shared objects
     *
     * @note All methods are static; this class should not be instantiated.
     */
    class WPCore_API ApplicationUtil
    {
    public:
        /**
         * @brief File extension for binary mesh files.
         * @details Used when loading or saving mesh data in the engine's native binary format.
         */
        static const String meshbinExt;

        /**
         * @brief File extension for material definition files.
         * @details Used when loading or saving material properties and shader configurations.
         */
        static const String materialExt;

        /**
         * @brief Property key for lighting data serialization.
         * @details Used as a key when serializing/deserializing scene lighting properties.
         */
        static const String lightingStr;

        /**
         * @brief Property key for actors data serialization.
         * @details Used as a key when serializing/deserializing scene actor collections.
         */
        static const String actorsStr;

        /**
         * @brief File extension for JSON scene files.
         * @details Retained as the default editable scene format for compatibility.
         */
        static const String builtinSceneExt;

        /**
         * @brief File extension for XML-based scene files.
         * @details Used for human-readable scene definitions in XML format.
         */
        static const String builtinXmlSceneExt;

        /**
         * @brief File extension for versioned binary scene files.
         * @details Used for compact, low-overhead deployment scene data.
         */
        static const String builtinBinarySceneExt;

        /**
         * @brief File extension for USD (Universal Scene Description) scene files.
         * @details Used for interoperability with other 3D applications supporting USD.
         */
        static const String builtinUsdSceneExt;

        /**
         * @brief Converts a concurrent array of smart pointers to a linked list and returns a raw
         * pointer.
         * @tparam T The type of objects in the array, must have a 'next' member pointer.
         * @param array The concurrent array of smart pointers to convert.
         * @return Raw pointer to the first element of the linked list, or nullptr if the array is empty.
         * @note The 'next' pointer of the last element is set to nullptr.
         * @warning The caller must ensure the array elements remain valid while using the returned
         * pointer.
         */
        template <class T>
        static T *getLinkedListPtr( const ConcurrentArray<SmartPtr<T>> &array );

        /**
         * @brief Converts a concurrent array of smart pointers to a linked list and returns a smart
         * pointer.
         * @tparam T The type of objects in the array, must have a 'next' member pointer.
         * @param array The concurrent array of smart pointers to convert.
         * @return Smart pointer to the first element of the linked list, or empty if the array is empty.
         * @note The 'next' pointer of the last element is set to nullptr.
         */
        template <class T>
        static SmartPtr<T> getLinkedList( const ConcurrentArray<SmartPtr<T>> &array );

        /**
         * @brief Converts a linked list of shared objects to an array.
         * @param object Pointer to the first element in the linked list.
         * @return Array containing all objects from the linked list.
         */
        static Array<ISharedObject *> getArray( ISharedObject *object );

        /**
         * @brief Converts a linked list of shared objects to a doubly-linked list.
         * @param object Pointer to the first element in the linked list.
         * @return List containing all objects from the linked list.
         */
        static List<ISharedObject *> getList( ISharedObject *object );

        static Set<ISharedObject *> getSet( ISharedObject *object );

        /**
         * @brief Converts a linked list of shared objects to a double-ended queue.
         * @param object Pointer to the first element in the linked list.
         * @return Deque containing all objects from the linked list.
         */
        static Deque<ISharedObject *> getDeque( ISharedObject *object );

        /**
         * @brief Appends an item to the end of a linked list.
         * @param object Pointer to the first element in the linked list (or nullptr for empty list).
         * @param item Pointer to the item to append.
         */
        static void push_back( ISharedObject *object, ISharedObject *item );

        /**
         * @brief Removes an item from a linked list.
         * @param object Pointer to the first element in the linked list.
         * @param item Pointer to the item to remove.
         * @return true if the item was found and removed, false otherwise.
         */
        static bool erase( ISharedObject *object, ISharedObject *item );

        /**
         * @brief Gets the runtime type name of a shared object.
         * @param object Smart pointer to the shared object.
         * @return String containing the object's type name, or empty string if object is null.
         */
        static String getObjectTypeName( SmartPtr<ISharedObject> object );

        /**
         * @brief Gets the runtime type name of a shared object.
         * @param object Raw pointer to the shared object.
         * @return String containing the object's type name, or empty string if object is null.
         */
        static String getObjectTypeName( ISharedObject *object );

        /**
         * @brief Converts a file path to its corresponding prefab path.
         * @param filePath The input file path to convert.
         * @return String containing the prefab path with appropriate extension.
         */
        static String getPrefabPath( const String &filePath );

        /**
         * @brief Loads a mesh from a file and creates a game actor containing it.
         * @param filePath Path to the mesh file (supports formats returned by
         * getSupportedMeshFormats()).
         * @return SmartPtr to the created actor containing the loaded mesh, or nullptr on failure.
         */
        static SmartPtr<scene::IGameActor> loadMesh( const String &filePath );

        /**
         * @brief Adds a menu item to a menu.
         * @param menu The menu to add the item to.
         * @param itemid Unique identifier for the menu item.
         * @param text The display text for the menu item.
         * @param help Help/tooltip text displayed when hovering over the item.
         * @param type The type of menu item (Normal, Check, Radio, etc.). Defaults to Normal.
         * @return SmartPtr to the created menu item.
         */
        static SmartPtr<ui::IUIMenuItem> addMenuItem(
            SmartPtr<ui::IUIMenu> menu, s32 itemid, const String &text, const String &help,
            ui::IUIMenuItem::Type type = ui::IUIMenuItem::Type::Normal );

        /**
         * @brief Adds a visual separator line to a menu.
         * @param menu The menu to add the separator to.
         * @return SmartPtr to the created separator item.
         */
        static SmartPtr<ui::IUIMenuItem> addMenuSeparator( SmartPtr<ui::IUIMenu> menu );

        /**
         * @brief Sets the display text for a tree node.
         * @param node The tree node to modify.
         * @param text The text to display on the node.
         * @return SmartPtr to the text UI element within the node.
         */
        static SmartPtr<ui::IUIElement> setText( SmartPtr<ui::IUITreeNode> node, const String &text );

        /**
         * @brief Sets an icon or image for a tree node.
         * @param node The tree node to modify.
         * @param imagePath Path to the image file to use as the node's icon.
         * @return SmartPtr to the image UI element within the node.
         */
        static SmartPtr<ui::IUIElement> setImage( SmartPtr<ui::IUITreeNode> node,
                                                  const String &imagePath );

        /**
         * @brief Retrieves the first child element of a UI element.
         * @param element The parent UI element.
         * @return SmartPtr to the first child element, or nullptr if no children exist.
         */
        static SmartPtr<ui::IUIElement> getFirstChild( SmartPtr<ui::IUIElement> element );

        /**
         * @brief Gets the display text from a tree node.
         * @param node The tree node to query.
         * @return String containing the node's text, or empty string if not set.
         */
        static String getText( SmartPtr<ui::IUITreeNode> node );

        /**
         * @brief Maps a factory type string to its corresponding component factory type.
         * @param factoryType The factory type identifier string.
         * @return String containing the component factory type name.
         */
        static String getComponentFactoryType( const String &factoryType );

        /**
         * @brief Converts C# source code files between paths.
         * @param srcPath Source path containing the C# code files.
         * @param dstPath Destination path for the converted code files.
         * @note Used for code generation or migration purposes.
         */
        static void convertCSharp( const String &srcPath, const String &dstPath );

        /**
         * @brief Gets the event handling priority for a component.
         * @param component The component to query.
         * @return Priority value where lower values indicate higher priority.
         */
        static u32 getEventPriority( SmartPtr<scene::IComponent> component );

        /**
         * @brief Creates a default scene with standard elements.
         * @details Typically includes a camera, directional light, and ground plane.
         */
        static void createDefaultScene();

        /**
         * @brief Creates a test overlay panel for UI testing.
         * @return SmartPtr to the created panel actor.
         */
        static SmartPtr<scene::IGameActor> createOverlayPanelTest();

        /**
         * @brief Creates a test overlay text element for UI testing.
         * @return SmartPtr to the created text actor.
         */
        static SmartPtr<scene::IGameActor> createOverlayTextTest();

        /**
         * @brief Creates a test overlay button for UI testing.
         * @return SmartPtr to the created button actor.
         */
        static SmartPtr<scene::IGameActor> createOverlayButtonTest();

        /**
         * @brief Creates a default cubemap for environment reflections.
         * @param addToScene If true, automatically adds the cubemap to the active scene.
         * @return SmartPtr to the created cubemap actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultCubemap( bool addToScene = true );

        /**
         * @brief Creates a default procedural sky.
         * @param addToScene If true, automatically adds the sky to the active scene.
         * @return SmartPtr to the created sky actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultSky( bool addToScene = true );

        /**
         * @brief Creates a camera with default settings.
         * @param addToScene If true, automatically adds the camera to the active scene.
         * @return SmartPtr to the created camera actor.
         */
        static SmartPtr<scene::IGameActor> createCamera( bool addToScene = true );

        /** Creates an editor-ready actor that owns a render-target texture component. */
        static SmartPtr<scene::IGameActor> createRenderTarget( u32 width = 512, u32 height = 512,
                                                               bool addToScene = true );

        /**
         * @brief Creates a default cube primitive with physics.
         * @param addToScene If true, automatically adds the cube to the active scene.
         * @return SmartPtr to the created cube actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultCube( bool addToScene = true );

        /**
         * @brief Creates a default cube mesh without physics components.
         * @param addToScene If true, automatically adds the cube mesh to the active scene.
         * @return SmartPtr to the created cube mesh actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultCubeMesh( bool addToScene = true );

        /**
         * @brief Creates a default ground plane.
         * @param addToScene If true, automatically adds the ground to the active scene.
         * @return SmartPtr to the created ground actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultGround( bool addToScene = true );

        /**
         * @brief Creates a default terrain with heightmap support.
         * @param addToScene If true, automatically adds the terrain to the active scene.
         * @return SmartPtr to the created terrain actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultTerrain( bool addToScene = true );

        /**
         * @brief Creates a default physics constraint between objects.
         * @return SmartPtr to the created constraint actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultConstraint();

        /**
         * @brief Creates a directional light (sun light).
         * @param addToScene If true, automatically adds the light to the active scene.
         * @return SmartPtr to the created directional light actor.
         */
        static SmartPtr<scene::IGameActor> createDirectionalLight( bool addToScene = true );

        /**
         * @brief Creates a point light with omnidirectional illumination.
         * @param addToScene If true, automatically adds the light to the active scene.
         * @return SmartPtr to the created point light actor.
         */
        static SmartPtr<scene::IGameActor> createPointLight( bool addToScene = true );

        /**
         * @brief Creates a default plane primitive.
         * @param addToScene If true, automatically adds the plane to the active scene.
         * @return SmartPtr to the created plane actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultPlane( bool addToScene = true );

        /**
         * @brief Creates a default generic vehicle.
         * @param addToScene If true, automatically adds the vehicle to the active scene.
         * @return SmartPtr to the created vehicle actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultVehicle( bool addToScene = true );

        /**
         * @brief Creates a default car with physics and wheels.
         * @param addToScene If true, automatically adds the car to the active scene.
         * @return SmartPtr to the created car actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultCar( bool addToScene = true );

        /**
         * @brief Creates a default truck with physics and multiple axles.
         * @param addToScene If true, automatically adds the truck to the active scene.
         * @return SmartPtr to the created truck actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultTruck( bool addToScene = true );

        /**
         * @brief Creates a default particle system.
         * @param addToScene If true, automatically adds the particle system to the active scene.
         * @return SmartPtr to the created particle system actor.
         */
        static SmartPtr<scene::IGameActor> createDefaultParticleSystem( bool addToScene = true );

        /**
         * @brief Creates a default material optimized for UI rendering.
         * @return SmartPtr to the created UI material.
         */
        static SmartPtr<render::IMaterial> createDefaultMaterialUI();

        /**
         * @brief Creates a default PBR material.
         * @return SmartPtr to the created material with default properties.
         */
        static SmartPtr<render::IMaterial> createDefaultMaterial();

        /**
         * @brief Creates a procedural test object for development/debugging.
         * @return SmartPtr to the created procedural test actor.
         */
        static SmartPtr<scene::IGameActor> createProceduralTest();

        /**
         * @brief Converts absolute mouse position to relative screen coordinates.
         * @param relativeMousePosition The input mouse position in screen space.
         * @return Vector2 containing normalized mouse position (0.0 to 1.0 range).
         */
        static Vector2<real_Num> getRelativeMousePos( Vector2<real_Num> relativeMousePosition );

        /**
         * @brief Initializes and registers all component and object factories.
         * @details Should be called during application startup to enable object creation.
         */
        static void createFactories();

        /**
         * @brief Creates and registers default engine materials.
         * @details Sets up standard materials used by the engine (error, default, wireframe, etc.).
         */
        static void createDefaultMaterials();

        /**
         * @brief Loads and registers the default system font.
         * @details Initializes the font used for UI text rendering.
         */
        static void createDefaultFont();

        /**
         * @brief Checks if a file is a supported font format.
         * @param filePath Path to the file to check.
         * @return true if the file extension matches a supported font format.
         */
        static bool isSupportedFont( const String &filePath );

        /**
         * @brief Checks if a file is a supported mesh format.
         * @param filePath Path to the file to check.
         * @return true if the file extension matches a supported mesh format.
         * @see getSupportedMeshFormats()
         */
        static bool isSupportedMesh( const String &filePath );

        /**
         * @brief Checks if a file is a supported texture format.
         * @param filePath Path to the file to check.
         * @return true if the file extension matches a supported texture format.
         * @see getSupportedTextureFormats()
         */
        static bool isSupportedTexture( const String &filePath );

        /**
         * @brief Checks if a file is a supported sound/audio format.
         * @param filePath Path to the file to check.
         * @return true if the file extension matches a supported sound format.
         * @see getSupportedAudioFormats()
         */
        static bool isSupportedSound( const String &filePath );

        /**
         * @brief Gets the list of supported mesh file formats.
         * @return Array of supported mesh format extensions (e.g., ".fbx", ".obj", ".gltf").
         */
        static Array<String> getSupportedMeshFormats();

        /**
         * @brief Gets the list of supported texture file formats.
         * @return Array of supported texture format extensions (e.g., ".png", ".jpg", ".dds").
         */
        static Array<String> getSupportedTextureFormats();

        /**
         * @brief Gets the list of supported video file formats.
         * @return Array of supported video format extensions (e.g., ".mp4", ".avi", ".webm").
         */
        static Array<String> getSupportedVideoFormats();

        /**
         * @brief Gets the list of supported font file formats.
         * @return Array of supported font format extensions (e.g., ".ttf", ".otf").
         */
        static Array<String> getSupportedFontFormats();

        /**
         * @brief Gets the list of supported prefab file formats.
         * @return Array of supported prefab format extensions.
         */
        static Array<String> getSupportedPrefabFormats();

        /**
         * @brief Gets the list of supported scene file formats.
         * @return Array of supported scene format extensions.
         */
        static Array<String> getSupportedSceneFormats();

        /**
         * @brief Gets the list of supported script file formats.
         * @return Array of supported script format extensions (e.g., ".lua", ".cs").
         */
        static Array<String> getSupportedScriptFormats();

        /**
         * @brief Gets the list of supported shader file formats.
         * @return Array of supported shader format extensions (e.g., ".hlsl", ".glsl").
         */
        static Array<String> getSupportedShaderFormats();

        /**
         * @brief Gets the list of supported database file formats.
         * @return Array of supported database format extensions (e.g., ".db", ".sqlite").
         */
        static Array<String> getSupportedDatabaseFormats();

        /**
         * @brief Gets the list of supported audio file formats.
         * @return Array of supported audio format extensions (e.g., ".wav", ".ogg", ".mp3").
         */
        static Array<String> getSupportedAudioFormats();

        /**
         * @brief Gets the creation order index of a shared object.
         * @param object Smart pointer to the shared object.
         * @return u32 representing the creation order index.
         */
        static u32 getCreationOrder( SmartPtr<ISharedObject> object );

        /**
         * @brief Checks array for valid textures.
         * @param textures Array of smart pointers to textures.
         * @return true if any texture in the array is valid.
         */
        static bool hasAnyTexture( const Array<SmartPtr<render::ITexture>> &textures );
    };

    template <class T>
    T *ApplicationUtil::getLinkedListPtr( const ConcurrentArray<SmartPtr<T>> &array )
    {
        T *first = nullptr;
        T *cur = nullptr;

        if( !array.empty() )
        {
            const auto &p = array.front();
            first = (T *)p.get();
            cur = first;

            for( size_t i = 1; i < array.size(); ++i )
            {
                auto elem = array[i].get();
                cur->next = elem;
                cur = elem;
            }
        }

        if( cur )
        {
            cur->next = nullptr;
        }

        return first;
    }

    template <class T>
    SmartPtr<T> ApplicationUtil::getLinkedList( const ConcurrentArray<SmartPtr<T>> &array )
    {
        SmartPtr<T> first;
        SmartPtr<T> cur;

        if( !array.empty() )
        {
            const auto &p = array.front();
            first = (T *)p.get();
            cur = first;

            for( size_t i = 1; i < array.size(); ++i )
            {
                auto elem = array[i].get();
                cur->next = elem;
                cur = elem;
            }
        }

        if( cur )
        {
            cur->next = nullptr;
        }

        return first;
    }

}  // namespace workphone

#endif  // ApplicationUtil_h__
