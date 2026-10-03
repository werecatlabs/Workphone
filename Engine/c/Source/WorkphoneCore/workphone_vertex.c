#include "workphone.h"
#include "workphone_font.h"
#include "workphone_image.h"
#include "workphone_math.h"
#include "workphone_utf8.h"
#include "workphone_vector.h"

#pragma warning( push )
#pragma warning( disable : 4116 )

#ifdef WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
void wp_draw_list_init( struct wp_draw_list *list )
{
    wp_size i = 0;
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    wp_zero( list, sizeof( *list ) );
    for( i = 0; i < WORKPHONE_LEN( list->circle_vtx ); ++i )
    {
        const wp_f32 a = ( (wp_f32)i / (wp_f32)WORKPHONE_LEN( list->circle_vtx ) ) * 2 * WORKPHONE_PI;
        list->circle_vtx[i].x = (wp_f32)wp_cos( a );
        list->circle_vtx[i].y = (wp_f32)wp_sin( a );
    }
}

void wp_draw_list_setup( struct wp_draw_list *canvas, const struct wp_convert_config *config,
                         struct wp_buffer *cmds, struct wp_buffer *vertices, struct wp_buffer *elements,
                         enum wp_anti_aliasing line_aa, enum wp_anti_aliasing shape_aa )
{
    WORKPHONE_ASSERT( canvas );
    WORKPHONE_ASSERT( config );
    WORKPHONE_ASSERT( cmds );
    WORKPHONE_ASSERT( vertices );
    WORKPHONE_ASSERT( elements );
    if( !canvas || !config || !cmds || !vertices || !elements )
        return;

    canvas->buffer = cmds;
    canvas->config = *config;
    canvas->elements = elements;
    canvas->vertices = vertices;
    canvas->line_AA = line_aa;
    canvas->shape_AA = shape_aa;
    canvas->clip_rect = wp_null_rect;

    canvas->cmd_offset = 0;
    canvas->element_count = 0;
    canvas->vertex_count = 0;
    canvas->cmd_offset = 0;
    canvas->cmd_count = 0;
    canvas->path_count = 0;
}

const struct wp_draw_command *wp__draw_list_begin( const struct wp_draw_list *canvas,
                                                   const struct wp_buffer *buffer )
{
    wp_byte *memory;
    wp_size offset;
    const struct wp_draw_command *cmd;

    WORKPHONE_ASSERT( buffer );
    if( !buffer || !buffer->size || !canvas->cmd_count )
        return 0;

    memory = (wp_byte *)buffer->memory.ptr;
    offset = buffer->memory.size - canvas->cmd_offset;
    cmd = wp_ptr_add( const struct wp_draw_command, memory, offset );
    return cmd;
}

const struct wp_draw_command *wp__draw_list_end( const struct wp_draw_list *canvas,
                                                 const struct wp_buffer *buffer )
{
    wp_size size;
    wp_size offset;
    wp_byte *memory;
    const struct wp_draw_command *end;

    WORKPHONE_ASSERT( buffer );
    WORKPHONE_ASSERT( canvas );
    if( !buffer || !canvas )
        return 0;

    memory = (wp_byte *)buffer->memory.ptr;
    size = buffer->memory.size;
    offset = size - canvas->cmd_offset;
    end = wp_ptr_add( const struct wp_draw_command, memory, offset );
    end -= ( canvas->cmd_count - 1 );
    return end;
}

const struct wp_draw_command *wp__draw_list_next( const struct wp_draw_command *cmd,
                                                  const struct wp_buffer *buffer,
                                                  const struct wp_draw_list *canvas )
{
    const struct wp_draw_command *end;
    WORKPHONE_ASSERT( buffer );
    WORKPHONE_ASSERT( canvas );
    if( !cmd || !buffer || !canvas )
        return 0;

    end = wp__draw_list_end( canvas, buffer );
    if( cmd <= end )
        return 0;
    return ( cmd - 1 );
}

struct wp_vec2f *wp_draw_list_alloc_path( struct wp_draw_list *list, wp_s32 count )
{
    struct wp_vec2f *points;
    WORKPHONE_STORAGE const wp_size point_align = sizeof( struct wp_vec2f );
    WORKPHONE_STORAGE const wp_size point_size = sizeof( struct wp_vec2f );
    points = (struct wp_vec2f *)wp_buffer_alloc( list->buffer, WORKPHONE_BUFFER_FRONT,
                                                point_size * (wp_size)count, point_align );

    if( !points )
        return 0;
    if( !list->path_offset )
    {
        void *memory = wp_buffer_memory( list->buffer );
        list->path_offset = (unsigned int)( (wp_byte *)points - (wp_byte *)memory );
    }
    list->path_count += (unsigned int)count;
    return points;
}

struct wp_vec2f wp_draw_list_path_last( struct wp_draw_list *list )
{
    void *memory;
    struct wp_vec2f *point;
    WORKPHONE_ASSERT( list->path_count );
    memory = wp_buffer_memory( list->buffer );
    point = wp_ptr_add( struct wp_vec2f, memory, list->path_offset );
    point += ( list->path_count - 1 );
    return *point;
}

struct wp_draw_command *wp_draw_list_push_command( struct wp_draw_list *list, struct wp_rect clip,
                                                   wp_handle texture )
{
    WORKPHONE_STORAGE const wp_size cmd_align = WORKPHONE_ALIGNOF( struct wp_draw_command );
    WORKPHONE_STORAGE const wp_size cmd_size = sizeof( struct wp_draw_command );
    struct wp_draw_command *cmd;

    WORKPHONE_ASSERT( list );
    cmd = (struct wp_draw_command *)wp_buffer_alloc( list->buffer, WORKPHONE_BUFFER_BACK, cmd_size,
                                                     cmd_align );

    if( !cmd )
        return 0;
    if( !list->cmd_count )
    {
        wp_byte *memory = (wp_byte *)wp_buffer_memory( list->buffer );
        wp_size total = wp_buffer_total( list->buffer );
        memory = wp_ptr_add( wp_byte, memory, total );
        list->cmd_offset = (wp_size)( memory - (wp_byte *)cmd );
    }

    cmd->elem_count = 0;
    cmd->clip_rect = clip;
    cmd->texture = texture;
#    ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    cmd->userdata = list->userdata;
#    endif

    list->cmd_count++;
    list->clip_rect = clip;
    return cmd;
}

struct wp_draw_command *wp_draw_list_command_last( struct wp_draw_list *list )
{
    void *memory;
    wp_size size;
    struct wp_draw_command *cmd;
    WORKPHONE_ASSERT( list->cmd_count );

    memory = wp_buffer_memory( list->buffer );
    size = wp_buffer_total( list->buffer );
    cmd = wp_ptr_add( struct wp_draw_command, memory, size - list->cmd_offset );
    return ( cmd - ( list->cmd_count - 1 ) );
}

void wp_draw_list_add_clip( struct wp_draw_list *list, struct wp_rect rect )
{
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    if( !list->cmd_count )
    {
        wp_draw_list_push_command( list, rect, list->config.tex_null.texture );
    }
    else
    {
        struct wp_draw_command *prev = wp_draw_list_command_last( list );
        if( prev->elem_count == 0 )
            prev->clip_rect = rect;
        wp_draw_list_push_command( list, rect, prev->texture );
    }
}

void wp_draw_list_push_image( struct wp_draw_list *list, wp_handle texture )
{
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    if( !list->cmd_count )
    {
        wp_draw_list_push_command( list, wp_null_rect, texture );
    }
    else
    {
        struct wp_draw_command *prev = wp_draw_list_command_last( list );
        if( prev->elem_count == 0 )
        {
            prev->texture = texture;
#    ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
            prev->userdata = list->userdata;
#    endif
        }
        else if( prev->texture.id != texture.id
#    ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
                 || prev->userdata.id != list->userdata.id
#    endif
        )
        {
            wp_draw_list_push_command( list, prev->clip_rect, texture );
        }
    }
}
#    ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
void wp_draw_list_push_userdata( struct wp_draw_list *list, wp_handle userdata )
{
    list->userdata = userdata;
}
#    endif

void *wp_draw_list_alloc_vertices( struct wp_draw_list *list, wp_size count )
{
    void *vtx;
    WORKPHONE_ASSERT( list );
    if( !list )
        return 0;
    vtx = wp_buffer_alloc( list->vertices, WORKPHONE_BUFFER_FRONT, list->config.vertex_size * count,
                           list->config.vertex_alignment );
    if( !vtx )
        return 0;
    list->vertex_count += (unsigned int)count;

    /* This assert triggers because your are drawing a lot of stuff and nuklear
     * defined `wp_draw_index` as `wp_u16` to safe space be default.
     *
     * So you reached the maximum number of indices or rather vertexes.
     * To solve this issue please change typedef `wp_draw_index` to `wp_u32`
     * and don't forget to specify the new element size in your drawing
     * backend (OpenGL, DirectX, ...). For example in OpenGL for `glDrawElements`
     * instead of specifying `GL_UNSIGNED_SHORT` you have to define `GL_UNSIGNED_INT`.
     * Sorry for the inconvenience. */
    if( sizeof( wp_draw_index ) == 2 )
        WORKPHONE_ASSERT( ( list->vertex_count < WORKPHONE_USHORT_MAX &&
                            "To many vertices for 16-bit vertex indices. Please read comment above on "
                            "how to solve this problem" ) );
    return vtx;
}

