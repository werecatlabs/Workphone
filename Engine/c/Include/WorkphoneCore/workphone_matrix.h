/**
 * @file wp_matrix.h
 * @brief C API for 3x3 and 4x4 matrix operations.
 */

#ifndef WORKPHONE_MATRIX_H
#define WORKPHONE_MATRIX_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_mat3f
{
    wp_f32 m[3][3];
} wp_mat3f;

typedef struct wp_mat4f
{
    wp_f32 m[4][4];
} wp_mat4f;

typedef struct wp_mat3d
{
    wp_f64 m[3][3];
} wp_mat3d;

typedef struct wp_mat4d
{
    wp_f64 m[4][4];
} wp_mat4d;

void wp_mat3f_identity( wp_mat3f *mat );
void wp_mat3f_zero( wp_mat3f *mat );
void wp_mat3f_set( wp_mat3f *mat, wp_f32 m00, wp_f32 m01, wp_f32 m02, wp_f32 m10, wp_f32 m11, wp_f32 m12,
                   wp_f32 m20, wp_f32 m21, wp_f32 m22 );
void wp_mat3f_copy( wp_mat3f *dest, const wp_mat3f *src );
void wp_mat3f_add( wp_mat3f *result, const wp_mat3f *a, const wp_mat3f *b );
void wp_mat3f_sub( wp_mat3f *result, const wp_mat3f *a, const wp_mat3f *b );
void wp_mat3f_mul( wp_mat3f *result, const wp_mat3f *a, const wp_mat3f *b );
void wp_mat3f_mul_scalar( wp_mat3f *result, const wp_mat3f *mat, wp_f32 scalar );
void wp_mat3f_transpose( wp_mat3f *result, const wp_mat3f *mat );
wp_f32 wp_mat3f_determinant( const wp_mat3f *mat );
wp_s32 wp_mat3f_invert( wp_mat3f *result, const wp_mat3f *mat );
void wp_mat3f_from_rotation_x( wp_mat3f *mat, wp_f32 angle );
void wp_mat3f_from_rotation_y( wp_mat3f *mat, wp_f32 angle );
void wp_mat3f_from_rotation_z( wp_mat3f *mat, wp_f32 angle );
void wp_mat3f_from_scale( wp_mat3f *mat, wp_f32 sx, wp_f32 sy, wp_f32 sz );

void wp_mat4f_identity( wp_mat4f *mat );
void wp_mat4f_zero( wp_mat4f *mat );
void wp_mat4f_set( wp_mat4f *mat, wp_f32 m00, wp_f32 m01, wp_f32 m02, wp_f32 m03, wp_f32 m10, wp_f32 m11,
                   wp_f32 m12, wp_f32 m13, wp_f32 m20, wp_f32 m21, wp_f32 m22, wp_f32 m23, wp_f32 m30,
                   wp_f32 m31, wp_f32 m32, wp_f32 m33 );
void wp_mat4f_copy( wp_mat4f *dest, const wp_mat4f *src );
void wp_mat4f_add( wp_mat4f *result, const wp_mat4f *a, const wp_mat4f *b );
void wp_mat4f_sub( wp_mat4f *result, const wp_mat4f *a, const wp_mat4f *b );
void wp_mat4f_mul( wp_mat4f *result, const wp_mat4f *a, const wp_mat4f *b );
void wp_mat4f_mul_scalar( wp_mat4f *result, const wp_mat4f *mat, wp_f32 scalar );
void wp_mat4f_transpose( wp_mat4f *result, const wp_mat4f *mat );
wp_f32 wp_mat4f_determinant( const wp_mat4f *mat );
wp_s32 wp_mat4f_invert( wp_mat4f *result, const wp_mat4f *mat );
void wp_mat4f_from_translation( wp_mat4f *mat, wp_f32 x, wp_f32 y, wp_f32 z );
void wp_mat4f_from_rotation_x( wp_mat4f *mat, wp_f32 angle );
void wp_mat4f_from_rotation_y( wp_mat4f *mat, wp_f32 angle );
void wp_mat4f_from_rotation_z( wp_mat4f *mat, wp_f32 angle );
void wp_mat4f_from_scale( wp_mat4f *mat, wp_f32 sx, wp_f32 sy, wp_f32 sz );
void wp_mat4f_look_at( wp_mat4f *mat, wp_f32 eyeX, wp_f32 eyeY, wp_f32 eyeZ, wp_f32 centerX,
                       wp_f32 centerY, wp_f32 centerZ, wp_f32 upX, wp_f32 upY, wp_f32 upZ );
