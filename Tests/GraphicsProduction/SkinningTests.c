#include "workphone_graphics_skinning.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(value) do { if( !(value) ) { fprintf(stderr, "skinning: line %d: %s\n", __LINE__, #value); return 1; } } while(0)

static void identity( wp_mat4f *m )
{
    int i;
    memset( m, 0, sizeof(*m) );
    for( i = 0; i < 4; ++i ) m->m[i][i] = 1.0f;
}

int main( void )
{
    wp_mat4f palette[2];
    wp_skin_vertex vertices[3];
    wp_skin_result output[3], previous[3];
    int i;
    identity( &palette[0] ); identity( &palette[1] );
    memset( vertices, 0, sizeof(vertices) );
    for( i = 0; i < 3; ++i )
    {
        vertices[i].position.x = 1.0f;
        vertices[i].normal.x = 1.0f;
    }
    vertices[0].weights[0] = 2.0f;
    vertices[1].joints[0] = 1;
    vertices[1].weights[0] = 1.0f;
    CHECK( wp_skin_vertices( vertices, 3, palette, 2, output ) );
    CHECK( fabs(output[0].position.x - 1.0f) < 1e-5 );
    /* Joint 1 rotates 90 degrees then translates, independent from joint 0. */
    palette[1].m[0][0] = palette[1].m[1][1] = 0.0f;
    palette[1].m[0][1] = -1.0f; palette[1].m[1][0] = 1.0f;
    palette[1].m[0][3] = 2.0f;
    CHECK( wp_skin_vertices( vertices, 3, palette, 2, output ) );
    CHECK( fabs(output[1].position.x - 2.0f) < 1e-5 && fabs(output[1].position.y - 1.0f) < 1e-5 );
    CHECK( fabs(output[1].normal.y - 1.0f) < 1e-5 );
    CHECK( output[2].position.x == 1.0f ); /* Zero weights preserve geometry. */
    vertices[0].weights[1] = 2.0f; vertices[0].joints[1] = 1;
    CHECK( wp_skin_vertices( vertices, 3, palette, 2, output ) );
    CHECK( fabs(output[0].position.x - 1.5f) < 1e-5 && fabs(output[0].position.y - 0.5f) < 1e-5 );
    memcpy( previous, output, sizeof(output) );
    vertices[2].weights[0] = -1.0f;
    CHECK( !wp_skin_vertices( vertices, 3, palette, 2, output ) );
    CHECK( memcmp(previous, output, sizeof(output)) == 0 ); /* Failure is transactional. */
    vertices[2].weights[0] = 1.0f; vertices[2].joints[0] = 2;
    CHECK( !wp_skin_vertices( vertices, 3, palette, 2, output ) );
    vertices[2].joints[0] = 0;
    palette[0].m[0][0] = 2.0f;
    CHECK( !wp_skin_vertices( vertices, 3, palette, 2, output ) ); /* Nonuniform scale rejected. */
    palette[0].m[1][1] = palette[0].m[2][2] = 2.0f;
    CHECK( wp_skin_vertices( vertices, 3, palette, 2, output ) );
    CHECK( fabs(output[2].position.x - 2.0f) < 1e-5 && fabs(output[2].normal.x - 1.0f) < 1e-5 );
    CHECK( !wp_skin_vertices( vertices, 3, palette, WP_SKIN_MAX_JOINTS + 1, output ) );
    CHECK( !wp_skin_vertices( NULL, 3, palette, 2, output ) );
    puts("PASS: bind pose, two-joint deformation, weights, normals, limits, transactional failures");
    return 0;
}
