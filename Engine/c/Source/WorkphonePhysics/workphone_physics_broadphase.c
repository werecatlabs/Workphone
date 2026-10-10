/** Shared, persistent dynamic AABB tree for scene contacts and C API queries. */
#include "workphone_physics_broadphase.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#if !defined( WP_PHYSICS_DISABLE_SIMD ) && \
    ( defined( _M_X64 ) || defined( __SSE2__ ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 2 ) )
#    define WP_BP_SSE2 1
#    include <xmmintrin.h>
#else
#    define WP_BP_SSE2 0
#endif

typedef struct wp_bp_box
{
    wp_vec3f min, max;
} wp_bp_box;
typedef struct wp_bp_node
{
    wp_bp_box box, exact;
    wp_rigidbody *body;
    wp_s32 parent, left, right, height, next, enabled, movable, rank;
    wp_u32 order;
} wp_bp_node;
typedef struct wp_bp_order
{
    wp_s32 proxy;
    wp_u32 order;
} wp_bp_order;
struct wp_broadphase
{
    wp_broadphase_type type;
    wp_vec3f world_min, world_max;
    wp_bp_node *nodes;
    wp_bp_order *ordered, *hits;
    wp_s32 *stack;
    wp_s32 capacity, used, free_node, root, proxy_count;
    wp_s32 ordered_count, order_dirty, simd_enabled;
    wp_broadphase_pair *pairs;
    wp_s32 pair_count, pair_capacity, pair_failed;
    wp_u32 next_order;
    void *native, *user_data;
};

