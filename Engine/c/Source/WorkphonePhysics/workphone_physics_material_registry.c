/**
 * @file workphone_physics_material_registry.c
 * @brief Implementation of the data-driven physics material registry/database.
 *
 * The registry owns named wp_physics_material handles and resolves them by
 * name at runtime; the database is the plain array of descriptors that the
 * editor authors and the runtime registers.  The text loader implements a
 * small, human-authorable material library grammar.
 */

#include "workphone_physics_material_registry.h"
#include "workphone_physics_material.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Local helpers
 * ====================================================================== */

static wp_s32 wp_str_same( const wp_c8 *a, const wp_c8 *b )
{
    return strcmp( a, b ) == 0;
}

static wp_s32 wp_str_ieq( const wp_c8 *a, const wp_c8 *b )
{
#if defined( _MSC_VER )
    return _stricmp( a, b ) == 0;
#else
    return strcasecmp( a, b ) == 0;
#endif
}

static const wp_c8 *wp_skip_ws( const wp_c8 *p )
{
    while( *p && isspace( (unsigned char) *p ) )
    {
        ++p;
    }
    return p;
}

static void wp_reg_copy_name( wp_c8 *dst, const wp_c8 *src )
{
    wp_u32 i;
    if( !dst )
    {
        return;
    }
    if( !src )
    {
        dst[0] = '\0';
        return;
    }
    for( i = 0; i < ( wp_u32 )( WP_PHYSICS_MATERIAL_MAX_NAME - 1 ) && src[i] != '\0'; ++i )
    {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static void wp_trim_inplace( wp_c8 *s )
{
    wp_c8 *end;
    wp_c8 *start;
    if( !s || !*s )
    {
        return;
    }
    start = (wp_c8 *)wp_skip_ws( s );
    end = s + strlen( s );
    while( end > start && isspace( (unsigned char) end[-1] ) )
    {
        --end;
    }
    *end = '\0';
    if( start != s )
    {
        memmove( s, start, (size_t)( end - start ) + 1 );
    }
}

static wp_s32 wp_parse_float( const wp_c8 *s, wp_f32 *out )
{
    char *end = NULL;
    wp_f32 v;
    if( !s || !*s || !out )
    {
        return 0;
    }
    v = (wp_f32)strtod( s, &end );
    if( end == s )
    {
        return 0;
    }
    *out = v;
    return 1;
}

static wp_s32 wp_parse_friction_combine( const wp_c8 *s, wp_friction_combine_mode *out )
{
    if( !s || !out )
    {
        return 0;
    }
    if( wp_str_ieq( s, "average" ) )  { *out = WORKPHONE_FRICTION_COMBINE_AVERAGE;  return 1; }
    if( wp_str_ieq( s, "min" ) )      { *out = WORKPHONE_FRICTION_COMBINE_MIN;      return 1; }
    if( wp_str_ieq( s, "max" ) )      { *out = WORKPHONE_FRICTION_COMBINE_MAX;      return 1; }
    if( wp_str_ieq( s, "multiply" ) ) { *out = WORKPHONE_FRICTION_COMBINE_MULTIPLY; return 1; }
    return 0;
}

static wp_s32 wp_parse_restitution_combine( const wp_c8 *s, wp_restitution_combine_mode *out )
{
    if( !s || !out )
    {
        return 0;
    }
    if( wp_str_ieq( s, "average" ) )  { *out = WORKPHONE_RESTITUTION_COMBINE_AVERAGE;  return 1; }
    if( wp_str_ieq( s, "min" ) )      { *out = WORKPHONE_RESTITUTION_COMBINE_MIN;      return 1; }
    if( wp_str_ieq( s, "max" ) )      { *out = WORKPHONE_RESTITUTION_COMBINE_MAX;      return 1; }
    if( wp_str_ieq( s, "multiply" ) ) { *out = WORKPHONE_RESTITUTION_COMBINE_MULTIPLY; return 1; }
    return 0;
}

static wp_s32 wp_db_reserve( wp_physics_material_database *db, wp_u32 extra )
{
    wp_u32 new_capacity;
    wp_physics_material_desc *new_mem;
    if( !db )
    {
        return 0;
    }
    if( db->count + extra <= db->capacity )
    {
        return 1;
    }
    new_capacity = db->capacity ? db->capacity : WP_PHYSICS_MATERIAL_DATABASE_INITIAL_CAPACITY;
    while( new_capacity < db->count + extra )
    {
        new_capacity *= 2;
    }
    new_mem = (wp_physics_material_desc *)realloc( db->materials,
                                                   sizeof( wp_physics_material_desc ) * new_capacity );
    if( !new_mem )
    {
        return 0;
    }
    db->materials = new_mem;
    db->capacity = new_capacity;
    return 1;
}

/* =========================================================================
 * Database
 * ====================================================================== */

void wp_physics_material_database_init( wp_physics_material_database *db )
{
    if( !db )
    {
        return;
    }
    db->materials = NULL;
    db->count = 0;
    db->capacity = 0;
}

void wp_physics_material_database_free( wp_physics_material_database *db )
{
    if( !db )
    {
        return;
    }
    free( db->materials );
    db->materials = NULL;
    db->count = 0;
    db->capacity = 0;
}

wp_s32 wp_physics_material_database_add( wp_physics_material_database *db,
                                         const wp_physics_material_desc *desc )
{
    if( !db || !desc )
    {
        return 0;
    }
    if( !wp_db_reserve( db, 1 ) )
    {
        return 0;
    }
    db->materials[db->count] = *desc;
    db->materials[db->count].name[WP_PHYSICS_MATERIAL_MAX_NAME - 1] = '\0';
    ++db->count;
    return 1;
}

wp_s32 wp_physics_material_database_load_from_text( const wp_c8 *text,
                                                    wp_physics_material_database *db )
{
    const wp_c8 *cursor;
    wp_s32 parsed = 0;
    wp_s32 have_open = 0;
    wp_physics_material_desc current;

    if( !text || !db )
    {
        return -1;
    }

    wp_physics_material_desc_reset( &current );
    cursor = text;

    while( *cursor )
    {
        wp_c8 buf[512];
        const wp_c8 *nl;
        wp_c8 *key;
        wp_c8 *value;
        wp_size len;

        const wp_c8 *line_start = cursor;
        nl = strchr( line_start, '\n' );
        if( nl )
        {
            len = (wp_size)( nl - line_start );
            cursor = nl + 1;
        }
        else
        {
            len = strlen( line_start );
            cursor = line_start + len;
        }
        if( len >= sizeof( buf ) )
        {
            len = sizeof( buf ) - 1;
        }
        memcpy( buf, line_start, len );
        buf[len] = '\0';
        if( len > 0 && buf[len - 1] == '\r' )
        {
            buf[len - 1] = '\0';
        }
        wp_trim_inplace( buf );

        if( buf[0] == '\0' || buf[0] == '#' )
        {
            continue;
        }

        /* "material <name>" starts a new descriptor; flush the previous one. */
        if( strncmp( buf, "material", 8 ) == 0 &&
            ( buf[8] == ' ' || buf[8] == '\t' ) )
        {
            if( have_open && current.name[0] != '\0' )
            {
                if( wp_physics_material_database_add( db, &current ) )
                {
                    ++parsed;
                }
            }
            wp_physics_material_desc_reset( &current );
            wp_reg_copy_name( current.name, wp_skip_ws( buf + 8 ) );
            have_open = 1;
            continue;
        }

        /* key value */
        key = buf;
        value = strchr( buf, ' ' );
        if( !value )
        {
            value = strchr( buf, '\t' );
        }
        if( !value )
        {
            continue;
        }
        *value = '\0';
        value = (wp_c8 *)wp_skip_ws( value + 1 );

        if( !have_open )
        {
            continue;
        }

        if( wp_str_ieq( key, "static_friction" ) )
        {
            wp_f32 v = 0.0f;
            if( wp_parse_float( value, &v ) ) { current.static_friction = v; }
        }
        else if( wp_str_ieq( key, "dynamic_friction" ) )
        {
            wp_f32 v = 0.0f;
            if( wp_parse_float( value, &v ) ) { current.dynamic_friction = v; }
        }
        else if( wp_str_ieq( key, "rolling_friction" ) )
        {
            wp_f32 v = 0.0f;
            if( wp_parse_float( value, &v ) ) { current.rolling_friction = v; }
        }
        else if( wp_str_ieq( key, "restitution" ) )
        {
            wp_f32 v = 0.0f;
            if( wp_parse_float( value, &v ) ) { current.restitution = v; }
        }
        else if( wp_str_ieq( key, "friction_combine" ) )
        {
            wp_parse_friction_combine( value, &current.friction_combine_mode );
        }
        else if( wp_str_ieq( key, "restitution_combine" ) )
        {
            wp_parse_restitution_combine( value, &current.restitution_combine_mode );
        }
        /* Unknown keys are ignored to stay forwards-compatible. */
    }

    if( have_open && current.name[0] != '\0' )
    {
        if( wp_physics_material_database_add( db, &current ) )
        {
            ++parsed;
        }
    }

    return parsed;
}

/* =========================================================================
 * Registry
 * ====================================================================== */

typedef struct wp_physics_material_entry
{
    wp_c8 name[WP_PHYSICS_MATERIAL_MAX_NAME];
    wp_physics_material *material;
} wp_physics_material_entry;

struct wp_physics_material_registry
{
    wp_physics_material_entry *entries;
    wp_u32 count;
    wp_u32 capacity;
    wp_physics_material *default_material;
};

static wp_s32 wp_reg_reserve( wp_physics_material_registry *reg, wp_u32 extra )
{
    wp_u32 new_capacity;
    wp_physics_material_entry *new_mem;
    if( reg->count + extra <= reg->capacity )
    {
        return 1;
    }
    new_capacity = reg->capacity ? reg->capacity : 8;
    while( new_capacity < reg->count + extra )
    {
        new_capacity *= 2;
    }
    new_mem = (wp_physics_material_entry *)realloc( reg->entries,
                                                    sizeof( wp_physics_material_entry ) * new_capacity );
    if( !new_mem )
    {
        return 0;
    }
    reg->entries = new_mem;
    reg->capacity = new_capacity;
    return 1;
}

static wp_s32 wp_reg_find_index( wp_physics_material_registry *reg, const wp_c8 *name )
{
    wp_u32 i;
    if( !reg || !name )
    {
        return -1;
    }
    for( i = 0; i < reg->count; ++i )
    {
        if( wp_str_same( reg->entries[i].name, name ) )
        {
            return (wp_s32)i;
        }
    }
    return -1;
}

wp_physics_material_registry *wp_physics_material_registry_create( void )
{
    wp_physics_material_registry *reg = (wp_physics_material_registry *)malloc( sizeof( *reg ) );
    if( !reg )
    {
        return NULL;
    }
    memset( reg, 0, sizeof( *reg ) );
    reg->default_material = wp_physics_material_create();
    if( reg->default_material )
    {
        wp_physics_material_set_name( reg->default_material, WP_PHYSICS_MATERIAL_DEFAULT_NAME );
    }
    return reg;
}

void wp_physics_material_registry_destroy( wp_physics_material_registry *reg )
{
    wp_u32 i;
    if( !reg )
    {
        return;
    }
    for( i = 0; i < reg->count; ++i )
    {
        wp_physics_material_destroy( reg->entries[i].material );
    }
    free( reg->entries );
    wp_physics_material_destroy( reg->default_material );
    free( reg );
}

void wp_physics_material_registry_clear( wp_physics_material_registry *reg )
{
    wp_u32 i;
    if( !reg )
    {
        return;
    }
    for( i = 0; i < reg->count; ++i )
    {
        wp_physics_material_destroy( reg->entries[i].material );
    }
    reg->count = 0;
}

wp_physics_material *wp_physics_material_registry_register( wp_physics_material_registry *reg,
                                                             const wp_physics_material_desc *desc )
{
    wp_physics_material *mat;
    wp_physics_material_entry *entry;
    if( !reg || !desc || desc->name[0] == '\0' )
    {
        return NULL;
    }
    if( wp_str_same( desc->name, WP_PHYSICS_MATERIAL_DEFAULT_NAME ) )
    {
        if( reg->default_material )
        {
            wp_physics_material_set_desc( reg->default_material, desc );
            return reg->default_material;
        }
        return NULL;
    }
    if( wp_reg_find_index( reg, desc->name ) >= 0 )
    {
        return NULL; /* duplicate name */
    }
    if( !wp_reg_reserve( reg, 1 ) )
    {
        return NULL;
    }
    mat = wp_physics_material_create_from_desc( desc );
    if( !mat )
    {
        return NULL;
    }
    entry = &reg->entries[reg->count++];
    wp_reg_copy_name( entry->name, desc->name );
    entry->material = mat;
    return mat;
}

wp_u32 wp_physics_material_registry_register_database( wp_physics_material_registry *reg,
                                                        const wp_physics_material_database *db )
{
    wp_u32 i;
    wp_u32 added = 0;
    if( !reg || !db )
    {
        return 0;
    }
    for( i = 0; i < db->count; ++i )
    {
        if( wp_physics_material_registry_register( reg, &db->materials[i] ) )
        {
            ++added;
        }
    }
    return added;
}

wp_s32 wp_physics_material_registry_unregister( wp_physics_material_registry *reg, const wp_c8 *name )
{
    wp_s32 idx;
    if( !reg || !name )
    {
        return 0;
    }
    if( wp_str_same( name, WP_PHYSICS_MATERIAL_DEFAULT_NAME ) )
    {
        return 0;
    }
    idx = wp_reg_find_index( reg, name );
    if( idx < 0 )
    {
        return 0;
    }
    wp_physics_material_destroy( reg->entries[idx].material );
    if( (wp_u32)idx + 1 < reg->count )
    {
        memmove( &reg->entries[idx], &reg->entries[idx + 1],
                 sizeof( wp_physics_material_entry ) * ( reg->count - (wp_u32)idx - 1 ) );
    }
    --reg->count;
    return 1;
}

wp_physics_material *wp_physics_material_registry_get_default( wp_physics_material_registry *reg )
{
    return reg ? reg->default_material : NULL;
}

wp_physics_material *wp_physics_material_registry_get_material( wp_physics_material_registry *reg,
                                                                 const wp_c8 *name )
{
    wp_s32 idx;
    if( !reg )
    {
        return NULL;
    }
    if( !name || name[0] == '\0' )
    {
        return reg->default_material;
    }
    idx = wp_reg_find_index( reg, name );
    if( idx >= 0 )
    {
        return reg->entries[idx].material;
    }
    return reg->default_material;
}

wp_physics_material *wp_physics_material_registry_find_material( wp_physics_material_registry *reg,
                                                                 const wp_c8 *name )
{
    wp_s32 idx;
    if( !reg || !name )
    {
        return NULL;
    }
    idx = wp_reg_find_index( reg, name );
    return idx >= 0 ? reg->entries[idx].material : NULL;
}

wp_s32 wp_physics_material_registry_has_material( wp_physics_material_registry *reg, const wp_c8 *name )
{
    return wp_physics_material_registry_find_material( reg, name ) != NULL;
}

wp_u32 wp_physics_material_registry_get_count( wp_physics_material_registry *reg )
{
    wp_u32 count;
    if( !reg )
    {
        return 0;
    }
    count = reg->count;
    if( reg->default_material )
    {
        ++count;
    }
    return count;
}

wp_physics_material *wp_physics_material_registry_get_by_index( wp_physics_material_registry *reg,
                                                                 wp_u32 index, wp_c8 *out_name )
{
    if( !reg )
    {
        return NULL;
    }
    if( reg->default_material )
    {
        if( index == 0 )
        {
            if( out_name )
            {
                strcpy( out_name, wp_physics_material_get_name( reg->default_material ) );
            }
            return reg->default_material;
        }
        --index;
    }
    if( index >= reg->count )
    {
        return NULL;
    }
    if( out_name )
    {
        strcpy( out_name, reg->entries[index].name );
    }
    return reg->entries[index].material;
}