wp_draw_index *wp_draw_list_alloc_elements( struct wp_draw_list *list, wp_size count )
{
    wp_draw_index *ids;
    struct wp_draw_command *cmd;
    WORKPHONE_STORAGE const wp_size elem_align = WORKPHONE_ALIGNOF( wp_draw_index );
    WORKPHONE_STORAGE const wp_size elem_size = sizeof( wp_draw_index );
    WORKPHONE_ASSERT( list );
    if( !list )
        return 0;

    ids = (wp_draw_index *)wp_buffer_alloc( list->elements, WORKPHONE_BUFFER_FRONT, elem_size * count,
                                            elem_align );
    if( !ids )
        return 0;
    cmd = wp_draw_list_command_last( list );
    list->element_count += (unsigned int)count;
    cmd->elem_count += (unsigned int)count;
    return ids;
}

wp_s32 wp_draw_vertex_layout_element_is_end_of_layout(
    const struct wp_draw_vertex_layout_element *element )
{
    return ( element->attribute == WORKPHONE_VERTEX_ATTRIBUTE_COUNT ||
             element->format == WORKPHONE_FORMAT_COUNT );
}

void wp_draw_vertex_color( void *attr, const wp_f32 *vals, enum wp_draw_vertex_layout_format format )
{
    /* if this triggers you tried to provide a value format for a color */
    wp_f32 val[4];
    WORKPHONE_ASSERT( format >= WORKPHONE_FORMAT_COLOR_BEGIN );
    WORKPHONE_ASSERT( format <= WORKPHONE_FORMAT_COLOR_END );
    if( format < WORKPHONE_FORMAT_COLOR_BEGIN || format > WORKPHONE_FORMAT_COLOR_END )
        return;

    val[0] = WORKPHONE_SATURATE( vals[0] );
    val[1] = WORKPHONE_SATURATE( vals[1] );
    val[2] = WORKPHONE_SATURATE( vals[2] );
    val[3] = WORKPHONE_SATURATE( vals[3] );

    switch( format )
    {
    default:
        WORKPHONE_ASSERT( 0 && "Invalid vertex layout color format" );
        break;
    case WORKPHONE_FORMAT_R8G8B8A8:
    case WORKPHONE_FORMAT_R8G8B8:
    {
        struct wp_color col = wp_rgba_fv( val );
        WORKPHONE_MEMCPY( attr, &col.r, sizeof( col ) );
    }
    break;
    case WORKPHONE_FORMAT_B8G8R8A8:
    {
        struct wp_color col = wp_rgba_fv( val );
        struct wp_color bgra = wp_rgba( col.b, col.g, col.r, col.a );
        WORKPHONE_MEMCPY( attr, &bgra, sizeof( bgra ) );
    }
    break;
    case WORKPHONE_FORMAT_R16G15B16:
    {
        wp_u16 col[3];
        col[0] = (wp_u16)( val[0] * (wp_f32)WORKPHONE_USHORT_MAX );
        col[1] = (wp_u16)( val[1] * (wp_f32)WORKPHONE_USHORT_MAX );
        col[2] = (wp_u16)( val[2] * (wp_f32)WORKPHONE_USHORT_MAX );
        WORKPHONE_MEMCPY( attr, col, sizeof( col ) );
    }
    break;
    case WORKPHONE_FORMAT_R16G15B16A16:
    {
        wp_u16 col[4];
        col[0] = (wp_u16)( val[0] * (wp_f32)WORKPHONE_USHORT_MAX );
        col[1] = (wp_u16)( val[1] * (wp_f32)WORKPHONE_USHORT_MAX );
        col[2] = (wp_u16)( val[2] * (wp_f32)WORKPHONE_USHORT_MAX );
        col[3] = (wp_u16)( val[3] * (wp_f32)WORKPHONE_USHORT_MAX );
        WORKPHONE_MEMCPY( attr, col, sizeof( col ) );
    }
    break;
    case WORKPHONE_FORMAT_R32G32B32:
    {
        wp_u32 col[3];
        col[0] = (wp_u32)( val[0] * (wp_f32)WORKPHONE_UINT_MAX );
        col[1] = (wp_u32)( val[1] * (wp_f32)WORKPHONE_UINT_MAX );
        col[2] = (wp_u32)( val[2] * (wp_f32)WORKPHONE_UINT_MAX );
        WORKPHONE_MEMCPY( attr, col, sizeof( col ) );
    }
    break;
    case WORKPHONE_FORMAT_R32G32B32A32:
    {
        wp_u32 col[4];
        col[0] = (wp_u32)( val[0] * (wp_f32)WORKPHONE_UINT_MAX );
        col[1] = (wp_u32)( val[1] * (wp_f32)WORKPHONE_UINT_MAX );
        col[2] = (wp_u32)( val[2] * (wp_f32)WORKPHONE_UINT_MAX );
        col[3] = (wp_u32)( val[3] * (wp_f32)WORKPHONE_UINT_MAX );
        WORKPHONE_MEMCPY( attr, col, sizeof( col ) );
    }
    break;
    case WORKPHONE_FORMAT_R32G32B32A32_FLOAT:
        WORKPHONE_MEMCPY( attr, val, sizeof( wp_f32 ) * 4 );
        break;
    case WORKPHONE_FORMAT_R32G32B32A32_DOUBLE:
    {
        wp_f64 col[4];
        col[0] = (wp_f64)val[0];
        col[1] = (wp_f64)val[1];
        col[2] = (wp_f64)val[2];
        col[3] = (wp_f64)val[3];
        WORKPHONE_MEMCPY( attr, col, sizeof( col ) );
    }
    break;
    case WORKPHONE_FORMAT_RGB32:
    case WORKPHONE_FORMAT_RGBA32:
    {
        struct wp_color col = wp_rgba_fv( val );
        wp_u32 color = wp_color_u32( col );
        WORKPHONE_MEMCPY( attr, &color, sizeof( color ) );
    }
    break;
    }
}

void wp_draw_vertex_element( void *dst, const wp_f32 *values, wp_s32 value_count,
                             enum wp_draw_vertex_layout_format format )
{
    wp_s32 value_index;
    void *attribute = dst;
    /* if this triggers you tried to provide a color format for a value */
    WORKPHONE_ASSERT( format < WORKPHONE_FORMAT_COLOR_BEGIN );
    if( format >= WORKPHONE_FORMAT_COLOR_BEGIN && format <= WORKPHONE_FORMAT_COLOR_END )
        return;
    for( value_index = 0; value_index < value_count; ++value_index )
    {
        switch( format )
        {
        default:
            WORKPHONE_ASSERT( 0 && "invalid vertex layout format" );
            break;
        case WORKPHONE_FORMAT_SCHAR:
        {
            wp_c8 value = (wp_c8)WORKPHONE_CLAMP( (wp_f32)WORKPHONE_SCHAR_MIN, values[value_index],
                                                  (wp_f32)WORKPHONE_SCHAR_MAX );
            WORKPHONE_MEMCPY( attribute, &value, sizeof( value ) );
            attribute = (void *)( (wp_c8 *)attribute + sizeof( wp_c8 ) );
        }
        break;
        case WORKPHONE_FORMAT_SSHORT:
        {
            wp_s16 value = (wp_s16)WORKPHONE_CLAMP( (wp_f32)WORKPHONE_SSHORT_MIN, values[value_index],
                                                    (wp_f32)WORKPHONE_SSHORT_MAX );
            WORKPHONE_MEMCPY( attribute, &value, sizeof( value ) );
            attribute = (void *)( (wp_c8 *)attribute + sizeof( value ) );
        }
        break;
        case WORKPHONE_FORMAT_SINT:
        {
            wp_s32 value = (wp_s32)WORKPHONE_CLAMP( (wp_f32)WORKPHONE_SINT_MIN, values[value_index],
                                                    (wp_f32)WORKPHONE_SINT_MAX );
            WORKPHONE_MEMCPY( attribute, &value, sizeof( value ) );
            attribute = (void *)( (wp_c8 *)attribute + sizeof( wp_s32 ) );
        }
        break;
        case WORKPHONE_FORMAT_UCHAR:
        {
            wp_u8 value = (wp_u8)WORKPHONE_CLAMP( (wp_f32)WORKPHONE_UCHAR_MIN, values[value_index],
                                                  (wp_f32)WORKPHONE_UCHAR_MAX );
            WORKPHONE_MEMCPY( attribute, &value, sizeof( value ) );
            attribute = (void *)( (wp_c8 *)attribute + sizeof( wp_u8 ) );
        }
        break;
        case WORKPHONE_FORMAT_USHORT:
        {
            wp_u16 value = (wp_u16)WORKPHONE_CLAMP( (wp_f32)WORKPHONE_USHORT_MIN, values[value_index],
                                                    (wp_f32)WORKPHONE_USHORT_MAX );
            WORKPHONE_MEMCPY( attribute, &value, sizeof( value ) );
            attribute = (void *)( (wp_c8 *)attribute + sizeof( value ) );
        }
        break;
        case WORKPHONE_FORMAT_UINT:
        {
            wp_u32 value = (wp_u32)WORKPHONE_CLAMP( (wp_f32)WORKPHONE_UINT_MIN, values[value_index],
                                                    (wp_f32)WORKPHONE_UINT_MAX );
            WORKPHONE_MEMCPY( attribute, &value, sizeof( value ) );
            attribute = (void *)( (wp_c8 *)attribute + sizeof( wp_u32 ) );
        }
        break;
        case WORKPHONE_FORMAT_FLOAT:
            WORKPHONE_MEMCPY( attribute, &values[value_index], sizeof( values[value_index] ) );
            attribute = (void *)( (wp_c8 *)attribute + sizeof( wp_f32 ) );
            break;
        case WORKPHONE_FORMAT_DOUBLE:
        {
            wp_f64 value = (wp_f64)values[value_index];
            WORKPHONE_MEMCPY( attribute, &value, sizeof( value ) );
            attribute = (void *)( (wp_c8 *)attribute + sizeof( wp_f64 ) );
        }
        break;
        }
    }
}

