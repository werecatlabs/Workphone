#include "workphone_graphics_skeleton.h"
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
    if( !self )
        return;
    self->next_auto_handle = 0;
    self->blend_state = ANIMBLEND_AVERAGE;
    self->manual_bones_dirty = 0;
    for( int i = 0; i < MAX_BONES; i++ )
    {
        self->bone_list[i] = NULL;
    }
}

/* Cleans up the skeleton and its bones.
 * Equivalent to: ~Skeleton() and unprepareImpl(). */
void wp_skeleton_destroy( wp_skeleton *self )
{
    if( !self )
        return;
    for( int i = 0; i < MAX_BONES; i++ )
    {
        if( self->bone_list[i] != NULL )
        {
            free( self->bone_list[i] );
            self->bone_list[i] = NULL;
        }
    }
}

/* Creates a bone within the skeleton.
 * Handles logic from Skeleton::createBone overloads. */
wp_bone *wp_skeleton_create_bone( wp_skeleton *self, const char *name, unsigned short handle )
{
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
        for( int i = 0; i < MAX_BONES; i++ )
        {
            if( self->bone_list[i] != NULL && strcmp( self->bone_list[i]->name, name ) == 0 )
            {
                fprintf( stderr, "Error: Bone with name %s already exists.\n", name );
                return NULL;
            }
        }
    }

    wp_bone *new_bone = (wp_bone *)malloc( sizeof( wp_bone ) );
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
    return wp_skeleton_create_bone( self, NULL, self->next_auto_handle++ );
}

wp_bone *wp_skeleton_create_bone_named( wp_skeleton *self, const char *name )
{
    /* Note: In C++ this implicitly uses the next available auto_handle */
    return wp_skeleton_create_bone( self, name, self->next_auto_handle++ );
}
