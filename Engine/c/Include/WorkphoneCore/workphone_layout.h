#ifndef workphone_layout_h__
#define workphone_layout_h__

#include "workphone_prerequisites.h"

/**
 * Sets the currently used minimum row height.
 * !!! \warning
 *     The passed height needs to include both your preferred row height
 *     as well as padding. No internal padding is added.
 *
 * ```c
 * void wp_layout_set_min_row_height(struct wp_context*, wp_f32 height);
 * ```
 *
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] height  | New minimum row height to be used for auto generating the row height
 */
WORKPHONE_API void wp_layout_set_min_row_height( struct wp_context *, wp_f32 height );

/**
 * Reset the currently used minimum row height back to `font_height + text_padding + padding`
 * ```c
 * void wp_layout_reset_min_row_height(struct wp_context*);
 * ```
 *
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 */
WORKPHONE_API void wp_layout_reset_min_row_height( struct wp_context * );

/**
 * \brief Returns the width of the next row allocate by one of the layouting functions
 *
 * \details
 * ```c
 * struct wp_rect wp_layout_widget_bounds(struct wp_context*);
 * ```
 *
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 *
 * \return `wp_rect` with both position and size of the next row
 */
WORKPHONE_API struct wp_rect wp_layout_widget_bounds( const struct wp_context *ctx );

/**
 * \brief Utility functions to calculate window ratio from pixel size
 *
 * \details
 * ```c
 * wp_f32 wp_layout_ratio_from_pixel(struct wp_context*, wp_f32 pixel_width);
 * ```
 *
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] pixel   | Pixel_width to convert to window ratio
 *
 * \returns `wp_rect` with both position and size of the next row
 */
WORKPHONE_API wp_f32 wp_layout_ratio_from_pixel( const struct wp_context *ctx, wp_f32 pixel_width );

/**
 * \brief Sets current row layout to share horizontal space
 * between @cols number of widgets evenly. Once called all subsequent widget
 * calls greater than @cols will allocate a new row with same layout.
 *
 * \details
 * ```c
 * void wp_layout_row_dynamic(struct wp_context *ctx, wp_f32 height, wp_s32 cols);
 * ```
 *
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] height  | Holds height of each widget in row or zero for auto layouting
 * \param[in] columns | Number of widget inside row
 */
WORKPHONE_API void wp_layout_row_dynamic( struct wp_context *ctx, wp_f32 height, wp_s32 cols );

/**
 * \brief Sets current row layout to fill @cols number of widgets
 * in row with same @item_width horizontal size. Once called all subsequent widget
 * calls greater than @cols will allocate a new row with same layout.
 *
 * \details
 * ```c
 * void wp_layout_row_static(struct wp_context *ctx, wp_f32 height, wp_s32 item_width, wp_s32 cols);
 * ```
 *
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] height  | Holds height of each widget in row or zero for auto layouting
 * \param[in] width   | Holds pixel width of each widget in the row
 * \param[in] columns | Number of widget inside row
 */
WORKPHONE_API void wp_layout_row_static( struct wp_context *ctx, wp_f32 height, wp_s32 item_width,
                                         wp_s32 cols );

/**
 * \brief Starts a new dynamic or fixed row with given height and columns.
 *
 * \details
 * ```c
 * void wp_layout_row_begin(struct wp_context *ctx, enum wp_layout_format fmt, wp_f32 row_height, wp_s32 cols);
 * ```
 *
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] fmt     | either `WORKPHONE_DYNAMIC` for window ratio or `WORKPHONE_STATIC` for fixed size columns
 * \param[in] height  | holds height of each widget in row or zero for auto layouting
 * \param[in] columns | Number of widget inside row
 */
WORKPHONE_API void wp_layout_row_begin( struct wp_context *ctx, enum wp_layout_format fmt,
                                        wp_f32 row_height, wp_s32 cols );

/**
 * \breif Specifies either window ratio or width of a single column
 *
 * \details
 * ```c
 * void wp_layout_row_push(struct wp_context*, wp_f32 value);
 * ```
 *
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] value   | either a window ratio or fixed width depending on @fmt in previous `wp_layout_row_begin` call
 */