void *wp_draw_vertex( void *dst, const struct wp_convert_config *config, struct wp_vec2f pos,
                      struct wp_vec2f uv, struct wp_colorf color )
{
    void *result = (void *)( (wp_c8 *)dst + config->vertex_size );
    const struct wp_draw_vertex_layout_element *elem_iter = config->vertex_layout;
    while( !wp_draw_vertex_layout_element_is_end_of_layout( elem_iter ) )
    {
        void *address = (void *)( (wp_c8 *)dst + elem_iter->offset );
        switch( elem_iter->attribute )
        {
        case WORKPHONE_VERTEX_ATTRIBUTE_COUNT:
        default:
            WORKPHONE_ASSERT( 0 && "wrong element attribute" );
            break;
        case WORKPHONE_VERTEX_POSITION:
            wp_draw_vertex_element( address, &pos.x, 2, elem_iter->format );
            break;
        case WORKPHONE_VERTEX_TEXCOORD:
            wp_draw_vertex_element( address, &uv.x, 2, elem_iter->format );
            break;
        case WORKPHONE_VERTEX_COLOR:
            wp_draw_vertex_color( address, &color.r, elem_iter->format );
            break;
        }
        elem_iter++;
    }
    return result;
}

void wp_draw_list_stroke_poly_line( struct wp_draw_list *list, const struct wp_vec2f *points,
                                    const wp_u32 points_count, struct wp_color color,
                                    enum wp_draw_list_stroke closed, wp_f32 thickness,
                                    enum wp_anti_aliasing aliasing )
{
    wp_size count;
    wp_s32 thick_line;
    struct wp_colorf col;
    struct wp_colorf col_trans;
    WORKPHONE_ASSERT( list );
    if( !list || points_count < 2 )
        return;

    color.a = (wp_byte)( (wp_f32)color.a * list->config.global_alpha );
    count = points_count;
    if( !closed )
        count = points_count - 1;
    thick_line = thickness > 1.0f;

#    ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    wp_draw_list_push_userdata( list, list->userdata );
#    endif

    color.a = (wp_byte)( (wp_f32)color.a * list->config.global_alpha );
    wp_color_fv( &col.r, color );
    col_trans = col;
    col_trans.a = 0;

