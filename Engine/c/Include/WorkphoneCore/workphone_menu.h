#ifndef workphone_menu_h__
#define workphone_menu_h__

#include "workphone_prerequisites.h"
#include "workphone_math.h"

struct wp_popup_buffer
{
    wp_size begin;
    wp_size end;
    wp_size parent;
    wp_size last;
    wp_size active;
};

struct wp_popup_state
{
    struct wp_window *win;
    enum wp_panel_type type;
    struct wp_popup_buffer buf;
    wp_hash name;
    wp_bool active;
    wp_u32 combo_count;
    wp_u32 con_count, con_old;
    wp_u32 active_con;
    struct wp_rect header;
};

struct wp_menu_state
{
    wp_f32 x, y, w, h;
    struct wp_scroll offset;
};

#endif  // workphone_menu_h__
