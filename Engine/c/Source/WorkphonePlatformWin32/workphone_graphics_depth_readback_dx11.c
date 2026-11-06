#define COBJMACROS
#include "workphone_graphics_depth_readback_dx11.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdlib.h>
#include <string.h>

struct wp_depth_readback_dx11
{
    ID3D11Device *device;
    ID3D11ComputeShader *single, *multiple;
    ID3D11Texture2D *output, *staging;
    ID3D11UnorderedAccessView *uav;
    ID3D11Buffer *constants;
    wp_u32 width, height;
};

static const char wp_depth_shader_source[] =
    "cbuffer Sizes : register(b0) { uint sourceWidth, sourceHeight, width, height; }\n"
    "#if MULTISAMPLE\n"
    "Texture2DMS<float> source : register(t0);\n"
    "#else\n"
    "Texture2D<float> source : register(t0);\n"
    "#endif\n"
    "RWTexture2D<float> output : register(u0);\n"
    "[numthreads(8,8,1)] void main(uint3 thread : SV_DispatchThreadID) {\n"
    "    if(thread.x>=width || thread.y>=height) return;\n"
    "    uint2 begin=uint2(thread.xy*uint2(sourceWidth,sourceHeight)/uint2(width,height));\n"
    "    uint2 end=uint2(((thread.xy+1)*uint2(sourceWidth,sourceHeight)+uint2(width,height)-1)/uint2(width,height));\n"
    "    float farthest=0;\n"
    "    #if MULTISAMPLE\n"
    "    uint sw,sh,samples; source.GetDimensions(sw,sh,samples);\n"
    "    #endif\n"
    "    for(uint y=begin.y;y<min(end.y,sourceHeight);++y)\n"
    "    for(uint x=begin.x;x<min(end.x,sourceWidth);++x) {\n"
    "        #if MULTISAMPLE\n"
    "        for(uint sampleIndex=0;sampleIndex<samples;++sampleIndex) farthest=max(farthest,source.Load(int2(x,y),sampleIndex));\n"
    "        #else\n"
    "        farthest=max(farthest,source.Load(int3(x,y,0)));\n"
    "        #endif\n"
    "    }\n"
    "    output[thread.xy]=farthest;\n"
    "}\n";

static void wp_depth_release_targets( wp_depth_readback_dx11 *readback )
{
    if( readback->uav ) ID3D11UnorderedAccessView_Release( readback->uav );
    if( readback->output ) ID3D11Texture2D_Release( readback->output );
    if( readback->staging ) ID3D11Texture2D_Release( readback->staging );
    if( readback->constants ) ID3D11Buffer_Release( readback->constants );
    readback->uav = NULL;
    readback->output = NULL;
    readback->staging = NULL;
    readback->constants = NULL;
    readback->width = readback->height = 0;
}

static void wp_depth_reset( wp_depth_readback_dx11 *readback )
{
    wp_depth_release_targets( readback );
    if( readback->single ) ID3D11ComputeShader_Release( readback->single );
    if( readback->multiple ) ID3D11ComputeShader_Release( readback->multiple );
    if( readback->device ) ID3D11Device_Release( readback->device );
    readback->single = NULL;
    readback->multiple = NULL;
    readback->device = NULL;
}

static ID3D11ComputeShader *wp_depth_shader( wp_depth_readback_dx11 *readback, wp_s32 multisample )
{
    ID3D11ComputeShader **cached;
    D3D_SHADER_MACRO macros[2] = { { "MULTISAMPLE", NULL }, { NULL, NULL } };
    ID3DBlob *code = NULL, *errors = NULL;
    HRESULT status;

    cached = multisample ? &readback->multiple : &readback->single;
    if( *cached ) return *cached;

    macros[0].Definition = multisample ? "1" : "0";
    status = D3DCompile( wp_depth_shader_source, sizeof( wp_depth_shader_source ) - 1, "OcclusionDepth",
                         macros, NULL, "main", "cs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &code,
                         &errors );
    if( errors ) ID3D10Blob_Release( errors );
    if( SUCCEEDED( status ) )
    {
        status = ID3D11Device_CreateComputeShader( readback->device, ID3D10Blob_GetBufferPointer( code ),
                                                   ID3D10Blob_GetBufferSize( code ), NULL, cached );
    }
    if( code ) ID3D10Blob_Release( code );
    return SUCCEEDED( status ) ? *cached : NULL;
}

