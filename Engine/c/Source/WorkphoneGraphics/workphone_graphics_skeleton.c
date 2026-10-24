#include "workphone_graphics_skeleton.h"
#include "workphone_graphics_memory_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Ported from OgreSkeleton.cpp (C++) to C89 compliant code.
 * Note: This is a simplified implementation mapping Ogre's Skeleton class logic
 * to a procedural C structure.
 */

/* Initializes the skeleton structure.
 * Equivalent to: Skeleton::Skeleton() and the parameterized constructor. */
void wp_skeleton_init( wp_skeleton *self )
{
    wp_skeleton_init_budgeted( self, NULL );
}

wp_s32 wp_skeleton_init_budgeted( wp_skeleton *self, const wp_graphics_memory_callbacks *callbacks )
{
    int i;
    wp_graphics_memory_callbacks memory;
    if( !self || !wp_graphics_memory_valid( callbacks ) ) return 0;
    /* The policy may be borrowed from this previously destroyed skeleton. */
    memset( &memory, 0, sizeof( memory ) );
    if( callbacks ) memory = *callbacks;
    self->memory = memory;
    self->next_auto_handle = 0;
    self->blend_state = ANIMBLEND_AVERAGE;
    self->manual_bones_dirty = 0;
    for( i = 0; i < MAX_BONES; i++ )
    {
        self->bone_list[i] = NULL;
    }
    return 1;
}

/* Cleans up the skeleton and its bones.
 * Equivalent to: ~Skeleton() and unprepareImpl(). */
void wp_skeleton_destroy( wp_skeleton *self )
{
    int i;
    if( !self )
        return;
    for( i = 0; i < MAX_BONES; i++ )
    {
        if( self->bone_list[i] != NULL )
        {
            wp_graphics_memory_free( self->bone_list[i] );
            self->bone_list[i] = NULL;
        }
    }
    self->next_auto_handle = 0;
}

/* Creates a bone within the skeleton.
 * Handles logic from Skeleton::createBone overloads. */
wp_bone *wp_skeleton_create_bone( wp_skeleton *self, const char *name, unsigned short handle )
{
    int i;
    wp_bone *new_bone;
    if( !self )
        return NULL;

    /* Check if provided handle is valid and not used. */
    if( handle >= MAX_BONES )
    {
        fprintf( stderr, "Error: Bone handle %d exceeds maximum allowed.\n", (int)handle );
        return NULL;
    }

    if( handle < (unsigned short)MAX_BONES && self->bone_list[handle] != NULL )
    {
        fprintf( stderr, "Error: Bone with handle %d already exists.\n", (int)handle );
        return NULL;
    }

    /* Logic corresponding to name check in mBoneListByName */
    if( name != NULL )
    {
        for( i = 0; i < MAX_BONES; i++ )
        {
            if( self->bone_list[i] != NULL && strcmp( self->bone_list[i]->name, name ) == 0 )
            {
                fprintf( stderr, "Error: Bone with name %s already exists.\n", name );
                return NULL;
            }
        }
    }

    new_bone = (wp_bone *)wp_graphics_memory_allocate(
        self->memory.reserve ? &self->memory : NULL, sizeof( wp_bone ), 0 );
    if( new_bone != NULL )
    {
        new_bone->handle = handle;
        if( name != NULL )
        {
            strncpy( new_bone->name, name, 63 );
            new_bone->name[63] = '\0';
        }
        else
        {
            strcpy( new_bone->name, "unnamed" );
        }
        self->bone_list[handle] = new_bone;
    }

    return new_bone;
}

/* Helper functions for C-style selection of overloads */
wp_bone *wp_skeleton_create_bone_auto( wp_skeleton *self )
{
    return wp_skeleton_create_bone_named( self, NULL );
}

wp_bone *wp_skeleton_create_bone_named( wp_skeleton *self, const char *name )
{
    unsigned short handle;
    wp_bone *bone;
    if( !self ) return NULL;
    handle = self->next_auto_handle;
    while( handle < MAX_BONES && self->bone_list[handle] ) ++handle;
    if( handle >= MAX_BONES ) return NULL;
    bone = wp_skeleton_create_bone( self, name, handle );
    if( bone ) self->next_auto_handle = handle + 1;
    return bone;
}
