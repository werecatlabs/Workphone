#include "workphone_graphics_light_grid.h"
#include <stdio.h>
#include <string.h>
#define REQUIRE(condition) do{if(!(condition)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#condition);return 1;}}while(0)
static wp_u32 cells[16*9*16*2],indices[16*9*16*32];
static int contains( wp_u32 x, wp_u32 z, wp_u32 light, wp_u32 gz )
{
    wp_u32 cell=(z*9+4)*16+x,i; (void)gz;
    for(i=0;i<cells[cell*2+1];++i)if(indices[cells[cell*2]+i]==light)return 1;
    return 0;
}
int main(void)
{
    wp_light_grid_camera camera;wp_local_light lights[40];wp_u32 count,dropped,i,cell;
    memset(&camera,0,sizeof(camera));memset(lights,0,sizeof(lights));
    camera.view_projection[0]=camera.view_projection[5]=camera.view_projection[10]=camera.view_projection[14]=1;
    camera.forward[2]=1;camera.z_near=1;camera.z_far=17;
    for(i=0;i<40;++i){lights[i].radius=.2f;lights[i].radiance[0]=1;}
    lights[0].position[0]=-2;lights[0].position[2]=4;
    lights[1].position[0]=2;lights[1].position[2]=4;
    lights[2].position[0]=-6;lights[2].position[2]=12;
    lights[3].position[2]=-4;lights[4].position[0]=100;lights[4].position[2]=4;
    REQUIRE(wp_light_grid_build(lights,5,&camera,16,9,1,cells,16*9,indices,16*9*32,&count,&dropped));
    REQUIRE(count>0 && dropped==0);REQUIRE(contains(4,0,0,1));REQUIRE(contains(4,0,2,1));
    REQUIRE(!contains(4,0,1,1));REQUIRE(contains(12,0,1,1));
    for(i=0;i<count;++i)REQUIRE(indices[i]<3);
    REQUIRE(wp_light_grid_build(lights,5,&camera,16,9,16,cells,16*9*16,indices,16*9*16*32,&count,&dropped));
    REQUIRE(contains(4,3,0,16));REQUIRE(!contains(4,3,2,16));
    REQUIRE(contains(4,11,2,16));REQUIRE(!contains(4,11,0,16));
    REQUIRE(contains(12,3,1,16));REQUIRE(!contains(12,3,0,16));
    for(i=1;i<40;++i)lights[i]=lights[0];
    REQUIRE(wp_light_grid_build(lights,40,&camera,16,9,16,cells,16*9*16,indices,16*9*16*32,&count,&dropped));
    REQUIRE(dropped>0);for(cell=0;cell<16*9*16;++cell)REQUIRE(cells[cell*2+1]<=32);
    REQUIRE(contains(4,3,31,16));REQUIRE(!contains(4,3,32,16));
    lights[0].position[0]=0;lights[0].position[2]=1;lights[0].radius=2;
    REQUIRE(wp_light_grid_build(lights,1,&camera,16,9,16,cells,16*9*16,indices,16*9*16*32,&count,&dropped));
    REQUIRE(contains(0,0,0,16));REQUIRE(contains(15,0,0,16));
    cells[0]=12345;camera.forward[2]=0;
    REQUIRE(!wp_light_grid_build(lights,1,&camera,16,9,16,cells,16*9*16,indices,16*9*16*32,&count,&dropped));
    REQUIRE(cells[0]==12345);camera.forward[2]=1;
    REQUIRE(wp_light_grid_build(0,0,&camera,16,9,16,cells,16*9*16,indices,16*9*16*32,&count,&dropped));
    REQUIRE(count==0 && dropped==0);for(cell=0;cell<16*9*16;++cell)REQUIRE(cells[cell*2+1]==0);
    puts("PASS: Forward+ XY separation, clustered depth separation, near-plane conservative coverage and bounded overflow");return 0;
}
