/**
 * @file WorkphoneTypes.hpp
 * @brief Core type definitions, enums, and macros for the WorkPhone engine.
 * @details This file defines fundamental types, enums, and macros used throughout the engine, including:
 *          - Fixed-width integer and floating-point type aliases
 *          - Engine-specific enumerations for rendering, physics, input, and more
 *          - Platform and compiler detection macros
 *          - Assertion and deprecation macros
 * @note This header is included by most engine modules and should remain lightweight.
 * @see WorkphoneConfig.hpp for build configuration options.
 */
#ifndef __WP_CoreEnums_h__
#define __WP_CoreEnums_h__

#include <Workphone/WorkphoneTypes.hpp>

#if WP_COMPILER == WP_COMPILER_MSVC
#    include <cstdint>
#endif

namespace workphone
{

    /**
     * @enum LoadingState
     * @brief Represents the lifecycle state of a resource or object.
     * @details Used by the resource manager to track loading progress and garbage collection.
     */
    enum class LoadingState : u8
    {
        None,           ///< Initial state, no operations performed.
        Allocated,      ///< Memory has been allocated for the resource.
        Unallocated,    ///< Memory has been freed or was never allocated.
        QueuedGC,       ///< Resource is queued for garbage collection.
        Unloading,      ///< Resource is currently being unloaded.
        Unloaded,       ///< Resource has been fully unloaded.
        Loading,        ///< Resource is currently being loaded.
        Loaded,         ///< Resource is fully loaded and ready for use.
        LoadingQueued,  ///< Resource load request is pending in the queue.
        Error,          ///< An error occurred during loading or unloading.
        Count           ///< Total number of loading states (used for iteration).
    };

    enum class LoadResult
    {
        Complete,
        InProgress,
        Failed,
    };

    enum class UnloadResult
    {
        Complete,
        InProgress,
    };

    /**
     * @enum TypeGroups
     * @brief Categories for organizing engine types in the reflection system.
     * @details Used for type introspection, serialization, and editor integration.
     */
    enum class TypeGroups
    {
        None,                 ///< Uncategorized type.
        Core,                 ///< Core engine types.
        FSM,                  ///< Finite state machine types.
        FSMListener,          ///< FSM event listener types.
        System,               ///< System-level types.
        Render,               ///< General rendering types.
        RenderNodes,          ///< Scene graph render nodes.
        RenderObjects,        ///< Renderable object types.
        RenderMaterials,      ///< Material types.
        RenderMaterialNodes,  ///< Material graph node types.
        RenderTextures,       ///< Texture types.
        Application,          ///< Application framework types.
        Data,                 ///< Data container types.
        Game,                 ///< Game logic types.
        Actor,                ///< Actor/entity types.
        Component,            ///< Component types.
        UI,                   ///< General UI types.
        UIText,               ///< Text rendering UI types.
        UITreeNodes,          ///< Tree view node types.
        UIRenderWindows,      ///< Render target window types.
        UIGraphicsWindows,    ///< Window widget types.
        State,                ///< State types.
        StateQueues,          ///< State queue types.
        StateMessages,        ///< State message types.
        StateListeners,       ///< State listener types.
        Factories,            ///< Factory types.
        Directors,            ///< Director/controller types.
        Resources,            ///< Resource types.
        Properties,           ///< Property types.
        IO,                   ///< General I/O types.
        IOArchive,            ///< Archive I/O types.
        IOStream,             ///< Stream I/O types.
        Jobs,                 ///< Job/task system types.
        Count                 ///< Total number of type groups.
    };

