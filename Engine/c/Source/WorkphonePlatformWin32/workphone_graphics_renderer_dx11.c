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
#    include <d3d11_1.h>
#    include <stddef.h>
#    include <d3dcompiler.h>
#    include <dxgi.h>
#    include <dxgi1_5.h>
#    include <dxgi1_6.h>
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
#    include "workphone_graphics_mesh.h"
#    include "../WorkphoneGraphics/workphone_graphics_mesh_memory.h"
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

/* Draw-owned transfer control; zero/unbound preserves the direct legacy path. */
#define WP_HDR_OUTPUT_HLSL \
    "cbuffer SceneOutput : register(b5) { float4 scene_output; };\n" \
    "float3 sceneDecode(float3 c) { c=max(c,0.0); return lerp(c/12.92,pow((c+0.055)/1.055,2.4),step(0.04045,c)); }\n"

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
static const wp_c8 s_ps_src[] = WP_HDR_OUTPUT_HLSL
    "struct PS_IN { float4 pos : SV_POSITION; float4 color : COLOR; };\n"
    "float4 ps_main(PS_IN i) : SV_TARGET { return float4(scene_output.x>0.5 ? sceneDecode(i.color.rgb) : i.color.rgb,i.color.a); }\n";

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
static const wp_c8 s_ps_ptc_src[] = WP_HDR_OUTPUT_HLSL
    "Texture2D tex : register(t0);\n"
    "SamplerState tex_sampler : register(s0);\n"
    "struct PS_IN { float4 pos : SV_POSITION; float2 uv : TEXCOORD0;\n"
    "               float4 color : COLOR; };\n"
    "float4 ps_main(PS_IN i) : SV_TARGET\n"
    "{\n"
    "    float4 t=tex.Sample(tex_sampler,i.uv);\n"
    "    if(scene_output.x>0.5) { if(scene_output.z<0.5) t.rgb=sceneDecode(t.rgb); return t*float4(sceneDecode(i.color.rgb),i.color.a); }\n"
    "    return t * i.color;\n"
    "}\n";

/* One declaration for both shader variants. CPU surface packing deliberately
 * keeps view constants and actual texture-transfer/readiness flags separate. */
#define WP_MATERIAL_CONSTANTS_HLSL \
    "#ifdef WP_MATERIAL_SPLIT_CONSTANTS\n" \
    "cbuffer Material : register(b1) { float4 base_color; float4 emissive_color; float4 specular_color;\n" \
    " float4 surface; float4 uv_transform; float4 controls; float4 texture_sources; float4 projection; };\n" \
    "cbuffer MaterialView : register(b6) { float4 light_color; float4 light_direction; float4 camera_position;\n" \
    " float4 ambient_color; float4 environment; float4 fog_color; float4 fog_params; };\n" \
    "cbuffer MaterialBindings : register(b7) { float4 map_flags; float4 extra_map_flags; };\n" \
    "#else\n" \
    "cbuffer Material : register(b1) { float4 base_color; float4 emissive_color; float4 specular_color;\n" \
    " float4 light_color; float4 light_direction; float4 camera_position; float4 surface; float4 uv_transform;\n" \
    " float4 controls; float4 map_flags; float4 extra_map_flags; float4 texture_sources; float4 projection;\n" \
    " float4 ambient_color; float4 environment; float4 fog_color; float4 fog_params; };\n" \
    "#endif\n"

#define WP_VERTEX_LIGHTING_HLSL "cbuffer VertexLighting:register(b8) { float4 vertex_light_direction; };\n"

/* ---- lit position + normal + texcoord + colour shaders --------------- */
static const wp_c8 s_vs_pntc_src[] =
    "cbuffer Transform : register(b0) { row_major float4x4 mvp; row_major float4x4 world;\n"
    " row_major float4x4 normal_matrix; };\n"
    WP_MATERIAL_CONSTANTS_HLSL
    WP_VERTEX_LIGHTING_HLSL
    "struct VS_IN { float3 pos : POSITION; float3 normal : NORMAL;\n"
    " float2 uv : TEXCOORD0; uint color : COLOR; };\n"
    "struct VS_OUT { float4 pos : SV_POSITION; float3 world_pos : TEXCOORD0;\n"
    " float3 normal : TEXCOORD1; float2 uv : TEXCOORD2; float4 color : COLOR;\n"
    " float3 object_pos : TEXCOORD3; float3 object_normal : TEXCOORD4; float4 clip_pos : TEXCOORD5; float vertex_light:TEXCOORD6; };\n"
    "VS_OUT vs_main(VS_IN i) { VS_OUT o;\n"
    " o.pos = mul(mvp, float4(i.pos, 1.0));\n"
    " o.pos.z = 0.5 * (o.pos.z + o.pos.w);\n"
    " o.world_pos = mul(world, float4(i.pos, 1.0)).xyz;\n"
    " o.normal = normalize(mul((float3x3)normal_matrix, i.normal));\n"
    " o.uv = i.uv; o.object_pos = i.pos; o.object_normal = i.normal; o.clip_pos = o.pos;\n"
    " o.color = float4(float((i.color >> 24u) & 255u),\n"
    "  float((i.color >> 16u) & 255u), float((i.color >> 8u) & 255u),\n"
    "  float(i.color & 255u)) / 255.0; o.vertex_light=saturate(dot(o.normal,-vertex_light_direction.xyz)*rsqrt(max(dot(vertex_light_direction.xyz,vertex_light_direction.xyz),1e-8))); return o; }\n";

static const wp_c8 s_ps_pntc_src[] = WP_HDR_OUTPUT_HLSL
    "cbuffer IrradianceProbes:register(b9) { float4 gi_controls; float4 gi_probes[40]; };\n"
    "float3 probeIrradiance(float3 position,float3 n) { float3 result=0; float sum=0;\n"
    " if(gi_controls.y<.5) return 0; float basis[9]; basis[0]=.282095; basis[1]=.488603*n.y;\n"
    " basis[2]=.488603*n.z;basis[3]=.488603*n.x;basis[4]=1.092548*n.x*n.y;\n"
    " basis[5]=1.092548*n.y*n.z;basis[6]=.315392*(3*n.z*n.z-1);\n"
    " basis[7]=1.092548*n.x*n.z;basis[8]=.546274*(n.x*n.x-n.y*n.y);\n"
    " [loop] for(int p=0;p<(int)gi_controls.x;++p) { float3 delta=position-gi_probes[p*10].xyz;\n"
    " float weight=1/max(dot(delta,delta),.0001); float3 value=0;\n"
    " [unroll] for(int j=0;j<9;++j) value+=gi_probes[p*10+j+1].rgb*basis[j];\n"
    " result+=max(value,0)*weight;sum+=weight; } return result/max(sum,.0001); }\n"
    "Texture2D shadow_tex : register(t8); SamplerComparisonState shadow_sampler : register(s2);\n"
    "cbuffer Shadow : register(b2) { row_major float4x4 light_matrix; float4 shadow_controls; };\n"
    "float shadowVisibility(float3 position, float ndl) {\n"
    " if (shadow_controls.x < 0.5) return 1.0;\n"
    " float4 p = mul(light_matrix, float4(position, 1.0)); float3 q = p.xyz / p.w;\n"
    " q = q * float3(0.5, -0.5, 0.5) + 0.5;\n"
    " if (any(q < 0.0) || any(q > 1.0)) return 1.0;\n"
    " float visibility = 0.0; float bias = shadow_controls.w * (1.0 + 2.0 * (1.0 - ndl));\n"
    " int radius=clamp((int)shadow_controls.x-1,0,3);\n"
    " [loop] for (int y = -radius; y <= radius; ++y) [loop] for (int x = -radius; x <= radius; ++x)\n"
    " visibility += shadow_tex.SampleCmpLevelZero(shadow_sampler, q.xy + float2(x,y) * shadow_controls.z, q.z - bias);\n"
    " return visibility / ((2*radius+1)*(2*radius+1)); }\n"
    "TextureCube environment_tex : register(t7); SamplerState environment_sampler : register(s1);\n"
    "Texture2D tex : register(t0); SamplerState tex_sampler : register(s0);\n"
    "Texture2D normal_tex : register(t1); Texture2D metallic_tex : register(t2);\n"
    "Texture2D roughness_tex : register(t3); Texture2D emission_tex : register(t4);\n"
    "Texture2D ao_tex : register(t5); Texture2D opacity_tex : register(t6);\n"
    "Texture2D terrain_albedo0 : register(t9); Texture2D terrain_normal0 : register(t10); Texture2D terrain_orm0 : register(t11);\n"
    "Texture2D terrain_albedo1 : register(t12); Texture2D terrain_normal1 : register(t13); Texture2D terrain_orm1 : register(t14);\n"
    "Texture2D terrain_albedo2 : register(t15); Texture2D terrain_normal2 : register(t16); Texture2D terrain_orm2 : register(t17);\n"
    "Texture2D terrain_albedo3 : register(t18); Texture2D terrain_normal3 : register(t19); Texture2D terrain_orm3 : register(t20);\n"
    "Texture2D terrain_control : register(t21); SamplerState terrain_control_sampler : register(s3); SamplerState terrain_sampler : register(s4);\n"
    "Texture2D terrain_detail : register(t22); Texture2D terrain_masks : register(t23);\n"
    "cbuffer Terrain : register(b3) { float4 terrain_transform; float4 terrain_controls;\n"
    " float4 terrain_tint[4]; float4 terrain_surface[4]; float4 terrain_maps[4]; float4 terrain_height[4];\n"
    " float4 terrain_appearance; float4 terrain_detail_params; float4 terrain_macro; float4 terrain_macro_tint;\n"
    " float4 terrain_weather; float4 terrain_snow; float4 terrain_snow_tint; };\n"
    "float3 decode_srgb(float3 c) { c=max(c,0.0);\n"
    " return lerp(c/12.92,pow((c+0.055)/1.055,2.4),step(0.04045,c)); }\n"
    "float3 terrainMapNormal(float3 position,float3 n,float2 uv,float3 mapped) {\n"
    " float3 dp1=ddx(position),dp2=ddy(position); float2 duv1=ddx(uv),duv2=ddy(uv);\n"
    " float3 p2=cross(dp2,n),p1=cross(n,dp1);\n"
    " float3 t=p2*duv1.x+p1*duv2.x,b=p2*duv1.y+p1*duv2.y;\n"
    " float inv=rsqrt(max(max(dot(t,t),dot(b,b)),1e-8));\n"
    " return normalize(t*inv*mapped.x+b*inv*mapped.y+n*mapped.z+n*1e-6); }\n"
    "void terrainLayerPlane(Texture2D a,Texture2D n,Texture2D o,float2 plane,float3 worldPos,float3 geometric,\n"
    " float4 tint,float4 params,float4 flags,float4 height,out float3 albedo,out float3 normal,out float4 orm) {\n"
    " float2 uv = plane * params.xy;\n"
    " float3 colour = flags.x > 0.5 ? a.Sample(terrain_sampler, uv).rgb : 1.0;\n"
    " albedo = (height.z > 0.5 ? colour : decode_srgb(colour)) * decode_srgb(tint.rgb);\n"
    " normal = flags.y > 0.5 ? n.Sample(terrain_sampler, uv).xyz * 2.0 - 1.0 : float3(0,0,1);\n"
    " normal.xy *= flags.w; normal = normalize(normal + float3(0,0,1e-6));\n"
    " if(terrain_detail_params.w>0.5 && terrain_detail_params.z>0.0) { float3 detail=terrain_detail.Sample(terrain_sampler,plane*terrain_detail_params.xy).xyz*2.0-1.0;\n"
    "   detail.xy*=terrain_detail_params.z; detail=normalize(detail+float3(0,0,1e-6));\n"
    "   normal=normalize(float3(normal.xy+detail.xy,normal.z*detail.z)+float3(0,0,1e-6)); }\n"
    " normal=terrainMapNormal(worldPos,geometric,plane,normal);\n"
    " orm = flags.z > 0.5 ? o.Sample(terrain_sampler, uv) : float4(1,1,1,0);\n"
    " orm.y *= params.z; orm.z *= params.w; orm.w=orm.w*height.x+height.y;\n"
    "}\n"
    "void terrainLayer(Texture2D a,Texture2D n,Texture2D o,float3 position,float3 objectNormal,float3 worldPos,float3 geometric,\n"
    " float4 tint,float4 params,float4 flags,float4 height,out float3 albedo,out float3 normal,out float4 orm) {\n"
    " if(terrain_appearance.x<0.5) { terrainLayerPlane(a,n,o,position.xz,worldPos,geometric,tint,params,flags,height,albedo,normal,orm); return; }\n"
    " float3 blend=pow(abs(objectNormal),terrain_appearance.y); blend/=max(dot(blend,1.0),1e-8);\n"
    " float3 ax,nx,ay,ny,az,nz; float4 ox,oy,oz;\n"
    " terrainLayerPlane(a,n,o,position.zy,worldPos,geometric,tint,params,flags,height,ax,nx,ox);\n"
    " terrainLayerPlane(a,n,o,position.xz,worldPos,geometric,tint,params,flags,height,ay,ny,oy);\n"
    " terrainLayerPlane(a,n,o,position.xy,worldPos,geometric,tint,params,flags,height,az,nz,oz);\n"
    " albedo=ax*blend.x+ay*blend.y+az*blend.z; orm=ox*blend.x+oy*blend.y+oz*blend.z;\n"
    " normal=normalize(nx*blend.x+ny*blend.y+nz*blend.z+geometric*1e-6);\n"
    "}\n"
    "float terrainMacroHash(float2 cell,uint seed) { uint2 bits=asuint(cell);\n"
    " uint h=bits.x*1664525u^bits.y*1013904223u^seed; h^=h>>16; h*=2246822519u; h^=h>>13;\n"
    " return float(h&16777215u)/16777215.0; }\n"
    "float terrainMacroNoise(float2 position) { float2 p=position*terrain_macro.x, cell=floor(p),f=frac(p); f=f*f*(3.0-2.0*f);\n"
    " uint seed=uint(terrain_macro.z)|(uint(terrain_macro.w)<<16);\n"
    " return lerp(lerp(terrainMacroHash(cell,seed),terrainMacroHash(cell+float2(1,0),seed),f.x),\n"
    " lerp(terrainMacroHash(cell+float2(0,1),seed),terrainMacroHash(cell+1.0,seed),f.x),f.y);\n"
    "}\n"
    WP_MATERIAL_CONSTANTS_HLSL
    "struct PS_IN { float4 pos : SV_POSITION; float3 world_pos : TEXCOORD0;\n"
    " float3 normal : TEXCOORD1; float2 uv : TEXCOORD2; float4 color : COLOR;\n"
    " float3 object_pos : TEXCOORD3; float3 object_normal : TEXCOORD4; float4 clip_pos : TEXCOORD5; float vertex_light:TEXCOORD6; };\n"
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
    " float3 colourSample = extra_map_flags.z > 0.5 ? sampled.rgb : decode_srgb(sampled.rgb);\n"
    " float3 albedo = colourSample * decode_srgb(base_color.rgb) * decode_srgb(i.color.rgb);\n"
    // Heightfields have an authored upward surface normal. Their two-sided raster
    // submission must not flip lighting into the ground based on front-face state.
    " float3 terrainNormal = normalize(i.normal), terrainOrm = float3(1,1,0);\n"
    " if (terrain_controls.x > 0.5) {\n"
    "   float4 weights = terrain_controls.y > 0.5 ? max(terrain_control.Sample(terrain_control_sampler, i.object_pos.xz * terrain_transform.zw + terrain_transform.xy), 0.0) : float4(1,0,0,0);\n"
    "   float total = dot(weights, 1.0); weights = total > 1e-8 ? weights / total : float4(1,0,0,0);\n"
    "   float3 a0,n0,a1,n1,a2,n2,a3,n3; float4 o0,o1,o2,o3;\n"
    "   terrainLayer(terrain_albedo0,terrain_normal0,terrain_orm0,i.object_pos,i.object_normal,i.world_pos,terrainNormal,terrain_tint[0],terrain_surface[0],terrain_maps[0],terrain_height[0],a0,n0,o0);\n"
    "   terrainLayer(terrain_albedo1,terrain_normal1,terrain_orm1,i.object_pos,i.object_normal,i.world_pos,terrainNormal,terrain_tint[1],terrain_surface[1],terrain_maps[1],terrain_height[1],a1,n1,o1);\n"
    "   terrainLayer(terrain_albedo2,terrain_normal2,terrain_orm2,i.object_pos,i.object_normal,i.world_pos,terrainNormal,terrain_tint[2],terrain_surface[2],terrain_maps[2],terrain_height[2],a2,n2,o2);\n"
    "   terrainLayer(terrain_albedo3,terrain_normal3,terrain_orm3,i.object_pos,i.object_normal,i.world_pos,terrainNormal,terrain_tint[3],terrain_surface[3],terrain_maps[3],terrain_height[3],a3,n3,o3);\n"
    "   if(terrain_appearance.z>0.5) { float4 heights=float4(o0.w,o1.w,o2.w,o3.w);\n"
    "     float4 eligible=lerp(-100.0,heights,step(1e-8,weights)); float high=max(max(eligible.x,eligible.y),max(eligible.z,eligible.w));\n"
    "     weights*=max(heights-high+terrain_appearance.w,0.0); weights/=max(dot(weights,1.0),1e-8); }\n"
    "   albedo = a0*weights.x+a1*weights.y+a2*weights.z+a3*weights.w;\n"
    "   terrainNormal = normalize(n0*weights.x+n1*weights.y+n2*weights.z+n3*weights.w + terrainNormal*1e-6);\n"
    "   terrainOrm = o0.xyz*weights.x+o1.xyz*weights.y+o2.xyz*weights.z+o3.xyz*weights.w;\n"
    "   if(terrain_macro.y>0.0) albedo*=lerp(1.0,terrain_macro_tint.rgb,terrain_macro.y*terrainMacroNoise(i.object_pos.xz));\n"
    "   float2 masks=terrain_snow.y>0.5 ? saturate(terrain_masks.Sample(terrain_control_sampler,i.object_pos.xz*terrain_transform.zw+terrain_transform.xy).rg) : 1.0;\n"
    "   float wet=terrain_weather.x*masks.x,snow=terrain_weather.w*masks.y;\n"
    "   albedo*=1.0-wet*terrain_weather.y; terrainOrm.y=lerp(terrainOrm.y,min(terrainOrm.y,terrain_weather.z),wet);\n"
    "   albedo=lerp(albedo,decode_srgb(terrain_snow_tint.rgb),snow); terrainOrm.yz=lerp(terrainOrm.yz,float2(terrain_snow.x,0),snow);\n"
    "   terrainNormal=normalize(lerp(terrainNormal,normalize(front ? i.normal : -i.normal),snow));\n"
    " }\n"
    " float alpha = base_color.a * i.color.a;\n"
    " if (texture_sources.w < 0.5) alpha *= sampled.a;\n"
    " else if (extra_map_flags.y > 0.5 && texture_sources.w < 2.5) {\n"
    "     float4 opacity = opacity_tex.Sample(tex_sampler, i.uv);\n"
    "     alpha *= texture_sources.w < 1.5 ? opacity.r : opacity.a; }\n"
    " if (terrain_controls.x > 0.5) alpha = 1.0;\n"
    " if (controls.z >= 0.0) clip(alpha - controls.z);\n"
    " if (shadow_controls.y > 0.5) { clip(alpha - 0.001); return 0.0; }\n"
    " float3 emission = emissive_color.rgb;\n"
    " if (map_flags.w > 0.5) { float3 e=emission_tex.Sample(tex_sampler, i.uv).rgb;\n"
    "     emission *= extra_map_flags.w > 0.5 ? e : decode_srgb(e); }\n"
    " if (uv_transform.z < 0.5) { float3 c = scene_output.x>0.5 ? max(albedo+emission,0.0) : pow(max(albedo + emission, 0.0), 1.0 / 2.2);\n"
    "     return float4(controls.w > 0.5 ? c * alpha : c, alpha); }\n"
    " float metalness = saturate(surface.x);\n"
    " float roughness = clamp(surface.y, 0.045, 1.0);\n"
    " float4 metallic = map_flags.y > 0.5 ? metallic_tex.Sample(tex_sampler, i.uv) : 1.0;\n"
    " float4 rough = map_flags.z > 0.5 ? roughness_tex.Sample(tex_sampler, i.uv) : 1.0;\n"
    " if (map_flags.y > 0.5) metalness *= texture_sources.x > 0.5 && texture_sources.x < 1.5 ? metallic.a : metallic.r;\n"
    " if (map_flags.y > 0.5 && texture_sources.x > 3.5) metalness = surface.x * metallic.b;\n"
    " if (texture_sources.x > 1.5 && texture_sources.x < 2.5) metalness = surface.x * sampled.a;\n"
    " if (map_flags.z > 0.5) {\n"
    "     float channel = texture_sources.y < 0.5 ? rough.r : rough.a;\n"
    "     if (texture_sources.y > 2.5 && texture_sources.y < 3.5) channel = rough.g;\n"
    "     if (texture_sources.y > 3.5 && texture_sources.y < 4.5) channel = 1.0 - rough.a;\n"
    "     if (texture_sources.y > 4.5) channel = rough.g;\n"
    "     roughness *= channel; }\n"
    " roughness = clamp(roughness, 0.045, 1.0);\n"
    " if (terrain_controls.x > 0.5) { metalness = saturate(terrainOrm.z); roughness = clamp(terrainOrm.y, 0.045, 1.0); }\n"
    " float3 n = normalize(front ? i.normal : -i.normal);\n"
    " if (map_flags.x > 0.5 && terrain_controls.x < 0.5) {\n"
    "     float3 dp1 = ddx(i.world_pos), dp2 = ddy(i.world_pos);\n"
    "     float2 duv1 = ddx(i.uv), duv2 = ddy(i.uv);\n"
    "     float3 p2 = cross(dp2, n), p1 = cross(n, dp1);\n"
    "     float3 t = p2 * duv1.x + p1 * duv2.x, b = p2 * duv1.y + p1 * duv2.y;\n"
    "     float inv = rsqrt(max(max(dot(t,t), dot(b,b)), 1e-8));\n"
    "     float3 mapped = normal_tex.Sample(tex_sampler, i.uv).xyz * 2.0 - 1.0;\n"
    "     mapped.xy *= controls.x; n = normalize(t * inv * mapped.x + b * inv * mapped.y + n * mapped.z); }\n"
    " if (terrain_controls.x > 0.5) n=terrainNormal;\n"
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
    " float3 color = (projection.z>.5 && projection.z<1.5) ? albedo*radiance*i.vertex_light/3.14159265 : (diffuse + specular) * radiance * ndl;\n"
    " color *= shadowVisibility(i.world_pos,ndl);\n"
    " float ao = 1.0;\n"
    " if (extra_map_flags.x > 0.5 && texture_sources.z < 1.5) {\n"
    "     float4 occlusion = ao_tex.Sample(tex_sampler, i.uv);\n"
    "     ao = texture_sources.z < 0.5 ? occlusion.r : occlusion.b; }\n"
    " if (texture_sources.z > 1.5 && texture_sources.z < 2.5) ao = i.color.a;\n"
    " if (terrain_controls.x > 0.5) ao = saturate(terrainOrm.x);\n"
    " ao = lerp(1.0, ao, saturate(controls.y));\n"
    " float3 ambient = ambient_color.w > 0.5 ? max(ambient_color.rgb, 0.0) : camera_position.www;\n"
    " float3 ambientF = f0 + (max(1.0 - roughness, f0) - f0) * pow(1.0 - ndv, 5.0);\n"
    " color += (1.0 - ambientF) * (1.0 - metalness) * albedo * ambient * hemi * ao;\n"
    " color += albedo*(1-metalness)*probeIrradiance(i.world_pos,n)*ao/3.14159265;\n"
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
    " if (fog_params.x > 0.5) {\n"
    "     float distanceToEye = length(camera_position.xyz - i.world_pos);\n"
    "     float amount = fog_params.x < 1.5 ? 1.0 - exp(-fog_params.y * distanceToEye) :\n"
    "         fog_params.x < 2.5 ? 1.0 - exp(-pow(fog_params.y * distanceToEye, 2.0)) :\n"
    "         (distanceToEye - fog_params.z) / max(fog_params.w - fog_params.z, 0.001);\n"
    "     color = lerp(color, max(fog_color.rgb, 0.0), saturate(amount)); }\n"
    " color = max(color, 0.0); if(scene_output.x<0.5) { color = color / (color + 1.0); color = pow(color, 1.0 / 2.2); }\n"
    " return float4(controls.w > 0.5 ? color * alpha : color, alpha); }\n";