static wp_vec3f zero3( void )
{
    wp_vec3f v = { 0, 0, 0 };
    return v;
}
static wp_s32 overlaps( wp_bp_box a, wp_bp_box b )
{
    return !( a.max.x < b.min.x || a.min.x > b.max.x || a.max.y < b.min.y || a.min.y > b.max.y ||
              a.max.z < b.min.z || a.min.z > b.max.z );
}
static wp_s32 contains( wp_bp_box a, wp_bp_box b )
{
    return a.min.x <= b.min.x && a.min.y <= b.min.y && a.min.z <= b.min.z && a.max.x >= b.max.x &&
           a.max.y >= b.max.y && a.max.z >= b.max.z;
}
static wp_s32 valid_box( wp_bp_box a )
{
    return isfinite( a.min.x ) && isfinite( a.min.y ) && isfinite( a.min.z ) && isfinite( a.max.x ) &&
           isfinite( a.max.y ) && isfinite( a.max.z ) && a.min.x <= a.max.x && a.min.y <= a.max.y &&
           a.min.z <= a.max.z;
}
static wp_bp_box combine( wp_bp_box a, wp_bp_box b )
{
    wp_bp_box r;
    r.min.x = fminf( a.min.x, b.min.x );
    r.max.x = fmaxf( a.max.x, b.max.x );
    r.min.y = fminf( a.min.y, b.min.y );
    r.max.y = fmaxf( a.max.y, b.max.y );
    r.min.z = fminf( a.min.z, b.min.z );
    r.max.z = fmaxf( a.max.z, b.max.z );
    return r;
}
/* Double precision keeps the finite +/-FLT_MAX plane bounds from overflowing. */
static double area( wp_bp_box a )
{
    double x = (double)a.max.x - a.min.x, y = (double)a.max.y - a.min.y;
    double z = (double)a.max.z - a.min.z;
    return 2.0 * ( x * y + x * z + y * z );
}
static wp_bp_box fatten( wp_bp_box a, wp_f32 margin )
{
    a.min.x = fmaxf( -FLT_MAX, a.min.x - margin );
    a.max.x = fminf( FLT_MAX, a.max.x + margin );
    a.min.y = fmaxf( -FLT_MAX, a.min.y - margin );
    a.max.y = fminf( FLT_MAX, a.max.y + margin );
    a.min.z = fmaxf( -FLT_MAX, a.min.z - margin );
    a.max.z = fminf( FLT_MAX, a.max.z + margin );
    return a;
}
static wp_s32 reserve_nodes( wp_broadphase *bp, wp_s32 required )
{
    wp_s32 capacity = bp->capacity ? bp->capacity : 32;
    wp_bp_node *nodes;
    wp_bp_order *ordered, *hits;
    wp_s32 *stack;
    if( required <= bp->capacity )
        return 1;
    while( capacity < required )
    {
        if( capacity > INT_MAX / 2 )
            return 0;
        capacity *= 2;
    }
    if( (size_t)capacity > SIZE_MAX / sizeof( *nodes ) )
        return 0;
    nodes = (wp_bp_node *)malloc( (size_t)capacity * sizeof( *nodes ) );
    ordered = (wp_bp_order *)malloc( (size_t)capacity * sizeof( *ordered ) );
    hits = (wp_bp_order *)malloc( (size_t)capacity * sizeof( *hits ) );
    stack = (wp_s32 *)malloc( (size_t)capacity * sizeof( *stack ) );
    if( !nodes || !ordered || !hits || !stack )
    {
        free( nodes );
        free( ordered );
        free( hits );
        free( stack );
        return 0;
    }
    if( bp->used )
        memcpy( nodes, bp->nodes, (size_t)bp->used * sizeof( *nodes ) );
    free( bp->nodes );
    free( bp->ordered );
    free( bp->hits );
    free( bp->stack );
    bp->nodes = nodes;
    bp->ordered = ordered;
    bp->hits = hits;
    bp->stack = stack;
    bp->capacity = capacity;
    return 1;
}
static wp_s32 alloc_node( wp_broadphase *bp )
{
    wp_s32 id;
    if( bp->free_node != -1 )
    {
        id = bp->free_node;
        bp->free_node = bp->nodes[id].next;
    }
    else
        id = bp->used++;
    memset( &bp->nodes[id], 0, sizeof( bp->nodes[id] ) );
    bp->nodes[id].parent = bp->nodes[id].left = bp->nodes[id].right = -1;
    return id;
}
static void free_node( wp_broadphase *bp, wp_s32 id )
{
    bp->nodes[id].body = NULL;
    bp->nodes[id].height = -1;
    bp->nodes[id].next = bp->free_node;
    bp->free_node = id;
}
static wp_s32 is_proxy( const wp_broadphase *bp, wp_s32 id )
{
    return bp && id >= 0 && id < bp->used && bp->nodes[id].height == 0 && bp->nodes[id].body;
}
static void refit( wp_broadphase *bp, wp_s32 id )
{
    wp_bp_node *a = &bp->nodes[id], *l = &bp->nodes[a->left], *r = &bp->nodes[a->right];
    a->box = combine( l->box, r->box );
    a->height = 1 + ( l->height > r->height ? l->height : r->height );
    a->enabled = l->enabled || r->enabled;
    a->movable = l->movable || r->movable;
}
static void replace_child( wp_broadphase *bp, wp_s32 parent, wp_s32 old, wp_s32 child )
{
    if( parent == -1 )
        bp->root = child;
    else if( bp->nodes[parent].left == old )
        bp->nodes[parent].left = child;
    else
        bp->nodes[parent].right = child;
    bp->nodes[child].parent = parent;
}
/* Height rotations bound traversal depth even for sorted insertions. */
static wp_s32 balance( wp_broadphase *bp, wp_s32 id )
{
    wp_bp_node *a = &bp->nodes[id];
    wp_s32 l = a->left, r = a->right, delta;
    if( a->height < 2 )
        return id;
    delta = bp->nodes[r].height - bp->nodes[l].height;
    if( delta > 1 )
    {
        wp_s32 rl = bp->nodes[r].left, rr = bp->nodes[r].right;
        replace_child( bp, a->parent, id, r );
        bp->nodes[r].left = id;
        a->parent = r;
        if( bp->nodes[rl].height > bp->nodes[rr].height )
        {
            a->right = rr;
            bp->nodes[rr].parent = id;
            bp->nodes[r].right = rl;
            bp->nodes[rl].parent = r;
        }
        else
        {
            a->right = rl;
            bp->nodes[rl].parent = id;
            bp->nodes[r].right = rr;
            bp->nodes[rr].parent = r;
        }
        refit( bp, id );
        refit( bp, r );
        return r;
    }
    if( delta < -1 )
    {
        wp_s32 ll = bp->nodes[l].left, lr = bp->nodes[l].right;
        replace_child( bp, a->parent, id, l );
        bp->nodes[l].right = id;
        a->parent = l;
        if( bp->nodes[ll].height > bp->nodes[lr].height )
        {
            a->left = lr;
            bp->nodes[lr].parent = id;
            bp->nodes[l].left = ll;
            bp->nodes[ll].parent = l;
        }
        else
        {
            a->left = ll;
            bp->nodes[ll].parent = id;
            bp->nodes[l].left = lr;
            bp->nodes[lr].parent = l;
        }
        refit( bp, id );
        refit( bp, l );
        return l;
    }
    return id;
}
static void repair_ancestors( wp_broadphase *bp, wp_s32 id )
{
    while( id != -1 )
    {
        refit( bp, id );
        id = balance( bp, id );
        id = bp->nodes[id].parent;
    }
}
static void insert_leaf( wp_broadphase *bp, wp_s32 leaf )
{
    wp_s32 sibling = bp->root, parent, old_parent;
    wp_bp_box box = bp->nodes[leaf].box;
    if( sibling == -1 )
    {
        bp->root = leaf;
        return;
    }
    while( bp->nodes[sibling].left != -1 )
    {
        wp_s32 l = bp->nodes[sibling].left, r = bp->nodes[sibling].right;
        double current = area( bp->nodes[sibling].box );
        double joined = area( combine( bp->nodes[sibling].box, box ) );
        double inherited = 2.0 * ( joined - current );
        double cost_l = area( combine( bp->nodes[l].box, box ) );
        double cost_r = area( combine( bp->nodes[r].box, box ) );
        if( bp->nodes[l].left != -1 )
            cost_l -= area( bp->nodes[l].box );
        if( bp->nodes[r].left != -1 )
            cost_r -= area( bp->nodes[r].box );
        cost_l += inherited;
        cost_r += inherited;
        if( 2.0 * joined < cost_l && 2.0 * joined < cost_r )
            break;
        sibling = cost_l < cost_r ? l : r;
    }
    old_parent = bp->nodes[sibling].parent;
    parent = alloc_node( bp );
    bp->nodes[parent].left = sibling;
    bp->nodes[parent].right = leaf;
    replace_child( bp, old_parent, sibling, parent );
    bp->nodes[sibling].parent = parent;
    bp->nodes[leaf].parent = parent;
    repair_ancestors( bp, parent );
}
static void remove_leaf( wp_broadphase *bp, wp_s32 leaf )
{
    wp_s32 parent = bp->nodes[leaf].parent, sibling, grand;
    if( parent == -1 )
    {
        bp->root = -1;
        return;
    }
    grand = bp->nodes[parent].parent;
    sibling = bp->nodes[parent].left == leaf ? bp->nodes[parent].right : bp->nodes[parent].left;
    replace_child( bp, grand, parent, sibling );
    free_node( bp, parent );
    bp->nodes[leaf].parent = -1;
    repair_ancestors( bp, grand );
}
static int compare_order( const void *pa, const void *pb )
{
    const wp_bp_order *a = (const wp_bp_order *)pa, *b = (const wp_bp_order *)pb;
    if( a->order != b->order )
        return a->order < b->order ? -1 : 1;
    return ( a->proxy > b->proxy ) - ( a->proxy < b->proxy );
}
static wp_s32 find_proxy( const wp_broadphase *bp, wp_rigidbody *body )
{
    wp_s32 i;
    if( !bp || !body )
        return -1;
    for( i = 0; i < bp->used; ++i )
        if( bp->nodes[i].body == body )
            return i;
    return -1;
}
wp_broadphase *wp_broadphase_create( wp_broadphase_type type )
{
    wp_broadphase *bp = (wp_broadphase *)calloc( 1, sizeof( *bp ) );
    if( !bp )
        return NULL;
    bp->type = type;
    bp->order_dirty = 1;
    bp->root = bp->free_node = -1;
    bp->world_min.x = bp->world_min.y = bp->world_min.z = -1000.0f;
    bp->world_max.x = bp->world_max.y = bp->world_max.z = 1000.0f;
    return bp;
}
void wp_broadphase_destroy( wp_broadphase *bp )
{
    if( !bp )
        return;
    free( bp->nodes );
    free( bp->ordered );
    free( bp->hits );
    free( bp->stack );
    free( bp->pairs );
    free( bp );
}
void wp_broadphase_clear( wp_broadphase *bp )
{
    if( !bp )
        return;
    bp->used = bp->proxy_count = bp->pair_count = 0;
    bp->root = bp->free_node = -1;
    bp->next_order = 0;
    bp->ordered_count = 0;
    bp->order_dirty = 1;
}
wp_s32 wp_broadphase_create_proxy( wp_broadphase *bp, wp_rigidbody *body, wp_vec3f min, wp_vec3f max )
{
    wp_bp_box box;
    wp_s32 id;
    box.min = min;
    box.max = max;
    if( !bp || !body || !valid_box( box ) || bp->used > INT_MAX - 2 ||
        !reserve_nodes( bp, bp->used + 2 ) )
        return -1;
    id = alloc_node( bp );
    bp->nodes[id].body = body;
    bp->nodes[id].exact = box;
    bp->nodes[id].box = fatten( box, 0.1f );
    bp->nodes[id].enabled = bp->nodes[id].movable = 1;
    bp->nodes[id].order = bp->next_order++;
    insert_leaf( bp, id );
    ++bp->proxy_count;
    bp->order_dirty = 1;
    bp->pair_count = 0;
    return id;
}
void wp_broadphase_destroy_proxy( wp_broadphase *bp, wp_s32 id )
{
    if( !is_proxy( bp, id ) )
        return;
    remove_leaf( bp, id );
    free_node( bp, id );
    --bp->proxy_count;
    bp->order_dirty = 1;
    bp->pair_count = 0;
}
void wp_broadphase_move_proxy( wp_broadphase *bp, wp_s32 id, wp_vec3f min, wp_vec3f max )
{
    wp_bp_box box;
    box.min = min;
    box.max = max;
    if( !is_proxy( bp, id ) || !valid_box( box ) )
        return;
    bp->nodes[id].exact = box;
    bp->pair_count = 0;
    /* Reinsert after a substantial shrink as well: a plane/large mesh edited
     * into a small shape must not retain its old, excessively loose bounds. */
    if( contains( bp->nodes[id].box, box ) && contains( fatten( box, 0.4f ), bp->nodes[id].box ) )
        return;
    remove_leaf( bp, id );
    bp->nodes[id].box = fatten( box, 0.1f );
    insert_leaf( bp, id );
}
void wp_broadphase_configure_proxy( wp_broadphase *bp, wp_s32 id, wp_s32 enabled, wp_s32 movable,
                                    wp_u32 order )
{
    wp_s32 parent, changed;
    if( !is_proxy( bp, id ) )
        return;
    changed = bp->nodes[id].enabled != ( enabled != 0 ) || bp->nodes[id].movable != ( movable != 0 );
    if( bp->nodes[id].enabled != ( enabled != 0 ) || bp->nodes[id].order != order )
        bp->order_dirty = 1;
    bp->nodes[id].enabled = enabled != 0;
    bp->nodes[id].movable = movable != 0;
    bp->nodes[id].order = order;
    bp->pair_count = 0;
    if( changed )
        for( parent = bp->nodes[id].parent; parent != -1; parent = bp->nodes[parent].parent )
            refit( bp, parent );
}
wp_s32 wp_broadphase_add_proxy( wp_broadphase *bp, wp_rigidbody *body, wp_vec3f min, wp_vec3f max )
{
    wp_s32 existing = find_proxy( bp, body );
    if( existing != -1 )
    {
        wp_broadphase_move_proxy( bp, existing, min, max );
        return 1;
    }
    return wp_broadphase_create_proxy( bp, body, min, max ) != -1;
}
void wp_broadphase_remove_proxy( wp_broadphase *bp, wp_rigidbody *body )
{
    wp_broadphase_destroy_proxy( bp, find_proxy( bp, body ) );
}
void wp_broadphase_update_proxy( wp_broadphase *bp, wp_rigidbody *body, wp_vec3f min, wp_vec3f max )
{
    wp_broadphase_move_proxy( bp, find_proxy( bp, body ), min, max );
}

