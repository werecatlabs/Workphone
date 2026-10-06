#include "workphone_graphics_skinning.h"
#include <float.h>
#include <math.h>

static int finite_value( double value )
{
    return value >= -FLT_MAX && value <= FLT_MAX;
}

static int finite_vector( wp_vec3f value )
{
    return finite_value( value.x ) && finite_value( value.y ) && finite_value( value.z );
}

wp_s32 wp_skin_vertices( const wp_skin_vertex *vertices, wp_u32 vertex_count,
                         const wp_mat4f *palette, wp_u32 joint_count,
                         wp_skin_result *output )
{
    wp_u32 i, j, r, c;
    double sum, length, axes[3], dot;
    if( !vertices || !output || !palette || !joint_count ||
        joint_count > WP_SKIN_MAX_JOINTS || !vertex_count ) return 0;
    for( j = 0; j < joint_count; ++j )
    {
        for( r = 0; r < 4; ++r )
            for( c = 0; c < 4; ++c )
                if( !finite_value( palette[j].m[r][c] ) ||
                    fabs( palette[j].m[r][c] ) > 1e6 ) return 0;
        if( fabs( palette[j].m[3][0] ) > 1e-6 || fabs( palette[j].m[3][1] ) > 1e-6 ||
            fabs( palette[j].m[3][2] ) > 1e-6 || fabs( palette[j].m[3][3] - 1.0 ) > 1e-6 ) return 0;
        for( c = 0; c < 3; ++c )
        {
            axes[c] = 0.0;
            for( r = 0; r < 3; ++r ) axes[c] += (double)palette[j].m[r][c] * palette[j].m[r][c];
        }
        if( axes[0] < 1e-12 || fabs( axes[0] - axes[1] ) > 1e-5 * axes[0] ||
            fabs( axes[0] - axes[2] ) > 1e-5 * axes[0] ) return 0;
        for( c = 0; c < 3; ++c )
            for( i = c + 1; i < 3; ++i )
            {
                dot = 0.0;
                for( r = 0; r < 3; ++r ) dot += (double)palette[j].m[r][c] * palette[j].m[r][i];
                if( fabs( dot ) > 1e-5 * axes[0] ) return 0;
            }
    }
    for( i = 0; i < vertex_count; ++i )
    {
        if( !finite_vector( vertices[i].position ) || !finite_vector( vertices[i].normal ) ||
            fabs( vertices[i].position.x ) > 1e6 || fabs( vertices[i].position.y ) > 1e6 ||
            fabs( vertices[i].position.z ) > 1e6 ) return 0;
        for( j = 0; j < WP_SKIN_INFLUENCES; ++j )
            if( !finite_value( vertices[i].weights[j] ) || vertices[i].weights[j] < 0.0f ||
                ( vertices[i].weights[j] > 0.0f && vertices[i].joints[j] >= joint_count ) ) return 0;
    }
    for( i = 0; i < vertex_count; ++i )
    {
        double p[3] = { 0.0, 0.0, 0.0 };
        double n[3] = { 0.0, 0.0, 0.0 };
        sum = 0.0;
        for( j = 0; j < WP_SKIN_INFLUENCES; ++j ) sum += vertices[i].weights[j];
        if( sum <= 0.0 )
        {
            output[i].position = vertices[i].position;
            output[i].normal = vertices[i].normal;
            continue;
        }
        for( j = 0; j < WP_SKIN_INFLUENCES; ++j )
        {
            const wp_mat4f *m;
            double w;
            if( vertices[i].weights[j] <= 0.0f ) continue;
            m = &palette[vertices[i].joints[j]];
            w = vertices[i].weights[j] / sum;
            length = (double)m->m[0][0] * m->m[0][0] +
                (double)m->m[1][0] * m->m[1][0] + (double)m->m[2][0] * m->m[2][0];
            for( r = 0; r < 3; ++r )
            {
                p[r] += w * ( (double)m->m[r][0] * vertices[i].position.x +
                    (double)m->m[r][1] * vertices[i].position.y +
                    (double)m->m[r][2] * vertices[i].position.z + m->m[r][3] );
                n[r] += w / length * ( (double)m->m[r][0] * vertices[i].normal.x +
                    (double)m->m[r][1] * vertices[i].normal.y +
                    (double)m->m[r][2] * vertices[i].normal.z );
            }
        }
        output[i].position.x = (wp_f32)p[0];
        output[i].position.y = (wp_f32)p[1];
        output[i].position.z = (wp_f32)p[2];
        length = sqrt( n[0] * n[0] + n[1] * n[1] + n[2] * n[2] );
        if( length > 1e-12 )
        {
            output[i].normal.x = (wp_f32)( n[0] / length );
            output[i].normal.y = (wp_f32)( n[1] / length );
            output[i].normal.z = (wp_f32)( n[2] / length );
        }
        else output[i].normal = vertices[i].normal;
    }
    return 1;
}
