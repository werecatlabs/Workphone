#ifndef workphone_group_h__
#define workphone_group_h__

#include "workphone_prerequisites.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 *
 *                                  GROUP
 *
 * =============================================================================*/
/**
 * \page Groups
 * Groups are basically windows inside windows. They allow to subdivide space
 * in a window to layout widgets as a group. Almost all more complex widget
 * layouting requirements can be solved using groups and basic layouting
 * fuctionality. Groups just like windows are identified by an unique name and
 * internally keep track of scrollbar offsets by default. However additional
 * versions are provided to directly manage the scrollbar.
 *
 * # Usage
 * To create a group you have to call one of the three `wp_group_begin_xxx`
 * functions to start group declarations and `wp_group_end` at the end. Furthermore it
 * is required to check the return value of `wp_group_begin_xxx` and only process
 * widgets inside the window if the value is not 0.
 * Nesting groups is possible and even encouraged since many layouting schemes
 * can only be achieved by nesting. Groups, unlike windows, need `wp_group_end`
 * to be only called if the corresponding `wp_group_begin_xxx` call does not return 0:
 *
 * ```c
 * if (wp_group_begin_xxx(ctx, ...) {
 *     // [... widgets ...]
 *     wp_group_end(ctx);
 * }
 * ```
 *
 * In the grand concept groups can be called after starting a window
 * with `wp_begin_xxx` and before calling `wp_end`:
 *
 * ```c
 * struct wp_context ctx;
 * wp_init_xxx(&ctx, ...);
 * while (1) {
 *     // Input
 *     Event evt;
 *     wp_input_begin(&ctx);
 *     while (GetEvent(&evt)) {
 *         if (evt.type == MOUSE_MOVE)
 *             wp_input_motion(&ctx, evt.motion.x, evt.motion.y);
 *         else if (evt.type == [...]) {
 *             wp_input_xxx(...);
 *         }
 *     }
 *     wp_input_end(&ctx);
 *     //
 *     // Window
 *     if (wp_begin_xxx(...) {
 *         // [...widgets...]
 *         wp_layout_row_dynamic(...);
 *         if (wp_group_begin_xxx(ctx, ...) {
 *             //[... widgets ...]
 *             wp_group_end(ctx);
 *         }
 *     }
 *     wp_end(ctx);
 *
 *     // Draw
 *     const struct wp_command *cmd = 0;
 *     wp_foreach(cmd, &ctx) {
 *     switch (cmd->type) {
 *     case WORKPHONE_COMMAND_LINE:
 *         your_draw_line_function(...)
 *         break;
 *     case WORKPHONE_COMMAND_RECT
 *         your_draw_rect_function(...)
 *         break;
 *     case ...:
 *         // [...]
 *     }
 *     wp_clear(&ctx);
 * }
 * wp_free(&ctx);
 * ```
 *
 * # Reference
 * Function                        | Description
 * --------------------------------|-------------------------------------------
 * \ref wp_group_begin                  | Start a new group with internal scrollbar handling
 * \ref wp_group_begin_titled           | Start a new group with separated name and title and internal scrollbar handling
 * \ref wp_group_end                    | Ends a group. Should only be called if wp_group_begin returned non-zero
 * \ref wp_group_scrolled_offset_begin  | Start a new group with manual separated handling of scrollbar x- and y-offset
 * \ref wp_group_scrolled_begin         | Start a new group with manual scrollbar handling
 * \ref wp_group_scrolled_end           | Ends a group with manual scrollbar handling. Should only be called if wp_group_begin returned non-zero
 * \ref wp_group_get_scroll             | Gets the scroll offset for the given group
 * \ref wp_group_set_scroll             | Sets the scroll offset for the given group
 */

/**
 * \brief Starts a new widget group. Requires a previous layouting function to specify a pos/size.
 * ```c
 * wp_bool wp_group_begin(struct wp_context*, const wp_c8 *title, wp_flags);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must powp_s32 to an previously initialized `wp_context` struct
 * \param[in] title   | Must be an unique identifier for this group that is also used for the group header
 * \param[in] flags   | Window flags defined in the wp_panel_flags section with a number of different group behaviors
 *
 * \returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
 */
WORKPHONE_API wp_bool wp_group_begin( struct wp_context *, const wp_c8 *title, wp_flags );

/**
 * \brief Starts a new widget group. Requires a previous layouting function to specify a pos/size.
 * ```c
 * wp_bool wp_group_begin_titled(struct wp_context*, const wp_c8 *name, const wp_c8 *title, wp_flags);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must powp_s32 to an previously initialized `wp_context` struct
 * \param[in] name    | Window identifier. Needs to be persistent over frames to identify the window
 * \param[in] title   | Window title displayed inside header if flag `WORKPHONE_WINDOW_TITLE` or either `WORKPHONE_WINDOW_CLOSABLE` or `WORKPHONE_WINDOW_MINIMIZED` was set
 * \param[in] flags   | Window flags defined in the wp_panel_flags section with a number of different group behaviors
 *
 * \returns `true(1)` if visible and fillable with widgets from this point
 * until `wp_group_end` or `false(0)` otherwise for example if collapsed

 */