WORKPHONE_API void wp_layout_row_push( struct wp_context *, wp_f32 value );

/**
 * \brief Finished previously started row
 *
 * \details
 * ```c
 * void wp_layout_row_end(struct wp_context*);
 * ```
 *
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 */
WORKPHONE_API void wp_layout_row_end( struct wp_context * );

/**
 * \brief Specifies row columns in array as either window ratio or size
 *
 * \details
 * ```c
 * void wp_layout_row(struct wp_context*, enum wp_layout_format, wp_f32 height, wp_s32 cols, const wp_f32 *ratio);
 * ```
 *
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] fmt     | Either `WORKPHONE_DYNAMIC` for window ratio or `WORKPHONE_STATIC` for fixed size columns
 * \param[in] height  | Holds height of each widget in row or zero for auto layouting
 * \param[in] columns | Number of widget inside row
 */
WORKPHONE_API void wp_layout_row( struct wp_context *, enum wp_layout_format, wp_f32 height, wp_s32 cols,
                                  const wp_f32 *ratio );

/**
 * # # wp_layout_row_template_begin
 * Begins the row template declaration
 * ```c
 * void wp_layout_row_template_begin(struct wp_context*, wp_f32 row_height);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] height  | Holds height of each widget in row or zero for auto layouting
 */
WORKPHONE_API void wp_layout_row_template_begin( struct wp_context *, wp_f32 row_height );

/**
 * # # wp_layout_row_template_push_dynamic
 * Adds a dynamic column that dynamically grows and can go to zero if not enough space
 * ```c
 * void wp_layout_row_template_push_dynamic(struct wp_context*);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] height  | Holds height of each widget in row or zero for auto layouting
 */
WORKPHONE_API void wp_layout_row_template_push_dynamic( struct wp_context * );

/**
 * # # wp_layout_row_template_push_variable
 * Adds a variable column that dynamically grows but does not shrink below specified pixel width
 * ```c
 * void wp_layout_row_template_push_variable(struct wp_context*, wp_f32 min_width);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] width   | Holds the minimum pixel width the next column must always be
 */
WORKPHONE_API void wp_layout_row_template_push_variable( struct wp_context *, wp_f32 min_width );

/**
 * # # wp_layout_row_template_push_static
 * Adds a static column that does not grow and will always have the same size
 * ```c
 * void wp_layout_row_template_push_static(struct wp_context*, wp_f32 width);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] width   | Holds the absolute pixel width value the next column must be
 */
WORKPHONE_API void wp_layout_row_template_push_static( struct wp_context *, wp_f32 width );

/**
 * # # wp_layout_row_template_end
 * Marks the end of the row template
 * ```c
 * void wp_layout_row_template_end(struct wp_context*);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 */
WORKPHONE_API void wp_layout_row_template_end( struct wp_context * );

/**
 * # # wp_layout_space_begin
 * Begins a new layouting space that allows to specify each widgets position and size.
 * ```c
 * void wp_layout_space_begin(struct wp_context*, enum wp_layout_format, wp_f32 height, wp_s32 widget_count);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_begin_xxx`
 * \param[in] fmt     | Either `WORKPHONE_DYNAMIC` for window ratio or `WORKPHONE_STATIC` for fixed size columns
 * \param[in] height  | Holds height of each widget in row or zero for auto layouting
 * \param[in] columns | Number of widgets inside row
 */
WORKPHONE_API void wp_layout_space_begin( struct wp_context *, enum wp_layout_format, wp_f32 height,
                                          wp_s32 widget_count );

/**
 * # # wp_layout_space_push
 * Pushes position and size of the next widget in own coordinate space either as pixel or ratio
 * ```c
 * void wp_layout_space_push(struct wp_context *ctx, struct wp_rect bounds);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_layout_space_begin`
 * \param[in] bounds  | Position and size in laoyut space local coordinates
 */
WORKPHONE_API void wp_layout_space_push( struct wp_context *, struct wp_rect bounds );

/**
 * # # wp_layout_space_end
 * Marks the end of the layout space
 * ```c
 * void wp_layout_space_end(struct wp_context*);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_layout_space_begin`
 */
