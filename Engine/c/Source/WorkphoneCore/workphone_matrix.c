/**
 * @file wp_matrix.c
 * @brief Implementation of C matrix API.
 */

#include "workphone_math.h"
#include "workphone_matrix.h"
#include <string.h>

void wp_mat3f_identity( wp_mat3f *mat )
{
    memset( mat, 0, sizeof( wp_mat3f ) );
    mat->m[0][0] = 1.0f;
    mat->m[1][1] = 1.0f;
    mat->m[2][2] = 1.0f;
}

void wp_mat3f_zero( wp_mat3f *mat )
{
    memset( mat, 0, sizeof( wp_mat3f ) );
}

void wp_mat3f_set( wp_mat3f *mat, wp_f32 m00, wp_f32 m01, wp_f32 m02, wp_f32 m10, wp_f32 m11, wp_f32 m12,
                   wp_f32 m20, wp_f32 m21, wp_f32 m22 )
{
    mat->m[0][0] = m00;
    mat->m[0][1] = m01;
    mat->m[0][2] = m02;
    mat->m[1][0] = m10;
    mat->m[1][1] = m11;
    mat->m[1][2] = m12;
    mat->m[2][0] = m20;
    mat->m[2][1] = m21;
    mat->m[2][2] = m22;
}

void wp_mat3f_copy( wp_mat3f *dest, const wp_mat3f *src )
{
    memcpy( dest, src, sizeof( wp_mat3f ) );
}

void wp_mat3f_add( wp_mat3f *result, const wp_mat3f *a, const wp_mat3f *b )
{
    wp_s32 i, j;
    for( i = 0; i < 3; i++ )
    {
        for( j = 0; j < 3; j++ )
        {
            result->m[i][j] = a->m[i][j] + b->m[i][j];
        }
    }
}

void wp_mat3f_sub( wp_mat3f *result, const wp_mat3f *a, const wp_mat3f *b )
{
    wp_s32 i, j;
    for( i = 0; i < 3; i++ )
    {
        for( j = 0; j < 3; j++ )
        {
            result->m[i][j] = a->m[i][j] - b->m[i][j];
        }
    }
}

void wp_mat3f_mul( wp_mat3f *result, const wp_mat3f *a, const wp_mat3f *b )
{
    wp_mat3f temp;
    wp_s32 i, j, k;
    for( i = 0; i < 3; i++ )
    {
        for( j = 0; j < 3; j++ )
        {
            temp.m[i][j] = 0.0f;
            for( k = 0; k < 3; k++ )
            {
                temp.m[i][j] += a->m[i][k] * b->m[k][j];
            }
        }
    }
    *result = temp;
}

void wp_mat3f_mul_scalar( wp_mat3f *result, const wp_mat3f *mat, wp_f32 scalar )
{
    wp_s32 i, j;
    for( i = 0; i < 3; i++ )
    {
        for( j = 0; j < 3; j++ )
        {
            result->m[i][j] = mat->m[i][j] * scalar;
        }
    }
}

void wp_mat3f_transpose( wp_mat3f *result, const wp_mat3f *mat )
{
    wp_mat3f temp;
    wp_s32 i, j;
    for( i = 0; i < 3; i++ )
    {
        for( j = 0; j < 3; j++ )
        {
            temp.m[i][j] = mat->m[j][i];
        }
    }
    *result = temp;
}

wp_f32 wp_mat3f_determinant( const wp_mat3f *mat )
{
    wp_f32 a = mat->m[0][0];
    wp_f32 b = mat->m[0][1];
    wp_f32 c = mat->m[0][2];
    wp_f32 d = mat->m[1][0];
    wp_f32 e = mat->m[1][1];
    wp_f32 f = mat->m[1][2];
    wp_f32 g = mat->m[2][0];
    wp_f32 h = mat->m[2][1];
    wp_f32 i = mat->m[2][2];

    return a * ( e * i - f * h ) - b * ( d * i - f * g ) + c * ( d * h - e * g );
}

