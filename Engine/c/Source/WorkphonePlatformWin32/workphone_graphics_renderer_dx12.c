/**
 * @file workphone_graphics_renderer_dx12.c
 * @brief Direct3D 12 renderer implementation.
 *
 * A single command list is recorded between begin_frame and end_frame. Dynamic
 * geometry is copied into a persistently mapped upload buffer that is reset each
 * frame. The CPU waits for the GPU at the end of each frame, which keeps
 * resource lifetime handling simple.
 */

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    include <d3d12.h>
#    include <dxgi1_4.h>
#    include <d3dcompiler.h>
#    include <stddef.h>
#    include <stdlib.h>
#    include <string.h>
#    include "workphone_graphics_renderer_dx12.h"
#    pragma comment( lib, "d3d12.lib" )
#    pragma comment( lib, "dxgi.lib" )
#    pragma comment( lib, "dxguid.lib" )
#    pragma comment( lib, "d3dcompiler.lib" )

#    define DX12_FRAME_COUNT 2
#    define DX12_UPLOAD_SIZE ( 16 * 1024 * 1024 )
#    define DX12_SRV_COUNT 256
#    define DX12_MAX_PSO 128
#    define DX12_RELEASE( p )                \
		do                                   \
		{                                    \
			if( p )                          \
			{                                \
				( p )->lpVtbl->Release( p ); \
				( p ) = NULL;                \
			}                                \
		} while( 0 )

static const char s_vs_pc_src[] =
	"cbuffer Transform : register(b0) { row_major float4x4 mvp; };\n"
	"struct VS_IN  { float3 pos : POSITION; uint color : COLOR; };\n"
	"struct VS_OUT { float4 pos : SV_POSITION; float4 color : COLOR; };\n"
	"VS_OUT vs_main(VS_IN i)\n"
	"{\n"
	"    VS_OUT o;\n"
	"    o.pos = mul(mvp, float4(i.pos, 1.0));\n"
	"    o.pos.z = 0.5 * (o.pos.z + o.pos.w);\n"
	"    o.color = float4(\n"
	"        float((i.color >> 24u) & 0xFFu) / 255.0,\n"
	"        float((i.color >> 16u) & 0xFFu) / 255.0,\n"
	"        float((i.color >>  8u) & 0xFFu) / 255.0,\n"
	"        float( i.color         & 0xFFu) / 255.0);\n"
	"    return o;\n"
	"}\n";

static const char s_ps_pc_src[] =
	"struct PS_IN { float4 pos : SV_POSITION; float4 color : COLOR; };\n"
	"float4 ps_main(PS_IN i) : SV_TARGET { return i.color; }\n";

static const char s_vs_ptc_src[] =
	"cbuffer Transform : register(b0) { row_major float4x4 mvp; };\n"
	"struct VS_IN  { float3 pos : POSITION; float2 uv : TEXCOORD0; uint color : COLOR; };\n"
	"struct VS_OUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR; };\n"
	"VS_OUT vs_main(VS_IN i)\n"
	"{\n"
	"    VS_OUT o;\n"
	"    o.pos = mul(mvp, float4(i.pos, 1.0));\n"
	"    o.pos.z = 0.5 * (o.pos.z + o.pos.w);\n"
	"    o.uv = i.uv;\n"
	"    o.color = float4(\n"
	"        float((i.color >> 24u) & 0xFFu) / 255.0,\n"
	"        float((i.color >> 16u) & 0xFFu) / 255.0,\n"
	"        float((i.color >>  8u) & 0xFFu) / 255.0,\n"
	"        float( i.color         & 0xFFu) / 255.0);\n"
	"    return o;\n"
	"}\n";

static const char s_ps_ptc_src[] =
	"Texture2D tex : register(t0);\n"
	"SamplerState tex_sampler : register(s0);\n"
	"struct PS_IN { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR; };\n"
	"float4 ps_main(PS_IN i) : SV_TARGET { return tex.Sample(tex_sampler, i.uv) * i.color; }\n";

typedef struct wp_dx12_pso_key
{
	wp_s32 layout; /* 0 = pc, 1 = ptc */
	wp_s32 topology;
	wp_s32 blend;
	wp_s32 fill;
	wp_s32 cull;
	wp_s32 depth_test;
	wp_s32 depth_write;
	wp_s32 depth_func;
} wp_dx12_pso_key;

typedef struct wp_dx12_pso_entry
{
	wp_dx12_pso_key key;
	ID3D12PipelineState *pso;
} wp_dx12_pso_entry;

typedef struct wp_renderer_dx12
{
	ID3D12Device *device;
	ID3D12CommandQueue *command_queue;
	IDXGISwapChain3 *swap_chain;
	ID3D12DescriptorHeap *rtv_heap;
	ID3D12DescriptorHeap *dsv_heap;
	ID3D12DescriptorHeap *srv_heap;
	ID3D12Resource *render_targets[DX12_FRAME_COUNT];
	ID3D12Resource *depth_buffer;
	ID3D12CommandAllocator *command_allocator;
	ID3D12GraphicsCommandList *command_list;
	ID3D12CommandAllocator *upload_allocator;
	ID3D12GraphicsCommandList *upload_list;
	ID3D12Fence *fence;
	HANDLE fence_event;
	UINT64 fence_value;
	ID3D12RootSignature *root_signature;
	ID3DBlob *vs_pc, *ps_pc, *vs_ptc, *ps_ptc;
	wp_dx12_pso_entry psos[DX12_MAX_PSO];
	wp_s32 pso_count;

	ID3D12Resource *upload_buffer;
	UINT8 *upload_ptr;
	UINT64 upload_offset;

	ID3D12Resource *white_texture;
	ID3D12Resource *texture;
	wp_s32 texture_owned;
	wp_s32 texture_dirty;
	wp_s32 texture_slot;
	wp_s32 srv_next;
	UINT rtv_inc, srv_inc;

	ID3D12Resource **retired;
	wp_s32 retired_count, retired_capacity;

	wp_s32 frame_open;
	wp_u32 frame_index;
	wp_s32 width;
	wp_s32 height;
	HWND hwnd;
	wp_f32 clear_r, clear_g, clear_b, clear_a, clear_depth;
	wp_viewport_i viewport;
	wp_s32 scissor_enabled;
	wp_viewport_i scissor;
	wp_blend_mode blend_mode;
	wp_fill_mode fill_mode;
	wp_cull_mode cull_mode;
	wp_s32 depth_test_enabled;
	wp_s32 depth_write_enabled;
	wp_depth_func depth_func;
	wp_mat4f world_matrix, view_matrix, proj_matrix, mvp_matrix;
	wp_s32 mvp_dirty;
	void *native;
} wp_renderer_dx12;

/* =========================================================================
 * Internal helpers -- math
 * ====================================================================== */

