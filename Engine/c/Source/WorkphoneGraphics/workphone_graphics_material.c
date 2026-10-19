/** @file workphone_graphics_material.c Backend-neutral C89 material implementation. */
#include "workphone_graphics_material.h"
#include "workphone_graphics_renderer.h"
#include "workphone_graphics_shader.h"
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

struct wp_graphics_material
{
    wp_graphics_material_values values;
    wp_c8 *texture_names[WP_MATERIAL_MAX_TEXTURES];
    wp_shader_program *shader_program;
    void *native;
    wp_graphics_material_sync_func sync_func;
    void *sync_user_data;
    wp_graphics_material_bind_func bind_func;
    void *bind_user_data;
    wp_graphics_material_release_func release_func;
    void *release_user_data;
    wp_u32 revision;
    wp_s32 is_dirty;
};

static wp_f32 wp_mat_clamp( wp_f32 v, wp_f32 lo, wp_f32 hi, wp_f32 fallback )
{
    if( v != v )
        return fallback;
    if( v < lo )
        return lo;
    if( v > hi )
        return hi;
    return v;
}

static wp_f32 wp_mat_positive( wp_f32 v, wp_f32 fallback )
{
    if( v != v || v > FLT_MAX )
        return fallback;
    return v < 0.0f ? 0.0f : v;
}

static wp_colour_f wp_mat_colour( wp_colour_f v, wp_colour_f fallback )
{
    v.r = wp_mat_positive( v.r, fallback.r );
    v.g = wp_mat_positive( v.g, fallback.g );
    v.b = wp_mat_positive( v.b, fallback.b );
    v.a = wp_mat_clamp( v.a, 0.0f, 1.0f, fallback.a );
    return v;
}

static wp_s32 wp_mat_colour_eq( wp_colour_f a, wp_colour_f b )
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static void wp_mat_touch( wp_graphics_material *mat )
{
    ++mat->revision;
    if( mat->revision == 0u )
        mat->revision = 1u;
    mat->is_dirty = 1;
}

static void wp_mat_name( wp_c8 *dst, const wp_c8 *src )
{
    if( src )
    {
        strncpy( dst, src, WP_MATERIAL_MAX_NAME - 1 );
        dst[WP_MATERIAL_MAX_NAME - 1] = '\0';
    }
    else
        dst[0] = '\0';
}

void wp_graphics_material_values_init( wp_graphics_material_values *d )
{
    if( !d )
        return;
    memset( d, 0, sizeof( *d ) );
    d->struct_size = (wp_u32)sizeof( *d );
    d->material_type = WORKPHONE_MATERIAL_TYPE_STANDARD;
    d->ambient.r = d->ambient.g = d->ambient.b = 0.03f;
    d->ambient.a = 1.0f;
    d->diffuse.r = d->diffuse.g = d->diffuse.b = d->diffuse.a = 1.0f;
    d->specular.r = d->specular.g = d->specular.b = 0.04f;
    d->specular.a = 1.0f;
    d->emissive.a = 1.0f;
    d->roughness = 0.5f;
    d->normal_scale = 1.0f;
    d->occlusion_strength = 1.0f;
    d->emissive_intensity = 1.0f;
    d->alpha_cutoff = 0.5f;
    d->clearcoat_roughness = 0.1f;
    d->index_of_refraction = 1.5f;
    d->flags = WORKPHONE_MATERIAL_FLAG_LIGHTING | WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE |
               WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK;
    d->blend_mode = WORKPHONE_BLEND_MODE_NONE;
    d->cull_mode = WORKPHONE_CULL_MODE_BACK;
}

void wp_graphics_material_desc_init( wp_graphics_material_desc *d )
{
    wp_graphics_material_values values;
    if( !d ) return;
    wp_graphics_material_values_init( &values );
    memset( d, 0, sizeof( *d ) );
    memcpy( d, &values, offsetof( wp_graphics_material_desc, texture_names ) );
    d->struct_size = (wp_u32)sizeof( *d );
}

/* The public descriptor prefix is deliberately preserved without type punning. */
typedef char wp_mat_values_prefix_check[
    sizeof( wp_graphics_material_values ) == offsetof( wp_graphics_material_desc, texture_names ) ? 1 : -1 ];