    /**
     * @enum KeyCodes
     * @brief Virtual key codes for keyboard and mouse input.
     * @details Values correspond to Windows virtual-key codes for cross-platform compatibility.
     * @see https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
     */
    enum class KeyCodes
    {
        KEY_LBUTTON = 0x01,     ///< Left mouse button.
        KEY_RBUTTON = 0x02,     ///< Right mouse button.
        KEY_CANCEL = 0x03,      ///< Control-break processing.
        KEY_MBUTTON = 0x04,     ///< Middle mouse button (three-button mouse).
        KEY_XBUTTON1 = 0x05,    ///< X1 mouse button (extended mouse).
        KEY_XBUTTON2 = 0x06,    ///< X2 mouse button (extended mouse).
        KEY_BACK = 0x08,        ///< BACKSPACE key.
        KEY_TAB = 0x09,         ///< TAB key.
        KEY_CLEAR = 0x0C,       ///< CLEAR key.
        KEY_RETURN = 0x0D,      ///< ENTER/RETURN key.
        KEY_SHIFT = 0x10,       ///< SHIFT key (either left or right).
        KEY_CONTROL = 0x11,     ///< CTRL key (either left or right).
        KEY_LALT = 0x12,        ///< Left ALT key.
        KEY_PAUSE = 0x13,       ///< PAUSE key.
        KEY_CAPITAL = 0x14,     ///< CAPS LOCK key.
        KEY_KANA = 0x15,        ///< IME Kana mode.
        KEY_HANGUEL = 0x15,     ///< IME Hangul mode (legacy, use KEY_HANGUL).
        KEY_HANGUL = 0x15,      ///< IME Hangul mode.
        KEY_JUNJA = 0x17,       ///< IME Junja mode.
        KEY_FINAL = 0x18,       ///< IME final mode.
        KEY_HANJA = 0x19,       ///< IME Hanja mode.
        KEY_KANJI = 0x19,       ///< IME Kanji mode.
        KEY_ESCAPE = 0x1B,      ///< ESC key.
        KEY_CONVERT = 0x1C,     ///< IME convert.
        KEY_NONCONVERT = 0x1D,  ///< IME non-convert.
        KEY_ACCEPT = 0x1E,      ///< IME accept.
        KEY_MODECHANGE = 0x1F,  ///< IME mode change request.
        KEY_SPACE = 0x20,       ///< SPACEBAR.
        KEY_PRIOR = 0x21,       ///< PAGE UP key.
        KEY_NEXT = 0x22,        ///< PAGE DOWN key.
        KEY_END = 0x23,         ///< END key.
        KEY_HOME = 0x24,        ///< HOME key.
        KEY_LEFT = 0x25,        ///< LEFT ARROW key.
        KEY_UP = 0x26,          ///< UP ARROW key.
        KEY_RIGHT = 0x27,       ///< RIGHT ARROW key.
        KEY_DOWN = 0x28,        ///< DOWN ARROW key.
        KEY_SELECT = 0x29,      ///< SELECT key.
        KEY_PRINT = 0x2A,       ///< PRINT key.
        KEY_EXECUT = 0x2B,      ///< EXECUTE key.
        KEY_SNAPSHOT = 0x2C,    ///< PRINT SCREEN key.
        KEY_INSERT = 0x2D,      ///< INSERT key.
        KEY_DELETE = 0x2E,      ///< DELETE key.
        KEY_HELP = 0x2F,        ///< HELP key.
        KEY_KEY_0 = 0x30,       ///< 0 key.
        KEY_KEY_1 = 0x31,       ///< 1 key.
        KEY_KEY_2 = 0x32,       ///< 2 key.
        KEY_KEY_3 = 0x33,       ///< 3 key.
        KEY_KEY_4 = 0x34,       ///< 4 key.
        KEY_KEY_5 = 0x35,       ///< 5 key.
        KEY_KEY_6 = 0x36,       ///< 6 key.
        KEY_KEY_7 = 0x37,       ///< 7 key.
        KEY_KEY_8 = 0x38,       ///< 8 key.
        KEY_KEY_9 = 0x39,       ///< 9 key.
        KEY_KEY_A = 0x41,       ///< A key.
        KEY_KEY_B = 0x42,       ///< B key.
        KEY_KEY_C = 0x43,       ///< C key.
        KEY_KEY_D = 0x44,       ///< D key.
        KEY_KEY_E = 0x45,       ///< E key.
        KEY_KEY_F = 0x46,       ///< F key.
        KEY_KEY_G = 0x47,       ///< G key.
        KEY_KEY_H = 0x48,       ///< H key.
        KEY_KEY_I = 0x49,       ///< I key.
        KEY_KEY_J = 0x4A,       ///< J key.
        KEY_KEY_K = 0x4B,       ///< K key.
        KEY_KEY_L = 0x4C,       ///< L key.
        KEY_KEY_M = 0x4D,       ///< M key.
        KEY_KEY_N = 0x4E,       ///< N key.
        KEY_KEY_O = 0x4F,       ///< O key.
        KEY_KEY_P = 0x50,       ///< P key.
        KEY_KEY_Q = 0x51,       ///< Q key.
        KEY_KEY_R = 0x52,       ///< R key.
        KEY_KEY_S = 0x53,       ///< S key.
        KEY_KEY_T = 0x54,       ///< T key.
        KEY_KEY_U = 0x55,       ///< U key.
        KEY_KEY_V = 0x56,       ///< V key.
        KEY_KEY_W = 0x57,       ///< W key.
        KEY_KEY_X = 0x58,       ///< X key.
        KEY_KEY_Y = 0x59,       ///< Y key.
        KEY_KEY_Z = 0x5A,       ///< Z key.
        KEY_LWIN = 0x5B,        ///< Left Windows key.
        KEY_RWIN = 0x5C,        ///< Right Windows key.
        KEY_APPS = 0x5D,        ///< Applications/Context menu key.
        KEY_SLEEP = 0x5F,       ///< Computer Sleep key.
        KEY_NUMPAD0 = 0x60,     ///< Numeric keypad 0 key.
        KEY_NUMPAD1 = 0x61,     ///< Numeric keypad 1 key.
        KEY_NUMPAD2 = 0x62,     ///< Numeric keypad 2 key.
        KEY_NUMPAD3 = 0x63,     ///< Numeric keypad 3 key.
        KEY_NUMPAD4 = 0x64,     ///< Numeric keypad 4 key.
        KEY_NUMPAD5 = 0x65,     ///< Numeric keypad 5 key.
        KEY_NUMPAD6 = 0x66,     ///< Numeric keypad 6 key.
        KEY_NUMPAD7 = 0x67,     ///< Numeric keypad 7 key.
        KEY_NUMPAD8 = 0x68,     ///< Numeric keypad 8 key.
        KEY_NUMPAD9 = 0x69,     ///< Numeric keypad 9 key.
        KEY_MULTIPLY = 0x6A,    ///< Numeric keypad Multiply (*) key.
        KEY_ADD = 0x6B,         ///< Numeric keypad Add (+) key.
        KEY_SEPARATOR = 0x6C,   ///< Numeric keypad Separator key.
        KEY_SUBTRACT = 0x6D,    ///< Numeric keypad Subtract (-) key.
        KEY_DECIMAL = 0x6E,     ///< Numeric keypad Decimal (.) key.
        KEY_DIVIDE = 0x6F,      ///< Numeric keypad Divide (/) key.
        KEY_F1 = 0x70,          ///< F1 function key.
        KEY_F2 = 0x71,          ///< F2 function key.
        KEY_F3 = 0x72,          ///< F3 function key.
        KEY_F4 = 0x73,          ///< F4 function key.
        KEY_F5 = 0x74,          ///< F5 function key.
        KEY_F6 = 0x75,          ///< F6 function key.
        KEY_F7 = 0x76,          ///< F7 function key.
        KEY_F8 = 0x77,          ///< F8 function key.
        KEY_F9 = 0x78,          ///< F9 function key.
        KEY_F10 = 0x79,         ///< F10 function key.
        KEY_F11 = 0x7A,         ///< F11 function key.
        KEY_F12 = 0x7B,         ///< F12 function key.
        KEY_F13 = 0x7C,         ///< F13 function key.
        KEY_F14 = 0x7D,         ///< F14 function key.
        KEY_F15 = 0x7E,         ///< F15 function key.
        KEY_F16 = 0x7F,         ///< F16 function key.
        KEY_F17 = 0x80,         ///< F17 function key.
        KEY_F18 = 0x81,         ///< F18 function key.
        KEY_F19 = 0x82,         ///< F19 function key.
        KEY_F20 = 0x83,         ///< F20 function key.
        KEY_F21 = 0x84,         ///< F21 function key.
        KEY_F22 = 0x85,         ///< F22 function key.
        KEY_F23 = 0x86,         ///< F23 function key.
        KEY_F24 = 0x87,         ///< F24 function key.
        KEY_NUMLOCK = 0x90,     ///< NUM LOCK key.
        KEY_SCROLL = 0x91,      ///< SCROLL LOCK key.
        KEY_LSHIFT = 0xA0,      ///< Left SHIFT key.
        KEY_RSHIFT = 0xA1,      ///< Right SHIFT key.
        KEY_LCONTROL = 0xA2,    ///< Left CONTROL key.
        KEY_RCONTROL = 0xA3,    ///< Right CONTROL key.
        KEY_RALT = 0xA4,        ///< Right ALT key.
        KEY_RMENU = 0xA5,       ///< Right MENU key.
        KEY_PLUS = 0xBB,        ///< Plus (+) key on main keyboard.
        KEY_COMMA = 0xBC,       ///< Comma (,) key.
        KEY_MINUS = 0xBD,       ///< Minus (-) key on main keyboard.
        KEY_PERIOD = 0xBE,      ///< Period (.) key.
        KEY_ATTN = 0xF6,        ///< Attn key.
        KEY_CRSEL = 0xF7,       ///< CrSel key.
        KEY_EXSEL = 0xF8,       ///< ExSel key.
        KEY_EREOF = 0xF9,       ///< Erase EOF key.
        KEY_PLAY = 0xFA,        ///< Play key.
        KEY_ZOOM = 0xFB,        ///< Zoom key.
        KEY_PA1 = 0xFD,         ///< PA1 key.
        KEY_OEM_CLEAR = 0xFE,   ///< Clear key.
        KEY_COUNT = 0xFF        ///< Total number of key codes (sentinel value, not a valid key).
    };