void wp_mat4f_perspective( wp_mat4f *mat, wp_f32 fovy, wp_f32 aspect, wp_f32 znear, wp_f32 zfar );
void wp_mat4f_ortho( wp_mat4f *mat, wp_f32 left, wp_f32 right, wp_f32 bottom, wp_f32 top, wp_f32 znear,
                     wp_f32 zfar );

void wp_mat3d_identity( wp_mat3d *mat );
void wp_mat3d_zero( wp_mat3d *mat );
void wp_mat3d_set( wp_mat3d *mat, wp_f64 m00, wp_f64 m01, wp_f64 m02, wp_f64 m10, wp_f64 m11, wp_f64 m12,
                   wp_f64 m20, wp_f64 m21, wp_f64 m22 );
void wp_mat3d_copy( wp_mat3d *dest, const wp_mat3d *src );
void wp_mat3d_add( wp_mat3d *result, const wp_mat3d *a, const wp_mat3d *b );
void wp_mat3d_sub( wp_mat3d *result, const wp_mat3d *a, const wp_mat3d *b );
void wp_mat3d_mul( wp_mat3d *result, const wp_mat3d *a, const wp_mat3d *b );
void wp_mat3d_mul_scalar( wp_mat3d *result, const wp_mat3d *mat, wp_f64 scalar );
void wp_mat3d_transpose( wp_mat3d *result, const wp_mat3d *mat );
wp_f64 wp_mat3d_determinant( const wp_mat3d *mat );
wp_s32 wp_mat3d_invert( wp_mat3d *result, const wp_mat3d *mat );
void wp_mat3d_from_rotation_x( wp_mat3d *mat, wp_f64 angle );
void wp_mat3d_from_rotation_y( wp_mat3d *mat, wp_f64 angle );
void wp_mat3d_from_rotation_z( wp_mat3d *mat, wp_f64 angle );
void wp_mat3d_from_scale( wp_mat3d *mat, wp_f64 sx, wp_f64 sy, wp_f64 sz );

void wp_mat4d_identity( wp_mat4d *mat );
void wp_mat4d_zero( wp_mat4d *mat );
void wp_mat4d_set( wp_mat4d *mat, wp_f64 m00, wp_f64 m01, wp_f64 m02, wp_f64 m03, wp_f64 m10, wp_f64 m11,
                   wp_f64 m12, wp_f64 m13, wp_f64 m20, wp_f64 m21, wp_f64 m22, wp_f64 m23, wp_f64 m30,
                   wp_f64 m31, wp_f64 m32, wp_f64 m33 );
void wp_mat4d_copy( wp_mat4d *dest, const wp_mat4d *src );
void wp_mat4d_add( wp_mat4d *result, const wp_mat4d *a, const wp_mat4d *b );
void wp_mat4d_sub( wp_mat4d *result, const wp_mat4d *a, const wp_mat4d *b );
void wp_mat4d_mul( wp_mat4d *result, const wp_mat4d *a, const wp_mat4d *b );
void wp_mat4d_mul_scalar( wp_mat4d *result, const wp_mat4d *mat, wp_f64 scalar );
void wp_mat4d_transpose( wp_mat4d *result, const wp_mat4d *mat );
wp_f64 wp_mat4d_determinant( const wp_mat4d *mat );
wp_s32 wp_mat4d_invert( wp_mat4d *result, const wp_mat4d *mat );
void wp_mat4d_from_translation( wp_mat4d *mat, wp_f64 x, wp_f64 y, wp_f64 z );
void wp_mat4d_from_rotation_x( wp_mat4d *mat, wp_f64 angle );
void wp_mat4d_from_rotation_y( wp_mat4d *mat, wp_f64 angle );
void wp_mat4d_from_rotation_z( wp_mat4d *mat, wp_f64 angle );
void wp_mat4d_from_scale( wp_mat4d *mat, wp_f64 sx, wp_f64 sy, wp_f64 sz );
void wp_mat4d_look_at( wp_mat4d *mat, wp_f64 eyeX, wp_f64 eyeY, wp_f64 eyeZ, wp_f64 centerX,
                       wp_f64 centerY, wp_f64 centerZ, wp_f64 upX, wp_f64 upY, wp_f64 upZ );
void wp_mat4d_perspective( wp_mat4d *mat, wp_f64 fovy, wp_f64 aspect, wp_f64 znear, wp_f64 zfar );
void wp_mat4d_ortho( wp_mat4d *mat, wp_f64 left, wp_f64 right, wp_f64 bottom, wp_f64 top, wp_f64 znear,
                     wp_f64 zfar );

#ifdef __cplusplus
}
#endif

#endif