/* The pixel interface is identical to scalar PNTC, including alpha/shadows.
 * Instance normal rows are inverse-transpose, so nonuniform scale is safe. */
static const wp_c8 s_vs_pntc_instanced_src[] =
    WP_VERTEX_LIGHTING_HLSL
    "cbuffer Transform : register(b0) { row_major float4x4 mvp; row_major float4x4 world;\n"
    " row_major float4x4 normal_matrix; };\n"
    "struct VS_IN { float3 pos : POSITION; float3 normal : NORMAL;\n"
    " float2 uv : TEXCOORD0; uint color : COLOR;\n"
    " float4 row0 : INSTANCE0; float4 row1 : INSTANCE1; float4 row2 : INSTANCE2;\n"
    " float4 normal0 : INSTANCE3; float4 normal1 : INSTANCE4; float4 normal2 : INSTANCE5;\n"
    " float4 tint : INSTANCE6; };\n"
    "struct VS_OUT { float4 pos : SV_POSITION; float3 world_pos : TEXCOORD0;\n"
    " float3 normal : TEXCOORD1; float2 uv : TEXCOORD2; float4 color : COLOR;\n"
    " float3 object_pos : TEXCOORD3; float3 object_normal : TEXCOORD4; float4 clip_pos : TEXCOORD5; float vertex_light:TEXCOORD6; };\n"
    "VS_OUT vs_main(VS_IN i) { VS_OUT o; float4 p = float4(i.pos, 1.0);\n"
    " float3 instance_pos = float3(dot(i.row0,p), dot(i.row1,p), dot(i.row2,p));\n"
    " float3 instance_normal = float3(dot(i.normal0.xyz,i.normal),\n"
    "  dot(i.normal1.xyz,i.normal), dot(i.normal2.xyz,i.normal));\n"
    " o.pos = mul(mvp, float4(instance_pos,1.0)); o.pos.z = 0.5*(o.pos.z+o.pos.w);\n"
    " o.world_pos = mul(world, float4(instance_pos,1.0)).xyz;\n"
    " o.normal = normalize(mul((float3x3)normal_matrix, instance_normal));\n"
    " o.uv=i.uv; o.object_pos=i.pos; o.object_normal=i.normal; o.clip_pos=o.pos;\n"
    " o.color=float4(float((i.color>>24u)&255u),float((i.color>>16u)&255u),\n"
    " float((i.color>>8u)&255u),float(i.color&255u))/255.0*i.tint; o.vertex_light=saturate(dot(o.normal,-vertex_light_direction.xyz)*rsqrt(max(dot(vertex_light_direction.xyz,vertex_light_direction.xyz),1e-8))); return o; }\n";

typedef struct wp_instance_gpu_dx11
{
    wp_f32 transform[3][4];
    wp_f32 normal[3][4];
    wp_vec4f tint;
} wp_instance_gpu_dx11;

static const wp_c8 s_vs_foliage_src[] =
    WP_VERTEX_LIGHTING_HLSL
    "cbuffer Transform : register(b0) { row_major float4x4 mvp; row_major float4x4 world; row_major float4x4 normal_matrix; };\n"
    "cbuffer Foliage : register(b4) { float4 foliage_time; float4 foliage_height; };\n"
    "struct VS_IN { float3 pos:POSITION; float3 normal:NORMAL; float2 uv:TEXCOORD0; uint color:COLOR;\n"
    " float4 row0:INSTANCE0; float4 row1:INSTANCE1; float4 row2:INSTANCE2;\n"
    " float4 normal0:INSTANCE3; float4 normal1:INSTANCE4; float4 normal2:INSTANCE5; float4 tint:INSTANCE6;\n"
    " float4 atlas:INSTANCE7; float4 wind:INSTANCE8; float4 interaction:INSTANCE9; };\n"
    "struct VS_OUT { float4 pos:SV_POSITION; float3 world_pos:TEXCOORD0; float3 normal:TEXCOORD1;\n"
    " float2 uv:TEXCOORD2; float4 color:COLOR; float3 object_pos:TEXCOORD3; float3 object_normal:TEXCOORD4; float4 clip_pos:TEXCOORD5; float vertex_light:TEXCOORD6; };\n"
    "VS_OUT vs_main(VS_IN i) { VS_OUT o; float inverseHeight=rcp(foliage_height.y-foliage_height.x);\n"
    " float weight=saturate((i.pos.y-foliage_height.x)*inverseHeight);\n"
    " float phase=foliage_time.x*foliage_time.y+i.wind.z;\n"
    " float2 bend=i.wind.xy*sin(phase)+i.interaction.xy+float2(sin(phase*2.0),cos(phase*3.0))*i.wind.w;\n"
    " float3 p=i.pos; p.xz+=bend*weight*weight; float3 n=i.normal;\n"
    " float derivative=(i.pos.y>=foliage_height.x && i.pos.y<=foliage_height.y)?2.0*weight*inverseHeight:0.0;\n"
    " n.y-=dot(n.xz,bend)*derivative; float4 local=float4(p,1);\n"
    " float3 ip=float3(dot(i.row0,local),dot(i.row1,local),dot(i.row2,local));\n"
    " float3 normal=float3(dot(i.normal0.xyz,n),dot(i.normal1.xyz,n),dot(i.normal2.xyz,n));\n"
    " o.pos=mul(mvp,float4(ip,1)); o.pos.z=0.5*(o.pos.z+o.pos.w); o.clip_pos=o.pos;\n"
    " o.world_pos=mul(world,float4(ip,1)).xyz; o.normal=normalize(mul((float3x3)normal_matrix,normal));\n"
    " o.uv=i.uv*i.atlas.zw+i.atlas.xy; o.object_pos=p; o.object_normal=normalize(n);\n"
    " o.color=float4(float((i.color>>24u)&255u),float((i.color>>16u)&255u),float((i.color>>8u)&255u),float(i.color&255u))/255.0*i.tint; o.vertex_light=saturate(dot(o.normal,-vertex_light_direction.xyz)*rsqrt(max(dot(vertex_light_direction.xyz,vertex_light_direction.xyz),1e-8))); return o; }\n";
typedef struct wp_foliage_gpu_dx11
{
    wp_instance_gpu_dx11 instance;
    wp_vec4f uv_rect, wind, interaction;
} wp_foliage_gpu_dx11;

typedef struct wp_shadow_constants_dx11
{
    wp_mat4f light_matrix;
    wp_vec4f controls;
} wp_shadow_constants_dx11;

typedef struct wp_irradiance_constants_dx11
{
    wp_vec4f controls;
    wp_vec4f probes[40];
} wp_irradiance_constants_dx11;

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

enum { WP_DX11_MESH_CACHE_BUCKETS = 1024 };

static unsigned wp_dx11_mesh_bucket( const wp_graphics_mesh *mesh )
{
    uintptr_t key = (uintptr_t)mesh;
    return (unsigned)( ( ( key >> 4 ) ^ ( key >> 20 ) ) & ( WP_DX11_MESH_CACHE_BUCKETS - 1u ) );
}

typedef struct wp_dx11_mesh_cache
{
    const wp_graphics_mesh *mesh;
    uint64_t generation;
    wp_u32 first, count;
    wp_geometry_dx11 *geometry;
    struct wp_dx11_mesh_cache *next;
} wp_dx11_mesh_cache;
static wp_renderer_dx11 *wp_dx11_renderers = NULL;
static volatile LONG64 wp_dx11_rtt_generation = 0;
static wp_s32 wp_rtt_dx11_view_allowed(wp_renderer_dx11 *, ID3D11ShaderResourceView *);

typedef struct wp_material_surface_constants_dx11
{
    wp_vec4f base_color, emissive_color, specular_color;
    wp_vec4f surface, uv_transform, controls, texture_sources, projection;
} wp_material_surface_constants_dx11;
typedef struct wp_material_view_constants_dx11
{
    wp_vec4f light_color, light_direction, camera_position;
    wp_vec4f ambient_color, environment, fog_color, fog_params;
} wp_material_view_constants_dx11;
typedef struct wp_material_binding_constants_dx11
{
    wp_vec4f map_flags, extra_map_flags;
} wp_material_binding_constants_dx11;
typedef char wp_material_surface_size_check[sizeof(wp_material_surface_constants_dx11) == 128 ? 1 : -1];
typedef char wp_material_view_size_check[sizeof(wp_material_view_constants_dx11) == 112 ? 1 : -1];
typedef char wp_material_binding_size_check[sizeof(wp_material_binding_constants_dx11) == 32 ? 1 : -1];

struct wp_renderer_dx11
{
    wp_u64 rtt_generation;
    DWORD rtt_owner_thread;
    wp_s32 rtt_active, rtt_hazard;
    wp_render_texture_dx11 *rtt_destination;
    wp_dx11_mesh_cache *meshes[WP_DX11_MESH_CACHE_BUCKETS];
    wp_renderer_dx11 *next_renderer;
    wp_render_statistics_dx11 statistics;
    wp_material_upload_statistics_dx11 material_upload_statistics;
    double frame_intervals[WP_DX11_FRAME_SAMPLES];
    double interval_total, cpu_total, present_total, gpu_total;
    LARGE_INTEGER frame_start, last_present, clock_frequency;
    wp_s32 measuring, frame_measuring;
    volatile LONG reset_statistics;
    unsigned statistics_epoch;
    wp_gpu_frame_query gpu_queries[WP_DX11_GPU_QUERY_COUNT];
    wp_s32 active_gpu_query;
    wp_s32 gpu_profiling_disabled;
    wp_s32 gpu_queries_initialized;
    wp_s32 pntc_bindings_valid;
    ID3D11Buffer *bound_vertex_buffer, *bound_index_buffer;
    ID3D11ShaderResourceView *bound_textures[8];
    ID3D11SamplerState *bound_sampler;
    /* D3D11 core objects */
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    IDXGISwapChain *swap_chain;