    if( aliasing == WORKPHONE_ANTI_ALIASING_ON )
    {
        /* ANTI-ALIASED STROKE */
        const wp_f32 AA_SIZE = 1.0f;
        WORKPHONE_STORAGE const wp_size pnt_align = WORKPHONE_ALIGNOF( struct wp_vec2f );
        WORKPHONE_STORAGE const wp_size pnt_size = sizeof( struct wp_vec2f );

        /* allocate vertices and elements  */
        wp_size i1 = 0;
        wp_size vertex_offset;
        wp_size index = list->vertex_count;

        const wp_size idx_count = ( thick_line ) ? ( count * 18 ) : ( count * 12 );
        const wp_size vtx_count = ( thick_line ) ? ( points_count * 4 ) : ( points_count * 3 );

        void *vtx = wp_draw_list_alloc_vertices( list, vtx_count );
        wp_draw_index *ids = wp_draw_list_alloc_elements( list, idx_count );

        wp_size size;
        struct wp_vec2f *normals, *temp;
        if( !vtx || !ids )
            return;

        /* temporary allocate normals + points */
        vertex_offset = (wp_size)( (wp_byte *)vtx - (wp_byte *)list->vertices->memory.ptr );
        wp_buffer_mark( list->vertices, WORKPHONE_BUFFER_FRONT );
        size = pnt_size * ( ( thick_line ) ? 5 : 3 ) * points_count;
        normals =
            (struct wp_vec2f *)wp_buffer_alloc( list->vertices, WORKPHONE_BUFFER_FRONT, size, pnt_align );
        if( !normals )
            return;
        temp = normals + points_count;

        /* make sure vertex pointer is still correct */
        vtx = (void *)( (wp_byte *)list->vertices->memory.ptr + vertex_offset );

        /* calculate normals */
        for( i1 = 0; i1 < count; ++i1 )
        {
            const wp_size i2 = ( ( i1 + 1 ) == points_count ) ? 0 : ( i1 + 1 );
            struct wp_vec2f diff = wp_vec2_sub( points[i2], points[i1] );
            wp_f32 len;

            /* vec2 inverted length  */
            len = wp_vec2_len_sqr( diff );
            if( len != 0.0f )
                len = wp_inv_sqrt( len );
            else
                len = 1.0f;

            diff = wp_vec2_muls( diff, len );
            normals[i1].x = diff.y;
            normals[i1].y = -diff.x;
        }

        if( !closed )
            normals[points_count - 1] = normals[points_count - 2];

        if( !thick_line )
        {
            wp_size idx1, i;
            if( !closed )
            {
                struct wp_vec2f d;
                temp[0] = wp_vec2_add( points[0], wp_vec2_muls( normals[0], AA_SIZE ) );
                temp[1] = wp_vec2_sub( points[0], wp_vec2_muls( normals[0], AA_SIZE ) );
                d = wp_vec2_muls( normals[points_count - 1], AA_SIZE );
                temp[( points_count - 1 ) * 2 + 0] = wp_vec2_add( points[points_count - 1], d );
                temp[( points_count - 1 ) * 2 + 1] = wp_vec2_sub( points[points_count - 1], d );
            }

            /* fill elements */
            idx1 = index;
            for( i1 = 0; i1 < count; i1++ )
            {
                struct wp_vec2f dm;
                wp_f32 dmr2;
                wp_size i2 = ( ( i1 + 1 ) == points_count ) ? 0 : ( i1 + 1 );
                wp_size idx2 = ( ( i1 + 1 ) == points_count ) ? index : ( idx1 + 3 );

                /* average normals */
                dm = wp_vec2_muls( wp_vec2_add( normals[i1], normals[i2] ), 0.5f );
                dmr2 = dm.x * dm.x + dm.y * dm.y;
                if( dmr2 > 0.000001f )
                {
                    wp_f32 scale = 1.0f / dmr2;
                    scale = WORKPHONE_MIN( 100.0f, scale );
                    dm = wp_vec2_muls( dm, scale );
                }

                dm = wp_vec2_muls( dm, AA_SIZE );
                temp[i2 * 2 + 0] = wp_vec2_add( points[i2], dm );
                temp[i2 * 2 + 1] = wp_vec2_sub( points[i2], dm );

                ids[0] = (wp_draw_index)( idx2 + 0 );
                ids[1] = (wp_draw_index)( idx1 + 0 );
                ids[2] = (wp_draw_index)( idx1 + 2 );
                ids[3] = (wp_draw_index)( idx1 + 2 );
                ids[4] = (wp_draw_index)( idx2 + 2 );
                ids[5] = (wp_draw_index)( idx2 + 0 );
                ids[6] = (wp_draw_index)( idx2 + 1 );
                ids[7] = (wp_draw_index)( idx1 + 1 );
                ids[8] = (wp_draw_index)( idx1 + 0 );
                ids[9] = (wp_draw_index)( idx1 + 0 );
                ids[10] = (wp_draw_index)( idx2 + 0 );
                ids[11] = (wp_draw_index)( idx2 + 1 );
                ids += 12;
                idx1 = idx2;
            }

            /* fill vertices */
            for( i = 0; i < points_count; ++i )
            {
                const struct wp_vec2f uv = list->config.tex_null.uv;
                vtx = wp_draw_vertex( vtx, &list->config, points[i], uv, col );
                vtx = wp_draw_vertex( vtx, &list->config, temp[i * 2 + 0], uv, col_trans );
                vtx = wp_draw_vertex( vtx, &list->config, temp[i * 2 + 1], uv, col_trans );
            }
        }
        else
        {
            wp_size idx1, i;
            const wp_f32 half_inner_thickness = ( thickness - AA_SIZE ) * 0.5f;
            if( !closed )
            {
                struct wp_vec2f d1 = wp_vec2_muls( normals[0], half_inner_thickness + AA_SIZE );
                struct wp_vec2f d2 = wp_vec2_muls( normals[0], half_inner_thickness );

                temp[0] = wp_vec2_add( points[0], d1 );
                temp[1] = wp_vec2_add( points[0], d2 );
                temp[2] = wp_vec2_sub( points[0], d2 );
                temp[3] = wp_vec2_sub( points[0], d1 );

                d1 = wp_vec2_muls( normals[points_count - 1], half_inner_thickness + AA_SIZE );
                d2 = wp_vec2_muls( normals[points_count - 1], half_inner_thickness );

                temp[( points_count - 1 ) * 4 + 0] = wp_vec2_add( points[points_count - 1], d1 );
                temp[( points_count - 1 ) * 4 + 1] = wp_vec2_add( points[points_count - 1], d2 );
                temp[( points_count - 1 ) * 4 + 2] = wp_vec2_sub( points[points_count - 1], d2 );
                temp[( points_count - 1 ) * 4 + 3] = wp_vec2_sub( points[points_count - 1], d1 );
            }

            /* add all elements */
            idx1 = index;
            for( i1 = 0; i1 < count; ++i1 )
            {
                struct wp_vec2f dm_out, dm_in;
                const wp_size i2 = ( ( i1 + 1 ) == points_count ) ? 0 : ( i1 + 1 );
                wp_size idx2 = ( ( i1 + 1 ) == points_count ) ? index : ( idx1 + 4 );

                /* average normals */
                struct wp_vec2f dm = wp_vec2_muls( wp_vec2_add( normals[i1], normals[i2] ), 0.5f );
                wp_f32 dmr2 = dm.x * dm.x + dm.y * dm.y;
                if( dmr2 > 0.000001f )
                {
                    wp_f32 scale = 1.0f / dmr2;
                    scale = WORKPHONE_MIN( 100.0f, scale );
                    dm = wp_vec2_muls( dm, scale );
                }

                dm_out = wp_vec2_muls( dm, ( ( half_inner_thickness ) + AA_SIZE ) );
                dm_in = wp_vec2_muls( dm, half_inner_thickness );
                temp[i2 * 4 + 0] = wp_vec2_add( points[i2], dm_out );
                temp[i2 * 4 + 1] = wp_vec2_add( points[i2], dm_in );
                temp[i2 * 4 + 2] = wp_vec2_sub( points[i2], dm_in );
                temp[i2 * 4 + 3] = wp_vec2_sub( points[i2], dm_out );

                /* add indexes */
                ids[0] = (wp_draw_index)( idx2 + 1 );
                ids[1] = (wp_draw_index)( idx1 + 1 );
                ids[2] = (wp_draw_index)( idx1 + 2 );
                ids[3] = (wp_draw_index)( idx1 + 2 );
                ids[4] = (wp_draw_index)( idx2 + 2 );
                ids[5] = (wp_draw_index)( idx2 + 1 );
                ids[6] = (wp_draw_index)( idx2 + 1 );
                ids[7] = (wp_draw_index)( idx1 + 1 );
                ids[8] = (wp_draw_index)( idx1 + 0 );
                ids[9] = (wp_draw_index)( idx1 + 0 );
                ids[10] = (wp_draw_index)( idx2 + 0 );
                ids[11] = (wp_draw_index)( idx2 + 1 );
                ids[12] = (wp_draw_index)( idx2 + 2 );
                ids[13] = (wp_draw_index)( idx1 + 2 );
                ids[14] = (wp_draw_index)( idx1 + 3 );
                ids[15] = (wp_draw_index)( idx1 + 3 );
                ids[16] = (wp_draw_index)( idx2 + 3 );
                ids[17] = (wp_draw_index)( idx2 + 2 );
                ids += 18;
                idx1 = idx2;
            }

            /* add vertices */
            for( i = 0; i < points_count; ++i )
            {
                const struct wp_vec2f uv = list->config.tex_null.uv;
                vtx = wp_draw_vertex( vtx, &list->config, temp[i * 4 + 0], uv, col_trans );
                vtx = wp_draw_vertex( vtx, &list->config, temp[i * 4 + 1], uv, col );
                vtx = wp_draw_vertex( vtx, &list->config, temp[i * 4 + 2], uv, col );
                vtx = wp_draw_vertex( vtx, &list->config, temp[i * 4 + 3], uv, col_trans );
            }
        }
        /* free temporary normals + points */
        wp_buffer_reset( list->vertices, WORKPHONE_BUFFER_FRONT );
    }
    else
    {
        /* NON ANTI-ALIASED STROKE */
        wp_size i1 = 0;
        wp_size idx = list->vertex_count;
        const wp_size idx_count = count * 6;
        const wp_size vtx_count = count * 4;
        void *vtx = wp_draw_list_alloc_vertices( list, vtx_count );
        wp_draw_index *ids = wp_draw_list_alloc_elements( list, idx_count );
        if( !vtx || !ids )
            return;

        for( i1 = 0; i1 < count; ++i1 )
        {
            wp_f32 dx, dy;
            const struct wp_vec2f uv = list->config.tex_null.uv;
            const wp_size i2 = ( ( i1 + 1 ) == points_count ) ? 0 : i1 + 1;
            const struct wp_vec2f p1 = points[i1];
            const struct wp_vec2f p2 = points[i2];
            struct wp_vec2f diff = wp_vec2_sub( p2, p1 );
            wp_f32 len;

            /* vec2 inverted length  */
            len = wp_vec2_len_sqr( diff );
            if( len != 0.0f )
                len = wp_inv_sqrt( len );
            else
                len = 1.0f;
            diff = wp_vec2_muls( diff, len );

            /* add vertices */
            dx = diff.x * ( thickness * 0.5f );
            dy = diff.y * ( thickness * 0.5f );

            vtx = wp_draw_vertex( vtx, &list->config, wp_make_vec2f( p1.x + dy, p1.y - dx ), uv, col );
            vtx = wp_draw_vertex( vtx, &list->config, wp_make_vec2f( p2.x + dy, p2.y - dx ), uv, col );
            vtx = wp_draw_vertex( vtx, &list->config, wp_make_vec2f( p2.x - dy, p2.y + dx ), uv, col );
            vtx = wp_draw_vertex( vtx, &list->config, wp_make_vec2f( p1.x - dy, p1.y + dx ), uv, col );

            ids[0] = (wp_draw_index)( idx + 0 );
            ids[1] = (wp_draw_index)( idx + 1 );
            ids[2] = (wp_draw_index)( idx + 2 );
            ids[3] = (wp_draw_index)( idx + 0 );
            ids[4] = (wp_draw_index)( idx + 2 );
            ids[5] = (wp_draw_index)( idx + 3 );

            ids += 6;
            idx += 4;
        }
    }
}

void wp_draw_list_fill_poly_convex( struct wp_draw_list *list, const struct wp_vec2f *points,
                                    const wp_u32 points_count, struct wp_color color,
                                    enum wp_anti_aliasing aliasing )
{
    struct wp_colorf col;
    struct wp_colorf col_trans;

    WORKPHONE_STORAGE const wp_size pnt_align = WORKPHONE_ALIGNOF( struct wp_vec2f );
    WORKPHONE_STORAGE const wp_size pnt_size = sizeof( struct wp_vec2f );
    WORKPHONE_ASSERT( list );
    if( !list || points_count < 3 )
        return;

#    ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    wp_draw_list_push_userdata( list, list->userdata );
#    endif

    color.a = (wp_byte)( (wp_f32)color.a * list->config.global_alpha );
    wp_color_fv( &col.r, color );
    col_trans = col;
    col_trans.a = 0;

