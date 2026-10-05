/**
 * @file workphone_graphics_renderer_dx11.c
 * @brief Direct3D 11 renderer implementation.
 *
 * Mirrors the software renderer interface but drives a hardware D3D11
 * pipeline.  Uses the C COM macro API (COBJMACROS) so that the file
 * compiles as plain C.
 */

#ifdef _WIN32

#    define COBJMACROS
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    include <d3d11.h>
#    include <d3dcompiler.h>
#    include <dxgi.h>
#    include <dxgi1_5.h>
#    include <limits.h>
#    include <stdlib.h>
#    include <string.h>
#    include <float.h>
#    include <stdio.h>

#    define WORKPHONE_IMPLEMENTATION
#    include "workphone_prerequisites.h"
#    include "workphone_graphics_viewport.h"
#    include "workphone_ui.h"
#    include "workphone.h"
#    include "workphone_graphics_renderer_dx11.h"
#    include "workphone_graphics_dx11_pixel_shader.h"
#    include "workphone_graphics_dx11_vertex_shader.h"

#    pragma comment( lib, "d3d11.lib" )
#    pragma comment( lib, "dxgi.lib" )
#    pragma comment( lib, "d3dcompiler.lib" )

/* =========================================================================
 * Constants
 * ====================================================================== */

#    define DX11_INITIAL_VB_SIZE ( 64 * 1024 )
#    define DX11_INITIAL_IB_SIZE ( 32 * 1024 )

/* =========================================================================
 * Embedded HLSL sources
 * ====================================================================== */

/* ---- position + colour (pc) vertex shader ----------------------------- */
static const wp_c8 s_vs_pc_src[] =
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

/* ---- shared pixel shader ---------------------------------------------- */
static const wp_c8 s_ps_src[] =
    "struct PS_IN { float4 pos : SV_POSITION; float4 color : COLOR; };\n"
    "float4 ps_main(PS_IN i) : SV_TARGET { return i.color; }\n";

/* ---- position + texcoord + colour (ptc) vertex shader ----------------- */
static const wp_c8 s_vs_ptc_src[] =
    "cbuffer Transform : register(b0) { row_major float4x4 mvp; };\n"
    "struct VS_IN  { float3 pos : POSITION; float2 uv : TEXCOORD0;\n"
    "                uint color : COLOR; };\n"
    "struct VS_OUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD0;\n"
    "                float4 color : COLOR; };\n"
    "VS_OUT vs_main(VS_IN i)\n"
    "{\n"
    "    VS_OUT o;\n"
    "    o.pos = mul(mvp, float4(i.pos, 1.0));\n"
    "    o.pos.z = 0.5 * (o.pos.z + o.pos.w);\n"
    "    o.uv  = i.uv;\n"
    "    o.color = float4(\n"
    "        float((i.color >> 24u) & 0xFFu) / 255.0,\n"
    "        float((i.color >> 16u) & 0xFFu) / 255.0,\n"
    "        float((i.color >>  8u) & 0xFFu) / 255.0,\n"
    "        float( i.color         & 0xFFu) / 255.0);\n"
    "    return o;\n"
    "}\n";

/* ---- position + texcoord + colour pixel shader ------------------------ */
static const wp_c8 s_ps_ptc_src[] =
    "Texture2D tex : register(t0);\n"
    "SamplerState tex_sampler : register(s0);\n"
    "struct PS_IN { float4 pos : SV_POSITION; float2 uv : TEXCOORD0;\n"
    "               float4 color : COLOR; };\n"
    "float4 ps_main(PS_IN i) : SV_TARGET\n"
    "{\n"
    "    return tex.Sample(tex_sampler, i.uv) * i.color;\n"
    "}\n";

/* ---- lit position + normal + texcoord + colour shaders --------------- */
static const wp_c8 s_vs_pntc_src[] =
    "cbuffer Transform : register(b0) { row_major float4x4 mvp; row_major float4x4 world;\n"
    " row_major float4x4 normal_matrix; };\n"
    "cbuffer Material : register(b1) { float4 base_color; float4 emissive_color;\n"
    " float4 specular_color; float4 light_color; float4 light_direction;\n"
    " float4 camera_position; float4 surface; float4 uv_transform;\n"
    " float4 controls; float4 map_flags; float4 extra_map_flags; float4 texture_sources; float4 projection; float4 ambient_color; float4 environment; };\n"
    "struct VS_IN { float3 pos : POSITION; float3 normal : NORMAL;\n"
    " float2 uv : TEXCOORD0; uint color : COLOR; };\n"
    "struct VS_OUT { float4 pos : SV_POSITION; float3 world_pos : TEXCOORD0;\n"
    " float3 normal : TEXCOORD1; float2 uv : TEXCOORD2; float4 color : COLOR;\n"
    " float3 object_pos : TEXCOORD3; float3 object_normal : TEXCOORD4; float4 clip_pos : TEXCOORD5; };\n"
    "VS_OUT vs_main(VS_IN i) { VS_OUT o;\n"
    " o.pos = mul(mvp, float4(i.pos, 1.0));\n"
    " o.pos.z = 0.5 * (o.pos.z + o.pos.w);\n"
    " o.world_pos = mul(world, float4(i.pos, 1.0)).xyz;\n"
    " o.normal = normalize(mul((float3x3)normal_matrix, i.normal));\n"
    " o.uv = i.uv; o.object_pos = i.pos; o.object_normal = i.normal; o.clip_pos = o.pos;\n"
    " o.color = float4(float((i.color >> 24u) & 255u),\n"
    "  float((i.color >> 16u) & 255u), float((i.color >> 8u) & 255u),\n"
    "  float(i.color & 255u)) / 255.0; return o; }\n";

static const wp_c8 s_ps_pntc_src[] =
    "TextureCube environment_tex : register(t7); SamplerState environment_sampler : register(s1);\n"
    "Texture2D tex : register(t0); SamplerState tex_sampler : register(s0);\n"
    "Texture2D normal_tex : register(t1); Texture2D metallic_tex : register(t2);\n"
    "Texture2D roughness_tex : register(t3); Texture2D emission_tex : register(t4);\n"
    "Texture2D ao_tex : register(t5); Texture2D opacity_tex : register(t6);\n"
    "cbuffer Material : register(b1) { float4 base_color; float4 emissive_color;\n"
    " float4 specular_color; float4 light_color; float4 light_direction;\n"
    " float4 camera_position; float4 surface; float4 uv_transform;\n"
    " float4 controls; float4 map_flags; float4 extra_map_flags; float4 texture_sources; float4 projection; float4 ambient_color; float4 environment; };\n"
    "struct PS_IN { float4 pos : SV_POSITION; float3 world_pos : TEXCOORD0;\n"
    " float3 normal : TEXCOORD1; float2 uv : TEXCOORD2; float4 color : COLOR;\n"
    " float3 object_pos : TEXCOORD3; float3 object_normal : TEXCOORD4; float4 clip_pos : TEXCOORD5; };\n"
    "float2 boxUV(float3 p, float3 n) { float3 axis = abs(n);\n"
    " if (axis.x >= axis.y && axis.x >= axis.z) return float2(n.x < 0.0 ? p.z : -p.z, p.y);\n"
    " if (axis.y >= axis.z) return float2(p.x, n.y < 0.0 ? -p.z : p.z);\n"
    " return float2(n.z < 0.0 ? -p.x : p.x, p.y); }\n"
    "float2 materialUV(PS_IN i) { float2 uv = i.uv;\n"
    " if (projection.x > 0.5) {\n"
    "     if (projection.x < 1.5) uv = boxUV(i.world_pos, i.normal);\n"
    "     else if (projection.x < 2.5) uv = boxUV(i.object_pos, i.object_normal);\n"
    "     else if (projection.x < 3.5) uv = i.world_pos.xz;\n"
    "     else if (projection.x < 4.5) uv = i.world_pos.xy;\n"
    "     else if (projection.x < 5.5) uv = i.world_pos.yz;\n"
    "     else uv = i.clip_pos.xy / i.clip_pos.w * float2(0.5, -0.5) + 0.5;\n"
    "     uv *= max(projection.y, 0.0001); }\n"
    " float angle = uv_transform.w * 0.01745329252;\n"
    " float sine = sin(angle); float cosine = cos(angle); uv = uv * surface.zw - 0.5;\n"
    " return float2(uv.x * cosine - uv.y * sine, uv.x * sine + uv.y * cosine)\n"
    "        + 0.5 + uv_transform.xy; }\n"
    "float3 fresnelSchlick(float cosTheta, float3 f0)\n"
    " { return f0 + (1.0 - f0) * pow(saturate(1.0 - cosTheta), 5.0); }\n"
    "float4 ps_main(PS_IN i, bool front : SV_IsFrontFace) : SV_TARGET {\n"
    " i.uv = materialUV(i);\n"
    " float4 sampled = tex.Sample(tex_sampler, i.uv);\n"
    " float3 albedo = pow(max(sampled.rgb * base_color.rgb * i.color.rgb, 0.0), 2.2);\n"
    " float alpha = base_color.a * i.color.a;\n"
    " if (texture_sources.w < 0.5) alpha *= sampled.a;\n"
    " else if (extra_map_flags.y > 0.5 && texture_sources.w < 2.5) {\n"
    "     float4 opacity = opacity_tex.Sample(tex_sampler, i.uv);\n"
    "     alpha *= texture_sources.w < 1.5 ? opacity.r : opacity.a; }\n"
    " if (controls.z >= 0.0) clip(alpha - controls.z);\n"
    " float3 emission = emissive_color.rgb;\n"
    " if (map_flags.w > 0.5) emission *= pow(max(emission_tex.Sample(tex_sampler, i.uv).rgb, 0.0), 2.2);\n"
    " if (uv_transform.z < 0.5) { float3 c = pow(max(albedo + emission, 0.0), 1.0 / 2.2);\n"
    "     return float4(controls.w > 0.5 ? c * alpha : c, alpha); }\n"
    " float metalness = saturate(surface.x);\n"
    " float roughness = clamp(surface.y, 0.045, 1.0);\n"
    " float4 metallic = map_flags.y > 0.5 ? metallic_tex.Sample(tex_sampler, i.uv) : 1.0;\n"
    " float4 rough = map_flags.z > 0.5 ? roughness_tex.Sample(tex_sampler, i.uv) : 1.0;\n"
    " if (map_flags.y > 0.5) metalness *= texture_sources.x > 0.5 && texture_sources.x < 1.5 ? metallic.a : metallic.r;\n"
    " if (texture_sources.x > 1.5 && texture_sources.x < 2.5) metalness = surface.x * sampled.a;\n"
    " if (map_flags.z > 0.5) {\n"
    "     float channel = texture_sources.y < 0.5 ? rough.r : rough.a;\n"
    "     if (texture_sources.y > 2.5 && texture_sources.y < 3.5) channel = rough.g;\n"
    "     if (texture_sources.y > 3.5) channel = 1.0 - rough.a;\n"
    "     roughness *= channel; }\n"
    " roughness = clamp(roughness, 0.045, 1.0);\n"
    " float3 n = normalize(front ? i.normal : -i.normal);\n"
    " if (map_flags.x > 0.5) {\n"
    "     float3 dp1 = ddx(i.world_pos), dp2 = ddy(i.world_pos);\n"
    "     float2 duv1 = ddx(i.uv), duv2 = ddy(i.uv);\n"
    "     float3 p2 = cross(dp2, n), p1 = cross(n, dp1);\n"
    "     float3 t = p2 * duv1.x + p1 * duv2.x, b = p2 * duv1.y + p1 * duv2.y;\n"
    "     float inv = rsqrt(max(max(dot(t,t), dot(b,b)), 1e-8));\n"
    "     float3 mapped = normal_tex.Sample(tex_sampler, i.uv).xyz * 2.0 - 1.0;\n"
    "     mapped.xy *= controls.x; n = normalize(t * inv * mapped.x + b * inv * mapped.y + n * mapped.z); }\n"
    " float3 dn1 = ddx(n), dn2 = ddy(n);\n"
    " float variance = min(2.0 * (dot(dn1, dn1) + dot(dn2, dn2)), 0.25);\n"
    " roughness = pow(saturate(pow(roughness, 4.0) + variance), 0.25);\n"
    " float3 v = normalize(camera_position.xyz - i.world_pos);\n"
    " float3 l = -light_direction.xyz * rsqrt(max(dot(light_direction.xyz, light_direction.xyz), 1e-8));\n"
    " float3 h = (v + l) * rsqrt(max(dot(v + l, v + l), 1e-8));\n"
    " float ndl = saturate(dot(n, l)); float ndv = max(saturate(dot(n, v)), 0.001);\n"
    " float ndh = saturate(dot(n, h)); float vdh = saturate(dot(v, h));\n"
    " float a = roughness * roughness; float a2 = a * a;\n"
    " float denom = ndh * ndh * (a2 - 1.0) + 1.0;\n"
    " float d = a2 / max(3.14159265 * denom * denom, 1e-12);\n"
    " float gv = ndl * sqrt(ndv * ndv * (1.0 - a2) + a2);\n"
    " float gl = ndv * sqrt(ndl * ndl * (1.0 - a2) + a2);\n"
    " float3 dielectric = saturate(specular_color.rgb);\n"
    " float3 f0 = lerp(dielectric, albedo, metalness);\n"
    " float3 f = fresnelSchlick(vdh, f0);\n"
    " float3 specular = d * f * 0.5 / max(gv + gl, 1e-6);\n"
    " float3 diffuse = (1.0 - f) * (1.0 - metalness) * albedo / 3.14159265;\n"
    " float3 radiance = light_color.rgb * light_color.a;\n"
    " float hemi = 0.35 + 0.65 * saturate(n.y * 0.5 + 0.5);\n"
    " float3 color = (diffuse + specular) * radiance * ndl;\n"
    " float ao = 1.0;\n"
    " if (extra_map_flags.x > 0.5 && texture_sources.z < 1.5) {\n"
    "     float4 occlusion = ao_tex.Sample(tex_sampler, i.uv);\n"
    "     ao = texture_sources.z < 0.5 ? occlusion.r : occlusion.b; }\n"
    " if (texture_sources.z > 1.5 && texture_sources.z < 2.5) ao = i.color.a;\n"
    " ao = lerp(1.0, ao, saturate(controls.y));\n"
    " float3 ambient = ambient_color.w > 0.5 ? max(ambient_color.rgb, 0.0) : camera_position.www;\n"
    " float3 ambientF = f0 + (max(1.0 - roughness, f0) - f0) * pow(1.0 - ndv, 5.0);\n"
    " color += (1.0 - ambientF) * (1.0 - metalness) * albedo * ambient * hemi * ao;\n"
    " if (environment.x > 0.5) {\n"
    "     float3 reflected = reflect(-v, n);\n"
    "     float3 env = environment_tex.SampleLevel(environment_sampler, reflected, roughness * environment.y).rgb;\n"
    "     float4 brdf = roughness * float4(-1.0, -0.0275, -0.572, 0.022) + float4(1.0, 0.0425, 1.04, -0.04);\n"
    "     float a004 = min(brdf.x * brdf.x, exp2(-9.28 * ndv)) * brdf.x + brdf.y;\n"
    "     float2 ab = float2(-1.04, 1.04) * a004 + brdf.zw;\n"
    "     float specAO = saturate(pow(ndv + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);\n"
    "     float3 envScale = environment.z > 0.5 ? float3(1.0, 1.0, 1.0) : ambient;\n"
    "     color += env * max(f0 * ab.x + ab.y, 0.0) * envScale * specAO; }\n"
    " color += emission;\n"
    " color = max(color, 0.0); color = color / (color + 1.0);\n"
    " color = pow(color, 1.0 / 2.2);\n"
    " return float4(controls.w > 0.5 ? color * alpha : color, alpha); }\n";

typedef struct wp_transform_constants_dx11
{
    wp_mat4f mvp;
    wp_mat4f world;
    wp_mat4f normal_matrix;
} wp_transform_constants_dx11;

/* =========================================================================
 * Internal structure
 * ====================================================================== */

#define WP_DX11_GPU_QUERY_COUNT 8
#define WP_DX11_FRAME_SAMPLES 8192
typedef struct wp_gpu_frame_query
{
    ID3D11Query *disjoint, *start, *end;
    wp_s32 pending;
    unsigned epoch;
} wp_gpu_frame_query;

struct wp_renderer_dx11
{
    wp_render_statistics_dx11 statistics;
    double frame_intervals[WP_DX11_FRAME_SAMPLES];
    double interval_total, cpu_total, present_total, gpu_total;
    LARGE_INTEGER frame_start, last_present, clock_frequency;
    wp_s32 measuring, frame_measuring;
    volatile LONG reset_statistics;
    unsigned statistics_epoch;
    wp_gpu_frame_query gpu_queries[WP_DX11_GPU_QUERY_COUNT];
    wp_s32 active_gpu_query;
    wp_s32 gpu_queries_initialized;
    wp_s32 pntc_bindings_valid;
    ID3D11Buffer *bound_vertex_buffer, *bound_index_buffer;
    ID3D11ShaderResourceView *bound_textures[8];
    ID3D11SamplerState *bound_sampler;
    /* D3D11 core objects */
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    IDXGISwapChain *swap_chain;

