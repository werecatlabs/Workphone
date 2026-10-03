#ifndef workphone_window_h__
#define workphone_window_h__

#include "workphone_command_buffer.h"

#ifndef WORKPHONE_WINDOW_MAX_NAME
#    define WORKPHONE_WINDOW_MAX_NAME 64
#endif

/**
 * @struct wp_edit_state
 * @brief Maintains the state of an edit widget.
 */
struct wp_edit_state
{
    wp_hash name;                ///< Unique identifier hash for the edit state
    unsigned int seq;            ///< Sequence number for state tracking
    unsigned int old;            ///< Previous sequence number
    int active, prev;            ///< Activity flags: active and previous
    int cursor;                  ///< Current cursor position
    int sel_start;               ///< Start index of the selection
    int sel_end;                 ///< End index of the selection
    struct wp_scroll scrollbar;  ///< Scrollbar state for the edit field
    unsigned char mode;          ///< Input mode
    unsigned char single_line;   ///< Flag indicating if the field is single-line
};

/**
 * @struct wp_property_state
 * @brief Maintains the state of a property editor widget.
 */
struct wp_property_state
{
    int active, prev;                               ///< Activity flags: active and previous
    char buffer[WORKPHONE_MAX_NUMBER_BUFFER];       ///< Text buffer for the property value
    int length;                                     ///< Current length of the text in the buffer
    int cursor;                                     ///< Current cursor position
    int select_start;                               ///< Start index of the selection
    int select_end;                                 ///< End index of the selection
    wp_hash name;                                   ///< Unique identifier hash for the property
    unsigned int seq;                               ///< Sequence number for state tracking
    unsigned int old;                               ///< Previous sequence number
    int state;                                      ///< Current state of the property
    int prev_state;                                 ///< Previous state of the property
    wp_hash prev_name;                              ///< Identifier hash of the previous property
    char prev_buffer[WORKPHONE_MAX_NUMBER_BUFFER];  ///< Buffer of the previous property value
    int prev_length;                                ///< Length of the previous property value
};

/**
 * @struct wp_window
 * @brief Represents a window in the Workphone UI system.
 */
struct wp_window
{
    unsigned int seq;                             ///< Sequence number for window tracking
    wp_hash name;                                 ///< Unique identifier hash for the window
    char name_string[WORKPHONE_WINDOW_MAX_NAME];  ///< Human-readable name of the window
    wp_flags flags;                               ///< Window behavior flags

    struct wp_rect bounds;            ///< Screen position and size
    struct wp_scroll scrollbar;       ///< Scrollbar state
    struct wp_command_buffer buffer;  ///< Buffer for recorded commands
    struct wp_panel *layout;          ///< Pointer to the window layout
    wp_f32 scrollbar_hiding_timer;    ///< Timer for auto-hiding the scrollbar

    /* persistent widget state */
    struct wp_property_state property;  ///< State for property editor widgets
    struct wp_popup_state popup;        ///< State for popup widgets
    struct wp_edit_state edit;          ///< State for edit widgets
    unsigned int scrolled;              ///< Scroll offset/state
    wp_bool widgets_disabled;           ///< Flag indicating if widgets are disabled

    struct wp_table *tables;   ///< Pointer to window-associated tables
    unsigned int table_count;  ///< Number of tables associated with the window

    /* window list hooks */
    struct wp_window *next;    ///< Next window in the global list
    struct wp_window *prev;    ///< Previous window in the global list
    struct wp_window *parent;  ///< Parent window if nested
};

/**
 * @struct wp_table
 * @brief Table structure for storing window-related state or data.
 */
struct wp_table
{
    struct wp_table *next;                         ///< Next table in the list
    struct wp_table *prev;                         ///< Previous table in the list
    unsigned int seq;                              ///< Sequence number for the table
    unsigned int size;                             ///< Current number of elements in the table
    wp_hash keys[WORKPHONE_VALUE_PAGE_CAPACITY];   ///< Array of keys for fast lookup
    wp_u32 values[WORKPHONE_VALUE_PAGE_CAPACITY];  ///< Array of corresponding values
};

/**
 * @union wp_page_data
 * @brief Union of different page elements to save space in the page structure.
 */
union wp_page_data
{
    struct wp_table tbl;   ///< Table data
    struct wp_panel pan;   ///< Panel data
    struct wp_window win;  ///< Window data
};

/**
 * @struct wp_page_element
 * @brief A single element within a memory page.
 */