static void wp_renderer_dx12_mat4f_mul( wp_mat4f *result, const wp_mat4f *a, const wp_mat4f *b )
{
	wp_mat4f tmp;
	wp_s32 i, j, k;

	for( i = 0; i < 4; i++ )
		for( j = 0; j < 4; j++ )
		{
			tmp.m[i][j] = 0.0f;
			for( k = 0; k < 4; k++ )
				tmp.m[i][j] += a->m[i][k] * b->m[k][j];
		}
	*result = tmp;
}

static void wp_renderer_dx12_update_mvp( wp_renderer_dx12 *r )
{
	wp_mat4f vw;

	if( !r->mvp_dirty )
		return;
	wp_renderer_dx12_mat4f_mul( &vw, &r->view_matrix, &r->world_matrix );
	wp_renderer_dx12_mat4f_mul( &r->mvp_matrix, &r->proj_matrix, &vw );
	r->mvp_dirty = 0;
}

/* =========================================================================
 * Internal helpers -- GPU synchronisation and resources
 * ====================================================================== */

static void wp_renderer_dx12_wait_for_gpu( wp_renderer_dx12 *r )
{
	if( !r->command_queue || !r->fence || !r->fence_event )
		return;
	r->fence_value++;
	if( FAILED( ID3D12CommandQueue_Signal( r->command_queue, r->fence, r->fence_value ) ) )
		return;
	if( ID3D12Fence_GetCompletedValue( r->fence ) < r->fence_value )
	{
		if( SUCCEEDED( ID3D12Fence_SetEventOnCompletion( r->fence, r->fence_value, r->fence_event ) ) )
			WaitForSingleObject( r->fence_event, INFINITE );
	}
}

static D3D12_HEAP_PROPERTIES wp_renderer_dx12_heap( D3D12_HEAP_TYPE type )
{
	D3D12_HEAP_PROPERTIES hp;
	memset( &hp, 0, sizeof( hp ) );
	hp.Type = type;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;
	return hp;
}

static D3D12_RESOURCE_DESC wp_renderer_dx12_buffer_desc( UINT64 size )
{
	D3D12_RESOURCE_DESC d;
	memset( &d, 0, sizeof( d ) );
	d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	d.Width = size;
	d.Height = 1;
	d.DepthOrArraySize = 1;
	d.MipLevels = 1;
	d.Format = DXGI_FORMAT_UNKNOWN;
	d.SampleDesc.Count = 1;
	d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	return d;
}

static void wp_renderer_dx12_barrier( ID3D12GraphicsCommandList *list, ID3D12Resource *resource,
									  D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after )
{
	D3D12_RESOURCE_BARRIER b;
	memset( &b, 0, sizeof( b ) );
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = resource;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	b.Transition.StateBefore = before;
	b.Transition.StateAfter = after;
	ID3D12GraphicsCommandList_ResourceBarrier( list, 1, &b );
}

static void wp_renderer_dx12_retire( wp_renderer_dx12 *r, ID3D12Resource *resource )
{
	if( !resource )
		return;
	if( !r->frame_open )
	{
		ID3D12Resource_Release( resource );
		return;
	}
	if( r->retired_count == r->retired_capacity )
	{
		wp_s32 cap = r->retired_capacity ? r->retired_capacity * 2 : 16;
		ID3D12Resource **p =
			(ID3D12Resource **)realloc( r->retired, sizeof( ID3D12Resource * ) * (size_t)cap );
		if( !p )
			return; /* leak rather than risk releasing a resource still referenced by the open frame */
		r->retired = p;
		r->retired_capacity = cap;
	}
	r->retired[r->retired_count++] = resource;
}

static void wp_renderer_dx12_release_retired( wp_renderer_dx12 *r )
{
	wp_s32 i;
	for( i = 0; i < r->retired_count; i++ )
		ID3D12Resource_Release( r->retired[i] );
	r->retired_count = 0;
}

static wp_s32 wp_renderer_dx12_create_rtvs( wp_renderer_dx12 *r )
{
	D3D12_CPU_DESCRIPTOR_HANDLE h;
	UINT i;

	ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart( r->rtv_heap, &h );
	for( i = 0; i < DX12_FRAME_COUNT; i++ )
	{
		if( FAILED( IDXGISwapChain3_GetBuffer( r->swap_chain, i, &IID_ID3D12Resource,
											   (void **)&r->render_targets[i] ) ) )
			return 0;
		ID3D12Device_CreateRenderTargetView( r->device, r->render_targets[i], NULL, h );
		h.ptr += r->rtv_inc;
	}
	return 1;
}

static wp_s32 wp_renderer_dx12_create_depth( wp_renderer_dx12 *r )
{
	D3D12_HEAP_PROPERTIES hp = wp_renderer_dx12_heap( D3D12_HEAP_TYPE_DEFAULT );
	D3D12_RESOURCE_DESC d;
	D3D12_CLEAR_VALUE cv;
	D3D12_CPU_DESCRIPTOR_HANDLE h;

	memset( &d, 0, sizeof( d ) );
	d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d.Width = (UINT64)r->width;
	d.Height = (UINT)r->height;
	d.DepthOrArraySize = 1;
	d.MipLevels = 1;
	d.Format = DXGI_FORMAT_D32_FLOAT;
	d.SampleDesc.Count = 1;
	d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	memset( &cv, 0, sizeof( cv ) );
	cv.Format = DXGI_FORMAT_D32_FLOAT;
	cv.DepthStencil.Depth = 1.0f;

	if( FAILED( ID3D12Device_CreateCommittedResource( r->device, &hp, D3D12_HEAP_FLAG_NONE, &d,
													  D3D12_RESOURCE_STATE_DEPTH_WRITE, &cv,
													  &IID_ID3D12Resource, (void **)&r->depth_buffer ) ) )
		return 0;

	ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart( r->dsv_heap, &h );
	ID3D12Device_CreateDepthStencilView( r->device, r->depth_buffer, NULL, h );
	return 1;
}

static void wp_renderer_dx12_release_targets( wp_renderer_dx12 *r )
{
	wp_s32 i;
	for( i = 0; i < DX12_FRAME_COUNT; i++ )
		DX12_RELEASE( r->render_targets[i] );
	DX12_RELEASE( r->depth_buffer );
}

static wp_s32 wp_renderer_dx12_compile( const char *src, const char *entry, const char *target,
										ID3DBlob **blob )
{
	ID3DBlob *errors = NULL;
	HRESULT hr = D3DCompile( src, strlen( src ), NULL, NULL, NULL, entry, target, 0, 0, blob, &errors );
	if( errors )
		ID3D10Blob_Release( errors );
	return SUCCEEDED( hr );
}

