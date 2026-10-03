#ifndef WORKPHONE_CONTEXT_IMPL_H_
#define WORKPHONE_CONTEXT_IMPL_H_

#include "workphone_color.h"
#include "workphone_draw.h"
#include "workphone_input.h"
#include "workphone_memory.h"
#include "workphone_panel.h"
#include "workphone_style.h"
#include "workphone_text.h"
#include "workphone_window.h"
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct chart_slot
{
    wp_chart_type type;
    wp_s32 index;
    wp_s32 count;
    struct wp_color color;
    struct wp_color highlight;
    wp_f32 min;
    wp_f32 max;
    wp_f32 range;
    struct wp_vec2f last;
    wp_bool show_markers;
};

struct wp_chart
{
    wp_s32 slot;
    wp_f32 x, y, w, h;
    struct chart_slot slots[WORKPHONE_CHART_MAX_SLOT];
};

#define wp_wp_f32 wp_f32
WORKPHONE_CONFIGURATION_STACK_TYPE( struct wp, style_item, style_item );
WORKPHONE_CONFIGURATION_STACK_TYPE( wp, wp_f32, wp_f32 );
WORKPHONE_CONFIGURATION_STACK_TYPE( wp, vec2, vec2f );
WORKPHONE_CONFIGURATION_STACK_TYPE( wp, flags, flags );
WORKPHONE_CONFIGURATION_STACK_TYPE( struct wp, color, color );
WORKPHONE_CONFIGURATION_STACK_TYPE( const struct wp, user_font, user_font * );
//WORKPHONE_CONFIGURATION_STACK_TYPE( enum wp, button_behavior, button_behavior );

struct wp_config_stack_button_behavior_element
{
    wp_button_behavior_enum *address;
    wp_button_behavior_enum old_value;
};

WORKPHONE_CONFIG_STACK( style_item, WORKPHONE_STYLE_ITEM_STACK_SIZE );
WORKPHONE_CONFIG_STACK( wp_f32, WORKPHONE_FLOAT_STACK_SIZE );
WORKPHONE_CONFIG_STACK( vec2, WORKPHONE_VECTOR_STACK_SIZE );
WORKPHONE_CONFIG_STACK( flags, WORKPHONE_FLAGS_STACK_SIZE );
WORKPHONE_CONFIG_STACK( color, WORKPHONE_COLOR_STACK_SIZE );
WORKPHONE_CONFIG_STACK( user_font, WORKPHONE_FONT_STACK_SIZE );
//WORKPHONE_CONFIG_STACK( button_behavior, WORKPHONE_BUTTON_BEHAVIOR_STACK_SIZE );

struct wp_config_stack_button_behavior
{
    int head;
    struct wp_config_stack_button_behavior_element elements[WORKPHONE_BUTTON_BEHAVIOR_STACK_SIZE];
};

struct wp_configuration_stacks
{
    struct wp_config_stack_style_item style_items;
    struct wp_config_stack_wp_f32 wp_f32s;
    struct wp_config_stack_vec2 vectors;
    struct wp_config_stack_flags flags;
    struct wp_config_stack_color colors;
    struct wp_config_stack_user_font fonts;
    struct wp_config_stack_button_behavior button_behaviors;
};

/*
struct wp_context
{
    struct wp_input input;
    struct wp_style style;
    struct wp_buffer memory;
    struct wp_clipboard clip;
    wp_flags last_widget_state;
    enum wp_button_behavior button_behavior;
    struct wp_configuration_stacks stacks;
    wp_f32 delta_time_seconds;

    struct wp_window *begin;
    struct wp_window *end;
    struct wp_window *active;
    struct wp_window *current;
    struct wp_page_element *freelist;
    unsigned int count;
    unsigned int seq;

    wp_bool use_pool;
    struct wp_pool pool;

    wp_bool build;
    struct wp_command_buffer overlay;

#ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    wp_handle userdata;
#endif

#ifdef WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
    struct wp_draw_list draw_list;
#endif
};
*/

struct wp_context
{
    /* public: can be accessed freely */
    struct wp_input input;
    struct wp_style style;
    struct wp_buffer memory;
    struct wp_clipboard clip;
    wp_flags last_widget_state;
    wp_button_behavior_enum button_behavior;
    struct wp_configuration_stacks stacks;
    wp_f32 delta_time_seconds;

/* private:
    should only be accessed if you
    know what you are doing */
#ifdef WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
    struct wp_draw_list draw_list;
#endif
#ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    wp_handle userdata;
#endif
    /** text editor objects are quite big because of an internal
     * undo/redo stack. Therefore it does not make sense to have one for
     * each window for temporary use cases, so I only provide *one* instance
     * for all windows. This works because the content is cleared anyway */
    struct wp_text_edit text_edit;
    /** draw buffer used for overlay drawing operation like cursor */
    struct wp_command_buffer overlay;

    /** windows */
    wp_s32 build;
    wp_s32 use_pool;
    struct wp_pool pool;
    struct wp_window *begin;
    struct wp_window *end;
    struct wp_window *active;
    struct wp_window *current;
    struct wp_page_element *freelist;
    wp_u32 count;
    wp_u32 seq;
};

/* Context management functions */
WORKPHONE_API wp_bool wp_init( struct wp_context *, const struct wp_allocator *,
                               const struct wp_user_font * );
WORKPHONE_API wp_bool wp_init_fixed( struct wp_context *, void *memory, wp_size size,
                                     const struct wp_user_font * );
WORKPHONE_API wp_bool wp_init_custom( struct wp_context *, struct wp_buffer *cmds,
                                      struct wp_buffer *pool, const struct wp_user_font * );
WORKPHONE_API void wp_clear( struct wp_context * );
WORKPHONE_API void wp_free( struct wp_context * );
#ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
WORKPHONE_API void wp_set_user_data( struct wp_context *, wp_handle );
#endif

WORKPHONE_API const struct wp_command *wp__begin( struct wp_context * );
WORKPHONE_API const struct wp_command *wp__next( struct wp_context *, const struct wp_command * );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_CONTEXT_IMPL_H_ */
