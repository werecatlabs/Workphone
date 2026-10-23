#ifndef WORKPHONE_GRAPHICS_MESH_BUDGET_CONTRACTS_H
#define WORKPHONE_GRAPHICS_MESH_BUDGET_CONTRACTS_H

typedef struct mesh_budget_test_state
{
    wp_u64 used[2], limit[2], peak[2], calls, reject_call, live;
} mesh_budget_test_state;
typedef struct mesh_budget_test_token
{
    mesh_budget_test_state *state;
    wp_u64 bytes;
    wp_s32 working;
} mesh_budget_test_token;

static void *mesh_budget_test_reserve( void *user, wp_u64 bytes, wp_s32 working )
{
    mesh_budget_test_state *state = (mesh_budget_test_state *)user;
    mesh_budget_test_token *token;
    ++state->calls;
    if( state->calls == state->reject_call || bytes > state->limit[working] - state->used[working] )
        return NULL;
    token = (mesh_budget_test_token *)malloc( sizeof( *token ) );
    if( !token ) return NULL;
    token->state = state;
    token->bytes = bytes;
    token->working = working;
    state->used[working] += bytes;
    if( state->used[working] > state->peak[working] ) state->peak[working] = state->used[working];
    ++state->live;
    return token;
}
static void mesh_budget_test_release( void *reservation )
{
    mesh_budget_test_token *token = (mesh_budget_test_token *)reservation;
    token->state->used[token->working] -= token->bytes;
    --token->state->live;
    free( token );
}
static void mesh_budget_test_reset( mesh_budget_test_state *state )
{
    memset( state, 0, sizeof( *state ) );
    state->limit[0] = state->limit[1] = (wp_u64)-1;
}

static int test_mesh_budget_lifetimes( void )
{
    const wp_graphics_mesh_vertex_p vertices[3] = { { {0,0,0} }, { {1,0,0} }, { {0,1,0} } };
    const wp_u32 indices[3] = {0,1,2}, invalid[3] = {0,1,99};
    mesh_budget_test_state state;
    struct wp_mesh_memory_callbacks callbacks;
    wp_graphics_mesh *mesh;
    const void *previous;
    const wp_vertex_pntc *lit;
    uint64_t generation;
    wp_u64 raw_bytes, populated_bytes;
    int i, ok = 1;
    mesh_budget_test_reset( &state );
    callbacks.user = &state;
    callbacks.reserve = mesh_budget_test_reserve;
    callbacks.release = mesh_budget_test_release;
    state.limit[0] = 0;
    if( wp_graphics_mesh_create_budgeted( &callbacks ) || state.live ) return 0;
    state.limit[0] = (wp_u64)-1;
    mesh = wp_graphics_mesh_create_budgeted( &callbacks );
    if( !mesh ) return 0;
    ok &= wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 3 ) &&
          wp_graphics_mesh_set_indices_u32( mesh, indices, 3 );
    raw_bytes = state.used[0];
    state.limit[0] = raw_bytes;
    ok &= !wp_graphics_mesh_prepare_lit_vertices( mesh ) && state.used[0] == raw_bytes;
    state.limit[0] = (wp_u64)-1;
    lit = wp_graphics_mesh_prepare_lit_vertices( mesh );
    ok &= lit && state.used[0] > raw_bytes && wp_graphics_mesh_prepare_lit_vertices( mesh ) == lit;
    populated_bytes = state.used[0];
    previous = wp_graphics_mesh_get_vertices( mesh );
    generation = wp_graphics_mesh_get_content_generation( mesh );
    state.limit[0] = populated_bytes;
    ok &= !wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 3 ) &&
          wp_graphics_mesh_get_vertices( mesh ) == previous &&
          wp_graphics_mesh_get_content_generation( mesh ) == generation &&
          wp_graphics_mesh_prepare_lit_vertices( mesh ) == lit && state.used[0] == populated_bytes;
    state.limit[0] = (wp_u64)-1;
    state.peak[0] = state.used[0];
    ok &= wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, previous, 3 ) &&
          state.peak[0] > populated_bytes && state.used[0] == raw_bytes;
    for( i = 0; i < 4; ++i ) ok &= wp_graphics_mesh_add_submesh( mesh, 0, 3, (wp_u32)i ) == i;
    previous = wp_graphics_mesh_get_submesh( mesh, 0 );
    populated_bytes = state.used[0];
    state.limit[0] = populated_bytes;
    ok &= wp_graphics_mesh_add_submesh( mesh, 0, 3, 4 ) < 0 &&
          wp_graphics_mesh_get_submesh_count( mesh ) == 4 &&
          wp_graphics_mesh_get_submesh( mesh, 0 ) == previous && state.used[0] == populated_bytes;
    state.limit[0] = (wp_u64)-1;
    state.peak[0] = state.used[0];
    ok &= wp_graphics_mesh_add_submesh( mesh, 0, 3, 4 ) == 4 && state.peak[0] > state.used[0];
    ok &= wp_graphics_mesh_set_indices_u32( mesh, invalid, 3 );
    raw_bytes = state.used[0];
    ok &= !wp_graphics_mesh_prepare_lit_vertices( mesh ) && state.used[0] == raw_bytes;
    ok &= wp_graphics_mesh_set_indices_u32( mesh, indices, 3 ) && wp_graphics_mesh_prepare_lit_vertices( mesh );
    wp_graphics_mesh_destroy( mesh );
    return ok && !state.used[0] && !state.used[1] && !state.live;
}