    /* Render targets */
    ID3D11RenderTargetView *rt_view;
    ID3D11RenderTargetView *active_rt_view;
    ID3D11Texture2D *ds_texture;
    ID3D11DepthStencilView *ds_view;
    ID3D11DepthStencilView *active_ds_view;

    /* Shaders -- position + colour */
    ID3D11VertexShader *vs_pc;
    ID3D11PixelShader *ps_pc;
    ID3D11InputLayout *layout_pc;

    /* Shaders -- position + texcoord + colour */
    ID3D11VertexShader *vs_ptc;
    ID3D11PixelShader *ps_ptc;
    ID3D11InputLayout *layout_ptc;
    ID3D11ShaderResourceView *ptc_texture_view;
    ID3D11ShaderResourceView *ptc_external_texture_view;
    ID3D11ShaderResourceView *ptc_white_texture_view;
    ID3D11SamplerState *ptc_sampler_state;

    /* Shaders -- lit position + normal + texcoord + colour */
    ID3D11VertexShader *vs_pntc;
    ID3D11PixelShader *ps_pntc;
    ID3D11InputLayout *layout_pntc;
    ID3D11SamplerState *pntc_sampler_state;
    ID3D11SamplerState *environment_sampler_state;
    ID3D11ShaderResourceView *environment_view; /* borrowed */
    wp_f32 environment_max_lod;
    ID3D11SamplerState *material_sampler;
    ID3D11SamplerState *material_sampler_cache[1600];
    ID3D11ShaderResourceView *material_texture_views[6];
    ID3D11Buffer *cb_material;
    wp_material_dx11 material;

    /* Dynamic buffers */
    ID3D11Buffer *vb;
    wp_s32 vb_size;
    ID3D11Buffer *ib;
    wp_s32 ib_size;

    /* Constant buffer (transform) */
    ID3D11Buffer *cb_transform;

    /* Cached state objects */
    ID3D11RasterizerState *rs_state;
    ID3D11BlendState *bs_state;
    ID3D11DepthStencilState *dss_state;

    /* Dimensions */
    wp_s32 width;
    wp_s32 height;
    HWND hwnd;
    UINT swap_chain_flags;
    wp_s32 flip_model;

    /* Clear values */
    wp_f32 clear_r;
    wp_f32 clear_g;
    wp_f32 clear_b;
    wp_f32 clear_a;
    wp_f32 clear_depth;

    /* Viewport / scissor */
    wp_viewport_i viewport;
    wp_s32 scissor_enabled;
    wp_viewport_i scissor;

    /* Render state cache */
    wp_blend_mode blend_mode;
    wp_fill_mode fill_mode;
    wp_cull_mode cull_mode;
    wp_s32 depth_test_enabled;
    wp_s32 depth_write_enabled;
    wp_depth_func depth_func;

    /* Transform */
    wp_mat4f world_matrix;
    wp_mat4f view_matrix;
    wp_mat4f proj_matrix;
    wp_mat4f mvp_matrix;
    wp_s32 mvp_dirty;
    wp_s32 material_dirty;

    /* Dirty flags for pipeline state objects */
    wp_s32 rs_dirty;
    wp_s32 bs_dirty;
    wp_s32 dss_dirty;

    /* Own each immutable state once; rs/bs/dss_state alias the active entry. */
    ID3D11RasterizerState *rs_cache[12];
    ID3D11BlendState *bs_cache[5];
    ID3D11DepthStencilState *dss_cache[32];

    void *native;

    /* UI rendering support */
    struct wp_context ctx;
    struct wp_font_atlas atlas;
    struct wp_buffer cmds;

    struct wp_draw_null_texture tex_null;

    ID3D11ShaderResourceView *font_texture_view;
    ID3D11InputLayout *input_layout;
    ID3D11Buffer *vertex_buffer;
    ID3D11Buffer *index_buffer;
    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader *pixel_shader;
    ID3D11Buffer *const_buffer;
    ID3D11BlendState *blend_state;
    ID3D11DepthStencilState *ui_depth_stencil_state;
    ID3D11RasterizerState *rasterizer_state;
    ID3D11SamplerState *sampler_state;
    D3D11_VIEWPORT viewport_d3d;
    wp_s32 max_vertex_buffer;
    wp_s32 max_index_buffer;
    wp_s32 ui_initialized;
    wp_s32 atlas_initialized;
    wp_s32 cmds_initialized;
};

struct wp_render_texture_dx11
{
    ID3D11Texture2D *texture;
    ID3D11RenderTargetView *rt_view;
    ID3D11ShaderResourceView *shader_view;
    ID3D11Texture2D *ds_texture;
    ID3D11DepthStencilView *ds_view;
    wp_s32 width;
    wp_s32 height;
};

struct wp_geometry_dx11
{
    ID3D11Buffer *vertex_buffer;
    ID3D11Buffer *index_buffer;
    wp_s32 vertex_count;
    wp_s32 index_count;
    DXGI_FORMAT index_format;
};

/* =========================================================================
 * Internal helpers -- matrices
 * ====================================================================== */

static void wp_renderer_dx11_mat4f_identity( wp_mat4f *m )
{
    memset( m, 0, sizeof( wp_mat4f ) );
    m->m[0][0] = m->m[1][1] = m->m[2][2] = m->m[3][3] = 1.0f;
}

static void wp_renderer_dx11_mat4f_mul( wp_mat4f *result, const wp_mat4f *a, const wp_mat4f *b )
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

static void wp_renderer_dx11_normal_matrix( wp_mat4f *result, const wp_mat4f *world )
{
    const wp_f32 a = world->m[0][0], b = world->m[0][1], c = world->m[0][2];
    const wp_f32 d = world->m[1][0], e = world->m[1][1], f = world->m[1][2];
    const wp_f32 g = world->m[2][0], h = world->m[2][1], i = world->m[2][2];
    const wp_f32 determinant = a * ( e * i - f * h ) - b * ( d * i - f * g ) +
                               c * ( d * h - e * g );
    wp_renderer_dx11_mat4f_identity( result );
    if( determinant > -1.0e-8f && determinant < 1.0e-8f )
        return;
    {
        const wp_f32 inverse = 1.0f / determinant;
        result->m[0][0] = ( e * i - f * h ) * inverse;
        result->m[0][1] = ( f * g - d * i ) * inverse;
        result->m[0][2] = ( d * h - e * g ) * inverse;
        result->m[1][0] = ( c * h - b * i ) * inverse;
        result->m[1][1] = ( a * i - c * g ) * inverse;
        result->m[1][2] = ( b * g - a * h ) * inverse;
        result->m[2][0] = ( b * f - c * e ) * inverse;
        result->m[2][1] = ( c * d - a * f ) * inverse;
        result->m[2][2] = ( a * e - b * d ) * inverse;
    }
}

static void wp_renderer_dx11_update_mvp( wp_renderer_dx11 *r )
{
    wp_mat4f vw;

    if( !r->mvp_dirty )
        return;

    wp_renderer_dx11_mat4f_mul( &vw, &r->view_matrix, &r->world_matrix );
    wp_renderer_dx11_mat4f_mul( &r->mvp_matrix, &r->proj_matrix, &vw );
    r->mvp_dirty = 0;
}

/* =========================================================================
 * Internal helpers -- shader compilation
 * ====================================================================== */

static HRESULT wp_renderer_dx11_compile_shader( const wp_c8 *source, const wp_c8 *entry,
                                                const wp_c8 *target, ID3DBlob **blob )
{
    ID3DBlob *errors = NULL;
    HRESULT hr;

    hr = D3DCompile( source, strlen( source ), NULL, NULL, NULL, entry, target, 0, 0, blob, &errors );

    if( errors )
        ID3D10Blob_Release( errors );

    return hr;
}

static wp_s32 wp_renderer_dx11_create_shaders( wp_renderer_dx11 *r )
{
    ID3DBlob *blob = NULL;
    HRESULT hr;

    /* ---- position + colour vertex shader ------------------------------ */
    hr = wp_renderer_dx11_compile_shader( s_vs_pc_src, "vs_main", "vs_4_0", &blob );
    if( FAILED( hr ) )
        return 0;

    hr = ID3D11Device_CreateVertexShader( r->device, ID3D10Blob_GetBufferPointer( blob ),
                                          ID3D10Blob_GetBufferSize( blob ), NULL, &r->vs_pc );

    if( SUCCEEDED( hr ) )
    {
        D3D11_INPUT_ELEMENT_DESC layout_desc[2];

        memset( layout_desc, 0, sizeof( layout_desc ) );
        layout_desc[0].SemanticName = "POSITION";
        layout_desc[0].SemanticIndex = 0;
        layout_desc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        layout_desc[0].InputSlot = 0;
        layout_desc[0].AlignedByteOffset = 0;
        layout_desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layout_desc[0].InstanceDataStepRate = 0;

        layout_desc[1].SemanticName = "COLOR";
        layout_desc[1].SemanticIndex = 0;
        layout_desc[1].Format = DXGI_FORMAT_R32_UINT;
        layout_desc[1].InputSlot = 0;
        layout_desc[1].AlignedByteOffset = 12;
        layout_desc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layout_desc[1].InstanceDataStepRate = 0;

        hr = ID3D11Device_CreateInputLayout( r->device, layout_desc, 2,
                                             ID3D10Blob_GetBufferPointer( blob ),
                                             ID3D10Blob_GetBufferSize( blob ), &r->layout_pc );
    }

    ID3D10Blob_Release( blob );
    blob = NULL;

    if( FAILED( hr ) )
        return 0;

    /* ---- shared pixel shader (pc) ------------------------------------- */
    hr = wp_renderer_dx11_compile_shader( s_ps_src, "ps_main", "ps_4_0", &blob );
    if( FAILED( hr ) )
        return 0;

    hr = ID3D11Device_CreatePixelShader( r->device, ID3D10Blob_GetBufferPointer( blob ),
                                         ID3D10Blob_GetBufferSize( blob ), NULL, &r->ps_pc );

    ID3D10Blob_Release( blob );
    blob = NULL;

    if( FAILED( hr ) )
        return 0;

    /* ---- position + texcoord + colour vertex shader ------------------- */
    hr = wp_renderer_dx11_compile_shader( s_vs_ptc_src, "vs_main", "vs_4_0", &blob );
    if( FAILED( hr ) )
        return 0;

    hr = ID3D11Device_CreateVertexShader( r->device, ID3D10Blob_GetBufferPointer( blob ),
                                          ID3D10Blob_GetBufferSize( blob ), NULL, &r->vs_ptc );

    if( SUCCEEDED( hr ) )
    {
        D3D11_INPUT_ELEMENT_DESC layout_desc[3];

        memset( layout_desc, 0, sizeof( layout_desc ) );
        layout_desc[0].SemanticName = "POSITION";
        layout_desc[0].SemanticIndex = 0;
        layout_desc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        layout_desc[0].InputSlot = 0;
        layout_desc[0].AlignedByteOffset = 0;
        layout_desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layout_desc[0].InstanceDataStepRate = 0;

        layout_desc[1].SemanticName = "TEXCOORD";
        layout_desc[1].SemanticIndex = 0;
        layout_desc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
        layout_desc[1].InputSlot = 0;
        layout_desc[1].AlignedByteOffset = 12;
        layout_desc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layout_desc[1].InstanceDataStepRate = 0;

        layout_desc[2].SemanticName = "COLOR";
        layout_desc[2].SemanticIndex = 0;
        layout_desc[2].Format = DXGI_FORMAT_R32_UINT;
        layout_desc[2].InputSlot = 0;
        layout_desc[2].AlignedByteOffset = 20;
        layout_desc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layout_desc[2].InstanceDataStepRate = 0;

        hr = ID3D11Device_CreateInputLayout( r->device, layout_desc, 3,
                                             ID3D10Blob_GetBufferPointer( blob ),
                                             ID3D10Blob_GetBufferSize( blob ), &r->layout_ptc );
    }

    ID3D10Blob_Release( blob );
    blob = NULL;

    if( FAILED( hr ) )
        return 0;

    /* ---- ptc pixel shader --------------------------------------------- */
    hr = wp_renderer_dx11_compile_shader( s_ps_ptc_src, "ps_main", "ps_4_0", &blob );
    if( FAILED( hr ) )
        return 0;

    hr = ID3D11Device_CreatePixelShader( r->device, ID3D10Blob_GetBufferPointer( blob ),
                                         ID3D10Blob_GetBufferSize( blob ), NULL, &r->ps_ptc );

    ID3D10Blob_Release( blob );
    blob = NULL;

    if( FAILED( hr ) )
        return 0;

    /* ---- lit pntc vertex shader -------------------------------------- */
    hr = wp_renderer_dx11_compile_shader( s_vs_pntc_src, "vs_main", "vs_4_0", &blob );
    if( FAILED( hr ) )
        return 0;

    hr = ID3D11Device_CreateVertexShader( r->device, ID3D10Blob_GetBufferPointer( blob ),
                                          ID3D10Blob_GetBufferSize( blob ), NULL, &r->vs_pntc );
    if( SUCCEEDED( hr ) )
    {
        D3D11_INPUT_ELEMENT_DESC layout_desc[4];
        memset( layout_desc, 0, sizeof( layout_desc ) );
        layout_desc[0].SemanticName = "POSITION";
        layout_desc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        layout_desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layout_desc[1].SemanticName = "NORMAL";
        layout_desc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        layout_desc[1].AlignedByteOffset = 12;
        layout_desc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layout_desc[2].SemanticName = "TEXCOORD";
        layout_desc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
        layout_desc[2].AlignedByteOffset = 24;
        layout_desc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layout_desc[3].SemanticName = "COLOR";
        layout_desc[3].Format = DXGI_FORMAT_R32_UINT;
        layout_desc[3].AlignedByteOffset = 32;
        layout_desc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        hr = ID3D11Device_CreateInputLayout( r->device, layout_desc, 4,
                                             ID3D10Blob_GetBufferPointer( blob ),
                                             ID3D10Blob_GetBufferSize( blob ), &r->layout_pntc );
    }
    ID3D10Blob_Release( blob );
    blob = NULL;
    if( FAILED( hr ) )
        return 0;

    hr = wp_renderer_dx11_compile_shader( s_ps_pntc_src, "ps_main", "ps_4_0", &blob );
    if( FAILED( hr ) )
        return 0;
    hr = ID3D11Device_CreatePixelShader( r->device, ID3D10Blob_GetBufferPointer( blob ),
                                         ID3D10Blob_GetBufferSize( blob ), NULL, &r->ps_pntc );
    ID3D10Blob_Release( blob );

    return SUCCEEDED( hr ) ? 1 : 0;
}

/* =========================================================================
 * Internal helpers -- render target / depth-stencil creation
 * ====================================================================== */

static wp_s32 wp_renderer_dx11_create_render_target( wp_renderer_dx11 *r )
{
    ID3D11Texture2D *back_buffer = NULL;
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
    HRESULT hr;

    hr = IDXGISwapChain_GetBuffer( r->swap_chain, 0, &IID_ID3D11Texture2D, (void **)&back_buffer );
    if( FAILED( hr ) )
        return 0;

    memset( &rtv_desc, 0, sizeof( rtv_desc ) );
    rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    hr = ID3D11Device_CreateRenderTargetView( r->device, (ID3D11Resource *)back_buffer, &rtv_desc,
                                              &r->rt_view );

    ID3D11Texture2D_Release( back_buffer );
    if( SUCCEEDED( hr ) )
        r->active_rt_view = r->rt_view;
    return SUCCEEDED( hr ) ? 1 : 0;
}

static wp_s32 wp_renderer_dx11_create_depth_stencil( wp_renderer_dx11 *r )
{
    D3D11_TEXTURE2D_DESC tex_desc;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    HRESULT hr;

    memset( &tex_desc, 0, sizeof( tex_desc ) );
    tex_desc.Width = (UINT)r->width;
    tex_desc.Height = (UINT)r->height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = ID3D11Device_CreateTexture2D( r->device, &tex_desc, NULL, &r->ds_texture );
    if( FAILED( hr ) )
        return 0;

    memset( &dsv_desc, 0, sizeof( dsv_desc ) );
    dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Texture2D.MipSlice = 0;

    hr = ID3D11Device_CreateDepthStencilView( r->device, (ID3D11Resource *)r->ds_texture, &dsv_desc,
                                               &r->ds_view );
    if( FAILED( hr ) )
    {
        ID3D11Texture2D_Release( r->ds_texture );
        r->ds_texture = NULL;
        return 0;
    }

    r->active_ds_view = r->ds_view;

    return 1;
}