    /**
     * @enum TaskId
     * @brief Identifies different task types for the multi-threaded job system.
     * @details Each task type can be assigned to specific threads for optimal resource utilization.
     */
    enum class TaskId : u8
    {
        Primary = 0,     ///< Primary/main thread task.
        Ai,              ///< Artificial intelligence processing.
        Animation,       ///< Animation updates and blending.
        Application,     ///< Application-level logic.
        Collision,       ///< Collision detection.
        Controls,        ///< Input/control processing.
        Dynamics,        ///< Dynamic physics simulation.
        GarbageCollect,  ///< Garbage collection and memory cleanup.
        Fluid,           ///< Fluid simulation.
        Input,           ///< Input handling.
        Physics,         ///< General physics processing.
        None,            ///< No specific task assignment.
        Render,          ///< Rendering operations.
        Sound,           ///< Audio processing.
        SoftBody,        ///< Soft body physics simulation.
        Count            ///< Total number of task types.
    };

    /**
     * @enum GradientMode
     * @brief Gradient interpolation modes.
     */
    enum class GradientMode : u8
    {
        Blend,  ///< Smooth blend.
        Fixed   ///< Fixed steps.
    };

    /**
     * @enum GarbageCollectorMode
     * @brief Modes for the engine's garbage collector.
     */
    enum class GarbageCollectorMode : u8
    {
        LowConsumption,  ///< Minimize memory usage.
        Balanced,        ///< Balance between performance and memory.
        Deferred,        ///< Defer collection for performance.
        Count            ///< Number of modes.
    };

    /**
     * @enum PolygonPointOrdering
     * @brief Defines the ordering of polygon points.
     */
    enum class PolygonPointOrdering : u8
    {
        PPO_CLOCKWISE,         ///< Clockwise ordering.
        PPO_COUNTER_CLOCKWISE  ///< Counter-clockwise ordering.
    };

    /**
     * @enum HorizontalAlignment
     * @brief Horizontal alignment options.
     */
    enum class HorizontalAlignment : u8
    {
        LEFT,
        RIGHT,
        CENTER,
        CUSTOM,
        COUNT
    };

    /**
     * @enum VerticalAlignment
     * @brief Vertical alignment options.
     */
    enum class VerticalAlignment : u8
    {
        TOP,
        BOTTOM,
        CENTER,
        CUSTOM,
        COUNT
    };

    /**
     * @enum Direction
     * @brief Orientation options.
     */
    enum class Direction : u8
    {
        Horizontal,
        Vertical,
        Count
    };

    /**
     * @enum TargetPlatform
     * @brief Supported target platforms.
     */
    enum class TargetPlatform
    {
        Windows,
        Linux,
        MacOS,
        Android,
        iOS,
        WindowsPhone,
        WindowsRT,
        XboxOne,
        PS4,
        Switch,
        HTML5,
        Custom,
        Count
    };

    /**
     * @enum RenderOperationType
     * @brief Types of render operations for graphics primitives.
     * @details Includes point, line, triangle, and patch operations, as well as adjacency types for
     * geometry shaders.
     */
    enum class RenderOperationType
    {
        /// A list of points, 1 vertex per point
        OT_POINT_LIST = 1,
        /// A list of lines, 2 vertices per line
        OT_LINE_LIST = 2,
        /// A strip of connected lines, 1 vertex per line plus 1 start vertex
        OT_LINE_STRIP = 3,
        /// A list of triangles, 3 vertices per triangle
        OT_TRIANGLE_LIST = 4,
        /// A strip of triangles, 3 vertices for the first triangle, and 1 per triangle after that
        OT_TRIANGLE_STRIP = 5,
        /// A fan of triangles, 3 vertices for the first triangle, and 1 per triangle after that
        OT_TRIANGLE_FAN = 6,
        /// Patch control point operations, used with tessellation stages
        OT_PATCH_1_CONTROL_POINT = 7,
        OT_PATCH_2_CONTROL_POINT = 8,
        OT_PATCH_3_CONTROL_POINT = 9,
        OT_PATCH_4_CONTROL_POINT = 10,
        OT_PATCH_5_CONTROL_POINT = 11,
        OT_PATCH_6_CONTROL_POINT = 12,
        OT_PATCH_7_CONTROL_POINT = 13,
        OT_PATCH_8_CONTROL_POINT = 14,
        OT_PATCH_9_CONTROL_POINT = 15,
        OT_PATCH_10_CONTROL_POINT = 16,
        OT_PATCH_11_CONTROL_POINT = 17,
        OT_PATCH_12_CONTROL_POINT = 18,
        OT_PATCH_13_CONTROL_POINT = 19,
        OT_PATCH_14_CONTROL_POINT = 20,
        OT_PATCH_15_CONTROL_POINT = 21,
        OT_PATCH_16_CONTROL_POINT = 22,
        OT_PATCH_17_CONTROL_POINT = 23,
        OT_PATCH_18_CONTROL_POINT = 24,
        OT_PATCH_19_CONTROL_POINT = 25,
        OT_PATCH_20_CONTROL_POINT = 26,
        OT_PATCH_21_CONTROL_POINT = 27,
        OT_PATCH_22_CONTROL_POINT = 28,
        OT_PATCH_23_CONTROL_POINT = 29,
        OT_PATCH_24_CONTROL_POINT = 30,
        OT_PATCH_25_CONTROL_POINT = 31,
        OT_PATCH_26_CONTROL_POINT = 32,
        OT_PATCH_27_CONTROL_POINT = 33,
        OT_PATCH_28_CONTROL_POINT = 34,
        OT_PATCH_29_CONTROL_POINT = 35,
        OT_PATCH_30_CONTROL_POINT = 36,
        OT_PATCH_31_CONTROL_POINT = 37,
        OT_PATCH_32_CONTROL_POINT = 38,
        // max valid base OT_ = (1 << 6) - 1
        /// Mark that the index buffer contains adjacency information
        OT_DETAIL_ADJACENCY_BIT = 1 << 6,
        /// like OT_POINT_LIST but with adjacency information for the geometry shader
        OT_LINE_LIST_ADJ = OT_LINE_LIST | OT_DETAIL_ADJACENCY_BIT,
        /// like OT_LINE_STRIP but with adjacency information for the geometry shader
        OT_LINE_STRIP_ADJ = OT_LINE_STRIP | OT_DETAIL_ADJACENCY_BIT,
        /// like OT_TRIANGLE_LIST but with adjacency information for the geometry shader
        OT_TRIANGLE_LIST_ADJ = OT_TRIANGLE_LIST | OT_DETAIL_ADJACENCY_BIT,
        /// like OT_TRIANGLE_STRIP but with adjacency information for the geometry shader
        OT_TRIANGLE_STRIP_ADJ = OT_TRIANGLE_STRIP | OT_DETAIL_ADJACENCY_BIT
    };

    /**
     * @enum RenderQueueGroupID
     * @brief IDs for different render queue groups.
     */
    enum class RenderQueueGroupID : u8
    {
        /// Use this queue for objects which must be rendered first e.g. backgrounds
        RENDER_QUEUE_BACKGROUND = 0,
        /// First queue (after backgrounds), used for skyboxes if rendered first
        RENDER_QUEUE_SKIES_EARLY = 5,
        RENDER_QUEUE_1 = 10,
        RENDER_QUEUE_2 = 20,
        RENDER_QUEUE_WORLD_GEOMETRY_1 = 25,
        RENDER_QUEUE_3 = 30,
        RENDER_QUEUE_4 = 40,
        /// The default render queue
        RENDER_QUEUE_MAIN = 50,
        RENDER_QUEUE_6 = 60,
        RENDER_QUEUE_7 = 70,
        RENDER_QUEUE_WORLD_GEOMETRY_2 = 75,
        RENDER_QUEUE_8 = 80,
        RENDER_QUEUE_9 = 90,
        /// Penultimate queue(before overlays), used for skyboxes if rendered last
        RENDER_QUEUE_SKIES_LATE = 95,
        /// Use this queue for objects which must be rendered last e.g. overlays
        RENDER_QUEUE_OVERLAY = 100,
        /// Final possible render queue, don't exceed this
        RENDER_QUEUE_MAX = 105
    };

