#ifndef WORKPHONE_GRAPHICS_MESH_OGRE_READER_H
#define WORKPHONE_GRAPHICS_MESH_OGRE_READER_H

#include "workphone_graphics_mesh.h"

wp_graphics_mesh *wp_graphics_mesh_read_ogre_from_buffer( const void *data, wp_u32 size );
wp_graphics_mesh *wp_graphics_mesh_read_ogre_from_buffer_budgeted(
    const void *data, wp_u32 size, const wp_mesh_memory_callbacks *callbacks );

#endif