static void wp_renderer_dx11_release_targets( wp_renderer_dx11 *r )
{
    r->active_rt_view = NULL;
    r->active_ds_view = NULL;

    if( r->rt_view )
    {
        ID3D11RenderTargetView_Release( r->rt_view );
        r->rt_view = NULL;
    }
    if( r->ds_view )
    {
        ID3D11DepthStencilView_Release( r->ds_view );
        r->ds_view = NULL;
    }
    if( r->ds_texture )
    {
        ID3D11Texture2D_Release( r->ds_texture );
        r->ds_texture = NULL;
    }
}

static wp_s32 wp_renderer_dx11_create_targets( wp_renderer_dx11 *r )
{
    if( !wp_renderer_dx11_create_render_target( r ) )
        return 0;
    if( !wp_renderer_dx11_create_depth_stencil( r ) )
    {
        wp_renderer_dx11_release_targets( r );
        return 0;
    }
    return 1;
}

static wp_s32 wp_renderer_dx11_dimensions_supported( wp_renderer_dx11 *r, wp_s32 width,
                                                      wp_s32 height )
{
    D3D_FEATURE_LEVEL feature_level;
    wp_s32 max_dimension;

    if( !r || !r->device || width <= 0 || height <= 0 )
        return 0;

    feature_level = ID3D11Device_GetFeatureLevel( r->device );
    if( feature_level >= D3D_FEATURE_LEVEL_11_0 )
        max_dimension = 16384;
    else if( feature_level >= D3D_FEATURE_LEVEL_10_0 )
        max_dimension = 8192;
    else if( feature_level >= D3D_FEATURE_LEVEL_9_3 )
        max_dimension = 4096;
    else
        max_dimension = 2048;

    return width <= max_dimension && height <= max_dimension;
}

static ID3D11ShaderResourceView *wp_renderer_dx11_create_texture_view(
    wp_renderer_dx11 *r, const void *pixels, wp_s32 width, wp_s32 height, wp_pixel_format format )
{
    const wp_u8 *source = (const wp_u8 *)pixels;
    wp_u8 *converted = NULL;
    D3D11_TEXTURE2D_DESC desc;
    D3D11_SUBRESOURCE_DATA data;
    ID3D11ShaderResourceView *view = NULL;
    ID3D11Texture2D *texture = NULL;
    DXGI_FORMAT dxgi_format;
    UINT pitch;
    HRESULT hr;

    if( !r || !r->device || !source || width <= 0 || height <= 0 ||
        width > (wp_s32)( UINT_MAX / 4u ) )
        return NULL;

    switch( format )
    {
    case WORKPHONE_PIXEL_FORMAT_RGBA8:
        dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        pitch = (UINT)width * 4u;
        break;
    case WORKPHONE_PIXEL_FORMAT_BGRA8:
        dxgi_format = DXGI_FORMAT_B8G8R8A8_UNORM;
        pitch = (UINT)width * 4u;
        break;
    case WORKPHONE_PIXEL_FORMAT_RGB8:
    case WORKPHONE_PIXEL_FORMAT_BGR8:
    {
        size_t pixel_count;
        size_t i;

        if( (size_t)width > SIZE_MAX / (size_t)height )
            return NULL;
        pixel_count = (size_t)width * (size_t)height;
        if( pixel_count > SIZE_MAX / 4u )
            return NULL;

        converted = (wp_u8 *)malloc( pixel_count * 4u );
        if( !converted )
            return NULL;

        for( i = 0; i < pixel_count; ++i )
        {
            if( format == WORKPHONE_PIXEL_FORMAT_RGB8 )
            {
                converted[i * 4u + 0u] = source[i * 3u + 0u];
                converted[i * 4u + 1u] = source[i * 3u + 1u];
                converted[i * 4u + 2u] = source[i * 3u + 2u];
            }
            else
            {
                converted[i * 4u + 0u] = source[i * 3u + 2u];
                converted[i * 4u + 1u] = source[i * 3u + 1u];
                converted[i * 4u + 2u] = source[i * 3u + 0u];
            }
            converted[i * 4u + 3u] = 255u;
        }

        source = converted;
        dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        pitch = (UINT)width * 4u;
        break;
    }
    default:
        return NULL;
    }

    memset( &desc, 0, sizeof( desc ) );
    desc.Width = (UINT)width;
    desc.Height = (UINT)height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = dxgi_format;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    memset( &data, 0, sizeof( data ) );
    data.pSysMem = source;
    data.SysMemPitch = pitch;

    hr = ID3D11Device_CreateTexture2D( r->device, &desc, &data, &texture );
    free( converted );
    if( FAILED( hr ) )
        return NULL;

    hr = ID3D11Device_CreateShaderResourceView( r->device, (ID3D11Resource *)texture, NULL, &view );
    ID3D11Texture2D_Release( texture );
    return SUCCEEDED( hr ) ? view : NULL;
}

static wp_s32 wp_renderer_dx11_create_ptc_resources( wp_renderer_dx11 *r )
{
    const wp_u8 white[4] = { 255u, 255u, 255u, 255u };
    D3D11_SAMPLER_DESC desc;
    HRESULT hr;

    r->ptc_white_texture_view = wp_renderer_dx11_create_texture_view(
        r, white, 1, 1, WORKPHONE_PIXEL_FORMAT_RGBA8 );
    if( !r->ptc_white_texture_view )
        return 0;

    memset( &desc, 0, sizeof( desc ) );
    // Bilinear sampling matches the Ogre UI path and avoids distorted glyph
    // weight and jagged widget edges at fractional pixel positions.
    desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    desc.MaxLOD = FLT_MAX;

    hr = ID3D11Device_CreateSamplerState( r->device, &desc, &r->ptc_sampler_state );
    if( FAILED( hr ) )
        return 0;

    memset( &desc, 0, sizeof( desc ) );
    desc.Filter = D3D11_FILTER_ANISOTROPIC;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MaxAnisotropy = 8;
    desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    desc.MaxLOD = FLT_MAX;
    hr = ID3D11Device_CreateSamplerState( r->device, &desc, &r->pntc_sampler_state );
    if( FAILED( hr ) ) return 0;
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    hr = ID3D11Device_CreateSamplerState( r->device, &desc, &r->environment_sampler_state );
    return SUCCEEDED( hr ) ? 1 : 0;
}

/* =========================================================================
 * Internal helpers -- dynamic buffer management
 * ====================================================================== */

static wp_s32 wp_renderer_dx11_ensure_vb( wp_renderer_dx11 *r, wp_s32 size_bytes )
{
    D3D11_BUFFER_DESC desc;
    HRESULT hr;

    if( r->vb && r->vb_size >= size_bytes )
        return 1;

    if( r->vb )
    {
        ID3D11Buffer_Release( r->vb );
        r->vb = NULL;
    }

    while( r->vb_size < size_bytes )
        r->vb_size *= 2;

    memset( &desc, 0, sizeof( desc ) );
    desc.ByteWidth = (UINT)r->vb_size;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    hr = ID3D11Device_CreateBuffer( r->device, &desc, NULL, &r->vb );
    return SUCCEEDED( hr ) ? 1 : 0;
}

static wp_s32 wp_renderer_dx11_ensure_ib( wp_renderer_dx11 *r, wp_s32 size_bytes )
{
    D3D11_BUFFER_DESC desc;
    HRESULT hr;

    if( r->ib && r->ib_size >= size_bytes )
        return 1;

    if( r->ib )
    {
        ID3D11Buffer_Release( r->ib );
        r->ib = NULL;
    }

    while( r->ib_size < size_bytes )
        r->ib_size *= 2;

    memset( &desc, 0, sizeof( desc ) );
    desc.ByteWidth = (UINT)r->ib_size;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    hr = ID3D11Device_CreateBuffer( r->device, &desc, NULL, &r->ib );
    return SUCCEEDED( hr ) ? 1 : 0;
}

/* =========================================================================
 * Internal helpers -- pipeline state objects
 * ====================================================================== */

static D3D11_COMPARISON_FUNC wp_renderer_dx11_map_depth_func( wp_depth_func func )
{
    switch( func )
    {
    case WORKPHONE_DEPTH_FUNC_NEVER:
        return D3D11_COMPARISON_NEVER;
    case WORKPHONE_DEPTH_FUNC_LESS:
        return D3D11_COMPARISON_LESS;
    case WORKPHONE_DEPTH_FUNC_EQUAL:
        return D3D11_COMPARISON_EQUAL;
    case WORKPHONE_DEPTH_FUNC_LEQUAL:
        return D3D11_COMPARISON_LESS_EQUAL;
    case WORKPHONE_DEPTH_FUNC_GREATER:
        return D3D11_COMPARISON_GREATER;
    case WORKPHONE_DEPTH_FUNC_NOTEQUAL:
        return D3D11_COMPARISON_NOT_EQUAL;
    case WORKPHONE_DEPTH_FUNC_GEQUAL:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case WORKPHONE_DEPTH_FUNC_ALWAYS:
        return D3D11_COMPARISON_ALWAYS;
    default:
        return D3D11_COMPARISON_LESS;
    }
}

static void wp_renderer_dx11_update_rasterizer_state( wp_renderer_dx11 *r )
{
    D3D11_RASTERIZER_DESC desc;
    HRESULT hr;
    UINT index;

    if( !r->rs_dirty )
        return;

    memset( &desc, 0, sizeof( desc ) );

    switch( r->fill_mode )
    {
    case WORKPHONE_FILL_MODE_WIREFRAME:
        desc.FillMode = D3D11_FILL_WIREFRAME;
        break;
    default:
        desc.FillMode = D3D11_FILL_SOLID;
        break;
    }

    switch( r->cull_mode )
    {
    case WORKPHONE_CULL_MODE_NONE:
        desc.CullMode = D3D11_CULL_NONE;
        break;
    case WORKPHONE_CULL_MODE_FRONT:
        desc.CullMode = D3D11_CULL_FRONT;
        break;
    default:
        desc.CullMode = D3D11_CULL_BACK;
        break;
    }

    desc.FrontCounterClockwise = FALSE;
    desc.DepthClipEnable = TRUE;
    desc.ScissorEnable = r->scissor_enabled ? TRUE : FALSE;

    index = ( desc.FillMode == D3D11_FILL_WIREFRAME ? 6u : 0u ) +
            ( (UINT)desc.CullMode - 1u ) * 2u + ( desc.ScissorEnable ? 1u : 0u );
    if( !r->rs_cache[index] )
    {
        hr = ID3D11Device_CreateRasterizerState( r->device, &desc, &r->rs_cache[index] );
        if( FAILED( hr ) )
            return;
    }
    if( r->rs_state != r->rs_cache[index] )
    {
        r->rs_state = r->rs_cache[index];
        ID3D11DeviceContext_RSSetState( r->context, r->rs_state );
        ++r->statistics.state_bindings;
    }

    r->rs_dirty = 0;
}

static void wp_renderer_dx11_update_blend_state( wp_renderer_dx11 *r )
{
    D3D11_BLEND_DESC desc;
    HRESULT hr;
    float factor[4] = { 0, 0, 0, 0 };
    UINT index;

    if( !r->bs_dirty )
        return;

    memset( &desc, 0, sizeof( desc ) );
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    switch( r->blend_mode )
    {
    case WORKPHONE_BLEND_MODE_PREMULTIPLIED:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        break;
    case WORKPHONE_BLEND_MODE_ALPHA:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        break;

    case WORKPHONE_BLEND_MODE_ADDITIVE:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        break;

    case WORKPHONE_BLEND_MODE_MULTIPLY:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        break;

    case WORKPHONE_BLEND_MODE_NONE: /* fall through */
    default:
        desc.RenderTarget[0].BlendEnable = FALSE;
        break;
    }

    index = (UINT)r->blend_mode <= (UINT)WORKPHONE_BLEND_MODE_PREMULTIPLIED ? (UINT)r->blend_mode : 0u;
    if( !r->bs_cache[index] )
    {
        hr = ID3D11Device_CreateBlendState( r->device, &desc, &r->bs_cache[index] );
        if( FAILED( hr ) )
            return;
    }
    if( r->bs_state != r->bs_cache[index] )
    {
        r->bs_state = r->bs_cache[index];
        ID3D11DeviceContext_OMSetBlendState( r->context, r->bs_state, factor, 0xFFFFFFFF );
        ++r->statistics.state_bindings;
    }

    r->bs_dirty = 0;
}

static void wp_renderer_dx11_update_depth_stencil_state( wp_renderer_dx11 *r )
{
    D3D11_DEPTH_STENCIL_DESC desc;
    HRESULT hr;
    UINT index;

    if( !r->dss_dirty )
        return;

    memset( &desc, 0, sizeof( desc ) );
    desc.DepthEnable = r->depth_test_enabled ? TRUE : FALSE;
    desc.DepthWriteMask =
        r->depth_write_enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = wp_renderer_dx11_map_depth_func( r->depth_func );
    desc.StencilEnable = FALSE;

    index = ( desc.DepthEnable ? 16u : 0u ) +
            ( desc.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL ? 8u : 0u ) +
            (UINT)desc.DepthFunc - 1u;
    if( !r->dss_cache[index] )
    {
        hr = ID3D11Device_CreateDepthStencilState( r->device, &desc, &r->dss_cache[index] );
        if( FAILED( hr ) )
            return;
    }
    if( r->dss_state != r->dss_cache[index] )
    {
        r->dss_state = r->dss_cache[index];
        ID3D11DeviceContext_OMSetDepthStencilState( r->context, r->dss_state, 0 );
        ++r->statistics.state_bindings;
    }

    r->dss_dirty = 0;
}

static void wp_renderer_dx11_apply_state( wp_renderer_dx11 *r )
{
    wp_renderer_dx11_update_rasterizer_state( r );
    wp_renderer_dx11_update_blend_state( r );
    wp_renderer_dx11_update_depth_stencil_state( r );
}

/* =========================================================================
 * Internal helpers -- viewport & scissor application
 * ====================================================================== */

static void wp_renderer_dx11_apply_viewport( wp_renderer_dx11 *r )
{
    D3D11_VIEWPORT vp;

    vp.TopLeftX = (FLOAT)r->viewport.x;
    vp.TopLeftY = (FLOAT)r->viewport.y;
    vp.Width = (FLOAT)r->viewport.width;
    vp.Height = (FLOAT)r->viewport.height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    ID3D11DeviceContext_RSSetViewports( r->context, 1, &vp );

    if( r->scissor_enabled )
    {
        D3D11_RECT sr;
        sr.left = (LONG)r->scissor.x;
        sr.top = (LONG)r->scissor.y;
        sr.right = (LONG)( r->scissor.x + r->scissor.width );
        sr.bottom = (LONG)( r->scissor.y + r->scissor.height );

        ID3D11DeviceContext_RSSetScissorRects( r->context, 1, &sr );
    }
}

/* =========================================================================
 * Internal helpers -- upload constant buffer (MVP)
 * ====================================================================== */

static wp_s32 wp_renderer_dx11_upload_constants( wp_renderer_dx11 *r, ID3D11Buffer *buffer,
                                                 const void *data, size_t size )
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    if( FAILED( ID3D11DeviceContext_Map( r->context, (ID3D11Resource *)buffer, 0,
                                        D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) )
        return 0;
    memcpy( mapped.pData, data, size );
    ID3D11DeviceContext_Unmap( r->context, (ID3D11Resource *)buffer, 0 );
    return 1;
}