wp_s32 wp_broadphase_set_simd_enabled( wp_broadphase *bp, wp_s32 enabled )
{
    if( !bp )
        return 0;
    bp->simd_enabled = WP_BP_SSE2 && enabled != 0;
    return bp->simd_enabled;
}
wp_s32 wp_broadphase_get_simd_enabled( const wp_broadphase *bp )
{
    return bp ? bp->simd_enabled : 0;
}

/* Internal nodes reuse rank for the maximum descendant rank. This allows a
 * query to prune whole subtrees whose pairs have already been emitted. */
static wp_s32 update_rank_bounds( wp_broadphase *bp, wp_s32 id )
{
    wp_bp_node *node = &bp->nodes[id];
    if( !node->enabled )
        return node->rank = -1;
    if( node->left != -1 )
    {
        wp_s32 left = update_rank_bounds( bp, node->left );
        wp_s32 right = update_rank_bounds( bp, node->right );
        node->rank = left > right ? left : right;
    }
    return node->rank;
}

#if WP_BP_SSE2
/* Four independent boxes per vector. _mm_set_ps gathers only valid scalar
 * components; it never issues an oversized load from a public wp_vec3f. */
static unsigned overlap_mask4( wp_bp_box query, const wp_bp_node *nodes, const wp_s32 *ids )
{
    const wp_bp_box *a = &nodes[ids[0]].box, *b = &nodes[ids[1]].box;
    const wp_bp_box *c = &nodes[ids[2]].box, *d = &nodes[ids[3]].box;
    __m128 separated =
        _mm_cmpgt_ps( _mm_set_ps( d->min.x, c->min.x, b->min.x, a->min.x ), _mm_set1_ps( query.max.x ) );
    separated = _mm_or_ps( separated, _mm_cmplt_ps( _mm_set_ps( d->max.x, c->max.x, b->max.x, a->max.x ),
                                                    _mm_set1_ps( query.min.x ) ) );
    separated = _mm_or_ps( separated, _mm_cmpgt_ps( _mm_set_ps( d->min.y, c->min.y, b->min.y, a->min.y ),
                                                    _mm_set1_ps( query.max.y ) ) );
    separated = _mm_or_ps( separated, _mm_cmplt_ps( _mm_set_ps( d->max.y, c->max.y, b->max.y, a->max.y ),
                                                    _mm_set1_ps( query.min.y ) ) );
    separated = _mm_or_ps( separated, _mm_cmpgt_ps( _mm_set_ps( d->min.z, c->min.z, b->min.z, a->min.z ),
                                                    _mm_set1_ps( query.max.z ) ) );
    separated = _mm_or_ps( separated, _mm_cmplt_ps( _mm_set_ps( d->max.z, c->max.z, b->max.z, a->max.z ),
                                                    _mm_set1_ps( query.min.z ) ) );
    return ( ~(unsigned)_mm_movemask_ps( separated ) ) & 15u;
}
#endif