    /* Scene-linear HDR staging and output state, owned by the render thread. */
    ID3D11Texture2D *hdr_texture;
    ID3D11RenderTargetView *hdr_target;
    ID3D11ShaderResourceView *hdr_view;
    ID3D11VertexShader *hdr_vs;
    ID3D11PixelShader *hdr_ps;
    ID3D11Buffer *hdr_output_cb;
    ID3D11RasterizerState *hdr_raster;
    ID3D11DepthStencilState *hdr_depth;
    ID3D11BlendState *hdr_blend;
    ID3D11RenderTargetView *hdr_saved_target;
    ID3D11DepthStencilView *hdr_saved_depth;
    D3D11_VIEWPORT hdr_saved_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    D3D11_RECT hdr_saved_scissors[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    UINT hdr_viewport_count, hdr_scissor_count, hdr_width, hdr_height;
    wp_s32 hdr_active, hdr_target_srgb;
    wp_f32 hdr_exposure;
    ID3D11Buffer *project_output_cb;
    wp_vec4f project_output_values;
    ID3D11Texture2D *scene_msaa_texture, *scene_depth_texture;
    ID3D11RenderTargetView *scene_msaa_target;
    ID3D11DepthStencilView *scene_depth_view;
    wp_s32 scene_requested_samples, scene_samples;
    UINT scene_width, scene_height, hdr_output_width, hdr_output_height;
    wp_f32 scene_render_scale;
    wp_viewport_i scene_saved_viewport;
    ID3D11Texture2D *scene_history_texture;
    ID3D11ShaderResourceView *scene_history_view;
    wp_s32 scene_history_valid, project_aa, project_bloom, project_motion_blur;
    wp_u64 scene_temporal_key;
    ID3D11Buffer *irradiance_cb;
    ID3D11Buffer *vertex_light_cb;
    wp_vec4f vertex_light_values;
    wp_irradiance_constants_dx11 irradiance_values;
    wp_u64 hdr_completed_frames;
    wp_vec4f hdr_output_values;
    wp_s32 hdr_output_valid;
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
    ID3D11VertexShader *vs_pntc_instanced;
    ID3D11InputLayout *layout_pntc_instanced;
    ID3D11Buffer *instance_buffer;
    ID3D11VertexShader *vs_foliage;
    ID3D11InputLayout *layout_foliage;
    ID3D11Buffer *foliage_buffer, *cb_foliage;
    ID3D11SamplerState *pntc_sampler_state;
    ID3D11SamplerState *environment_sampler_state;
    ID3D11ShaderResourceView *environment_view; /* borrowed */
    wp_f32 environment_max_lod;
    ID3D11SamplerState *material_sampler;
    ID3D11SamplerState *material_sampler_cache[1600];
    ID3D11ShaderResourceView *material_texture_views[6];
    ID3D11Buffer *cb_terrain;
    wp_terrain_material_dx11 terrain;
    ID3D11ShaderResourceView *terrain_views[WP_TERRAIN_MATERIAL_VIEWS]; /* borrowed */
    ID3D11SamplerState *terrain_control_sampler;
    wp_s32 terrain_dirty;
    ID3D11Buffer *cb_material;
    wp_material_dx11 material;
    wp_s32 material_upload_mode;
    wp_u32 material_split_valid;
    ID3D11PixelShader *ps_pntc_split;
    ID3D11Buffer *cb_material_surface, *cb_material_view, *cb_material_bindings;
    wp_material_surface_constants_dx11 uploaded_surface;
    wp_material_view_constants_dx11 uploaded_view;
    wp_material_binding_constants_dx11 uploaded_bindings;
    ID3D11Texture2D *shadow_texture;
    ID3D11DepthStencilView *shadow_depth;
    ID3D11ShaderResourceView *shadow_view;
    ID3D11SamplerState *shadow_sampler;
    ID3D11Buffer *cb_shadow;
    wp_shadow_constants_dx11 shadow;
    wp_s32 shadow_size, shadow_pass, shadow_ready, shadow_dirty;
    wp_s32 shadow_filter_quality;
    ID3D11RenderTargetView *shadow_saved_rt;
    ID3D11DepthStencilView *shadow_saved_ds;
    wp_viewport_i shadow_saved_viewport;
    wp_s32 shadow_saved_scissor;


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
    wp_u64 generation;
    wp_render_texture_dx11 *scratch;
    ID3DDeviceContextState *rtt_state;
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
    wp_s32 vertex_stride;
    ID3D11Device *device; /* borrowed identity; the buffers retain their device */
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

static volatile LONG wp_project_renderer_mode, wp_project_shader_model, wp_project_validation, wp_project_no_fallback;
static wp_dx11_shader_compiler_callback wp_project_shader_compiler;
static void *wp_project_shader_compiler_user;
void wp_renderer_dx11_set_shader_compiler(wp_dx11_shader_compiler_callback callback,void *user)
{ wp_project_shader_compiler=callback;wp_project_shader_compiler_user=user; }

static HRESULT wp_renderer_dx11_compile_shader_options( const wp_c8 *source, const wp_c8 *entry,
    const wp_c8 *target, const D3D_SHADER_MACRO *defines, ID3DBlob **blob )
{
    ID3DBlob *errors = NULL;
    HRESULT hr;

    if(wp_project_shader_model>0)
    {
        if(!strcmp((const char*)target,"vs_4_0")) target=(const wp_c8*)"vs_5_0";
        if(!strcmp((const char*)target,"ps_4_0")) target=(const wp_c8*)"ps_5_0";
    }
    if(wp_project_shader_compiler)
    {
        char *expanded=NULL;const wp_c8 *provider_source=source;wp_s32 status;
        if(defines)
        {
            size_t bytes=strlen(source)+1;const D3D_SHADER_MACRO *d;
            for(d=defines;d->Name;++d) bytes+=strlen(d->Name)+(d->Definition?strlen(d->Definition):1)+10;
            expanded=(char*)malloc(bytes);if(!expanded) return E_OUTOFMEMORY;
            expanded[0]=0;
            for(d=defines;d->Name;++d) {strcat(expanded,"#define ");strcat(expanded,d->Name);strcat(expanded," ");strcat(expanded,d->Definition?d->Definition:"1");strcat(expanded,"\n");}
            strcat(expanded,source);provider_source=(const wp_c8*)expanded;
        }
        status=wp_project_shader_compiler(wp_project_shader_compiler_user,provider_source,entry,target,(void**)blob);
        if(expanded) free(expanded);
        if(status) return status>0&&*blob?S_OK:E_FAIL;
    }

    hr = D3DCompile( source, strlen( source ), NULL, defines, NULL, entry, target, 0, 0, blob, &errors );

    if( errors )
        ID3D10Blob_Release( errors );

    return hr;
}

void wp_renderer_dx11_set_startup_options( wp_s32 mode, wp_s32 shader_model,
                                          wp_s32 validation, wp_s32 allow_fallback )
{
    InterlockedExchange(&wp_project_renderer_mode,mode);
    InterlockedExchange(&wp_project_shader_model,shader_model);
    InterlockedExchange(&wp_project_validation,!!validation);
    InterlockedExchange(&wp_project_no_fallback,!allow_fallback);
}

static HRESULT wp_renderer_dx11_compile_shader( const wp_c8 *source, const wp_c8 *entry,
                                                const wp_c8 *target, ID3DBlob **blob )
{
    if(wp_project_shader_model>0)
    {
        if(!strcmp((const char*)target,"vs_4_0")) target=(const wp_c8*)"vs_5_0";
        if(!strcmp((const char*)target,"ps_4_0")) target=(const wp_c8*)"ps_5_0";
    }
    return wp_renderer_dx11_compile_shader_options( source, entry, target, NULL, blob );
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

    desc.FrontCounterClockwise = TRUE;
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

static wp_f32 wp_renderer_dx11_view_samples_linear( ID3D11ShaderResourceView *view );
static wp_s32 wp_renderer_dx11_upload_material_part( wp_renderer_dx11 *r, ID3D11Buffer *buffer,
    const void *value, void *uploaded, size_t bytes, wp_u32 bit, wp_u64 *uploads, wp_u64 *total_bytes )
{
    if( ( r->material_split_valid & bit ) && !memcmp( value, uploaded, bytes ) ) return 1;
    if( !wp_renderer_dx11_upload_constants( r, buffer, value, bytes ) )
    { ++r->material_upload_statistics.map_failures; return 0; }
    memcpy( uploaded, value, bytes );
    r->material_split_valid |= bit;
    ++*uploads; *total_bytes += bytes;
    return 1;
}

static wp_s32 wp_renderer_dx11_upload_material( wp_renderer_dx11 *r )
{
    wp_material_surface_constants_dx11 surface;
    wp_material_view_constants_dx11 view;
    wp_material_binding_constants_dx11 bindings;
    wp_material_upload_statistics_dx11 *stats = &r->material_upload_statistics;
    if( r->material_upload_mode == WP_DX11_MATERIAL_UPLOAD_COMBINED )
    {
        if( !wp_renderer_dx11_upload_constants( r, r->cb_material, &r->material, sizeof( r->material ) ) )
        { ++stats->map_failures; return 0; }
        ++stats->combined_uploads; stats->combined_bytes += sizeof( r->material );
        return 1;
    }
    memcpy( &surface.base_color, &r->material.base_color, sizeof(wp_vec4f) * 3 );
    memcpy( &surface.surface, &r->material.surface, sizeof(wp_vec4f) * 3 );
    memcpy( &surface.texture_sources, &r->material.texture_sources, sizeof(wp_vec4f) * 2 );
    memcpy( &view.light_color, &r->material.light_color, sizeof(wp_vec4f) * 3 );
    memcpy( &view.ambient_color, &r->material.ambient_color, sizeof(wp_vec4f) * 4 );
    memcpy( &bindings.map_flags, &r->material.map_flags, sizeof(wp_vec4f) * 2 );
    if( !wp_renderer_dx11_upload_material_part( r, r->cb_material_surface, &surface, &r->uploaded_surface,
            sizeof(surface), 1, &stats->surface_uploads, &stats->surface_bytes ) ) return 0;
    if( !wp_renderer_dx11_upload_material_part( r, r->cb_material_view, &view, &r->uploaded_view,
            sizeof(view), 2, &stats->view_uploads, &stats->view_bytes ) ) return 0;
    return wp_renderer_dx11_upload_material_part( r, r->cb_material_bindings, &bindings, &r->uploaded_bindings,
            sizeof(bindings), 4, &stats->binding_uploads, &stats->binding_bytes );
}

static void wp_renderer_dx11_bind_output( wp_renderer_dx11 *r, wp_f32 texture_linear )
{
    wp_vec4f controls;
    if( !r->hdr_output_cb ) return;
    controls.x = r->hdr_active ? 1.0f : 0.0f;
    controls.y = r->hdr_exposure;
    controls.z = texture_linear;
    controls.w = r->hdr_target_srgb ? 1.0f : 0.0f;
    if( !r->hdr_output_valid || memcmp( &controls, &r->hdr_output_values, sizeof( controls ) ) )
    {
        ID3D11DeviceContext_UpdateSubresource( r->context, (ID3D11Resource *)r->hdr_output_cb,
                                             0, NULL, &controls, 0, 0 );
        r->hdr_output_values = controls;
        r->hdr_output_valid = 1;
    }
    ID3D11DeviceContext_PSSetConstantBuffers( r->context, 5, 1, &r->hdr_output_cb );
}

static wp_s32 wp_renderer_dx11_upload_mvp( wp_renderer_dx11 *r )
{
    wp_transform_constants_dx11 constants;

    if( !r || !r->cb_transform )
        return 0;

    wp_renderer_dx11_bind_output( r, r->hdr_output_values.z );
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

/* Validate before the first draw, then populate directly into bounded mapped
 * upload storage. Double intermediates avoid overflow for finite float input. */
static wp_s32 wp_renderer_dx11_pack_instance( const wp_instance_pntc_dx11 *source,
                                             wp_instance_gpu_dx11 *destination )
{
    double cofactors[3][3], determinant, value;
    double a, b, c, d, e, f, g, h, i;
    wp_s32 row, column;
    const wp_mat4f *m = &source->transform;
    const wp_f32 tint[4] = { source->tint.x, source->tint.y, source->tint.z, source->tint.w };
    for( row = 0; row < 4; ++row )
    {
        if( !( tint[row] >= -FLT_MAX && tint[row] <= FLT_MAX ) ) return 0;
        for( column = 0; column < 4; ++column )
            if( !( m->m[row][column] >= -FLT_MAX && m->m[row][column] <= FLT_MAX ) ) return 0;
    }
    if( m->m[3][0] != 0 || m->m[3][1] != 0 || m->m[3][2] != 0 || m->m[3][3] != 1 ) return 0;
    a=m->m[0][0]; b=m->m[0][1]; c=m->m[0][2];
    d=m->m[1][0]; e=m->m[1][1]; f=m->m[1][2];
    g=m->m[2][0]; h=m->m[2][1]; i=m->m[2][2];
    cofactors[0][0]=e*i-f*h; cofactors[0][1]=f*g-d*i; cofactors[0][2]=d*h-e*g;
    cofactors[1][0]=c*h-b*i; cofactors[1][1]=a*i-c*g; cofactors[1][2]=b*g-a*h;
    cofactors[2][0]=b*f-c*e; cofactors[2][1]=c*d-a*f; cofactors[2][2]=a*e-b*d;
    determinant=a*cofactors[0][0]+b*cofactors[0][1]+c*cofactors[0][2];
    if( !( determinant > 0 ) ) return 0;
    for( row = 0; row < 3; ++row )
    {
        for( column = 0; column < 3; ++column )
        {
            value = cofactors[row][column] / determinant;
            if( !( value >= -FLT_MAX && value <= FLT_MAX ) ) return 0;
            if( destination ) destination->normal[row][column] = (wp_f32)value;
        }
        if( destination ) destination->normal[row][3] = 0;
    }
    if( destination )
    {
        memcpy( destination->transform, m->m, sizeof( destination->transform ) );
        destination->tint = source->tint;
    }
    return 1;
}

wp_s32 wp_renderer_dx11_validate_instances_pntc(
    const wp_instance_pntc_dx11 *instances, wp_s32 instance_count )
{
    wp_s32 i;
    if( instance_count < 0 || ( instance_count > 0 && !instances ) ) return 0;
    for( i = 0; i < instance_count; ++i )
        if( !wp_renderer_dx11_pack_instance( instances+i, NULL ) ) return 0;
    return 1;
}

static wp_s32 wp_renderer_dx11_ensure_instances( wp_renderer_dx11 *r )
{
    ID3DBlob *blob = NULL;
    ID3D11VertexShader *shader = NULL;
    ID3D11InputLayout *layout = NULL;
    ID3D11Buffer *buffer = NULL;
    D3D11_INPUT_ELEMENT_DESC elements[11];
    D3D11_BUFFER_DESC desc;
    HRESULT hr;
    wp_s32 i;
    if( r->instance_buffer ) return 1;
    hr = wp_renderer_dx11_compile_shader( s_vs_pntc_instanced_src, "vs_main", "vs_4_0", &blob );
    if( FAILED( hr ) ) return 0;
    hr = ID3D11Device_CreateVertexShader( r->device, ID3D10Blob_GetBufferPointer( blob ),
                                         ID3D10Blob_GetBufferSize( blob ), NULL, &shader );
    memset( elements, 0, sizeof( elements ) );
    elements[0].SemanticName="POSITION"; elements[0].Format=DXGI_FORMAT_R32G32B32_FLOAT;
    elements[1].SemanticName="NORMAL"; elements[1].Format=DXGI_FORMAT_R32G32B32_FLOAT;
    elements[1].AlignedByteOffset=12;
    elements[2].SemanticName="TEXCOORD"; elements[2].Format=DXGI_FORMAT_R32G32_FLOAT;
    elements[2].AlignedByteOffset=24;
    elements[3].SemanticName="COLOR"; elements[3].Format=DXGI_FORMAT_R32_UINT;
    elements[3].AlignedByteOffset=32;
    for( i = 4; i < 11; ++i )
    {
        elements[i].SemanticName="INSTANCE"; elements[i].SemanticIndex=(UINT)(i-4);
        elements[i].Format=DXGI_FORMAT_R32G32B32A32_FLOAT; elements[i].InputSlot=1;
        elements[i].AlignedByteOffset=(UINT)((i-4)*16);
        elements[i].InputSlotClass=D3D11_INPUT_PER_INSTANCE_DATA;
        elements[i].InstanceDataStepRate=1;
    }
    if( SUCCEEDED( hr ) )
        hr = ID3D11Device_CreateInputLayout( r->device, elements, 11,
                    ID3D10Blob_GetBufferPointer( blob ), ID3D10Blob_GetBufferSize( blob ), &layout );
    ID3D10Blob_Release( blob );
    memset( &desc, 0, sizeof( desc ) );
    desc.ByteWidth = WP_DX11_INSTANCE_CAPACITY * (UINT)sizeof( wp_instance_gpu_dx11 );
    desc.Usage = D3D11_USAGE_DYNAMIC; desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if( SUCCEEDED( hr ) ) hr = ID3D11Device_CreateBuffer( r->device, &desc, NULL, &buffer );
    if( FAILED( hr ) )
    {
        if( buffer ) ID3D11Buffer_Release( buffer );
        if( layout ) ID3D11InputLayout_Release( layout );
        if( shader ) ID3D11VertexShader_Release( shader );
        return 0;
    }
    r->vs_pntc_instanced=shader; r->layout_pntc_instanced=layout; r->instance_buffer=buffer;
    ++r->statistics.instance_buffer_creations;
    return 1;
}

static wp_s32 wp_renderer_dx11_ensure_foliage( wp_renderer_dx11 *r )
{
    ID3DBlob *blob = NULL;
    ID3D11VertexShader *shader = NULL;
    ID3D11InputLayout *layout = NULL;
    ID3D11Buffer *buffer = NULL, *constants = NULL;
    D3D11_INPUT_ELEMENT_DESC elements[14];
    D3D11_BUFFER_DESC desc;
    HRESULT hr;
    wp_s32 i;
    if( r->foliage_buffer ) return 1;
    hr = wp_renderer_dx11_compile_shader( s_vs_foliage_src, "vs_main", "vs_4_0", &blob );
    if( FAILED( hr ) ) return 0;
    hr = ID3D11Device_CreateVertexShader( r->device, ID3D10Blob_GetBufferPointer( blob ),
                                         ID3D10Blob_GetBufferSize( blob ), NULL, &shader );
    memset( elements, 0, sizeof( elements ) );
    elements[0].SemanticName="POSITION"; elements[0].Format=DXGI_FORMAT_R32G32B32_FLOAT;
    elements[1].SemanticName="NORMAL"; elements[1].Format=DXGI_FORMAT_R32G32B32_FLOAT;
    elements[1].AlignedByteOffset=12;
    elements[2].SemanticName="TEXCOORD"; elements[2].Format=DXGI_FORMAT_R32G32_FLOAT;
    elements[2].AlignedByteOffset=24;
    elements[3].SemanticName="COLOR"; elements[3].Format=DXGI_FORMAT_R32_UINT;
    elements[3].AlignedByteOffset=32;
    for( i = 4; i < 14; ++i )
    {
        elements[i].SemanticName="INSTANCE"; elements[i].SemanticIndex=(UINT)(i-4);
        elements[i].Format=DXGI_FORMAT_R32G32B32A32_FLOAT; elements[i].InputSlot=1;
        elements[i].AlignedByteOffset=(UINT)((i-4)*16);
        elements[i].InputSlotClass=D3D11_INPUT_PER_INSTANCE_DATA;
        elements[i].InstanceDataStepRate=1;
    }
    if( SUCCEEDED( hr ) )
        hr = ID3D11Device_CreateInputLayout( r->device, elements, 14,
                    ID3D10Blob_GetBufferPointer( blob ), ID3D10Blob_GetBufferSize( blob ), &layout );
    ID3D10Blob_Release( blob );
    memset( &desc, 0, sizeof( desc ) );
    desc.ByteWidth = WP_DX11_INSTANCE_CAPACITY * (UINT)sizeof( wp_foliage_gpu_dx11 );
    desc.Usage = D3D11_USAGE_DYNAMIC; desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if( SUCCEEDED( hr ) ) hr = ID3D11Device_CreateBuffer( r->device, &desc, NULL, &buffer );
    desc.ByteWidth = sizeof( wp_foliage_parameters_dx11 );
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if( SUCCEEDED( hr ) ) hr = ID3D11Device_CreateBuffer( r->device, &desc, NULL, &constants );
    if( FAILED( hr ) )
    {
        if( buffer ) ID3D11Buffer_Release( buffer ); if( constants ) ID3D11Buffer_Release( constants );
        if( layout ) ID3D11InputLayout_Release( layout );
        if( shader ) ID3D11VertexShader_Release( shader );
        return 0;
    }
    r->vs_foliage=shader; r->layout_foliage=layout; r->foliage_buffer=buffer; r->cb_foliage=constants;
    ++r->statistics.instance_buffer_creations;
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
    r->rtt_generation = (wp_u64)InterlockedIncrement64(&wp_dx11_rtt_generation);
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
        UINT project_flags=wp_project_validation ? D3D11_CREATE_DEVICE_DEBUG : 0;
        IDXGIAdapter1 *project_adapter=NULL;
        D3D_DRIVER_TYPE project_driver=wp_project_renderer_mode==3 ?
            (attempt<2 ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE) :
            (attempt<2 ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_WARP);
        if(attempt>=2 && (wp_project_no_fallback || wp_project_renderer_mode==1)) break;
        if(wp_project_renderer_mode==2 && project_driver==D3D_DRIVER_TYPE_HARDWARE)
        {
            IDXGIFactory6 *preference_factory=NULL;
            if(SUCCEEDED(CreateDXGIFactory1(&IID_IDXGIFactory6,(void**)&preference_factory)))
            {
                IDXGIFactory6_EnumAdapterByGpuPreference(preference_factory,0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                        &IID_IDXGIAdapter1,(void**)&project_adapter);
                IDXGIFactory6_Release(preference_factory);
            }
        }
        r->flip_model = prefer_flip && attempt % 2 == 0;
        sc_desc.BufferCount = r->flip_model ? 2 : 1;
        sc_desc.SwapEffect = r->flip_model ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
        sc_desc.Flags = r->flip_model ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        if( !prefer_flip && attempt % 2 != 0 ) continue;
        hr = D3D11CreateDeviceAndSwapChain( (IDXGIAdapter*)project_adapter,
                                            project_adapter ? D3D_DRIVER_TYPE_UNKNOWN : project_driver,
                                            NULL, project_flags, NULL, 0,
                                            D3D11_SDK_VERSION, &sc_desc, &r->swap_chain, &r->device,
                                            &feature_level, &r->context );
        if(hr==DXGI_ERROR_SDK_COMPONENT_MISSING && project_flags && !wp_project_no_fallback)
            hr=D3D11CreateDeviceAndSwapChain((IDXGIAdapter*)project_adapter,project_adapter ? D3D_DRIVER_TYPE_UNKNOWN : project_driver,NULL,0,NULL,0,D3D11_SDK_VERSION,
                                            &sc_desc,&r->swap_chain,&r->device,&feature_level,&r->context);
        if(project_adapter) IDXGIAdapter1_Release(project_adapter);
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

    buf_desc.ByteWidth = sizeof( wp_terrain_material_dx11 );
    hr = ID3D11Device_CreateBuffer( r->device, &buf_desc, NULL, &r->cb_terrain );
    if( FAILED( hr ) ) { wp_renderer_dx11_destroy( r ); return NULL; }
    r->terrain_dirty = 1;
    {
        D3D11_SAMPLER_DESC desc;
        memset( &desc, 0, sizeof( desc ) );
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MaxLOD = D3D11_FLOAT32_MAX;
        hr = ID3D11Device_CreateSamplerState( r->device, &desc, &r->terrain_control_sampler );
        if( FAILED( hr ) ) { wp_renderer_dx11_destroy( r ); return NULL; }
    }

    buf_desc.ByteWidth = sizeof( wp_shadow_constants_dx11 );
    hr = ID3D11Device_CreateBuffer( r->device, &buf_desc, NULL, &r->cb_shadow );
    if( FAILED( hr ) )
    {
        wp_renderer_dx11_destroy( r );
        return NULL;
    }
    r->shadow_dirty = 1;

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
    r->next_renderer = wp_dx11_renderers;
    wp_dx11_renderers = r;

    return r;
}

/* GPU tone mapping follows wp_hdr_tonemap_agx's current rational filmic policy.
 * This is not a claim to implement a complete colour-management AgX transform. */
static const char wp_hdr_composite_vs[] =
    "struct O { float4 p:SV_POSITION; }; O main(uint id:SV_VertexID) { O o;"
    " float2 uv=float2((id<<1)&2,id&2); o.p=float4(uv*float2(2,-2)+float2(-1,1),0,1); return o; }";
static const char wp_hdr_composite_ps[] =
    "cbuffer SceneOutput:register(b5) { float4 scene_output; }; Texture2D scene:register(t0);"
    "cbuffer ProjectOutput:register(b6) { float4 project_output; float4 project_dimensions; float4 project_effects; };"
    "Texture2D history:register(t1);"
    "float3 filmic(float3 x) { float white=((11.2*(.15*11.2+.05)+.004)/(11.2*(.15*11.2+.5)+.06))-.02/.3;"
    " return (((x*(.15*x+.05)+.004)/(x*(.15*x+.5)+.06))-.02/.3)/white; }"
    "float4 main(float4 p:SV_POSITION):SV_TARGET {"
    " uint sw,sh; scene.GetDimensions(sw,sh);"
    " float2 size=project_output.w>0.5 ? max(project_dimensions.xy,float2(1,1)) : float2(sw,sh);"
    " int2 pixel=min(int2(float2(sw,sh)-1),int2(p.xy*float2(sw,sh)/size));"
    " int2 hi=int2(sw,sh)-1; float3 c=max(scene.Load(int3(pixel,0)).rgb,0.0);"
    " float3 n=scene.Load(int3(clamp(pixel+int2(0,-1),int2(0,0),hi),0)).rgb;"
    " float3 s=scene.Load(int3(clamp(pixel+int2(0,1),int2(0,0),hi),0)).rgb;"
    " float3 e=scene.Load(int3(clamp(pixel+int2(1,0),int2(0,0),hi),0)).rgb;"
    " float3 w=scene.Load(int3(clamp(pixel+int2(-1,0),int2(0,0),hi),0)).rgb;"
    " if(project_effects.x>0.5 && project_effects.x<1.5) {"
    " float3 weights=float3(.2126,.7152,.0722); float lo=min(dot(c,weights),min(min(dot(n,weights),dot(s,weights)),min(dot(e,weights),dot(w,weights))));"
    " float high=max(dot(c,weights),max(max(dot(n,weights),dot(s,weights)),max(dot(e,weights),dot(w,weights))));"
    " if(high-lo>max(.05,high*.125)) c=lerp(c,(n+s+e+w)*.25,.5); }"
    " if(project_effects.w>.5 && (project_effects.z>.5 || (project_effects.x>1.5 && project_effects.x<2.5))) {"
    " float3 previous=history.Load(int3(pixel,0)).rgb;"
    " float3 lower=min(c,min(min(n,s),min(e,w))),upper=max(c,max(max(n,s),max(e,w)));"
    " if(project_effects.z<.5) previous=clamp(previous,lower,upper);"
    " c=lerp(c,previous,project_effects.z>.5 ? .25 : .8); }"
    " if(project_effects.y>.5) { float3 glow=0;"
    " [unroll] for(int by=-2;by<=2;++by) [unroll] for(int bx=-2;bx<=2;++bx)"
    " glow+=max(scene.Load(int3(clamp(pixel+int2(bx,by)*2,int2(0,0),hi),0)).rgb-1,0);"
    " c+=glow*(.08/25); } c*=scene_output.y;"
    " float mode=project_output.w>0.5 ? project_output.z : 1.0;"
    " if(project_output.w>0.5 && project_output.x<0.5) mode=0.0;"
    " if(mode>2.5) c=filmic(c);"
    " else if(mode>1.5 && mode<2.5) c=c/(1+c);"
    " else if(mode>0.5) c=(c*(2.51*c+0.03))/(c*(2.43*c+0.59)+0.14);"
    " c=saturate(c);"
    " if(scene_output.w<0.5 && (project_output.w<0.5 || project_output.y>0.5))"
    " c=lerp(12.92*c,1.055*pow(c,1.0/2.4)-0.055,step(0.0031308,c));"
    " return float4(c,1); }";

static wp_s32 wp_renderer_dx11_prepare_hdr( wp_renderer_dx11 *r, UINT width, UINT height )
{
    ID3D11Texture2D *texture = NULL;
    ID3D11RenderTargetView *target = NULL;
    ID3D11ShaderResourceView *view = NULL;
    ID3D11Texture2D *history_texture=NULL;
    ID3D11ShaderResourceView *history_view=NULL;
    ID3D11VertexShader *vs = NULL;
    ID3D11PixelShader *ps = NULL;
    ID3D11Buffer *cb = NULL;
    ID3D11RasterizerState *raster = NULL;
    ID3D11DepthStencilState *depth = NULL;
    ID3D11BlendState *blend = NULL;
    ID3DBlob *blob = NULL;
    D3D11_TEXTURE2D_DESC td;
    D3D11_BUFFER_DESC bd;
    D3D11_RASTERIZER_DESC rd;
    D3D11_DEPTH_STENCIL_DESC dd;
    D3D11_BLEND_DESC blend_desc;
    UINT support = 0;
    wp_s32 replace_target = !r->hdr_texture || r->hdr_width != width || r->hdr_height != height;
    if( FAILED( ID3D11Device_CheckFormatSupport( r->device, DXGI_FORMAT_R16G16B16A16_FLOAT, &support ) ) ||
        ( support & ( D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_SHADER_LOAD ) ) !=
        ( D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_SHADER_LOAD ) ) return 0;
    if( replace_target )
    {
        memset( &td, 0, sizeof( td ) );
        td.Width = width; td.Height = height; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if( FAILED( ID3D11Device_CreateTexture2D( r->device, &td, NULL, &texture ) ) ||
            FAILED( ID3D11Device_CreateRenderTargetView( r->device, (ID3D11Resource *)texture, NULL, &target ) ) ||
            FAILED( ID3D11Device_CreateShaderResourceView( r->device, (ID3D11Resource *)texture, NULL, &view ) ) ) goto fail;
        td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        if(FAILED(ID3D11Device_CreateTexture2D(r->device,&td,NULL,&history_texture)) ||
           FAILED(ID3D11Device_CreateShaderResourceView(r->device,(ID3D11Resource*)history_texture,NULL,&history_view))) goto fail;
    }
    if( !r->hdr_vs )
    {
        if( FAILED( wp_renderer_dx11_compile_shader( wp_hdr_composite_vs, "main", "vs_4_0", &blob ) ) ) goto fail;
        if( FAILED( ID3D11Device_CreateVertexShader( r->device, ID3D10Blob_GetBufferPointer( blob ),
                            ID3D10Blob_GetBufferSize( blob ), NULL, &vs ) ) ) goto fail;
        ID3D10Blob_Release( blob ); blob = NULL;
        if( FAILED( wp_renderer_dx11_compile_shader( wp_hdr_composite_ps, "main", "ps_4_0", &blob ) ) ) goto fail;
        if( FAILED( ID3D11Device_CreatePixelShader( r->device, ID3D10Blob_GetBufferPointer( blob ),
                            ID3D10Blob_GetBufferSize( blob ), NULL, &ps ) ) ) goto fail;
        ID3D10Blob_Release( blob ); blob = NULL;
        memset( &bd, 0, sizeof( bd ) ); bd.ByteWidth = sizeof( wp_vec4f );
        bd.Usage = D3D11_USAGE_DEFAULT; bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if( FAILED( ID3D11Device_CreateBuffer( r->device, &bd, NULL, &cb ) ) ) goto fail;
        memset( &rd, 0, sizeof( rd ) ); rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE; rd.ScissorEnable = TRUE;
        if( FAILED( ID3D11Device_CreateRasterizerState( r->device, &rd, &raster ) ) ) goto fail;
        memset( &dd, 0, sizeof( dd ) ); dd.DepthFunc = D3D11_COMPARISON_ALWAYS;
        if( FAILED( ID3D11Device_CreateDepthStencilState( r->device, &dd, &depth ) ) ) goto fail;
        memset( &blend_desc, 0, sizeof( blend_desc ) );
        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if( FAILED( ID3D11Device_CreateBlendState( r->device, &blend_desc, &blend ) ) ) goto fail;
    }
    /* Commit only a complete candidate. Failed resize leaves the last usable set. */
    if( replace_target )
    {
        if( r->hdr_view ) ID3D11ShaderResourceView_Release( r->hdr_view );
        if( r->hdr_target ) ID3D11RenderTargetView_Release( r->hdr_target );
        if( r->hdr_texture ) ID3D11Texture2D_Release( r->hdr_texture );
        r->hdr_texture = texture; r->hdr_target = target; r->hdr_view = view;
        r->hdr_width = width; r->hdr_height = height;
        if(r->scene_history_view) ID3D11ShaderResourceView_Release(r->scene_history_view);
        if(r->scene_history_texture) ID3D11Texture2D_Release(r->scene_history_texture);
        r->scene_history_view=history_view;r->scene_history_texture=history_texture;r->scene_history_valid=0;
    }
    if( vs )
    {
        r->hdr_vs = vs; r->hdr_ps = ps; r->hdr_output_cb = cb;
        r->hdr_raster = raster; r->hdr_depth = depth; r->hdr_blend = blend;
    }
    return 1;
fail:
    if( blob ) ID3D10Blob_Release( blob );
    if( texture ) ID3D11Texture2D_Release( texture );
    if( target ) ID3D11RenderTargetView_Release( target );
    if( view ) ID3D11ShaderResourceView_Release( view );
    if(history_view) ID3D11ShaderResourceView_Release(history_view);
    if(history_texture) ID3D11Texture2D_Release(history_texture);
    if( vs ) ID3D11VertexShader_Release( vs );
    if( ps ) ID3D11PixelShader_Release( ps );
    if( cb ) ID3D11Buffer_Release( cb );
    if( raster ) ID3D11RasterizerState_Release( raster );
    if( depth ) ID3D11DepthStencilState_Release( depth );
    if( blend ) ID3D11BlendState_Release( blend );
    return 0;
}

wp_u32 wp_renderer_dx11_supported_scene_samples( wp_renderer_dx11 *r )
{
    wp_u32 mask = 1u;
    UINT samples, color, depth;
    if(!r || !r->device) return 0;
    for(samples=2; samples<=8; samples*=2)
    {
        color=depth=0;
        if(SUCCEEDED(ID3D11Device_CheckMultisampleQualityLevels(r->device,DXGI_FORMAT_R16G16B16A16_FLOAT,samples,&color)) && color &&
           SUCCEEDED(ID3D11Device_CheckMultisampleQualityLevels(r->device,DXGI_FORMAT_D24_UNORM_S8_UINT,samples,&depth)) && depth) mask |= samples;
    }
    return mask;
}

void wp_renderer_dx11_set_scene_sampling( wp_renderer_dx11 *r, wp_s32 samples, wp_f32 scale )
{
    if(!r) return;
    r->scene_requested_samples = samples == 2 || samples == 4 || samples == 8 ? samples : 1;
    r->scene_render_scale = _finite(scale) ? (scale < .25f ? .25f : scale > 2.0f ? 2.0f : scale) : 1.0f;
}

void wp_renderer_dx11_set_post_options( wp_renderer_dx11 *r, wp_s32 aa, wp_s32 bloom,
                                       wp_s32 motion_blur, wp_u64 temporal_key )
{
    if(!r) return;
    r->project_aa=aa;r->project_bloom=!!bloom;r->project_motion_blur=!!motion_blur;
    if(r->scene_temporal_key!=temporal_key) r->scene_history_valid=0;
    r->scene_temporal_key=temporal_key;
}

wp_s32 wp_renderer_dx11_set_irradiance_probes( wp_renderer_dx11 *r,
                                             const void *probes, wp_u32 count, wp_s32 mode )
{
    wp_irradiance_constants_dx11 values;
    D3D11_BUFFER_DESC description;
    wp_u32 i;
    const wp_f32 *input=(const wp_f32*)probes;
    if(!r || !r->device || !r->context || count>4 || (count && !probes)) return 0;
    for(i=0;i<count*40u;++i) if(!_finite(input[i])) return 0;
    memset(&values,0,sizeof(values));values.controls.x=(wp_f32)count;values.controls.y=(wp_f32)mode;
    if(count) memcpy(values.probes,probes,count*40u*sizeof(wp_f32));
    if(!r->irradiance_cb)
    {
        if(!count || !mode) return 1;
        memset(&description,0,sizeof(description));description.ByteWidth=sizeof(values);
        description.Usage=D3D11_USAGE_DEFAULT;description.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        if(FAILED(ID3D11Device_CreateBuffer(r->device,&description,NULL,&r->irradiance_cb))) return 0;
    }
    if(memcmp(&values,&r->irradiance_values,sizeof(values)))
    {
        ID3D11DeviceContext_UpdateSubresource(r->context,(ID3D11Resource*)r->irradiance_cb,0,NULL,&values,0,0);
        r->irradiance_values=values;r->pntc_bindings_valid=0;
    }
    return 1;
}

static wp_s32 wp_renderer_dx11_prepare_scene_sampling( wp_renderer_dx11 *r, UINT width, UINT height )
{
    D3D11_TEXTURE2D_DESC td;
    D3D11_DEPTH_STENCIL_VIEW_DESC dd;
    ID3D11Texture2D *color=NULL,*depth=NULL;
    ID3D11RenderTargetView *target=NULL;
    ID3D11DepthStencilView *depth_view=NULL;
    wp_s32 samples=r->scene_requested_samples ? r->scene_requested_samples : 1;
    if(r->scene_width==width && r->scene_height==height && r->scene_samples==samples && r->scene_depth_view) return 1;
    if(!(wp_renderer_dx11_supported_scene_samples(r) & samples)) return 0;
    memset(&td,0,sizeof(td));
    td.Width=width;td.Height=height;td.MipLevels=1;td.ArraySize=1;
    td.SampleDesc.Count=(UINT)samples;td.Usage=D3D11_USAGE_DEFAULT;
    if(samples>1)
    {
        td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;td.BindFlags=D3D11_BIND_RENDER_TARGET;
        if(FAILED(ID3D11Device_CreateTexture2D(r->device,&td,NULL,&color)) ||
           FAILED(ID3D11Device_CreateRenderTargetView(r->device,(ID3D11Resource*)color,NULL,&target))) goto fail;
    }
    td.Format=DXGI_FORMAT_R24G8_TYPELESS;td.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
    memset(&dd,0,sizeof(dd));dd.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;
    dd.ViewDimension=samples>1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
    if(FAILED(ID3D11Device_CreateTexture2D(r->device,&td,NULL,&depth)) ||
       FAILED(ID3D11Device_CreateDepthStencilView(r->device,(ID3D11Resource*)depth,&dd,&depth_view))) goto fail;
    if(r->scene_msaa_target) ID3D11RenderTargetView_Release(r->scene_msaa_target);
    if(r->scene_msaa_texture) ID3D11Texture2D_Release(r->scene_msaa_texture);
    if(r->scene_depth_view) ID3D11DepthStencilView_Release(r->scene_depth_view);
    if(r->scene_depth_texture) ID3D11Texture2D_Release(r->scene_depth_texture);
    r->scene_msaa_target=target;r->scene_msaa_texture=color;
    r->scene_depth_view=depth_view;r->scene_depth_texture=depth;
    r->scene_width=width;r->scene_height=height;r->scene_samples=samples;
    return 1;
fail:
    if(target) ID3D11RenderTargetView_Release(target);
    if(color) ID3D11Texture2D_Release(color);
    if(depth_view) ID3D11DepthStencilView_Release(depth_view);
    if(depth) ID3D11Texture2D_Release(depth);
    return 0;
}

void *wp_renderer_dx11_get_scene_depth_texture( wp_renderer_dx11 *r )
{
    return r ? r->scene_depth_texture : NULL;
}

wp_s32 wp_renderer_dx11_begin_hdr_scene( wp_renderer_dx11 *r, wp_f32 exposure )
{
    ID3D11RenderTargetView *targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView *depth = NULL;
    ID3D11Resource *resource = NULL;
    ID3D11Texture2D *texture = NULL;
    D3D11_RENDER_TARGET_VIEW_DESC vd;
    D3D11_TEXTURE2D_DESC td;
    ID3D11ShaderResourceView *bound_views[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11ShaderResourceView *bound_view = NULL;
    ID3D11Resource *bound_resource = NULL;
    UINT i;
    wp_s32 valid = 1;
    const float black[4] = { 0, 0, 0, 0 };
    if( !r || r->hdr_active || r->shadow_pass || !_finite( exposure ) || exposure <= 0.0f ) return 0;
    memset( targets, 0, sizeof( targets ) );
    ID3D11DeviceContext_OMGetRenderTargets( r->context, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, &depth );
    if( !targets[0] ) valid = 0;
    for( i = 1; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i ) if( targets[i] ) valid = 0;
    if( valid )
    {
        ID3D11RenderTargetView_GetDesc( targets[0], &vd );
        if( vd.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE2D || vd.Texture2D.MipSlice != 0 ||
            ( vd.Format != DXGI_FORMAT_R8G8B8A8_UNORM && vd.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB &&
              vd.Format != DXGI_FORMAT_B8G8R8A8_UNORM && vd.Format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB ) ) valid = 0;
        ID3D11RenderTargetView_GetResource( targets[0], &resource );
        if( !resource || FAILED( ID3D11Resource_QueryInterface( resource, &IID_ID3D11Texture2D, (void **)&texture ) ) ) valid = 0;
        if( texture )
        {
            ID3D11Texture2D_GetDesc( texture, &td );
            if( td.ArraySize != 1 || td.SampleDesc.Count != 1 || !td.Width || !td.Height ) valid = 0;
        }
    }
    if( valid && depth )
    {
        ID3D11Resource *depth_resource = NULL;
        ID3D11Texture2D *depth_texture = NULL;
        D3D11_TEXTURE2D_DESC depth_desc;
        D3D11_DEPTH_STENCIL_VIEW_DESC depth_view;
        ID3D11DepthStencilView_GetDesc( depth, &depth_view );
        if( depth_view.ViewDimension != D3D11_DSV_DIMENSION_TEXTURE2D || depth_view.Texture2D.MipSlice != 0 ) valid = 0;
        ID3D11DepthStencilView_GetResource( depth, &depth_resource );
        if( !depth_resource || FAILED( ID3D11Resource_QueryInterface( depth_resource,
            &IID_ID3D11Texture2D, (void **)&depth_texture ) ) ) valid = 0;
        if( depth_texture )
        {
            ID3D11Texture2D_GetDesc( depth_texture, &depth_desc );
            if( depth_desc.Width != td.Width || depth_desc.Height != td.Height ||
                depth_desc.SampleDesc.Count != 1 || depth_desc.ArraySize != 1 ) valid = 0;
            ID3D11Texture2D_Release( depth_texture );
        }
        if( depth_resource ) ID3D11Resource_Release( depth_resource );
    }
    /* A still-bound borrowed HDR view must not be silently displaced by OM binding. */
    if( valid && r->hdr_texture )
    {
        ID3D11DeviceContext_PSGetShaderResources( r->context, 0,
            D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, bound_views );
        for( i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i )
        {
            bound_view = bound_views[i];
            if( bound_view )
            {
                ID3D11ShaderResourceView_GetResource( bound_view, &bound_resource );
                if( bound_resource == (ID3D11Resource *)r->hdr_texture ) valid = 0;
                if( bound_resource ) ID3D11Resource_Release( bound_resource );
                ID3D11ShaderResourceView_Release( bound_view );
                bound_view = NULL; bound_resource = NULL;
            }
        }
    }
    if( valid )
    {
        wp_f32 scale=r->scene_render_scale > 0 ? r->scene_render_scale : 1.0f;
        UINT width=(UINT)(td.Width*scale),height=(UINT)(td.Height*scale);
        if(!width) width=1;if(!height) height=1;
        r->hdr_output_width=td.Width;r->hdr_output_height=td.Height;
        valid=wp_renderer_dx11_prepare_hdr(r,width,height) && wp_renderer_dx11_prepare_scene_sampling(r,width,height);
    }
    if( texture ) ID3D11Texture2D_Release( texture );
    if( resource ) ID3D11Resource_Release( resource );
    if( !valid )
    {
        for( i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i )
            if( targets[i] ) ID3D11RenderTargetView_Release( targets[i] );
        if( depth ) ID3D11DepthStencilView_Release( depth );
        return 0;
    }
    r->hdr_saved_target = targets[0]; r->hdr_saved_depth = depth;
    r->hdr_viewport_count = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    r->hdr_scissor_count = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D11DeviceContext_RSGetViewports( r->context, &r->hdr_viewport_count, r->hdr_saved_viewports );
    ID3D11DeviceContext_RSGetScissorRects( r->context, &r->hdr_scissor_count, r->hdr_saved_scissors );
    r->hdr_target_srgb = vd.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || vd.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    r->hdr_exposure = exposure < .01f ? .01f : ( exposure > 64.0f ? 64.0f : exposure );
    r->hdr_active = 1;
    r->active_rt_view = r->scene_samples>1 ? r->scene_msaa_target : r->hdr_target;
    r->active_ds_view = r->scene_depth_view;
    r->scene_saved_viewport=r->viewport;
    r->viewport.x=(wp_s32)((double)r->viewport.x*r->hdr_width/r->hdr_output_width);
    r->viewport.y=(wp_s32)((double)r->viewport.y*r->hdr_height/r->hdr_output_height);
    r->viewport.width=(wp_s32)((double)r->viewport.width*r->hdr_width/r->hdr_output_width);
    r->viewport.height=(wp_s32)((double)r->viewport.height*r->hdr_height/r->hdr_output_height);
    ID3D11DeviceContext_OMSetRenderTargets( r->context, 1, &r->active_rt_view, r->active_ds_view );
    ID3D11DeviceContext_ClearRenderTargetView( r->context, r->active_rt_view, black );
    ID3D11DeviceContext_ClearDepthStencilView(r->context,r->active_ds_view,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,1.0f,0);
    {
        D3D11_VIEWPORT viewport;
        D3D11_RECT rectangle;
        viewport.TopLeftX=(FLOAT)r->viewport.x;viewport.TopLeftY=(FLOAT)r->viewport.y;
        viewport.Width=(FLOAT)r->viewport.width;viewport.Height=(FLOAT)r->viewport.height;
        viewport.MinDepth=0;viewport.MaxDepth=1;
        rectangle.left=r->viewport.x;rectangle.top=r->viewport.y;
        rectangle.right=r->viewport.x+r->viewport.width;rectangle.bottom=r->viewport.y+r->viewport.height;
        ID3D11DeviceContext_RSSetViewports(r->context,1,&viewport);
        ID3D11DeviceContext_RSSetScissorRects(r->context,1,&rectangle);
    }
    if(r->project_output_values.w>0.5f)
        wp_renderer_dx11_set_output_options(r,(wp_s32)r->project_output_values.x,
                                            (wp_s32)r->project_output_values.y,(wp_s32)r->project_output_values.z);
    wp_renderer_dx11_bind_output( r, 0 );
    r->pntc_bindings_valid = 0;
    return 1;
}

void wp_renderer_dx11_set_output_options( wp_renderer_dx11 *r, wp_s32 hdr_enabled,
                                         wp_s32 linear_workflow, wp_s32 tonemapping )
{
    D3D11_BUFFER_DESC description;
    wp_vec4f options, constants[3];
    if( !r || !r->device || !r->context ) return;
    options.x = hdr_enabled ? 1.0f : 0.0f;
    options.y = linear_workflow ? 1.0f : 0.0f;
    options.z = (wp_f32)(tonemapping >= 0 && tonemapping <= 3 ? tonemapping : 3);
    options.w = 1.0f;
    if( !r->project_output_cb )
    {
        memset( &description, 0, sizeof(description) );
        description.ByteWidth = sizeof(constants);
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if( FAILED( ID3D11Device_CreateBuffer( r->device, &description, NULL, &r->project_output_cb ) ) ) return;
        memset( &r->project_output_values, 0, sizeof(r->project_output_values) );
    }
    {
        constants[0]=options;
        constants[1].x=(wp_f32)r->hdr_output_width;constants[1].y=(wp_f32)r->hdr_output_height;
        constants[1].z=(wp_f32)r->hdr_width;constants[1].w=(wp_f32)r->hdr_height;
        constants[2].x=(wp_f32)r->project_aa;constants[2].y=(wp_f32)r->project_bloom;
        constants[2].z=(wp_f32)r->project_motion_blur;constants[2].w=(wp_f32)r->scene_history_valid;
        ID3D11DeviceContext_UpdateSubresource( r->context, (ID3D11Resource*)r->project_output_cb,
                                             0, NULL, constants, 0, 0 );
        r->project_output_values = options;
    }
    ID3D11DeviceContext_PSSetConstantBuffers( r->context, 6, 1, &r->project_output_cb );
}

void wp_renderer_dx11_end_hdr_scene( wp_renderer_dx11 *r )
{
    ID3D11RasterizerState *raster = NULL;
    ID3D11DepthStencilState *depth = NULL;
    ID3D11BlendState *blend = NULL;
    ID3D11VertexShader *vs = NULL;
    ID3D11PixelShader *ps = NULL;
    ID3D11InputLayout *layout = NULL;
    ID3D11ShaderResourceView *view = NULL, *empty_view = NULL;
    ID3D11ShaderResourceView *old_history=NULL;
    ID3D11Buffer *old_project_cb=NULL;
    D3D11_PRIMITIVE_TOPOLOGY topology;
    UINT stencil = 0, mask = 0;
    float factors[4];
    D3D11_RECT full;
    if( !r || !r->hdr_active ) return;
    if( r->shadow_pass ) wp_renderer_dx11_end_shadow_map( r );
    ID3D11DeviceContext_RSGetState( r->context, &raster );
    ID3D11DeviceContext_OMGetDepthStencilState( r->context, &depth, &stencil );
    ID3D11DeviceContext_OMGetBlendState( r->context, &blend, factors, &mask );
    ID3D11DeviceContext_VSGetShader( r->context, &vs, NULL, NULL );
    ID3D11DeviceContext_PSGetShader( r->context, &ps, NULL, NULL );
    ID3D11DeviceContext_IAGetInputLayout( r->context, &layout );
    ID3D11DeviceContext_IAGetPrimitiveTopology( r->context, &topology );
    ID3D11DeviceContext_PSGetShaderResources( r->context, 0, 1, &view );
    ID3D11DeviceContext_PSGetShaderResources(r->context,1,1,&old_history);
    ID3D11DeviceContext_PSGetConstantBuffers(r->context,6,1,&old_project_cb);
    r->hdr_active = 0;
    r->viewport=r->scene_saved_viewport;
    if(r->scene_samples>1)
    {
        ID3D11DeviceContext_OMSetRenderTargets(r->context,0,NULL,NULL);
        ID3D11DeviceContext_ResolveSubresource(r->context,(ID3D11Resource*)r->hdr_texture,0,
                                              (ID3D11Resource*)r->scene_msaa_texture,0,DXGI_FORMAT_R16G16B16A16_FLOAT);
    }
    r->active_rt_view = r->hdr_saved_target; r->active_ds_view = r->hdr_saved_depth;
    ID3D11DeviceContext_OMSetRenderTargets( r->context, 1, &r->hdr_saved_target, r->hdr_saved_depth );
    ID3D11DeviceContext_RSSetViewports( r->context, r->hdr_viewport_count, r->hdr_saved_viewports );
    full.left = 0; full.top = 0; full.right = (LONG)r->hdr_output_width; full.bottom = (LONG)r->hdr_output_height;
    ID3D11DeviceContext_RSSetScissorRects( r->context, 1, &full );
    ID3D11DeviceContext_RSSetState( r->context, r->hdr_raster );
    ID3D11DeviceContext_OMSetDepthStencilState( r->context, r->hdr_depth, 0 );
    ID3D11DeviceContext_OMSetBlendState( r->context, r->hdr_blend, NULL, 0xffffffffu );
    wp_renderer_dx11_bind_output( r, 1 );
    ID3D11DeviceContext_PSSetConstantBuffers(r->context,6,1,&r->project_output_cb);
    ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &r->hdr_view );
    ID3D11DeviceContext_PSSetShaderResources(r->context,1,1,&r->scene_history_view);
    ID3D11DeviceContext_IASetInputLayout( r->context, NULL );
    ID3D11DeviceContext_IASetPrimitiveTopology( r->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    ID3D11DeviceContext_VSSetShader( r->context, r->hdr_vs, NULL, 0 );
    ID3D11DeviceContext_PSSetShader( r->context, r->hdr_ps, NULL, 0 );
    ID3D11DeviceContext_Draw( r->context, 3, 0 );
    ++r->statistics.draws; ++r->statistics.triangles;
    ++r->hdr_completed_frames;
    ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &empty_view );
    ID3D11DeviceContext_PSSetShaderResources(r->context,1,1,&empty_view);
    if(r->scene_history_texture)
    {
        ID3D11DeviceContext_CopyResource(r->context,(ID3D11Resource*)r->scene_history_texture,(ID3D11Resource*)r->hdr_texture);
        r->scene_history_valid=1;
    }
    ID3D11DeviceContext_PSSetShaderResources(r->context,1,1,&old_history);
    ID3D11DeviceContext_PSSetConstantBuffers(r->context,6,1,&old_project_cb);
    ID3D11DeviceContext_PSSetShaderResources( r->context, 0, 1, &view );
    ID3D11DeviceContext_RSSetState( r->context, raster );
    ID3D11DeviceContext_OMSetDepthStencilState( r->context, depth, stencil );
    ID3D11DeviceContext_OMSetBlendState( r->context, blend, factors, mask );
    ID3D11DeviceContext_RSSetScissorRects( r->context, r->hdr_scissor_count, r->hdr_saved_scissors );
    ID3D11DeviceContext_VSSetShader( r->context, vs, NULL, 0 );
    ID3D11DeviceContext_PSSetShader( r->context, ps, NULL, 0 );
    ID3D11DeviceContext_IASetInputLayout( r->context, layout );
    ID3D11DeviceContext_IASetPrimitiveTopology( r->context, topology );
    if( raster ) ID3D11RasterizerState_Release( raster );
    if( depth ) ID3D11DepthStencilState_Release( depth );
    if( blend ) ID3D11BlendState_Release( blend );
    if( vs ) ID3D11VertexShader_Release( vs );
    if( ps ) ID3D11PixelShader_Release( ps );
    if( layout ) ID3D11InputLayout_Release( layout );
    if( view ) ID3D11ShaderResourceView_Release( view );
    if(old_history) ID3D11ShaderResourceView_Release(old_history);
    if(old_project_cb) ID3D11Buffer_Release(old_project_cb);
    ID3D11RenderTargetView_Release( r->hdr_saved_target ); r->hdr_saved_target = NULL;
    if( r->hdr_saved_depth ) ID3D11DepthStencilView_Release( r->hdr_saved_depth );
    r->hdr_saved_depth = NULL;
    r->pntc_bindings_valid = 0;
    r->rs_dirty = r->bs_dirty = r->dss_dirty = 1;
    wp_renderer_dx11_bind_output( r, 0 );
}

wp_s32 wp_renderer_dx11_is_hdr_scene_active( const wp_renderer_dx11 *r )
{
    return r ? r->hdr_active : 0;
}
wp_u64 wp_renderer_dx11_get_hdr_completed_frames( const wp_renderer_dx11 *r )
{
    return r ? r->hdr_completed_frames : 0;
}
void *wp_renderer_dx11_get_hdr_scene_view( const wp_renderer_dx11 *r )
{
    return r ? r->hdr_view : NULL;
}

void wp_renderer_dx11_destroy( wp_renderer_dx11 *r )
{
    wp_s32 i;
    wp_renderer_dx11 **owner;
    wp_dx11_mesh_cache *entry, *next;
    if( !r )
        return;
    for( owner = &wp_dx11_renderers; *owner; owner = &( *owner )->next_renderer )
    {
        if( *owner == r ) { *owner = r->next_renderer; break; }
    }
    for( i = 0; i < WP_DX11_MESH_CACHE_BUCKETS; ++i )
    {
        for( entry = r->meshes[i]; entry; entry = next )
        {
            next = entry->next;
            wp_renderer_dx11_destroy_geometry( entry->geometry );
            free( entry );
        }
    }

    if( r->context )
        ID3D11DeviceContext_ClearState( r->context );
    if( r->hdr_saved_target ) ID3D11RenderTargetView_Release( r->hdr_saved_target );
    if( r->hdr_saved_depth ) ID3D11DepthStencilView_Release( r->hdr_saved_depth );
    if( r->hdr_view ) ID3D11ShaderResourceView_Release( r->hdr_view );
    if( r->hdr_target ) ID3D11RenderTargetView_Release( r->hdr_target );
    if( r->hdr_texture ) ID3D11Texture2D_Release( r->hdr_texture );
    if( r->hdr_vs ) ID3D11VertexShader_Release( r->hdr_vs );
    if( r->hdr_ps ) ID3D11PixelShader_Release( r->hdr_ps );
    if( r->hdr_output_cb ) ID3D11Buffer_Release( r->hdr_output_cb );
    if( r->project_output_cb ) ID3D11Buffer_Release( r->project_output_cb );
    if(r->scene_msaa_target) ID3D11RenderTargetView_Release(r->scene_msaa_target);
    if(r->scene_msaa_texture) ID3D11Texture2D_Release(r->scene_msaa_texture);
    if(r->scene_depth_view) ID3D11DepthStencilView_Release(r->scene_depth_view);
    if(r->scene_depth_texture) ID3D11Texture2D_Release(r->scene_depth_texture);
    if(r->scene_history_view) ID3D11ShaderResourceView_Release(r->scene_history_view);
    if(r->scene_history_texture) ID3D11Texture2D_Release(r->scene_history_texture);
    if(r->irradiance_cb) ID3D11Buffer_Release(r->irradiance_cb);
    if(r->vertex_light_cb) ID3D11Buffer_Release(r->vertex_light_cb);
    if( r->hdr_raster ) ID3D11RasterizerState_Release( r->hdr_raster );
    if( r->hdr_depth ) ID3D11DepthStencilState_Release( r->hdr_depth );
    if( r->hdr_blend ) ID3D11BlendState_Release( r->hdr_blend );

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
    if( r->cb_terrain ) ID3D11Buffer_Release( r->cb_terrain );
    if( r->terrain_control_sampler ) ID3D11SamplerState_Release( r->terrain_control_sampler );
    if( r->cb_material )
        ID3D11Buffer_Release( r->cb_material );
    if( r->cb_material_surface ) ID3D11Buffer_Release( r->cb_material_surface );
    if( r->cb_material_view ) ID3D11Buffer_Release( r->cb_material_view );
    if( r->cb_material_bindings ) ID3D11Buffer_Release( r->cb_material_bindings );
    if( r->ps_pntc_split ) ID3D11PixelShader_Release( r->ps_pntc_split );
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
    if( r->shadow_texture ) ID3D11Texture2D_Release( r->shadow_texture );
    if( r->shadow_depth ) ID3D11DepthStencilView_Release( r->shadow_depth );
    if( r->shadow_view ) ID3D11ShaderResourceView_Release( r->shadow_view );
    if( r->shadow_sampler ) ID3D11SamplerState_Release( r->shadow_sampler );
    if( r->cb_shadow ) ID3D11Buffer_Release( r->cb_shadow );

    if( r->layout_pntc )
        ID3D11InputLayout_Release( r->layout_pntc );
    if( r->layout_pntc_instanced ) ID3D11InputLayout_Release( r->layout_pntc_instanced );
    if( r->vs_pntc_instanced ) ID3D11VertexShader_Release( r->vs_pntc_instanced );
    if( r->instance_buffer ) ID3D11Buffer_Release( r->instance_buffer );
    if( r->layout_foliage ) ID3D11InputLayout_Release( r->layout_foliage );
    if( r->vs_foliage ) ID3D11VertexShader_Release( r->vs_foliage );
    if( r->foliage_buffer ) ID3D11Buffer_Release( r->foliage_buffer );
    if( r->cb_foliage ) ID3D11Buffer_Release( r->cb_foliage );
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

    /* Active HDR/shadow passes retain saved output state. Resizing here would
     * disrupt their bindings and can fail on retained swap-chain references. */
    if( !r || r->rtt_active || r->hdr_active || r->shadow_pass ||
        !wp_renderer_dx11_dimensions_supported( r, width, height ) )
        return 0;

    if( width == r->width && height == r->height && r->rt_view && r->ds_view ) return 1;

    old_width = r->width;
    old_height = r->height;
    ID3D11DeviceContext_OMSetRenderTargets( r->context, 0, NULL, NULL );
    wp_renderer_dx11_release_targets( r );

    hr = IDXGISwapChain_ResizeBuffers( r->swap_chain, 0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN,
                                       r->swap_chain_flags );
    if( FAILED( hr ) )
    {
        /* ResizeBuffers leaves the old swap-chain buffers intact on failure. */
        if( wp_renderer_dx11_create_targets( r ) )
            ID3D11DeviceContext_OMSetRenderTargets( r->context, 1, &r->active_rt_view,
                                                    r->active_ds_view );
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
            if( wp_renderer_dx11_create_targets( r ) )
                ID3D11DeviceContext_OMSetRenderTargets( r->context, 1, &r->active_rt_view,
                                                        r->active_ds_view );
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

wp_s32 wp_renderer_dx11_get_material_upload_statistics( const wp_renderer_dx11 *r,
    wp_material_upload_statistics_dx11 *out )
{
    if( !r || !out || out->struct_size < sizeof( *out ) ) return 0;
    *out = r->material_upload_statistics;
    out->struct_size = sizeof( *out );
    return 1;
}

static int compare_frame_interval( const void *a, const void *b )
{
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : x > y;
}

void wp_renderer_dx11_get_statistics_counters( const wp_renderer_dx11 *r,
                                               wp_render_statistics_dx11 *out )
{
    if( !out ) return;
    memset( out, 0, sizeof( *out ) );
    if( !r ) return;
    *out = r->statistics;
    out->interval_p95_ms = 0; /* History is deliberately unavailable in this path. */
    out->flip_model = r->flip_model;
    if( out->frames )
    {
        out->cpu_frame_ms = r->cpu_total / out->frames;
        out->present_ms = r->present_total / out->frames;
    }
    if( out->gpu_samples ) out->gpu_frame_ms = r->gpu_total / out->gpu_samples;
    if( !out->interval_samples ) return;
    out->interval_ms = r->interval_total / out->interval_samples;
}

void wp_renderer_dx11_get_statistics( const wp_renderer_dx11 *r,
                                      wp_render_statistics_dx11 *out )
{
    double *sorted;
    size_t count;
    wp_renderer_dx11_get_statistics_counters( r, out );
    if( !r || !out || !out->interval_samples ) return;
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
        memset( &r->material_upload_statistics, 0, sizeof( r->material_upload_statistics ) );
        r->interval_total = r->cpu_total = r->present_total = r->gpu_total = 0;
        r->last_present.QuadPart = 0;
        ++r->statistics_epoch;
        r->measuring = 1;
    }
    if( !r->measuring ) return;
    r->frame_measuring = 1;
    QueryPerformanceCounter( &r->frame_start );
    if( r->gpu_profiling_disabled ) { r->active_gpu_query = -1; return; }
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

void wp_renderer_dx11_set_gpu_profiling( wp_renderer_dx11 *r, wp_s32 enabled )
{
    if( !r ) return;
    r->gpu_profiling_disabled = !enabled;
    if( enabled && !r->measuring ) wp_renderer_dx11_reset_statistics(r);
}

void wp_renderer_dx11_begin_frame( wp_renderer_dx11 *r )
{
    if( !r )
        return;

    if(!r->rtt_owner_thread) r->rtt_owner_thread = GetCurrentThreadId();
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
    if( r ) wp_renderer_dx11_try_present( r, r->hwnd, vsync );
}

wp_s32 wp_renderer_dx11_try_present( wp_renderer_dx11 *r, void *expected_hwnd, wp_s32 vsync )
{
    LARGE_INTEGER before, after;
    HRESULT result;
    if( !r || !r->swap_chain || !expected_hwnd || r->hwnd != (HWND)expected_hwnd ||
        !r->rt_view || r->rtt_active || r->active_rt_view != r->rt_view || r->hdr_active || r->shadow_pass )
        return 0;
    if( r->frame_measuring && r->active_gpu_query >= 0 )
    {
        wp_gpu_frame_query *q = &r->gpu_queries[r->active_gpu_query];
        ID3D11DeviceContext_End( r->context, (ID3D11Asynchronous *)q->end );
        ID3D11DeviceContext_End( r->context, (ID3D11Asynchronous *)q->disjoint );
        q->pending = 1;
    }
    QueryPerformanceCounter( &before );
    result = IDXGISwapChain_Present( r->swap_chain, vsync ? 1 : 0,
                           !vsync && r->flip_model ? DXGI_PRESENT_ALLOW_TEARING : 0 );
    QueryPerformanceCounter( &after );
    ++r->statistics.present_attempts;
    r->statistics.last_present_hresult = (wp_s32)result;
    if( result == DXGI_STATUS_OCCLUDED ) ++r->statistics.present_occluded;
    else if( result != S_OK ) ++r->statistics.present_failures;
    if( result != S_OK )
    {
        r->last_present.QuadPart = 0;
        r->frame_measuring = 0;
        r->active_gpu_query = -1;
        return result == DXGI_STATUS_OCCLUDED ? 2 : -1;
    }
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
    return 1;
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
    wp_mat4f effective;
    if(!r || !mat) return;
    effective=*mat;
    if(r->project_aa==2 && r->hdr_active && !r->shadow_pass && r->hdr_width && r->hdr_height)
    {
        static const wp_f32 jitter_x[8]={0.0f,-.25f,.25f,-.375f,.125f,-.125f,.375f,-.4375f};
        static const wp_f32 jitter_y[8]={-.1666667f,.1666667f,-.3888889f,-.0555556f,.2777778f,-.2777778f,.0555556f,.3888889f};
        wp_u32 index=(wp_u32)(r->hdr_completed_frames%8u);
        effective.m[0][2]+=2.0f*jitter_x[index]/r->hdr_width;
        effective.m[1][2]+=2.0f*jitter_y[index]/r->hdr_height;
    }
    if(memcmp(&r->proj_matrix,&effective,sizeof(effective))==0) return;
    r->proj_matrix = effective;
    r->mvp_dirty = 1;
}

void wp_renderer_dx11_set_material( wp_renderer_dx11 *r,
                                    const wp_material_dx11 *material )
{
    wp_material_dx11 resolved;
    if( !r || !material ) return;
    resolved = *material;
    // Transfer flags belong to bound SRVs, not caller-authored material state.
    // Preserve them here so submitting an unchanged material does not force a
    // constant-buffer upload on every sRGB draw; bind_geometry refreshes on change.
    resolved.extra_map_flags.z = r->material.extra_map_flags.z;
    resolved.extra_map_flags.w = r->material.extra_map_flags.w;
    resolved.environment.x = r->environment_view ? 1.0f : 0.0f;
    resolved.environment.y = r->environment_max_lod;
    if( memcmp( &r->material, &resolved, sizeof( resolved ) ) == 0 ) return;
    r->material = resolved;
    r->material_dirty = 1;
}

wp_s32 wp_renderer_dx11_get_material_upload_mode( const wp_renderer_dx11 *r )
{ return r ? r->material_upload_mode : -1; }

wp_s32 wp_renderer_dx11_set_material_upload_mode( wp_renderer_dx11 *r, wp_s32 mode )
{
    static const D3D_SHADER_MACRO defines[] = { { "WP_MATERIAL_SPLIT_CONSTANTS", "1" }, { NULL, NULL } };
    ID3D11Buffer *buffers[3] = { NULL, NULL, NULL };
    ID3D11PixelShader *shader = NULL;
    ID3DBlob *blob = NULL;
    D3D11_BUFFER_DESC desc;
    const UINT sizes[3] = { 128, 112, 32 };
    wp_s32 i;
    HRESULT hr;
    if( !r || ( mode != WP_DX11_MATERIAL_UPLOAD_COMBINED && mode != WP_DX11_MATERIAL_UPLOAD_SPLIT ) ) return 0;
    if( mode == r->material_upload_mode ) return 1;
    if( r->rtt_active || r->hdr_active || r->shadow_pass ) return 0;
    if( mode == WP_DX11_MATERIAL_UPLOAD_SPLIT && !r->ps_pntc_split )
    {
        hr = wp_renderer_dx11_compile_shader_options( s_ps_pntc_src, "ps_main", "ps_4_0", defines, &blob );
        if( FAILED(hr) ) goto fail;
        hr = ID3D11Device_CreatePixelShader( r->device, ID3D10Blob_GetBufferPointer(blob),
            ID3D10Blob_GetBufferSize(blob), NULL, &shader );
        if( FAILED(hr) ) goto fail;
        memset( &desc, 0, sizeof(desc) );
        desc.Usage = D3D11_USAGE_DYNAMIC; desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        for( i = 0; i < 3; ++i )
        {
            desc.ByteWidth = sizes[i];
            if( FAILED( ID3D11Device_CreateBuffer( r->device, &desc, NULL, &buffers[i] ) ) ) goto fail;
        }
        r->ps_pntc_split = shader;
        r->cb_material_surface = buffers[0]; r->cb_material_view = buffers[1]; r->cb_material_bindings = buffers[2];
        ID3D10Blob_Release(blob);
    }
    r->material_upload_mode = mode;
    r->material_dirty = 1;
    wp_renderer_dx11_invalidate_state( r );
    return 1;
fail:
    for( i = 0; i < 3; ++i ) if( buffers[i] ) ID3D11Buffer_Release(buffers[i]);
    if( shader ) ID3D11PixelShader_Release(shader);
    if( blob ) ID3D10Blob_Release(blob);
    return 0;
}

void wp_renderer_dx11_set_environment( wp_renderer_dx11 *r, void *view, wp_f32 max_lod )
{
    if(r && !wp_rtt_dx11_view_allowed(r, (ID3D11ShaderResourceView *)view)) return;
    if( !r || ( r->environment_view == (ID3D11ShaderResourceView *)view &&
                 r->environment_max_lod == max_lod ) ) return;
    r->environment_view = (ID3D11ShaderResourceView *)view;
    r->environment_max_lod = max_lod;
    r->material.environment.x = view ? 1.0f : 0.0f;
    r->material.environment.y = max_lod;
    r->material_dirty = 1;
}

static wp_f32 wp_renderer_dx11_view_samples_linear( ID3D11ShaderResourceView *view );

static wp_s32 wp_renderer_dx11_stage_terrain_material( wp_renderer_dx11 *r,
    const wp_terrain_material_dx11 *material, void *const *views, wp_s32 apply )
{
    wp_terrain_material_dx11 value;
    ID3D11ShaderResourceView *candidate[WP_TERRAIN_MATERIAL_VIEWS] = { 0 };
    wp_s32 i;
    if( !r ) return 0;
    memset( &value, 0, sizeof( value ) );
    if( material )
    {
        const wp_f32 *floats = (const wp_f32 *)material;
        for( i = 0; i < (wp_s32)( sizeof( *material ) / sizeof( wp_f32 ) ); ++i )
            if( !( floats[i] >= -FLT_MAX && floats[i] <= FLT_MAX ) ) return 0;
        value = *material;
        if( value.controls.x != 1.0f || value.control_transform.z <= 0.0f ||
            value.control_transform.w <= 0.0f || !views ) return 0;
        if( (value.appearance.x != 0 && value.appearance.x != 1) ||
            value.appearance.y < 1 || value.appearance.y > 16 ||
            (value.appearance.z != 0 && value.appearance.z != 1) || value.appearance.w < .001f || value.appearance.w > 1 ||
            value.detail.x <= 0 || value.detail.y <= 0 || value.detail.z < 0 || value.detail.z > 16 ||
            value.macro.x <= 0 || value.macro.y < 0 || value.macro.y > 1 ||
            value.macro.z < 0 || value.macro.z > 65535 || floorf(value.macro.z) != value.macro.z ||
            value.macro.w < 0 || value.macro.w > 65535 || floorf(value.macro.w) != value.macro.w ||
            value.weather.x < 0 || value.weather.x > 1 || value.weather.y < 0 || value.weather.y > 1 ||
            value.weather.z < .045f || value.weather.z > 1 || value.weather.w < 0 || value.weather.w > 1 ||
            value.snow.x < .045f || value.snow.x > 1 ) return 0;
        for( i = 0; i < 4; ++i )
        {
            const wp_vec4f *p = &value.surface[i], *m = &value.maps[i], *t = &value.tint[i];
            const wp_vec4f *h = &value.height[i];
            if( p->x <= 0 || p->y <= 0 || p->z < 0 || p->z > 1 || p->w < 0 || p->w > 1 ||
                m->w < 0 || m->w > 16 || t->x < 0 || t->x > 1 || t->y < 0 || t->y > 1 ||
                t->z < 0 || t->z > 1 || t->w < 0 || t->w > 1 || h->x < 0 || h->x > 4 || h->y < -1 || h->y > 1 ||
                ((const wp_f32*)&value.macro_tint)[i] < 0 || ((const wp_f32*)&value.macro_tint)[i] > 1 ||
                ((const wp_f32*)&value.snow_tint)[i] < 0 || ((const wp_f32*)&value.snow_tint)[i] > 1 ) return 0;
            if( (m->x > 0.5f && !views[i*3]) || (m->y > 0.5f && !views[i*3+1]) ||
                (m->z > 0.5f && !views[i*3+2]) ) return 0;
        }
        if( value.controls.y > 0.5f && !views[12] ) return 0;
        if( value.detail.w > 0.5f && !views[13] ) return 0;
        if( value.snow.y > 0.5f && !views[14] ) return 0;
        for( i = 0; i < WP_TERRAIN_MATERIAL_VIEWS; ++i )
        {
            ID3D11Device *device = NULL;
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            candidate[i] = (ID3D11ShaderResourceView *)views[i];
            if( i % 3 == 0 && i < 12 )
                value.height[i / 3].z = wp_renderer_dx11_view_samples_linear( candidate[i] );
            if( !candidate[i] ) continue;
            ID3D11ShaderResourceView_GetDevice( candidate[i], &device );
            if( device != r->device ) { if( device ) ID3D11Device_Release( device ); return 0; }
            ID3D11Device_Release( device );
            ID3D11ShaderResourceView_GetDesc( candidate[i], &desc );
            if( desc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D ) return 0;
        }
    }
    if( !apply ) return 1;
    for(i = 0; i < WP_TERRAIN_MATERIAL_VIEWS; ++i)
        if(!wp_rtt_dx11_view_allowed(r, candidate[i])) return 0;
    if( memcmp( &r->terrain, &value, sizeof( value ) ) )
    { r->terrain = value; r->terrain_dirty = 1; }
    if( memcmp( r->terrain_views, candidate, sizeof( candidate ) ) )
    { memcpy( r->terrain_views, candidate, sizeof( candidate ) ); r->pntc_bindings_valid = 0; }
    /* The immediate context owns COM references to bound views. Release them at
       unbind, before the owner retires its material and returns allocation leases;
       waiting for another draw would leave unaccounted resident terrain textures. */
    if( !material && r->context )
    {
        ID3D11DeviceContext_PSSetShaderResources( r->context, 9, WP_TERRAIN_MATERIAL_VIEWS, candidate );
        r->pntc_bindings_valid = 0;
    }
    return 1;
}

wp_s32 wp_renderer_dx11_validate_terrain_material( wp_renderer_dx11 *r,
    const wp_terrain_material_dx11 *material, void *const *views )
{ return wp_renderer_dx11_stage_terrain_material( r, material, views, 0 ); }

wp_s32 wp_renderer_dx11_set_terrain_material( wp_renderer_dx11 *r,
    const wp_terrain_material_dx11 *material, void *const *views )
{ return wp_renderer_dx11_stage_terrain_material( r, material, views, 1 ); }

void wp_renderer_dx11_set_material_textures( wp_renderer_dx11 *r, void *const *views )
{
    wp_s32 i;
    if( !r )
        return;
    for( i = 0; i < 6; ++i )
    {
        r->material_texture_views[i] = views ? (ID3D11ShaderResourceView *)views[i] : NULL;
        (void)wp_rtt_dx11_view_allowed(r, r->material_texture_views[i]);
    }
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
        if(!wp_rtt_dx11_view_allowed(r, texture_view)) return;
        wp_renderer_dx11_bind_output( r, wp_renderer_dx11_view_samples_linear( texture_view ) );
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
        if(!wp_rtt_dx11_view_allowed(r, texture_view)) return 0;
        wp_renderer_dx11_bind_output( r, wp_renderer_dx11_view_samples_linear( texture_view ) );
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
        if(!wp_rtt_dx11_view_allowed(r, texture_view)) return;
        wp_renderer_dx11_bind_output( r, wp_renderer_dx11_view_samples_linear( texture_view ) );
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

    (void)wp_rtt_dx11_view_allowed(r, (ID3D11ShaderResourceView *)texture_view);
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

void *wp_renderer_dx11_create_texture_mips_native( wp_renderer_dx11 *r,
    const wp_texture_mip_dx11 *levels, wp_u32 level_count )
{
    D3D11_TEXTURE2D_DESC desc;
    D3D11_SUBRESOURCE_DATA data[15];
    ID3D11Texture2D *texture = NULL;
    ID3D11ShaderResourceView *view = NULL;
    wp_u32 i, width, height;
    HRESULT hr;
    if( !r || !levels || !level_count || level_count > 15 ||
        !wp_renderer_dx11_dimensions_supported( r, levels[0].width, levels[0].height ) ) return NULL;
    width = levels[0].width; height = levels[0].height;
    memset( data, 0, sizeof(data) );
    for( i = 0; i < level_count; ++i )
    {
        if( !levels[i].bgra || levels[i].width != width || levels[i].height != height ) return NULL;
        data[i].pSysMem = levels[i].bgra;
        data[i].SysMemPitch = width * 4u;
        if( width == 1 && height == 1 && i + 1 < level_count ) return NULL;
        width = width > 1 ? width / 2 : 1; height = height > 1 ? height / 2 : 1;
    }
    memset( &desc, 0, sizeof(desc) );
    desc.Width = levels[0].width; desc.Height = levels[0].height;
    desc.MipLevels = level_count; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    hr = ID3D11Device_CreateTexture2D( r->device, &desc, data, &texture );
    if( FAILED(hr) ) return NULL;
    hr = ID3D11Device_CreateShaderResourceView( r->device, (ID3D11Resource *)texture, NULL, &view );
    ID3D11Texture2D_Release(texture);
    return SUCCEEDED(hr) ? view : NULL;
}

void *wp_renderer_dx11_create_texture_rgba32f_native( wp_renderer_dx11 *r,
    const wp_f32 *rgba, wp_u32 width, wp_u32 height, wp_u64 float_count )
{
    D3D11_TEXTURE2D_DESC desc;
    D3D11_SUBRESOURCE_DATA data;
    ID3D11Texture2D *texture = NULL;
    ID3D11ShaderResourceView *view = NULL;
    wp_u64 count;
    HRESULT hr;
    if( !rgba || !wp_renderer_dx11_dimensions_supported( r, width, height ) ) return NULL;
    count = (wp_u64)width * height * 4u;
    if( count != float_count || count > (256ull * 1024 * 1024) / sizeof(wp_f32) ) return NULL;
    memset( &desc, 0, sizeof(desc) );
    desc.Width = width; desc.Height = height; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    memset( &data, 0, sizeof(data) );
    data.pSysMem = rgba; data.SysMemPitch = width * 4u * sizeof(wp_f32);
    hr = ID3D11Device_CreateTexture2D( r->device, &desc, &data, &texture );
    if( FAILED(hr) ) return NULL;
    hr = ID3D11Device_CreateShaderResourceView( r->device, (ID3D11Resource *)texture, NULL, &view );
    ID3D11Texture2D_Release(texture);
    return SUCCEEDED(hr) ? view : NULL;
}

void *wp_dx11_create_cubemap_rgba32f_native( void *native_device,
    const wp_texture_cube_mip_dx11 *levels, wp_u32 level_count )
{
    ID3D11Device *device = (ID3D11Device *)native_device;
    D3D11_TEXTURE2D_DESC desc;
    D3D11_SUBRESOURCE_DATA data[48];
    ID3D11Texture2D *texture = NULL;
    ID3D11ShaderResourceView *view = NULL;
    wp_u32 face, mip, index, size;
    HRESULT hr;
    if( !device || !levels || level_count != 48 ) return NULL;
    memset( data, 0, sizeof(data) );
    for( face = 0; face < 6; ++face )
    {
        for( mip = 0; mip < 8; ++mip )
        {
            index = face * 8 + mip;
            size = 128u >> mip;
            if( !levels[index].rgba || levels[index].width != size ||
                levels[index].height != size ||
                levels[index].float_count != (wp_u64)size * size * 4u ) return NULL;
            data[index].pSysMem = levels[index].rgba;
            data[index].SysMemPitch = size * 4u * sizeof(wp_f32);
        }
    }
    memset( &desc, 0, sizeof(desc) );
    desc.Width = desc.Height = 128;
    desc.MipLevels = 8; desc.ArraySize = 6;
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    hr = ID3D11Device_CreateTexture2D( device, &desc, data, &texture );
    if( FAILED(hr) ) return NULL;
    hr = ID3D11Device_CreateShaderResourceView( device, (ID3D11Resource *)texture, NULL, &view );
    ID3D11Texture2D_Release(texture);
    return SUCCEEDED(hr) ? view : NULL;
}

struct wp_dx11_cubemap_owner
{
    ID3D11ShaderResourceView *view;
    ID3D11ShaderResourceView *sources[6];
};

wp_s32 wp_dx11_texture_view_matches_device( void *native_view, void *native_device )
{
    ID3D11ShaderResourceView *view = (ID3D11ShaderResourceView *)native_view;
    ID3D11Device *device = (ID3D11Device *)native_device, *owner = NULL;
    wp_s32 matches;
    if( !view || !device ) return 0;
    ID3D11ShaderResourceView_GetDevice(view, &owner);
    matches = owner == device;
    if( owner ) ID3D11Device_Release(owner);
    return matches;
}

wp_s32 wp_dx11_device_matches_context( void *native_device, void *native_context )
{
    ID3D11Device *device = (ID3D11Device *)native_device;
    ID3D11DeviceContext *context = (ID3D11DeviceContext *)native_context;
    ID3D11Device *owner = NULL;
    wp_s32 matches;
    if( !device || !context ) return 0;
    ID3D11DeviceContext_GetDevice( context, &owner );
    matches = owner == device;
    if( owner ) ID3D11Device_Release(owner);
    return matches;
}

wp_dx11_cubemap_owner *wp_dx11_cubemap_owner_create( void *native_device,
    const wp_texture_cube_mip_dx11 *levels, wp_u32 level_count,
    void *const source_views[6] )
{
    ID3D11Device *device = (ID3D11Device *)native_device;
    wp_dx11_cubemap_owner *owner;
    wp_u32 i;
    wp_s32 has_sources;
    if( !device || !source_views ) return NULL;
    has_sources = source_views[0] != NULL;
    for( i = 0; i < 6; ++i )
    {
        if( (source_views[i] != NULL) != has_sources ) return NULL;
        if( has_sources )
        {
            if( !wp_dx11_texture_view_matches_device(source_views[i],device) ) return NULL;
        }
    }
    owner = (wp_dx11_cubemap_owner *)calloc(1,sizeof(*owner));
    if( !owner ) return NULL;
    owner->view = (ID3D11ShaderResourceView *)wp_dx11_create_cubemap_rgba32f_native(
        device,levels,level_count);
    if( !owner->view ) { free(owner); return NULL; }
    for( i = 0; i < 6; ++i )
    {
        owner->sources[i] = (ID3D11ShaderResourceView *)source_views[i];
        if( owner->sources[i] ) ID3D11ShaderResourceView_AddRef(owner->sources[i]);
    }
    return owner;
}

void wp_dx11_cubemap_owner_destroy( wp_dx11_cubemap_owner *owner )
{
    wp_u32 i;
    if( !owner ) return;
    if( owner->view ) ID3D11ShaderResourceView_Release(owner->view);
    for( i = 0; i < 6; ++i )
        if( owner->sources[i] ) ID3D11ShaderResourceView_Release(owner->sources[i]);
    free(owner);
}

void *wp_dx11_cubemap_owner_get_view( const wp_dx11_cubemap_owner *owner )
{
    return owner ? owner->view : NULL;
}

wp_s32 wp_dx11_cubemap_owner_matches( const wp_dx11_cubemap_owner *owner,
    void *native_device, void *const source_views[6] )
{
    wp_u32 i;
    if( !owner || !owner->view || !native_device || !source_views ) return 0;
    for( i = 0; i < 6; ++i )
        if( owner->sources[i] != source_views[i] ) return 0;
    return wp_dx11_texture_view_matches_device(owner->view,native_device);
}

wp_s32 wp_dx11_read_texture_face_native( void *native_device, void *native_context,
    void *native_view, wp_texture_face_readback_dx11 *output )
{
    ID3D11Device *device = (ID3D11Device *)native_device;
    ID3D11DeviceContext *context = (ID3D11DeviceContext *)native_context;
    ID3D11ShaderResourceView *view = (ID3D11ShaderResourceView *)native_view;
    ID3D11Device *owner = NULL;
    ID3D11Resource *resource = NULL;
    ID3D11Texture2D *source = NULL, *staging = NULL;
    D3D11_SHADER_RESOURCE_VIEW_DESC view_desc;
    D3D11_TEXTURE2D_DESC desc;
    D3D11_MAPPED_SUBRESOURCE mapped;
    wp_u32 format = 0, bytes_per_pixel = 0, mip, y;
    size_t row_bytes, total_bytes;
    unsigned char *pixels = NULL;
    wp_s32 result = 0;
    if( !output ) return 0;
    memset( output, 0, sizeof(*output) );
    if( !device || !context || !view ) return 0;
    ID3D11DeviceContext_GetDevice( context, &owner );
    if( owner != device ) goto cleanup;
    ID3D11Device_Release(owner); owner = NULL;
    ID3D11ShaderResourceView_GetResource( view, &resource );
    if( !resource || FAILED(ID3D11Resource_QueryInterface( resource, &IID_ID3D11Texture2D,
                                                           (void **)&source )) ) goto cleanup;
    ID3D11ShaderResourceView_GetDesc( view, &view_desc );
    if( view_desc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D ) goto cleanup;
    ID3D11Texture2D_GetDevice( source, &owner );
    if( owner != device ) goto cleanup;
    ID3D11Device_Release(owner); owner = NULL;
    ID3D11Texture2D_GetDesc( source, &desc );
    mip = view_desc.Texture2D.MostDetailedMip;
    if( desc.ArraySize != 1 || desc.SampleDesc.Count != 1 || mip >= desc.MipLevels ) goto cleanup;
    switch( view_desc.Format )
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        format = WP_DX11_FACE_BGRA8; bytes_per_pixel = 4; break;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        format = WP_DX11_FACE_RGBA8; bytes_per_pixel = 4; break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        format = WP_DX11_FACE_RGBA16F; bytes_per_pixel = 8; break;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        format = WP_DX11_FACE_RGBA32F; bytes_per_pixel = 16; break;
    default: goto cleanup;
    }
    desc.Width = desc.Width >> mip ? desc.Width >> mip : 1;
    desc.Height = desc.Height >> mip ? desc.Height >> mip : 1;
    if( desc.Width > 2048 || desc.Height > 2048 ) goto cleanup;
    row_bytes = (size_t)desc.Width * bytes_per_pixel;
    total_bytes = row_bytes * desc.Height;
    desc.MipLevels = desc.ArraySize = 1;
    desc.BindFlags = desc.MiscFlags = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if( FAILED(ID3D11Device_CreateTexture2D(device,&desc,NULL,&staging)) ) goto cleanup;
    ID3D11DeviceContext_CopySubresourceRegion(context,(ID3D11Resource *)staging,0,0,0,0,
                                               (ID3D11Resource *)source,mip,NULL);
    if( FAILED(ID3D11DeviceContext_Map(context,(ID3D11Resource *)staging,0,
                                       D3D11_MAP_READ,0,&mapped)) ) goto cleanup;
    pixels = (unsigned char *)malloc(total_bytes);
    if( pixels && mapped.RowPitch >= row_bytes )
    {
        for( y = 0; y < desc.Height; ++y )
            memcpy(pixels + y * row_bytes,(const unsigned char *)mapped.pData + y * mapped.RowPitch,
                   row_bytes);
        output->pixels = pixels;
        output->width = desc.Width; output->height = desc.Height;
        output->row_pitch = (wp_u32)row_bytes; output->format = format;
        pixels = NULL;
        result = 1;
    }
    ID3D11DeviceContext_Unmap(context,(ID3D11Resource *)staging,0);
cleanup:
    free(pixels);
    if( staging ) ID3D11Texture2D_Release(staging);
    if( source ) ID3D11Texture2D_Release(source);
    if( resource ) ID3D11Resource_Release(resource);
    if( owner ) ID3D11Device_Release(owner);
    return result;
}

void wp_dx11_release_texture_face_readback_native( wp_texture_face_readback_dx11 *output )
{
    if( !output ) return;
    free(output->pixels);
    memset(output,0,sizeof(*output));
}

void *wp_renderer_dx11_create_texture_encoded_mips_native( wp_renderer_dx11 *r,
    const wp_texture_encoded_mip_dx11 *levels, wp_u32 count, wp_u32 format )
{
    D3D11_TEXTURE2D_DESC desc;
    D3D11_SUBRESOURCE_DATA data[15];
    ID3D11Texture2D *texture = NULL;
    ID3D11ShaderResourceView *view = NULL;
    wp_u32 i, w, h, block, support = 0;
    uint64_t total = 0;
    HRESULT hr;
    if( !r || !levels || !count || count > 15 ||
        !wp_renderer_dx11_dimensions_supported(r,levels[0].width,levels[0].height) ) return NULL;
    block = format==71 || format==72 ? 8 : format==77 || format==78 ? 16 : 0;
    if( !block && format!=87 && format!=91 ) return NULL;
    w=levels[0].width;h=levels[0].height;
    if( block && (w%4 || h%4) ) return NULL;
    hr=ID3D11Device_CheckFormatSupport(r->device,(DXGI_FORMAT)format,&support);
    if(FAILED(hr) || (support & (D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_SHADER_SAMPLE)) !=
                                (D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_SHADER_SAMPLE)) return NULL;
    memset(data,0,sizeof(data));
    for(i=0;i<count;++i)
    {
        wp_u32 row=block ? ((w+3)/4)*block : w*4;
        uint64_t size=(uint64_t)row*(block ? (h+3)/4 : h);
        if(!levels[i].bytes || levels[i].width!=w || levels[i].height!=h ||
           levels[i].byte_count!=size || size>256ull*1024*1024-total) return NULL;
        total+=size;
        data[i].pSysMem=levels[i].bytes;data[i].SysMemPitch=row;data[i].SysMemSlicePitch=(UINT)size;
        if(w==1 && h==1 && i+1<count) return NULL;
        w=w>1?w/2:1;h=h>1?h/2:1;
    }
    memset(&desc,0,sizeof(desc));
    desc.Width=levels[0].width;desc.Height=levels[0].height;desc.MipLevels=count;desc.ArraySize=1;
    desc.Format=(DXGI_FORMAT)format;desc.SampleDesc.Count=1;
    desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    hr=ID3D11Device_CreateTexture2D(r->device,&desc,data,&texture);
    if(FAILED(hr)) return NULL;
    hr=ID3D11Device_CreateShaderResourceView(r->device,(ID3D11Resource *)texture,NULL,&view);
    ID3D11Texture2D_Release(texture);
    return SUCCEEDED(hr)?view:NULL;
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
    target->generation = (wp_u64)InterlockedIncrement64(&wp_dx11_rtt_generation);
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
    wp_renderer_dx11_destroy_render_texture(target->scratch);
    if(target->rtt_state) ID3DDeviceContextState_Release(target->rtt_state);
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

static wp_s32 copy_texture_rgba8( ID3D11Device *device, ID3D11DeviceContext *context,
    ID3D11Texture2D *texture, wp_s32 width, wp_s32 height, void *output, wp_u64 output_bytes )
{
    ID3D11Texture2D *staging = NULL;
    D3D11_TEXTURE2D_DESC desc;
    D3D11_MAPPED_SUBRESOURCE mapped;
    wp_u64 row_bytes;
    wp_u32 y;
    wp_s32 result = 0;
    if( !device || !context || !texture || !output || width <= 0 || height <= 0 )
        return 0;
    row_bytes = (wp_u64)width * 4u;
    if( output_bytes < row_bytes * (wp_u64)height ) return 0;
    ID3D11Texture2D_GetDesc(texture,&desc);
    if( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM || desc.Width != (UINT)width ||
        desc.Height != (UINT)height || desc.MipLevels != 1 || desc.ArraySize != 1 ||
        desc.SampleDesc.Count != 1 ) return 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    if( FAILED(ID3D11Device_CreateTexture2D(device,&desc,NULL,&staging)) ) goto cleanup;
    ID3D11DeviceContext_CopyResource(context,(ID3D11Resource *)staging,
                                    (ID3D11Resource *)texture);
    memset(&mapped,0,sizeof(mapped));
    if( FAILED(ID3D11DeviceContext_Map(context,(ID3D11Resource *)staging,0,D3D11_MAP_READ,0,
                                       &mapped)) ) goto cleanup;
    if( mapped.pData && mapped.RowPitch >= row_bytes )
    {
        for( y = 0; y < (wp_u32)height; ++y )
            memcpy((unsigned char *)output + y * row_bytes,
                   (const unsigned char *)mapped.pData + y * mapped.RowPitch,
                   (size_t)row_bytes);
        result = 1;
    }
    ID3D11DeviceContext_Unmap(context,(ID3D11Resource *)staging,0);
cleanup:
    if( staging ) ID3D11Texture2D_Release(staging);
    return result;
}

wp_s32 wp_renderer_dx11_copy_render_texture_rgba8(
    const wp_render_texture_dx11 *target, void *output, wp_u64 output_bytes )
{
    ID3D11Device *device = NULL;
    ID3D11DeviceContext *context = NULL;
    wp_s32 result = 0;
    if( !target || !target->texture ) return 0;
    ID3D11Texture2D_GetDevice(target->texture,&device);
    if( !device ) return 0;
    ID3D11Device_GetImmediateContext(device,&context);
    if( context ) result = copy_texture_rgba8(device,context,target->texture,target->width,
                                               target->height,output,output_bytes);
    if( context ) ID3D11DeviceContext_Release(context);
    ID3D11Device_Release(device);
    return result;
}

wp_s32 wp_renderer_dx11_copy_back_buffer_rgba8(
    wp_renderer_dx11 *r, void *expected_hwnd, void *output, wp_u64 output_bytes )
{
    ID3D11Texture2D *back_buffer = NULL;
    wp_s32 result;
    if( !r || !r->swap_chain || !expected_hwnd || r->hwnd != (HWND)expected_hwnd ) return 0;
    if( FAILED(IDXGISwapChain_GetBuffer(r->swap_chain,0,&IID_ID3D11Texture2D,
                                       (void **)&back_buffer)) ) return 0;
    result = copy_texture_rgba8(r->device,r->context,back_buffer,r->width,r->height,
                                output,output_bytes);
    ID3D11Texture2D_Release(back_buffer);
    return result;
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

    geometry->vertex_stride = sizeof( wp_vertex_ptc );
    geometry->device = r->device;
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

    geometry->vertex_stride = sizeof( wp_vertex_pntc );
    geometry->device = r->device;
    geometry->vertex_count = vertex_count;
    geometry->index_count = index_count;
    geometry->index_format = indices_are_u32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    ++r->statistics.geometry_creations;
    return geometry;
}

void wp_renderer_dx11_forget_mesh( const wp_graphics_mesh *mesh )
{
    wp_renderer_dx11 *r;
    wp_dx11_mesh_cache **owner, *entry;
    for( r = wp_dx11_renderers; r; r = r->next_renderer )
    {
        owner = &r->meshes[wp_dx11_mesh_bucket( mesh )];
        while( *owner )
        {
            entry = *owner;
            if( entry->mesh == mesh )
            {
                *owner = entry->next;
                wp_renderer_dx11_destroy_geometry( entry->geometry );
                free( entry );
            }
            else owner = &entry->next;
        }
    }
}
wp_geometry_dx11 *wp_renderer_dx11_prepare_mesh_geometry( wp_renderer_dx11 *r,
    wp_graphics_mesh *mesh, wp_u32 first, wp_u32 count )
{
    return wp_renderer_dx11_prepare_mesh_geometry_budgeted( r, mesh, first, count, NULL );
}

wp_geometry_dx11 *wp_renderer_dx11_prepare_mesh_geometry_budgeted( wp_renderer_dx11 *r,
    wp_graphics_mesh *mesh, wp_u32 first, wp_u32 count,
    const wp_gpu_allocation_callbacks *callbacks )
{
    wp_dx11_mesh_cache *entry, **owner;
    const wp_vertex_pntc *vertices;
    const void *source_indices;
    wp_u32 *indices;
    wp_u32 elements, vertex_count, i, a, b, c, out, output_count, swap;
    uint64_t generation;
    wp_s32 strip;
    unsigned bucket;
    wp_geometry_dx11 *geometry;
    void *reservation = NULL;
    if( !r || !mesh ) return NULL;
    if( callbacks && ( !callbacks->reserve || !callbacks->retain || !callbacks->release ) ) return NULL;
    strip = wp_graphics_mesh_get_primitive_type( mesh ) == WORKPHONE_PRIMITIVE_TRIANGLE_STRIP;
    if( !strip && wp_graphics_mesh_get_primitive_type( mesh ) != WORKPHONE_PRIMITIVE_TRIANGLE_LIST ) return NULL;
    vertex_count = wp_graphics_mesh_get_vertex_count( mesh );
    source_indices = wp_graphics_mesh_get_indices( mesh );
    elements = source_indices ? wp_graphics_mesh_get_index_count( mesh ) : vertex_count;
    if( first > elements || count > elements - first || count < 3u || vertex_count > INT_MAX ) return NULL;
    if( !strip ) { first = 0; count = elements - elements % 3u; }
    if( count < 3u || ( strip && count - 2u > (wp_u32)INT_MAX / 3u ) || ( !strip && count > INT_MAX ) ) return NULL;
    generation = wp_graphics_mesh_get_content_generation( mesh );
    bucket = wp_dx11_mesh_bucket( mesh );
    for( entry = r->meshes[bucket]; entry; entry = entry->next )
        if( entry->mesh == mesh && entry->first == first && entry->count == count && entry->generation == generation ) return entry->geometry;
    vertices = wp_graphics_mesh_prepare_lit_vertices( mesh );
    if( !vertices ) return NULL;
    output_count = strip ? ( count - 2u ) * 3u : count;
    if( (size_t)output_count > (size_t)-1 / sizeof( *indices ) ) return NULL;
    indices = (wp_u32 *)wp_mesh_memory_allocate_working( mesh, (size_t)output_count * sizeof( *indices ) );
    if( !indices ) return NULL;
    out = 0;
    for( i = 0; i <= count - 3u; i += strip ? 1u : 3u )
    {
        a = first + i; b = a + 1u; c = a + 2u;
        if( source_indices )
        {
            if( wp_graphics_mesh_get_index_format( mesh ) == WORKPHONE_INDEX_FORMAT_UINT32 )
            { a = ( (const wp_u32 *)source_indices )[a]; b = ( (const wp_u32 *)source_indices )[b]; c = ( (const wp_u32 *)source_indices )[c]; }
            else
            { a = ( (const uint16_t *)source_indices )[a]; b = ( (const uint16_t *)source_indices )[b]; c = ( (const uint16_t *)source_indices )[c]; }
        }
        if( a >= vertex_count || b >= vertex_count || c >= vertex_count ) { wp_mesh_memory_free( indices ); return NULL; }
        if( strip && ( i & 1u ) ) { swap = a; a = b; b = swap; }
        indices[out++] = a; indices[out++] = b; indices[out++] = c;
    }
    if( callbacks )
    {
        reservation = callbacks->reserve( callbacks->user,
            (wp_u64)vertex_count * sizeof( wp_vertex_pntc ) + (wp_u64)out * sizeof( wp_u32 ) );
        if( !reservation ) { wp_mesh_memory_free( indices ); return NULL; }
    }
    geometry = wp_renderer_dx11_create_indexed_geometry_pntc( r, vertices, (wp_s32)vertex_count, indices, (wp_s32)out, 1 );
    wp_mesh_memory_free( indices );
    if( reservation )
    {
        if( geometry && ( !callbacks->retain( reservation, geometry->vertex_buffer ) ||
                         !callbacks->retain( reservation, geometry->index_buffer ) ) )
        {
            wp_renderer_dx11_destroy_geometry( geometry );
            geometry = NULL;
        }
        callbacks->release( reservation );
    }
    if( !geometry ) return NULL;
    entry = (wp_dx11_mesh_cache *)malloc( sizeof( *entry ) );
    if( !entry ) { wp_renderer_dx11_destroy_geometry( geometry ); return NULL; }
    /* Publication is transactional: allocation/admission failure retains the
     * previous physical geometry and its reservation. */
    owner = &r->meshes[bucket];
    while( *owner )
    {
        wp_dx11_mesh_cache *retired = *owner;
        if( retired->mesh == mesh && retired->generation != generation )
        {
            *owner = retired->next;
            wp_renderer_dx11_destroy_geometry( retired->geometry );
            free( retired );
        }
        else owner = &retired->next;
    }
    entry->mesh = mesh; entry->generation = generation; entry->first = first; entry->count = count;
    entry->geometry = geometry; entry->next = r->meshes[bucket]; r->meshes[bucket] = entry;
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

    wp_renderer_dx11_bind_output( r, wp_renderer_dx11_view_samples_linear( texture_view ) );
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

static wp_f32 wp_renderer_dx11_view_samples_linear( ID3D11ShaderResourceView *view )
{
    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    if( !view ) return 0.0f;
    ID3D11ShaderResourceView_GetDesc( view, &desc );
    switch( desc.Format )
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return 1.0f;
    default: return 0.0f;
    }
}

static wp_s32 wp_renderer_dx11_bind_geometry_pntc( wp_renderer_dx11 *r,
                                          const wp_geometry_dx11 *geometry,
                                          wp_s32 index_start, wp_s32 index_count,
                                          wp_s32 base_vertex )
{
    ID3D11ShaderResourceView *texture_view;
    UINT stride = (UINT)sizeof( wp_vertex_pntc );
    UINT offset = 0;

    wp_s32 rtt_index;
    if(r && r->rtt_active) {
        if(!wp_rtt_dx11_view_allowed(r, r->ptc_external_texture_view) ||
           !wp_rtt_dx11_view_allowed(r, r->environment_view)) return 0;
        for(rtt_index = 0; rtt_index < 6; ++rtt_index)
            if(!wp_rtt_dx11_view_allowed(r, r->material_texture_views[rtt_index])) return 0;
        for(rtt_index = 0; rtt_index < WP_TERRAIN_MATERIAL_VIEWS; ++rtt_index)
            if(!wp_rtt_dx11_view_allowed(r, r->terrain_views[rtt_index])) return 0;
    }
    if( !r || !geometry || index_start < 0 || index_count <= 0 ||
        index_start > geometry->index_count ||
        index_count > geometry->index_count - index_start )
        return 0;

    wp_renderer_dx11_apply_state( r );
    if( !wp_renderer_dx11_upload_mvp( r ) )
        return 0;
    // Derive transfer ownership from the views actually used by this draw. These
    // flags must be refreshed even when the same material switches texture views.
    texture_view = r->ptc_external_texture_view ? r->ptc_external_texture_view
                                                 : r->ptc_white_texture_view;
    if(!r->vertex_light_cb)
    {
        D3D11_BUFFER_DESC description;
        memset(&description,0,sizeof(description));description.ByteWidth=sizeof(wp_vec4f);
        description.Usage=D3D11_USAGE_DEFAULT;description.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        if(FAILED(ID3D11Device_CreateBuffer(r->device,&description,NULL,&r->vertex_light_cb))) return 0;
        memset(&r->vertex_light_values,0,sizeof(r->vertex_light_values));
        r->vertex_light_values.w=-1;
    }
    if(memcmp(&r->vertex_light_values,&r->material.light_direction,sizeof(wp_vec4f)))
    {
        ID3D11DeviceContext_UpdateSubresource(r->context,(ID3D11Resource*)r->vertex_light_cb,0,NULL,&r->material.light_direction,0,0);
        r->vertex_light_values=r->material.light_direction;
    }
    ID3D11DeviceContext_VSSetConstantBuffers(r->context,8,1,&r->vertex_light_cb);
    {
        const wp_f32 colour_linear = wp_renderer_dx11_view_samples_linear( texture_view );
        const wp_f32 emission_linear = wp_renderer_dx11_view_samples_linear( r->material_texture_views[3] );
        if( r->material.extra_map_flags.z != colour_linear ||
            r->material.extra_map_flags.w != emission_linear )
        {
            r->material.extra_map_flags.z = colour_linear;
            r->material.extra_map_flags.w = emission_linear;
            r->material_dirty = 1;
        }
    }
    if( r->material_dirty )
    {
        if( !wp_renderer_dx11_upload_material( r ) )
            return 0;
        r->material_dirty = 0;
        ++r->statistics.material_uploads;
    }
    if( r->shadow_dirty )
    {
        if( !wp_renderer_dx11_upload_constants( r, r->cb_shadow, &r->shadow, sizeof( r->shadow ) ) ) return 0;
        r->shadow_dirty = 0;
    }
    if( r->terrain_dirty )
    {
        if( !wp_renderer_dx11_upload_constants( r, r->cb_terrain, &r->terrain, sizeof( r->terrain ) ) ) return 0;
        r->terrain_dirty = 0;
    }
    // ptc_texture_view is the ImGui atlas. It is a valid fallback for UI draws,
    // but an untextured mesh must sample white rather than arbitrary font glyphs.
    texture_view = r->ptc_external_texture_view ? r->ptc_external_texture_view
                                                 : r->ptc_white_texture_view;

    if( !r->pntc_bindings_valid )
    {
        ID3D11ShaderResourceView *shadow_view = r->shadow_pass ? NULL : r->shadow_view;
        ID3D11Buffer *surface = r->material_upload_mode ? r->cb_material_surface : r->cb_material;
        ID3D11Buffer *view = r->material_upload_mode ? r->cb_material_view : NULL;
        ID3D11Buffer *bindings = r->material_upload_mode ? r->cb_material_bindings : NULL;
        ID3D11DeviceContext_PSSetConstantBuffers( r->context, 3, 1, &r->cb_terrain );
        ID3D11DeviceContext_PSSetShaderResources( r->context, 9, WP_TERRAIN_MATERIAL_VIEWS, r->terrain_views );
        ID3D11DeviceContext_PSSetSamplers( r->context, 3, 1, &r->terrain_control_sampler );
        ID3D11DeviceContext_PSSetSamplers( r->context, 4, 1, &r->pntc_sampler_state );
        ID3D11DeviceContext_PSSetConstantBuffers( r->context, 2, 1, &r->cb_shadow );
        ID3D11DeviceContext_PSSetShaderResources( r->context, 8, 1, &shadow_view );
        ID3D11DeviceContext_PSSetSamplers( r->context, 2, 1, &r->shadow_sampler );
        ID3D11DeviceContext_IASetInputLayout( r->context, r->layout_pntc );
        ID3D11DeviceContext_VSSetShader( r->context, r->vs_pntc, NULL, 0 );
        ID3D11DeviceContext_PSSetShader( r->context,
            r->material_upload_mode ? r->ps_pntc_split : r->ps_pntc, NULL, 0 );
        ID3D11DeviceContext_VSSetConstantBuffers( r->context, 1, 1, &surface );
        ID3D11DeviceContext_PSSetConstantBuffers( r->context, 1, 1, &surface );
        ID3D11DeviceContext_PSSetConstantBuffers( r->context, 6, 1, &view );
        ID3D11DeviceContext_PSSetConstantBuffers( r->context, 7, 1, &bindings );
        ID3D11DeviceContext_PSSetConstantBuffers(r->context,9,1,&r->irradiance_cb);
        ID3D11DeviceContext_PSSetSamplers( r->context, 1, 1, &r->environment_sampler_state );
        ID3D11DeviceContext_IASetPrimitiveTopology( r->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        r->statistics.state_bindings += 12;
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
    return 1;
}

wp_s32 wp_renderer_dx11_material_view_allowed(wp_renderer_dx11 *r, void *view)
{
    ID3D11RenderTargetView *outputs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView *depth = NULL;
    ID3D11Resource *input = NULL, *output = NULL;
    wp_s32 allowed = 1, i;
    if(!r || !r->context) return 0;
    if(!wp_rtt_dx11_view_allowed(r, (ID3D11ShaderResourceView *)view)) return 0;
    if(!view) return 1;
    /* Legacy Claw targets bind outputs directly, outside RTT job ownership.
     * Validate the actual context as well, before D3D silently nulls an SRV. */
    ID3D11ShaderResourceView_GetResource((ID3D11ShaderResourceView *)view, &input);
    ID3D11DeviceContext_OMGetRenderTargets(r->context,
        D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, outputs, &depth);
    for(i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) if(outputs[i])
    {
        ID3D11RenderTargetView_GetResource(outputs[i], &output);
        if(output == input) allowed = 0;
        if(output) ID3D11Resource_Release(output);
        output = NULL;
        ID3D11RenderTargetView_Release(outputs[i]);
    }
    if(depth)
    {
        ID3D11DepthStencilView_GetResource(depth, &output);
        if(output == input) allowed = 0;
        if(output) ID3D11Resource_Release(output);
        ID3D11DepthStencilView_Release(depth);
    }
    if(input) ID3D11Resource_Release(input);
    return allowed;
}

void wp_renderer_dx11_draw_geometry_pntc( wp_renderer_dx11 *r,
                                        const wp_geometry_dx11 *geometry,
                                        wp_s32 index_start, wp_s32 index_count,
                                        wp_s32 base_vertex )
{
    wp_renderer_dx11_draw_geometry_pntc_bound(r, geometry, index_start, index_count, base_vertex, NULL, NULL);
}

wp_s32 wp_renderer_dx11_draw_geometry_pntc_bound(wp_renderer_dx11 *r,
    const wp_geometry_dx11 *geometry, wp_s32 index_start, wp_s32 index_count,
    wp_s32 base_vertex, wp_material_bind_dx11 bind, void *user)
{
    if( !wp_renderer_dx11_bind_geometry_pntc( r, geometry, index_start, index_count, base_vertex ) ) return 0;
    if(bind) { r->pntc_bindings_valid = 0; if(!bind(r, 0, user)) return 0; }
    ++r->statistics.draws;
    r->statistics.triangles += (uint64_t)index_count / 3;
    ID3D11DeviceContext_DrawIndexed( r->context, (UINT)index_count, (UINT)index_start,
                                     (INT)base_vertex );
    return 1;
}

wp_s32 wp_renderer_dx11_draw_geometry_pntc_instanced(
    wp_renderer_dx11 *r, const wp_geometry_dx11 *geometry,
    wp_s32 index_start, wp_s32 index_count, wp_s32 base_vertex,
    const wp_instance_pntc_dx11 *instances, wp_s32 instance_count )
{
    return wp_renderer_dx11_draw_geometry_pntc_instanced_bound(r, geometry, index_start,
        index_count, base_vertex, instances, instance_count, NULL, NULL);
}

wp_s32 wp_renderer_dx11_draw_geometry_pntc_instanced_bound(wp_renderer_dx11 *r,
    const wp_geometry_dx11 *geometry, wp_s32 index_start, wp_s32 index_count, wp_s32 base_vertex,
    const wp_instance_pntc_dx11 *instances, wp_s32 instance_count, wp_material_bind_dx11 bind, void *user)
{
    wp_s32 i, count, submitted = 0;
    D3D11_MAPPED_SUBRESOURCE mapped;
    wp_instance_gpu_dx11 *destination;
    UINT stride = (UINT)sizeof( wp_instance_gpu_dx11 ), offset = 0;
    ID3D11Buffer *empty_buffer = NULL;
    if( !r || !geometry || geometry->device != r->device ||
        geometry->vertex_stride != sizeof( wp_vertex_pntc ) ||
        !instances || instance_count <= 0 || index_start < 0 || index_count <= 0 ||
        index_start > geometry->index_count || index_count > geometry->index_count-index_start ) return 0;
    if( !wp_renderer_dx11_validate_instances_pntc( instances, instance_count ) ) return 0;
    if( !wp_renderer_dx11_ensure_instances( r ) ) return 0;
    if( !wp_renderer_dx11_bind_geometry_pntc( r, geometry, index_start, index_count, base_vertex ) ) return 0;
    ID3D11DeviceContext_IASetInputLayout( r->context, r->layout_pntc_instanced );
    ID3D11DeviceContext_VSSetShader( r->context, r->vs_pntc_instanced, NULL, 0 );
    ID3D11DeviceContext_IASetVertexBuffers( r->context, 1, 1, &r->instance_buffer, &stride, &offset );
    r->statistics.state_bindings += 3;
    r->pntc_bindings_valid = 0;
    if(bind && !bind(r, 1, user))
    {
        ID3D11DeviceContext_IASetVertexBuffers(r->context, 1, 1, &empty_buffer, &stride, &offset);
        return 0;
    }
    while( submitted < instance_count )
    {
        count = instance_count-submitted;
        if( count > WP_DX11_INSTANCE_CAPACITY ) count = WP_DX11_INSTANCE_CAPACITY;
        if( FAILED( ID3D11DeviceContext_Map( r->context, (ID3D11Resource *)r->instance_buffer,
                                            0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) ) break;
        destination = (wp_instance_gpu_dx11 *)mapped.pData;
        for( i = 0; i < count; ++i )
            wp_renderer_dx11_pack_instance( instances+submitted+i, destination+i );
        ID3D11DeviceContext_Unmap( r->context, (ID3D11Resource *)r->instance_buffer, 0 );
        ID3D11DeviceContext_DrawIndexedInstanced( r->context, (UINT)index_count, (UINT)count,
                                                 (UINT)index_start, (INT)base_vertex, 0 );
        ++r->statistics.draws; ++r->statistics.instanced_draws;
        ++r->statistics.instance_uploads;
        r->statistics.instance_upload_bytes += (uint64_t)count*sizeof( wp_instance_gpu_dx11 );
        r->statistics.instances += (uint64_t)count;
        r->statistics.triangles += (uint64_t)(index_count/3)*(uint64_t)count;
        submitted += count;
    }
    /* Ordinary mesh/UI draws restore their own shader/layout. Release the
     * instance stream binding as well, including after a failed upload. */
    ID3D11DeviceContext_IASetVertexBuffers( r->context, 1, 1, &empty_buffer, &stride, &offset );
    ++r->statistics.state_bindings;
    return submitted;
}

wp_s32 wp_renderer_dx11_validate_foliage_instances(
    const wp_foliage_instance_dx11 *instances, wp_s32 count, const wp_foliage_parameters_dx11 *parameters )
{
    wp_s32 i, j;
    const wp_f32 *p;
    if( count < 0 || (count && !instances) || !parameters ) return 0;
    p = (const wp_f32 *)parameters;
    for( j = 0; j < 8; ++j ) if( !(p[j] >= -1000000.0f && p[j] <= 1000000.0f) ) return 0;
    if( parameters->time.y < 0 || parameters->time.y > 100 || parameters->height.y - parameters->height.x < 0.0001f ) return 0;
    for( i = 0; i < count; ++i )
    {
        if( !wp_renderer_dx11_pack_instance( &instances[i].instance, NULL ) ) return 0;
        p = (const wp_f32 *)&instances[i].uv_rect;
        for( j = 0; j < 12; ++j ) if( !(p[j] >= -1000000.0f && p[j] <= 1000000.0f) ) return 0;
        if( p[0] < 0 || p[1] < 0 || p[2] <= 0 || p[3] <= 0 || p[0]+p[2] > 1.000001f || p[1]+p[3] > 1.000001f ) return 0;
    }
    return 1;
}
wp_s32 wp_renderer_dx11_draw_foliage_instances(
    wp_renderer_dx11 *r, const wp_geometry_dx11 *geometry, wp_s32 start, wp_s32 indices,
    const wp_foliage_instance_dx11 *instances, wp_s32 total, const wp_foliage_parameters_dx11 *parameters )
{
    wp_s32 i, count, submitted = 0;
    D3D11_MAPPED_SUBRESOURCE mapped;
    wp_foliage_gpu_dx11 *destination;
    UINT stride = sizeof( wp_foliage_gpu_dx11 ), offset = 0;
    ID3D11Buffer *empty = NULL;
    if( !r || !geometry || geometry->device != r->device || total <= 0 ||
        geometry->vertex_stride != sizeof( wp_vertex_pntc ) || start < 0 || indices <= 0 ||
        start > geometry->index_count || indices > geometry->index_count-start ||
        !wp_renderer_dx11_validate_foliage_instances( instances, total, parameters ) ) return 0;
    if( !wp_renderer_dx11_ensure_foliage( r ) ||
        !wp_renderer_dx11_upload_constants( r, r->cb_foliage, parameters, sizeof(*parameters) ) ||
        !wp_renderer_dx11_bind_geometry_pntc( r, geometry, start, indices, 0 ) ) return 0;
    ID3D11DeviceContext_VSSetConstantBuffers( r->context, 4, 1, &r->cb_foliage );
    ID3D11DeviceContext_IASetInputLayout( r->context, r->layout_foliage );
    ID3D11DeviceContext_VSSetShader( r->context, r->vs_foliage, NULL, 0 );
    ID3D11DeviceContext_IASetVertexBuffers( r->context, 1, 1, &r->foliage_buffer, &stride, &offset );
    r->pntc_bindings_valid = 0; r->statistics.state_bindings += 4;
    while( submitted < total )
    {
        count = total-submitted;
        if( count > WP_DX11_INSTANCE_CAPACITY ) count = WP_DX11_INSTANCE_CAPACITY;
        if( FAILED( ID3D11DeviceContext_Map( r->context, (ID3D11Resource *)r->foliage_buffer,
                    0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) ) break;
        destination = (wp_foliage_gpu_dx11 *)mapped.pData;
        for( i = 0; i < count; ++i )
        {
            const wp_foliage_instance_dx11 *source = instances+submitted+i;
            wp_renderer_dx11_pack_instance( &source->instance, &destination[i].instance );
            destination[i].uv_rect = source->uv_rect;
            destination[i].wind = source->wind;
            destination[i].interaction = source->interaction;
        }
        ID3D11DeviceContext_Unmap( r->context, (ID3D11Resource *)r->foliage_buffer, 0 );
        ID3D11DeviceContext_DrawIndexedInstanced( r->context, (UINT)indices, (UINT)count, (UINT)start, 0, 0 );
        ++r->statistics.draws; ++r->statistics.instanced_draws; ++r->statistics.instance_uploads;
        r->statistics.instances += count;
        r->statistics.instance_upload_bytes += (uint64_t)count*sizeof( wp_foliage_gpu_dx11 );
        r->statistics.triangles += (uint64_t)(indices/3)*count;
        submitted += count;
    }
    ID3D11DeviceContext_IASetVertexBuffers( r->context, 1, 1, &empty, &stride, &offset );
    ID3D11DeviceContext_VSSetConstantBuffers( r->context, 4, 1, &empty );
    r->statistics.state_bindings += 2;
    return submitted;
}

/* Reuse a directional depth map across scene/viewport passes. */
wp_s32 wp_renderer_dx11_begin_shadow_map( wp_renderer_dx11 *r, const wp_mat4f *light_matrix, wp_s32 size )
{
    ID3D11ShaderResourceView *null_view = NULL;
    D3D11_TEXTURE2D_DESC td;
    D3D11_DEPTH_STENCIL_VIEW_DESC dd;
    D3D11_SHADER_RESOURCE_VIEW_DESC vd;
    D3D11_SAMPLER_DESC sd;
    ID3D11Texture2D *texture = NULL;
    ID3D11DepthStencilView *depth = NULL;
    ID3D11ShaderResourceView *view = NULL;
    if( !r || !light_matrix || r->shadow_pass || size < 64 || size > 8192 ) return 0;
    wp_renderer_dx11_enable_shadow_receiving( r, 0 );
    r->shadow_ready = 0;
    ID3D11DeviceContext_PSSetShaderResources( r->context, 8, 1, &null_view );
    if( !r->shadow_sampler )
    {
        memset( &sd, 0, sizeof( sd ) );
        sd.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 1.0f;
        sd.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        if( FAILED( ID3D11Device_CreateSamplerState( r->device, &sd, &r->shadow_sampler ) ) ) return 0;
    }
    if( r->shadow_size != size )
    {
        memset( &td, 0, sizeof( td ) );
        td.Width = td.Height = (UINT)size;
        td.MipLevels = td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R32_TYPELESS;
        td.SampleDesc.Count = 1;
        td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        memset( &dd, 0, sizeof( dd ) );
        dd.Format = DXGI_FORMAT_D32_FLOAT;
        dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        memset( &vd, 0, sizeof( vd ) );
        vd.Format = DXGI_FORMAT_R32_FLOAT;
        vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        vd.Texture2D.MipLevels = 1;
        if( FAILED( ID3D11Device_CreateTexture2D( r->device, &td, NULL, &texture ) ) ||
            FAILED( ID3D11Device_CreateDepthStencilView( r->device, (ID3D11Resource *)texture, &dd, &depth ) ) ||
            FAILED( ID3D11Device_CreateShaderResourceView( r->device, (ID3D11Resource *)texture, &vd, &view ) ) )
        {
            if( texture ) ID3D11Texture2D_Release( texture );
            if( depth ) ID3D11DepthStencilView_Release( depth );
            if( view ) ID3D11ShaderResourceView_Release( view );
            return 0;
        }
        if( r->shadow_texture ) ID3D11Texture2D_Release( r->shadow_texture );
        if( r->shadow_depth ) ID3D11DepthStencilView_Release( r->shadow_depth );
        if( r->shadow_view ) ID3D11ShaderResourceView_Release( r->shadow_view );
        r->shadow_texture = texture; r->shadow_depth = depth; r->shadow_view = view;
        r->shadow_size = size;
    }
    r->shadow_saved_rt = r->active_rt_view;
    r->shadow_saved_ds = r->active_ds_view;
    r->shadow_saved_viewport = r->viewport;
    r->shadow_saved_scissor = r->scissor_enabled;
    r->shadow_pass = 1;
    r->shadow.light_matrix = *light_matrix;
    r->shadow.controls.y = 1.0f;
    r->shadow.controls.z = 1.0f / size;
    r->shadow.controls.w = 0.0003f;
    r->shadow_dirty = 1;
    r->pntc_bindings_valid = 0;
    r->active_rt_view = NULL;
    r->active_ds_view = r->shadow_depth;
    ID3D11DeviceContext_OMSetRenderTargets( r->context, 0, NULL, r->shadow_depth );
    ID3D11DeviceContext_ClearDepthStencilView( r->context, r->shadow_depth, D3D11_CLEAR_DEPTH, 1.0f, 0 );
    wp_renderer_dx11_set_scissor_enabled( r, 0 );
    { wp_viewport_i viewport = { 0, 0, size, size }; wp_renderer_dx11_set_viewport( r, viewport ); }
    return 1;
}

void wp_renderer_dx11_end_shadow_map( wp_renderer_dx11 *r )
{
    if( !r || !r->shadow_pass ) return;
    r->shadow_pass = 0;
    r->shadow_ready = 1;
    r->shadow.controls.y = 0.0f;
    r->shadow_dirty = 1;
    r->pntc_bindings_valid = 0;
    r->active_rt_view = r->shadow_saved_rt;
    r->active_ds_view = r->shadow_saved_ds;
    ID3D11DeviceContext_OMSetRenderTargets( r->context, 1, &r->active_rt_view, r->active_ds_view );
    wp_renderer_dx11_set_viewport( r, r->shadow_saved_viewport );
    wp_renderer_dx11_set_scissor_enabled( r, r->shadow_saved_scissor );
}

void wp_renderer_dx11_enable_shadow_receiving( wp_renderer_dx11 *r, wp_s32 enabled )
{
    wp_f32 value;
    if( !r ) return;
    value = enabled && r->shadow_ready && !r->shadow_pass ?
            (wp_f32)(r->shadow_filter_quality > 0 ? r->shadow_filter_quality : 2) : 0.0f;
    if( r->shadow.controls.x != value )
    {
        r->shadow.controls.x = value;
        r->shadow_dirty = 1;
    }
}

void wp_renderer_dx11_set_shadow_filter_quality( wp_renderer_dx11 *r, wp_s32 quality )
{
    if(!r) return;
    r->shadow_filter_quality = quality < 1 ? 1 : quality > 4 ? 4 : quality;
    if(r->shadow.controls.x > 0)
    { r->shadow.controls.x = (wp_f32)r->shadow_filter_quality; r->shadow_dirty = 1; }
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


/* RTT execution uses a separate context state and scratch colour/depth. Native
 * input caches are restored as well as the D3D bindings; subsequent native draws
 * re-upload constants. Shader resource contents outside the destination are not
 * transactional, and callbacks must not change resource ownership/lifetime. */
typedef struct wp_rtt_dx11_context {
    wp_renderer_dx11 *r;
    wp_render_texture_dx11 *target;
    wp_rtt_check_func dependency, validate, draw;
    void *user;
    ID3D11DeviceContext1 *context1;
    ID3DDeviceContextState *previous;
    ID3D11RenderTargetView *old_rt;
    ID3D11DepthStencilView *old_ds;
    unsigned char inputs[offsetof(wp_renderer_dx11, rs_cache) - offsetof(wp_renderer_dx11, clear_r)];
    wp_material_dx11 material;
    wp_terrain_material_dx11 terrain;
    wp_shadow_constants_dx11 shadow;
    ID3D11ShaderResourceView *material_views[6], *terrain_views[WP_TERRAIN_MATERIAL_VIEWS];
    ID3D11ShaderResourceView *external, *environment;
    ID3D11SamplerState *material_sampler;
    wp_f32 environment_lod;
    D3D11_VIEWPORT ui_viewport;
    wp_s32 started;
} wp_rtt_dx11_context;

static wp_s32 wp_rtt_dx11_view_matches(ID3D11ShaderResourceView *view,
                                      wp_render_texture_dx11 *target)
{
    ID3D11Resource *resource = NULL;
    wp_s32 match;
    if(!view) return 0;
    ID3D11ShaderResourceView_GetResource(view, &resource);
    match = resource == (ID3D11Resource *)target->texture ||
            resource == (ID3D11Resource *)target->ds_texture;
    if(resource) ID3D11Resource_Release(resource);
    return match;
}

static wp_s32 wp_rtt_dx11_view_allowed(wp_renderer_dx11 *r, ID3D11ShaderResourceView *view)
{
    if(r && r->rtt_active && wp_rtt_dx11_view_matches(view, r->rtt_destination)) {
        r->rtt_hazard = 1;
        return 0;
    }
    return 1;
}

static wp_s32 wp_rtt_dx11_sampled(wp_renderer_dx11 *r, wp_render_texture_dx11 *target)
{
    ID3D11ShaderResourceView *views[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11UnorderedAccessView *uavs[D3D11_PS_CS_UAV_REGISTER_COUNT];
    ID3D11Resource *resource;
    wp_s32 stage, i, match = 0;
    for(stage = 0; stage < 6; ++stage) {
        memset(views, 0, sizeof(views));
        switch(stage) {
        case 0: ID3D11DeviceContext_VSGetShaderResources(r->context, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views); break;
        case 1: ID3D11DeviceContext_PSGetShaderResources(r->context, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views); break;
        case 2: ID3D11DeviceContext_GSGetShaderResources(r->context, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views); break;
        case 3: ID3D11DeviceContext_HSGetShaderResources(r->context, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views); break;
        case 4: ID3D11DeviceContext_DSGetShaderResources(r->context, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views); break;
        default: ID3D11DeviceContext_CSGetShaderResources(r->context, 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views); break;
        }
        for(i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i) if(views[i]) {
            if(wp_rtt_dx11_view_matches(views[i], target)) match = 1;
            ID3D11ShaderResourceView_Release(views[i]);
        }
    }
    if(ID3D11Device_GetFeatureLevel(r->device) >= D3D_FEATURE_LEVEL_11_0) {
        for(stage = 0; stage < 2; ++stage) {
            memset(uavs, 0, sizeof(uavs));
            if(!stage) ID3D11DeviceContext_CSGetUnorderedAccessViews(r->context, 0, D3D11_PS_CS_UAV_REGISTER_COUNT, uavs);
            else ID3D11DeviceContext_OMGetRenderTargetsAndUnorderedAccessViews(r->context, 0, NULL, NULL, 0, D3D11_PS_CS_UAV_REGISTER_COUNT, uavs);
            for(i = 0; i < D3D11_PS_CS_UAV_REGISTER_COUNT; ++i) if(uavs[i]) {
                resource = NULL;
                ID3D11UnorderedAccessView_GetResource(uavs[i], &resource);
                if(resource == (ID3D11Resource *)target->texture || resource == (ID3D11Resource *)target->ds_texture) match = 1;
                if(resource) ID3D11Resource_Release(resource);
                ID3D11UnorderedAccessView_Release(uavs[i]);
            }
        }
    }
    return match;
}

static wp_rtt_result wp_rtt_dx11_dependency(void *data, const wp_graphics_rtt_desc *desc)
{
    wp_rtt_dx11_context *c = (wp_rtt_dx11_context *)data;
    return c->dependency ? c->dependency(c->user, desc) : WP_RTT_OK;
}
static wp_rtt_result wp_rtt_dx11_validate(void *data, const wp_graphics_rtt_desc *desc)
{
    wp_rtt_dx11_context *c = (wp_rtt_dx11_context *)data;
    wp_renderer_dx11 *r = c->r;
    ID3D11Device *device = NULL;
    wp_s32 same;
    if(r->rtt_owner_thread && r->rtt_owner_thread != GetCurrentThreadId()) return WP_RTT_WRONG_THREAD;
    if(desc->renderer_generation != r->rtt_generation || desc->target_generation != c->target->generation ||
       desc->width != c->target->width || desc->height != c->target->height) return WP_RTT_STALE;
    ID3D11Texture2D_GetDevice(c->target->texture, &device);
    same = device == r->device;
    if(device) ID3D11Device_Release(device);
    if(!same || FAILED(ID3D11Device_GetDeviceRemovedReason(r->device))) return WP_RTT_STALE;
    if((c->started && r->rtt_hazard) || wp_rtt_dx11_sampled(r, c->target)) return WP_RTT_HAZARD;
    if(!c->started && (r->rtt_active || r->hdr_active || r->shadow_pass)) return WP_RTT_BUSY;
    return c->validate ? c->validate(c->user, desc) : WP_RTT_OK;
}
static wp_rtt_result wp_rtt_dx11_begin(void *data, const wp_graphics_rtt_desc *desc)
{
    wp_rtt_dx11_context *c = (wp_rtt_dx11_context *)data;
    wp_renderer_dx11 *r = c->r;
    wp_render_texture_dx11 *target = c->target;
    ID3D11Device1 *device1 = NULL;
    D3D_FEATURE_LEVEL level;
    HRESULT hr;
    if((wp_u64)desc->width * (wp_u64)desc->height > (256u * 1024u * 1024u) / 8u) return WP_RTT_INVALID;
    if(!target->scratch) {
        target->scratch = wp_renderer_dx11_create_render_texture(r, target->width, target->height);
        if(!target->scratch) return WP_RTT_BACKEND_FAILED;
    }
    hr = ID3D11DeviceContext_QueryInterface(r->context, &IID_ID3D11DeviceContext1, (void **)&c->context1);
    if(FAILED(hr)) return WP_RTT_UNSUPPORTED;
    if(!target->rtt_state) {
        hr = ID3D11Device_QueryInterface(r->device, &IID_ID3D11Device1, (void **)&device1);
        if(FAILED(hr)) return WP_RTT_UNSUPPORTED;
        level = ID3D11Device_GetFeatureLevel(r->device);
        hr = ID3D11Device1_CreateDeviceContextState(device1, 0, &level, 1, D3D11_SDK_VERSION,
                                                    &IID_ID3D11Device, NULL, &target->rtt_state);
        ID3D11Device1_Release(device1);
        if(FAILED(hr)) return WP_RTT_UNSUPPORTED;
    }
    memcpy(c->inputs, &r->clear_r, sizeof(c->inputs));
    c->ui_viewport = r->viewport_d3d;
    c->material = r->material; c->terrain = r->terrain; c->shadow = r->shadow;
    memcpy(c->material_views, r->material_texture_views, sizeof(c->material_views));
    memcpy(c->terrain_views, r->terrain_views, sizeof(c->terrain_views));
    c->external = r->ptc_external_texture_view; c->environment = r->environment_view;
    c->environment_lod = r->environment_max_lod; c->material_sampler = r->material_sampler;
    c->old_rt = r->active_rt_view; c->old_ds = r->active_ds_view;
    ID3D11DeviceContext1_SwapDeviceContextState(c->context1, target->rtt_state, &c->previous);
    c->started = 1; r->rtt_destination = target; r->rtt_hazard = 0; r->rtt_active = 1;
    if(!r->rtt_owner_thread) r->rtt_owner_thread = GetCurrentThreadId();
    ID3D11DeviceContext_CopyResource(r->context, (ID3D11Resource *)target->scratch->texture, (ID3D11Resource *)target->texture);
    ID3D11DeviceContext_CopyResource(r->context, (ID3D11Resource *)target->scratch->ds_texture, (ID3D11Resource *)target->ds_texture);
    wp_renderer_dx11_invalidate_state(r);
    r->mvp_dirty = r->material_dirty = r->terrain_dirty = r->shadow_dirty = 1;
    wp_renderer_dx11_set_render_texture(r, target->scratch);
    wp_renderer_dx11_set_viewport(r, desc->viewport);
    wp_renderer_dx11_set_clear_color(r, desc->colour[0], desc->colour[1], desc->colour[2], desc->colour[3]);
    wp_renderer_dx11_set_clear_depth(r, desc->depth);
    wp_renderer_dx11_clear(r, (desc->clear_colour ? WORKPHONE_CLEAR_FLAG_COLOR : 0) |
                               (desc->clear_depth ? WORKPHONE_CLEAR_FLAG_DEPTH : 0));
    return WP_RTT_OK;
}
static wp_rtt_result wp_rtt_dx11_draw(void *data, const wp_graphics_rtt_desc *desc)
{
    wp_rtt_dx11_context *c = (wp_rtt_dx11_context *)data;
    return c->draw(c->user, desc);
}
static wp_rtt_result wp_rtt_dx11_finish(void *data, wp_s32 publish)
{
    wp_rtt_dx11_context *c = (wp_rtt_dx11_context *)data;
    wp_renderer_dx11 *r = c->r;
    wp_rtt_result result = WP_RTT_OK;
    if(c->started) {
        if(r->hdr_active || r->shadow_pass) {
            publish = 0; result = WP_RTT_BACKEND_FAILED;
            if(r->hdr_active) wp_renderer_dx11_end_hdr_scene(r);
            else if(r->shadow_pass) wp_renderer_dx11_end_shadow_map(r);
        }
        if(publish) {
            ID3D11DeviceContext_OMSetRenderTargets(r->context, 0, NULL, NULL);
            ID3D11DeviceContext_CopyResource(r->context, (ID3D11Resource *)c->target->texture, (ID3D11Resource *)c->target->scratch->texture);
            ID3D11DeviceContext_CopyResource(r->context, (ID3D11Resource *)c->target->ds_texture, (ID3D11Resource *)c->target->scratch->ds_texture);
        }
        ID3D11DeviceContext_ClearState(r->context);
        ID3D11DeviceContext1_SwapDeviceContextState(c->context1, c->previous, NULL);
        ID3DDeviceContextState_Release(c->previous); c->previous = NULL;
        memcpy(&r->clear_r, c->inputs, sizeof(c->inputs)); r->viewport_d3d = c->ui_viewport;
        r->material = c->material; r->terrain = c->terrain; r->shadow = c->shadow;
        memcpy(r->material_texture_views, c->material_views, sizeof(c->material_views));
        memcpy(r->terrain_views, c->terrain_views, sizeof(c->terrain_views));
        r->ptc_external_texture_view = c->external; r->environment_view = c->environment;
        r->environment_max_lod = c->environment_lod; r->material_sampler = c->material_sampler;
        r->active_rt_view = c->old_rt; r->active_ds_view = c->old_ds;
        r->mvp_dirty = r->material_dirty = r->terrain_dirty = r->shadow_dirty = 1;
        wp_renderer_dx11_invalidate_state(r);
        r->rtt_active = 0; r->rtt_destination = NULL; r->rtt_hazard = 0;
    }
    if(c->context1) { ID3D11DeviceContext1_Release(c->context1); c->context1 = NULL; }
    return result;
}
wp_u64 wp_renderer_dx11_get_rtt_generation(const wp_renderer_dx11 *r)
{ return r ? r->rtt_generation : 0; }
wp_u64 wp_renderer_dx11_get_render_texture_generation(const wp_render_texture_dx11 *target)
{ return target ? target->generation : 0; }
wp_rtt_result wp_renderer_dx11_execute_rtt_job(wp_renderer_dx11 *r,
    wp_render_texture_dx11 *target, const wp_graphics_rtt_desc *desc,
    wp_rtt_check_func dependency, wp_rtt_check_func validate, wp_rtt_check_func draw, void *user)
{
    wp_graphics_rtt_job *job;
    wp_graphics_rtt_ops ops;
    wp_rtt_dx11_context context;
    wp_rtt_result result;
    wp_s32 i;
    if(!r || !target || !desc || !draw || !(desc->depth >= 0.0f && desc->depth <= 1.0f)) return WP_RTT_INVALID;
    if(desc->format != WP_RTT_RGBA8_UNORM || desc->colour_space != WP_RTT_LEGACY_LDR) return WP_RTT_UNSUPPORTED;
    for(i = 0; i < 4; ++i) if(!(desc->colour[i] >= -FLT_MAX && desc->colour[i] <= FLT_MAX)) return WP_RTT_INVALID;
    memset(&context, 0, sizeof(context)); memset(&ops, 0, sizeof(ops));
    context.r = r; context.target = target; context.user = user;
    context.dependency = dependency; context.validate = validate; context.draw = draw;
    ops.dependency = wp_rtt_dx11_dependency; ops.validate = wp_rtt_dx11_validate;
    ops.begin = wp_rtt_dx11_begin; ops.draw = wp_rtt_dx11_draw; ops.finish = wp_rtt_dx11_finish;
    job = wp_graphics_rtt_job_create();
    if(!job) return WP_RTT_BACKEND_FAILED;
    result = wp_graphics_rtt_job_configure(job, desc, &ops, &context);
    if(result == WP_RTT_OK) result = wp_graphics_rtt_job_execute(job);
    wp_graphics_rtt_job_destroy(job);
    return result;
}

#endif /* _WIN32 */