static wp_s32 wp_renderer_dx11_upload_mvp( wp_renderer_dx11 *r )
{
    wp_transform_constants_dx11 constants;

    if( !r || !r->cb_transform )
        return 0;

    /*
     * Re-uploading an unchanged transform creates needless synchronization work,
     * particularly with the WARP driver. Matrix setters mark the cache dirty, so
     * unchanged submeshes/faces only need the existing buffer rebound.
     */
    if( !r->mvp_dirty )
    {
        ID3D11DeviceContext_VSSetConstantBuffers( r->context, 0, 1, &r->cb_transform );
        return 1;
    }

    wp_renderer_dx11_update_mvp( r );
    constants.mvp = r->mvp_matrix;
    constants.world = r->world_matrix;
    wp_renderer_dx11_normal_matrix( &constants.normal_matrix, &r->world_matrix );
    /* Discard lets queued draws retain their constants while the next draw writes
     * fresh storage. Updating a shared DEFAULT constant buffer stalls WARP. */
    if( !wp_renderer_dx11_upload_constants( r, r->cb_transform, &constants, sizeof( constants ) ) )
    {
        r->mvp_dirty = 1;
        return 0;
    }

    ++r->statistics.transform_uploads;
    ID3D11DeviceContext_VSSetConstantBuffers( r->context, 0, 1, &r->cb_transform );
    return 1;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_renderer_dx11 *wp_renderer_dx11_create( void *hwnd, wp_s32 width, wp_s32 height )
{
    wp_renderer_dx11 *r;
    DXGI_SWAP_CHAIN_DESC sc_desc;
    D3D11_BUFFER_DESC buf_desc;
    D3D_FEATURE_LEVEL feature_level;
    HRESULT hr;
    wp_s32 attempt;
    IDXGIFactory5 *factory = NULL;
    BOOL tearing_supported = FALSE;
    wchar_t legacy_override[2];
    wp_s32 prefer_flip;

    if( !hwnd || width <= 0 || height <= 0 )
        return NULL;

    r = (wp_renderer_dx11 *)malloc( sizeof( wp_renderer_dx11 ) );
    if( !r )
        return NULL;

    memset( r, 0, sizeof( wp_renderer_dx11 ) );
    r->active_gpu_query = -1;
    QueryPerformanceFrequency( &r->clock_frequency );
    r->hwnd = (HWND)hwnd;
    r->width = width;
    r->height = height;

    /* ---- swap-chain descriptor ---------------------------------------- */
    memset( &sc_desc, 0, sizeof( sc_desc ) );
    sc_desc.BufferDesc.Width = (UINT)width;
    sc_desc.BufferDesc.Height = (UINT)height;
    sc_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sc_desc.BufferDesc.RefreshRate.Numerator = 60;
    sc_desc.BufferDesc.RefreshRate.Denominator = 1;
    sc_desc.SampleDesc.Count = 1;
    sc_desc.SampleDesc.Quality = 0;
    sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sc_desc.BufferCount = 1;
    sc_desc.OutputWindow = r->hwnd;
    sc_desc.Windowed = TRUE;
    sc_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    /* Flip presentation is opt-in until frame pacing has been validated for
     * the hosting application. Failed/unsupported requests retain legacy. */
    if( SUCCEEDED( CreateDXGIFactory1( &IID_IDXGIFactory5, (void **)&factory ) ) )
    {
        IDXGIFactory5_CheckFeatureSupport( factory, DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                           &tearing_supported, sizeof( tearing_supported ) );
        IDXGIFactory5_Release( factory );
    }
    prefer_flip = tearing_supported && GetEnvironmentVariableW(
        L"WORKPHONE_DX11_FLIP_SWAP_CHAIN", legacy_override, 2 ) && !GetEnvironmentVariableW(
        L"WORKPHONE_DX11_LEGACY_SWAP_CHAIN", legacy_override, 2 );

    /* ---- device + swap-chain creation --------------------------------- */
    /* Retry with WARP when hardware is unavailable. Release partial objects
     * from a failed creation attempt before trying another driver. */
    for( attempt = 0; attempt < 4; ++attempt )
    {
        r->flip_model = prefer_flip && attempt % 2 == 0;
        sc_desc.BufferCount = r->flip_model ? 2 : 1;
        sc_desc.SwapEffect = r->flip_model ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
        sc_desc.Flags = r->flip_model ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        if( !prefer_flip && attempt % 2 != 0 ) continue;
        hr = D3D11CreateDeviceAndSwapChain( NULL,
                                            attempt < 2 ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_WARP,
                                            NULL, 0, NULL, 0,
                                            D3D11_SDK_VERSION, &sc_desc, &r->swap_chain, &r->device,
                                            &feature_level, &r->context );
        if( SUCCEEDED( hr ) )
            break;
        if( r->swap_chain )
            IDXGISwapChain_Release( r->swap_chain );
        if( r->context )
            ID3D11DeviceContext_Release( r->context );
        if( r->device )
            ID3D11Device_Release( r->device );
        r->swap_chain = NULL;
        r->context = NULL;
        r->device = NULL;
    }

    if( FAILED( hr ) )
    {
        free( r );
        return NULL;
    }

    r->swap_chain_flags = sc_desc.Flags;
    if( r->flip_model )
    {
        IDXGIDevice1 *dxgi_device = NULL;
        if( SUCCEEDED( ID3D11Device_QueryInterface( r->device, &IID_IDXGIDevice1, (void **)&dxgi_device ) ) )
        {
            IDXGIDevice1_SetMaximumFrameLatency( dxgi_device, 1 );
            IDXGIDevice1_Release( dxgi_device );
        }
    }

    /* ---- render target ------------------------------------------------ */
    if( !wp_renderer_dx11_create_render_target( r ) )
    {
        wp_renderer_dx11_destroy( r );
        return NULL;
    }

    /* ---- depth stencil------------------------------------------------- */
    if( !wp_renderer_dx11_create_depth_stencil( r ) )
    {
        wp_renderer_dx11_destroy( r );
        return NULL;
    }

    /* ---- shaders + input layouts -------------------------------------- */
    if( !wp_renderer_dx11_create_shaders( r ) )
    {
        wp_renderer_dx11_destroy( r );
        return NULL;
    }

    if( !wp_renderer_dx11_create_ptc_resources( r ) )
    {
        wp_renderer_dx11_destroy( r );
        return NULL;
    }

    /* ---- constant buffers (transform and material) ------------------- */
    memset( &buf_desc, 0, sizeof( buf_desc ) );
    buf_desc.ByteWidth = sizeof( wp_transform_constants_dx11 );
    buf_desc.Usage = D3D11_USAGE_DYNAMIC;
    buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buf_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = ID3D11Device_CreateBuffer( r->device, &buf_desc, NULL, &r->cb_transform );
    if( FAILED( hr ) )
    {
        wp_renderer_dx11_destroy( r );
        return NULL;
    }

    buf_desc.ByteWidth = sizeof( wp_material_dx11 );
    hr = ID3D11Device_CreateBuffer( r->device, &buf_desc, NULL, &r->cb_material );
    if( FAILED( hr ) )
    {
        wp_renderer_dx11_destroy( r );
        return NULL;
    }

    /* ---- initial dynamic buffers -------------------------------------- */
    r->vb_size = DX11_INITIAL_VB_SIZE;
    r->ib_size = DX11_INITIAL_IB_SIZE;

    if( !wp_renderer_dx11_ensure_vb( r, r->vb_size ) || !wp_renderer_dx11_ensure_ib( r, r->ib_size ) )
    {
        wp_renderer_dx11_destroy( r );
        return NULL;
    }

    /* ---- default state ------------------------------------------------ */
    r->clear_r = 0.0f;
    r->clear_g = 0.0f;
    r->clear_b = 0.0f;
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

    wp_renderer_dx11_mat4f_identity( &r->world_matrix );
    wp_renderer_dx11_mat4f_identity( &r->view_matrix );
    wp_renderer_dx11_mat4f_identity( &r->proj_matrix );
    wp_renderer_dx11_mat4f_identity( &r->mvp_matrix );
    r->material.base_color.x = 1.0f;
    r->material.base_color.y = 1.0f;
    r->material.base_color.z = 1.0f;
    r->material.base_color.w = 1.0f;
    r->material.specular_color = r->material.base_color;
    r->material.specular_color.x = 0.04f;
    r->material.specular_color.y = 0.04f;
    r->material.specular_color.z = 0.04f;
    r->material.light_color = r->material.base_color;
    r->material.light_direction.y = -1.0f;
    r->material.light_color.w = 3.0f;
    r->material.camera_position.w = 0.12f;
    r->material.surface.y = 0.5f;
    r->material.surface.z = 1.0f;
    r->material.surface.w = 1.0f;
    r->material.uv_transform.z = 1.0f;
    r->mvp_dirty = 1;
    r->material_dirty = 1;

    r->rs_dirty = 1;
    r->bs_dirty = 1;
    r->dss_dirty = 1;

    return r;
}

void wp_renderer_dx11_destroy( wp_renderer_dx11 *r )
{
    wp_s32 i;
    if( !r )
        return;

    if( r->context )
        ID3D11DeviceContext_ClearState( r->context );
    for( i = 0; i < WP_DX11_GPU_QUERY_COUNT; ++i )
    {
        wp_gpu_frame_query *q = &r->gpu_queries[i];
        if( q->disjoint ) ID3D11Query_Release( q->disjoint );
        if( q->start ) ID3D11Query_Release( q->start );
        if( q->end ) ID3D11Query_Release( q->end );
    }

    if( r->atlas_initialized )
        wp_font_atlas_clear( &r->atlas );
    if( r->cmds_initialized )
        wp_buffer_free( &r->cmds );
    if( r->ui_initialized )
        wp_free( &r->ctx );

    if( r->sampler_state )
        ID3D11SamplerState_Release( r->sampler_state );
    if( r->font_texture_view )
        ID3D11ShaderResourceView_Release( r->font_texture_view );
    if( r->vertex_buffer )
        ID3D11Buffer_Release( r->vertex_buffer );
    if( r->index_buffer )
        ID3D11Buffer_Release( r->index_buffer );
    if( r->blend_state )
        ID3D11BlendState_Release( r->blend_state );
    if( r->ui_depth_stencil_state )
        ID3D11DepthStencilState_Release( r->ui_depth_stencil_state );
    if( r->pixel_shader )
        ID3D11PixelShader_Release( r->pixel_shader );
    if( r->const_buffer )
        ID3D11Buffer_Release( r->const_buffer );
    if( r->vertex_shader )
        ID3D11VertexShader_Release( r->vertex_shader );
    if( r->input_layout )
        ID3D11InputLayout_Release( r->input_layout );
    if( r->rasterizer_state )
        ID3D11RasterizerState_Release( r->rasterizer_state );

    for( i = 0; i < 12; ++i )
        if( r->rs_cache[i] )
            ID3D11RasterizerState_Release( r->rs_cache[i] );
    for( i = 0; i < 5; ++i )
        if( r->bs_cache[i] )
            ID3D11BlendState_Release( r->bs_cache[i] );
    for( i = 0; i < 1600; ++i )
        if( r->material_sampler_cache[i] )
            ID3D11SamplerState_Release( r->material_sampler_cache[i] );
    for( i = 0; i < 32; ++i )
        if( r->dss_cache[i] )
            ID3D11DepthStencilState_Release( r->dss_cache[i] );

    if( r->cb_transform )
        ID3D11Buffer_Release( r->cb_transform );
    if( r->cb_material )
        ID3D11Buffer_Release( r->cb_material );
    if( r->vb )
        ID3D11Buffer_Release( r->vb );
    if( r->ib )
        ID3D11Buffer_Release( r->ib );

    if( r->layout_pc )
        ID3D11InputLayout_Release( r->layout_pc );
    if( r->vs_pc )
        ID3D11VertexShader_Release( r->vs_pc );
    if( r->ps_pc )
        ID3D11PixelShader_Release( r->ps_pc );

    if( r->layout_ptc )
        ID3D11InputLayout_Release( r->layout_ptc );
    if( r->vs_ptc )
        ID3D11VertexShader_Release( r->vs_ptc );
    if( r->ps_ptc )
        ID3D11PixelShader_Release( r->ps_ptc );
    if( r->ptc_sampler_state )
        ID3D11SamplerState_Release( r->ptc_sampler_state );
    if( r->ptc_texture_view )
        ID3D11ShaderResourceView_Release( r->ptc_texture_view );
    if( r->ptc_white_texture_view )
        ID3D11ShaderResourceView_Release( r->ptc_white_texture_view );
    if( r->layout_pntc )
        ID3D11InputLayout_Release( r->layout_pntc );
    if( r->vs_pntc )
        ID3D11VertexShader_Release( r->vs_pntc );
    if( r->ps_pntc )
        ID3D11PixelShader_Release( r->ps_pntc );
    if( r->pntc_sampler_state )
        ID3D11SamplerState_Release( r->pntc_sampler_state );
    if( r->environment_sampler_state )
        ID3D11SamplerState_Release( r->environment_sampler_state );

    if( r->ds_view )
        ID3D11DepthStencilView_Release( r->ds_view );
    if( r->ds_texture )
        ID3D11Texture2D_Release( r->ds_texture );
    if( r->rt_view )
        ID3D11RenderTargetView_Release( r->rt_view );

    if( r->swap_chain )
        IDXGISwapChain_Release( r->swap_chain );
    if( r->context )
        ID3D11DeviceContext_Release( r->context );
    if( r->device )
        ID3D11Device_Release( r->device );

    free( r );
}

/* =========================================================================
 * Resize
 * ====================================================================== */

wp_s32 wp_renderer_dx11_resize( wp_renderer_dx11 *r, wp_s32 width, wp_s32 height )
{
    wp_s32 old_width;
    wp_s32 old_height;
    HRESULT hr;

    if( !wp_renderer_dx11_dimensions_supported( r, width, height ) )
        return 0;

    old_width = r->width;
    old_height = r->height;
    ID3D11DeviceContext_OMSetRenderTargets( r->context, 0, NULL, NULL );
    wp_renderer_dx11_release_targets( r );

    hr = IDXGISwapChain_ResizeBuffers( r->swap_chain, 0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN,
                                       r->swap_chain_flags );
    if( FAILED( hr ) )
    {
        /* ResizeBuffers leaves the old swap-chain buffers intact on failure. */
        (void)wp_renderer_dx11_create_targets( r );
        return 0;
    }

    r->width = width;
    r->height = height;

    if( !wp_renderer_dx11_create_targets( r ) )
    {
        /* Roll the swap chain back so a failed resize does not strand the renderer. */
        wp_renderer_dx11_release_targets( r );
        hr = IDXGISwapChain_ResizeBuffers( r->swap_chain, 0, (UINT)old_width, (UINT)old_height,
                                           DXGI_FORMAT_UNKNOWN, r->swap_chain_flags );
        if( SUCCEEDED( hr ) )
        {
            r->width = old_width;
            r->height = old_height;
            (void)wp_renderer_dx11_create_targets( r );
        }
        return 0;
    }

    r->viewport.x = 0;
    r->viewport.y = 0;
    r->viewport.width = width;
    r->viewport.height = height;

    r->viewport_d3d.Width = (FLOAT)width;
    r->viewport_d3d.Height = (FLOAT)height;
    if( r->const_buffer )
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if( SUCCEEDED( ID3D11DeviceContext_Map( r->context, (ID3D11Resource *)r->const_buffer, 0,
                                                D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) )
        {
            wp_renderer_dx11_get_projection_matrix( width, height, (wp_f32 *)mapped.pData );
            ID3D11DeviceContext_Unmap( r->context, (ID3D11Resource *)r->const_buffer, 0 );
        }
    }

    return 1;
}

/* =========================================================================
 * Frame lifecycle
 * ====================================================================== */

void wp_renderer_dx11_invalidate_state( wp_renderer_dx11 *r )
{
    if( !r ) return;
    r->pntc_bindings_valid = 0;
    r->rs_state = NULL;
    r->bs_state = NULL;
    r->dss_state = NULL;
    r->rs_dirty = r->bs_dirty = r->dss_dirty = 1;
}

void wp_renderer_dx11_reset_statistics( wp_renderer_dx11 *r )
{
    if( r ) InterlockedExchange( &r->reset_statistics, 1 );
}

static int compare_frame_interval( const void *a, const void *b )
{
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : x > y;
}

void wp_renderer_dx11_get_statistics( const wp_renderer_dx11 *r,
                                      wp_render_statistics_dx11 *out )
{
    double *sorted;
    size_t count;
    if( !out ) return;
    memset( out, 0, sizeof( *out ) );
    if( !r ) return;
    *out = r->statistics;
    out->flip_model = r->flip_model;
    if( out->frames )
    {
        out->cpu_frame_ms = r->cpu_total / out->frames;
        out->present_ms = r->present_total / out->frames;
    }
    if( out->gpu_samples ) out->gpu_frame_ms = r->gpu_total / out->gpu_samples;
    if( !out->interval_samples ) return;
    out->interval_ms = r->interval_total / out->interval_samples;
    count = out->interval_samples < WP_DX11_FRAME_SAMPLES ?
        (size_t)out->interval_samples : WP_DX11_FRAME_SAMPLES;
    sorted = (double *)malloc( count * sizeof( double ) );
    if( sorted )
    {
        memcpy( sorted, r->frame_intervals, count * sizeof( double ) );
        qsort( sorted, count, sizeof( double ), compare_frame_interval );
        out->interval_p95_ms = sorted[( count * 95 + 99 ) / 100 - 1];
        free( sorted );
    }
}

static void begin_frame_statistics( wp_renderer_dx11 *r )
{
    wp_s32 i;
    D3D11_QUERY_DESC desc;
    if( r->frame_measuring ) return; /* Multiple offscreen/window passes form one frame. */
    if( InterlockedExchange( &r->reset_statistics, 0 ) )
    {
        memset( &r->statistics, 0, sizeof( r->statistics ) );
        r->interval_total = r->cpu_total = r->present_total = r->gpu_total = 0;
        r->last_present.QuadPart = 0;
        ++r->statistics_epoch;
        r->measuring = 1;
    }
    if( !r->measuring ) return;
    r->frame_measuring = 1;
    QueryPerformanceCounter( &r->frame_start );
    if( !r->gpu_queries_initialized )
    {
        r->gpu_queries_initialized = 1;
        desc.MiscFlags = 0;
        for( i = 0; i < WP_DX11_GPU_QUERY_COUNT; ++i )
        {
            wp_gpu_frame_query *q = &r->gpu_queries[i];
            desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
            if( FAILED( ID3D11Device_CreateQuery( r->device, &desc, &q->disjoint ) ) ) continue;
            desc.Query = D3D11_QUERY_TIMESTAMP;
            ID3D11Device_CreateQuery( r->device, &desc, &q->start );
            ID3D11Device_CreateQuery( r->device, &desc, &q->end );
        }
    }
    r->active_gpu_query = -1;
    for( i = 0; i < WP_DX11_GPU_QUERY_COUNT; ++i )
    {
        wp_gpu_frame_query *q = &r->gpu_queries[i];
        if( q->pending )
        {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT timing;
            UINT64 start, end;
            if( ID3D11DeviceContext_GetData( r->context, (ID3D11Asynchronous *)q->disjoint,
                    &timing, sizeof( timing ), D3D11_ASYNC_GETDATA_DONOTFLUSH ) != S_OK ||
                ID3D11DeviceContext_GetData( r->context, (ID3D11Asynchronous *)q->start,
                    &start, sizeof( start ), D3D11_ASYNC_GETDATA_DONOTFLUSH ) != S_OK ||
                ID3D11DeviceContext_GetData( r->context, (ID3D11Asynchronous *)q->end,
                    &end, sizeof( end ), D3D11_ASYNC_GETDATA_DONOTFLUSH ) != S_OK ) continue;
            q->pending = 0;
            if( q->epoch == r->statistics_epoch && !timing.Disjoint && timing.Frequency && end >= start )
            {
                r->gpu_total += 1000.0 * (double)( end - start ) / timing.Frequency;
                ++r->statistics.gpu_samples;
            }
        }
        if( r->active_gpu_query < 0 && q->disjoint && q->start && q->end )
            r->active_gpu_query = i;
    }
    if( r->active_gpu_query >= 0 )
    {
        wp_gpu_frame_query *q = &r->gpu_queries[r->active_gpu_query];
        q->epoch = r->statistics_epoch;
        ID3D11DeviceContext_Begin( r->context, (ID3D11Asynchronous *)q->disjoint );
        ID3D11DeviceContext_End( r->context, (ID3D11Asynchronous *)q->start );
    }
}

void wp_renderer_dx11_begin_frame( wp_renderer_dx11 *r )
{
    if( !r )
        return;

    begin_frame_statistics( r );
    wp_renderer_dx11_invalidate_state( r );
    ID3D11DeviceContext_OMSetRenderTargets( r->context, 1, &r->active_rt_view,
                                            r->active_ds_view );

    wp_renderer_dx11_apply_viewport( r );
    wp_renderer_dx11_apply_state( r );
}

void wp_renderer_dx11_end_frame( wp_renderer_dx11 *r )
{
    (void)r;
}

/* =========================================================================
 * Present
 * ====================================================================== */

void wp_renderer_dx11_present( wp_renderer_dx11 *r, wp_s32 vsync )
{
    LARGE_INTEGER before, after;
    if( !r || !r->swap_chain )
        return;
    if( r->frame_measuring && r->active_gpu_query >= 0 )
    {
        wp_gpu_frame_query *q = &r->gpu_queries[r->active_gpu_query];
        ID3D11DeviceContext_End( r->context, (ID3D11Asynchronous *)q->end );
        ID3D11DeviceContext_End( r->context, (ID3D11Asynchronous *)q->disjoint );
        q->pending = 1;
    }
    QueryPerformanceCounter( &before );
    IDXGISwapChain_Present( r->swap_chain, vsync ? 1 : 0,
                           !vsync && r->flip_model ? DXGI_PRESENT_ALLOW_TEARING : 0 );
    QueryPerformanceCounter( &after );
    if( r->frame_measuring )
    {
        double scale = 1000.0 / r->clock_frequency.QuadPart;
        if( r->last_present.QuadPart )
        {
            double interval = ( after.QuadPart - r->last_present.QuadPart ) * scale;
            r->frame_intervals[r->statistics.interval_samples % WP_DX11_FRAME_SAMPLES] = interval;
            ++r->statistics.interval_samples;
            r->interval_total += interval;
        }
        r->last_present = after;
        ++r->statistics.frames;
        r->cpu_total += ( after.QuadPart - r->frame_start.QuadPart ) * scale;
        r->present_total += ( after.QuadPart - before.QuadPart ) * scale;
        r->frame_measuring = 0;
        r->active_gpu_query = -1;
    }
}

/* =========================================================================
 * Clear
 * ====================================================================== */

void wp_renderer_dx11_set_clear_color( wp_renderer_dx11 *r, wp_f32 cr, wp_f32 cg, wp_f32 cb, wp_f32 ca )
{
    if( !r )
        return;
    r->clear_r = cr;
    r->clear_g = cg;
    r->clear_b = cb;
    r->clear_a = ca;
}

void wp_renderer_dx11_set_clear_depth( wp_renderer_dx11 *r, wp_f32 depth )
{
    if( !r )
        return;
    r->clear_depth = depth;
}

void wp_renderer_dx11_clear( wp_renderer_dx11 *r, uint32_t flags )
{
    if( !r )
        return;

    if( ( flags & WORKPHONE_CLEAR_FLAG_COLOR ) && r->active_rt_view )
    {
        float color[4];
        color[0] = r->clear_r;
        color[1] = r->clear_g;
        color[2] = r->clear_b;
        color[3] = r->clear_a;
        ID3D11DeviceContext_ClearRenderTargetView( r->context, r->active_rt_view, color );
    }

    if( ( flags & WORKPHONE_CLEAR_FLAG_DEPTH ) && r->active_ds_view )
    {
        ID3D11DeviceContext_ClearDepthStencilView( r->context, r->active_ds_view, D3D11_CLEAR_DEPTH,
                                                   r->clear_depth, 0 );
    }
}

/* =========================================================================
 * Viewport and scissor
 * ====================================================================== */

void wp_renderer_dx11_set_viewport( wp_renderer_dx11 *r, wp_viewport_i viewport )
{
    if( !r )
        return;
    r->viewport = viewport;
    wp_renderer_dx11_apply_viewport( r );
}

wp_viewport_i wp_renderer_dx11_get_viewport( const wp_renderer_dx11 *r )
{
    wp_viewport_i zero;
    if( r )
        return r->viewport;
    memset( &zero, 0, sizeof( zero ) );
    return zero;
}

void wp_renderer_dx11_set_scissor_enabled( wp_renderer_dx11 *r, wp_s32 enabled )
{
    if( !r )
        return;
    if( r->scissor_enabled != enabled )
    {
        r->scissor_enabled = enabled;
        r->rs_dirty = 1;
    }
}

void wp_renderer_dx11_set_scissor_rect( wp_renderer_dx11 *r, wp_viewport_i scissor )
{
    if( !r )
        return;
    r->scissor = scissor;
    wp_renderer_dx11_apply_viewport( r );
}

/* =========================================================================
 * Render state
 * ====================================================================== */

void wp_renderer_dx11_set_blend_mode( wp_renderer_dx11 *r, wp_blend_mode mode )
{
    if( !r )
        return;
    if( r->blend_mode != mode )
    {
        r->blend_mode = mode;
        r->bs_dirty = 1;
    }
}

wp_blend_mode wp_renderer_dx11_get_blend_mode( const wp_renderer_dx11 *r )
{
    return r ? r->blend_mode : WORKPHONE_BLEND_MODE_NONE;
}

void wp_renderer_dx11_set_fill_mode( wp_renderer_dx11 *r, wp_fill_mode mode )
{
    if( !r )
        return;
    if( r->fill_mode != mode )
    {
        r->fill_mode = mode;
        r->rs_dirty = 1;
    }
}

wp_fill_mode wp_renderer_dx11_get_fill_mode( const wp_renderer_dx11 *r )
{
    return r ? r->fill_mode : WORKPHONE_FILL_MODE_SOLID;
}

void wp_renderer_dx11_set_cull_mode( wp_renderer_dx11 *r, wp_cull_mode mode )
{
    if( !r )
        return;
    if( r->cull_mode != mode )
    {
        r->cull_mode = mode;
        r->rs_dirty = 1;
    }
}

wp_cull_mode wp_renderer_dx11_get_cull_mode( const wp_renderer_dx11 *r )
{
    return r ? r->cull_mode : WORKPHONE_CULL_MODE_BACK;
}

void wp_renderer_dx11_set_depth_test_enabled( wp_renderer_dx11 *r, wp_s32 enabled )
{
    if( !r )
        return;
    if( r->depth_test_enabled != enabled )
    {
        r->depth_test_enabled = enabled;
        r->dss_dirty = 1;
    }
}

wp_s32 wp_renderer_dx11_get_depth_test_enabled( const wp_renderer_dx11 *r )
{
    return r ? r->depth_test_enabled : 0;
}

void wp_renderer_dx11_set_depth_write_enabled( wp_renderer_dx11 *r, wp_s32 enabled )
{
    if( !r )
        return;
    if( r->depth_write_enabled != enabled )
    {
        r->depth_write_enabled = enabled;
        r->dss_dirty = 1;
    }
}

wp_s32 wp_renderer_dx11_get_depth_write_enabled( const wp_renderer_dx11 *r )
{
    return r ? r->depth_write_enabled : 0;
}

void wp_renderer_dx11_set_depth_func( wp_renderer_dx11 *r, wp_depth_func func )
{
    if( !r )
        return;
    if( r->depth_func != func )
    {
        r->depth_func = func;
        r->dss_dirty = 1;
    }
}

wp_depth_func wp_renderer_dx11_get_depth_func( const wp_renderer_dx11 *r )
{
    return r ? r->depth_func : WORKPHONE_DEPTH_FUNC_LESS;
}

/* =========================================================================
 * Transform matrices
 * ====================================================================== */

void wp_renderer_dx11_set_world_matrix( wp_renderer_dx11 *r, const wp_mat4f *mat )
{
    if( !r || !mat || memcmp( &r->world_matrix, mat, sizeof( *mat ) ) == 0 )
        return;
    r->world_matrix = *mat;
    r->mvp_dirty = 1;
}

void wp_renderer_dx11_set_view_matrix( wp_renderer_dx11 *r, const wp_mat4f *mat )
{
    if( !r || !mat || memcmp( &r->view_matrix, mat, sizeof( *mat ) ) == 0 )
        return;
    r->view_matrix = *mat;
    r->mvp_dirty = 1;
}

void wp_renderer_dx11_set_projection_matrix( wp_renderer_dx11 *r, const wp_mat4f *mat )
{
    if( !r || !mat || memcmp( &r->proj_matrix, mat, sizeof( *mat ) ) == 0 )
        return;
    r->proj_matrix = *mat;
    r->mvp_dirty = 1;
}

void wp_renderer_dx11_set_material( wp_renderer_dx11 *r,
                                    const wp_material_dx11 *material )
{
    wp_material_dx11 resolved;
    if( !r || !material ) return;
    resolved = *material;
    resolved.environment.x = r->environment_view ? 1.0f : 0.0f;
    resolved.environment.y = r->environment_max_lod;
    if( memcmp( &r->material, &resolved, sizeof( resolved ) ) == 0 ) return;
    r->material = resolved;
    r->material_dirty = 1;
}

void wp_renderer_dx11_set_environment( wp_renderer_dx11 *r, void *view, wp_f32 max_lod )
{
    if( !r || ( r->environment_view == (ID3D11ShaderResourceView *)view &&
                 r->environment_max_lod == max_lod ) ) return;
    r->environment_view = (ID3D11ShaderResourceView *)view;
    r->environment_max_lod = max_lod;
    r->material.environment.x = view ? 1.0f : 0.0f;
    r->material.environment.y = max_lod;
    r->material_dirty = 1;
}

void wp_renderer_dx11_set_material_textures( wp_renderer_dx11 *r, void *const *views )
{
    wp_s32 i;
    if( !r )
        return;
    for( i = 0; i < 6; ++i )
        r->material_texture_views[i] = views ? (ID3D11ShaderResourceView *)views[i] : NULL;
    if( !views )
    {
        ID3D11DeviceContext_PSSetShaderResources( r->context, 1, 6, r->material_texture_views );
        r->pntc_bindings_valid = 0;
    }
}

void wp_renderer_dx11_set_material_sampler( wp_renderer_dx11 *r, wp_u32 wrap_u,
                                            wp_u32 wrap_v, wp_u32 filter, wp_u32 anisotropy )
{
    static const D3D11_TEXTURE_ADDRESS_MODE addresses[5] = {
        D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_MIRROR,
        D3D11_TEXTURE_ADDRESS_MIRROR_ONCE, D3D11_TEXTURE_ADDRESS_BORDER };
    static const D3D11_FILTER filters[4] = { D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_FILTER_ANISOTROPIC };
    D3D11_SAMPLER_DESC desc;
    wp_u32 index;
    if( !r )
        return;
    if( wrap_u > 4 ) wrap_u = 0;
    if( wrap_v > 4 ) wrap_v = 0;
    if( filter > 3 ) filter = 1;
    if( anisotropy < 1 ) anisotropy = 1;
    if( anisotropy > 16 ) anisotropy = 16;
    if( filter != 3 ) anisotropy = 1;
    index = ( ( wrap_u * 5 + wrap_v ) * 4 + filter ) * 16 + anisotropy - 1;
    if( !r->material_sampler_cache[index] )
    {
        memset( &desc, 0, sizeof( desc ) );
        desc.Filter = filters[filter];
        desc.AddressU = addresses[wrap_u];
        desc.AddressV = addresses[wrap_v];
        desc.AddressW = desc.AddressV;
        desc.MaxAnisotropy = anisotropy;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MaxLOD = D3D11_FLOAT32_MAX;
        if( FAILED( ID3D11Device_CreateSamplerState( r->device, &desc,
                                                     &r->material_sampler_cache[index] ) ) )
        {
            r->material_sampler = NULL;
            return;
        }
    }
    r->material_sampler = r->material_sampler_cache[index];
}

/* =========================================================================
 * Internal helpers -- generic draw
 * ====================================================================== */

static void dx11_draw_vertices( wp_renderer_dx11 *r, const void *vertex_data, wp_s32 vertex_count,
                                wp_s32 stride, D3D11_PRIMITIVE_TOPOLOGY topology, wp_s32 is_ptc )
{
    D3D11_BOX update_box;
    UINT offset = 0;
    UINT s = (UINT)stride;
    wp_s32 size_bytes;

    if( !r || !vertex_data || vertex_count <= 0 )
        return;

    size_bytes = vertex_count * stride;

    if( !wp_renderer_dx11_ensure_vb( r, size_bytes ) )
        return;

    r->pntc_bindings_valid = 0;
    wp_renderer_dx11_apply_state( r );
    if( !wp_renderer_dx11_upload_mvp( r ) )
        return;

    update_box.left = 0;
    update_box.top = 0;
    update_box.front = 0;
    update_box.right = (UINT)size_bytes;
    update_box.bottom = 1;
    update_box.back = 1;
    ID3D11DeviceContext_UpdateSubresource( r->context, (ID3D11Resource *)r->vb, 0, &update_box,
                                           vertex_data, 0, 0 );

    if( is_ptc )
    {
        ID3D11ShaderResourceView *texture_view =
            r->ptc_external_texture_view
                ? r->ptc_external_texture_view
                : ( r->ptc_texture_view ? r->ptc_texture_view : r->ptc_white_texture_view );
        ID3D11DeviceContext_IASetInputLayout( r->context, r->layout_ptc );
        ID3D11DeviceContext_VSSetShader( r->context, r->vs_ptc, NULL, 0 );
        ID3D11DeviceContext_PSSetShader( r->context, r->ps_ptc, NULL, 0 );
        ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &texture_view );
        ID3D11DeviceContext_PSSetSamplers( r->context, 0, 1, &r->ptc_sampler_state );
    }
    else
    {
        ID3D11DeviceContext_IASetInputLayout( r->context, r->layout_pc );
        ID3D11DeviceContext_VSSetShader( r->context, r->vs_pc, NULL, 0 );
        ID3D11DeviceContext_PSSetShader( r->context, r->ps_pc, NULL, 0 );
    }

    ID3D11DeviceContext_IASetVertexBuffers( r->context, 0, 1, &r->vb, &s, &offset );
    ID3D11DeviceContext_IASetPrimitiveTopology( r->context, topology );
    ++r->statistics.draws;
    if( topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST )
        r->statistics.triangles += (uint64_t)vertex_count / 3;
    ID3D11DeviceContext_Draw( r->context, (UINT)vertex_count, 0 );
}

