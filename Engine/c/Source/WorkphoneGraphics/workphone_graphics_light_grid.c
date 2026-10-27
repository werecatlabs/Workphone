#include "workphone_graphics_light_grid.h"
#include <math.h>
#include <float.h>

typedef struct wp_light_bounds { wp_s32 first[3], last[3], visible; } wp_light_bounds;
static int wp_grid_finite( wp_f32 v ) { return v==v && v>=-FLT_MAX && v<=FLT_MAX; }
static wp_f32 wp_grid_dot( const wp_f32 *a, const wp_f32 *b )
{ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
static wp_s32 wp_grid_index( wp_f32 unit, wp_u32 count )
{
    wp_s32 result=(wp_s32)floor((double)(unit*(wp_f32)count));
    if(result<0)return 0;if(result>=(wp_s32)count)return (wp_s32)count-1;return result;
}
static void wp_grid_ratio( wp_f32 center, wp_f32 radius, wp_f32 wmin, wp_f32 wmax,
                           wp_f32 *minimum, wp_f32 *maximum )
{
    wp_f32 values[4];wp_u32 i;
    values[0]=(center-radius)/wmin;values[1]=(center-radius)/wmax;
    values[2]=(center+radius)/wmin;values[3]=(center+radius)/wmax;
    *minimum=*maximum=values[0];for(i=1;i<4;++i){if(values[i]<*minimum)*minimum=values[i];if(values[i]>*maximum)*maximum=values[i];}
}
wp_s32 wp_light_grid_build( const wp_local_light *lights, wp_u32 count,
    const wp_light_grid_camera *camera, wp_u32 gx, wp_u32 gy, wp_u32 gz,
    wp_u32 *cells, wp_u32 cell_capacity, wp_u32 *indices, wp_u32 index_capacity,
    wp_u32 *index_count, wp_u32 *discarded )
{
    wp_light_bounds bounds[WP_LIGHT_GRID_MAX_LIGHTS];
    wp_u32 i,j,x,y,z,cell,offset,admitted,lost,total_cells;
    wp_f32 delta[3],depth,zmin,zmax,clip[4],spread[4],wmin,wmax;
    wp_f32 xmin,xmax,ymin,ymax,forward_length;
    if(!camera || !cells || !indices || !index_count || !discarded ||
       count>WP_LIGHT_GRID_MAX_LIGHTS || (count && !lights) || !gx || !gy || !gz ||
       gx>WP_LIGHT_GRID_MAX_X || gy>WP_LIGHT_GRID_MAX_Y || gz>WP_LIGHT_GRID_MAX_Z)return 0;
    total_cells=gx*gy*gz;
    if(cell_capacity<total_cells || index_capacity<total_cells*WP_LIGHT_GRID_CELL_LIGHTS)return 0;
    for(i=0;i<16;++i)if(!wp_grid_finite(camera->view_projection[i]))return 0;
    for(i=0;i<3;++i)if(!wp_grid_finite(camera->position[i]) || !wp_grid_finite(camera->forward[i]))return 0;
    forward_length=wp_grid_dot(camera->forward,camera->forward);
    if(forward_length<.99f || forward_length>1.01f || !wp_grid_finite(camera->z_near) ||
       !wp_grid_finite(camera->z_far) || camera->z_near<=0 || camera->z_far<=camera->z_near)return 0;
    for(i=0;i<count;++i)
    {
        const wp_f32 *data=(const wp_f32 *)&lights[i];
        for(j=0;j<8;++j)if(!wp_grid_finite(data[j]))return 0;
        if(lights[i].radius<=0 || lights[i].radiance[0]<0 || lights[i].radiance[1]<0 || lights[i].radiance[2]<0)return 0;
    }
    for(i=0;i<count;++i)
    {
        bounds[i].visible=0;
        for(j=0;j<3;++j)delta[j]=lights[i].position[j]-camera->position[j];
        depth=wp_grid_dot(delta,camera->forward);zmin=depth-lights[i].radius;zmax=depth+lights[i].radius;
        if(zmax<camera->z_near || zmin>camera->z_far)continue;
        for(j=0;j<4;++j)
        {
            const wp_f32 *row=camera->view_projection+j*4;
            clip[j]=wp_grid_dot(row,lights[i].position)+row[3];
            spread[j]=lights[i].radius*(wp_f32)sqrt((double)wp_grid_dot(row,row));
        }
        wmin=clip[3]-spread[3];wmax=clip[3]+spread[3];if(wmax<=0)continue;
        xmin=ymin=-1;xmax=ymax=1;
        if(wmin>0.00001f && zmin>camera->z_near)
        {
            wp_grid_ratio(clip[0],spread[0],wmin,wmax,&xmin,&xmax);
            wp_grid_ratio(clip[1],spread[1],wmin,wmax,&ymin,&ymax);
            if(xmax < -1 || xmin > 1 || ymax < -1 || ymin > 1)continue;
        }
        bounds[i].first[0]=wp_grid_index((xmin+1)*.5f-.00001f,gx);
        bounds[i].last[0]=wp_grid_index((xmax+1)*.5f+.00001f,gx);
        bounds[i].first[1]=wp_grid_index((1-ymax)*.5f-.00001f,gy);
        bounds[i].last[1]=wp_grid_index((1-ymin)*.5f+.00001f,gy);
        bounds[i].first[2]=wp_grid_index((zmin-camera->z_near)/(camera->z_far-camera->z_near),gz);
        bounds[i].last[2]=wp_grid_index((zmax-camera->z_near)/(camera->z_far-camera->z_near),gz);
        bounds[i].visible=1;
    }
    offset=lost=cell=0;
    for(z=0;z<gz;++z)for(y=0;y<gy;++y)for(x=0;x<gx;++x)
    {
        cells[cell*2]=offset;admitted=0;
        for(i=0;i<count;++i)if(bounds[i].visible && (wp_s32)x>=bounds[i].first[0] && (wp_s32)x<=bounds[i].last[0] &&
            (wp_s32)y>=bounds[i].first[1] && (wp_s32)y<=bounds[i].last[1] &&
            (wp_s32)z>=bounds[i].first[2] && (wp_s32)z<=bounds[i].last[2])
        {if(admitted<WP_LIGHT_GRID_CELL_LIGHTS){indices[offset++]=i;++admitted;}else ++lost;}
        cells[cell*2+1]=admitted;++cell;
    }
    *index_count=offset;*discarded=lost;return 1;
}
