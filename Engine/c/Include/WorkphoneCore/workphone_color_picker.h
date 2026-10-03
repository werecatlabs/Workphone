#ifndef workphone_color_picker_h__
#define workphone_color_picker_h__

#include "workphone_prerequisites.h"

WORKPHONE_API struct wp_colorf wp_color_picker( struct wp_context *, struct wp_colorf,
                                                enum wp_color_format );
WORKPHONE_API wp_bool wp_color_pick( struct wp_context *, struct wp_colorf *, enum wp_color_format );

WORKPHONE_API struct wp_colorf wp_color_picker( struct wp_context *, struct wp_colorf,
                                                enum wp_color_format );
WORKPHONE_API wp_bool wp_color_pick( struct wp_context *, struct wp_colorf *, enum wp_color_format );

WORKPHONE_API void wp_property_int( struct wp_context *, const wp_c8 *name, wp_s32 min, wp_s32 *val,
                                    wp_s32 max, wp_s32 step, wp_f32 inc_per_pixel );

WORKPHONE_API void wp_property_wp_f32( struct wp_context *, const wp_c8 *name, wp_f32 min, wp_f32 *val,
                                       wp_f32 max, wp_f32 step, wp_f32 inc_per_pixel );

WORKPHONE_API void wp_property_wp_f64( struct wp_context *, const wp_c8 *name, wp_f64 min, wp_f64 *val,
                                       wp_f64 max, wp_f64 step, wp_f32 inc_per_pixel );

WORKPHONE_API wp_s32 wp_propertyi( struct wp_context *, const wp_c8 *name, wp_s32 min, wp_s32 val,
                                   wp_s32 max, wp_s32 step, wp_f32 inc_per_pixel );

WORKPHONE_API wp_f32 wp_propertyf( struct wp_context *, const wp_c8 *name, wp_f32 min, wp_f32 val,
                                   wp_f32 max, wp_f32 step, wp_f32 inc_per_pixel );

WORKPHONE_API wp_f64 wp_propertyd( struct wp_context *, const wp_c8 *name, wp_f64 min, wp_f64 val,
                                   wp_f64 max, wp_f64 step, wp_f32 inc_per_pixel );

#endif  // workphone_color_picker_h__