    /**
     * @enum FrameBuffer
     * @brief Frame buffer selection for rendering operations.
     */
    enum class FrameBuffer : u8
    {
        Front,
        Back,
        Auto,
        Count
    };

    /**
     * @enum FSMEvent
     * @brief Event IDs for finite state machine (FSM) transitions.
     */
    enum class FSMEvent : u8
    {
        Change,         ///< State change event.
        Enter,          ///< State enter event.
        Leave,          ///< State leave event.
        Pending,        ///< State change pending event.
        Complete,       ///< State change complete event.
        NewState,       ///< New state event.
        WaitForChange,  ///< Wait for state change event.

        Count  ///< Total number of events.
    };

    /**
     * @enum FSMReturnType
     * @brief Return types for FSM operations.
     */
    enum class FSMReturnType : s8
    {
        Failed = -1,    ///< Failed return.
        Ok,             ///< OK return.
        Accept,         ///< Accept return.
        Cancel,         ///< Cancel return.
        Ignore,         ///< Ignore return.
        WaitForChange,  ///< Wait for state change return.
        NotLoaded,      ///< Not loaded return.
        NotHandled,     ///< Not handled return.

        Count  ///< Total number of return types.
    };

    /**
     * @enum TerrainTextureTypes
     * @brief Types of textures used by terrain materials.
     */
    enum class TerrainTextureTypes : u8
    {
        Base,
        ///< Base texture.
        Splat,
        ///< Splat texture.
        Diffuse1,
        ///< First diffuse texture.
        Diffuse2,
        ///< Second diffuse texture.
        Diffuse3,
        ///< Third diffuse texture.
        Diffuse4,
        ///< Fourth diffuse texture.
        Normal1,
        ///< First normal texture.
        Normal2,
        ///< Second normal texture.
        Normal3,
        ///< Third normal texture.
        Normal4,
        ///< Fourth normal texture.
        Count  ///< Count of texture types.
    };

    /**
     * @enum PbsTextureTypes
     * @brief Types of textures used by PBS (Physically Based Shading) materials.
     */
    enum class PbsTextureTypes : u8
    {
        PBSM_DIFFUSE,
        ///< Diffuse texture.
        PBSM_NORMAL,
        ///< Normal texture.
        PBSM_SPECULAR,
        ///< Specular texture.
        PBSM_METALLIC = PBSM_SPECULAR,
        ///< Metallic texture.
        PBSM_ROUGHNESS,
        ///< Roughness texture.
        PBSM_DETAIL_WEIGHT,
        ///< Detail weight texture.
        PBSM_DETAIL0,
        ///< First detail texture.
        PBSM_DETAIL1,
        ///< Second detail texture.
        PBSM_DETAIL2,
        ///< Third detail texture.
        PBSM_DETAIL3,
        ///< Fourth detail texture.
        PBSM_DETAIL0_NM,
        ///< First detail normal map texture.
        PBSM_DETAIL1_NM,
        ///< Second detail normal map texture.
        PBSM_DETAIL2_NM,
        ///< Third detail normal map texture.
        PBSM_DETAIL3_NM,
        ///< Fourth detail normal map texture.
        PBSM_EMISSIVE,
        ///< Emissive texture.
        PBSM_REFLECTION,
        ///< Reflection texture.
        NUM_PBSM_SOURCES = PBSM_REFLECTION,
        ///< Number of PBSM sources.
        NUM_PBSM_TEXTURE_TYPES  ///< Number of PBSM texture types.
    };

    /**
     * @enum PbsBlendModes
     * @brief Blend modes for PBS materials.
     */
    enum class PbsBlendModes : u8
    {
        PBSM_BLEND_NORMAL_NON_PREMUL,
        ///< Normal blend mode.
        PBSM_BLEND_NORMAL_PREMUL,
        ///< Normal premultiplied blend mode.
        PBSM_BLEND_ADD,
        ///< Add blend mode.
        PBSM_BLEND_SUBTRACT,
        ///< Subtract blend mode.
        PBSM_BLEND_MULTIPLY,
        ///< Multiply blend mode.
        PBSM_BLEND_MULTIPLY2X,
        ///< Multiply 2x blend mode.
        PBSM_BLEND_SCREEN,
        ///< Screen blend mode.
        PBSM_BLEND_OVERLAY,
        ///< Overlay blend mode.
        PBSM_BLEND_LIGHTEN,
        PBSM_BLEND_DARKEN,
        PBSM_BLEND_GRAIN_EXTRACT,
        PBSM_BLEND_GRAIN_MERGE,
        PBSM_BLEND_DIFFERENCE,
        NUM_PBSM_BLEND_MODES
    };

    /**
     * @enum SkyboxType
     * @brief Types of skyboxes supported by the engine.
     */
    enum class SkyboxType
    {
        /// Cube map skybox
        CubeMap,
        /// Spherical skybox
        Spherical,
        /// Plane-based skybox
        PlaneBased,

        Count
    };

    /**
     * @enum SkyboxTextureTypes
     * @brief Types of textures used by skybox materials.
     */
    enum class SkyboxTextureTypes : u8
    {
        Front,
        Back,
        Left,
        Right,
        Up,
        Down,

        Count
    };

    /**
     * @enum SkyboxCubeTextureTypes
     * @brief Types of textures used by cube skybox materials.
     */
    enum class SkyboxCubeTextureTypes : u8
    {
        Cube,

        Count
    };

    /**
     * @enum PbrWorkflows
     * @brief Types of PBR (Physically Based Rendering) workflows.
     */
    enum class PbrWorkflows : u8
    {
        SpecularWorkflow,
        SpecularAsFresnelWorkflow,
        MetallicWorkflow
    };

    /**
     * @enum MaterialType
     * @brief Types of materials supported by the engine.
     */
    enum class MaterialType : u8
    {
        Standard,
        StandardSpecular,
        StandardTriPlanar,
        TerrainStandard,
        TerrainSpecular,
        TerrainDiffuse,
        Skybox,
        SkyboxCubemap,
        UI,
        Custom,

        Count
    };

    /**
     * @enum TangentMode
     * @brief Defines interpolation behavior for animation curve tangents.
     * @details Controls how keyframe values are interpolated in animation curves.
     */
    enum class TangentMode : u8
    {
        Free,         ///< Tangent can be freely adjusted by the user.
        Auto,         ///< Tangent is automatically calculated for smooth interpolation.
        Linear,       ///< Linear interpolation between keyframes.
        Constant,     ///< Step/constant interpolation (no blending).
        ClampedAuto,  ///< Auto tangent clamped to prevent overshooting.
        Count         ///< Total number of tangent modes.
    };