static void wp_mat_sanitise( wp_graphics_material_values *out, const wp_graphics_material_values *in )
{
    wp_graphics_material_values def;
    wp_graphics_material_values_init( &def );
    *out = *in;
    out->struct_size = (wp_u32)sizeof( *out );
    if( out->material_type < WORKPHONE_MATERIAL_TYPE_STANDARD ||
        out->material_type > WORKPHONE_MATERIAL_TYPE_CUSTOM )
        out->material_type = def.material_type;
    if( out->blend_mode < WORKPHONE_BLEND_MODE_NONE || out->blend_mode > WORKPHONE_BLEND_MODE_PREMULTIPLIED )
        out->blend_mode = def.blend_mode;
    if( out->cull_mode < WORKPHONE_CULL_MODE_NONE || out->cull_mode > WORKPHONE_CULL_MODE_FRONT )
        out->cull_mode = def.cull_mode;
    out->ambient = wp_mat_colour( out->ambient, def.ambient );
    out->diffuse = wp_mat_colour( out->diffuse, def.diffuse );
    out->specular = wp_mat_colour( out->specular, def.specular );
    out->emissive = wp_mat_colour( out->emissive, def.emissive );
    out->metalness = wp_mat_clamp( out->metalness, 0.0f, 1.0f, def.metalness );
    out->roughness = wp_mat_clamp( out->roughness, 0.0f, 1.0f, def.roughness );
    out->normal_scale = wp_mat_clamp( out->normal_scale, 0.0f, 2.0f, def.normal_scale );
    out->occlusion_strength =
        wp_mat_clamp( out->occlusion_strength, 0.0f, 1.0f, def.occlusion_strength );
    out->emissive_intensity = wp_mat_positive( out->emissive_intensity, def.emissive_intensity );
    out->alpha_cutoff = wp_mat_clamp( out->alpha_cutoff, 0.0f, 1.0f, def.alpha_cutoff );
    out->clearcoat = wp_mat_clamp( out->clearcoat, 0.0f, 1.0f, def.clearcoat );
    out->clearcoat_roughness =
        wp_mat_clamp( out->clearcoat_roughness, 0.0f, 1.0f, def.clearcoat_roughness );
    out->transmission = wp_mat_clamp( out->transmission, 0.0f, 1.0f, def.transmission );
    out->index_of_refraction =
        wp_mat_clamp( out->index_of_refraction, 1.0f, 2.5f, def.index_of_refraction );
    out->flags &= WORKPHONE_MATERIAL_FLAG_ALL;
}

wp_graphics_material *wp_graphics_material_create( void )
{
    wp_graphics_material *m;
    m = (wp_graphics_material *)malloc( sizeof( *m ) );
    if( !m )
        return NULL;
    memset( m, 0, sizeof( *m ) );
    wp_graphics_material_values_init( &m->values );
    m->revision = 1u;
    m->is_dirty = 1;
    return m;
}

wp_graphics_material *wp_graphics_material_create_from_desc( const wp_graphics_material_desc *d )
{
    wp_graphics_material *m;
    m = wp_graphics_material_create();
    if( m && d && !wp_graphics_material_set_desc( m, d ) )
    {
        wp_graphics_material_destroy( m );
        return NULL;
    }
    return m;
}

wp_graphics_material *wp_graphics_material_clone( const wp_graphics_material *source )
{
    wp_graphics_material *clone;
    if( !source )
        return NULL;
    clone = wp_graphics_material_create();
    if( clone && !wp_graphics_material_copy( clone, source ) )
    { wp_graphics_material_destroy( clone ); return NULL; }
    if( clone )
        wp_graphics_material_set_shader_program( clone, source->shader_program );
    return clone;
}

void wp_graphics_material_destroy( wp_graphics_material *mat )
{
    wp_s32 i;
    if( mat ) for( i = 0; i < WP_MATERIAL_MAX_TEXTURES; ++i ) free( mat->texture_names[i] );
    if( mat && mat->shader_program )
        wp_shader_program_destroy( mat->shader_program );
    if( mat && mat->native && mat->release_func )
        mat->release_func( mat->native, mat->release_user_data );
    free( mat );
}

