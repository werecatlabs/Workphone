#define COBJMACROS
#include <d3d11.h>
#include "workphone_graphics_particle_compute_dx11.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"GPU particles line %d: %s\n",__LINE__,#x); return 1; } } while(0)

int main(void)
{
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    ID3D11Buffer *constant, *retained;
    ID3D11ComputeShader *shader;
    D3D11_BUFFER_DESC description;
    D3D_FEATURE_LEVEL feature;
    D3D_DRIVER_TYPE driver;
    wp_particle_compute_dx11 *compute;
    wp_particle_simulation *cpu, *gpu;
    wp_particle_simulation_settings settings;
    const wp_particle_sample *a, *b;
    wp_particle_sample paused;
    wp_u32 frame, index, channel, dispatches;
    HRESULT result;
    device=NULL; context=NULL; driver=D3D_DRIVER_TYPE_HARDWARE;
    if(getenv("WP_PARTICLE_TEST_WARP")) driver=D3D_DRIVER_TYPE_WARP;
    result=D3D11CreateDevice(NULL,driver,NULL,0,NULL,0,D3D11_SDK_VERSION,&device,&feature,&context);
    if(FAILED(result) && driver==D3D_DRIVER_TYPE_HARDWARE)
    {
        driver=D3D_DRIVER_TYPE_WARP;
        result=D3D11CreateDevice(NULL,driver,NULL,0,NULL,0,D3D11_SDK_VERSION,&device,&feature,&context);
    }
    if(FAILED(result) || feature<D3D_FEATURE_LEVEL_11_0)
    {
        if(context) ID3D11DeviceContext_Release(context);
        if(device) ID3D11Device_Release(device);
        puts("SKIP: no DX11 feature-level 11 compute device"); return 77;
    }
    compute=wp_particle_compute_dx11_create(device,context,256); CHECK(compute);
    cpu=wp_particle_simulation_create(256,42); gpu=wp_particle_simulation_create(256,42); CHECK(cpu&&gpu);
    wp_particle_simulation_default_settings(&settings); settings.rate=120;
    settings.lifetime_min=.3f; settings.lifetime_max=.7f;
    settings.gravity.x=-1; settings.gravity.y=-9.8f; settings.gravity.z=2;
    settings.velocity.x=3; settings.velocity.y=5; settings.velocity.z=-2;
    settings.color_end[0]=0; settings.color_end[1]=.25f; settings.color_end[2]=.5f; settings.color_end[3]=0;
    CHECK(wp_particle_simulation_configure(cpu,&settings)); CHECK(wp_particle_simulation_configure(gpu,&settings));
    CHECK(wp_particle_simulation_set_state(cpu,WORKPHONE_PARTICLE_STATE_STARTED));
    CHECK(wp_particle_simulation_set_state(gpu,WORKPHONE_PARTICLE_STATE_STARTED));
    wp_particle_simulation_set_integrator(gpu,wp_particle_compute_dx11_integrate,compute);
    memset(&description,0,sizeof(description)); description.ByteWidth=64;
    description.Usage=D3D11_USAGE_DEFAULT; description.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    constant=NULL; CHECK(SUCCEEDED(ID3D11Device_CreateBuffer(device,&description,NULL,&constant)));
    ID3D11DeviceContext_CSSetConstantBuffers(context,0,1,&constant);
    for(frame=0;frame<60;++frame)
    {
        CHECK(wp_particle_simulation_advance(cpu,1.0f/60)); CHECK(wp_particle_simulation_advance(gpu,1.0f/60));
        CHECK(wp_particle_simulation_get_count(cpu)==wp_particle_simulation_get_count(gpu));
        a=wp_particle_simulation_get_samples(cpu); b=wp_particle_simulation_get_samples(gpu);
        for(index=0;index<wp_particle_simulation_get_count(cpu);++index)
        {
            CHECK(fabs(a[index].age-b[index].age)<1e-6);
            CHECK(fabs(a[index].position.x-b[index].position.x)<1e-4);
            CHECK(fabs(a[index].position.y-b[index].position.y)<1e-4);
            CHECK(fabs(a[index].position.z-b[index].position.z)<1e-4);
            CHECK(fabs(a[index].velocity.y-b[index].velocity.y)<1e-4);
            CHECK(a[index].lifetime==b[index].lifetime && a[index].size==b[index].size);
            for(channel=0;channel<4;++channel) CHECK(fabs(a[index].color[channel]-b[index].color[channel])<1e-5);
        }
    }
    dispatches=wp_particle_compute_dx11_get_dispatches(compute); CHECK(dispatches>0);
    CHECK(wp_particle_simulation_get_accelerated_steps(gpu)==dispatches);
    CHECK(wp_particle_simulation_get_accelerator_failures(gpu)==0);
    retained=NULL; ID3D11DeviceContext_CSGetConstantBuffers(context,0,1,&retained); CHECK(retained==constant);
    if(retained) ID3D11Buffer_Release(retained);
    shader=NULL; ID3D11DeviceContext_CSGetShader(context,&shader,NULL,NULL); CHECK(shader==NULL);
    paused=wp_particle_simulation_get_samples(gpu)[0];
    CHECK(wp_particle_simulation_set_state(gpu,WORKPHONE_PARTICLE_STATE_PAUSED));
    CHECK(wp_particle_simulation_advance(gpu,1));
    CHECK(memcmp(&paused,wp_particle_simulation_get_samples(gpu),sizeof(paused))==0);
    CHECK(wp_particle_compute_dx11_get_dispatches(compute)==dispatches);
    CHECK(wp_particle_simulation_set_state(gpu,WORKPHONE_PARTICLE_STATE_STARTED));
    wp_particle_simulation_set_integrator(gpu,wp_particle_compute_dx11_integrate,NULL);
    CHECK(wp_particle_simulation_advance(cpu,1.0f/60)); CHECK(wp_particle_simulation_advance(gpu,1.0f/60));
    CHECK(wp_particle_simulation_get_accelerator_failures(gpu)>0);
    CHECK(wp_particle_simulation_get_count(cpu)==wp_particle_simulation_get_count(gpu));
    a=wp_particle_simulation_get_samples(cpu); b=wp_particle_simulation_get_samples(gpu);
    CHECK(fabs(a[0].position.y-b[0].position.y)<1e-4);
    wp_particle_simulation_destroy(cpu); wp_particle_simulation_destroy(gpu);
    wp_particle_compute_dx11_destroy(compute);
    retained=NULL; ID3D11DeviceContext_CSSetConstantBuffers(context,0,1,&retained);
    ID3D11Buffer_Release(constant); ID3D11DeviceContext_Release(context); ID3D11Device_Release(device);
    printf("PASS: %s actual compute dispatches=%u; CPU trajectories/lifetime/color, pause, state restoration and fallback\n",
        driver==D3D_DRIVER_TYPE_WARP?"WARP":"hardware",dispatches);
    return 0;
}