    if( aliasing == WORKPHONE_ANTI_ALIASING_ON )
    {
        wp_size i = 0;
        wp_size i0 = 0;
        wp_size i1 = 0;

        const wp_f32 AA_SIZE = 1.0f;
        wp_size vertex_offset = 0;
        wp_size index = list->vertex_count;

        const wp_size idx_count = ( points_count - 2 ) * 3 + points_count * 6;
        const wp_size vtx_count = ( points_count * 2 );

        void *vtx = wp_draw_list_alloc_vertices( list, vtx_count );
        wp_draw_index *ids = wp_draw_list_alloc_elements( list, idx_count );

        wp_size size = 0;
        struct wp_vec2f *normals = 0;
        wp_u32 vtx_inner_idx = (unsigned int)( index + 0 );
        wp_u32 vtx_outer_idx = (unsigned int)( index + 1 );
        if( !vtx || !ids )
            return;

        /* temporary allocate normals */
        vertex_offset = (wp_size)( (wp_byte *)vtx - (wp_byte *)list->vertices->memory.ptr );
        wp_buffer_mark( list->vertices, WORKPHONE_BUFFER_FRONT );
        size = pnt_size * points_count;
        normals =
            (struct wp_vec2f *)wp_buffer_alloc( list->vertices, WORKPHONE_BUFFER_FRONT, size, pnt_align );
        if( !normals )
            return;
        vtx = (void *)( (wp_byte *)list->vertices->memory.ptr + vertex_offset );

        /* add elements */
        for( i = 2; i < points_count; i++ )
        {
            ids[0] = (wp_draw_index)( vtx_inner_idx );
            ids[1] = (wp_draw_index)( vtx_inner_idx + ( ( i - 1 ) << 1 ) );
            ids[2] = (wp_draw_index)( vtx_inner_idx + ( i << 1 ) );
            ids += 3;
        }

        /* compute normals */
        for( i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++ )
        {
            struct wp_vec2f p0 = points[i0];
            struct wp_vec2f p1 = points[i1];
            struct wp_vec2f diff = wp_vec2_sub( p1, p0 );

            /* vec2 inverted length  */
            wp_f32 len = wp_vec2_len_sqr( diff );
            if( len != 0.0f )
                len = wp_inv_sqrt( len );
            else
                len = 1.0f;

            diff = wp_vec2_muls( diff, len );

            normals[i0].x = diff.y;
            normals[i0].y = -diff.x;
        }

        /* add vertices + indexes */
        for( i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++ )
        {
            const struct wp_vec2f uv = list->config.tex_null.uv;
            struct wp_vec2f n0 = normals[i0];
            struct wp_vec2f n1 = normals[i1];
            struct wp_vec2f dm = wp_vec2_muls( wp_vec2_add( n0, n1 ), 0.5f );
            wp_f32 dmr2 = dm.x * dm.x + dm.y * dm.y;
            if( dmr2 > 0.000001f )
            {
                wp_f32 scale = 1.0f / dmr2;
                scale = WORKPHONE_MIN( scale, 100.0f );
                dm = wp_vec2_muls( dm, scale );
            }
            dm = wp_vec2_muls( dm, AA_SIZE * 0.5f );

            /* add vertices */
            vtx = wp_draw_vertex( vtx, &list->config, wp_vec2_sub( points[i1], dm ), uv, col );
            vtx = wp_draw_vertex( vtx, &list->config, wp_vec2_add( points[i1], dm ), uv, col_trans );

            /* add indexes */
            ids[0] = (wp_draw_index)( vtx_inner_idx + ( i1 << 1 ) );
            ids[1] = (wp_draw_index)( vtx_inner_idx + ( i0 << 1 ) );
            ids[2] = (wp_draw_index)( vtx_outer_idx + ( i0 << 1 ) );
            ids[3] = (wp_draw_index)( vtx_outer_idx + ( i0 << 1 ) );
            ids[4] = (wp_draw_index)( vtx_outer_idx + ( i1 << 1 ) );
            ids[5] = (wp_draw_index)( vtx_inner_idx + ( i1 << 1 ) );
            ids += 6;
        }
        /* free temporary normals + points */
        wp_buffer_reset( list->vertices, WORKPHONE_BUFFER_FRONT );
    }
    else
    {
        wp_size i = 0;
        wp_size index = list->vertex_count;
        const wp_size idx_count = ( points_count - 2 ) * 3;
        const wp_size vtx_count = points_count;
        void *vtx = wp_draw_list_alloc_vertices( list, vtx_count );
        wp_draw_index *ids = wp_draw_list_alloc_elements( list, idx_count );

        if( !vtx || !ids )
            return;
        for( i = 0; i < vtx_count; ++i )
            vtx = wp_draw_vertex( vtx, &list->config, points[i], list->config.tex_null.uv, col );
        for( i = 2; i < points_count; ++i )
        {
            ids[0] = (wp_draw_index)index;
            ids[1] = (wp_draw_index)( index + i - 1 );
            ids[2] = (wp_draw_index)( index + i );
            ids += 3;
        }
    }
}

void wp_draw_list_path_clear( struct wp_draw_list *list )
{
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    wp_buffer_reset( list->buffer, WORKPHONE_BUFFER_FRONT );
    list->path_count = 0;
    list->path_offset = 0;
}

void wp_draw_list_path_line_to( struct wp_draw_list *list, struct wp_vec2f pos )
{
    struct wp_vec2f *points = 0;
    struct wp_draw_command *cmd = 0;
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    if( !list->cmd_count )
        wp_draw_list_add_clip( list, wp_null_rect );

    cmd = wp_draw_list_command_last( list );
    if( cmd && cmd->texture.ptr != list->config.tex_null.texture.ptr )
        wp_draw_list_push_image( list, list->config.tex_null.texture );

    points = wp_draw_list_alloc_path( list, 1 );
    if( !points )
        return;
    points[0] = pos;
}

void wp_draw_list_path_arc_to_fast( struct wp_draw_list *list, struct wp_vec2f center, wp_f32 radius,
                                    wp_s32 a_min, wp_s32 a_max )
{
    wp_s32 a = 0;
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    if( a_min <= a_max )
    {
        for( a = a_min; a <= a_max; a++ )
        {
            const struct wp_vec2f c = list->circle_vtx[(wp_size)a % WORKPHONE_LEN( list->circle_vtx )];
            const wp_f32 x = center.x + c.x * radius;
            const wp_f32 y = center.y + c.y * radius;
            wp_draw_list_path_line_to( list, wp_make_vec2f( x, y ) );
        }
    }
}

void wp_draw_list_path_arc_to( struct wp_draw_list *list, struct wp_vec2f center, wp_f32 radius,
                               wp_f32 a_min, wp_f32 a_max, wp_u32 segments )
{
    wp_u32 i = 0;
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    if( radius == 0.0f )
        return;

    /*  This algorithm for arc drawing relies on these two trigonometric identities[1]:
            sin(a + b) = sin(a) * cos(b) + cos(a) * sin(b)
            cos(a + b) = cos(a) * cos(b) - sin(a) * sin(b)

        Two coordinates (x, y) of a powp_s32 on a circle centered on
        the origin can be written in polar form as:
            x = r * cos(a)
            y = r * sin(a)
        where r is the radius of the circle,
            a is the angle between (x, y) and the origin.

        This allows us to rotate the coordinates around the
        origin by an angle b using the following transformation:
            x' = r * cos(a + b) = x * cos(b) - y * sin(b)
            y' = r * sin(a + b) = y * cos(b) + x * sin(b)

        [1] https://en.wikipedia.org/wiki/List_of_trigonometric_identities#Angle_sum_and_difference_identities
    */
    {
        const wp_f32 d_angle = ( a_max - a_min ) / (wp_f32)segments;
        const wp_f32 sin_d = (wp_f32)wp_sin( d_angle );
        const wp_f32 cos_d = (wp_f32)wp_cos( d_angle );

        wp_f32 cx = (wp_f32)wp_cos( a_min ) * radius;
        wp_f32 cy = (wp_f32)wp_sin( a_min ) * radius;
        for( i = 0; i <= segments; ++i )
        {
            wp_f32 new_cx, new_cy;
            const wp_f32 x = center.x + cx;
            const wp_f32 y = center.y + cy;
            wp_draw_list_path_line_to( list, wp_make_vec2f( x, y ) );

            new_cx = cx * cos_d - cy * sin_d;
            new_cy = cy * cos_d + cx * sin_d;
            cx = new_cx;
            cy = new_cy;
        }
    }
}