struct wp_page_element
{
    union wp_page_data data;       ///< The actual data of the element
    struct wp_page_element *next;  ///< Next element in the page list
    struct wp_page_element *prev;  ///< Previous element in the page list
};

/**
 * @struct wp_page
 * @brief A memory page containing a list of UI elements.
 */
struct wp_page
{
    unsigned int size;              ///< Size of the page
    struct wp_page *next;           ///< Next page in the sequence
    struct wp_page_element win[1];  ///< Flexible array member for page elements
};

/**
 * # # wp_begin
 * Starts a new window; needs to be called every frame for every
 * window (unless hidden) or otherwise the window gets removed
 *
 * ```c
 * wp_bool wp_begin(struct wp_context *ctx, const wp_c8 *title, struct wp_rect bounds, wp_flags flags);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] title   | Window title and identifier. Needs to be persistent over frames to identify the window
 * \param[in] bounds  | Initial position and window size. However if you do not define `WORKPHONE_WINDOW_SCALABLE` or `WORKPHONE_WINDOW_MOVABLE` you can set window position and size every frame
 * \param[in] flags   | Window flags defined in the wp_panel_flags section with a number of different window behaviors
 *
 * \returns `true(1)` if the window can be filled up with widgets from this point
 * until `wp_end` or `false(0)` otherwise for example if minimized

 */
WORKPHONE_API wp_bool wp_begin( struct wp_context *ctx, const wp_c8 *title, struct wp_rect bounds,
                                wp_flags flags );

/**
 * # # wp_begin_titled
 * Extended window start with separated title and identifier to allow multiple
 * windows with same title but not name
 *
 * ```c
 * wp_bool wp_begin_titled(struct wp_context *ctx, const wp_c8 *name, const wp_c8 *title, struct wp_rect bounds, wp_flags flags);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Window identifier. Needs to be persistent over frames to identify the window
 * \param[in] title   | Window title displayed inside header if flag `WORKPHONE_WINDOW_TITLE` or either `WORKPHONE_WINDOW_CLOSABLE` or `WORKPHONE_WINDOW_MINIMIZED` was set
 * \param[in] bounds  | Initial position and window size. However if you do not define `WORKPHONE_WINDOW_SCALABLE` or `WORKPHONE_WINDOW_MOVABLE` you can set window position and size every frame
 * \param[in] flags   | Window flags defined in the wp_panel_flags section with a number of different window behaviors
 *
 * \returns `true(1)` if the window can be filled up with widgets from this point
 * until `wp_end` or `false(0)` otherwise for example if minimized

 */
WORKPHONE_API wp_bool wp_begin_titled( struct wp_context *ctx, const wp_c8 *name, const wp_c8 *title,
                                       struct wp_rect bounds, wp_flags flags );

/**
 * # # wp_end
 * Needs to be called at the end of the window building process to process scaling, scrollbars and general cleanup.
 * All widget calls after this functions will result in asserts or no state changes
 *
 * ```c
 * void wp_end(struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct

 */
WORKPHONE_API void wp_end( struct wp_context *ctx );

/**
 * @brief Finds and returns a window by its name.
 *
 * @param ctx Must point to a previously initialized `wp_context` struct.
 * @param name Window identifier.
 * @return A pointer to the `wp_window` struct if found, otherwise NULL.
 */
WORKPHONE_API struct wp_window *wp_window_find( const struct wp_context *ctx, const wp_c8 *name );

/**
 * # # wp_window_get_bounds
 * \returns a rectangle with screen position and size of the currently processed window
 *
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 * ```c
 * struct wp_rect wp_window_get_bounds(const struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns a `wp_rect` struct with window upper left window position and size

 */
WORKPHONE_API struct wp_rect wp_window_get_bounds( const struct wp_context *ctx );

/**
 * # # wp_window_get_position
 * \returns the position of the currently processed window.
 *
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 * ```c
 * struct wp_vec2f wp_window_get_position(const struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns a `wp_vec2f` struct with window upper left position

 */
WORKPHONE_API struct wp_vec2f wp_window_get_position( const struct wp_context *ctx );

/**
 * # # wp_window_get_size
 * \returns the size with width and height of the currently processed window.
 *
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 * ```c
 * struct wp_vec2f wp_window_get_size(const struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns a `wp_vec2f` struct with window width and height

 */
WORKPHONE_API struct wp_vec2f wp_window_get_size( const struct wp_context *ctx );