static wp_s32 wp_renderer_dx12_create_root_signature( wp_renderer_dx12 *r )
{
	D3D12_ROOT_PARAMETER params[2];
	D3D12_DESCRIPTOR_RANGE range;
	D3D12_STATIC_SAMPLER_DESC sampler;
	D3D12_ROOT_SIGNATURE_DESC desc;
	ID3DBlob *blob = NULL, *errors = NULL;
	HRESULT hr;

	memset( params, 0, sizeof( params ) );
	memset( &range, 0, sizeof( range ) );
	memset( &sampler, 0, sizeof( sampler ) );
	memset( &desc, 0, sizeof( desc ) );

	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	params[0].Constants.ShaderRegister = 0;
	params[0].Constants.Num32BitValues = 16;
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.NumDescriptors = 1;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[1].DescriptorTable.NumDescriptorRanges = 1;
	params[1].DescriptorTable.pDescriptorRanges = &range;
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	desc.NumParameters = 2;
	desc.pParameters = params;
	desc.NumStaticSamplers = 1;
	desc.pStaticSamplers = &sampler;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	hr = D3D12SerializeRootSignature( &desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errors );
	if( errors )
		ID3D10Blob_Release( errors );
	if( FAILED( hr ) )
		return 0;

	hr = ID3D12Device_CreateRootSignature( r->device, 0, ID3D10Blob_GetBufferPointer( blob ),
										   ID3D10Blob_GetBufferSize( blob ), &IID_ID3D12RootSignature,
										   (void **)&r->root_signature );
	ID3D10Blob_Release( blob );
	return SUCCEEDED( hr );
}

/* =========================================================================
 * Internal helpers -- textures
 * ====================================================================== */

static ID3D12Resource *wp_renderer_dx12_create_texture_resource( wp_renderer_dx12 *r, const void *pixels,
																 wp_s32 width, wp_s32 height,
																 wp_pixel_format format )
{
	const wp_s32 bpp =
		( format == WORKPHONE_PIXEL_FORMAT_RGB8 || format == WORKPHONE_PIXEL_FORMAT_BGR8 ) ? 3 : 4;
	const wp_s32 bgr = ( format == WORKPHONE_PIXEL_FORMAT_BGRA8 || format == WORKPHONE_PIXEL_FORMAT_BGR8 );
	const UINT pitch = ( (UINT)width * 4u + 255u ) & ~255u;
	const UINT64 upload_size = (UINT64)pitch * (UINT64)height;
	ID3D12Resource *texture = NULL, *upload = NULL;
	D3D12_HEAP_PROPERTIES hp_default = wp_renderer_dx12_heap( D3D12_HEAP_TYPE_DEFAULT );
	D3D12_HEAP_PROPERTIES hp_upload = wp_renderer_dx12_heap( D3D12_HEAP_TYPE_UPLOAD );
	D3D12_RESOURCE_DESC td, ud;
	D3D12_TEXTURE_COPY_LOCATION dst, src;
	UINT8 *mapped = NULL;
	ID3D12CommandList *lists[1];
	wp_s32 x, y;

	if( !r || !pixels || width <= 0 || height <= 0 || r->frame_open )
		return NULL;

	ud = wp_renderer_dx12_buffer_desc( upload_size );
	memset( &td, 0, sizeof( td ) );
	td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	td.Width = (UINT64)width;
	td.Height = (UINT)height;
	td.DepthOrArraySize = 1;
	td.MipLevels = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	if( FAILED( ID3D12Device_CreateCommittedResource( r->device, &hp_upload, D3D12_HEAP_FLAG_NONE, &ud,
													  D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
													  &IID_ID3D12Resource, (void **)&upload ) ) )
		return NULL;
	if( FAILED( ID3D12Device_CreateCommittedResource( r->device, &hp_default, D3D12_HEAP_FLAG_NONE, &td,
													  D3D12_RESOURCE_STATE_COPY_DEST, NULL,
													  &IID_ID3D12Resource, (void **)&texture ) ) )
	{
		ID3D12Resource_Release( upload );
		return NULL;
	}
	if( FAILED( ID3D12Resource_Map( upload, 0, NULL, (void **)&mapped ) ) )
	{
		ID3D12Resource_Release( upload );
		ID3D12Resource_Release( texture );
		return NULL;
	}
	for( y = 0; y < height; y++ )
	{
		const UINT8 *s = (const UINT8 *)pixels + (size_t)y * (size_t)width * (size_t)bpp;
		UINT8 *d = mapped + (size_t)y * pitch;
		for( x = 0; x < width; x++, s += bpp, d += 4 )
		{
			d[0] = bgr ? s[2] : s[0];
			d[1] = s[1];
			d[2] = bgr ? s[0] : s[2];
			d[3] = bpp == 4 ? s[3] : 255;
		}
	}
	ID3D12Resource_Unmap( upload, 0, NULL );

	memset( &dst, 0, sizeof( dst ) );
	dst.pResource = texture;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	memset( &src, 0, sizeof( src ) );
	src.pResource = upload;
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	src.PlacedFootprint.Footprint.Width = (UINT)width;
	src.PlacedFootprint.Footprint.Height = (UINT)height;
	src.PlacedFootprint.Footprint.Depth = 1;
	src.PlacedFootprint.Footprint.RowPitch = pitch;

	if( FAILED( ID3D12CommandAllocator_Reset( r->upload_allocator ) ) ||
		FAILED( ID3D12GraphicsCommandList_Reset( r->upload_list, r->upload_allocator, NULL ) ) )
	{
		ID3D12Resource_Release( upload );
		ID3D12Resource_Release( texture );
		return NULL;
	}
	ID3D12GraphicsCommandList_CopyTextureRegion( r->upload_list, &dst, 0, 0, 0, &src, NULL );
	wp_renderer_dx12_barrier( r->upload_list, texture, D3D12_RESOURCE_STATE_COPY_DEST,
							  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );
	ID3D12GraphicsCommandList_Close( r->upload_list );
	lists[0] = (ID3D12CommandList *)r->upload_list;
	ID3D12CommandQueue_ExecuteCommandLists( r->command_queue, 1, lists );
	wp_renderer_dx12_wait_for_gpu( r );

	ID3D12Resource_Release( upload );
	return texture;
}

static void wp_renderer_dx12_write_srv( wp_renderer_dx12 *r, ID3D12Resource *texture, wp_s32 slot )
{
	D3D12_SHADER_RESOURCE_VIEW_DESC sd;
	D3D12_CPU_DESCRIPTOR_HANDLE h;

	memset( &sd, 0, sizeof( sd ) );
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	sd.Texture2D.MipLevels = 1;

	ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart( r->srv_heap, &h );
	h.ptr += (SIZE_T)slot * r->srv_inc;
	ID3D12Device_CreateShaderResourceView( r->device, texture, &sd, h );
}

static void wp_renderer_dx12_bind_texture( wp_renderer_dx12 *r )
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpu;
	wp_s32 slot;

	if( r->texture_slot >= 0 && !r->texture_dirty )
		return;

	if( !r->texture )
		slot = 0;
	else if( r->srv_next < DX12_SRV_COUNT )
	{
		slot = r->srv_next++;
		wp_renderer_dx12_write_srv( r, r->texture, slot );
	}
	else
		slot = r->texture_slot >= 0 ? r->texture_slot : 0;

	r->texture_slot = slot;
	r->texture_dirty = 0;
	ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart( r->srv_heap, &gpu );
	gpu.ptr += (UINT64)slot * r->srv_inc;
	ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable( r->command_list, 1, gpu );
}