wp_s32 wp_graphics_material_get_values( const wp_graphics_material *mat, wp_graphics_material_values *v )
{
    wp_u32 copy_size;
    if( !mat || !v || v->struct_size < sizeof( wp_u32 ) ) return 0;
    copy_size = v->struct_size < sizeof( *v ) ? v->struct_size : (wp_u32)sizeof( *v );
    memcpy( v, &mat->values, copy_size );
    v->struct_size = (wp_u32)sizeof( *v );
    return 1;
}

wp_s32 wp_graphics_material_set_values( wp_graphics_material *mat, const wp_graphics_material_values *v )
{
    wp_graphics_material_values input, clean;
    wp_u32 copy_size;
    if( !mat || !v || v->struct_size < sizeof( wp_u32 ) ) return 0;
    wp_graphics_material_values_init( &input );
    copy_size = v->struct_size < sizeof( input ) ? v->struct_size : (wp_u32)sizeof( input );
    memcpy( &input, v, copy_size );
    wp_mat_sanitise( &clean, &input );
    if( memcmp( &mat->values, &clean, sizeof( clean ) ) )
    { mat->values = clean; wp_mat_touch( mat ); }
    return 1;
}

wp_u32 wp_graphics_material_get_storage_size( const wp_graphics_material *mat )
{
    wp_u32 bytes;
    wp_s32 i;
    if( !mat ) return 0;
    bytes = (wp_u32)sizeof( *mat );
    for( i = 0; i < WP_MATERIAL_MAX_TEXTURES; ++i )
        if( mat->texture_names[i] ) bytes += (wp_u32)strlen( mat->texture_names[i] ) + 1u;
    return bytes;
}

wp_s32 wp_graphics_material_get_desc( const wp_graphics_material *mat, wp_graphics_material_desc *d )
{
    wp_graphics_material_desc out;
    wp_u32 copy_size;
    wp_s32 i;
    if( !mat || !d || d->struct_size < sizeof( wp_u32 ) ) return 0;
    memset( &out, 0, sizeof( out ) );
    memcpy( &out, &mat->values, sizeof( mat->values ) );
    out.struct_size = (wp_u32)sizeof( out );
    for( i = 0; i < WP_MATERIAL_MAX_TEXTURES; ++i )
        wp_mat_name( out.texture_names[i], mat->texture_names[i] );
    copy_size = d->struct_size < sizeof( *d ) ? d->struct_size : (wp_u32)sizeof( *d );
    memcpy( d, &out, copy_size );
    return 1;
}

wp_s32 wp_graphics_material_set_desc( wp_graphics_material *mat, const wp_graphics_material_desc *d )
{
    wp_graphics_material_desc input;
    wp_graphics_material_values values;
    wp_c8 *candidates[WP_MATERIAL_MAX_TEXTURES];
    wp_u32 copy_size;
    wp_s32 i, changed;
    if( !mat || !d || d->struct_size < sizeof( wp_u32 ) ) return 0;
    wp_graphics_material_desc_init( &input );
    copy_size = d->struct_size < sizeof( input ) ? d->struct_size : (wp_u32)sizeof( input );
    memcpy( &input, d, copy_size );
    memset( candidates, 0, sizeof( candidates ) );
    /* Allocate the complete candidate first. Failure preserves all existing state. */
    for( i = 0; i < WP_MATERIAL_MAX_TEXTURES; ++i )
    {
        input.texture_names[i][WP_MATERIAL_MAX_NAME - 1] = '\0';
        if( strcmp( wp_graphics_material_get_texture_name( mat, i ), input.texture_names[i] ) &&
            input.texture_names[i][0] )
        {
            candidates[i] = (wp_c8 *)malloc( strlen( input.texture_names[i] ) + 1u );
            if( !candidates[i] )
            {
                for( i = 0; i < WP_MATERIAL_MAX_TEXTURES; ++i ) free( candidates[i] );
                return 0;
            }
            strcpy( candidates[i], input.texture_names[i] );
        }
    }
    memcpy( &values, &input, sizeof( values ) );
    values.struct_size = (wp_u32)sizeof( values );
    wp_graphics_material_set_values( mat, &values );
    changed = 0;
    for( i = 0; i < WP_MATERIAL_MAX_TEXTURES; ++i )
        if( strcmp( wp_graphics_material_get_texture_name( mat, i ), input.texture_names[i] ) )
        {
            free( mat->texture_names[i] );
            mat->texture_names[i] = candidates[i];
            changed = 1;
        }
    if( changed ) wp_mat_touch( mat );
    return 1;
}

