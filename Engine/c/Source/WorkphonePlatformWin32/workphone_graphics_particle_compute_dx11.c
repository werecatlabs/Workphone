#define COBJMACROS
#include "workphone_graphics_particle_compute_dx11.h"
#include <stdlib.h>
#include <string.h>
#if defined( _WIN32 )
#include <d3d11.h>
#include <d3dcompiler.h>

struct wp_particle_compute_dx11
{
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    ID3D11ComputeShader *shader;
    ID3D11Buffer *particles, *readback, *constants;
    ID3D11UnorderedAccessView *view;
    wp_u32 capacity, dispatches;
};

static const char particle_shader[] =
    "struct Particle { float3 position; float3 velocity; float age; float lifetime; float size; float4 color; };"
    "RWStructuredBuffer<Particle> particles : register(u0);"
    "cbuffer Settings : register(b0) { float3 gravity; float dt; float4 colorStart; float4 colorEnd; uint count; float3 padding; };"
    "[numthreads(64,1,1)] void main(uint3 id : SV_DispatchThreadID) {"
    " if(id.x >= count) return; Particle p=particles[id.x]; p.age += dt;"
    " p.position += p.velocity*dt + 0.5*gravity*dt*dt; p.velocity += gravity*dt;"
    " p.color = colorStart + (colorEnd-colorStart)*(p.age/p.lifetime); particles[id.x]=p; }";

void wp_particle_compute_dx11_destroy( wp_particle_compute_dx11 *s )
{
    if( !s ) return;
    if( s->view ) ID3D11UnorderedAccessView_Release( s->view );
    if( s->constants ) ID3D11Buffer_Release( s->constants );
    if( s->readback ) ID3D11Buffer_Release( s->readback );
    if( s->particles ) ID3D11Buffer_Release( s->particles );
    if( s->shader ) ID3D11ComputeShader_Release( s->shader );
    if( s->context ) ID3D11DeviceContext_Release( s->context );
    if( s->device ) ID3D11Device_Release( s->device );
    free( s );
}