static wp_s32 wp_depth_targets( wp_depth_readback_dx11 *readback, wp_u32 width, wp_u32 height )
{
    D3D11_TEXTURE2D_DESC texture;
    D3D11_BUFFER_DESC buffer;

    if( width == readback->width && height == readback->height ) return 1;
    wp_depth_release_targets( readback );

    memset( &texture, 0, sizeof( texture ) );
    texture.Width = width;
    texture.Height = height;
    texture.MipLevels = texture.ArraySize = texture.SampleDesc.Count = 1;
    texture.Format = DXGI_FORMAT_R32_FLOAT;
    texture.Usage = D3D11_USAGE_DEFAULT;
    texture.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    if( FAILED( ID3D11Device_CreateTexture2D( readback->device, &texture, NULL, &readback->output ) ) ||
        FAILED( ID3D11Device_CreateUnorderedAccessView( readback->device,
                (ID3D11Resource *)readback->output, NULL, &readback->uav ) ) ) return 0;

    texture.Usage = D3D11_USAGE_STAGING;
    texture.BindFlags = 0;
    texture.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if( FAILED( ID3D11Device_CreateTexture2D( readback->device, &texture, NULL, &readback->staging ) ) )
        return 0;

    memset( &buffer, 0, sizeof( buffer ) );
    buffer.ByteWidth = 16;
    buffer.Usage = D3D11_USAGE_DEFAULT;
    buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if( FAILED( ID3D11Device_CreateBuffer( readback->device, &buffer, NULL, &readback->constants ) ) )
        return 0;

    readback->width = width;
    readback->height = height;
    return 1;
}

wp_depth_readback_dx11 *wp_depth_readback_dx11_create( void )
{
    return (wp_depth_readback_dx11 *)calloc( 1, sizeof( wp_depth_readback_dx11 ) );
}

void wp_depth_readback_dx11_destroy( wp_depth_readback_dx11 *readback )
{
    if( !readback ) return;
    wp_depth_reset( readback );
    free( readback );
}

wp_s32 wp_renderer_dx11_depth_surface( wp_renderer_dx11 *renderer, wp_u32 *width, wp_u32 *height,
                                       wp_u32 *samples )
{
    ID3D11Texture2D *texture;
    D3D11_TEXTURE2D_DESC desc;

    if( !renderer || !width || !height || !samples ) return 0;
    texture = (ID3D11Texture2D *)wp_renderer_dx11_get_scene_depth_texture( renderer );
    if( !texture ) return 0;
    ID3D11Texture2D_GetDesc( texture, &desc );
    *width = desc.Width;
    *height = desc.Height;
    *samples = desc.SampleDesc.Count;
    return 1;
}

