/**
 * @file workphone_factory_mgr.c
 * @brief Implementation of the factory manager.
 */

#include "workphone_factory_mgr.h"
#include "workphone_factory.h"
#include "workphone_resource_database.h"
#include "workphone_game_prefab.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_s32 s_entries_grow( wp_factory_mgr *mgr )
{
    wp_u32 new_cap = mgr->entry_capacity + WP_FACTORY_MGR_ENTRY_GROW;
    wp_factory_entry *buf =
        (wp_factory_entry *)realloc( mgr->entries, new_cap * sizeof( wp_factory_entry ) );
    if( !buf )
        return 0;

    memset( buf + mgr->entry_capacity, 0, WP_FACTORY_MGR_ENTRY_GROW * sizeof( wp_factory_entry ) );
    mgr->entries = buf;
    mgr->entry_capacity = new_cap;
    return 1;
}

static wp_s32 s_prefabs_grow( wp_factory_mgr *mgr )
{
    wp_u32 new_cap = mgr->prefab_capacity + WP_FACTORY_MGR_PREFAB_GROW;
    wp_game_prefab **buf =
        (wp_game_prefab **)realloc( mgr->prefabs, new_cap * sizeof( wp_game_prefab * ) );
    if( !buf )
        return 0;

    memset( buf + mgr->prefab_capacity, 0, WP_FACTORY_MGR_PREFAB_GROW * sizeof( wp_game_prefab * ) );
    mgr->prefabs = buf;
    mgr->prefab_capacity = new_cap;
    return 1;
}

static wp_factory_entry *s_find_entry( const wp_factory_mgr *mgr, const wp_c8 *name )
{
    wp_u32 i;
    if( !name )
        return NULL;

    for( i = 0; i < mgr->num_entries; ++i )
    {
        if( strncmp( mgr->entries[i].name, name, WP_FACTORY_MGR_MAX_NAME ) == 0 )
            return &mgr->entries[i];
    }
    return NULL;
}