WORKPHONE_API void wp_layout_space_end( struct wp_context * );

/**
 * # # wp_layout_space_bounds
 * Utility function to calculate total space allocated for `wp_layout_space`
 * ```c
 * struct wp_rect wp_layout_space_bounds(struct wp_context*);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_layout_space_begin`
 *
 * \returns `wp_rect` holding the total space allocated
 */
WORKPHONE_API struct wp_rect wp_layout_space_bounds( const struct wp_context *ctx );

/**
 * # # wp_layout_space_to_screen
 * Converts vector from wp_layout_space coordinate space into screen space
 * ```c
 * struct wp_vec2f wp_layout_space_to_screen(struct wp_context*, struct wp_vec2f);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_layout_space_begin`
 * \param[in] vec     | Position to convert from layout space into screen coordinate space
 *
 * \returns transformed `wp_vec2f` in screen space coordinates
 */
WORKPHONE_API struct wp_vec2f wp_layout_space_to_screen( const struct wp_context *ctx,
                                                        struct wp_vec2f vec );

/**
 * # # wp_layout_space_to_local
 * Converts vector from layout space into screen space
 * ```c
 * struct wp_vec2f wp_layout_space_to_local(struct wp_context*, struct wp_vec2f);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_layout_space_begin`
 * \param[in] vec     | Position to convert from screen space into layout coordinate space
 *
 * \returns transformed `wp_vec2f` in layout space coordinates
 */
WORKPHONE_API struct wp_vec2f wp_layout_space_to_local( const struct wp_context *ctx,
                                                       struct wp_vec2f vec );

/**
 * # # wp_layout_space_rect_to_screen
 * Converts rectangle from screen space into layout space
 * ```c
 * struct wp_rect wp_layout_space_rect_to_screen(struct wp_context*, struct wp_rect);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_layout_space_begin`
 * \param[in] bounds  | Rectangle to convert from layout space into screen space
 *
 * \returns transformed `wp_rect` in screen space coordinates
 */
WORKPHONE_API struct wp_rect wp_layout_space_rect_to_screen( const struct wp_context *ctx,
                                                             struct wp_rect bounds );

/**
 * # # wp_layout_space_rect_to_local
 * Converts rectangle from layout space into screen space
 * ```c
 * struct wp_rect wp_layout_space_rect_to_local(struct wp_context*, struct wp_rect);
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_layout_space_begin`
 * \param[in] bounds  | Rectangle to convert from layout space into screen space
 *
 * \returns transformed `wp_rect` in layout space coordinates
 */
WORKPHONE_API struct wp_rect wp_layout_space_rect_to_local( const struct wp_context *ctx,
                                                            struct wp_rect bounds );

/**
 * # # wp_spacer
 * Spacer is a dummy widget that consumes space as usual but doesn't draw anything
 * ```c
 * void wp_spacer(struct wp_context* );
 * ```
 *
 * Parameter   | Description
 * ------------|-----------------------------------------------------------
 * \param[in] ctx     | Must point to an previously initialized `wp_context` struct after call `wp_layout_space_begin`
 *
 */
WORKPHONE_API void wp_spacer( struct wp_context *ctx );

/* layout */
WORKPHONE_API float wp_layout_row_calculate_usable_space( const struct wp_style *style,
                                                          enum wp_panel_type type, float total_space,
                                                          int columns );
WORKPHONE_API void wp_panel_layout( const struct wp_context *ctx, struct wp_window *win, float height,
                                    int cols );
WORKPHONE_API void wp_row_layout( struct wp_context *ctx, enum wp_layout_format fmt, float height,
                                  int cols, int width );
WORKPHONE_API void wp_panel_alloc_row( const struct wp_context *ctx, struct wp_window *win );
WORKPHONE_API void wp_layout_widget_space( struct wp_rect *bounds, const struct wp_context *ctx,
                                           struct wp_window *win, int modify );
WORKPHONE_API void wp_panel_alloc_space( struct wp_rect *bounds, const struct wp_context *ctx );
WORKPHONE_API void wp_layout_peek( struct wp_rect *bounds, const struct wp_context *ctx );

#endif  // workphone_layout_h__