static wp_s32 dx11_prepare_indexed( wp_renderer_dx11 *r, const void *vertex_data,
                                    wp_s32 vertex_count, wp_s32 stride, const void *indices,
                                    wp_s32 index_count, wp_s32 index_stride,
                                    DXGI_FORMAT index_format,
                                    D3D11_PRIMITIVE_TOPOLOGY topology, wp_s32 is_ptc )
{
    D3D11_BOX update_box;
    UINT offset = 0;
    UINT s = (UINT)stride;
    wp_s32 vb_bytes, ib_bytes;

    if( !r || !vertex_data || !indices || vertex_count <= 0 || index_count <= 0 )
        return 0;

    vb_bytes = vertex_count * stride;
    ib_bytes = index_count * index_stride;

    if( !wp_renderer_dx11_ensure_vb( r, vb_bytes ) || !wp_renderer_dx11_ensure_ib( r, ib_bytes ) )
        return 0;

    wp_renderer_dx11_apply_state( r );
    if( !wp_renderer_dx11_upload_mvp( r ) )
        return 0;

    /* DEFAULT buffers let the driver rename/copy resources instead of blocking the
     * render thread while WARP or a busy GPU consumes the preceding draw. */
    update_box.left = 0;
    update_box.top = 0;
    update_box.front = 0;
    update_box.right = (UINT)vb_bytes;
    update_box.bottom = 1;
    update_box.back = 1;
    ID3D11DeviceContext_UpdateSubresource( r->context, (ID3D11Resource *)r->vb, 0, &update_box,
                                           vertex_data, 0, 0 );

    update_box.right = (UINT)ib_bytes;
    ID3D11DeviceContext_UpdateSubresource( r->context, (ID3D11Resource *)r->ib, 0, &update_box,
                                           indices, 0, 0 );

    if( is_ptc )
    {
        ID3D11ShaderResourceView *texture_view =
            r->ptc_external_texture_view
                ? r->ptc_external_texture_view
                : ( r->ptc_texture_view ? r->ptc_texture_view : r->ptc_white_texture_view );
        ID3D11DeviceContext_IASetInputLayout( r->context, r->layout_ptc );
        ID3D11DeviceContext_VSSetShader( r->context, r->vs_ptc, NULL, 0 );
        ID3D11DeviceContext_PSSetShader( r->context, r->ps_ptc, NULL, 0 );
        ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &texture_view );
        ID3D11DeviceContext_PSSetSamplers( r->context, 0, 1, &r->ptc_sampler_state );
    }
    else
    {
        ID3D11DeviceContext_IASetInputLayout( r->context, r->layout_pc );
        ID3D11DeviceContext_VSSetShader( r->context, r->vs_pc, NULL, 0 );
        ID3D11DeviceContext_PSSetShader( r->context, r->ps_pc, NULL, 0 );
    }

    ID3D11DeviceContext_IASetVertexBuffers( r->context, 0, 1, &r->vb, &s, &offset );
    ID3D11DeviceContext_IASetIndexBuffer( r->context, r->ib, index_format, 0 );
    ID3D11DeviceContext_IASetPrimitiveTopology( r->context, topology );
    return 1;
}