static void wp_renderer_dx12_replace_texture( wp_renderer_dx12 *r, ID3D12Resource *texture, wp_s32 owned )
{
	if( r->texture_owned )
		wp_renderer_dx12_retire( r, r->texture );
	r->texture = texture;
	r->texture_owned = owned;
	r->texture_dirty = 1;
}

/* =========================================================================
 * Internal helpers -- pipeline state and drawing
 * ====================================================================== */

static D3D12_BLEND wp_renderer_dx12_blend_factor( wp_s32 destination, wp_blend_mode mode )
{
	switch( mode )
	{
	case WORKPHONE_BLEND_MODE_ALPHA:
		return destination ? D3D12_BLEND_INV_SRC_ALPHA : D3D12_BLEND_SRC_ALPHA;
	case WORKPHONE_BLEND_MODE_ADDITIVE:
		return destination ? D3D12_BLEND_ONE : D3D12_BLEND_SRC_ALPHA;
	case WORKPHONE_BLEND_MODE_MULTIPLY:
		return destination ? D3D12_BLEND_ZERO : D3D12_BLEND_DEST_COLOR;
	case WORKPHONE_BLEND_MODE_PREMULTIPLIED:
		return destination ? D3D12_BLEND_INV_SRC_ALPHA : D3D12_BLEND_ONE;
	default:
		return destination ? D3D12_BLEND_ZERO : D3D12_BLEND_ONE;
	}
}

static D3D12_COMPARISON_FUNC wp_renderer_dx12_depth_func( wp_s32 func )
{
	switch( func )
	{
	case WORKPHONE_DEPTH_FUNC_NEVER:
		return D3D12_COMPARISON_FUNC_NEVER;
	case WORKPHONE_DEPTH_FUNC_EQUAL:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case WORKPHONE_DEPTH_FUNC_LEQUAL:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case WORKPHONE_DEPTH_FUNC_GREATER:
		return D3D12_COMPARISON_FUNC_GREATER;
	case WORKPHONE_DEPTH_FUNC_NOTEQUAL:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case WORKPHONE_DEPTH_FUNC_GEQUAL:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case WORKPHONE_DEPTH_FUNC_ALWAYS:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	default:
		return D3D12_COMPARISON_FUNC_LESS;
	}
}

static ID3D12PipelineState *wp_renderer_dx12_get_pso( wp_renderer_dx12 *r, const wp_dx12_pso_key *key )
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d;
	D3D12_INPUT_ELEMENT_DESC layout[3];
	ID3DBlob *vs = key->layout ? r->vs_ptc : r->vs_pc;
	ID3DBlob *ps = key->layout ? r->ps_ptc : r->ps_pc;
	wp_blend_mode blend = (wp_blend_mode)key->blend;
	wp_s32 i;

	for( i = 0; i < r->pso_count; i++ )
		if( memcmp( &r->psos[i].key, key, sizeof( *key ) ) == 0 )
			return r->psos[i].pso;
	if( r->pso_count >= DX12_MAX_PSO )
		return NULL;

	memset( layout, 0, sizeof( layout ) );
	layout[0].SemanticName = "POSITION";
	layout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	if( key->layout )
	{
		layout[1].SemanticName = "TEXCOORD";
		layout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		layout[1].AlignedByteOffset = (UINT)offsetof( wp_vertex_ptc, uv );
		layout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		layout[2].SemanticName = "COLOR";
		layout[2].Format = DXGI_FORMAT_R32_UINT;
		layout[2].AlignedByteOffset = (UINT)offsetof( wp_vertex_ptc, color );
		layout[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	}
	else
	{
		layout[1].SemanticName = "COLOR";
		layout[1].Format = DXGI_FORMAT_R32_UINT;
		layout[1].AlignedByteOffset = (UINT)offsetof( wp_vertex_pc, color );
		layout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	}

	memset( &d, 0, sizeof( d ) );
	d.pRootSignature = r->root_signature;
	d.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer( vs );
	d.VS.BytecodeLength = ID3D10Blob_GetBufferSize( vs );
	d.PS.pShaderBytecode = ID3D10Blob_GetBufferPointer( ps );
	d.PS.BytecodeLength = ID3D10Blob_GetBufferSize( ps );
	d.InputLayout.pInputElementDescs = layout;
	d.InputLayout.NumElements = key->layout ? 3 : 2;
	d.SampleMask = 0xFFFFFFFFu;

	d.BlendState.RenderTarget[0].BlendEnable = blend != WORKPHONE_BLEND_MODE_NONE;
	d.BlendState.RenderTarget[0].SrcBlend = wp_renderer_dx12_blend_factor( 0, blend );
	d.BlendState.RenderTarget[0].DestBlend = wp_renderer_dx12_blend_factor( 1, blend );
	d.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	d.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	d.RasterizerState.FillMode =
		key->fill == WORKPHONE_FILL_MODE_SOLID ? D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME;
	d.RasterizerState.CullMode = key->cull == WORKPHONE_CULL_MODE_NONE
									 ? D3D12_CULL_MODE_NONE
									 : ( key->cull == WORKPHONE_CULL_MODE_FRONT ? D3D12_CULL_MODE_FRONT
																				: D3D12_CULL_MODE_BACK );
	d.RasterizerState.FrontCounterClockwise = TRUE;
	d.RasterizerState.DepthClipEnable = TRUE;

	d.DepthStencilState.DepthEnable = key->depth_test != 0;
	d.DepthStencilState.DepthWriteMask =
		key->depth_write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	d.DepthStencilState.DepthFunc = wp_renderer_dx12_depth_func( key->depth_func );

	d.PrimitiveTopologyType = (D3D12_PRIMITIVE_TOPOLOGY_TYPE)key->topology;
	d.NumRenderTargets = 1;
	d.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	d.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	d.SampleDesc.Count = 1;

	if( FAILED( ID3D12Device_CreateGraphicsPipelineState( r->device, &d, &IID_ID3D12PipelineState,
														  (void **)&r->psos[r->pso_count].pso ) ) )
		return NULL;
	r->psos[r->pso_count].key = *key;
	return r->psos[r->pso_count++].pso;
}

static wp_s32 wp_renderer_dx12_upload( wp_renderer_dx12 *r, const void *data, UINT64 size, UINT64 *offset )
{
	UINT64 start = ( r->upload_offset + 3u ) & ~(UINT64)3u;
	if( !r->upload_ptr || start + size > DX12_UPLOAD_SIZE )
		return 0;
	memcpy( r->upload_ptr + start, data, (size_t)size );
	r->upload_offset = start + size;
	*offset = start;
	return 1;
}