wp_s32 wp_graphics_material_copy( wp_graphics_material *dst, const wp_graphics_material *src )
{
    wp_graphics_material_desc d;
    if( !dst || !src ) return 0;
    d.struct_size = (wp_u32)sizeof( d );
    if( !wp_graphics_material_get_desc( src, &d ) || !wp_graphics_material_set_desc( dst, &d ) ) return 0;
    wp_graphics_material_set_shader_program( dst, src->shader_program );
    return 1;
}

void wp_graphics_material_reset( wp_graphics_material *mat )
{
    wp_graphics_material_desc d;
    if( !mat )
        return;
    wp_graphics_material_desc_init( &d );
    wp_graphics_material_set_desc( mat, &d );
}

wp_graphics_material_type wp_graphics_material_get_type( const wp_graphics_material *mat )
{
    return mat ? mat->values.material_type : WORKPHONE_MATERIAL_TYPE_STANDARD;
}

void wp_graphics_material_set_type( wp_graphics_material *mat, wp_graphics_material_type type )
{
    if( mat && type >= WORKPHONE_MATERIAL_TYPE_STANDARD && type <= WORKPHONE_MATERIAL_TYPE_CUSTOM &&
        mat->values.material_type != type )
    {
        mat->values.material_type = type;
        wp_mat_touch( mat );
    }
}

static wp_colour_f wp_mat_zero( void )
{
    wp_colour_f c;
    memset( &c, 0, sizeof( c ) );
    return c;
}

#define WP_COLOUR_ACCESSORS( Name, Field )                                         \
    wp_colour_f wp_graphics_material_get_##Name( const wp_graphics_material *m )   \
    {                                                                              \
        return m ? m->values.Field : wp_mat_zero();                                  \
    }                                                                              \
    void wp_graphics_material_set_##Name( wp_graphics_material *m, wp_colour_f v ) \
    {                                                                              \
        wp_colour_f c;                                                             \
        if( !m )                                                                   \
            return;                                                                \
        c = wp_mat_colour( v, m->values.Field );                                     \
        if( !wp_mat_colour_eq( c, m->values.Field ) )                                \
        {                                                                          \
            m->values.Field = c;                                                     \
            wp_mat_touch( m );                                                     \
        }                                                                          \
    }

WP_COLOUR_ACCESSORS( ambient, ambient )
WP_COLOUR_ACCESSORS( diffuse, diffuse )
WP_COLOUR_ACCESSORS( specular, specular )
WP_COLOUR_ACCESSORS( emissive, emissive )

#define WP_SCALAR_ACCESSORS( Name, Field, Default, Lo, Hi )                   \
    wp_f32 wp_graphics_material_get_##Name( const wp_graphics_material *m )   \
    {                                                                         \
        return m ? m->values.Field : Default;                                   \
    }                                                                         \
    void wp_graphics_material_set_##Name( wp_graphics_material *m, wp_f32 v ) \
    {                                                                         \
        wp_f32 c;                                                             \
        if( !m )                                                              \
            return;                                                           \
        c = wp_mat_clamp( v, Lo, Hi, m->values.Field );                         \
        if( c != m->values.Field )                                              \
        {                                                                     \
            m->values.Field = c;                                                \
            wp_mat_touch( m );                                                \
        }                                                                     \
    }

WP_SCALAR_ACCESSORS( metalness, metalness, 0.0f, 0.0f, 1.0f )
WP_SCALAR_ACCESSORS( roughness, roughness, 0.5f, 0.0f, 1.0f )
WP_SCALAR_ACCESSORS( normal_scale, normal_scale, 1.0f, 0.0f, 2.0f )
WP_SCALAR_ACCESSORS( occlusion_strength, occlusion_strength, 1.0f, 0.0f, 1.0f )
WP_SCALAR_ACCESSORS( alpha_cutoff, alpha_cutoff, 0.5f, 0.0f, 1.0f )
WP_SCALAR_ACCESSORS( clearcoat, clearcoat, 0.0f, 0.0f, 1.0f )
WP_SCALAR_ACCESSORS( clearcoat_roughness, clearcoat_roughness, 0.1f, 0.0f, 1.0f )
WP_SCALAR_ACCESSORS( transmission, transmission, 0.0f, 0.0f, 1.0f )
WP_SCALAR_ACCESSORS( index_of_refraction, index_of_refraction, 1.5f, 1.0f, 2.5f )