static void dx11_draw_prepared_indexed( wp_renderer_dx11 *r, wp_s32 index_start,
                                         wp_s32 index_count, wp_s32 base_vertex,
                                         wp_s32 is_ptc )
{
    if( !r || index_start < 0 || index_count <= 0 )
        return;

    r->pntc_bindings_valid = 0;
    wp_renderer_dx11_apply_state( r );
    if( !wp_renderer_dx11_upload_mvp( r ) )
        return;

    if( is_ptc )
    {
        ID3D11ShaderResourceView *texture_view =
            r->ptc_external_texture_view
                ? r->ptc_external_texture_view
                : ( r->ptc_texture_view ? r->ptc_texture_view : r->ptc_white_texture_view );
        ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &texture_view );
    }

    ++r->statistics.draws;
    r->statistics.triangles += (uint64_t)index_count / 3;
    ID3D11DeviceContext_DrawIndexed( r->context, (UINT)index_count, (UINT)index_start,
                                     (INT)base_vertex );
}

static void dx11_draw_indexed( wp_renderer_dx11 *r, const void *vertex_data, wp_s32 vertex_count,
                               wp_s32 stride, const void *indices, wp_s32 index_count,
                               wp_s32 index_stride, DXGI_FORMAT index_format,
                               D3D11_PRIMITIVE_TOPOLOGY topology, wp_s32 is_ptc )
{
    if( dx11_prepare_indexed( r, vertex_data, vertex_count, stride, indices, index_count,
                              index_stride, index_format, topology, is_ptc ) )
    {
        dx11_draw_prepared_indexed( r, 0, index_count, 0, is_ptc );
    }
}

/* =========================================================================
 * Texture binding
 * ====================================================================== */

void wp_renderer_dx11_set_texture( wp_renderer_dx11 *r, const void *pixels, wp_s32 width,
                                   wp_s32 height, wp_pixel_format format )
{
    ID3D11ShaderResourceView *view;

    if( !r )
        return;

    r->ptc_external_texture_view = NULL;

    if( !pixels || width <= 0 || height <= 0 )
    {
        if( r->ptc_texture_view )
            ID3D11ShaderResourceView_Release( r->ptc_texture_view );
        r->ptc_texture_view = NULL;
        return;
    }

    view = wp_renderer_dx11_create_texture_view( r, pixels, width, height, format );
    if( !view )
        return;

    if( r->ptc_texture_view )
        ID3D11ShaderResourceView_Release( r->ptc_texture_view );
    r->ptc_texture_view = view;
}

void wp_renderer_dx11_set_texture_native( wp_renderer_dx11 *r, void *texture_view )
{
    if( !r )
        return;

    r->ptc_external_texture_view = (ID3D11ShaderResourceView *)texture_view;
}

void *wp_renderer_dx11_create_texture_native( wp_renderer_dx11 *r, const void *pixels,
                                               wp_s32 width, wp_s32 height,
                                               wp_pixel_format format )
{
    return wp_renderer_dx11_create_texture_view( r, pixels, width, height, format );
}

void wp_renderer_dx11_destroy_texture_native( void *texture_view )
{
    if( texture_view )
        ID3D11ShaderResourceView_Release( (ID3D11ShaderResourceView *)texture_view );
}

void wp_renderer_dx11_set_render_target_native( wp_renderer_dx11 *r, void *render_target_view )
{
    if( !r )
        return;

    r->pntc_bindings_valid = 0;
    r->active_rt_view =
        render_target_view ? (ID3D11RenderTargetView *)render_target_view : r->rt_view;
    r->active_ds_view = r->ds_view;
    ID3D11DeviceContext_OMSetRenderTargets( r->context, 1, &r->active_rt_view,
                                            r->active_ds_view );
}

wp_render_texture_dx11 *wp_renderer_dx11_create_render_texture( wp_renderer_dx11 *r, wp_s32 width,
                                                                 wp_s32 height )
{
    wp_render_texture_dx11 *target;
    D3D11_TEXTURE2D_DESC texture_desc;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    HRESULT hr;

    if( !wp_renderer_dx11_dimensions_supported( r, width, height ) )
        return NULL;

    target = (wp_render_texture_dx11 *)malloc( sizeof( wp_render_texture_dx11 ) );
    if( !target )
        return NULL;
    memset( target, 0, sizeof( wp_render_texture_dx11 ) );
    target->width = width;
    target->height = height;

    memset( &texture_desc, 0, sizeof( texture_desc ) );
    texture_desc.Width = (UINT)width;
    texture_desc.Height = (UINT)height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    hr = ID3D11Device_CreateTexture2D( r->device, &texture_desc, NULL, &target->texture );
    if( FAILED( hr ) )
        goto fail;
    hr = ID3D11Device_CreateRenderTargetView( r->device, (ID3D11Resource *)target->texture, NULL,
                                              &target->rt_view );
    if( FAILED( hr ) )
        goto fail;
    hr = ID3D11Device_CreateShaderResourceView( r->device, (ID3D11Resource *)target->texture, NULL,
                                                &target->shader_view );
    if( FAILED( hr ) )
        goto fail;

    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    hr = ID3D11Device_CreateTexture2D( r->device, &texture_desc, NULL, &target->ds_texture );
    if( FAILED( hr ) )
        goto fail;

    memset( &dsv_desc, 0, sizeof( dsv_desc ) );
    dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = ID3D11Device_CreateDepthStencilView( r->device, (ID3D11Resource *)target->ds_texture,
                                               &dsv_desc, &target->ds_view );
    if( FAILED( hr ) )
        goto fail;

    return target;

fail:
    wp_renderer_dx11_destroy_render_texture( target );
    return NULL;
}

void wp_renderer_dx11_destroy_render_texture( wp_render_texture_dx11 *target )
{
    if( !target )
        return;
    if( target->ds_view )
        ID3D11DepthStencilView_Release( target->ds_view );
    if( target->ds_texture )
        ID3D11Texture2D_Release( target->ds_texture );
    if( target->shader_view )
        ID3D11ShaderResourceView_Release( target->shader_view );
    if( target->rt_view )
        ID3D11RenderTargetView_Release( target->rt_view );
    if( target->texture )
        ID3D11Texture2D_Release( target->texture );
    free( target );
}

void wp_renderer_dx11_set_render_texture( wp_renderer_dx11 *r, wp_render_texture_dx11 *target )
{
    ID3D11ShaderResourceView *null_view = NULL;

    if( !r )
        return;

    /* A camera must never sample the same texture it is currently rendering into. */
    ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &null_view );
    r->ptc_external_texture_view = NULL;

    r->pntc_bindings_valid = 0;
    r->active_rt_view = target ? target->rt_view : r->rt_view;
    r->active_ds_view = target ? target->ds_view : r->ds_view;
    ID3D11DeviceContext_OMSetRenderTargets( r->context, 1, &r->active_rt_view,
                                            r->active_ds_view );
}

void *wp_renderer_dx11_get_render_texture_resource( const wp_render_texture_dx11 *target )
{
    return target ? target->texture : NULL;
}

void *wp_renderer_dx11_get_render_texture_view( const wp_render_texture_dx11 *target )
{
    return target ? target->shader_view : NULL;
}

void *wp_renderer_dx11_get_render_texture_target_view( const wp_render_texture_dx11 *target )
{
    return target ? target->rt_view : NULL;
}

/* =========================================================================
 * Draw calls
 * ====================================================================== */

void wp_renderer_dx11_draw_triangles_pc( wp_renderer_dx11 *r, const wp_vertex_pc *vertices,
                                         wp_s32 vertex_count )
{
    dx11_draw_vertices( r, vertices, vertex_count, (wp_s32)sizeof( wp_vertex_pc ),
                        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0 );
}

void wp_renderer_dx11_draw_indexed_triangles_pc( wp_renderer_dx11 *r, const wp_vertex_pc *vertices,
                                                 wp_s32 vertex_count, const uint16_t *indices,
                                                 wp_s32 index_count )
{
    dx11_draw_indexed( r, vertices, vertex_count, (wp_s32)sizeof( wp_vertex_pc ), indices, index_count,
                       (wp_s32)sizeof( uint16_t ), DXGI_FORMAT_R16_UINT,
                       D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0 );
}

void wp_renderer_dx11_draw_lines_pc( wp_renderer_dx11 *r, const wp_vertex_pc *vertices,
                                     wp_s32 vertex_count )
{
    dx11_draw_vertices( r, vertices, vertex_count, (wp_s32)sizeof( wp_vertex_pc ),
                        D3D11_PRIMITIVE_TOPOLOGY_LINELIST, 0 );
}

void wp_renderer_dx11_draw_points_pc( wp_renderer_dx11 *r, const wp_vertex_pc *vertices,
                                      wp_s32 vertex_count )
{
    dx11_draw_vertices( r, vertices, vertex_count, (wp_s32)sizeof( wp_vertex_pc ),
                        D3D11_PRIMITIVE_TOPOLOGY_POINTLIST, 0 );
}

void wp_renderer_dx11_draw_triangles_ptc( wp_renderer_dx11 *r, const wp_vertex_ptc *vertices,
                                          wp_s32 vertex_count )
{
    dx11_draw_vertices( r, vertices, vertex_count, (wp_s32)sizeof( wp_vertex_ptc ),
                        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 1 );
}

void wp_renderer_dx11_draw_indexed_triangles_ptc( wp_renderer_dx11 *r, const wp_vertex_ptc *vertices,
                                                  wp_s32 vertex_count, const uint16_t *indices,
                                                  wp_s32 index_count )
{
    dx11_draw_indexed( r, vertices, vertex_count, (wp_s32)sizeof( wp_vertex_ptc ), indices, index_count,
                       (wp_s32)sizeof( uint16_t ), DXGI_FORMAT_R16_UINT,
                       D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 1 );
}

void wp_renderer_dx11_draw_indexed_triangles_ptc_u32( wp_renderer_dx11 *r,
                                                       const wp_vertex_ptc *vertices,
                                                       wp_s32 vertex_count,
                                                       const uint32_t *indices,
                                                       wp_s32 index_count )
{
    dx11_draw_indexed( r, vertices, vertex_count, (wp_s32)sizeof( wp_vertex_ptc ), indices,
                       index_count, (wp_s32)sizeof( uint32_t ), DXGI_FORMAT_R32_UINT,
                       D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 1 );
}

wp_s32 wp_renderer_dx11_prepare_indexed_triangles_ptc( wp_renderer_dx11 *r,
                                                        const wp_vertex_ptc *vertices,
                                                        wp_s32 vertex_count,
                                                        const uint16_t *indices,
                                                        wp_s32 index_count )
{
    return dx11_prepare_indexed( r, vertices, vertex_count, (wp_s32)sizeof( wp_vertex_ptc ),
                                  indices, index_count, (wp_s32)sizeof( uint16_t ),
                                  DXGI_FORMAT_R16_UINT, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 1 );
}

wp_s32 wp_renderer_dx11_prepare_indexed_triangles_ptc_u32( wp_renderer_dx11 *r,
                                                            const wp_vertex_ptc *vertices,
                                                            wp_s32 vertex_count,
                                                            const uint32_t *indices,
                                                            wp_s32 index_count )
{
    return dx11_prepare_indexed( r, vertices, vertex_count, (wp_s32)sizeof( wp_vertex_ptc ),
                                  indices, index_count, (wp_s32)sizeof( uint32_t ),
                                  DXGI_FORMAT_R32_UINT, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 1 );
}

void wp_renderer_dx11_draw_prepared_indexed_triangles_ptc( wp_renderer_dx11 *r,
                                                            wp_s32 index_start,
                                                            wp_s32 index_count,
                                                            wp_s32 base_vertex )
{
    dx11_draw_prepared_indexed( r, index_start, index_count, base_vertex, 1 );
}

wp_geometry_dx11 *wp_renderer_dx11_create_indexed_geometry_ptc(
    wp_renderer_dx11 *r, const wp_vertex_ptc *vertices, wp_s32 vertex_count,
    const void *indices, wp_s32 index_count, wp_s32 indices_are_u32 )
{
    wp_geometry_dx11 *geometry;
    D3D11_BUFFER_DESC desc;
    D3D11_SUBRESOURCE_DATA initial_data;
    const wp_s32 index_stride = indices_are_u32 ? (wp_s32)sizeof( uint32_t )
                                                 : (wp_s32)sizeof( uint16_t );
    HRESULT hr;

    if( !r || !vertices || !indices || vertex_count <= 0 || index_count <= 0 ||
        vertex_count > INT_MAX / (wp_s32)sizeof( wp_vertex_ptc ) ||
        index_count > INT_MAX / index_stride )
        return NULL;

    geometry = (wp_geometry_dx11 *)malloc( sizeof( wp_geometry_dx11 ) );
    if( !geometry )
        return NULL;
    memset( geometry, 0, sizeof( wp_geometry_dx11 ) );

    memset( &desc, 0, sizeof( desc ) );
    desc.ByteWidth = (UINT)( vertex_count * (wp_s32)sizeof( wp_vertex_ptc ) );
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    memset( &initial_data, 0, sizeof( initial_data ) );
    initial_data.pSysMem = vertices;
    hr = ID3D11Device_CreateBuffer( r->device, &desc, &initial_data, &geometry->vertex_buffer );
    if( FAILED( hr ) )
    {
        free( geometry );
        return NULL;
    }

    desc.ByteWidth = (UINT)( index_count * index_stride );
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    initial_data.pSysMem = indices;
    hr = ID3D11Device_CreateBuffer( r->device, &desc, &initial_data, &geometry->index_buffer );
    if( FAILED( hr ) )
    {
        ID3D11Buffer_Release( geometry->vertex_buffer );
        free( geometry );
        return NULL;
    }

    geometry->vertex_count = vertex_count;
    geometry->index_count = index_count;
    geometry->index_format = indices_are_u32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    ++r->statistics.geometry_creations;
    return geometry;
}