void wp_draw_list_path_rect_to( struct wp_draw_list *list, struct wp_vec2f a, struct wp_vec2f b,
                                wp_f32 rounding )
{
    wp_f32 r;
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    r = rounding;
    r = WORKPHONE_MIN( r, ( ( b.x - a.x ) < 0 ) ? -( b.x - a.x ) : ( b.x - a.x ) );
    r = WORKPHONE_MIN( r, ( ( b.y - a.y ) < 0 ) ? -( b.y - a.y ) : ( b.y - a.y ) );

    if( r == 0.0f )
    {
        wp_draw_list_path_line_to( list, a );
        wp_draw_list_path_line_to( list, wp_make_vec2f( b.x, a.y ) );
        wp_draw_list_path_line_to( list, b );
        wp_draw_list_path_line_to( list, wp_make_vec2f( a.x, b.y ) );
    }
    else
    {
        wp_draw_list_path_arc_to_fast( list, wp_make_vec2f( a.x + r, a.y + r ), r, 6, 9 );
        wp_draw_list_path_arc_to_fast( list, wp_make_vec2f( b.x - r, a.y + r ), r, 9, 12 );
        wp_draw_list_path_arc_to_fast( list, wp_make_vec2f( b.x - r, b.y - r ), r, 0, 3 );
        wp_draw_list_path_arc_to_fast( list, wp_make_vec2f( a.x + r, b.y - r ), r, 3, 6 );
    }
}

void wp_draw_list_path_curve_to( struct wp_draw_list *list, struct wp_vec2f p2, struct wp_vec2f p3,
                                 struct wp_vec2f p4, wp_u32 num_segments )
{
    wp_f32 t_step;
    wp_u32 i_step;
    struct wp_vec2f p1;

    WORKPHONE_ASSERT( list );
    WORKPHONE_ASSERT( list->path_count );
    if( !list || !list->path_count )
        return;
    num_segments = WORKPHONE_MAX( num_segments, 1 );

    p1 = wp_draw_list_path_last( list );
    t_step = 1.0f / (wp_f32)num_segments;
    for( i_step = 1; i_step <= num_segments; ++i_step )
    {
        wp_f32 t = t_step * (wp_f32)i_step;
        wp_f32 u = 1.0f - t;
        wp_f32 w1 = u * u * u;
        wp_f32 w2 = 3 * u * u * t;
        wp_f32 w3 = 3 * u * t * t;
        wp_f32 w4 = t * t * t;
        wp_f32 x = w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x;
        wp_f32 y = w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y;
        wp_draw_list_path_line_to( list, wp_make_vec2f( x, y ) );
    }
}

void wp_draw_list_path_fill( struct wp_draw_list *list, struct wp_color color )
{
    struct wp_vec2f *points;
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    points = (struct wp_vec2f *)wp_buffer_memory( list->buffer );
    wp_draw_list_fill_poly_convex( list, points, list->path_count, color, list->config.shape_AA );
    wp_draw_list_path_clear( list );
}

void wp_draw_list_path_stroke( struct wp_draw_list *list, struct wp_color color,
                               enum wp_draw_list_stroke closed, wp_f32 thickness )
{
    struct wp_vec2f *points;
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    points = (struct wp_vec2f *)wp_buffer_memory( list->buffer );
    wp_draw_list_stroke_poly_line( list, points, list->path_count, color, closed, thickness,
                                   list->config.line_AA );
    wp_draw_list_path_clear( list );
}

void wp_draw_list_stroke_line( struct wp_draw_list *list, struct wp_vec2f a, struct wp_vec2f b,
                               struct wp_color col, wp_f32 thickness )
{
    WORKPHONE_ASSERT( list );
    if( !list || !col.a )
        return;
    if( list->line_AA == WORKPHONE_ANTI_ALIASING_ON )
    {
        wp_draw_list_path_line_to( list, a );
        wp_draw_list_path_line_to( list, b );
    }
    else
    {
        wp_draw_list_path_line_to( list, wp_vec2_sub( a, wp_make_vec2f( 0.5f, 0.5f ) ) );
        wp_draw_list_path_line_to( list, wp_vec2_sub( b, wp_make_vec2f( 0.5f, 0.5f ) ) );
    }
    wp_draw_list_path_stroke( list, col, WORKPHONE_STROKE_OPEN, thickness );
}

void wp_draw_list_fill_rect( struct wp_draw_list *list, struct wp_rect rect, struct wp_color col,
                             wp_f32 rounding )
{
    WORKPHONE_ASSERT( list );
    if( !list || !col.a )
        return;

    if( list->line_AA == WORKPHONE_ANTI_ALIASING_ON )
    {
        wp_draw_list_path_rect_to( list, wp_make_vec2f( rect.x, rect.y ),
                                   wp_make_vec2f( rect.x + rect.w, rect.y + rect.h ), rounding );
    }
    else
    {
        wp_draw_list_path_rect_to( list, wp_make_vec2f( rect.x - 0.5f, rect.y - 0.5f ),
                                   wp_make_vec2f( rect.x + rect.w, rect.y + rect.h ), rounding );
    }
    wp_draw_list_path_fill( list, col );
}

void wp_draw_list_stroke_rect( struct wp_draw_list *list, struct wp_rect rect, struct wp_color col,
                               wp_f32 rounding, wp_f32 thickness )
{
    WORKPHONE_ASSERT( list );
    if( !list || !col.a )
        return;
    if( list->line_AA == WORKPHONE_ANTI_ALIASING_ON )
    {
        wp_draw_list_path_rect_to( list, wp_make_vec2f( rect.x, rect.y ),
                                   wp_make_vec2f( rect.x + rect.w, rect.y + rect.h ), rounding );
    }
    else
    {
        wp_draw_list_path_rect_to( list, wp_make_vec2f( rect.x - 0.5f, rect.y - 0.5f ),
                                   wp_make_vec2f( rect.x + rect.w, rect.y + rect.h ), rounding );
    }
    wp_draw_list_path_stroke( list, col, WORKPHONE_STROKE_CLOSED, thickness );
}

void wp_draw_list_fill_rect_multi_color( struct wp_draw_list *list, struct wp_rect rect,
                                         struct wp_color left, struct wp_color top,
                                         struct wp_color right, struct wp_color bottom )
{
    void *vtx;
    struct wp_colorf col_left, col_top;
    struct wp_colorf col_right, col_bottom;
    wp_draw_index *idx;
    wp_draw_index index;

    wp_color_fv( &col_left.r, left );
    wp_color_fv( &col_right.r, right );
    wp_color_fv( &col_top.r, top );
    wp_color_fv( &col_bottom.r, bottom );

    WORKPHONE_ASSERT( list );
    if( !list )
        return;

    wp_draw_list_push_image( list, list->config.tex_null.texture );
    index = (wp_draw_index)list->vertex_count;
    vtx = wp_draw_list_alloc_vertices( list, 4 );
    idx = wp_draw_list_alloc_elements( list, 6 );
    if( !vtx || !idx )
        return;

    idx[0] = (wp_draw_index)( index + 0 );
    idx[1] = (wp_draw_index)( index + 1 );
    idx[2] = (wp_draw_index)( index + 2 );
    idx[3] = (wp_draw_index)( index + 0 );
    idx[4] = (wp_draw_index)( index + 2 );
    idx[5] = (wp_draw_index)( index + 3 );

    vtx = wp_draw_vertex( vtx, &list->config, wp_make_vec2f( rect.x, rect.y ), list->config.tex_null.uv,
                          col_left );
    vtx = wp_draw_vertex( vtx, &list->config, wp_make_vec2f( rect.x + rect.w, rect.y ),
                          list->config.tex_null.uv, col_top );
    vtx = wp_draw_vertex( vtx, &list->config, wp_make_vec2f( rect.x + rect.w, rect.y + rect.h ),
                          list->config.tex_null.uv, col_right );
    vtx = wp_draw_vertex( vtx, &list->config, wp_make_vec2f( rect.x, rect.y + rect.h ),
                          list->config.tex_null.uv, col_bottom );
}
void wp_draw_list_fill_triangle( struct wp_draw_list *list, struct wp_vec2f a, struct wp_vec2f b,
                                 struct wp_vec2f c, struct wp_color col )
{
    WORKPHONE_ASSERT( list );
    if( !list || !col.a )
        return;
    wp_draw_list_path_line_to( list, a );
    wp_draw_list_path_line_to( list, b );
    wp_draw_list_path_line_to( list, c );
    wp_draw_list_path_fill( list, col );
}
void wp_draw_list_stroke_triangle( struct wp_draw_list *list, struct wp_vec2f a, struct wp_vec2f b,
                                   struct wp_vec2f c, struct wp_color col, wp_f32 thickness )
{
    WORKPHONE_ASSERT( list );
    if( !list || !col.a )
        return;
    wp_draw_list_path_line_to( list, a );
    wp_draw_list_path_line_to( list, b );
    wp_draw_list_path_line_to( list, c );
    wp_draw_list_path_stroke( list, col, WORKPHONE_STROKE_CLOSED, thickness );
}
void wp_draw_list_fill_circle( struct wp_draw_list *list, struct wp_vec2f center, wp_f32 radius,
                               struct wp_color col, wp_u32 segs )
{
    wp_f32 a_max;
    WORKPHONE_ASSERT( list );
    if( !list || !col.a )
        return;
    a_max = WORKPHONE_PI * 2.0f * ( (wp_f32)segs - 1.0f ) / (wp_f32)segs;
    wp_draw_list_path_arc_to( list, center, radius, 0.0f, a_max, segs );
    wp_draw_list_path_fill( list, col );
}