/**
 * wp_window_get_width
 * \returns the width of the currently processed window.
 *
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 * ```c
 * wp_f32 wp_window_get_width(const struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns the current window width
 */
WORKPHONE_API wp_f32 wp_window_get_width( const struct wp_context *ctx );

/**
 * # # wp_window_get_height
 * \returns the height of the currently processed window.
 *
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 * ```c
 * wp_f32 wp_window_get_height(const struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns the current window height

 */
WORKPHONE_API wp_f32 wp_window_get_height( const struct wp_context *ctx );

/**
 * # # wp_window_get_panel
 * \returns the underlying panel which contains all processing state of the current window.
 *
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 * !!! \warning
 *     Do not keep the returned panel pointer around, it is only valid until `wp_end`
 * ```c
 * struct wp_panel* wp_window_get_panel(struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns a pointer to window internal `wp_panel` state.

 */
WORKPHONE_API struct wp_panel *wp_window_get_panel( const struct wp_context *ctx );

/**
 * # # wp_window_get_content_region
 * \returns the position and size of the currently visible and non-clipped space
 * inside the currently processed window.
 *
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 *
 * ```c
 * struct wp_rect wp_window_get_content_region(struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns `wp_rect` struct with screen position and size (no scrollbar offset)
 * of the visible space inside the current window

 */
WORKPHONE_API struct wp_rect wp_window_get_content_region( const struct wp_context *ctx );

/**
 * # # wp_window_get_content_region_min
 * \returns the upper left position of the currently visible and non-clipped
 * space inside the currently processed window.
 *
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 *
 * ```c
 * struct wp_vec2f wp_window_get_content_region_min(struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * returns `wp_vec2f` struct with  upper left screen position (no scrollbar offset)
 * of the visible space inside the current window

 */
WORKPHONE_API struct wp_vec2f wp_window_get_content_region_min( const struct wp_context *ctx );

/**
 * # # wp_window_get_content_region_max
 * \returns the lower right screen position of the currently visible and
 * non-clipped space inside the currently processed window.
 *
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 *
 * ```c
 * struct wp_vec2f wp_window_get_content_region_max(struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns `wp_vec2f` struct with lower right screen position (no scrollbar offset)
 * of the visible space inside the current window

 */
WORKPHONE_API struct wp_vec2f wp_window_get_content_region_max( const struct wp_context *ctx );

/**
 * # # wp_window_get_content_region_size
 * \returns the size of the currently visible and non-clipped space inside the
 * currently processed window
 *
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 *
 * ```c
 * struct wp_vec2f wp_window_get_content_region_size(struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns `wp_vec2f` struct with size the visible space inside the current window

 */
WORKPHONE_API struct wp_vec2f wp_window_get_content_region_size( const struct wp_context *ctx );

/**
 * # # wp_window_get_canvas
 * \returns the draw command buffer. Can be used to draw custom widgets
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 * !!! \warning
 *     Do not keep the returned command buffer pointer around it is only valid until `wp_end`
 *
 * ```c
 * struct wp_command_buffer* wp_window_get_canvas(struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns a pointer to window internal `wp_command_buffer` struct used as
 * drawing canvas. Can be used to do custom drawing.
 */
WORKPHONE_API struct wp_command_buffer *wp_window_get_canvas( const struct wp_context *ctx );

/**
 * # # wp_window_get_scroll
 * Gets the scroll offset for the current window
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 *
 * ```c
 * void wp_window_get_scroll(struct wp_context *ctx, wp_u32 *offset_x, wp_u32 *offset_y);
 * ```
 *
 * Parameter    | Description
 * -------------|-----------------------------------------------------------
 * \param[in] ctx      | Must point to an previously initialized `wp_context` struct
 * \param[in] offset_x | A pointer to the x offset output (or NULL to ignore)
 * \param[in] offset_y | A pointer to the y offset output (or NULL to ignore)

 */
WORKPHONE_API void wp_window_get_scroll( const struct wp_context *ctx, wp_u32 *offset_x,
                                         wp_u32 *offset_y );

/**
 * # # wp_window_has_focus
 * \returns if the currently processed window is currently active
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 * ```c
 * wp_bool wp_window_has_focus(const struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns `false(0)` if current window is not active or `true(1)` if it is

 */
WORKPHONE_API wp_bool wp_window_has_focus( const struct wp_context *ctx );

/**
 * # # wp_window_is_hovered
 * Return if the current window is being hovered
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 * ```c
 * wp_bool wp_window_is_hovered(struct wp_context *ctx);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns `true(1)` if current window is hovered or `false(0)` otherwise

 */
