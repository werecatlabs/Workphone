/**
 * @file workphone_game_prefab.c
 * @brief Implementation of the C game prefab API.
 */

#include "workphone_game_prefab.h"

#include <string.h>

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_game_prefab_init( wp_game_prefab *prefab )
{
    memset( prefab, 0, sizeof( *prefab ) );
    prefab->name[0] = '\0';
    prefab->actor = NULL;
    prefab->data = NULL;
    prefab->create_fn = NULL;
    prefab->create_fn_user_data = NULL;
}

void wp_game_prefab_destroy( wp_game_prefab *prefab )
{
    prefab->actor = NULL;
    prefab->data = NULL;
    prefab->create_fn = NULL;
    prefab->create_fn_user_data = NULL;
}

/* =========================================================================
 * Name
 * ====================================================================== */

void wp_game_prefab_set_name( wp_game_prefab *prefab, const wp_c8 *name )
{
    if( name )
    {
        strncpy( prefab->name, name, WP_PREFAB_MAX_NAME - 1 );
        prefab->name[WP_PREFAB_MAX_NAME - 1] = '\0';
    }
    else
    {
        prefab->name[0] = '\0';
    }
}

const wp_c8 *wp_game_prefab_get_name( const wp_game_prefab *prefab )
{
    return prefab->name;
}

/* =========================================================================
 * Actor
 * ====================================================================== */

struct wp_game_actor *wp_game_prefab_get_actor( const wp_game_prefab *prefab )
{
    return prefab->actor;
}

void wp_game_prefab_set_actor( wp_game_prefab *prefab, struct wp_game_actor *actor )
{
    prefab->actor = actor;
}

/* =========================================================================
 * Data
 * ====================================================================== */

void *wp_game_prefab_get_data( const wp_game_prefab *prefab )
{
    return prefab->data;
}

void wp_game_prefab_set_data( wp_game_prefab *prefab, void *data )
{
    prefab->data = data;
}

/* =========================================================================
 * Instantiation
 * ====================================================================== */

void wp_game_prefab_set_create_fn( wp_game_prefab *prefab, wp_prefab_create_actor_fn fn,
                                   void *user_data )
{
    prefab->create_fn = fn;
    prefab->create_fn_user_data = user_data;
}

struct wp_game_actor *wp_game_prefab_create_actor( wp_game_prefab *prefab )
{
    if( prefab->create_fn )
        return prefab->create_fn( prefab, prefab->create_fn_user_data );
    return NULL;
}
