#ifndef workphone_property_h__
#define workphone_property_h__

#include "workphone_prerequisites.h"

union wp_property
{
    wp_s32 i;
    wp_f32 f;
    wp_f64 d;
};

struct wp_property_variant
{
    enum wp_property_kind kind;
    union wp_property value;
    union wp_property min_value;
    union wp_property max_value;
    union wp_property step;
};

WORKPHONE_LIB struct wp_property_variant wp_property_variant_int( wp_s32 value, wp_s32 min_value,
                                                                  wp_s32 max_value, wp_s32 step );
WORKPHONE_LIB struct wp_property_variant wp_property_variant_wp_f32( wp_f32 value, wp_f32 min_value,
                                                                     wp_f32 max_value, wp_f32 step );
WORKPHONE_LIB struct wp_property_variant wp_property_variant_wp_f64( wp_f64 value, wp_f64 min_value,
                                                                     wp_f64 max_value, wp_f64 step );

#endif  // workphone_property_h__
