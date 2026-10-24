#include <workphone_graphics_animation.h>
#include <workphone_graphics_skeleton.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct budget_state { wp_u64 used, limit, calls, releases; } budget_state;
typedef struct budget_token { budget_state *owner; wp_u64 bytes; } budget_token;

static void *reserve_body( void *user, wp_u64 bytes, wp_s32 working )
{
    budget_state *state = (budget_state *)user;
    budget_token *token;
    ++state->calls;
    if( working || state->used > state->limit || bytes > state->limit - state->used ) return NULL;
    token = (budget_token *)malloc( sizeof( *token ) );
    if( !token ) return NULL;
    token->owner = state;
    token->bytes = bytes;
    state->used += bytes;
    return token;
}
static void release_body( void *reservation )
{
    budget_token *token = (budget_token *)reservation;
    token->owner->used -= token->bytes;
    ++token->owner->releases;
    free( token );
}

#define CHECK( condition ) do { if( !( condition ) ) { \
    fprintf( stderr, "animation budget contract failed at %d: %s\n", __LINE__, #condition ); return 1; } } while( 0 )

int main( void )
{
    budget_state state;
    wp_graphics_memory_callbacks policy, invalid;
    wp_animation *animation;
    wp_numeric_animation_track *track;
    wp_numeric_keyframe *key;
    wp_skeleton skeleton;
    wp_bone *bone;
    wp_u64 body_bytes, bone_bytes, calls;
    wp_f32 target;
    memset( &state, 0, sizeof( state ) );
    policy.user = &state;
    policy.reserve = reserve_body;
    policy.release = release_body;
    invalid = policy;
    invalid.release = NULL;
    CHECK( !wp_animation_create_budgeted( "denied", 1.0f, &policy ) && state.used == 0 );
    CHECK( !wp_animation_create_budgeted( "invalid", 1.0f, &invalid ) );
    state.limit = (wp_u64)-1;
    animation = wp_animation_create_budgeted( "city", 2.0f, &policy );
    CHECK( animation && state.used > sizeof( *animation ) );
    body_bytes = state.used;
    state.limit = body_bytes;
    calls = state.calls;
    target = 0.0f;
    track = wp_animation_create_numeric_track( animation, 7, &target );
    CHECK( track );
    key = wp_numeric_track_create_keyframe( track, 0.0f ); CHECK( key ); key->value = 2.0f;
    key = wp_numeric_track_create_keyframe( track, 2.0f ); CHECK( key ); key->value = 6.0f;
    wp_animation_apply( animation, 1.0f, 1.0f );
    CHECK( target == 4.0f && state.calls == calls && state.used == body_bytes );
    wp_animation_destroy( animation );
    CHECK( state.used == 0 );
    state.limit = body_bytes - 1;
    CHECK( !wp_animation_create_budgeted( "one-byte-short", 1.0f, &policy ) && state.used == 0 );
    state.limit = body_bytes;
    animation = wp_animation_create_budgeted( "exact", 1.0f, &policy ); CHECK( animation );
    wp_animation_destroy( animation ); CHECK( state.used == 0 );

    CHECK( !wp_skeleton_init_budgeted( NULL, &policy ) );
    CHECK( wp_skeleton_init_budgeted( &skeleton, &policy ) );
    skeleton.next_auto_handle = 17;
    CHECK( !wp_skeleton_init_budgeted( &skeleton, &invalid ) && skeleton.next_auto_handle == 17 );
    skeleton.next_auto_handle = 0;
    state.limit = 0;
    CHECK( !wp_skeleton_create_bone_auto( &skeleton ) && skeleton.next_auto_handle == 0 );
    CHECK( !skeleton.bone_list[0] && state.used == 0 );
    state.limit = (wp_u64)-1;
    bone = wp_skeleton_create_bone_named( &skeleton, "root" );
    CHECK( bone && bone->handle == 0 && skeleton.next_auto_handle == 1 );
    CHECK( state.used > sizeof( *bone ) );
    bone_bytes = state.used;
    state.limit = bone_bytes * 2 - 1;
    CHECK( !wp_skeleton_create_bone_auto( &skeleton ) && skeleton.next_auto_handle == 1 );
    CHECK( skeleton.bone_list[0] == bone && !skeleton.bone_list[1] && state.used == bone_bytes );
    state.limit = bone_bytes * 2;
    CHECK( wp_skeleton_create_bone_auto( &skeleton ) && skeleton.next_auto_handle == 2 );
    calls = state.calls;
    CHECK( !wp_skeleton_create_bone_named( &skeleton, "root" ) && skeleton.next_auto_handle == 2 );
    CHECK( state.calls == calls && state.used == bone_bytes * 2 );
    wp_skeleton_destroy( &skeleton );
    CHECK( state.used == 0 && state.calls == calls && skeleton.next_auto_handle == 0 );
    wp_skeleton_destroy( &skeleton ); CHECK( state.used == 0 );
    CHECK( wp_skeleton_init_budgeted( &skeleton, &skeleton.memory ) );
    state.limit = body_bytes + bone_bytes;
    animation = wp_animation_create_budgeted( "shared", 1.0f, &policy ); CHECK( animation );
    CHECK( wp_skeleton_create_bone_auto( &skeleton ) && state.used == state.limit );
    CHECK( !wp_skeleton_create_bone_auto( &skeleton ) && skeleton.next_auto_handle == 1 );
    wp_animation_destroy( animation );
    CHECK( wp_skeleton_create_bone_auto( &skeleton ) && state.used == bone_bytes * 2 );
    wp_skeleton_destroy( &skeleton ); CHECK( state.used == 0 );
    wp_skeleton_init( &skeleton );
    CHECK( wp_skeleton_create_bone( &skeleton, "explicit", 0 ) );
    bone = wp_skeleton_create_bone_auto( &skeleton ); CHECK( bone && bone->handle == 1 );
    wp_skeleton_destroy( &skeleton );
    animation = wp_animation_create( "legacy", 1.0f ); CHECK( animation ); wp_animation_destroy( animation );
    puts( "Native animation/skeleton budget contracts passed" );
    return 0;
}