WORKPHONE_API wp_bool wp_window_is_hovered( const struct wp_context *ctx );

/**
 * # # wp_window_is_collapsed
 * \returns if the window with given name is currently minimized/collapsed
 * ```c
 * wp_bool wp_window_is_collapsed(struct wp_context *ctx, const wp_c8 *name);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of window you want to check if it is collapsed
 *
 * \returns `true(1)` if current window is minimized and `false(0)` if window not
 * found or is not minimized

 */
WORKPHONE_API wp_bool wp_window_is_collapsed( const struct wp_context *ctx, const wp_c8 *name );

/**
 * # # wp_window_is_closed
 * \returns if the window with given name was closed by calling `wp_close`
 * ```c
 * wp_bool wp_window_is_closed(struct wp_context *ctx, const wp_c8 *name);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of window you want to check if it is closed
 *
 * \returns `true(1)` if current window was closed or `false(0)` window not found or not closed

 */
WORKPHONE_API wp_bool wp_window_is_closed( const struct wp_context *ctx, const wp_c8 *name );

/**
 * # # wp_window_is_hidden
 * \returns if the window with given name is hidden
 * ```c
 * wp_bool wp_window_is_hidden(struct wp_context *ctx, const wp_c8 *name);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of window you want to check if it is hidden
 *
 * \returns `true(1)` if current window is hidden or `false(0)` window not found or visible

 */
WORKPHONE_API wp_bool wp_window_is_hidden( const struct wp_context *ctx, const wp_c8 *name );

/**
 * # # wp_window_is_active
 * Same as wp_window_has_focus for some reason
 * ```c
 * wp_bool wp_window_is_active(struct wp_context *ctx, const wp_c8 *name);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of window you want to check if it is active
 *
 * \returns `true(1)` if current window is active or `false(0)` window not found or not active
 */
WORKPHONE_API wp_bool wp_window_is_active( const struct wp_context *ctx, const wp_c8 *name );

/**
 * # # wp_window_is_any_hovered
 * \returns if the any window is being hovered
 * ```c
 * wp_bool wp_window_is_any_hovered(struct wp_context*);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns `true(1)` if any window is hovered or `false(0)` otherwise
 */
WORKPHONE_API wp_bool wp_window_is_any_hovered( const struct wp_context *ctx );

/**
 * # # wp_item_is_any_active
 * \returns if the any window is being hovered or any widget is currently active.
 * Can be used to decide if input should be processed by UI or your specific input handling.
 * Example could be UI and 3D camera to move inside a 3D space.
 * ```c
 * wp_bool wp_item_is_any_active(struct wp_context*);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 *
 * \returns `true(1)` if any window is hovered or any item is active or `false(0)` otherwise

 */
WORKPHONE_API wp_bool wp_item_is_any_active( const struct wp_context *ctx );

/**
 * # # wp_window_set_bounds
 * Updates position and size of window with passed in name
 * ```c
 * void wp_window_set_bounds(struct wp_context*, const wp_c8 *name, struct wp_rect bounds);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of the window to modify both position and size
 * \param[in] bounds  | Must point to a `wp_rect` struct with the new position and size

 */
WORKPHONE_API void wp_window_set_bounds( struct wp_context *ctx, const wp_c8 *name,
                                         struct wp_rect bounds );

/**
 * # # wp_window_set_position
 * Updates position of window with passed name
 * ```c
 * void wp_window_set_position(struct wp_context*, const wp_c8 *name, struct wp_vec2f pos);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of the window to modify both position
 * \param[in] pos     | Must point to a `wp_vec2f` struct with the new position

 */
WORKPHONE_API void wp_window_set_position( struct wp_context *ctx, const wp_c8 *name,
                                           struct wp_vec2f pos );

/**
 * # # wp_window_set_size
 * Updates size of window with passed in name
 * ```c
 * void wp_window_set_size(struct wp_context*, const wp_c8 *name, struct wp_vec2f);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of the window to modify both window size
 * \param[in] size    | Must point to a `wp_vec2f` struct with new window size

 */
WORKPHONE_API void wp_window_set_size( struct wp_context *ctx, const wp_c8 *name, struct wp_vec2f size );

/**
 * # # wp_window_set_focus
 * Sets the window with given name as active
 * ```c
 * void wp_window_set_focus(struct wp_context*, const wp_c8 *name);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of the window to set focus on

 */