    /**
     * @enum DataFormat
     * @brief Supported data serialization formats.
     * @details Used for loading and saving scene data, assets, and configuration.
     */
    enum class DataFormat : u8
    {
        JSON,  ///< JavaScript Object Notation format.
        XML,   ///< Extensible Markup Language format.
        YAML,  ///< YAML Ain't Markup Language format.
        USD,   ///< Universal Scene Description format.
        COUNT  ///< Total number of supported formats.
    };

    /**
     * @enum PlaneIntersectionRelation
     * @brief Describes the spatial relationship between a 3D object and a plane.
     */
    enum class PlaneIntersectionRelation : u8
    {
        ISREL3D_FRONT,     ///< Object is entirely in front of the plane.
        ISREL3D_BACK,      ///< Object is entirely behind the plane.
        ISREL3D_PLANAR,    ///< Object lies on the plane.
        ISREL3D_SPANNING,  ///< Object spans both sides of the plane.
        ISREL3D_CLIPPED    ///< Object has been clipped by the plane.
    };

    /**
     * @enum PlaneSide
     * @brief Indicates which side of a plane a point or object resides on.
     * @details The positive side is the half-space in the direction of the plane normal.
     */
    enum class PlaneSide : u8
    {
        NO_SIDE,        ///< Point lies exactly on the plane.
        POSITIVE_SIDE,  ///< Point is on the positive (normal direction) side.
        NEGATIVE_SIDE,  ///< Point is on the negative (opposite to normal) side.
        BOTH_SIDE       ///< Object spans both sides of the plane.
    };

    /**
     * @enum FrustumPlane
     * @brief Identifies the six planes of a view frustum.
     * @details Used for frustum culling operations.
     */
    enum class FrustumPlane : u8
    {
        FRUSTUM_PLANE_NEAR = 0,   ///< Near clipping plane.
        FRUSTUM_PLANE_FAR = 1,    ///< Far clipping plane.
        FRUSTUM_PLANE_LEFT = 2,   ///< Left clipping plane.
        FRUSTUM_PLANE_RIGHT = 3,  ///< Right clipping plane.
        FRUSTUM_PLANE_TOP = 4,    ///< Top clipping plane.
        FRUSTUM_PLANE_BOTTOM = 5  ///< Bottom clipping plane.
    };

    /**
     * @enum VertexElementType
     * @brief Specifies the data type of a vertex buffer element.
     * @details Defines the underlying data format for vertex attributes.
     * @note Some types (marked deprecated) are not universally supported on all hardware.
     */
    enum class VertexElementType : u8
    {
        VET_FLOAT1 = 0,                     ///< Single 32-bit float.
        VET_FLOAT2 = 1,                     ///< Two 32-bit floats (vec2).
        VET_FLOAT3 = 2,                     ///< Three 32-bit floats (vec3).
        VET_FLOAT4 = 3,                     ///< Four 32-bit floats (vec4).
        VET_SHORT1 = 5,                     ///< Single 16-bit signed integer. @deprecated
        VET_SHORT2 = 6,                     ///< Two 16-bit signed integers.
        VET_SHORT3 = 7,                     ///< Three 16-bit signed integers. @deprecated
        VET_SHORT4 = 8,                     ///< Four 16-bit signed integers.
        VET_UBYTE4 = 9,                     ///< Four 8-bit unsigned integers.
        _DETAIL_SWAP_RB = 10,               ///< Internal use: swap red and blue channels.
        VET_DOUBLE1 = 12,                   ///< Single 64-bit double (limited hardware support).
        VET_DOUBLE2 = 13,                   ///< Two 64-bit doubles (limited hardware support).
        VET_DOUBLE3 = 14,                   ///< Three 64-bit doubles (limited hardware support).
        VET_DOUBLE4 = 15,                   ///< Four 64-bit doubles (limited hardware support).
        VET_USHORT1 = 16,                   ///< Single 16-bit unsigned integer. @deprecated
        VET_USHORT2 = 17,                   ///< Two 16-bit unsigned integers.
        VET_USHORT3 = 18,                   ///< Three 16-bit unsigned integers. @deprecated
        VET_USHORT4 = 19,                   ///< Four 16-bit unsigned integers.
        VET_INT1 = 20,                      ///< Single 32-bit signed integer.
        VET_INT2 = 21,                      ///< Two 32-bit signed integers.
        VET_INT3 = 22,                      ///< Three 32-bit signed integers.
        VET_INT4 = 23,                      ///< Four 32-bit signed integers.
        VET_UINT1 = 24,                     ///< Single 32-bit unsigned integer.
        VET_UINT2 = 25,                     ///< Two 32-bit unsigned integers.
        VET_UINT3 = 26,                     ///< Three 32-bit unsigned integers.
        VET_UINT4 = 27,                     ///< Four 32-bit unsigned integers.
        VET_BYTE4 = 28,                     ///< Four 8-bit signed bytes.
        VET_BYTE4_NORM = 29,                ///< Four 8-bit signed bytes (normalized to -1.0 to 1.0).
        VET_UBYTE4_NORM = 30,               ///< Four 8-bit unsigned bytes (normalized to 0.0 to 1.0).
        VET_SHORT2_NORM = 31,               ///< Two 16-bit signed shorts (normalized to -1.0 to 1.0).
        VET_SHORT4_NORM = 32,               ///< Four 16-bit signed shorts (normalized to -1.0 to 1.0).
        VET_USHORT2_NORM = 33,              ///< Two 16-bit unsigned shorts (normalized to 0.0 to 1.0).
        VET_USHORT4_NORM = 34,              ///< Four 16-bit unsigned shorts (normalized to 0.0 to 1.0).
        VET_COLOUR = VET_UBYTE4_NORM,       ///< Color type. @deprecated Use VET_UBYTE4_NORM.
        VET_COLOUR_ARGB = VET_UBYTE4_NORM,  ///< ARGB color. @deprecated Use VET_UBYTE4_NORM.
        VET_COLOUR_ABGR = VET_UBYTE4_NORM   ///< ABGR color. @deprecated Use VET_UBYTE4_NORM.
    };

    /**
     * @enum VertexElementSemantic
     * @brief Defines the semantic meaning of vertex buffer elements.
     * @details Used to bind vertex data to shader inputs.
     */
    enum class VertexElementSemantic : u8
    {
        VES_POSITION = 1,             ///< Vertex position (typically 3 floats).
        VES_BLEND_WEIGHTS = 2,        ///< Skeletal animation blend weights.
        VES_BLEND_INDICES = 3,        ///< Skeletal animation bone indices.
        VES_NORMAL = 4,               ///< Vertex normal (3 floats).
        VES_DIFFUSE = 5,              ///< Diffuse/albedo vertex color.
        VES_SPECULAR = 6,             ///< Specular vertex color.
        VES_TEXTURE_COORDINATES = 7,  ///< Texture coordinates (UV).
        VES_BINORMAL = 8,             ///< Binormal vector (Y axis if normal is Z).
        VES_TANGENT = 9,              ///< Tangent vector (X axis if normal is Z).
        VES_OTHER = 10                ///< Custom/other vertex semantics.
    };