static inline void collect_node( wp_broadphase *bp, const wp_bp_node *a, wp_s32 id, wp_s32 *top,
                                 wp_s32 *found )
{
    wp_bp_node *b = &bp->nodes[id];
    if( b->left != -1 )
    {
        bp->stack[( *top )++] = b->left;
        bp->stack[( *top )++] = b->right;
    }
    else if( overlaps( a->exact, b->exact ) )
    {
        bp->hits[*found].proxy = id;
        bp->hits[( *found )++].order = (wp_u32)b->rank;
    }
}

static inline void collect_scalar_step( wp_broadphase *bp, const wp_bp_node *a, wp_s32 rank, wp_s32 *top,
                                        wp_s32 *found )
{
    wp_s32 id = bp->stack[--( *top )];
    const wp_bp_node *b = &bp->nodes[id];
    if( b->enabled && b->rank > rank && ( a->movable || b->movable ) && overlaps( a->exact, b->box ) )
        collect_node( bp, a, id, top, found );
}

static wp_s32 collect_scalar_query( wp_broadphase *bp, const wp_bp_node *a, wp_s32 rank )
{
    wp_s32 top = 1, found = 0;
    bp->stack[0] = bp->root;
    while( top )
        collect_scalar_step( bp, a, rank, &top, &found );
    return found;
}