wp_s32 wp_mat3f_invert( wp_mat3f *result, const wp_mat3f *mat )
{
    wp_f32 det = wp_mat3f_determinant( mat );

    if( wp_absf( det ) < WORKPHONE_EPSILON_F )
    {
        return 0;
    }

    wp_f32 invDet = 1.0f / det;
    wp_mat3f temp;

    temp.m[0][0] = ( mat->m[1][1] * mat->m[2][2] - mat->m[1][2] * mat->m[2][1] ) * invDet;
    temp.m[0][1] = ( mat->m[0][2] * mat->m[2][1] - mat->m[0][1] * mat->m[2][2] ) * invDet;
    temp.m[0][2] = ( mat->m[0][1] * mat->m[1][2] - mat->m[0][2] * mat->m[1][1] ) * invDet;
    temp.m[1][0] = ( mat->m[1][2] * mat->m[2][0] - mat->m[1][0] * mat->m[2][2] ) * invDet;
    temp.m[1][1] = ( mat->m[0][0] * mat->m[2][2] - mat->m[0][2] * mat->m[2][0] ) * invDet;
    temp.m[1][2] = ( mat->m[0][2] * mat->m[1][0] - mat->m[0][0] * mat->m[1][2] ) * invDet;
    temp.m[2][0] = ( mat->m[1][0] * mat->m[2][1] - mat->m[1][1] * mat->m[2][0] ) * invDet;
    temp.m[2][1] = ( mat->m[0][1] * mat->m[2][0] - mat->m[0][0] * mat->m[2][1] ) * invDet;
    temp.m[2][2] = ( mat->m[0][0] * mat->m[1][1] - mat->m[0][1] * mat->m[1][0] ) * invDet;

    *result = temp;
    return 1;
}

void wp_mat3f_from_rotation_x( wp_mat3f *mat, wp_f32 angle )
{
    wp_f32 c = wp_cosf( angle );
    wp_f32 s = wp_sinf( angle );
    wp_mat3f_identity( mat );
    mat->m[1][1] = c;
    mat->m[1][2] = -s;
    mat->m[2][1] = s;
    mat->m[2][2] = c;
}

void wp_mat3f_from_rotation_y( wp_mat3f *mat, wp_f32 angle )
{
    wp_f32 c = wp_cosf( angle );
    wp_f32 s = wp_sinf( angle );
    wp_mat3f_identity( mat );
    mat->m[0][0] = c;
    mat->m[0][2] = s;
    mat->m[2][0] = -s;
    mat->m[2][2] = c;
}

void wp_mat3f_from_rotation_z( wp_mat3f *mat, wp_f32 angle )
{
    wp_f32 c = wp_cosf( angle );
    wp_f32 s = wp_sinf( angle );
    wp_mat3f_identity( mat );
    mat->m[0][0] = c;
    mat->m[0][1] = -s;
    mat->m[1][0] = s;
    mat->m[1][1] = c;
}

void wp_mat3f_from_scale( wp_mat3f *mat, wp_f32 sx, wp_f32 sy, wp_f32 sz )
{
    wp_mat3f_zero( mat );
    mat->m[0][0] = sx;
    mat->m[1][1] = sy;
    mat->m[2][2] = sz;
}

void wp_mat4f_identity( wp_mat4f *mat )
{
    memset( mat, 0, sizeof( wp_mat4f ) );
    mat->m[0][0] = 1.0f;
    mat->m[1][1] = 1.0f;
    mat->m[2][2] = 1.0f;
    mat->m[3][3] = 1.0f;
}

void wp_mat4f_zero( wp_mat4f *mat )
{
    memset( mat, 0, sizeof( wp_mat4f ) );
}

void wp_mat4f_set( wp_mat4f *mat, wp_f32 m00, wp_f32 m01, wp_f32 m02, wp_f32 m03, wp_f32 m10, wp_f32 m11,
                   wp_f32 m12, wp_f32 m13, wp_f32 m20, wp_f32 m21, wp_f32 m22, wp_f32 m23, wp_f32 m30,
                   wp_f32 m31, wp_f32 m32, wp_f32 m33 )
{
    mat->m[0][0] = m00;
    mat->m[0][1] = m01;
    mat->m[0][2] = m02;
    mat->m[0][3] = m03;
    mat->m[1][0] = m10;
    mat->m[1][1] = m11;
    mat->m[1][2] = m12;
    mat->m[1][3] = m13;
    mat->m[2][0] = m20;
    mat->m[2][1] = m21;
    mat->m[2][2] = m22;
    mat->m[2][3] = m23;
    mat->m[3][0] = m30;
    mat->m[3][1] = m31;
    mat->m[3][2] = m32;
    mat->m[3][3] = m33;
}