    /** The types of animation interpolation available. */
    enum class InterpolationMode : u8
    {
        /** Values are interpolated along straight lines. */
        LINEAR,
        /** Values are interpolated along a spline, resulting in smoother changes in direction. */
        SPLINE
    };

    /** The types of rotational interpolation available. */
    enum class RotationInterpolationMode : u8
    {
        /** Values are interpolated linearly. This is faster but does not
            necessarily give a completely accurate result.
        */
        LINEAR,
        /** Values are interpolated spherically. This is more accurate but
            has a higher cost.
        */
        SPHERICAL
    };

    /** High-level filtering options providing shortcuts to settings the
    minification, magnification and mip filters. */
    enum class TextureFilterOptions : u8
    {
        /// Equal to: min=FO_POINT, mag=FO_POINT, mip=FO_NONE
        NONE,
        /// Equal to: min=FO_LINEAR, mag=FO_LINEAR, mip=FO_POINT
        BILINEAR,
        /// Equal to: min=FO_LINEAR, mag=FO_LINEAR, mip=FO_LINEAR
        TRILINEAR,
        /// Equal to: min=FO_ANISOTROPIC, max=FO_ANISOTROPIC, mip=FO_LINEAR
        ANISOTROPIC
    };

    enum class FilterType : u8
    {
        /// The filter used when shrinking a texture
        MIN,
        /// The filter used when magnifying a texture
        MAG,
        /// The filter used when determining the mipmap
        MIP
    };

    /** Filtering options for textures / mipmaps. */
    enum class FilterOptions : u8
    {
        /// No filtering, used for FT_MIP to turn off mipmapping
        NONE,
        /// Use the closest pixel
        POINT,
        /// Average of a 2x2 pixel area, denotes bilinear for MIN and MAG, trilinear for MIP
        LINEAR,
        /// Similar to FO_LINEAR, but compensates for the angle of the texture plane
        ANISOTROPIC
    };

    /** Comparison functions used for the depth/stencil buffer operations and
     * others.
     */
    enum class CompareFunction : u8
    {
        ALWAYS_FAIL,
        ALWAYS_PASS,
        LESS,
        LESS_EQUAL,
        EQUAL,
        NOT_EQUAL,
        GREATER_EQUAL,
        GREATER,
        Count,
    };

    /** Texture addressing modes - default is TAM_WRAP.
     */
    enum class TextureAddressingMode : u8
    {
        /// %Any value beyond 1.0 wraps back to 0.0. %Texture is repeated.
        Wrap,
        /// %Texture flips every boundary, meaning texture is mirrored every 1.0 u or v
        Mirror,
        /// Values beyond 1.0 are clamped to 1.0. %Texture ’streaks’ beyond 1.0 since last line
        /// of pixels is used across the rest of the address space. Useful for textures which
        /// need exact coverage from 0.0 to 1.0 without the ’fuzzy edge’ wrap gives when
        /// combined with filtering.
        Clamp,
        /// %Texture coordinates outside the range [0.0, 1.0] are set to the border colour.
        Border,
        /// Unknown
        Unknown = 99
    };

    /**
     * @brief Enumeration of available tire models for simulation.
     */
    enum class TireModel : u8
    {
        Simple,   //!< Basic tire model with simplified physics
        Pacejka,  //!< Advanced Pacejka tire model for realistic behavior
        Brush     //!< Brush tire model for balanced performance and accuracy
    };

    enum class VehicleDriveType : u8
    {
        FrontWheelDrive,  //!< Front-wheel drive configuration
        RearWheelDrive,   //!< Rear-wheel drive configuration
        AllWheelDrive,    //!< All-wheel drive configuration
        FourWheelDrive,   //!< Four-wheel drive configuration
        Count             //!< Total number of drive types
    };

    enum class VertexAnimationType : u8
    {
        /// No animation
        VAT_NONE = 0,
        /// Morph animation is made up of many interpolated snapshot keyframes
        VAT_MORPH = 1,
        /// Pose animation is made up of a single delta pose keyframe
        VAT_POSE = 2
    };

    /** The target animation mode */
    enum class TargetMode : u8
    {
        /// Interpolate vertex positions in software
        TM_SOFTWARE,
        /** Bind keyframe 1 to position, and keyframe 2 to a texture coordinate
            for interpolation in hardware */
        TM_HARDWARE
    };

    enum class SkeletonAnimationBlendMode : u8
    {
        /// Animations are applied by calculating a weighted average of all animations
        ANIMBLEND_AVERAGE = 0,
        /// Animations are applied by calculating a weighted cumulative total
        ANIMBLEND_CUMULATIVE = 1
    };

    /**
     * @enum ParameterType
     * @brief Identifies the data type of a property or parameter.
     * @details Used by the property system for serialization and editor integration.
     */
    enum class ParameterType : u8
    {
        PARAM_TYPE_VOID,         ///< Void type.
        PARAM_TYPE_NULL,         ///< Null type.
        PARAM_TYPE_BOOL,         ///< Boolean type.
        PARAM_TYPE_U8,           ///< 8-bit unsigned integer.
        PARAM_TYPE_U16,          ///< 16-bit unsigned integer.
        PARAM_TYPE_U32,          ///< 32-bit unsigned integer.
        PARAM_TYPE_S8,           ///< 8-bit signed integer.
        PARAM_TYPE_S16,          ///< 16-bit signed integer.
        PARAM_TYPE_S32,          ///< 32-bit signed integer.
        PARAM_TYPE_F32,          ///< 32-bit floating point.
        PARAM_TYPE_S64,          ///< 64-bit signed integer.
        PARAM_TYPE_F64,          ///< 64-bit floating point.
        PARAM_TYPE_CHAR_PTR,     ///< C-style string pointer.
        PARAM_TYPE_PTR,          ///< Generic pointer.
        PARAM_TYPE_BUTTON,       ///< Button action (UI).
        PARAM_TYPE_OBJECT,       ///< Object reference.
        PARAM_TYPE_COMPONENT,    ///< Component reference.
        PARAM_TYPE_TEXTURE,      ///< Texture resource reference.
        PARAM_TYPE_RESOURCE,     ///< Generic resource reference.
        PARAM_TYPE_STR,          ///< String type.
        PARAM_TYPE_VEC2I,        ///< 2D integer vector.
        PARAM_TYPE_VEC2F,        ///< 2D float vector.
        PARAM_TYPE_VEC2D,        ///< 2D double vector.
        PARAM_TYPE_VEC3I,        ///< 3D integer vector.
        PARAM_TYPE_VEC3F,        ///< 3D float vector.
        PARAM_TYPE_VEC3D,        ///< 3D double vector.
        PARAM_TYPE_QUATF,        ///< Float quaternion.
        PARAM_TYPE_QUATD,        ///< Double quaternion.
        PARAM_TYPE_COLOUR,       ///< Floating-point color (RGBA).
        PARAM_TYPE_COLOURI,      ///< Integer color (RGBA).
        PARAM_TYPE_AABB3,        ///< 3D axis-aligned bounding box (float).
        PARAM_TYPE_AABB3D,       ///< 3D axis-aligned bounding box (double).
        PARAM_TYPE_ARRAY,        ///< Array/list type.
        PARAM_TYPE_ENUM,         ///< Enumeration type.
        PARAM_TYPE_TRANSFORM3,   ///< 3D transform (float).
        PARAM_TYPE_TRANSFORM3D,  ///< 3D transform (double).
        PARAM_TYPE_COUNT         ///< Total number of parameter types.
    };