WORKPHONE_API wp_bool wp_group_begin_titled( struct wp_context *, const wp_c8 *name, const wp_c8 *title,
                                             wp_flags );

/**
 * # # wp_group_end
 * Ends a widget group
 * ```c
 * void wp_group_end(struct wp_context*);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must powp_s32 to an previously initialized `wp_context` struct
 */
WORKPHONE_API void wp_group_end( struct wp_context * );

/**
 * # # wp_group_scrolled_offset_begin
 * starts a new widget group. requires a previous layouting function to specify
 * a size. Does not keep track of scrollbar.
 * ```c
 * wp_bool wp_group_scrolled_offset_begin(struct wp_context*, wp_u32 *x_offset, wp_u32 *y_offset, const wp_c8 *title, wp_flags flags);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must powp_s32 to an previously initialized `wp_context` struct
 * \param[in] x_offset| Scrollbar x-offset to offset all widgets inside the group horizontally.
 * \param[in] y_offset| Scrollbar y-offset to offset all widgets inside the group vertically
 * \param[in] title   | Window unique group title used to both identify and display in the group header
 * \param[in] flags   | Window flags defined in the wp_panel_flags section with a number of different group behaviors
 *
 * \returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
 */
WORKPHONE_API wp_bool wp_group_scrolled_offset_begin( struct wp_context *, wp_u32 *x_offset,
                                                      wp_u32 *y_offset, const wp_c8 *title,
                                                      wp_flags flags );

/**
 * # # wp_group_scrolled_begin
 * Starts a new widget group. requires a previous
 * layouting function to specify a size. Does not keep track of scrollbar.
 * ```c
 * wp_bool wp_group_scrolled_begin(struct wp_context*, struct wp_scroll *off, const wp_c8 *title, wp_flags);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must powp_s32 to an previously initialized `wp_context` struct
 * \param[in] off     | Both x- and y- scroll offset. Allows for manual scrollbar control
 * \param[in] title   | Window unique group title used to both identify and display in the group header
 * \param[in] flags   | Window flags defined in the wp_panel_flags section with a number of different group behaviors
 *
 * \returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
 */
WORKPHONE_API wp_bool wp_group_scrolled_begin( struct wp_context *, struct wp_scroll *off,
                                               const wp_c8 *title, wp_flags );

/**
 * # # wp_group_scrolled_end
 * Ends a widget group after calling wp_group_scrolled_offset_begin or wp_group_scrolled_begin.
 * ```c
 * void wp_group_scrolled_end(struct wp_context*);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must powp_s32 to an previously initialized `wp_context` struct after calling `wp_group_xxx_push_xxx`
 */
WORKPHONE_API void wp_group_scrolled_end( struct wp_context * );

/**
 * # # wp_group_get_scroll
 * Gets the scroll position of the given group.
 * ```c
 * void wp_group_get_scroll(struct wp_context*, const wp_c8 *id, wp_u32 *x_offset, wp_u32 *y_offset);
 * ```
 *
 * Parameter    | Description
 * -------------|-----------------------------------------------------------
 * \param[in] ctx      | Must powp_s32 to an previously initialized `wp_context` struct
 * \param[in] id       | The id of the group to get the scroll position of
 * \param[in] x_offset | A pointer to the x offset output (or NULL to ignore)
 * \param[in] y_offset | A pointer to the y offset output (or NULL to ignore)
 */
WORKPHONE_API void wp_group_get_scroll( struct wp_context *, const wp_c8 *id, wp_u32 *x_offset,
                                        wp_u32 *y_offset );

/**
 * # # wp_group_set_scroll
 * Sets the scroll position of the given group.
 * ```c
 * void wp_group_set_scroll(struct wp_context*, const wp_c8 *id, wp_u32 x_offset, wp_u32 y_offset);
 * ```
 *
 * Parameter    | Description
 * -------------|-----------------------------------------------------------
 * \param[in] ctx      | Must powp_s32 to an previously initialized `wp_context` struct
 * \param[in] id       | The id of the group to scroll
 * \param[in] x_offset | The x offset to scroll to
 * \param[in] y_offset | The y offset to scroll to
 */
WORKPHONE_API void wp_group_set_scroll( struct wp_context *, const wp_c8 *id, wp_u32 x_offset,
                                        wp_u32 y_offset );

#ifdef __cplusplus
}
#endif
#endif  // workphone_group_h__