void wp_mat4f_copy( wp_mat4f *dest, const wp_mat4f *src )
{
    memcpy( dest, src, sizeof( wp_mat4f ) );
}

void wp_mat4f_add( wp_mat4f *result, const wp_mat4f *a, const wp_mat4f *b )
{
    wp_s32 i, j;
    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < 4; j++ )
        {
            result->m[i][j] = a->m[i][j] + b->m[i][j];
        }
    }
}

void wp_mat4f_sub( wp_mat4f *result, const wp_mat4f *a, const wp_mat4f *b )
{
    wp_s32 i, j;
    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < 4; j++ )
        {
            result->m[i][j] = a->m[i][j] - b->m[i][j];
        }
    }
}

void wp_mat4f_mul( wp_mat4f *result, const wp_mat4f *a, const wp_mat4f *b )
{
    wp_mat4f temp;
    wp_s32 i, j, k;
    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < 4; j++ )
        {
            temp.m[i][j] = 0.0f;
            for( k = 0; k < 4; k++ )
            {
                temp.m[i][j] += a->m[i][k] * b->m[k][j];
            }
        }
    }
    *result = temp;
}

void wp_mat4f_mul_scalar( wp_mat4f *result, const wp_mat4f *mat, wp_f32 scalar )
{
    wp_s32 i, j;
    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < 4; j++ )
        {
            result->m[i][j] = mat->m[i][j] * scalar;
        }
    }
}

void wp_mat4f_transpose( wp_mat4f *result, const wp_mat4f *mat )
{
    wp_mat4f temp;
    wp_s32 i, j;
    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < 4; j++ )
        {
            temp.m[i][j] = mat->m[j][i];
        }
    }
    *result = temp;
}

wp_f32 wp_mat4f_determinant( const wp_mat4f *mat )
{
    wp_f32 a0 = mat->m[0][0] * mat->m[1][1] - mat->m[0][1] * mat->m[1][0];
    wp_f32 a1 = mat->m[0][0] * mat->m[1][2] - mat->m[0][2] * mat->m[1][0];
    wp_f32 a2 = mat->m[0][0] * mat->m[1][3] - mat->m[0][3] * mat->m[1][0];
    wp_f32 a3 = mat->m[0][1] * mat->m[1][2] - mat->m[0][2] * mat->m[1][1];
    wp_f32 a4 = mat->m[0][1] * mat->m[1][3] - mat->m[0][3] * mat->m[1][1];
    wp_f32 a5 = mat->m[0][2] * mat->m[1][3] - mat->m[0][3] * mat->m[1][2];
    wp_f32 b0 = mat->m[2][0] * mat->m[3][1] - mat->m[2][1] * mat->m[3][0];
    wp_f32 b1 = mat->m[2][0] * mat->m[3][2] - mat->m[2][2] * mat->m[3][0];
    wp_f32 b2 = mat->m[2][0] * mat->m[3][3] - mat->m[2][3] * mat->m[3][0];
    wp_f32 b3 = mat->m[2][1] * mat->m[3][2] - mat->m[2][2] * mat->m[3][1];
    wp_f32 b4 = mat->m[2][1] * mat->m[3][3] - mat->m[2][3] * mat->m[3][1];
    wp_f32 b5 = mat->m[2][2] * mat->m[3][3] - mat->m[2][3] * mat->m[3][2];

    return a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
}