    enum class AabbExtent : u8
    {
        Null,     ///< An empty AABB.
        Finite,   ///< A finite AABB.
        Infinite  ///< An AABB that encompasses all possible values.
    };

    /**
     * @enum LightTypes
     * @brief Defines the types of light sources supported by the renderer.
     * @details Each light type has different behavior and performance characteristics.
     */
    enum class LightTypes : u8
    {
        LT_DIRECTIONAL = 0,  ///< Directional light (sun-like, parallel rays, no position).
        LT_POINT = 1,        ///< Point light (omnidirectional, has position, no direction).
        LT_SPOTLIGHT = 2,    ///< Spotlight (cone of light with position, direction, and falloff).
        LT_VPL = 3,          ///< Volumetric Projected Light (spotlight with 3D texture projection).
        MAX_FORWARD_PLUS_LIGHTS = 4,  ///< Maximum light index for Forward+ rendering.
        LT_AREA_APPROX = 4,           ///< Approximate area light (non-PBR, flexible and fast).
        LT_AREA_LTC = 5,  ///< Linearly Transformed Cosines area light (PBR-accurate, slower).
        Count             ///< Total number of light types.
    };

    /**
     * @enum Endian
     * @brief The endianness options for written files.
     */
    enum class Endian : u8
    {
        ENDIAN_NATIVE, /**< Use the platform native endian. */
        ENDIAN_BIG,    /**< Use big endian (0x1000 is serialized as 0x10 0x00). */
        ENDIAN_LITTLE  /**< Use little endian (0x1000 is serialized as 0x00 0x10). */
    };

    /**
     * @enum UITypes
     * @brief Identifies different UI widget types.
     * @details Used by the UI system for widget creation and type-specific behavior.
     */
    enum class UITypes : u8
    {
        None,            ///< No widget type / invalid.
        Application,     ///< Application root widget.
        Button,          ///< Push button widget.
        Dropdown,        ///< Dropdown/combo box widget.
        Filebrowser,     ///< File browser dialog widget.
        Labelcheckbox,   ///< Labeled checkbox widget.
        Labeltextinput,  ///< Labeled text input widget.
        Menu,            ///< Menu widget.
        Menubar,         ///< Menu bar widget.
        Menuitem,        ///< Menu item widget.
        Propertygrid,    ///< Property grid/inspector widget.
        Renderwindow,    ///< Render viewport widget.
        Text,            ///< Static text label widget.
        TextEntry,       ///< Text entry/input widget.
        ToggleButton,    ///< Toggle/checkbox button widget.
        ToggleGroup,     ///< Radio button group widget.
        Toolbar,         ///< Toolbar widget.
        Treectrl,        ///< Tree view control widget.
        Treenode,        ///< Tree view node widget.
        Vector2,         ///< 2D vector editor widget.
        Vector3,         ///< 3D vector editor widget.
        Vector4,         ///< 4D vector editor widget.
        Window,          ///< Window container widget.
        Count            ///< Total number of UI types.
    };

    /**
     * @brief Enumeration of supported target languages for script generation
     */
    enum class LanguageType
    {
        CPP,     ///< C++ language target
        CSHARP,  ///< C# language target
        LUA,     ///< Lua scripting language target

        Count  ///< Total count of supported languages
    };

    /** Pixel format for lightmap data */
    enum class LightmapFormat
    {
        RGB8,    /** 8-bit per channel RGB */
        RGBA8,   /** 8-bit per channel RGBA */
        RGB16F,  /** 16-bit float per channel RGB */
        RGBA16F, /** 16-bit float per channel RGBA */
        RGB32F,  /** 32-bit float per channel RGB */
        RGBA32F  /** 32-bit float per channel RGBA */
    };

    /** Enum identifying the intended usage of a texture, which may affect memory placement and
     * performance. */
    enum class TextureUsage
    {
        /// @copydoc HardwareBuffer::Usage
        TU_STATIC = ( 1 << 1 ),
        TU_DYNAMIC = ( 1 << 2 ),
        TU_WRITE_ONLY = ( 1 << 3 ),
        TU_STATIC_WRITE_ONLY = ( 1 << 4 ),
        TU_DYNAMIC_WRITE_ONLY = ( 1 << 5 ),
        TU_DYNAMIC_WRITE_ONLY_DISCARDABLE = ( 1 << 6 ),
        /// mipmaps will be automatically generated for this texture
        TU_AUTOMIPMAP = ( 1 << 7 ),
        /// this texture will be a render target, i.e. used as a target for render to texture
        /// setting this flag will ignore all other texture usages except TU_AUTOMIPMAP
        TU_RENDERTARGET = ( 1 << 8 ),
        /// default to automatic mipmap generation static textures
        TU_DEFAULT = TU_AUTOMIPMAP | TU_STATIC_WRITE_ONLY
    };

    /** Enum identifying the texture type
     */
    enum class TextureType
    {
        /// 1D texture, used in combination with 1D texture coordinates
        TEX_TYPE_1D = 1,
        /// 2D texture, used in combination with 2D texture coordinates (default)
        TEX_TYPE_2D = 2,
        /// 3D volume texture, used in combination with 3D texture coordinates
        TEX_TYPE_3D = 3,
        /// 3D cube map, used in combination with 3D texture coordinates
        TEX_TYPE_CUBE_MAP = 4,
        /// 2D texture Array
        TEX_TYPE_2D_ARRAY = 5
    };

    /** Enum identifying special mipmap numbers
     */
    enum class TextureMipmap
    {
        /// Generate mipmaps up to 1x1
        MIP_UNLIMITED = 0x7FFFFFFF,
        /// Use TextureManager default
        MIP_DEFAULT = -1
    };

