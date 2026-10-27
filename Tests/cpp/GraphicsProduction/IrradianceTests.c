#include "workphone_graphics_irradiance.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"irradiance line %d: %s\n",__LINE__,#x);return 1;}}while(0)
typedef struct TestScene { float albedo[3]; int plane,blockShadows; unsigned int rays,shadowRays; } TestScene;
static wp_s32 trace(void *opaque,const float origin[3],const float direction[3],float maximum,wp_irradiance_hit *hit)
{
    TestScene *scene;float distance;unsigned int c;scene=(TestScene *)opaque;++scene->rays;
    if(origin[1]<.01f && direction[1]>0)
    {++scene->shadowRays;if(scene->blockShadows){memset(hit,0,sizeof(*hit));hit->distance=.1f;return 1;}}
    if(!scene->plane || direction[1]>=-1e-6f)return 0;
    distance=-origin[1]/direction[1];if(distance<=.0001f || distance>maximum)return 0;
    memset(hit,0,sizeof(*hit));hit->normal[1]=1;hit->distance=distance;
    for(c=0;c<3;++c){hit->position[c]=origin[c]+direction[c]*distance;hit->albedo[c]=scene->albedo[c];}return 1;
}
static void environment(void *context,const float direction[3],float radiance[3])
{(void)context;(void)direction;radiance[0]=.25f;radiance[1]=.5f;radiance[2]=1;}
int main(void)
{
    TestScene scene;wp_irradiance_scene input;wp_irradiance_light lights[2];
    wp_irradiance_probe red,blue,blocked,again,sky,staticLight,dynamicLight,combined,all;
    float position[3]={0,1,0},down[3]={0,-1,0},up[3]={0,1,0},a[3],b[3];unsigned int i,c;
    memset(&scene,0,sizeof(scene));memset(&input,0,sizeof(input));memset(lights,0,sizeof(lights));
    input.context=&scene;input.trace=trace;input.environment=environment;input.lights=lights;input.light_count=1;
    lights[0].directional=1;lights[0].direction[1]=-1;lights[0].radiance[0]=lights[0].radiance[1]=lights[0].radiance[2]=4;
    scene.plane=1;scene.albedo[0]=1;
    CHECK(sizeof(wp_irradiance_probe)==160);
    CHECK(wp_irradiance_bake_probe(&input,position,2048,32,0,0,&red));
    CHECK(wp_irradiance_bake_probe(&input,position,2048,32,0,0,&again));CHECK(memcmp(&red,&again,sizeof(red))==0);
    wp_irradiance_evaluate(&red,down,a);CHECK(a[0]>3.5f && a[1]<1e-6f && a[2]<1e-6f);CHECK(scene.shadowRays>0);
    scene.albedo[0]=0;scene.albedo[2]=1;
    CHECK(wp_irradiance_bake_probe(&input,position,2048,32,0,0,&blue));wp_irradiance_evaluate(&blue,down,b);
    CHECK(b[2]>3.5f && b[0]<1e-6f && b[1]<1e-6f);
    scene.blockShadows=1;CHECK(wp_irradiance_bake_probe(&input,position,2048,32,0,0,&blocked));
    wp_irradiance_evaluate(&blocked,down,a);CHECK(a[0]+a[1]+a[2]<1e-6f);
    scene.plane=0;scene.blockShadows=0;input.light_count=0;
    CHECK(wp_irradiance_bake_probe(&input,position,2048,32,0,1,&sky));wp_irradiance_evaluate(&sky,up,a);
    CHECK(fabs(a[0]-.25*3.141592654)<.02 && fabs(a[1]-.5*3.141592654)<.02 && fabs(a[2]-3.141592654)<.02);
    scene.plane=1;scene.albedo[0]=scene.albedo[1]=scene.albedo[2]=1;input.light_count=2;
    lights[1]=lights[0];lights[1].dynamic=1;lights[1].radiance[0]=1;lights[1].radiance[1]=2;lights[1].radiance[2]=3;
    CHECK(wp_irradiance_bake_probe(&input,position,512,32,1,0,&staticLight));
    CHECK(wp_irradiance_bake_probe(&input,position,512,32,2,0,&dynamicLight));
    CHECK(wp_irradiance_bake_probe(&input,position,512,32,0,0,&all));wp_irradiance_combine(&staticLight,&dynamicLight,&combined);
    for(i=0;i<9;++i)for(c=0;c<3;++c)CHECK(fabs(combined.sh[i][c]-all.sh[i][c])<.0001f);
    CHECK(!wp_irradiance_bake_probe(&input,position,0,32,0,0,&all));
    puts("PASS: actual colored diffuse bounce, occluded light visibility, deterministic SH9, sky convolution and static/dynamic Hybrid additivity");return 0;
}