wp_s32 wp_mat4f_invert( wp_mat4f *result, const wp_mat4f *mat )
{
    wp_f32 a0 = mat->m[0][0] * mat->m[1][1] - mat->m[0][1] * mat->m[1][0];
    wp_f32 a1 = mat->m[0][0] * mat->m[1][2] - mat->m[0][2] * mat->m[1][0];
    wp_f32 a2 = mat->m[0][0] * mat->m[1][3] - mat->m[0][3] * mat->m[1][0];
    wp_f32 a3 = mat->m[0][1] * mat->m[1][2] - mat->m[0][2] * mat->m[1][1];
    wp_f32 a4 = mat->m[0][1] * mat->m[1][3] - mat->m[0][3] * mat->m[1][1];
    wp_f32 a5 = mat->m[0][2] * mat->m[1][3] - mat->m[0][3] * mat->m[1][2];
    wp_f32 b0 = mat->m[2][0] * mat->m[3][1] - mat->m[2][1] * mat->m[3][0];
    wp_f32 b1 = mat->m[2][0] * mat->m[3][2] - mat->m[2][2] * mat->m[3][0];
    wp_f32 b2 = mat->m[2][0] * mat->m[3][3] - mat->m[2][3] * mat->m[3][0];
    wp_f32 b3 = mat->m[2][1] * mat->m[3][2] - mat->m[2][2] * mat->m[3][1];
    wp_f32 b4 = mat->m[2][1] * mat->m[3][3] - mat->m[2][3] * mat->m[3][1];
    wp_f32 b5 = mat->m[2][2] * mat->m[3][3] - mat->m[2][3] * mat->m[3][2];

    wp_f32 det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;

    if( wp_absf( det ) < WORKPHONE_EPSILON_F )
    {
        return 0;
    }

    wp_f32 invDet = 1.0f / det;
    wp_mat4f temp;

    temp.m[0][0] = ( +mat->m[1][1] * b5 - mat->m[1][2] * b4 + mat->m[1][3] * b3 ) * invDet;
    temp.m[0][1] = ( -mat->m[0][1] * b5 + mat->m[0][2] * b4 - mat->m[0][3] * b3 ) * invDet;
    temp.m[0][2] = ( +mat->m[3][1] * a5 - mat->m[3][2] * a4 + mat->m[3][3] * a3 ) * invDet;
    temp.m[0][3] = ( -mat->m[2][1] * a5 + mat->m[2][2] * a4 - mat->m[2][3] * a3 ) * invDet;
    temp.m[1][0] = ( -mat->m[1][0] * b5 + mat->m[1][2] * b2 - mat->m[1][3] * b1 ) * invDet;
    temp.m[1][1] = ( +mat->m[0][0] * b5 - mat->m[0][2] * b2 + mat->m[0][3] * b1 ) * invDet;
    temp.m[1][2] = ( -mat->m[3][0] * a5 + mat->m[3][2] * a2 - mat->m[3][3] * a1 ) * invDet;
    temp.m[1][3] = ( +mat->m[2][0] * a5 - mat->m[2][2] * a2 + mat->m[2][3] * a1 ) * invDet;
    temp.m[2][0] = ( +mat->m[1][0] * b4 - mat->m[1][1] * b2 + mat->m[1][3] * b0 ) * invDet;
    temp.m[2][1] = ( -mat->m[0][0] * b4 + mat->m[0][1] * b2 - mat->m[0][3] * b0 ) * invDet;
    temp.m[2][2] = ( +mat->m[3][0] * a4 - mat->m[3][1] * a2 + mat->m[3][3] * a0 ) * invDet;
    temp.m[2][3] = ( -mat->m[2][0] * a4 + mat->m[2][1] * a2 - mat->m[2][3] * a0 ) * invDet;
    temp.m[3][0] = ( -mat->m[1][0] * b3 + mat->m[1][1] * b1 - mat->m[1][2] * b0 ) * invDet;
    temp.m[3][1] = ( +mat->m[0][0] * b3 - mat->m[0][1] * b1 + mat->m[0][2] * b0 ) * invDet;
    temp.m[3][2] = ( -mat->m[3][0] * a3 + mat->m[3][1] * a1 - mat->m[3][2] * a0 ) * invDet;
    temp.m[3][3] = ( +mat->m[2][0] * a3 - mat->m[2][1] * a1 + mat->m[2][2] * a0 ) * invDet;

    *result = temp;
    return 1;
}

void wp_mat4f_from_translation( wp_mat4f *mat, wp_f32 x, wp_f32 y, wp_f32 z )
{
    wp_mat4f_identity( mat );
    mat->m[0][3] = x;
    mat->m[1][3] = y;
    mat->m[2][3] = z;
}

void wp_mat4f_from_rotation_x( wp_mat4f *mat, wp_f32 angle )
{
    wp_f32 c = wp_cosf( angle );
    wp_f32 s = wp_sinf( angle );
    wp_mat4f_identity( mat );
    mat->m[1][1] = c;
    mat->m[1][2] = -s;
    mat->m[2][1] = s;
    mat->m[2][2] = c;
}

void wp_mat4f_from_rotation_y( wp_mat4f *mat, wp_f32 angle )
{
    wp_f32 c = wp_cosf( angle );
    wp_f32 s = wp_sinf( angle );
    wp_mat4f_identity( mat );
    mat->m[0][0] = c;
    mat->m[0][2] = s;
    mat->m[2][0] = -s;
    mat->m[2][2] = c;
}

