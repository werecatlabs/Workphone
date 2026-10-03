#ifndef workphone_script_table_h__
#define workphone_script_table_h__

#include "workphone_script_value.h"

typedef struct
{
    wp_c8 *key;
    wp_script_value value;
} wp_script_entry;

typedef struct
{
    wp_script_entry entries[TABLE_SIZE];
} wp_script_table;

WP_SCRIPT_LIB wp_u32 wp_script_table_hash( const wp_c8 *s );

WP_SCRIPT_LIB wp_s32 wp_script_table_set( wp_script_table *t, const wp_c8 *key, wp_script_value v );
WP_SCRIPT_LIB wp_script_value wp_script_table_get( const wp_script_table *t, const wp_c8 *key );
WP_SCRIPT_LIB wp_s32 wp_script_table_contains( const wp_script_table *t, const wp_c8 *key );
WP_SCRIPT_LIB void wp_script_table_clear( wp_script_table *t );

#endif /* workphone_script_table_h__ */
