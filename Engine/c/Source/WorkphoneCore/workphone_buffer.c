#include "workphone.h"
#include "workphone_prerequisites.h"

/* ==============================================================
 *
 *                          BUFFER
 *
 * ===============================================================*/
#ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
void *wp_malloc( wp_handle unused, void *old, wp_size size )
{
    WORKPHONE_UNUSED( unused );
    WORKPHONE_UNUSED( old );
    return malloc( size );
}

void wp_mfree( wp_handle unused, void *ptr )
{
    WORKPHONE_UNUSED( unused );
    free( ptr );
}
void wp_buffer_init_default( struct wp_buffer *buffer )
{
    struct wp_allocator alloc;
    alloc.userdata.ptr = 0;
    alloc.alloc = wp_malloc;
    alloc.free = wp_mfree;
    wp_buffer_init( buffer, &alloc, WORKPHONE_BUFFER_DEFAULT_INITIAL_SIZE );
}
#endif

void wp_buffer_init( struct wp_buffer *b, const struct wp_allocator *a, wp_size initial_size )
{
    WORKPHONE_ASSERT( b );
    WORKPHONE_ASSERT( a );
    WORKPHONE_ASSERT( initial_size );
    if( !b || !a || !initial_size )
        return;

    wp_zero( b, sizeof( *b ) );
    b->type = WORKPHONE_BUFFER_DYNAMIC;
    b->memory.ptr = a->alloc( a->userdata, 0, initial_size );
    b->memory.size = initial_size;
    b->size = initial_size;
    b->grow_factor = 2.0f;
    b->pool = *a;
}
void wp_buffer_init_fixed( struct wp_buffer *b, void *m, wp_size size )
{
    WORKPHONE_ASSERT( b );
    WORKPHONE_ASSERT( m );
    WORKPHONE_ASSERT( size );
    if( !b || !m || !size )
        return;

    wp_zero( b, sizeof( *b ) );
    b->type = WORKPHONE_BUFFER_FIXED;
    b->memory.ptr = m;
    b->memory.size = size;
    b->size = size;
}
WORKPHONE_LIB void *wp_buffer_align( void *unaligned, wp_size align, wp_size *alignment,
                                     enum wp_buffer_allocation_type type )
{
    void *memory = 0;
    switch( type )
    {
    default:
    case WORKPHONE_BUFFER_MAX:
    case WORKPHONE_BUFFER_FRONT:
        if( align )
        {
            memory = WORKPHONE_ALIGN_PTR( unaligned, align );
            *alignment = (wp_size)( (wp_byte *)memory - (wp_byte *)unaligned );
        }
        else
        {
            memory = unaligned;
            *alignment = 0;
        }
        break;
    case WORKPHONE_BUFFER_BACK:
        if( align )
        {
            memory = WORKPHONE_ALIGN_PTR_BACK( unaligned, align );
            *alignment = (wp_size)( (wp_byte *)unaligned - (wp_byte *)memory );
        }
        else
        {
            memory = unaligned;
            *alignment = 0;
        }
        break;
    }
    return memory;
}
WORKPHONE_LIB void *wp_buffer_realloc( struct wp_buffer *b, wp_size capacity, wp_size *size )
{
    void *temp;
    wp_size buffer_size;

    WORKPHONE_ASSERT( b );
    WORKPHONE_ASSERT( size );
    if( !b || !size || !b->pool.alloc || !b->pool.free )
        return 0;

    buffer_size = b->memory.size;
    temp = b->pool.alloc( b->pool.userdata, b->memory.ptr, capacity );
    WORKPHONE_ASSERT( temp );
    if( !temp )
        return 0;

    *size = capacity;
    if( temp != b->memory.ptr )
    {
        WORKPHONE_MEMCPY( temp, b->memory.ptr, buffer_size );
        b->pool.free( b->pool.userdata, b->memory.ptr );
    }

    if( b->size == buffer_size )
    {
        /* no back buffer so just set correct size */
        b->size = capacity;
        return temp;
    }
    else
    {
        /* copy back buffer to the end of the new buffer */
        void *dst, *src;
        wp_size back_size;
        back_size = buffer_size - b->size;
        dst = wp_ptr_add( void, temp, capacity - back_size );
        src = wp_ptr_add( void, temp, b->size );
        WORKPHONE_MEMCPY( dst, src, back_size );
        b->size = capacity - back_size;
    }
    return temp;
}