void wp_draw_list_stroke_circle( struct wp_draw_list *list, struct wp_vec2f center, wp_f32 radius,
                                 struct wp_color col, wp_u32 segs, wp_f32 thickness )
{
    wp_f32 a_max;
    WORKPHONE_ASSERT( list );
    if( !list || !col.a )
        return;
    a_max = WORKPHONE_PI * 2.0f * ( (wp_f32)segs - 1.0f ) / (wp_f32)segs;
    wp_draw_list_path_arc_to( list, center, radius, 0.0f, a_max, segs );
    wp_draw_list_path_stroke( list, col, WORKPHONE_STROKE_CLOSED, thickness );
}

void wp_draw_list_stroke_curve( struct wp_draw_list *list, struct wp_vec2f p0, struct wp_vec2f cp0,
                                struct wp_vec2f cp1, struct wp_vec2f p1, struct wp_color col,
                                wp_u32 segments, wp_f32 thickness )
{
    WORKPHONE_ASSERT( list );
    if( !list || !col.a )
        return;
    wp_draw_list_path_line_to( list, p0 );
    wp_draw_list_path_curve_to( list, cp0, cp1, p1, segments );
    wp_draw_list_path_stroke( list, col, WORKPHONE_STROKE_OPEN, thickness );
}

void wp_draw_list_push_rect_uv( struct wp_draw_list *list, struct wp_vec2f a, struct wp_vec2f c,
                                struct wp_vec2f uva, struct wp_vec2f uvc, struct wp_color color )
{
    void *vtx;
    struct wp_vec2f uvb;
    struct wp_vec2f uvd;
    struct wp_vec2f b;
    struct wp_vec2f d;

    struct wp_colorf col;
    wp_draw_index *idx;
    wp_draw_index index;
    WORKPHONE_ASSERT( list );
    if( !list )
        return;

    wp_color_fv( &col.r, color );
    uvb = wp_make_vec2f( uvc.x, uva.y );
    uvd = wp_make_vec2f( uva.x, uvc.y );
    b = wp_make_vec2f( c.x, a.y );
    d = wp_make_vec2f( a.x, c.y );

    index = (wp_draw_index)list->vertex_count;
    vtx = wp_draw_list_alloc_vertices( list, 4 );
    idx = wp_draw_list_alloc_elements( list, 6 );
    if( !vtx || !idx )
        return;

    idx[0] = (wp_draw_index)( index + 0 );
    idx[1] = (wp_draw_index)( index + 1 );
    idx[2] = (wp_draw_index)( index + 2 );
    idx[3] = (wp_draw_index)( index + 0 );
    idx[4] = (wp_draw_index)( index + 2 );
    idx[5] = (wp_draw_index)( index + 3 );

    vtx = wp_draw_vertex( vtx, &list->config, a, uva, col );
    vtx = wp_draw_vertex( vtx, &list->config, b, uvb, col );
    vtx = wp_draw_vertex( vtx, &list->config, c, uvc, col );
    vtx = wp_draw_vertex( vtx, &list->config, d, uvd, col );
}

void wp_draw_list_add_image( struct wp_draw_list *list, struct wp_image texture, struct wp_rect rect,
                             struct wp_color color )
{
    WORKPHONE_ASSERT( list );
    if( !list )
        return;
    /* push new command with given texture */
    wp_draw_list_push_image( list, texture.handle );
    if( wp_image_is_subimage( &texture ) )
    {
        /* add region inside of the texture  */
        struct wp_vec2f uv[2];
        uv[0].x = (wp_f32)texture.region[0] / (wp_f32)texture.w;
        uv[0].y = (wp_f32)texture.region[1] / (wp_f32)texture.h;
        uv[1].x = (wp_f32)( texture.region[0] + texture.region[2] ) / (wp_f32)texture.w;
        uv[1].y = (wp_f32)( texture.region[1] + texture.region[3] ) / (wp_f32)texture.h;
        wp_draw_list_push_rect_uv( list, wp_make_vec2f( rect.x, rect.y ),
                                   wp_make_vec2f( rect.x + rect.w, rect.y + rect.h ), uv[0], uv[1],
                                   color );
    }
    else
        wp_draw_list_push_rect_uv( list, wp_make_vec2f( rect.x, rect.y ),
                                   wp_make_vec2f( rect.x + rect.w, rect.y + rect.h ),
                                   wp_make_vec2f( 0.0f, 0.0f ), wp_make_vec2f( 1.0f, 1.0f ), color );
}

void wp_draw_list_add_text( struct wp_draw_list *list, const struct wp_user_font *font,
                            struct wp_rect rect, const wp_c8 *text, wp_s32 len, wp_f32 font_height,
                            struct wp_color fg )
{
    wp_f32 x = 0;
    wp_s32 text_len = 0;
    wp_rune unicode = 0;
    wp_rune next = 0;
    wp_s32 glyph_len = 0;
    wp_s32 next_glyph_len = 0;
    struct wp_user_font_glyph g;

    WORKPHONE_ASSERT( list );
    if( !list || !len || !text )
        return;
    if( !WORKPHONE_INTERSECT( rect.x, rect.y, rect.w, rect.h, list->clip_rect.x, list->clip_rect.y,
                              list->clip_rect.w, list->clip_rect.h ) )
        return;

    wp_draw_list_push_image( list, font->texture );
    x = rect.x;
    glyph_len = wp_utf_decode( text, &unicode, len );
    if( !glyph_len )
        return;

    /* draw every glyph image */
    fg.a = (wp_byte)( (wp_f32)fg.a * list->config.global_alpha );
    while( text_len < len && glyph_len )
    {
        wp_f32 gx, gy, gh, gw;
        wp_f32 wp_c8_width = 0;
        if( unicode == WORKPHONE_UTF_INVALID )
            break;

        /* query currently drawn glyph information */
        next_glyph_len = wp_utf_decode( text + text_len + glyph_len, &next, (wp_s32)len - text_len );
        font->query( font->userdata, font_height, &g, unicode,
                     ( next == WORKPHONE_UTF_INVALID ) ? '\0' : next );

        /* calculate and draw glyph drawing rectangle and image */
        gx = x + g.offset.x;
        gy = rect.y + g.offset.y;
        gw = g.width;
        gh = g.height;
        wp_c8_width = g.xadvance;
        wp_draw_list_push_rect_uv( list, wp_make_vec2f( gx, gy ), wp_make_vec2f( gx + gw, gy + gh ),
                                   g.uv[0], g.uv[1], fg );

        /* offset next glyph */
        text_len += glyph_len;
        x += wp_c8_width;
        glyph_len = next_glyph_len;
        unicode = next;
    }
}

wp_flags wp_convert( struct wp_context *ctx, struct wp_buffer *cmds, struct wp_buffer *vertices,
                     struct wp_buffer *elements, const struct wp_convert_config *config )
{
    wp_flags res = WORKPHONE_CONVERT_SUCCESS;
    const struct wp_command *cmd;
    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( cmds );
    WORKPHONE_ASSERT( vertices );
    WORKPHONE_ASSERT( elements );
    WORKPHONE_ASSERT( config );
    WORKPHONE_ASSERT( config->vertex_layout );
    WORKPHONE_ASSERT( config->vertex_size );
    if( !ctx || !cmds || !vertices || !elements || !config || !config->vertex_layout )
        return WORKPHONE_CONVERT_INVALID_PARAM;

