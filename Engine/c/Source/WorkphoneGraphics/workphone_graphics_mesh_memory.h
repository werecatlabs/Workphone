#ifndef WORKPHONE_GRAPHICS_MESH_MEMORY_H
#define WORKPHONE_GRAPHICS_MESH_MEMORY_H
#include "workphone_graphics_mesh.h"
#include "workphone_graphics_memory_internal.h"

/* Preserve internal mesh spellings while sharing asset allocation ownership. */
#define wp_mesh_memory_header wp_graphics_memory_header
#define wp_mesh_memory_valid wp_graphics_memory_valid
#define wp_mesh_memory_allocate wp_graphics_memory_allocate
#define wp_mesh_memory_free wp_graphics_memory_free
#define wp_mesh_memory_reallocate wp_graphics_memory_reallocate

void *wp_mesh_memory_allocate_working( wp_graphics_mesh *mesh, size_t bytes );
#endif
