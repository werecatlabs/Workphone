/**
 * @file workphone_graphics_mesh_reader.h
 * @brief Binary deserialization API for wp_graphics_mesh objects.
 *
 * Auto-detects both meshes written by workphone_graphics_mesh_writer and the
 * Ogre-compatible chunked format used by the C++ MeshSerializerImpl. Shared
 * and per-submesh geometry are flattened into the C mesh representation.
 * Ogre material names are converted to material IDs with 32-bit FNV-1a.
 */

#ifndef WORKPHONE_GRAPHICS_MESH_READER_H
#define WORKPHONE_GRAPHICS_MESH_READER_H

#include <stdint.h>
#include <stdio.h>
#include "workphone_graphics_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reads a WPM1 or Workphone/Ogre mesh from a binary file.
 *
 * @param path Null-terminated path to the input file. Must not be NULL.
 * @return Newly allocated wp_graphics_mesh on success, or NULL on failure.
 *         The caller is responsible for destroying it with wp_graphics_mesh_destroy().
 */
wp_graphics_mesh *wp_graphics_mesh_read( const wp_c8 *path );

/**
 * @brief Reads a mesh from an already-open FILE stream.
 *
 * The stream must be opened in binary mode.  On return the file position has
 * advanced past the mesh data.
 *
 * @param file Open FILE stream positioned at the start of the mesh data. Must not be NULL.
 * @return Newly allocated wp_graphics_mesh on success, or NULL on failure.
 *         The caller is responsible for destroying it with wp_graphics_mesh_destroy().
 */
wp_graphics_mesh *wp_graphics_mesh_read_from_file( FILE *file );

/**
 * @brief Deserializes an auto-detected WPM1 or Workphone/Ogre mesh buffer.
 *
 * @param data Pointer to the buffer containing the serialized mesh. Must not be NULL.
 * @param size Byte length of the buffer.
 * @return Newly allocated wp_graphics_mesh on success, or NULL on failure.
 *         The caller is responsible for destroying it with wp_graphics_mesh_destroy().
 */
wp_graphics_mesh *wp_graphics_mesh_read_from_buffer( const void *data, wp_u32 size );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_MESH_READER_H */