    wp_draw_list_setup( &ctx->draw_list, config, cmds, vertices, elements, config->line_AA,
                        config->shape_AA );
    wp_foreach( cmd, ctx )
    {
#    ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
        ctx->draw_list.userdata = cmd->userdata;
#    endif
        switch( cmd->type )
        {
        case WORKPHONE_COMMAND_NOP:
            break;
        case WORKPHONE_COMMAND_SCISSOR:
        {
            const struct wp_command_scissor *s = (const struct wp_command_scissor *)cmd;
            wp_draw_list_add_clip( &ctx->draw_list, wp_make_rect( s->x, s->y, s->w, s->h ) );
        }
        break;
        case WORKPHONE_COMMAND_LINE:
        {
            const struct wp_command_line *l = (const struct wp_command_line *)cmd;
            wp_draw_list_stroke_line( &ctx->draw_list, wp_make_vec2f( l->begin.x, l->begin.y ),
                                      wp_make_vec2f( l->end.x, l->end.y ), l->color, l->line_thickness );
        }
        break;
        case WORKPHONE_COMMAND_CURVE:
        {
            const struct wp_command_curve *q = (const struct wp_command_curve *)cmd;
            wp_draw_list_stroke_curve( &ctx->draw_list, wp_make_vec2f( q->begin.x, q->begin.y ),
                                       wp_make_vec2f( q->ctrl[0].x, q->ctrl[0].y ),
                                       wp_make_vec2f( q->ctrl[1].x, q->ctrl[1].y ),
                                       wp_make_vec2f( q->end.x, q->end.y ), q->color,
                                       config->curve_segment_count, q->line_thickness );
        }
        break;
        case WORKPHONE_COMMAND_RECT:
        {
            const struct wp_command_rect *r = (const struct wp_command_rect *)cmd;
            wp_draw_list_stroke_rect( &ctx->draw_list, wp_make_rect( r->x, r->y, r->w, r->h ), r->color,
                                      (wp_f32)r->rounding, r->line_thickness );
        }
        break;
        case WORKPHONE_COMMAND_RECT_FILLED:
        {
            const struct wp_command_rect_filled *r = (const struct wp_command_rect_filled *)cmd;
            wp_draw_list_fill_rect( &ctx->draw_list, wp_make_rect( r->x, r->y, r->w, r->h ), r->color,
                                    (wp_f32)r->rounding );
        }
        break;
        case WORKPHONE_COMMAND_RECT_MULTI_COLOR:
        {
            const struct wp_command_rect_multi_color *r =
                (const struct wp_command_rect_multi_color *)cmd;
            wp_draw_list_fill_rect_multi_color( &ctx->draw_list, wp_make_rect( r->x, r->y, r->w, r->h ),
                                                r->left, r->top, r->right, r->bottom );
        }
        break;
        case WORKPHONE_COMMAND_CIRCLE:
        {
            const struct wp_command_circle *c = (const struct wp_command_circle *)cmd;
            wp_draw_list_stroke_circle(
                &ctx->draw_list,
                wp_make_vec2f( (wp_f32)c->x + (wp_f32)c->w / 2, (wp_f32)c->y + (wp_f32)c->h / 2 ),
                (wp_f32)c->w / 2, c->color, config->circle_segment_count, c->line_thickness );
        }
        break;
        case WORKPHONE_COMMAND_CIRCLE_FILLED:
        {
            const struct wp_command_circle_filled *c = (const struct wp_command_circle_filled *)cmd;
            wp_draw_list_fill_circle(
                &ctx->draw_list,
                wp_make_vec2f( (wp_f32)c->x + (wp_f32)c->w / 2, (wp_f32)c->y + (wp_f32)c->h / 2 ),
                (wp_f32)c->w / 2, c->color, config->circle_segment_count );
        }
        break;
        case WORKPHONE_COMMAND_ARC:
        {
            const struct wp_command_arc *c = (const struct wp_command_arc *)cmd;
            wp_draw_list_path_line_to( &ctx->draw_list, wp_make_vec2f( c->cx, c->cy ) );
            wp_draw_list_path_arc_to( &ctx->draw_list, wp_make_vec2f( c->cx, c->cy ), c->r, c->a[0],
                                      c->a[1], config->arc_segment_count );
            wp_draw_list_path_stroke( &ctx->draw_list, c->color, WORKPHONE_STROKE_CLOSED,
                                      c->line_thickness );
        }
        break;
        case WORKPHONE_COMMAND_ARC_FILLED:
        {
            const struct wp_command_arc_filled *c = (const struct wp_command_arc_filled *)cmd;
            wp_draw_list_path_line_to( &ctx->draw_list, wp_make_vec2f( c->cx, c->cy ) );
            wp_draw_list_path_arc_to( &ctx->draw_list, wp_make_vec2f( c->cx, c->cy ), c->r, c->a[0],
                                      c->a[1], config->arc_segment_count );
            wp_draw_list_path_fill( &ctx->draw_list, c->color );
        }
        break;
        case WORKPHONE_COMMAND_TRIANGLE:
        {
            const struct wp_command_triangle *t = (const struct wp_command_triangle *)cmd;
            wp_draw_list_stroke_triangle( &ctx->draw_list, wp_make_vec2f( t->a.x, t->a.y ),
                                          wp_make_vec2f( t->b.x, t->b.y ),
                                          wp_make_vec2f( t->c.x, t->c.y ), t->color, t->line_thickness );
        }
        break;
        case WORKPHONE_COMMAND_TRIANGLE_FILLED:
        {
            const struct wp_command_triangle_filled *t = (const struct wp_command_triangle_filled *)cmd;
            wp_draw_list_fill_triangle( &ctx->draw_list, wp_make_vec2f( t->a.x, t->a.y ),
                                        wp_make_vec2f( t->b.x, t->b.y ), wp_make_vec2f( t->c.x, t->c.y ),
                                        t->color );
        }
        break;
        case WORKPHONE_COMMAND_POLYGON:
        {
            wp_s32 i;
            const struct wp_command_polygon *p = (const struct wp_command_polygon *)cmd;
            for( i = 0; i < p->point_count; ++i )
            {
                struct wp_vec2f pnt = wp_make_vec2f( (wp_f32)p->points[i].x, (wp_f32)p->points[i].y );
                wp_draw_list_path_line_to( &ctx->draw_list, pnt );
            }
            wp_draw_list_path_stroke( &ctx->draw_list, p->color, WORKPHONE_STROKE_CLOSED,
                                      p->line_thickness );
        }
        break;
        case WORKPHONE_COMMAND_POLYGON_FILLED:
        {
            wp_s32 i;
            const struct wp_command_polygon_filled *p = (const struct wp_command_polygon_filled *)cmd;
            for( i = 0; i < p->point_count; ++i )
            {
                struct wp_vec2f pnt = wp_make_vec2f( (wp_f32)p->points[i].x, (wp_f32)p->points[i].y );
                wp_draw_list_path_line_to( &ctx->draw_list, pnt );
            }
            wp_draw_list_path_fill( &ctx->draw_list, p->color );
        }
        break;
        case WORKPHONE_COMMAND_POLYLINE:
        {
            wp_s32 i;
            const struct wp_command_polyline *p = (const struct wp_command_polyline *)cmd;
            for( i = 0; i < p->point_count; ++i )
            {
                struct wp_vec2f pnt = wp_make_vec2f( (wp_f32)p->points[i].x, (wp_f32)p->points[i].y );
                wp_draw_list_path_line_to( &ctx->draw_list, pnt );
            }
            wp_draw_list_path_stroke( &ctx->draw_list, p->color, WORKPHONE_STROKE_OPEN,
                                      p->line_thickness );
        }
        break;
        case WORKPHONE_COMMAND_TEXT:
        {
            const struct wp_command_text *t = (const struct wp_command_text *)cmd;
            wp_draw_list_add_text( &ctx->draw_list, t->font, wp_make_rect( t->x, t->y, t->w, t->h ),
                                   t->string, t->length, t->height, t->foreground );
        }
        break;
        case WORKPHONE_COMMAND_IMAGE:
        {
            const struct wp_command_image *i = (const struct wp_command_image *)cmd;
            wp_draw_list_add_image( &ctx->draw_list, i->img, wp_make_rect( i->x, i->y, i->w, i->h ),
                                    i->col );
        }
        break;
        case WORKPHONE_COMMAND_CUSTOM:
        {
            const struct wp_command_custom *c = (const struct wp_command_custom *)cmd;
            c->callback( &ctx->draw_list, c->x, c->y, c->w, c->h, c->callback_data );
        }
        break;
        default:
            break;
        }
    }
    res |= ( cmds->needed > cmds->allocated + ( cmds->memory.size - cmds->size ) )
               ? WORKPHONE_CONVERT_COMMAND_BUFFER_FULL
               : 0;
    res |= ( vertices->needed > vertices->allocated ) ? WORKPHONE_CONVERT_VERTEX_BUFFER_FULL : 0;
    res |= ( elements->needed > elements->allocated ) ? WORKPHONE_CONVERT_ELEMENT_BUFFER_FULL : 0;
    return res;
}

const struct wp_draw_command *wp__draw_begin( const struct wp_context *ctx,
                                              const struct wp_buffer *buffer )
{
    return wp__draw_list_begin( &ctx->draw_list, buffer );
}

const struct wp_draw_command *wp__draw_end( const struct wp_context *ctx,
                                            const struct wp_buffer *buffer )
{
    return wp__draw_list_end( &ctx->draw_list, buffer );
}

const struct wp_draw_command *wp__draw_next( const struct wp_draw_command *cmd,
                                             const struct wp_buffer *buffer,
                                             const struct wp_context *ctx )
{
    return wp__draw_list_next( cmd, buffer, &ctx->draw_list );
}

#endif

#pragma warning( pop )