void wp_mat4f_from_rotation_z( wp_mat4f *mat, wp_f32 angle )
{
    wp_f32 c = wp_cosf( angle );
    wp_f32 s = wp_sinf( angle );
    wp_mat4f_identity( mat );
    mat->m[0][0] = c;
    mat->m[0][1] = -s;
    mat->m[1][0] = s;
    mat->m[1][1] = c;
}

void wp_mat4f_from_scale( wp_mat4f *mat, wp_f32 sx, wp_f32 sy, wp_f32 sz )
{
    wp_mat4f_zero( mat );
    mat->m[0][0] = sx;
    mat->m[1][1] = sy;
    mat->m[2][2] = sz;
    mat->m[3][3] = 1.0f;
}

void wp_mat4f_look_at( wp_mat4f *mat, wp_f32 eyeX, wp_f32 eyeY, wp_f32 eyeZ, wp_f32 centerX,
                       wp_f32 centerY, wp_f32 centerZ, wp_f32 upX, wp_f32 upY, wp_f32 upZ )
{
    wp_f32 fx = centerX - eyeX;
    wp_f32 fy = centerY - eyeY;
    wp_f32 fz = centerZ - eyeZ;

    wp_f32 flen = wp_sqrtf( fx * fx + fy * fy + fz * fz );
    if( flen > WORKPHONE_EPSILON_F )
    {
        fx /= flen;
        fy /= flen;
        fz /= flen;
    }

    wp_f32 sx = fy * upZ - fz * upY;
    wp_f32 sy = fz * upX - fx * upZ;
    wp_f32 sz = fx * upY - fy * upX;

    wp_f32 slen = wp_sqrtf( sx * sx + sy * sy + sz * sz );
    if( slen > WORKPHONE_EPSILON_F )
    {
        sx /= slen;
        sy /= slen;
        sz /= slen;
    }

    wp_f32 ux = sy * fz - sz * fy;
    wp_f32 uy = sz * fx - sx * fz;
    wp_f32 uz = sx * fy - sy * fx;

    wp_mat4f_identity( mat );
    mat->m[0][0] = sx;
    mat->m[0][1] = ux;
    mat->m[0][2] = -fx;
    mat->m[1][0] = sy;
    mat->m[1][1] = uy;
    mat->m[1][2] = -fy;
    mat->m[2][0] = sz;
    mat->m[2][1] = uz;
    mat->m[2][2] = -fz;
    mat->m[0][3] = -( sx * eyeX + sy * eyeY + sz * eyeZ );
    mat->m[1][3] = -( ux * eyeX + uy * eyeY + uz * eyeZ );
    mat->m[2][3] = -( ( -fx ) * eyeX + ( -fy ) * eyeY + ( -fz ) * eyeZ );
}

void wp_mat4f_perspective( wp_mat4f *mat, wp_f32 fovy, wp_f32 aspect, wp_f32 znear, wp_f32 zfar )
{
    wp_f32 f = 1.0f / wp_tanf( fovy * 0.5f );
    wp_f32 nf = 1.0f / ( znear - zfar );

    wp_mat4f_zero( mat );
    mat->m[0][0] = f / aspect;
    mat->m[1][1] = f;
    mat->m[2][2] = ( zfar + znear ) * nf;
    mat->m[2][3] = ( 2.0f * zfar * znear ) * nf;
    mat->m[3][2] = -1.0f;
}

void wp_mat4f_ortho( wp_mat4f *mat, wp_f32 left, wp_f32 right, wp_f32 bottom, wp_f32 top, wp_f32 znear,
                     wp_f32 zfar )
{
    wp_f32 rl = 1.0f / ( right - left );
    wp_f32 tb = 1.0f / ( top - bottom );
    wp_f32 fn = 1.0f / ( zfar - znear );

    wp_mat4f_zero( mat );
    mat->m[0][0] = 2.0f * rl;
    mat->m[1][1] = 2.0f * tb;
    mat->m[2][2] = -2.0f * fn;
    mat->m[0][3] = -( right + left ) * rl;
    mat->m[1][3] = -( top + bottom ) * tb;
    mat->m[2][3] = -( zfar + znear ) * fn;
    mat->m[3][3] = 1.0f;
}