wp_f32 wp_graphics_material_get_emissive_intensity( const wp_graphics_material *m )
{
    return m ? m->values.emissive_intensity : 1.0f;
}

void wp_graphics_material_set_emissive_intensity( wp_graphics_material *m, wp_f32 v )
{
    wp_f32 c;
    if( !m )
        return;
    c = wp_mat_positive( v, m->values.emissive_intensity );
    if( c != m->values.emissive_intensity )
    {
        m->values.emissive_intensity = c;
        wp_mat_touch( m );
    }
}

wp_u32 wp_graphics_material_get_flags( const wp_graphics_material *m )
{
    return m ? m->values.flags : 0u;
}

void wp_graphics_material_set_flags( wp_graphics_material *m, wp_u32 flags )
{
    flags &= WORKPHONE_MATERIAL_FLAG_ALL;
    if( m && m->values.flags != flags )
    {
        m->values.flags = flags;
        wp_mat_touch( m );
    }
}

void wp_graphics_material_set_flag( wp_graphics_material *m, wp_u32 flag, wp_s32 enabled )
{
    wp_u32 flags;
    if( !m )
        return;
    flags = enabled ? m->values.flags | flag : m->values.flags & ~flag;
    wp_graphics_material_set_flags( m, flags );
}

wp_s32 wp_graphics_material_has_flag( const wp_graphics_material *m, wp_u32 flag )
{
    return m ? ( m->values.flags & flag ) != 0u : 0;
}

wp_blend_mode wp_graphics_material_get_blend_mode( const wp_graphics_material *m )
{
    return m ? m->values.blend_mode : WORKPHONE_BLEND_MODE_NONE;
}

void wp_graphics_material_set_blend_mode( wp_graphics_material *m, wp_blend_mode mode )
{
    if( m && mode >= WORKPHONE_BLEND_MODE_NONE && mode <= WORKPHONE_BLEND_MODE_PREMULTIPLIED &&
        m->values.blend_mode != mode )
    {
        m->values.blend_mode = mode;
        wp_mat_touch( m );
    }
}

wp_cull_mode wp_graphics_material_get_cull_mode( const wp_graphics_material *m )
{
    return m ? m->values.cull_mode : WORKPHONE_CULL_MODE_BACK;
}

void wp_graphics_material_set_cull_mode( wp_graphics_material *m, wp_cull_mode mode )
{
    if( m && mode >= WORKPHONE_CULL_MODE_NONE && mode <= WORKPHONE_CULL_MODE_FRONT &&
        m->values.cull_mode != mode )
    {
        m->values.cull_mode = mode;
        wp_mat_touch( m );
    }
}

const wp_c8 *wp_graphics_material_get_texture_name( const wp_graphics_material *m, wp_s32 layer )
{
    return !m || layer < 0 || layer >= WP_MATERIAL_MAX_TEXTURES || !m->texture_names[layer] ?
        "" : m->texture_names[layer];
}

void wp_graphics_material_set_texture_name( wp_graphics_material *m, wp_s32 layer, const wp_c8 *name )
{
    wp_c8 clean[WP_MATERIAL_MAX_NAME];
    wp_c8 *candidate;
    if( !m || layer < 0 || layer >= WP_MATERIAL_MAX_TEXTURES ) return;
    wp_mat_name( clean, name );
    if( !strcmp( wp_graphics_material_get_texture_name( m, layer ), clean ) ) return;
    candidate = NULL;
    if( clean[0] )
    {
        candidate = (wp_c8 *)malloc( strlen( clean ) + 1u );
        if( !candidate ) return;
        strcpy( candidate, clean );
    }
    free( m->texture_names[layer] );
    m->texture_names[layer] = candidate;
    wp_mat_touch( m );
}