wp_particle_compute_dx11 *wp_particle_compute_dx11_create( void *device, void *context, wp_u32 capacity )
{
    wp_particle_compute_dx11 *s;
    ID3DBlob *code, *error;
    D3D11_BUFFER_DESC buffer;
    D3D11_UNORDERED_ACCESS_VIEW_DESC view;
    HRESULT status;
    if( !device || !context || !capacity || capacity > WP_PARTICLE_SIMULATION_MAX_CAPACITY ||
        ID3D11Device_GetFeatureLevel( (ID3D11Device *)device ) < D3D_FEATURE_LEVEL_11_0 ) return NULL;
    s = (wp_particle_compute_dx11 *)calloc( 1, sizeof(*s) );
    if( !s ) return NULL;
    s->device = (ID3D11Device *)device; ID3D11Device_AddRef( s->device );
    s->context = (ID3D11DeviceContext *)context; ID3D11DeviceContext_AddRef( s->context );
    s->capacity = capacity; code = error = NULL;
    status = D3DCompile( particle_shader, sizeof(particle_shader)-1, "WorkphoneParticleCompute", NULL,
        NULL, "main", "cs_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_IEEE_STRICTNESS, 0, &code, &error );
    if( error ) ID3D10Blob_Release( error );
    if( FAILED(status) ) { wp_particle_compute_dx11_destroy(s); return NULL; }
    status = ID3D11Device_CreateComputeShader( s->device, ID3D10Blob_GetBufferPointer(code),
        ID3D10Blob_GetBufferSize(code), NULL, &s->shader );
    ID3D10Blob_Release(code);
    if( FAILED(status) ) { wp_particle_compute_dx11_destroy(s); return NULL; }
    memset( &buffer, 0, sizeof(buffer) );
    buffer.ByteWidth = capacity * (UINT)sizeof(wp_particle_sample);
    buffer.Usage = D3D11_USAGE_DEFAULT; buffer.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    buffer.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; buffer.StructureByteStride = sizeof(wp_particle_sample);
    if( FAILED(ID3D11Device_CreateBuffer(s->device, &buffer, NULL, &s->particles)) )
    { wp_particle_compute_dx11_destroy(s); return NULL; }
    memset( &view, 0, sizeof(view) ); view.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    view.Format = DXGI_FORMAT_UNKNOWN; view.Buffer.NumElements = capacity;
    if( FAILED(ID3D11Device_CreateUnorderedAccessView(s->device, (ID3D11Resource *)s->particles, &view, &s->view)) )
    { wp_particle_compute_dx11_destroy(s); return NULL; }
    buffer.Usage = D3D11_USAGE_STAGING; buffer.BindFlags = 0; buffer.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    buffer.MiscFlags = 0; buffer.StructureByteStride = 0;
    if( FAILED(ID3D11Device_CreateBuffer(s->device, &buffer, NULL, &s->readback)) )
    { wp_particle_compute_dx11_destroy(s); return NULL; }
    memset( &buffer, 0, sizeof(buffer) ); buffer.ByteWidth = 64;
    buffer.Usage = D3D11_USAGE_DEFAULT; buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if( FAILED(ID3D11Device_CreateBuffer(s->device, &buffer, NULL, &s->constants)) )
    { wp_particle_compute_dx11_destroy(s); return NULL; }
    return s;
}

wp_s32 wp_particle_compute_dx11_integrate( void *compute, wp_particle_sample *samples,
    wp_u32 count, const wp_particle_simulation_settings *settings, wp_f32 seconds )
{
    wp_particle_compute_dx11 *s;
    struct { float gravity[3], dt, start[4], end[4]; unsigned int count; float padding[3]; } values;
    ID3D11ComputeShader *previous_shader;
    ID3D11ClassInstance *instances[256];
    UINT instance_count, initial_count;
    ID3D11Buffer *previous_constants;
    ID3D11UnorderedAccessView *previous_view, *empty_view;
    D3D11_MAPPED_SUBRESOURCE mapped;
    D3D11_BOX box;
    HRESULT status;
    s = (wp_particle_compute_dx11 *)compute;
    if( !s || !samples || !settings || !count || count > s->capacity ||
        !(seconds > 0.0f && seconds <= 1.0f) ) return 0;
    if( FAILED(ID3D11Device_GetDeviceRemovedReason(s->device)) ) return 0;
    memset(&values,0,sizeof(values)); values.gravity[0]=settings->gravity.x;
    values.gravity[1]=settings->gravity.y; values.gravity[2]=settings->gravity.z; values.dt=seconds;
    memcpy(values.start,settings->color_start,sizeof(values.start)); memcpy(values.end,settings->color_end,sizeof(values.end));
    values.count=count; memset(&box,0,sizeof(box)); box.right=count*sizeof(wp_particle_sample); box.bottom=box.back=1;
    ID3D11DeviceContext_UpdateSubresource(s->context,(ID3D11Resource *)s->particles,0,&box,samples,0,0);
    ID3D11DeviceContext_UpdateSubresource(s->context,(ID3D11Resource *)s->constants,0,NULL,&values,0,0);
    previous_shader=NULL; previous_constants=NULL; previous_view=NULL; instance_count=256;
    ID3D11DeviceContext_CSGetShader(s->context,&previous_shader,instances,&instance_count);
    ID3D11DeviceContext_CSGetConstantBuffers(s->context,0,1,&previous_constants);
    ID3D11DeviceContext_CSGetUnorderedAccessViews(s->context,0,1,&previous_view);
    initial_count=(UINT)-1;
    ID3D11DeviceContext_CSSetShader(s->context,s->shader,NULL,0);
    ID3D11DeviceContext_CSSetConstantBuffers(s->context,0,1,&s->constants);
    ID3D11DeviceContext_CSSetUnorderedAccessViews(s->context,0,1,&s->view,&initial_count);
    ID3D11DeviceContext_Dispatch(s->context,(count+63)/64,1,1);
    empty_view=NULL; ID3D11DeviceContext_CSSetUnorderedAccessViews(s->context,0,1,&empty_view,&initial_count);
    ID3D11DeviceContext_CopyResource(s->context,(ID3D11Resource *)s->readback,(ID3D11Resource *)s->particles);
    ID3D11DeviceContext_CSSetShader(s->context,previous_shader,instances,instance_count);
    ID3D11DeviceContext_CSSetConstantBuffers(s->context,0,1,&previous_constants);
    ID3D11DeviceContext_CSSetUnorderedAccessViews(s->context,0,1,&previous_view,&initial_count);
    if(previous_shader) ID3D11ComputeShader_Release(previous_shader);
    if(previous_constants) ID3D11Buffer_Release(previous_constants);
    if(previous_view) ID3D11UnorderedAccessView_Release(previous_view);
    while(instance_count) ID3D11ClassInstance_Release(instances[--instance_count]);
    status=ID3D11DeviceContext_Map(s->context,(ID3D11Resource *)s->readback,0,D3D11_MAP_READ,0,&mapped);
    if(FAILED(status)) return 0;
    memcpy(samples,mapped.pData,count*sizeof(wp_particle_sample));
    ID3D11DeviceContext_Unmap(s->context,(ID3D11Resource *)s->readback,0);
    if(s->dispatches!=(wp_u32)~0u) ++s->dispatches;
    return 1;
}
wp_u32 wp_particle_compute_dx11_get_dispatches( const wp_particle_compute_dx11 *s )
{ return s?s->dispatches:0; }
#else
struct wp_particle_compute_dx11 { int unused; };
wp_particle_compute_dx11 *wp_particle_compute_dx11_create(void *d,void *c,wp_u32 n)
{ (void)d;(void)c;(void)n;return NULL; }
void wp_particle_compute_dx11_destroy(wp_particle_compute_dx11 *s) { (void)s; }
wp_s32 wp_particle_compute_dx11_integrate(void *s,wp_particle_sample *p,wp_u32 n,const wp_particle_simulation_settings *o,wp_f32 dt)
{ (void)s;(void)p;(void)n;(void)o;(void)dt;return 0; }
wp_u32 wp_particle_compute_dx11_get_dispatches(const wp_particle_compute_dx11 *s) { (void)s;return 0; }
#endif
