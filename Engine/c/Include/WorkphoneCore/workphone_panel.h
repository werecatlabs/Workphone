#ifndef workphone_panel_h__
#define workphone_panel_h__

#include "workphone_menu.h"

struct wp_row_layout
{
    enum wp_panel_row_layout_type type;
    int index;
    wp_f32 height;
    wp_f32 min_height;
    int columns;
    const wp_f32 *ratio;
    wp_f32 item_width;
    wp_f32 item_height;
    wp_f32 item_offset;
    wp_f32 filled;
    struct wp_rect item;
    int tree_depth;
    wp_f32 templates[WORKPHONE_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS];
};

struct wp_panel
{
    enum wp_panel_type type;
    wp_flags flags;
    struct wp_rect bounds;
    wp_u32 *offset_x;
    wp_u32 *offset_y;
    wp_f32 at_x;
    wp_f32 at_y;
    wp_f32 max_x;
    wp_f32 footer_height;
    wp_f32 header_height;
    wp_f32 border;
    unsigned int has_scrolling;
    struct wp_rect clip;
    struct wp_row_layout row;
    struct wp_chart *chart;
    struct wp_menu_state menu;
    struct wp_panel *parent;
};

WORKPHONE_LIB void *wp_create_panel( struct wp_context *ctx );
WORKPHONE_LIB void wp_free_panel( struct wp_context *, struct wp_panel *pan );
WORKPHONE_LIB wp_bool wp_panel_has_header( wp_flags flags, const wp_c8 *title );
WORKPHONE_LIB struct wp_vec2f wp_panel_get_padding( const struct wp_style *style,
                                                   enum wp_panel_type type );
WORKPHONE_LIB wp_f32 wp_panel_get_border( const struct wp_style *style, wp_flags flags,
                                          enum wp_panel_type type );
WORKPHONE_LIB struct wp_color wp_panel_get_border_color( const struct wp_style *style,
                                                         enum wp_panel_type type );
WORKPHONE_LIB wp_bool wp_panel_is_sub( enum wp_panel_type type );
WORKPHONE_LIB wp_bool wp_panel_is_nonblock( enum wp_panel_type type );
WORKPHONE_LIB wp_bool wp_panel_begin( struct wp_context *ctx, const wp_c8 *title,
                                      enum wp_panel_type panel_type );
WORKPHONE_LIB void wp_panel_end( struct wp_context *ctx );

#endif  // workphone_panel_h__
