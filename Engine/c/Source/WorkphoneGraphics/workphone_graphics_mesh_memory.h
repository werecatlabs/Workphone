#ifndef WORKPHONE_GRAPHICS_MESH_MEMORY_H
#define WORKPHONE_GRAPHICS_MESH_MEMORY_H

#include "workphone_graphics_mesh.h"
#include <stdlib.h>
#include <string.h>

/* Internal render backends use the mesh's admission policy for conversion
 * scratch. Release through wp_mesh_memory_free after the upload has copied it. */
void *wp_mesh_memory_allocate_working( wp_graphics_mesh *mesh, size_t bytes );

/* Allocation-local ownership keeps decoder scratch and retained caches charged
 * through every failure path. The union preserves malloc's fundamental alignment.
 * The reservation includes this header, not just the requested payload. */
typedef union wp_mesh_memory_header
{
    struct
    {
        size_t size;
        void *reservation;
        void ( *release )( void * );
    } allocation;
    long double alignment;
    void *pointer_alignment;
} wp_mesh_memory_header;

static wp_s32 wp_mesh_memory_valid( const wp_mesh_memory_callbacks *callbacks )
{
    return !callbacks || ( callbacks->reserve && callbacks->release );
}

static void *wp_mesh_memory_allocate( const wp_mesh_memory_callbacks *callbacks,
                                     size_t bytes, wp_s32 working )
{
    wp_mesh_memory_header *header;
    void *reservation;
    size_t total;
    if( !bytes || bytes > (size_t)-1 - sizeof( *header ) || !wp_mesh_memory_valid( callbacks ) )
        return NULL;
    total = bytes + sizeof( *header );
    reservation = callbacks ? callbacks->reserve( callbacks->user, (wp_u64)total, working ) : NULL;
    if( callbacks && !reservation ) return NULL;
    header = (wp_mesh_memory_header *)malloc( total );
    if( !header )
    {
        if( reservation ) callbacks->release( reservation );
        return NULL;
    }
    header->allocation.size = bytes;
    header->allocation.reservation = reservation;
    header->allocation.release = callbacks ? callbacks->release : NULL;
    return header + 1;
}

static void wp_mesh_memory_free( void *allocation )
{
    wp_mesh_memory_header *header;
    void *reservation;
    void ( *release )( void * );
    if( !allocation ) return;
    header = (wp_mesh_memory_header *)allocation - 1;
    reservation = header->allocation.reservation;
    release = header->allocation.release;
    free( header );
    if( reservation ) release( reservation );
}

static void *wp_mesh_memory_reallocate( const wp_mesh_memory_callbacks *callbacks,
                                       void *previous, size_t bytes, wp_s32 working )
{
    void *next;
    size_t old_bytes;
    next = wp_mesh_memory_allocate( callbacks, bytes, working );
    if( !next ) return NULL;
    if( previous )
    {
        old_bytes = ( (wp_mesh_memory_header *)previous - 1 )->allocation.size;
        memcpy( next, previous, old_bytes < bytes ? old_bytes : bytes );
        wp_mesh_memory_free( previous );
    }
    return next;
}
#endif
