#include "workphone_graphics_irradiance.h"
#include <math.h>
#include <float.h>
#include <string.h>

static wp_f32 gi_dot( const wp_f32 a[3], const wp_f32 b[3] )
{ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
static wp_f32 gi_normalize( wp_f32 a[3] )
{
    wp_f32 length;
    length=(wp_f32)sqrt(gi_dot(a,a));
    if(length>1e-8f) {a[0]/=length;a[1]/=length;a[2]/=length;}
    return length;
}
static void gi_basis( const wp_f32 d[3], wp_f32 y[9] )
{
    y[0]=0.282094792f;y[1]=0.488602512f*d[1];y[2]=0.488602512f*d[2];y[3]=0.488602512f*d[0];
    y[4]=1.092548431f*d[0]*d[1];y[5]=1.092548431f*d[1]*d[2];
    y[6]=0.315391565f*(3*d[2]*d[2]-1);y[7]=1.092548431f*d[0]*d[2];
    y[8]=0.546274215f*(d[0]*d[0]-d[1]*d[1]);
}
static void gi_radiance( const wp_irradiance_scene *scene, const wp_f32 origin[3],
    const wp_f32 direction[3], wp_f32 distance, wp_s32 selection, wp_s32 environment, wp_f32 result[3] )
{
    wp_irradiance_hit hit, blocker;
    wp_f32 incoming[3], light_direction[3], shifted[3], light_distance, weight, attenuation;
    wp_u32 i,c;
    memset(result,0,3*sizeof(*result));memset(&hit,0,sizeof(hit));
    if(!scene->trace || !scene->trace(scene->context,origin,direction,distance,&hit))
    { if(environment && scene->environment)scene->environment(scene->context,direction,result);return; }
    gi_normalize(hit.normal);
    for(c=0;c<3;++c) {result[c]=selection==2?0:hit.emission[c];shifted[c]=hit.position[c]+hit.normal[c]*0.002f;}
    if(environment && scene->environment)
    {
        scene->environment(scene->context,hit.normal,incoming);
        for(c=0;c<3;++c)result[c]+=hit.albedo[c]*incoming[c];
    }
    for(i=0;i<scene->light_count;++i)
    {
        const wp_irradiance_light *light;
        light=&scene->lights[i];
        if((selection==1 && light->dynamic) || (selection==2 && !light->dynamic))continue;
        if(light->directional)
        {for(c=0;c<3;++c)light_direction[c]=-light->direction[c];light_distance=distance;attenuation=1;}
        else
        {
            for(c=0;c<3;++c)light_direction[c]=light->position[c]-hit.position[c];
            light_distance=gi_normalize(light_direction);
            if(light_distance<=0.002f || (light->range>0 && light_distance>=light->range))continue;
            attenuation=1.0f/(1.0f+light_distance*light_distance);
            if(light->range>0) {weight=1-light_distance/light->range;attenuation*=weight*weight;}
        }
        gi_normalize(light_direction);weight=gi_dot(hit.normal,light_direction);
        if(weight<=0)continue;
        memset(&blocker,0,sizeof(blocker));
        if(scene->trace(scene->context,shifted,light_direction,light_distance-0.002f,&blocker))continue;
        for(c=0;c<3;++c)result[c]+=hit.albedo[c]*light->radiance[c]*weight*attenuation/3.141592654f;
    }
}
wp_s32 wp_irradiance_bake_probe( const wp_irradiance_scene *scene, const wp_f32 position[3],
    wp_u32 rays, wp_f32 distance, wp_s32 selection, wp_s32 environment, wp_irradiance_probe *probe )
{
    wp_irradiance_probe result;
    wp_f32 direction[3], radiance[3], basis[9], z, radius, phi, convolution;
    wp_u32 i,j,c;
    if(!scene || !position || !probe || !rays || rays>4096 || !scene->trace ||
        !(distance>0 && distance<=1e6f) || selection<0 || selection>2 ||
        (scene->light_count && !scene->lights))return 0;
    memset(&result,0,sizeof(result));
    for(c=0;c<3;++c){if(!(position[c]>=-FLT_MAX && position[c]<=FLT_MAX))return 0;result.position[c]=position[c];}
    for(i=0;i<rays;++i)
    {
        z=1.0f-2.0f*((wp_f32)i+0.5f)/(wp_f32)rays;radius=(wp_f32)sqrt(1.0f-z*z);
        phi=(wp_f32)i*2.39996323f;direction[0]=radius*(wp_f32)cos(phi);direction[1]=radius*(wp_f32)sin(phi);direction[2]=z;
        gi_radiance(scene,position,direction,distance,selection,environment,radiance);gi_basis(direction,basis);
        for(j=0;j<9;++j)
        {
            convolution=j==0?3.141592654f:j<4?2.094395102f:0.785398163f;
            for(c=0;c<3;++c)
            {
                if(!(radiance[c]>=0 && radiance[c]<=1e12f))return 0;
                result.sh[j][c]+=radiance[c]*basis[j]*(12.56637061f/(wp_f32)rays)*convolution;
            }
        }
    }
    *probe=result;return 1;
}
void wp_irradiance_evaluate( const wp_irradiance_probe *probe, const wp_f32 normal[3], wp_f32 irradiance[3] )
{
    wp_f32 direction[3],basis[9];wp_u32 i,c;
    if(!irradiance)return;memset(irradiance,0,3*sizeof(*irradiance));if(!probe || !normal)return;
    memcpy(direction,normal,sizeof(direction));if(gi_normalize(direction)<1e-8f)return;gi_basis(direction,basis);
    for(i=0;i<9;++i)for(c=0;c<3;++c)irradiance[c]+=probe->sh[i][c]*basis[i];
    for(c=0;c<3;++c)if(irradiance[c]<0)irradiance[c]=0;
}
void wp_irradiance_combine( const wp_irradiance_probe *a, const wp_irradiance_probe *b, wp_irradiance_probe *out )
{
    wp_u32 i,c;if(!a || !b || !out)return;*out=*a;
    for(i=0;i<9;++i)for(c=0;c<3;++c)out->sh[i][c]=a->sh[i][c]+b->sh[i][c];
}