WORKPHONE_LIB void *wp_buffer_alloc( struct wp_buffer *b, enum wp_buffer_allocation_type type,
                                     wp_size size, wp_size align )
{
    wp_s32 full;
    wp_size alignment;
    void *unaligned;
    void *memory;

    WORKPHONE_ASSERT( b );
    WORKPHONE_ASSERT( size );

    if( !b || !size )
        return 0;

    b->needed += size;

    /* calculate total size with needed alignment + size */
    if( type == WORKPHONE_BUFFER_FRONT )
        unaligned = wp_ptr_add( void, b->memory.ptr, b->allocated );
    else
        unaligned = wp_ptr_add( void, b->memory.ptr, b->size - size );
    memory = wp_buffer_align( unaligned, align, &alignment, type );

    /* check if buffer has enough memory*/
    if( type == WORKPHONE_BUFFER_FRONT )
        full = ( ( b->allocated + size + alignment ) > b->size );
    else
        full = ( ( b->size - WORKPHONE_MIN( b->size, ( size + alignment ) ) ) <= b->allocated );

    if( full )
    {
        wp_size capacity;

        if( b->type != WORKPHONE_BUFFER_DYNAMIC )
            return 0;

        WORKPHONE_ASSERT( b->pool.alloc && b->pool.free );

        if( b->type != WORKPHONE_BUFFER_DYNAMIC || !b->pool.alloc || !b->pool.free )
            return 0;

        /* buffer is full so allocate bigger buffer if dynamic */
        capacity = (wp_size)( (wp_f32)b->memory.size * b->grow_factor );
        capacity = WORKPHONE_MAX( capacity, wp_round_up_pow2( (wp_u32)( b->allocated + size ) ) );
        b->memory.ptr = wp_buffer_realloc( b, capacity, &b->memory.size );
        if( !b->memory.ptr )
            return 0;

        /* align newly allocated pointer */
        if( type == WORKPHONE_BUFFER_FRONT )
            unaligned = wp_ptr_add( void, b->memory.ptr, b->allocated );
        else
            unaligned = wp_ptr_add( void, b->memory.ptr, b->size - size );
        memory = wp_buffer_align( unaligned, align, &alignment, type );
    }

    if( type == WORKPHONE_BUFFER_FRONT )
        b->allocated += size + alignment;
    else
        b->size -= ( size + alignment );

    b->needed += alignment;
    b->calls++;
    return memory;
}

void wp_buffer_push( struct wp_buffer *b, enum wp_buffer_allocation_type type, const void *memory,
                     wp_size size, wp_size align )
{
    void *mem = wp_buffer_alloc( b, type, size, align );
    if( !mem )
        return;

    WORKPHONE_MEMCPY( mem, memory, size );
}

void wp_buffer_mark( struct wp_buffer *buffer, enum wp_buffer_allocation_type type )
{
    WORKPHONE_ASSERT( buffer );
    if( !buffer )
        return;

    buffer->marker[type].active = wp_true;

    if( type == WORKPHONE_BUFFER_BACK )
        buffer->marker[type].offset = buffer->size;
    else
        buffer->marker[type].offset = buffer->allocated;
}

void wp_buffer_reset( struct wp_buffer *buffer, enum wp_buffer_allocation_type type )
{
    WORKPHONE_ASSERT( buffer );

    if( !buffer )
        return;

    if( type == WORKPHONE_BUFFER_BACK )
    {
        /* reset back buffer either back to marker or empty */
        buffer->needed -= ( buffer->memory.size - buffer->marker[type].offset );
        if( buffer->marker[type].active )
            buffer->size = buffer->marker[type].offset;
        else
            buffer->size = buffer->memory.size;
        buffer->marker[type].active = wp_false;
    }
    else
    {
        /* reset front buffer either back to back marker or empty */
        buffer->needed -= ( buffer->allocated - buffer->marker[type].offset );

        if( buffer->marker[type].active )
            buffer->allocated = buffer->marker[type].offset;
        else
            buffer->allocated = 0;

        buffer->marker[type].active = wp_false;
    }
}

void wp_buffer_clear( struct wp_buffer *b )
{
    WORKPHONE_ASSERT( b );
    if( !b )
        return;
    b->allocated = 0;
    b->size = b->memory.size;
    b->calls = 0;
    b->needed = 0;
}

void wp_buffer_free( struct wp_buffer *b )
{
    WORKPHONE_ASSERT( b );
    if( !b || !b->memory.ptr )
        return;
    if( b->type == WORKPHONE_BUFFER_FIXED )
        return;
    if( !b->pool.free )
        return;
    WORKPHONE_ASSERT( b->pool.free );
    b->pool.free( b->pool.userdata, b->memory.ptr );
}

void wp_buffer_info( struct wp_memory_status *s, const struct wp_buffer *b )
{
    WORKPHONE_ASSERT( b );
    WORKPHONE_ASSERT( s );
    if( !s || !b )
        return;
    s->allocated = b->allocated;
    s->size = b->memory.size;
    s->needed = b->needed;
    s->memory = b->memory.ptr;
    s->calls = b->calls;
}

void *wp_buffer_memory( struct wp_buffer *buffer )
{
    WORKPHONE_ASSERT( buffer );
    if( !buffer )
        return 0;

    return buffer->memory.ptr;
}

const void *wp_buffer_memory_const( const struct wp_buffer *buffer )
{
    WORKPHONE_ASSERT( buffer );
    if( !buffer )
        return 0;

    return buffer->memory.ptr;
}

wp_size wp_buffer_total( const struct wp_buffer *buffer )
{
    WORKPHONE_ASSERT( buffer );
    if( !buffer )
        return 0;

    return buffer->memory.size;
}