    /** The pixel format used for images, textures, and render surfaces */
    enum class PixelFormat
    {
        /// Unknown pixel format.
        PF_UNKNOWN = 0,
        /// 8-bit pixel format, all bits luminance.
        PF_L8 = 1,
        PF_BYTE_L = PF_L8,
        /// 16-bit pixel format, all bits luminance.
        PF_L16 = 2,
        PF_SHORT_L = PF_L16,
        /// 8-bit pixel format, all bits alpha.
        PF_A8 = 3,
        PF_BYTE_A = PF_A8,
        /// 8-bit pixel format, 4 bits alpha, 4 bits luminance.
        PF_A4L4 = 4,
        /// 2 byte pixel format, 1 byte luminance, 1 byte alpha
        PF_BYTE_LA = 5,
        /// 16-bit pixel format, 5 bits red, 6 bits green, 5 bits blue.
        PF_R5G6B5 = 6,
        /// 16-bit pixel format, 5 bits red, 6 bits green, 5 bits blue.
        PF_B5G6R5 = 7,
        /// 8-bit pixel format, 2 bits blue, 3 bits green, 3 bits red.
        PF_R3G3B2 = 31,
        /// 16-bit pixel format, 4 bits for alpha, red, green and blue.
        PF_A4R4G4B4 = 8,
        /// 16-bit pixel format, 5 bits for blue, green, red and 1 for alpha.
        PF_A1R5G5B5 = 9,
        /// 24-bit pixel format, 8 bits for red, green and blue.
        PF_R8G8B8 = 10,
        /// 24-bit pixel format, 8 bits for blue, green and red.
        PF_B8G8R8 = 11,
        /// 32-bit pixel format, 8 bits for alpha, red, green and blue.
        PF_A8R8G8B8 = 12,
        /// 32-bit pixel format, 8 bits for blue, green, red and alpha.
        PF_A8B8G8R8 = 13,
        /// 32-bit pixel format, 8 bits for blue, green, red and alpha.
        PF_B8G8R8A8 = 14,
        /// 32-bit pixel format, 8 bits for red, green, blue and alpha.
        PF_R8G8B8A8 = 28,
        /// 32-bit pixel format, 8 bits for red, 8 bits for green, 8 bits for blue
        /// like PF_A8R8G8B8, but alpha will get discarded
        PF_X8R8G8B8 = 26,
        /// 32-bit pixel format, 8 bits for blue, 8 bits for green, 8 bits for red
        /// like PF_A8B8G8R8, but alpha will get discarded
        PF_X8B8G8R8 = 27,
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
        /// 3 byte pixel format, 1 byte for red, 1 byte for green, 1 byte for blue
        PF_BYTE_RGB = PF_R8G8B8,
        /// 3 byte pixel format, 1 byte for blue, 1 byte for green, 1 byte for red
        PF_BYTE_BGR = PF_B8G8R8,
        /// 4 byte pixel format, 1 byte for blue, 1 byte for green, 1 byte for red and one byte
        /// for alpha
        PF_BYTE_BGRA = PF_B8G8R8A8,
        /// 4 byte pixel format, 1 byte for red, 1 byte for green, 1 byte for blue, and one byte
        /// for alpha
        PF_BYTE_RGBA = PF_R8G8B8A8,
#else
        /// 3 byte pixel format, 1 byte for red, 1 byte for green, 1 byte for blue
        PF_BYTE_RGB = PF_B8G8R8,
        /// 3 byte pixel format, 1 byte for blue, 1 byte for green, 1 byte for red
        PF_BYTE_BGR = PF_R8G8B8,
        /// 4 byte pixel format, 1 byte for blue, 1 byte for green, 1 byte for red and one byte
        /// for alpha
        PF_BYTE_BGRA = PF_A8R8G8B8,
        /// 4 byte pixel format, 1 byte for red, 1 byte for green, 1 byte for blue, and one byte
        /// for alpha
        PF_BYTE_RGBA = PF_A8B8G8R8,
#endif
        /// 32-bit pixel format, 2 bits for alpha, 10 bits for red, green and blue.
        PF_A2R10G10B10 = 15,
        /// 32-bit pixel format, 10 bits for blue, green and red, 2 bits for alpha.
        PF_A2B10G10R10 = 16,
        /// DDS (DirectDraw Surface) DXT1 format
        PF_DXT1 = 17,
        /// DDS (DirectDraw Surface) DXT2 format
        PF_DXT2 = 18,
        /// DDS (DirectDraw Surface) DXT3 format
        PF_DXT3 = 19,
        /// DDS (DirectDraw Surface) DXT4 format
        PF_DXT4 = 20,
        /// DDS (DirectDraw Surface) DXT5 format
        PF_DXT5 = 21,
        // 16-bit pixel format, 16 bits (float) for red
        PF_FLOAT16_R = 32,
        // 48-bit pixel format, 16 bits (float) for red, 16 bits (float) for green, 16 bits
        // (float) for blue
        PF_FLOAT16_RGB = 22,
        // 64-bit pixel format, 16 bits (float) for red, 16 bits (float) for green, 16 bits
        // (float) for blue, 16 bits (float) for alpha
        PF_FLOAT16_RGBA = 23,
        // 32-bit pixel format, 32 bits (float) for red
        PF_FLOAT32_R = 33,
        // 96-bit pixel format, 32 bits (float) for red, 32 bits (float) for green, 32 bits
        // (float) for blue
        PF_FLOAT32_RGB = 24,
        // 128-bit pixel format, 32 bits (float) for red, 32 bits (float) for green, 32 bits
        // (float) for blue, 32 bits (float) for alpha
        PF_FLOAT32_RGBA = 25,
        // 32-bit, 2-channel s10e5 floating point pixel format, 16-bit green, 16-bit red
        PF_FLOAT16_GR = 35,
        // 64-bit, 2-channel floating point pixel format, 32-bit green, 32-bit red
        PF_FLOAT32_GR = 36,
        // Depth texture format
        PF_DEPTH = 29,
        // 64-bit pixel format, 16 bits for red, green, blue and alpha
        PF_SHORT_RGBA = 30,
        // 32-bit pixel format, 16-bit green, 16-bit red
        PF_SHORT_GR = 34,
        // 48-bit pixel format, 16 bits for red, green and blue
        PF_SHORT_RGB = 37,
        /// PVRTC (PowerVR) RGB 2 bpp
        PF_PVRTC_RGB2 = 38,
        /// PVRTC (PowerVR) RGBA 2 bpp
        PF_PVRTC_RGBA2 = 39,
        /// PVRTC (PowerVR) RGB 4 bpp
        PF_PVRTC_RGB4 = 40,
        /// PVRTC (PowerVR) RGBA 4 bpp
        PF_PVRTC_RGBA4 = 41,
        /// 8-bit pixel format, all bits red.
        PF_R8 = 42,
        /// 16-bit pixel format, 8 bits red, 8 bits green.
        PF_RG8 = 43,
        // Number of pixel formats currently defined
        PF_COUNT = 44
    };

    /**
     * @brief Enumeration of application states.
     *
     * Defines the various states that the application can be in during its lifecycle.
     * The finite state machine uses these states to manage application flow.
     */
    enum class ApplicationState : u8
    {
        None,
        ///< Initial state, application not yet initialized
        Loading,
        ///< Application is loading resources and initializing systems
        Running,
        ///< Application is running the main loop
        Close,
        ///< Application is preparing to close
        Exit,
        ///< Application is exiting and cleaning up
        Count  ///< Total number of states (for bounds checking)
    };

    enum class PanelFlags
    {
        BORDER = 1,
        MOVABLE = 1 << 1,
        SCALABLE = 1 << 2,
        CLOSABLE = 1 << 3,
        MINIMIZABLE = 1 << 4,
        NO_SCROLLBAR = 1 << 5,
        TITLE = 1 << 6,
        SCROLL_AUTO_HIDE = 1 << 7,
        BACKGROUND = 1 << 8,
        SCALE_LEFT = 1 << 9,
        NO_INPUT = 1 << 10
    };

    enum class EventType
    {
        Loading,
        Object,
        Input,
        IO,
        UI,
        Window,
        Scene,
        Actor,
        Component,
        Application,
        Renderer,

        Count
    };

    enum class GrowthPolicy
    {
        Fixed,

        // Grow by m_growthSize nodes.
        Grow,

        // Double the current capacity.
        // If capacity == 0, m_growthSize is used.
        Double,

        Default = Double
    };

}  // namespace workphone

#endif  // __WP_CoreEnums_h__
