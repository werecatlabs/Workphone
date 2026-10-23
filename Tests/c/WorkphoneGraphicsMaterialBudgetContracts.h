#ifndef WORKPHONE_GRAPHICS_MATERIAL_BUDGET_CONTRACTS_H
#define WORKPHONE_GRAPHICS_MATERIAL_BUDGET_CONTRACTS_H

typedef struct mat_budget_state { wp_u64 used, limit, peak; int calls, deny_call, tokens; } mat_budget_state;
typedef struct mat_budget_token { mat_budget_state *owner; wp_u64 bytes; } mat_budget_token;
static void *mat_budget_reserve( void *user, wp_u64 bytes, wp_s32 working )
{
    mat_budget_state *state = (mat_budget_state *)user;
    mat_budget_token *token;
    ++state->calls;
    if( working || state->calls == state->deny_call || bytes > state->limit - state->used ) return NULL;
    token = (mat_budget_token *)malloc( sizeof( *token ) );
    if( !token ) return NULL;
    token->owner = state; token->bytes = bytes;
    state->used += bytes; ++state->tokens;
    if( state->used > state->peak ) state->peak = state->used;
    return token;
}
static void mat_budget_release( void *value )
{
    mat_budget_token *token = (mat_budget_token *)value;
    token->owner->used -= token->bytes; --token->owner->tokens; free( token );
}
static int material_budget_contracts( void )
{
    mat_budget_state state, destination_state;
    wp_material_memory_callbacks callbacks, destination_callbacks;
    wp_graphics_material *material, *clone, *destination;
    wp_graphics_material_desc desired;
    const wp_c8 *borrowed;
    wp_u64 resident, empty;
    wp_u32 revision;
    int i, ok = 1, allocations;
    memset( &state, 0, sizeof( state ) );
    callbacks.user = &state; callbacks.reserve = mat_budget_reserve; callbacks.release = mat_budget_release;
    allocations = allocation_calls;
    ok &= check( !wp_graphics_material_create_budgeted( &callbacks ) && allocation_calls == allocations && !state.tokens,
                 "descriptor admission precedes physical allocation" );
    state.limit = (wp_u64)-1;
    material = wp_graphics_material_create_budgeted( &callbacks );
    if( !material ) return 0;
    empty = state.used;
    ok &= check( empty == sizeof( wp_graphics_material ) + sizeof( wp_graphics_memory_header ),
                 "native object and allocation header are exactly admitted" );
    wp_graphics_material_set_texture_name( material, 0, "Last good" );
    resident = state.used; borrowed = wp_graphics_material_get_texture_name( material, 0 );
    revision = wp_graphics_material_get_revision( material );
    wp_graphics_material_desc_init( &desired );
    desired.roughness = 0.25f;
    strcpy( desired.texture_names[0], "Replacement" );
    strcpy( desired.texture_names[39], "City detail" );
    for( i = 1; i <= 3; ++i )
    {
        state.deny_call = state.calls + i;
        ok &= check( !wp_graphics_material_set_desc( material, &desired ) && state.used == resident &&
                     wp_graphics_material_get_revision( material ) == revision &&
                     wp_graphics_material_get_texture_name( material, 0 ) == borrowed &&
                     !wp_graphics_material_get_texture_name( material, 39 )[0],
                     "every denied name/table stage preserves the last descriptor and reservations" );
        state.deny_call = 0;
    }
    fail_after = 0;
    ok &= check( !wp_graphics_material_set_desc( material, &desired ) && state.used == resident,
                 "physical allocation failure returns its admitted reservation" );
    fail_after = -1;
    state.limit = resident;
    ok &= check( !wp_graphics_material_clone( material ) && state.used == resident,
                 "clones independently admit their descriptor and sparse names" );
    wp_graphics_material_set_sync_func( material, test_sync, NULL );
    ok &= check( !material->callbacks && state.used == resident &&
                 wp_graphics_material_get_revision( material ) == revision,
                 "denied optional callback storage preserves registration and revision" );
    state.limit = (wp_u64)-1;
    state.peak = state.used;
    ok &= check( wp_graphics_material_set_desc( material, &desired ) && state.peak > state.used,
                 "descriptor retry admits old/new overlap before committing" );
    clone = wp_graphics_material_clone( material );
    ok &= check( clone && state.used > resident &&
                 wp_graphics_material_get_texture_name( clone, 0 ) != wp_graphics_material_get_texture_name( material, 0 ),
                 "clone owns separate charged name allocations" );
    memset( &destination_state, 0, sizeof( destination_state ) ); destination_state.limit = (wp_u64)-1;
    destination_callbacks = callbacks; destination_callbacks.user = &destination_state;
    destination = wp_graphics_material_create_budgeted( &destination_callbacks );
    destination_state.limit = destination_state.used;
    ok &= check( destination && !wp_graphics_material_copy( destination, material ) &&
                 destination_state.used == empty && !wp_graphics_material_get_texture_name( destination, 0 )[0],
                 "copy respects destination admission without mutating either descriptor" );
    destination_state.limit = (wp_u64)-1;
    ok &= check( wp_graphics_material_copy( destination, material ), "copy retries with restored destination capacity" );
    wp_graphics_material_destroy( destination );
    wp_graphics_material_destroy( clone );
    wp_graphics_material_destroy( material );
    ok &= check( !state.used && !state.tokens && !destination_state.used && !destination_state.tokens,
                 "all descriptor, name, callback and clone reservations retire" );
    return ok;
}
#endif