static void wp_renderer_dx12_apply_viewport( wp_renderer_dx12 *r )
{
	D3D12_VIEWPORT vp;
	D3D12_RECT sc;
	const wp_viewport_i *s = r->scissor_enabled ? &r->scissor : &r->viewport;

	vp.TopLeftX = (FLOAT)r->viewport.x;
	vp.TopLeftY = (FLOAT)r->viewport.y;
	vp.Width = (FLOAT)r->viewport.width;
	vp.Height = (FLOAT)r->viewport.height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	sc.left = s->x;
	sc.top = s->y;
	sc.right = s->x + s->width;
	sc.bottom = s->y + s->height;
	ID3D12GraphicsCommandList_RSSetViewports( r->command_list, 1, &vp );
	ID3D12GraphicsCommandList_RSSetScissorRects( r->command_list, 1, &sc );
}

static void wp_renderer_dx12_draw( wp_renderer_dx12 *r, wp_s32 layout, wp_s32 stride,
								   D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_type,
								   D3D_PRIMITIVE_TOPOLOGY topology, const void *vertices,
								   wp_s32 vertex_count, const uint16_t *indices, wp_s32 index_count )
{
	wp_dx12_pso_key key;
	ID3D12PipelineState *pso;
	D3D12_VERTEX_BUFFER_VIEW vbv;
	UINT64 vb_offset = 0, ib_offset = 0;
	const wp_s32 indexed = indices != NULL;

	if( !r || !r->frame_open || !vertices || vertex_count <= 0 )
		return;
	if( indexed && index_count <= 0 )
		return;

	memset( &key, 0, sizeof( key ) );
	key.layout = layout;
	key.topology = (wp_s32)topology_type;
	key.blend = (wp_s32)r->blend_mode;
	key.fill = (wp_s32)r->fill_mode;
	key.cull = (wp_s32)r->cull_mode;
	key.depth_test = r->depth_test_enabled != 0;
	key.depth_write = r->depth_write_enabled != 0;
	key.depth_func = (wp_s32)r->depth_func;
	pso = wp_renderer_dx12_get_pso( r, &key );
	if( !pso )
		return;

	if( !wp_renderer_dx12_upload( r, vertices, (UINT64)vertex_count * (UINT64)stride, &vb_offset ) )
		return;
	if( indexed &&
		!wp_renderer_dx12_upload( r, indices, (UINT64)index_count * sizeof( uint16_t ), &ib_offset ) )
		return;

	wp_renderer_dx12_update_mvp( r );
	wp_renderer_dx12_apply_viewport( r );

	ID3D12GraphicsCommandList_SetPipelineState( r->command_list, pso );
	ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants( r->command_list, 0, 16, &r->mvp_matrix, 0 );
	wp_renderer_dx12_bind_texture( r );

	ID3D12GraphicsCommandList_IASetPrimitiveTopology( r->command_list, topology );
	vbv.BufferLocation = ID3D12Resource_GetGPUVirtualAddress( r->upload_buffer ) + vb_offset;
	vbv.SizeInBytes = (UINT)( (UINT64)vertex_count * (UINT64)stride );
	vbv.StrideInBytes = (UINT)stride;
	ID3D12GraphicsCommandList_IASetVertexBuffers( r->command_list, 0, 1, &vbv );

	if( indexed )
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = ID3D12Resource_GetGPUVirtualAddress( r->upload_buffer ) + ib_offset;
		ibv.SizeInBytes = (UINT)( (UINT)index_count * sizeof( uint16_t ) );
		ibv.Format = DXGI_FORMAT_R16_UINT;
		ID3D12GraphicsCommandList_IASetIndexBuffer( r->command_list, &ibv );
		ID3D12GraphicsCommandList_DrawIndexedInstanced( r->command_list, (UINT)index_count, 1, 0, 0, 0 );
	}
	else
		ID3D12GraphicsCommandList_DrawInstanced( r->command_list, (UINT)vertex_count, 1, 0, 0 );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