wp_s32 wp_renderer_dx11_read_depth( wp_depth_readback_dx11 *readback, wp_renderer_dx11 *renderer,
                                   wp_f32 *depth, wp_u32 width, wp_u32 height, wp_u32 capacity )
{
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    ID3D11Texture2D *texture;
    D3D11_TEXTURE2D_DESC desc;
    ID3D11ComputeShader *shader;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ID3D11ShaderResourceView *srv = NULL, *old_srv = NULL;
    ID3D11RenderTargetView *old_targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { NULL };
    ID3D11DepthStencilView *old_depth = NULL;
    ID3D11ComputeShader *old_shader = NULL;
    ID3D11ClassInstance *instances[256] = { NULL };
    UINT instance_count = 256, i;
    ID3D11UnorderedAccessView *old_uav = NULL, *empty_uav = NULL;
    ID3D11Buffer *old_constants = NULL;
    UINT sizes[4];
    D3D11_MAPPED_SUBRESOURCE mapped;

    if( !readback || !renderer || !depth || !width || !height || width > 256 || height > 256 ||
        width * height > capacity ) return 0;

    device = (ID3D11Device *)wp_renderer_dx11_get_device( renderer );
    context = (ID3D11DeviceContext *)wp_renderer_dx11_get_context( renderer );
    texture = (ID3D11Texture2D *)wp_renderer_dx11_get_scene_depth_texture( renderer );
    if( !device || !context || !texture ) return 0;

    ID3D11Texture2D_GetDesc( texture, &desc );
    if( width > desc.Width || height > desc.Height ) return 0;

    if( readback->device != device )
    {
        wp_depth_reset( readback );
        ID3D11Device_AddRef( device );
        readback->device = device;
    }
    if( !wp_depth_targets( readback, width, height ) ) return 0;
    shader = wp_depth_shader( readback, desc.SampleDesc.Count > 1 );
    if( !shader ) return 0;

    memset( &srv_desc, 0, sizeof( srv_desc ) );
    srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    if( desc.SampleDesc.Count > 1 )
    {
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    }
    else
    {
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = 1;
    }
    if( FAILED( ID3D11Device_CreateShaderResourceView( device, (ID3D11Resource *)texture, &srv_desc, &srv ) ) )
        return 0;

    ID3D11DeviceContext_OMGetRenderTargets( context, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,
                                          old_targets, &old_depth );
    ID3D11DeviceContext_CSGetShader( context, &old_shader, instances, &instance_count );
    ID3D11DeviceContext_CSGetShaderResources( context, 0, 1, &old_srv );
    ID3D11DeviceContext_CSGetUnorderedAccessViews( context, 0, 1, &old_uav );
    ID3D11DeviceContext_CSGetConstantBuffers( context, 0, 1, &old_constants );

    ID3D11DeviceContext_OMSetRenderTargets( context, 0, NULL, NULL );
    sizes[0] = desc.Width;
    sizes[1] = desc.Height;
    sizes[2] = width;
    sizes[3] = height;
    ID3D11DeviceContext_UpdateSubresource( context, (ID3D11Resource *)readback->constants, 0, NULL, sizes, 0, 0 );
    ID3D11DeviceContext_CSSetShader( context, shader, NULL, 0 );
    ID3D11DeviceContext_CSSetShaderResources( context, 0, 1, &srv );
    ID3D11DeviceContext_CSSetUnorderedAccessViews( context, 0, 1, &readback->uav, NULL );
    ID3D11DeviceContext_CSSetConstantBuffers( context, 0, 1, &readback->constants );
    ID3D11DeviceContext_Dispatch( context, ( width + 7 ) / 8, ( height + 7 ) / 8, 1 );
    ID3D11DeviceContext_CSSetUnorderedAccessViews( context, 0, 1, &empty_uav, NULL );
    ID3D11DeviceContext_CopyResource( context, (ID3D11Resource *)readback->staging,
                                     (ID3D11Resource *)readback->output );

    ID3D11DeviceContext_CSSetShaderResources( context, 0, 1, &old_srv );
    ID3D11DeviceContext_CSSetUnorderedAccessViews( context, 0, 1, &old_uav, NULL );
    ID3D11DeviceContext_CSSetConstantBuffers( context, 0, 1, &old_constants );
    ID3D11DeviceContext_CSSetShader( context, old_shader, instances, instance_count );
    ID3D11DeviceContext_OMSetRenderTargets( context, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,
                                          old_targets, old_depth );
    ID3D11ShaderResourceView_Release( srv );
    if( old_srv ) ID3D11ShaderResourceView_Release( old_srv );
    if( old_uav ) ID3D11UnorderedAccessView_Release( old_uav );
    if( old_shader ) ID3D11ComputeShader_Release( old_shader );
    if( old_constants ) ID3D11Buffer_Release( old_constants );
    if( old_depth ) ID3D11DepthStencilView_Release( old_depth );
    for( i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i )
    {
        if( old_targets[i] ) ID3D11RenderTargetView_Release( old_targets[i] );
    }
    for( i = 0; i < instance_count; ++i )
    {
        if( instances[i] ) ID3D11ClassInstance_Release( instances[i] );
    }

    if( FAILED( ID3D11DeviceContext_Map( context, (ID3D11Resource *)readback->staging, 0,
                                        D3D11_MAP_READ, 0, &mapped ) ) ) return 0;
    for( i = 0; i < height; ++i )
    {
        memcpy( depth + (size_t)i * width, (const char *)mapped.pData + (size_t)i * mapped.RowPitch,
                width * sizeof( *depth ) );
    }
    ID3D11DeviceContext_Unmap( context, (ID3D11Resource *)readback->staging, 0 );
    return 1;
}