#if WP_BP_SSE2
static wp_s32 collect_simd_query( wp_broadphase *bp, const wp_bp_node *a, wp_s32 rank )
{
    wp_s32 top = 1, found = 0;
    bp->stack[0] = bp->root;
    while( top )
    {
        if( top >= 4 )
        {
            wp_s32 ids[4], lane;
            unsigned mask = 0;
            for( lane = 0; lane < 4; ++lane )
            {
                const wp_bp_node *node;
                ids[lane] = bp->stack[--top];
                node = &bp->nodes[ids[lane]];
                if( node->enabled && node->rank > rank && ( a->movable || node->movable ) )
                    mask |= 1u << lane;
            }
            if( mask )
                mask &= overlap_mask4( a->exact, bp->nodes, ids );
            for( lane = 0; lane < 4; ++lane )
                if( mask & ( 1u << lane ) )
                    collect_node( bp, a, ids[lane], &top, &found );
        }
        else
            collect_scalar_step( bp, a, rank, &top, &found );
    }
    return found;
}
#endif

void wp_broadphase_visit_pairs( wp_broadphase *bp, wp_broadphase_pair_callback callback, void *context )
{
    wp_s32 i, count = 0;
#if WP_BP_SSE2
    wp_s32 previous_hits = 0;
#endif
    if( !bp || !callback || bp->root == -1 || !bp->nodes[bp->root].movable )
        return;
    if( bp->order_dirty )
    {
        for( i = 0; i < bp->used; ++i )
            if( bp->nodes[i].body && bp->nodes[i].enabled )
            {
                bp->ordered[count].proxy = i;
                bp->ordered[count++].order = bp->nodes[i].order;
            }
        qsort( bp->ordered, count, sizeof( *bp->ordered ), compare_order );
        for( i = 0; i < count; ++i )
            bp->nodes[bp->ordered[i].proxy].rank = i;
        bp->ordered_count = count;
        bp->order_dirty = 0;
    }
    count = bp->ordered_count;
    /* Tree rotations can change ancestor rank bounds even without a reorder. */
    update_rank_bounds( bp, bp->root );
    for( i = 0; i < count; ++i )
    {
        wp_bp_node *a = &bp->nodes[bp->ordered[i].proxy];
        wp_s32 found, h;
#if WP_BP_SSE2
        /* Dense neighbors favor scalar traversal. Decide once per query so
         * scalar traversal pays no SIMD dispatch cost at each visited node. */
        if( bp->simd_enabled && previous_hits < 8 )
            found = collect_simd_query( bp, a, i );
        else
#endif
            found = collect_scalar_query( bp, a, i );
#if WP_BP_SSE2
        previous_hits = found;
#endif
        if( found > 1 )
            qsort( bp->hits, found, sizeof( *bp->hits ), compare_order );
        for( h = 0; h < found; ++h )
        {
            wp_broadphase_pair pair;
            pair.body_a = a->body;
            pair.body_b = bp->nodes[bp->hits[h].proxy].body;
            callback( &pair, context );
        }
    }
}
static void cache_pair( const wp_broadphase_pair *pair, void *context )
{
    wp_broadphase *bp = (wp_broadphase *)context;
    if( bp->pair_failed )
        return;
    if( bp->pair_count == bp->pair_capacity )
    {
        wp_s32 capacity;
        wp_broadphase_pair *pairs;
        if( bp->pair_capacity > INT_MAX / 2 )
        {
            bp->pair_failed = 1;
            return;
        }
        capacity = bp->pair_capacity ? bp->pair_capacity * 2 : 64;
        if( (size_t)capacity > SIZE_MAX / sizeof( *pairs ) )
        {
            bp->pair_failed = 1;
            return;
        }
        pairs = (wp_broadphase_pair *)realloc( bp->pairs, (size_t)capacity * sizeof( *pairs ) );
        if( !pairs )
        {
            bp->pair_failed = 1;
            return;
        }
        bp->pairs = pairs;
        bp->pair_capacity = capacity;
    }
    bp->pairs[bp->pair_count++] = *pair;
}
wp_s32 wp_broadphase_calculate_overlapping_pairs_checked( wp_broadphase *bp )
{
    if( !bp )
        return 0;
    bp->pair_count = bp->pair_failed = 0;
    wp_broadphase_visit_pairs( bp, cache_pair, bp );
    if( bp->pair_failed )
        bp->pair_count = 0;
    return !bp->pair_failed;
}
void wp_broadphase_calculate_overlapping_pairs( wp_broadphase *bp )
{
    (void)wp_broadphase_calculate_overlapping_pairs_checked( bp );
}
const wp_broadphase_pair *wp_broadphase_get_pair_cache( const wp_broadphase *bp )
{
    return bp && bp->pair_count ? bp->pairs : NULL;
}
wp_s32 wp_broadphase_get_pair_count( const wp_broadphase *bp )
{
    return bp ? bp->pair_count : 0;
}
wp_s32 wp_broadphase_get_proxy_count( const wp_broadphase *bp )
{
    return bp ? bp->proxy_count : 0;
}
wp_s32 wp_broadphase_query_aabb( wp_broadphase *bp, wp_vec3f min, wp_vec3f max, wp_rigidbody **out,
                                 wp_s32 limit )
{
    wp_bp_box box;
    wp_s32 top = 0, found = 0;
    box.min = min;
    box.max = max;
    if( !bp || !out || limit <= 0 || bp->root == -1 || !valid_box( box ) )
        return 0;
    bp->stack[top++] = bp->root;
    while( top && found < limit )
    {
        wp_s32 id = bp->stack[--top];
        wp_bp_node *node = &bp->nodes[id];
        if( !node->enabled || !overlaps( box, node->box ) )
            continue;
        if( node->left != -1 )
        {
            bp->stack[top++] = node->left;
            bp->stack[top++] = node->right;
        }
        else if( node->enabled && overlaps( box, node->exact ) )
            out[found++] = node->body;
    }
    return found;
}
wp_broadphase_type wp_broadphase_get_type( const wp_broadphase *bp )
{
    return bp ? bp->type : WORKPHONE_BROADPHASE_SAP;
}
wp_vec3f wp_broadphase_get_world_min( const wp_broadphase *bp )
{
    return bp ? bp->world_min : zero3();
}
wp_vec3f wp_broadphase_get_world_max( const wp_broadphase *bp )
{
    return bp ? bp->world_max : zero3();
}
void wp_broadphase_set_world_min( wp_broadphase *bp, wp_vec3f min )
{
    if( bp )
        bp->world_min = min;
}
void wp_broadphase_set_world_max( wp_broadphase *bp, wp_vec3f max )
{
    if( bp )
        bp->world_max = max;
}
void *wp_broadphase_get_native( const wp_broadphase *bp )
{
    return bp ? bp->native : NULL;
}
void wp_broadphase_set_native( wp_broadphase *bp, void *native )
{
    if( bp )
        bp->native = native;
}
void *wp_broadphase_get_user_data( const wp_broadphase *bp )
{
    return bp ? bp->user_data : NULL;
}
void wp_broadphase_set_user_data( wp_broadphase *bp, void *data )
{
    if( bp )
        bp->user_data = data;
}