static wp_s32 wp_renderer_dx12_init( wp_renderer_dx12 *r )
{
	IDXGIFactory4 *factory = NULL;
	IDXGISwapChain1 *sc1 = NULL;
	DXGI_SWAP_CHAIN_DESC1 sd;
	D3D12_COMMAND_QUEUE_DESC qd;
	D3D12_DESCRIPTOR_HEAP_DESC hd;
	D3D12_HEAP_PROPERTIES hp;
	D3D12_RESOURCE_DESC ud;
	wp_s32 ok = 0;
	const UINT8 white[4] = { 255, 255, 255, 255 };

	if( FAILED( D3D12CreateDevice( NULL, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&r->device ) ) )
		return 0;

	memset( &qd, 0, sizeof( qd ) );
	qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if( FAILED( ID3D12Device_CreateCommandQueue( r->device, &qd, &IID_ID3D12CommandQueue,
												 (void **)&r->command_queue ) ) )
		return 0;

	if( FAILED( CreateDXGIFactory1( &IID_IDXGIFactory4, (void **)&factory ) ) )
		return 0;

	memset( &sd, 0, sizeof( sd ) );
	sd.Width = (UINT)r->width;
	sd.Height = (UINT)r->height;
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = DX12_FRAME_COUNT;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	if( SUCCEEDED( IDXGIFactory4_CreateSwapChainForHwnd( factory, (IUnknown *)r->command_queue, r->hwnd, &sd,
														 NULL, NULL, &sc1 ) ) )
	{
		IDXGIFactory4_MakeWindowAssociation( factory, r->hwnd, DXGI_MWA_NO_ALT_ENTER );
		if( SUCCEEDED( IDXGISwapChain1_QueryInterface( sc1, &IID_IDXGISwapChain3, (void **)&r->swap_chain ) ) )
			ok = 1;
		IDXGISwapChain1_Release( sc1 );
	}
	IDXGIFactory4_Release( factory );
	if( !ok )
		return 0;

	memset( &hd, 0, sizeof( hd ) );
	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	hd.NumDescriptors = DX12_FRAME_COUNT;
	if( FAILED( ID3D12Device_CreateDescriptorHeap( r->device, &hd, &IID_ID3D12DescriptorHeap,
												   (void **)&r->rtv_heap ) ) )
		return 0;
	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hd.NumDescriptors = 1;
	if( FAILED( ID3D12Device_CreateDescriptorHeap( r->device, &hd, &IID_ID3D12DescriptorHeap,
												   (void **)&r->dsv_heap ) ) )
		return 0;
	hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hd.NumDescriptors = DX12_SRV_COUNT;
	hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if( FAILED( ID3D12Device_CreateDescriptorHeap( r->device, &hd, &IID_ID3D12DescriptorHeap,
												   (void **)&r->srv_heap ) ) )
		return 0;
	r->rtv_inc = ID3D12Device_GetDescriptorHandleIncrementSize( r->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
	r->srv_inc = ID3D12Device_GetDescriptorHandleIncrementSize( r->device,
																D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

	if( !wp_renderer_dx12_create_rtvs( r ) || !wp_renderer_dx12_create_depth( r ) )
		return 0;

	if( FAILED( ID3D12Device_CreateCommandAllocator( r->device, D3D12_COMMAND_LIST_TYPE_DIRECT,
													 &IID_ID3D12CommandAllocator,
													 (void **)&r->command_allocator ) ) ||
		FAILED( ID3D12Device_CreateCommandList( r->device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT,
												r->command_allocator, NULL, &IID_ID3D12GraphicsCommandList,
												(void **)&r->command_list ) ) )
		return 0;
	ID3D12GraphicsCommandList_Close( r->command_list );

	if( FAILED( ID3D12Device_CreateCommandAllocator( r->device, D3D12_COMMAND_LIST_TYPE_DIRECT,
													 &IID_ID3D12CommandAllocator,
													 (void **)&r->upload_allocator ) ) ||
		FAILED( ID3D12Device_CreateCommandList( r->device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT,
												r->upload_allocator, NULL, &IID_ID3D12GraphicsCommandList,
												(void **)&r->upload_list ) ) )
		return 0;
	ID3D12GraphicsCommandList_Close( r->upload_list );

	if( FAILED( ID3D12Device_CreateFence( r->device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence,
										  (void **)&r->fence ) ) )
		return 0;
	r->fence_event = CreateEventW( NULL, FALSE, FALSE, NULL );
	if( !r->fence_event )
		return 0;

	if( !wp_renderer_dx12_compile( s_vs_pc_src, "vs_main", "vs_5_0", &r->vs_pc ) ||
		!wp_renderer_dx12_compile( s_ps_pc_src, "ps_main", "ps_5_0", &r->ps_pc ) ||
		!wp_renderer_dx12_compile( s_vs_ptc_src, "vs_main", "vs_5_0", &r->vs_ptc ) ||
		!wp_renderer_dx12_compile( s_ps_ptc_src, "ps_main", "ps_5_0", &r->ps_ptc ) )
		return 0;
	if( !wp_renderer_dx12_create_root_signature( r ) )
		return 0;

	hp = wp_renderer_dx12_heap( D3D12_HEAP_TYPE_UPLOAD );
	ud = wp_renderer_dx12_buffer_desc( DX12_UPLOAD_SIZE );
	if( FAILED( ID3D12Device_CreateCommittedResource( r->device, &hp, D3D12_HEAP_FLAG_NONE, &ud,
													  D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
													  &IID_ID3D12Resource, (void **)&r->upload_buffer ) ) ||
		FAILED( ID3D12Resource_Map( r->upload_buffer, 0, NULL, (void **)&r->upload_ptr ) ) )
		return 0;

	r->white_texture =
		wp_renderer_dx12_create_texture_resource( r, white, 1, 1, WORKPHONE_PIXEL_FORMAT_RGBA8 );
	if( !r->white_texture )
		return 0;
	wp_renderer_dx12_write_srv( r, r->white_texture, 0 );
	r->srv_next = 1;
	r->texture_slot = -1;
	return 1;
}

wp_renderer_dx12 *wp_renderer_dx12_create( void *hwnd, wp_s32 width, wp_s32 height )
{
	wp_renderer_dx12 *r;

	if( !hwnd || width <= 0 || height <= 0 )
		return NULL;
	r = (wp_renderer_dx12 *)malloc( sizeof( wp_renderer_dx12 ) );
	if( !r )
		return NULL;
	memset( r, 0, sizeof( wp_renderer_dx12 ) );
	r->hwnd = (HWND)hwnd;
	r->width = width;
	r->height = height;
	r->clear_a = 1.0f;
	r->clear_depth = 1.0f;
	r->viewport.x = 0;
	r->viewport.y = 0;
	r->viewport.width = width;
	r->viewport.height = height;
	r->blend_mode = WORKPHONE_BLEND_MODE_NONE;
	r->fill_mode = WORKPHONE_FILL_MODE_SOLID;
	r->cull_mode = WORKPHONE_CULL_MODE_BACK;
	r->depth_test_enabled = 1;
	r->depth_write_enabled = 1;
	r->depth_func = WORKPHONE_DEPTH_FUNC_LESS;
	r->texture_slot = -1;
	wp_mat4f_identity( &r->world_matrix );
	wp_mat4f_identity( &r->view_matrix );
	wp_mat4f_identity( &r->proj_matrix );
	wp_mat4f_identity( &r->mvp_matrix );
	r->mvp_dirty = 1;

	if( !wp_renderer_dx12_init( r ) )
	{
		wp_renderer_dx12_destroy( r );
		return NULL;
	}
	return r;
}

void wp_renderer_dx12_destroy( wp_renderer_dx12 *r )
{
	wp_s32 i;

	if( !r )
		return;
	if( r->frame_open && r->command_list )
		ID3D12GraphicsCommandList_Close( r->command_list );
	r->frame_open = 0;
	wp_renderer_dx12_wait_for_gpu( r );
	wp_renderer_dx12_release_retired( r );
	free( r->retired );

	for( i = 0; i < r->pso_count; i++ )
		DX12_RELEASE( r->psos[i].pso );
	if( r->texture_owned )
		DX12_RELEASE( r->texture );
	DX12_RELEASE( r->white_texture );
	if( r->upload_buffer && r->upload_ptr )
		ID3D12Resource_Unmap( r->upload_buffer, 0, NULL );
	DX12_RELEASE( r->upload_buffer );
	DX12_RELEASE( r->root_signature );
	DX12_RELEASE( r->vs_pc );
	DX12_RELEASE( r->ps_pc );
	DX12_RELEASE( r->vs_ptc );
	DX12_RELEASE( r->ps_ptc );
	wp_renderer_dx12_release_targets( r );
	DX12_RELEASE( r->srv_heap );
	DX12_RELEASE( r->dsv_heap );
	DX12_RELEASE( r->rtv_heap );
	DX12_RELEASE( r->command_list );
	DX12_RELEASE( r->command_allocator );
	DX12_RELEASE( r->upload_list );
	DX12_RELEASE( r->upload_allocator );
	DX12_RELEASE( r->fence );
	if( r->fence_event )
		CloseHandle( r->fence_event );
	DX12_RELEASE( r->swap_chain );
	DX12_RELEASE( r->command_queue );
	DX12_RELEASE( r->device );
	free( r );
}

wp_s32 wp_renderer_dx12_resize( wp_renderer_dx12 *r, wp_s32 width, wp_s32 height )
{
	if( !r || width <= 0 || height <= 0 || !r->swap_chain )
		return 0;
	if( r->frame_open )
		wp_renderer_dx12_end_frame( r );
	wp_renderer_dx12_wait_for_gpu( r );
	wp_renderer_dx12_release_targets( r );

	if( FAILED( IDXGISwapChain3_ResizeBuffers( r->swap_chain, DX12_FRAME_COUNT, (UINT)width, (UINT)height,
											   DXGI_FORMAT_UNKNOWN, 0 ) ) )
		return 0;
	r->width = width;
	r->height = height;
	r->viewport.x = 0;
	r->viewport.y = 0;
	r->viewport.width = width;
	r->viewport.height = height;
	return wp_renderer_dx12_create_rtvs( r ) && wp_renderer_dx12_create_depth( r );
}

/* =========================================================================
 * Frame lifecycle
 * ====================================================================== */

void wp_renderer_dx12_begin_frame( wp_renderer_dx12 *r )
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv, dsv;
	ID3D12DescriptorHeap *heaps[1];

	if( !r || r->frame_open || !r->swap_chain || !r->render_targets[0] )
		return;
	if( FAILED( ID3D12CommandAllocator_Reset( r->command_allocator ) ) ||
		FAILED( ID3D12GraphicsCommandList_Reset( r->command_list, r->command_allocator, NULL ) ) )
		return;

	r->frame_index = IDXGISwapChain3_GetCurrentBackBufferIndex( r->swap_chain );
	wp_renderer_dx12_barrier( r->command_list, r->render_targets[r->frame_index],
							  D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );

	ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart( r->rtv_heap, &rtv );
	rtv.ptr += (SIZE_T)r->frame_index * r->rtv_inc;
	ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart( r->dsv_heap, &dsv );
	ID3D12GraphicsCommandList_OMSetRenderTargets( r->command_list, 1, &rtv, FALSE, &dsv );

	heaps[0] = r->srv_heap;
	ID3D12GraphicsCommandList_SetDescriptorHeaps( r->command_list, 1, heaps );
	ID3D12GraphicsCommandList_SetGraphicsRootSignature( r->command_list, r->root_signature );

	r->upload_offset = 0;
	r->srv_next = 1;
	r->texture_slot = -1;
	r->frame_open = 1;
	wp_renderer_dx12_apply_viewport( r );
}

void wp_renderer_dx12_end_frame( wp_renderer_dx12 *r )
{
	ID3D12CommandList *lists[1];

	if( !r || !r->frame_open )
		return;
	wp_renderer_dx12_barrier( r->command_list, r->render_targets[r->frame_index],
							  D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );
	ID3D12GraphicsCommandList_Close( r->command_list );
	lists[0] = (ID3D12CommandList *)r->command_list;
	ID3D12CommandQueue_ExecuteCommandLists( r->command_queue, 1, lists );
	wp_renderer_dx12_wait_for_gpu( r );
	r->frame_open = 0;
	wp_renderer_dx12_release_retired( r );
}

void wp_renderer_dx12_present( wp_renderer_dx12 *r, wp_s32 vsync )
{
	if( !r || !r->swap_chain )
		return;
	if( r->frame_open )
		wp_renderer_dx12_end_frame( r );
	IDXGISwapChain3_Present( r->swap_chain, vsync ? 1 : 0, 0 );
}

/* =========================================================================
 * Clear
 * ====================================================================== */

void wp_renderer_dx12_set_clear_color( wp_renderer_dx12 *r, wp_f32 cr, wp_f32 cg, wp_f32 cb, wp_f32 ca )
{
	if( !r )
		return;
	r->clear_r = cr;
	r->clear_g = cg;
	r->clear_b = cb;
	r->clear_a = ca;
}

void wp_renderer_dx12_set_clear_depth( wp_renderer_dx12 *r, wp_f32 depth )
{
	if( !r )
		return;
	r->clear_depth = depth;
}

void wp_renderer_dx12_clear( wp_renderer_dx12 *r, wp_u32 flags )
{
	D3D12_CPU_DESCRIPTOR_HANDLE h;

	if( !r || !r->frame_open )
		return;
	if( flags & WORKPHONE_CLEAR_FLAG_COLOR )
	{
		FLOAT color[4];
		color[0] = r->clear_r;
		color[1] = r->clear_g;
		color[2] = r->clear_b;
		color[3] = r->clear_a;
		ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart( r->rtv_heap, &h );
		h.ptr += (SIZE_T)r->frame_index * r->rtv_inc;
		ID3D12GraphicsCommandList_ClearRenderTargetView( r->command_list, h, color, 0, NULL );
	}
	if( flags & WORKPHONE_CLEAR_FLAG_DEPTH )
	{
		ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart( r->dsv_heap, &h );
		ID3D12GraphicsCommandList_ClearDepthStencilView( r->command_list, h, D3D12_CLEAR_FLAG_DEPTH,
														 r->clear_depth, 0, 0, NULL );
	}
}

/* =========================================================================
 * Viewport and scissor
 * ====================================================================== */

void wp_renderer_dx12_set_viewport( wp_renderer_dx12 *r, wp_viewport_i viewport )
{
	if( !r )
		return;
	r->viewport = viewport;
	if( r->frame_open )
		wp_renderer_dx12_apply_viewport( r );
}

wp_viewport_i wp_renderer_dx12_get_viewport( const wp_renderer_dx12 *r )
{
	wp_viewport_i zero;
	memset( &zero, 0, sizeof( zero ) );
	if( r )
		return r->viewport;
	return zero;
}

void wp_renderer_dx12_set_scissor_enabled( wp_renderer_dx12 *r, wp_s32 enabled )
{
	if( !r )
		return;
	r->scissor_enabled = enabled;
	if( r->frame_open )
		wp_renderer_dx12_apply_viewport( r );
}

void wp_renderer_dx12_set_scissor_rect( wp_renderer_dx12 *r, wp_viewport_i scissor )
{
	if( !r )
		return;
	r->scissor = scissor;
	if( r->frame_open )
		wp_renderer_dx12_apply_viewport( r );
}

/* =========================================================================
 * Render state
 * ====================================================================== */

void wp_renderer_dx12_set_blend_mode( wp_renderer_dx12 *r, wp_blend_mode mode )
{
	if( !r )
		return;
	r->blend_mode = mode;
}

wp_blend_mode wp_renderer_dx12_get_blend_mode( const wp_renderer_dx12 *r )
{
	return r ? r->blend_mode : WORKPHONE_BLEND_MODE_NONE;
}

void wp_renderer_dx12_set_fill_mode( wp_renderer_dx12 *r, wp_fill_mode mode )
{
	if( !r )
		return;
	r->fill_mode = mode;
}

wp_fill_mode wp_renderer_dx12_get_fill_mode( const wp_renderer_dx12 *r )
{
	return r ? r->fill_mode : WORKPHONE_FILL_MODE_SOLID;
}

void wp_renderer_dx12_set_cull_mode( wp_renderer_dx12 *r, wp_cull_mode mode )
{
	if( !r )
		return;
	r->cull_mode = mode;
}

wp_cull_mode wp_renderer_dx12_get_cull_mode( const wp_renderer_dx12 *r )
{
	return r ? r->cull_mode : WORKPHONE_CULL_MODE_BACK;
}

void wp_renderer_dx12_set_depth_test_enabled( wp_renderer_dx12 *r, wp_s32 enabled )
{
	if( !r )
		return;
	r->depth_test_enabled = enabled;
}

wp_s32 wp_renderer_dx12_get_depth_test_enabled( const wp_renderer_dx12 *r )
{
	return r ? r->depth_test_enabled : 0;
}

void wp_renderer_dx12_set_depth_write_enabled( wp_renderer_dx12 *r, wp_s32 enabled )
{
	if( !r )
		return;
	r->depth_write_enabled = enabled;
}

wp_s32 wp_renderer_dx12_get_depth_write_enabled( const wp_renderer_dx12 *r )
{
	return r ? r->depth_write_enabled : 0;
}

void wp_renderer_dx12_set_depth_func( wp_renderer_dx12 *r, wp_depth_func func )
{
	if( !r )
		return;
	r->depth_func = func;
}

wp_depth_func wp_renderer_dx12_get_depth_func( const wp_renderer_dx12 *r )
{
	return r ? r->depth_func : WORKPHONE_DEPTH_FUNC_LESS;
}

/* =========================================================================
 * Transform matrices
 * ====================================================================== */

void wp_renderer_dx12_set_world_matrix( wp_renderer_dx12 *r, const wp_mat4f *mat )
{
	if( !r || !mat )
		return;
	r->world_matrix = *mat;
	r->mvp_dirty = 1;
}

void wp_renderer_dx12_set_view_matrix( wp_renderer_dx12 *r, const wp_mat4f *mat )
{
	if( !r || !mat )
		return;
	r->view_matrix = *mat;
	r->mvp_dirty = 1;
}

void wp_renderer_dx12_set_projection_matrix( wp_renderer_dx12 *r, const wp_mat4f *mat )
{
	if( !r || !mat )
		return;
	r->proj_matrix = *mat;
	r->mvp_dirty = 1;
}

/* =========================================================================
 * Textures
 * ====================================================================== */

void wp_renderer_dx12_set_texture( wp_renderer_dx12 *r, const void *pixels, wp_s32 width, wp_s32 height,
								   wp_pixel_format format )
{
	ID3D12Resource *texture;

	if( !r )
		return;
	if( !pixels )
	{
		wp_renderer_dx12_replace_texture( r, NULL, 0 );
		return;
	}
	texture = wp_renderer_dx12_create_texture_resource( r, pixels, width, height, format );
	if( texture )
		wp_renderer_dx12_replace_texture( r, texture, 1 );
}

void wp_renderer_dx12_set_texture_native( wp_renderer_dx12 *r, void *texture )
{
	if( !r )
		return;
	wp_renderer_dx12_replace_texture( r, (ID3D12Resource *)texture, 0 );
}

void *wp_renderer_dx12_create_texture_native( wp_renderer_dx12 *r, const void *pixels, wp_s32 width,
											  wp_s32 height, wp_pixel_format format )
{
	return wp_renderer_dx12_create_texture_resource( r, pixels, width, height, format );
}

void wp_renderer_dx12_destroy_texture_native( void *texture )
{
	ID3D12Resource *resource = (ID3D12Resource *)texture;
	if( resource )
		ID3D12Resource_Release( resource );
}

/* =========================================================================
 * Draw calls
 * ====================================================================== */

void wp_renderer_dx12_draw_triangles_pc( wp_renderer_dx12 *r, const wp_vertex_pc *vertices,
										 wp_s32 vertex_count )
{
	wp_renderer_dx12_draw( r, 0, (wp_s32)sizeof( wp_vertex_pc ), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
						   D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, vertex_count, NULL, 0 );
}

void wp_renderer_dx12_draw_indexed_triangles_pc( wp_renderer_dx12 *r, const wp_vertex_pc *vertices,
												 wp_s32 vertex_count, const uint16_t *indices,
												 wp_s32 index_count )
{
	if( !indices )
		return;
	wp_renderer_dx12_draw( r, 0, (wp_s32)sizeof( wp_vertex_pc ), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
						   D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, vertex_count, indices,
						   index_count );
}

void wp_renderer_dx12_draw_lines_pc( wp_renderer_dx12 *r, const wp_vertex_pc *vertices, wp_s32 vertex_count )
{
	wp_renderer_dx12_draw( r, 0, (wp_s32)sizeof( wp_vertex_pc ), D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
						   D3D_PRIMITIVE_TOPOLOGY_LINELIST, vertices, vertex_count, NULL, 0 );
}

void wp_renderer_dx12_draw_points_pc( wp_renderer_dx12 *r, const wp_vertex_pc *vertices, wp_s32 vertex_count )
{
	wp_renderer_dx12_draw( r, 0, (wp_s32)sizeof( wp_vertex_pc ), D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
						   D3D_PRIMITIVE_TOPOLOGY_POINTLIST, vertices, vertex_count, NULL, 0 );
}

void wp_renderer_dx12_draw_triangles_ptc( wp_renderer_dx12 *r, const wp_vertex_ptc *vertices,
										  wp_s32 vertex_count )
{
	wp_renderer_dx12_draw( r, 1, (wp_s32)sizeof( wp_vertex_ptc ), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
						   D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, vertex_count, NULL, 0 );
}

void wp_renderer_dx12_draw_indexed_triangles_ptc( wp_renderer_dx12 *r, const wp_vertex_ptc *vertices,
												  wp_s32 vertex_count, const uint16_t *indices,
												  wp_s32 index_count )
{
	if( !indices )
		return;
	wp_renderer_dx12_draw( r, 1, (wp_s32)sizeof( wp_vertex_ptc ), D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
						   D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, vertex_count, indices,
						   index_count );
}

/* =========================================================================
 * Dimension queries and native access
 * ====================================================================== */

wp_s32 wp_renderer_dx12_get_width( const wp_renderer_dx12 *r )
{
	return r ? r->width : 0;
}

wp_s32 wp_renderer_dx12_get_height( const wp_renderer_dx12 *r )
{
	return r ? r->height : 0;
}

void wp_renderer_dx12_get_native( const wp_renderer_dx12 *r, void **pp )
{
	if( !pp )
		return;
	*pp = r ? r->native : NULL;
}

void wp_renderer_dx12_set_native( wp_renderer_dx12 *r, void *native )
{
	if( r )
		r->native = native;
}

void *wp_renderer_dx12_get_device( const wp_renderer_dx12 *r )
{
	return r ? (void *)r->device : NULL;
}

void *wp_renderer_dx12_get_command_queue( const wp_renderer_dx12 *r )
{
	return r ? (void *)r->command_queue : NULL;
}

void *wp_renderer_dx12_get_swap_chain( const wp_renderer_dx12 *r )
{
	return r ? (void *)r->swap_chain : NULL;
}

#endif /* _WIN32 */