wp_s32 wp_graphics_material_is_dirty( const wp_graphics_material *m )
{
    return m ? m->is_dirty : 0;
}
void wp_graphics_material_mark_dirty( wp_graphics_material *m )
{
    if( m )
        wp_mat_touch( m );
}
void wp_graphics_material_clear_dirty( wp_graphics_material *m )
{
    if( m )
        m->is_dirty = 0;
}
wp_u32 wp_graphics_material_get_revision( const wp_graphics_material *m )
{
    return m ? m->revision : 0u;
}

wp_shader_program *wp_graphics_material_get_shader_program( const wp_graphics_material *m )
{
    return m ? m->shader_program : NULL;
}

void wp_graphics_material_set_shader_program( wp_graphics_material *m, wp_shader_program *program )
{
    wp_shader_program *old_program;
    if( !m || m->shader_program == program )
        return;
    if( program )
        wp_shader_program_retain( program );
    old_program = m->shader_program;
    m->shader_program = program;
    if( old_program )
        wp_shader_program_destroy( old_program );
    wp_mat_touch( m );
}

void wp_graphics_material_set_sync_func( wp_graphics_material *m, wp_graphics_material_sync_func func,
                                         void *user_data )
{
    if( !m )
        return;
    if( m->sync_func != func || m->sync_user_data != user_data )
    {
        m->sync_func = func;
        m->sync_user_data = func ? user_data : NULL;
        wp_mat_touch( m );
    }
}

void wp_graphics_material_set_bind_func( wp_graphics_material *m, wp_graphics_material_bind_func func,
                                         void *user_data )
{
    if( !m )
        return;
    if( m->bind_func != func || m->bind_user_data != user_data )
    {
        m->bind_func = func;
        m->bind_user_data = func ? user_data : NULL;
        wp_mat_touch( m );
    }
}

void wp_graphics_material_set_release_func( wp_graphics_material *m,
                                            wp_graphics_material_release_func func, void *user_data )
{
    if( !m )
        return;
    m->release_func = func;
    m->release_user_data = func ? user_data : NULL;
}

wp_s32 wp_graphics_material_update( wp_graphics_material *m )
{
    wp_u32 revision;
    if( !m )
        return 0;
    if( !m->is_dirty )
        return 1;
    if( !m->sync_func )
    {
        m->is_dirty = 0;
        return 1;
    }
    revision = m->revision;
    if( !m->sync_func( m, m->native, m->sync_user_data ) )
        return 0;
    if( m->revision == revision )
        m->is_dirty = 0;
    return 1;
}

wp_s32 wp_graphics_material_apply( wp_graphics_material *m, wp_renderer *renderer )
{
    wp_blend_mode blend;
    wp_s32 depth_write;
    if( !m || !renderer || !wp_graphics_material_update( m ) )
        return 0;
    if( m->shader_program && !wp_shader_program_bind( m->shader_program, renderer ) )
        return 0;
    if( !m->shader_program && !wp_renderer_reset_program( renderer ) )
        return 0;
    blend = m->values.blend_mode;
    depth_write = ( m->values.flags & WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE ) != 0u;
    if( ( m->values.flags & WORKPHONE_MATERIAL_FLAG_TRANSPARENT ) != 0u )
    {
        if( blend == WORKPHONE_BLEND_MODE_NONE )
            blend = WORKPHONE_BLEND_MODE_ALPHA;
        depth_write = 0;
    }
    wp_renderer_set_blend_mode( renderer, blend );
    wp_renderer_set_cull_mode( renderer, m->values.cull_mode );
    wp_renderer_set_depth_test_enabled( renderer,
                                        ( m->values.flags & WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK ) != 0u );
    wp_renderer_set_depth_write_enabled( renderer, depth_write );
    if( m->bind_func )
        return m->bind_func( m, renderer, m->native, m->bind_user_data ) != 0;
    wp_renderer_set_texture_native( renderer, NULL );
    return 1;
}

void *wp_graphics_material_get_native( const wp_graphics_material *m )
{
    return m ? m->native : NULL;
}
void wp_graphics_material_set_native( wp_graphics_material *m, void *native )
{
    if( m && m->native != native )
    {
        if( m->native && m->release_func )
            m->release_func( m->native, m->release_user_data );
        m->native = native;
        wp_mat_touch( m );
    }
}