wp_geometry_dx11 *wp_renderer_dx11_create_indexed_geometry_pntc(
    wp_renderer_dx11 *r, const wp_vertex_pntc *vertices, wp_s32 vertex_count,
    const void *indices, wp_s32 index_count, wp_s32 indices_are_u32 )
{
    wp_geometry_dx11 *geometry;
    D3D11_BUFFER_DESC desc;
    D3D11_SUBRESOURCE_DATA initial_data;
    const wp_s32 index_stride = indices_are_u32 ? (wp_s32)sizeof( uint32_t )
                                                 : (wp_s32)sizeof( uint16_t );
    HRESULT hr;

    if( !r || !vertices || !indices || vertex_count <= 0 || index_count <= 0 ||
        vertex_count > INT_MAX / (wp_s32)sizeof( wp_vertex_pntc ) ||
        index_count > INT_MAX / index_stride )
        return NULL;

    geometry = (wp_geometry_dx11 *)malloc( sizeof( wp_geometry_dx11 ) );
    if( !geometry )
        return NULL;
    memset( geometry, 0, sizeof( wp_geometry_dx11 ) );

    memset( &desc, 0, sizeof( desc ) );
    desc.ByteWidth = (UINT)( vertex_count * (wp_s32)sizeof( wp_vertex_pntc ) );
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    memset( &initial_data, 0, sizeof( initial_data ) );
    initial_data.pSysMem = vertices;
    hr = ID3D11Device_CreateBuffer( r->device, &desc, &initial_data, &geometry->vertex_buffer );
    if( FAILED( hr ) )
    {
        free( geometry );
        return NULL;
    }

    desc.ByteWidth = (UINT)( index_count * index_stride );
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    initial_data.pSysMem = indices;
    hr = ID3D11Device_CreateBuffer( r->device, &desc, &initial_data, &geometry->index_buffer );
    if( FAILED( hr ) )
    {
        ID3D11Buffer_Release( geometry->vertex_buffer );
        free( geometry );
        return NULL;
    }

    geometry->vertex_count = vertex_count;
    geometry->index_count = index_count;
    geometry->index_format = indices_are_u32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    ++r->statistics.geometry_creations;
    return geometry;
}

void wp_renderer_dx11_destroy_geometry( wp_geometry_dx11 *geometry )
{
    if( !geometry )
        return;
    if( geometry->index_buffer )
        ID3D11Buffer_Release( geometry->index_buffer );
    if( geometry->vertex_buffer )
        ID3D11Buffer_Release( geometry->vertex_buffer );
    free( geometry );
}

void wp_renderer_dx11_draw_geometry_ptc( wp_renderer_dx11 *r,
                                         const wp_geometry_dx11 *geometry,
                                         wp_s32 index_start, wp_s32 index_count,
                                         wp_s32 base_vertex )
{
    ID3D11ShaderResourceView *texture_view;
    UINT stride = (UINT)sizeof( wp_vertex_ptc );
    UINT offset = 0;

    if( !r || !geometry || index_start < 0 || index_count <= 0 ||
        index_start > geometry->index_count ||
        index_count > geometry->index_count - index_start )
        return;

    r->pntc_bindings_valid = 0;
    wp_renderer_dx11_apply_state( r );
    if( !wp_renderer_dx11_upload_mvp( r ) )
        return;
    texture_view =
        r->ptc_external_texture_view
            ? r->ptc_external_texture_view
            : ( r->ptc_texture_view ? r->ptc_texture_view : r->ptc_white_texture_view );

    ID3D11DeviceContext_IASetInputLayout( r->context, r->layout_ptc );
    ID3D11DeviceContext_VSSetShader( r->context, r->vs_ptc, NULL, 0 );
    ID3D11DeviceContext_PSSetShader( r->context, r->ps_ptc, NULL, 0 );
    ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &texture_view );
    ID3D11DeviceContext_PSSetSamplers( r->context, 0, 1, &r->ptc_sampler_state );
    ID3D11DeviceContext_IASetVertexBuffers( r->context, 0, 1, &geometry->vertex_buffer,
                                            &stride, &offset );
    ID3D11DeviceContext_IASetIndexBuffer( r->context, geometry->index_buffer,
                                          geometry->index_format, 0 );
    ID3D11DeviceContext_IASetPrimitiveTopology( r->context,
                                                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    ++r->statistics.draws;
    r->statistics.triangles += (uint64_t)index_count / 3;
    ID3D11DeviceContext_DrawIndexed( r->context, (UINT)index_count, (UINT)index_start,
                                     (INT)base_vertex );
}

void wp_renderer_dx11_draw_geometry_pntc( wp_renderer_dx11 *r,
                                          const wp_geometry_dx11 *geometry,
                                          wp_s32 index_start, wp_s32 index_count,
                                          wp_s32 base_vertex )
{
    ID3D11ShaderResourceView *texture_view;
    UINT stride = (UINT)sizeof( wp_vertex_pntc );
    UINT offset = 0;

    if( !r || !geometry || index_start < 0 || index_count <= 0 ||
        index_start > geometry->index_count ||
        index_count > geometry->index_count - index_start )
        return;

    wp_renderer_dx11_apply_state( r );
    if( !wp_renderer_dx11_upload_mvp( r ) )
        return;
    if( r->material_dirty )
    {
        if( !wp_renderer_dx11_upload_constants( r, r->cb_material, &r->material, sizeof( r->material ) ) )
            return;
        r->material_dirty = 0;
        ++r->statistics.material_uploads;
    }
    // ptc_texture_view is the ImGui atlas. It is a valid fallback for UI draws,
    // but an untextured mesh must sample white rather than arbitrary font glyphs.
    texture_view = r->ptc_external_texture_view ? r->ptc_external_texture_view
                                                 : r->ptc_white_texture_view;

    if( !r->pntc_bindings_valid )
    {
        ID3D11DeviceContext_IASetInputLayout( r->context, r->layout_pntc );
        ID3D11DeviceContext_VSSetShader( r->context, r->vs_pntc, NULL, 0 );
        ID3D11DeviceContext_PSSetShader( r->context, r->ps_pntc, NULL, 0 );
        ID3D11DeviceContext_VSSetConstantBuffers( r->context, 1, 1, &r->cb_material );
        ID3D11DeviceContext_PSSetConstantBuffers( r->context, 1, 1, &r->cb_material );
        ID3D11DeviceContext_PSSetSamplers( r->context, 1, 1, &r->environment_sampler_state );
        ID3D11DeviceContext_IASetPrimitiveTopology( r->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        r->statistics.state_bindings += 7;
    }
    if( !r->pntc_bindings_valid || r->bound_textures[0] != texture_view )
    {
        ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &texture_view );
        r->bound_textures[0] = texture_view;
        ++r->statistics.state_bindings;
    }
    if( !r->pntc_bindings_valid || memcmp( r->bound_textures + 1,
                                         r->material_texture_views, sizeof( r->material_texture_views ) ) )
    {
        ID3D11DeviceContext_PSSetShaderResources( r->context, 1, 6, r->material_texture_views );
        memcpy( r->bound_textures + 1, r->material_texture_views, sizeof( r->material_texture_views ) );
        ++r->statistics.state_bindings;
    }
    if( !r->pntc_bindings_valid || r->bound_textures[7] != r->environment_view )
    {
        ID3D11DeviceContext_PSSetShaderResources( r->context, 7, 1, &r->environment_view );
        r->bound_textures[7] = r->environment_view;
        ++r->statistics.state_bindings;
    }
    {
        ID3D11SamplerState *sampler = r->material_sampler ? r->material_sampler : r->pntc_sampler_state;
        if( !r->pntc_bindings_valid || r->bound_sampler != sampler )
        {
            ID3D11DeviceContext_PSSetSamplers( r->context, 0, 1, &sampler );
            r->bound_sampler = sampler;
            ++r->statistics.state_bindings;
        }
    }
    if( !r->pntc_bindings_valid || r->bound_vertex_buffer != geometry->vertex_buffer ||
        r->bound_index_buffer != geometry->index_buffer )
    {
        ID3D11DeviceContext_IASetVertexBuffers( r->context, 0, 1, &geometry->vertex_buffer, &stride, &offset );
        ID3D11DeviceContext_IASetIndexBuffer( r->context, geometry->index_buffer, geometry->index_format, 0 );
        r->bound_vertex_buffer = geometry->vertex_buffer;
        r->bound_index_buffer = geometry->index_buffer;
        r->statistics.state_bindings += 2;
    }
    r->pntc_bindings_valid = 1;
    ++r->statistics.draws;
    r->statistics.triangles += (uint64_t)index_count / 3;
    ID3D11DeviceContext_DrawIndexed( r->context, (UINT)index_count, (UINT)index_start,
                                     (INT)base_vertex );
}

/* =========================================================================
 * Dimension queries
 * ====================================================================== */

wp_s32 wp_renderer_dx11_get_width( const wp_renderer_dx11 *r )
{
    return r ? r->width : 0;
}

wp_s32 wp_renderer_dx11_get_height( const wp_renderer_dx11 *r )
{
    return r ? r->height : 0;
}

/* =========================================================================
 * Native object access
 * ====================================================================== */

void wp_renderer_dx11_get_native( const wp_renderer_dx11 *r, void **pp )
{
    if( !pp )
        return;
    *pp = r ? r->native : NULL;
}

void wp_renderer_dx11_set_native( wp_renderer_dx11 *r, void *native )
{
    if( r )
        r->native = native;
}

/* =========================================================================
 * DX11-specific accessors
 * ====================================================================== */

void *wp_renderer_dx11_get_device( const wp_renderer_dx11 *r )
{
    return r ? (void *)r->device : NULL;
}

void *wp_renderer_dx11_get_context( const wp_renderer_dx11 *r )
{
    return r ? (void *)r->context : NULL;
}

void *wp_renderer_dx11_get_swap_chain( const wp_renderer_dx11 *r )
{
    return r ? (void *)r->swap_chain : NULL;
}

void wp_renderer_dx11_get_projection_matrix( wp_s32 width, wp_s32 height, wp_f32 *result )
{
    const wp_f32 left = 0.0f;
    const wp_f32 right = (wp_f32)width;
    const wp_f32 top = 0.0f;
    const wp_f32 bottom = (wp_f32)height;
    wp_f32 matrix[4][4] = {
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.5f, 0.0f },
        { 0.0f, 0.0f, 0.5f, 1.0f },
    };

    if( !result || width <= 0 || height <= 0 )
        return;

    matrix[0][0] = 2.0f / ( right - left );
    matrix[1][1] = 2.0f / ( top - bottom );
    matrix[3][0] = ( right + left ) / ( left - right );
    matrix[3][1] = ( top + bottom ) / ( bottom - top );
    memcpy( result, matrix, sizeof( matrix ) );
}

void wp_renderer_dx11_font_stash_begin( wp_renderer_dx11 *r, struct wp_font_atlas **atlas )
{
    if( !atlas )
        return;
    *atlas = ( r && r->atlas_initialized ) ? &r->atlas : NULL;
}

void wp_renderer_dx11_font_stash_end( wp_renderer_dx11 *r )
{
    const void *image;
    wp_s32 width, height;
    D3D11_TEXTURE2D_DESC desc;
    D3D11_SUBRESOURCE_DATA data;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv;
    ID3D11Texture2D *texture = NULL;
    HRESULT hr;

    if( !r || !r->atlas_initialized || !r->device )
        return;

    image = wp_font_atlas_bake( &r->atlas, &width, &height, WORKPHONE_FONT_ATLAS_RGBA32 );
    if( !image || width <= 0 || height <= 0 )
        return;

    memset( &desc, 0, sizeof( desc ) );
    desc.Width = (UINT)width;
    desc.Height = (UINT)height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    memset( &data, 0, sizeof( data ) );
    data.pSysMem = image;
    data.SysMemPitch = (UINT)( width * 4 );
    hr = ID3D11Device_CreateTexture2D( r->device, &desc, &data, &texture );
    if( FAILED( hr ) )
        return;

    memset( &srv, 0, sizeof( srv ) );
    srv.Format = desc.Format;
    srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MipLevels = 1;
    hr = ID3D11Device_CreateShaderResourceView( r->device, (ID3D11Resource *)texture, &srv,
                                                &r->font_texture_view );
    ID3D11Texture2D_Release( texture );
    if( FAILED( hr ) )
        return;

    wp_font_atlas_end( &r->atlas, wp_handle_ptr( r->font_texture_view ), &r->tex_null );
    if( r->atlas.default_font )
        wp_style_set_font( &r->ctx, &r->atlas.default_font->handle );
}

void wp_renderer_dx11_render( wp_renderer_dx11 *r, enum wp_anti_aliasing anti_aliasing )
{
    const wp_f32 blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const UINT stride = sizeof( struct wp_d3d11_vertex );
    const UINT vertex_offset = 0;
    const struct wp_draw_command *cmd;
    D3D11_MAPPED_SUBRESOURCE vertices;
    D3D11_MAPPED_SUBRESOURCE indices;
    HRESULT hr;
    UINT index_offset = 0;
    wp_s32 vertices_mapped = 0;
    wp_s32 indices_mapped = 0;
#ifdef WORKPHONE_UINT_DRAW_INDEX
    const DXGI_FORMAT index_format = DXGI_FORMAT_R32_UINT;
#else
    const DXGI_FORMAT index_format = DXGI_FORMAT_R16_UINT;
#endif

    if( !r || !r->ui_initialized || !r->cmds_initialized || !r->context )
        return;

    wp_renderer_dx11_invalidate_state( r );
    hr = ID3D11DeviceContext_Map( r->context, (ID3D11Resource *)r->vertex_buffer, 0,
                                  D3D11_MAP_WRITE_DISCARD, 0, &vertices );
    if( FAILED( hr ) )
        goto cleanup;
    vertices_mapped = 1;

    hr = ID3D11DeviceContext_Map( r->context, (ID3D11Resource *)r->index_buffer, 0,
                                  D3D11_MAP_WRITE_DISCARD, 0, &indices );
    if( FAILED( hr ) )
        goto cleanup;
    indices_mapped = 1;

    {
        struct wp_convert_config config;
        struct wp_buffer vertex_buffer;
        struct wp_buffer index_buffer;
        const struct wp_draw_vertex_layout_element vertex_layout[] = {
            { WORKPHONE_VERTEX_POSITION, WORKPHONE_FORMAT_FLOAT,
              WORKPHONE_OFFSETOF( struct wp_d3d11_vertex, position ) },
            { WORKPHONE_VERTEX_TEXCOORD, WORKPHONE_FORMAT_FLOAT,
              WORKPHONE_OFFSETOF( struct wp_d3d11_vertex, uv ) },
            { WORKPHONE_VERTEX_COLOR, WORKPHONE_FORMAT_R8G8B8A8,
              WORKPHONE_OFFSETOF( struct wp_d3d11_vertex, col ) },
            { WORKPHONE_VERTEX_LAYOUT_END }
        };

        memset( &config, 0, sizeof( config ) );
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof( struct wp_d3d11_vertex );
        config.global_alpha = 1.0f;
        config.shape_AA = anti_aliasing;
        config.line_AA = anti_aliasing;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.tex_null = r->tex_null;

        wp_buffer_init_fixed( &vertex_buffer, vertices.pData, (wp_size)r->max_vertex_buffer );
        wp_buffer_init_fixed( &index_buffer, indices.pData, (wp_size)r->max_index_buffer );
        if( wp_convert( &r->ctx, &r->cmds, &vertex_buffer, &index_buffer, &config ) !=
            WORKPHONE_CONVERT_SUCCESS )
            goto cleanup;
    }

    ID3D11DeviceContext_Unmap( r->context, (ID3D11Resource *)r->index_buffer, 0 );
    indices_mapped = 0;
    ID3D11DeviceContext_Unmap( r->context, (ID3D11Resource *)r->vertex_buffer, 0 );
    vertices_mapped = 0;

    ID3D11DeviceContext_IASetInputLayout( r->context, r->input_layout );
    ID3D11DeviceContext_IASetVertexBuffers( r->context, 0, 1, &r->vertex_buffer, &stride,
                                            &vertex_offset );
    ID3D11DeviceContext_IASetIndexBuffer( r->context, r->index_buffer, index_format, 0 );
    ID3D11DeviceContext_IASetPrimitiveTopology( r->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    ID3D11DeviceContext_VSSetShader( r->context, r->vertex_shader, NULL, 0 );
    ID3D11DeviceContext_VSSetConstantBuffers( r->context, 0, 1, &r->const_buffer );
    ID3D11DeviceContext_PSSetShader( r->context, r->pixel_shader, NULL, 0 );
    ID3D11DeviceContext_PSSetSamplers( r->context, 0, 1, &r->sampler_state );
    ID3D11DeviceContext_OMSetBlendState( r->context, r->blend_state, blend_factor, 0xffffffff );
    ID3D11DeviceContext_OMSetDepthStencilState( r->context, r->ui_depth_stencil_state, 0 );
    ID3D11DeviceContext_RSSetState( r->context, r->rasterizer_state );
    ID3D11DeviceContext_RSSetViewports( r->context, 1, &r->viewport_d3d );

    wp_draw_foreach( cmd, &r->ctx, &r->cmds )
    {
        D3D11_RECT scissor;
        ID3D11ShaderResourceView *texture_view;
        if( !cmd->elem_count )
            continue;

        scissor.left = (LONG)cmd->clip_rect.x;
        scissor.top = (LONG)cmd->clip_rect.y;
        scissor.right = (LONG)( cmd->clip_rect.x + cmd->clip_rect.w );
        scissor.bottom = (LONG)( cmd->clip_rect.y + cmd->clip_rect.h );
        if( scissor.right <= scissor.left || scissor.bottom <= scissor.top )
        {
            index_offset += cmd->elem_count;
            continue;
        }

        texture_view = (ID3D11ShaderResourceView *)cmd->texture.ptr;
        ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &texture_view );
        ID3D11DeviceContext_RSSetScissorRects( r->context, 1, &scissor );
        ++r->statistics.draws;
        r->statistics.triangles += (uint64_t)cmd->elem_count / 3;
        ID3D11DeviceContext_DrawIndexed( r->context, (UINT)cmd->elem_count, index_offset, 0 );
        index_offset += cmd->elem_count;
    }

cleanup:
    if( indices_mapped )
        ID3D11DeviceContext_Unmap( r->context, (ID3D11Resource *)r->index_buffer, 0 );
    if( vertices_mapped )
        ID3D11DeviceContext_Unmap( r->context, (ID3D11Resource *)r->vertex_buffer, 0 );

    {
        ID3D11ShaderResourceView *no_texture = NULL;
        ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &no_texture );
    }
    ID3D11DeviceContext_RSSetState( r->context, r->rs_state );
    ID3D11DeviceContext_OMSetBlendState( r->context, r->bs_state, blend_factor, 0xffffffff );
    ID3D11DeviceContext_OMSetDepthStencilState( r->context, r->dss_state, 0 );
    wp_renderer_dx11_apply_viewport( r );
    wp_clear( &r->ctx );
    wp_buffer_clear( &r->cmds );
}