static void mesh_budget_put_u16( unsigned char **cursor, uint16_t value )
{
    memcpy( *cursor, &value, 2 );
    *cursor += 2;
}
static void mesh_budget_put_u32( unsigned char **cursor, uint32_t value )
{
    memcpy( *cursor, &value, 4 );
    *cursor += 4;
}
static void mesh_budget_put_chunk( unsigned char **cursor, uint16_t id, uint32_t size )
{
    mesh_budget_put_u16( cursor, id );
    mesh_budget_put_u32( cursor, size );
}

/* Generated Ogre fixture keeps full scratch-failure coverage independent of
 * external media: one position stream, three shared vertices and one section. */
static wp_u32 mesh_budget_ogre_fixture( unsigned char *bytes )
{
    const char version[] = "[MeshSerializer_v1.8]\n";
    const float positions[9] = {0,0,0, 1,0,0, 0,1,0};
    unsigned char *p = bytes;
    mesh_budget_put_u16( &p, 0x1000 );
    memcpy( p, version, sizeof( version ) - 1 ); p += sizeof( version ) - 1;
    mesh_budget_put_chunk( &p, 0x3000, 114 ); *p++ = 0;
    mesh_budget_put_chunk( &p, 0x5000, 84 ); mesh_budget_put_u32( &p, 3 );
    mesh_budget_put_chunk( &p, 0x5100, 22 );
    mesh_budget_put_chunk( &p, 0x5110, 16 );
    mesh_budget_put_u16( &p, 0 ); mesh_budget_put_u16( &p, 2 );
    mesh_budget_put_u16( &p, 1 ); mesh_budget_put_u16( &p, 0 ); mesh_budget_put_u16( &p, 0 );
    mesh_budget_put_chunk( &p, 0x5200, 52 ); mesh_budget_put_u16( &p, 0 ); mesh_budget_put_u16( &p, 12 );
    mesh_budget_put_chunk( &p, 0x5210, 42 ); memcpy( p, positions, sizeof( positions ) ); p += sizeof( positions );
    mesh_budget_put_chunk( &p, 0x4000, 23 ); memcpy( p, "test\n", 5 ); p += 5;
    *p++ = 1; mesh_budget_put_u32( &p, 3 ); *p++ = 0;
    mesh_budget_put_u16( &p, 0 ); mesh_budget_put_u16( &p, 1 ); mesh_budget_put_u16( &p, 2 );
    return (wp_u32)( p - bytes );
}

static int mesh_budget_decode_sweep( const void *data, wp_u32 size, int expect_working )
{
    mesh_budget_test_state state;
    wp_mesh_memory_callbacks callbacks;
    wp_graphics_mesh *mesh;
    wp_u64 call_count, rejected;
    mesh_budget_test_reset( &state );
    callbacks.user = &state; callbacks.reserve = mesh_budget_test_reserve; callbacks.release = mesh_budget_test_release;
    mesh = wp_graphics_mesh_read_from_buffer_budgeted( data, size, &callbacks );
    if( !mesh || !state.used[0] || state.used[1] || ( expect_working && !state.peak[1] ) )
    {
        wp_graphics_mesh_destroy( mesh );
        return 0;
    }
    call_count = state.calls;
    wp_graphics_mesh_destroy( mesh );
    if( state.live || state.used[0] || state.used[1] ) return 0;
    for( rejected = 1; rejected <= call_count; ++rejected )
    {
        mesh_budget_test_reset( &state ); state.reject_call = rejected;
        mesh = wp_graphics_mesh_read_from_buffer_budgeted( data, size, &callbacks );
        if( mesh ) { wp_graphics_mesh_destroy( mesh ); return 0; }
        if( state.live || state.used[0] || state.used[1] ) return 0;
    }
    return 1;
}

static int test_mesh_budget_decoding( void )
{
    unsigned char fixture[256];
    wp_graphics_mesh *source;
    void *wpm;
    wp_u32 size;
    int ok;
    size = mesh_budget_ogre_fixture( fixture );
    if( !mesh_budget_decode_sweep( fixture, size, 1 ) ) return 0;
    source = wp_graphics_mesh_read_from_buffer( fixture, size );
    if( !source || !wp_graphics_mesh_write_to_buffer( source, &wpm, &size ) )
    { wp_graphics_mesh_destroy( source ); return 0; }
    ok = mesh_budget_decode_sweep( wpm, size, 0 );
    free( wpm ); wp_graphics_mesh_destroy( source );
    return ok;
}
#endif