static wp_game_prefab *s_find_prefab( const wp_factory_mgr *mgr, const wp_c8 *name )
{
    wp_u32 i;
    if( !name )
        return NULL;

    for( i = 0; i < mgr->num_prefabs; ++i )
    {
        if( mgr->prefabs[i] && strncmp( mgr->prefabs[i]->name, name, WP_PREFAB_MAX_NAME ) == 0 )
            return mgr->prefabs[i];
    }
    return NULL;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_factory_mgr_init( wp_factory_mgr *mgr )
{
    if( !mgr )
        return;

    memset( mgr, 0, sizeof( *mgr ) );

    wp_resource_db_init( &mgr->res_db );
    wp_factory_init_with_db( &mgr->factory, &mgr->res_db );

    mgr->entries = NULL;
    mgr->num_entries = 0;
    mgr->entry_capacity = 0;

    mgr->prefabs = NULL;
    mgr->num_prefabs = 0;
    mgr->prefab_capacity = 0;

    mgr->user_data = NULL;
}

void wp_factory_mgr_destroy( wp_factory_mgr *mgr )
{
    if( !mgr )
        return;

    free( mgr->entries );
    mgr->entries = NULL;
    mgr->num_entries = 0;
    mgr->entry_capacity = 0;

    free( mgr->prefabs );
    mgr->prefabs = NULL;
    mgr->num_prefabs = 0;
    mgr->prefab_capacity = 0;

    wp_factory_destroy( &mgr->factory );
    wp_resource_db_destroy( &mgr->res_db );

    mgr->user_data = NULL;
}

/* =========================================================================
 * Inner accessors
 * ====================================================================== */

wp_factory *wp_factory_mgr_get_factory( wp_factory_mgr *mgr )
{
    return mgr ? &mgr->factory : NULL;
}

wp_resource_database *wp_factory_mgr_get_resource_db( wp_factory_mgr *mgr )
{
    return mgr ? &mgr->res_db : NULL;
}

/* =========================================================================
 * Actor creation
 * ====================================================================== */

wp_game_actor *wp_factory_mgr_create_actor( wp_factory_mgr *mgr )
{
    return mgr ? wp_factory_create_actor( &mgr->factory ) : NULL;
}

wp_game_actor *wp_factory_mgr_create_named_actor( wp_factory_mgr *mgr, const wp_c8 *name )
{
    return mgr ? wp_factory_create_named_actor( &mgr->factory, name ) : NULL;
}

void wp_factory_mgr_destroy_actor( wp_factory_mgr *mgr, wp_game_actor *actor )
{
    if( mgr )
        wp_factory_destroy_actor( &mgr->factory, actor );
}

/* =========================================================================
 * Component creation
 * ====================================================================== */

wp_game_component *wp_factory_mgr_create_component( wp_factory_mgr *mgr, enum wp_component_type type )
{
    return mgr ? wp_factory_create_component( &mgr->factory, type ) : NULL;
}

void wp_factory_mgr_destroy_component( wp_factory_mgr *mgr, wp_game_component *comp )
{
    if( mgr )
        wp_factory_destroy_component( &mgr->factory, comp );
}

/* =========================================================================
 * Resource creation
 * ====================================================================== */

wp_resource *wp_factory_mgr_create_resource( wp_factory_mgr *mgr, wp_resource_type type,
                                             const wp_c8 *name, const wp_c8 *path )
{
    return mgr ? wp_factory_create_resource( &mgr->factory, type, name, path ) : NULL;
}

void wp_factory_mgr_destroy_resource( wp_factory_mgr *mgr, wp_resource *resource )
{
    if( mgr )
        wp_factory_destroy_resource( &mgr->factory, resource );
}

/* =========================================================================
 * Named-creator registry
 * ====================================================================== */

wp_s32 wp_factory_mgr_register_creator( wp_factory_mgr *mgr, const wp_c8 *name,
                                        wp_factory_creator_fn creator, void *user_data )
{
    wp_factory_entry *entry;

    if( !mgr || !name || !creator )
        return 0;

    /* Replace existing entry if the name is already registered. */
    entry = s_find_entry( mgr, name );
    if( entry )
    {
        entry->creator = creator;
        entry->user_data = user_data;
        return 1;
    }

    /* Grow the array if needed. */
    if( mgr->num_entries >= mgr->entry_capacity )
    {
        if( !s_entries_grow( mgr ) )
            return 0;
    }

    entry = &mgr->entries[mgr->num_entries++];
    strncpy( entry->name, name, WP_FACTORY_MGR_MAX_NAME - 1 );
    entry->name[WP_FACTORY_MGR_MAX_NAME - 1] = '\0';
    entry->creator = creator;
    entry->user_data = user_data;
    return 1;
}

void wp_factory_mgr_unregister_creator( wp_factory_mgr *mgr, const wp_c8 *name )
{
    wp_u32 i;

    if( !mgr || !name )
        return;

    for( i = 0; i < mgr->num_entries; ++i )
    {
        if( strncmp( mgr->entries[i].name, name, WP_FACTORY_MGR_MAX_NAME ) == 0 )
        {
            /* Swap with last entry and shrink. */
            mgr->entries[i] = mgr->entries[--mgr->num_entries];
            memset( &mgr->entries[mgr->num_entries], 0, sizeof( wp_factory_entry ) );
            return;
        }
    }
}

wp_game_actor *wp_factory_mgr_create( wp_factory_mgr *mgr, const wp_c8 *name )
{
    wp_factory_entry *entry;

    if( !mgr || !name )
        return NULL;

    entry = s_find_entry( mgr, name );
    if( !entry )
        return NULL;

    return entry->creator( mgr, entry->user_data );
}

wp_s32 wp_factory_mgr_has_creator( const wp_factory_mgr *mgr, const wp_c8 *name )
{
    return ( mgr && s_find_entry( mgr, name ) ) ? 1 : 0;
}

/* =========================================================================
 * Prefab registry
 * ====================================================================== */

wp_s32 wp_factory_mgr_register_prefab( wp_factory_mgr *mgr, wp_game_prefab *prefab )
{
    if( !mgr || !prefab )
        return 0;

    /* Reject duplicates. */
    if( s_find_prefab( mgr, prefab->name ) )
        return 0;

    if( mgr->num_prefabs >= mgr->prefab_capacity )
    {
        if( !s_prefabs_grow( mgr ) )
            return 0;
    }

    mgr->prefabs[mgr->num_prefabs++] = prefab;
    return 1;
}

void wp_factory_mgr_unregister_prefab( wp_factory_mgr *mgr, const wp_c8 *name )
{
    wp_u32 i;

    if( !mgr || !name )
        return;

    for( i = 0; i < mgr->num_prefabs; ++i )
    {
        if( mgr->prefabs[i] && strncmp( mgr->prefabs[i]->name, name, WP_PREFAB_MAX_NAME ) == 0 )
        {
            /* Swap with last and shrink. */
            mgr->prefabs[i] = mgr->prefabs[--mgr->num_prefabs];
            mgr->prefabs[mgr->num_prefabs] = NULL;
            return;
        }
    }
}

wp_game_prefab *wp_factory_mgr_find_prefab( const wp_factory_mgr *mgr, const wp_c8 *name )
{
    return ( mgr && name ) ? s_find_prefab( mgr, name ) : NULL;
}

wp_game_actor *wp_factory_mgr_instantiate( wp_factory_mgr *mgr, const wp_c8 *name )
{
    wp_game_prefab *prefab;

    if( !mgr || !name )
        return NULL;

    prefab = s_find_prefab( mgr, name );
    if( !prefab )
        return NULL;

    return wp_factory_instantiate_prefab( &mgr->factory, prefab );
}

wp_u32 wp_factory_mgr_get_prefab_count( const wp_factory_mgr *mgr )
{
    return mgr ? mgr->num_prefabs : 0;
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_factory_mgr_get_user_data( const wp_factory_mgr *mgr )
{
    return mgr ? mgr->user_data : NULL;
}

void wp_factory_mgr_set_user_data( wp_factory_mgr *mgr, void *user_data )
{
    if( mgr )
        mgr->user_data = user_data;
}