void wp_renderer_dx11_clipboard_paste( wp_handle usr, struct wp_text_edit *edit )
{
    (void)usr;
    if( !edit || !IsClipboardFormatAvailable( CF_UNICODETEXT ) || !OpenClipboard( NULL ) )
        return;

    {
        HGLOBAL memory = GetClipboardData( CF_UNICODETEXT );
        if( memory )
        {
            LPCWSTR wide = (LPCWSTR)GlobalLock( memory );
            if( wide )
            {
                wp_s32 wide_len = (wp_s32)lstrlenW( wide );
                wp_s32 utf8_len = WideCharToMultiByte( CP_UTF8, 0, wide, wide_len, NULL, 0, NULL, NULL );
                if( utf8_len > 0 )
                {
                    wp_c8 *utf8 = (wp_c8 *)malloc( (size_t)utf8_len );
                    if( utf8 )
                    {
                        WideCharToMultiByte( CP_UTF8, 0, wide, wide_len, utf8, utf8_len, NULL, NULL );
                        wp_textedit_paste( edit, utf8, utf8_len );
                        free( utf8 );
                    }
                }
                GlobalUnlock( memory );
            }
        }
    }
    CloseClipboard();
}

void wp_renderer_dx11_clipboard_copy( wp_handle usr, const wp_c8 *text, wp_s32 len )
{
    (void)usr;
    if( !text || len <= 0 || !OpenClipboard( NULL ) )
        return;

    {
        wp_s32 wide_len = MultiByteToWideChar( CP_UTF8, 0, text, len, NULL, 0 );
        if( wide_len > 0 )
        {
            HGLOBAL memory = GlobalAlloc( GMEM_MOVEABLE, (size_t)( wide_len + 1 ) * sizeof( WCHAR ) );
            if( memory )
            {
                WCHAR *wide = (WCHAR *)GlobalLock( memory );
                if( wide )
                {
                    MultiByteToWideChar( CP_UTF8, 0, text, len, wide, wide_len );
                    wide[wide_len] = L'\0';
                    GlobalUnlock( memory );
                    EmptyClipboard();
                    if( !SetClipboardData( CF_UNICODETEXT, memory ) )
                        GlobalFree( memory );
                }
                else
                    GlobalFree( memory );
            }
        }
    }
    CloseClipboard();
}

wp_s32 wp_renderer_dx11_handle_event( wp_renderer_dx11 *r, HWND wnd, UINT msg, WPARAM wparam,
                                      LPARAM lparam )
{
    struct wp_context *ctx;

    if( !r || !r->ui_initialized )
        return 0;
    ctx = &r->ctx;

    switch( msg )
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        wp_s32 down = !( ( lparam >> 31 ) & 1 );
        wp_s32 ctrl = ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
        switch( wparam )
        {
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            wp_input_key( ctx, WORKPHONE_KEY_SHIFT, down );
            return 1;
        case VK_DELETE:
            wp_input_key( ctx, WORKPHONE_KEY_DEL, down );
            return 1;
        case VK_RETURN:
            wp_input_key( ctx, WORKPHONE_KEY_ENTER, down );
            return 1;
        case VK_TAB:
            wp_input_key( ctx, WORKPHONE_KEY_TAB, down );
            return 1;
        case VK_UP:
            wp_input_key( ctx, WORKPHONE_KEY_UP, down );
            return 1;
        case VK_DOWN:
            wp_input_key( ctx, WORKPHONE_KEY_DOWN, down );
            return 1;
        case VK_LEFT:
            wp_input_key( ctx, ctrl ? WORKPHONE_KEY_TEXT_WORD_LEFT : WORKPHONE_KEY_LEFT, down );
            return 1;
        case VK_RIGHT:
            wp_input_key( ctx, ctrl ? WORKPHONE_KEY_TEXT_WORD_RIGHT : WORKPHONE_KEY_RIGHT, down );
            return 1;
        case VK_BACK:
            wp_input_key( ctx, WORKPHONE_KEY_BACKSPACE, down );
            return 1;
        case VK_HOME:
            wp_input_key( ctx, WORKPHONE_KEY_TEXT_START, down );
            wp_input_key( ctx, WORKPHONE_KEY_SCROLL_START, down );
            return 1;
        case VK_END:
            wp_input_key( ctx, WORKPHONE_KEY_TEXT_END, down );
            wp_input_key( ctx, WORKPHONE_KEY_SCROLL_END, down );
            return 1;
        case VK_NEXT:
            wp_input_key( ctx, WORKPHONE_KEY_SCROLL_DOWN, down );
            return 1;
        case VK_PRIOR:
            wp_input_key( ctx, WORKPHONE_KEY_SCROLL_UP, down );
            return 1;
        case 'A':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_TEXT_SELECT_ALL, down );
            return ctrl;
        case 'C':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_COPY, down );
            return ctrl;
        case 'V':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_PASTE, down );
            return ctrl;
        case 'X':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_CUT, down );
            return ctrl;
        case 'Z':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_TEXT_UNDO, down );
            return ctrl;
        case 'R':
            if( ctrl )
                wp_input_key( ctx, WORKPHONE_KEY_TEXT_REDO, down );
            return ctrl;
        default:
            return 0;
        }
    }
    case WM_CHAR:
        if( wparam >= 32 )
        {
            wp_input_unicode( ctx, (wp_rune)wparam );
            return 1;
        }
        return 0;
    case WM_LBUTTONDOWN:
        wp_input_button( ctx, WORKPHONE_BUTTON_LEFT, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 1 );
        SetCapture( wnd );
        return 1;
    case WM_LBUTTONUP:
        wp_input_button( ctx, WORKPHONE_BUTTON_LEFT, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 0 );
        wp_input_button( ctx, WORKPHONE_BUTTON_DOUBLE, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 0 );
        ReleaseCapture();
        return 1;
    case WM_RBUTTONDOWN:
        wp_input_button( ctx, WORKPHONE_BUTTON_RIGHT, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 1 );
        SetCapture( wnd );
        return 1;
    case WM_RBUTTONUP:
        wp_input_button( ctx, WORKPHONE_BUTTON_RIGHT, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 0 );
        ReleaseCapture();
        return 1;
    case WM_MBUTTONDOWN:
        wp_input_button( ctx, WORKPHONE_BUTTON_MIDDLE, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 1 );
        SetCapture( wnd );
        return 1;
    case WM_MBUTTONUP:
        wp_input_button( ctx, WORKPHONE_BUTTON_MIDDLE, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 0 );
        ReleaseCapture();
        return 1;
    case WM_MOUSEWHEEL:
        wp_input_scroll( ctx, wp_make_vec2f( 0.0f, (wp_f32)(short)HIWORD( wparam ) / WHEEL_DELTA ) );
        return 1;
    case WM_MOUSEMOVE:
        wp_input_motion( ctx, (short)LOWORD( lparam ), (short)HIWORD( lparam ) );
        return 1;
    case WM_LBUTTONDBLCLK:
        wp_input_button( ctx, WORKPHONE_BUTTON_DOUBLE, (short)LOWORD( lparam ),
                         (short)HIWORD( lparam ), 1 );
        return 1;
    default:
        return 0;
    }
}

struct wp_context *wp_renderer_dx11_init( wp_renderer_dx11 *r, wp_s32 width, wp_s32 height,
                                          wp_u32 max_vertex_buffer, wp_u32 max_index_buffer )
{
    HRESULT hr;
    D3D11_BUFFER_DESC buf_desc;
    D3D11_BLEND_DESC blend_desc;
    D3D11_DEPTH_STENCIL_DESC depth_desc;
    D3D11_RASTERIZER_DESC rast_desc;
    D3D11_SAMPLER_DESC samp_desc;
    D3D11_INPUT_ELEMENT_DESC layout_desc[3];

    if( !r || !r->device || !r->context )
        return NULL;

    r->max_vertex_buffer = (wp_s32)max_vertex_buffer;
    r->max_index_buffer = (wp_s32)max_index_buffer;

    /* ---- initialise UI context ---------------------------------------- */
    memset( &r->ctx, 0, sizeof( r->ctx ) );
    if( !wp_init_default( &r->ctx, 0 ) )
        return NULL;
    r->ui_initialized = 1;
    r->ctx.clip.copy = wp_renderer_dx11_clipboard_copy;
    r->ctx.clip.paste = wp_renderer_dx11_clipboard_paste;
    r->ctx.clip.userdata = wp_handle_ptr( r );

    /* ---- initialise font atlas and add default font ------------------- */
    wp_font_atlas_init_default( &r->atlas );
    r->atlas_initialized = 1;
    wp_font_atlas_begin( &r->atlas );
    r->atlas.default_font = wp_font_atlas_add_default( &r->atlas, 13.0f, 0 );
    if( !r->atlas.default_font )
        return NULL;

    /* ---- initialise command buffer ------------------------------------ */
    wp_buffer_init_default( &r->cmds );
    r->cmds_initialized = 1;

    /* ---- create UI vertex shader from precompiled bytecode ------------ */
    hr = ID3D11Device_CreateVertexShader( r->device, wp_dx11_vertex_shader,
                                          sizeof( wp_dx11_vertex_shader ), NULL, &r->vertex_shader );
    if( FAILED( hr ) )
        return NULL;

    /* ---- create UI pixel shader from precompiled bytecode ------------- */
    hr = ID3D11Device_CreatePixelShader( r->device, wp_dx11_pixel_shader, sizeof( wp_dx11_pixel_shader ),
                                         NULL, &r->pixel_shader );
    if( FAILED( hr ) )
        return NULL;

    /* ---- create UI input layout matching wp_d3d11_vertex -------------- */
    memset( layout_desc, 0, sizeof( layout_desc ) );

    layout_desc[0].SemanticName = "POSITION";
    layout_desc[0].SemanticIndex = 0;
    layout_desc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
    layout_desc[0].AlignedByteOffset = 0;
    layout_desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    layout_desc[1].SemanticName = "TEXCOORD";
    layout_desc[1].SemanticIndex = 0;
    layout_desc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    layout_desc[1].AlignedByteOffset = 8;
    layout_desc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    layout_desc[2].SemanticName = "COLOR";
    layout_desc[2].SemanticIndex = 0;
    layout_desc[2].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    layout_desc[2].AlignedByteOffset = 16;
    layout_desc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    hr = ID3D11Device_CreateInputLayout( r->device, layout_desc, 3, wp_dx11_vertex_shader,
                                         sizeof( wp_dx11_vertex_shader ), &r->input_layout );
    if( FAILED( hr ) )
        return NULL;

    /* ---- create UI constant buffer (projection matrix) ---------------- */
    memset( &buf_desc, 0, sizeof( buf_desc ) );
    buf_desc.ByteWidth = sizeof( float ) * 4 * 4;
    buf_desc.Usage = D3D11_USAGE_DYNAMIC;
    buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buf_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    {
        wp_f32 matrix[16];
        D3D11_SUBRESOURCE_DATA data;
        wp_renderer_dx11_get_projection_matrix( width, height, matrix );
        memset( &data, 0, sizeof( data ) );
        data.pSysMem = matrix;
        hr = ID3D11Device_CreateBuffer( r->device, &buf_desc, &data, &r->const_buffer );
    }
    if( FAILED( hr ) )
        return NULL;

    /* ---- create UI dynamic vertex buffer ------------------------------ */
    memset( &buf_desc, 0, sizeof( buf_desc ) );
    buf_desc.ByteWidth = (UINT)max_vertex_buffer;
    buf_desc.Usage = D3D11_USAGE_DYNAMIC;
    buf_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    buf_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = ID3D11Device_CreateBuffer( r->device, &buf_desc, NULL, &r->vertex_buffer );
    if( FAILED( hr ) )
        return NULL;

    /* ---- create UI dynamic index buffer ------------------------------- */
    memset( &buf_desc, 0, sizeof( buf_desc ) );
    buf_desc.ByteWidth = (UINT)max_index_buffer;
    buf_desc.Usage = D3D11_USAGE_DYNAMIC;
    buf_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    buf_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = ID3D11Device_CreateBuffer( r->device, &buf_desc, NULL, &r->index_buffer );
    if( FAILED( hr ) )
        return NULL;

    /* ---- create UI blend state (alpha blending) ----------------------- */
    memset( &blend_desc, 0, sizeof( blend_desc ) );
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = ID3D11Device_CreateBlendState( r->device, &blend_desc, &r->blend_state );
    if( FAILED( hr ) )
        return NULL;

    /* ---- create UI depth state (coplanar primitives must retain order) -- */
    memset( &depth_desc, 0, sizeof( depth_desc ) );
    depth_desc.DepthEnable = FALSE;
    depth_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depth_desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depth_desc.StencilEnable = FALSE;

    hr = ID3D11Device_CreateDepthStencilState( r->device, &depth_desc, &r->ui_depth_stencil_state );
    if( FAILED( hr ) )
        return NULL;

    /* ---- create UI rasterizer state (no culling, scissor enabled) ----- */
    memset( &rast_desc, 0, sizeof( rast_desc ) );
    rast_desc.FillMode = D3D11_FILL_SOLID;
    rast_desc.CullMode = D3D11_CULL_NONE;
    rast_desc.FrontCounterClockwise = FALSE;
    rast_desc.DepthClipEnable = TRUE;
    rast_desc.ScissorEnable = TRUE;

    hr = ID3D11Device_CreateRasterizerState( r->device, &rast_desc, &r->rasterizer_state );
    if( FAILED( hr ) )
        return NULL;

    /* ---- create UI sampler state -------------------------------------- */
    memset( &samp_desc, 0, sizeof( samp_desc ) );
    samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

    hr = ID3D11Device_CreateSamplerState( r->device, &samp_desc, &r->sampler_state );
    if( FAILED( hr ) )
        return NULL;

    /* ---- store viewport dimensions ------------------------------------ */
    r->viewport_d3d.TopLeftX = 0.0f;
    r->viewport_d3d.TopLeftY = 0.0f;
    r->viewport_d3d.Width = (float)width;
    r->viewport_d3d.Height = (float)height;
    r->viewport_d3d.MinDepth = 0.0f;
    r->viewport_d3d.MaxDepth = 1.0f;

    return &r->ctx;
}

#endif /* _WIN32 */