WORKPHONE_API void wp_window_set_focus( struct wp_context *ctx, const wp_c8 *name );

/**
 * # # wp_window_set_scroll
 * Sets the scroll offset for the current window
 * !!! \warning
 *     Only call this function between calls `wp_begin_xxx` and `wp_end`
 *
 * ```c
 * void wp_window_set_scroll(struct wp_context *ctx, wp_u32 offset_x, wp_u32 offset_y);
 * ```
 *
 * Parameter    | Description
 * -------------|-----------------------------------------------------------
 * \param[in] ctx      | Must point to an previously initialized `wp_context` struct
 * \param[in] offset_x | The x offset to scroll to
 * \param[in] offset_y | The y offset to scroll to

 */
WORKPHONE_API void wp_window_set_scroll( struct wp_context *ctx, wp_u32 offset_x, wp_u32 offset_y );

/**
 * # # wp_window_close
 * Closes a window and marks it for being freed at the end of the frame
 * ```c
 * void wp_window_close(struct wp_context *ctx, const wp_c8 *name);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of the window to close

 */
WORKPHONE_API void wp_window_close( struct wp_context *ctx, const wp_c8 *name );

/**
 * # # wp_window_collapse
 * Updates collapse state of a window with given name
 * ```c
 * void wp_window_collapse(struct wp_context*, const wp_c8 *name, enum wp_collapse_states state);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of the window to close
 * \param[in] state   | value out of wp_collapse_states section

 */
WORKPHONE_API void wp_window_collapse( struct wp_context *ctx, const wp_c8 *name,
                                       enum wp_collapse_states state );

/**
 * # # wp_window_collapse_if
 * Updates collapse state of a window with given name if given condition is met
 * ```c
 * void wp_window_collapse_if(struct wp_context*, const wp_c8 *name, enum wp_collapse_states, wp_s32 cond);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of the window to either collapse or maximize
 * \param[in] state   | value out of wp_collapse_states section the window should be put into
 * \param[in] cond    | condition that has to be met to actually commit the collapse state change

 */
WORKPHONE_API void wp_window_collapse_if( struct wp_context *ctx, const wp_c8 *name,
                                          enum wp_collapse_states state, wp_s32 cond );

/**
 * # # wp_window_show
 * updates visibility state of a window with given name
 * ```c
 * void wp_window_show(struct wp_context*, const wp_c8 *name, enum wp_show_states);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of the window to either collapse or maximize
 * \param[in] state   | state with either visible or hidden to modify the window with
 */
WORKPHONE_API void wp_window_show( struct wp_context *ctx, const wp_c8 *name,
                                   enum wp_show_states state );

/**
 * # # wp_window_show_if
 * Updates visibility state of a window with given name if a given condition is met
 * ```c
 * void wp_window_show_if(struct wp_context*, const wp_c8 *name, enum wp_show_states, wp_s32 cond);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct
 * \param[in] name    | Identifier of the window to either hide or show
 * \param[in] state   | state with either visible or hidden to modify the window with
 * \param[in] cond    | condition that has to be met to actually commit the visibility state change

 */
WORKPHONE_API void wp_window_show_if( struct wp_context *ctx, const wp_c8 *name,
                                      enum wp_show_states state, wp_s32 cond );

/**
 * # # wp_window_show_if
 * Line for visual separation. Draws a line with thickness determined by the current row height.
 * ```c
 * void wp_rule_horizontal(struct wp_context *ctx, struct wp_color color, wp_bool rounding)
 * ```
 *
 * Parameter       | Description
 * ----------------|-------------------------------------------------------
 * \param[in] ctx         | Must point to an previously initialized `wp_context` struct
 * \param[in] color       | Color of the horizontal line
 * \param[in] rounding    | Whether or not to make the line round
 */
WORKPHONE_API void wp_rule_horizontal( struct wp_context *ctx, struct wp_color color, wp_bool rounding );

WORKPHONE_API void *wp_create_window( struct wp_context *ctx );
WORKPHONE_API void wp_remove_window( struct wp_context *, struct wp_window * );
WORKPHONE_API void wp_free_window( struct wp_context *ctx, struct wp_window *win );
WORKPHONE_API struct wp_window *wp_find_window( const struct wp_context *ctx, wp_hash hash,
                                                const wp_c8 *name );
WORKPHONE_API void wp_insert_window( struct wp_context *ctx, struct wp_window *win,
                                     enum wp_window_insert_location loc );

#endif  // workphone_window_h__
