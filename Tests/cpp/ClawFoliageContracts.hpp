#pragma once

#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <workphone_graphics_renderer.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace claw_foliage_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using Microsoft::WRL::ComPtr;

    inline void require( bool value, const char *message )
    {
        if( !value ) throw std::runtime_error( message );
    }

    inline wp_mat4f identity()
    {
        wp_mat4f result{};
        for( int i=0; i<4; ++i ) result.m[i][i]=1;
        return result;
    }

    inline wp_instance_pntc_dx11 instance( float x, float y, float scale, wp_vec4f tint )
    {
        wp_instance_pntc_dx11 result{ identity(), tint };
        result.transform.m[0][0]=result.transform.m[1][1]=result.transform.m[2][2]=scale;
        result.transform.m[0][3]=x; result.transform.m[1][3]=y;
        return result;
    }

    using Geometry = std::unique_ptr<wp_geometry_dx11, decltype(&wp_renderer_dx11_destroy_geometry)>;
    inline Geometry geometry( wp_renderer_dx11 *native, const std::vector<wp_vertex_pntc> &vertices,
                              const std::vector<wp_u32> &indices )
    {
        Geometry result( wp_renderer_dx11_create_indexed_geometry_pntc( native, vertices.data(),
            static_cast<wp_s32>(vertices.size()), indices.data(), static_cast<wp_s32>(indices.size()), 1 ),
            &wp_renderer_dx11_destroy_geometry );
        require( result != nullptr, "foliage immutable shared geometry must allocate" );
        return result;
    }

    inline wp_render_statistics_dx11 statistics( wp_renderer_dx11 *native )
    {
        wp_render_statistics_dx11 result{};
        wp_renderer_dx11_get_statistics( native, &result );
        return result;
    }

    inline std::vector<unsigned char> pixels( wp_renderer_dx11 *native )
    {
        auto *device=static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));
        auto *context=static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(native));
        ComPtr<ID3D11RenderTargetView> view;
        context->OMGetRenderTargets(1,view.GetAddressOf(),nullptr);
        require( view != nullptr, "foliage colour target must be bound" );
        ComPtr<ID3D11Resource> resource; view->GetResource(resource.GetAddressOf());
        ComPtr<ID3D11Texture2D> source;
        require( SUCCEEDED(resource.As(&source)), "foliage colour target must be a texture" );
        D3D11_TEXTURE2D_DESC desc{}; source->GetDesc(&desc);
        require( desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM, "foliage fixture requires RGBA8" );
        desc.Usage=D3D11_USAGE_STAGING; desc.BindFlags=0; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging;
        require( SUCCEEDED(device->CreateTexture2D(&desc,nullptr,staging.GetAddressOf())),
                 "foliage readback must allocate" );
        context->CopyResource(staging.Get(),source.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        require( SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)),
                 "foliage colour pixels must map" );
        std::vector<unsigned char> result(static_cast<size_t>(desc.Width)*desc.Height*4);
        for( UINT y=0; y<desc.Height; ++y )
            std::memcpy(result.data()+y*desc.Width*4,
                static_cast<const unsigned char *>(mapped.pData)+y*mapped.RowPitch,desc.Width*4);
        context->Unmap(staging.Get(),0);
        return result;
    }

    inline void equivalent( const std::vector<unsigned char> &a,
                            const std::vector<unsigned char> &b, const char *message )
    {
        require(a.size()==b.size(),message);
        size_t changed=0, covered=0;
        for( size_t i=0; i<a.size(); i+=4 )
        {
            bool different=false;
            for( size_t c=0; c<3; ++c ) different |= std::abs(int(a[i+c])-int(b[i+c]))>2;
            changed += different;
            covered += a[i] || a[i+1] || a[i+2];
        }
        require( covered>100, "foliage comparisons must contain visible geometry" );
        require( changed<=a.size()/4/200, message ); // At most 0.5% raster boundary rounding.
    }

    inline float shadowDepth( wp_renderer_dx11 *native, UINT x, UINT y )
    {
        auto *device=static_cast<ID3D11Device *>(wp_renderer_dx11_get_device(native));
        auto *context=static_cast<ID3D11DeviceContext *>(wp_renderer_dx11_get_context(native));
        ComPtr<ID3D11DepthStencilView> view;
        context->OMGetRenderTargets(0,nullptr,view.GetAddressOf());
        require(view != nullptr,"foliage shadow pass must bind a depth target");
        ComPtr<ID3D11Resource> resource; view->GetResource(resource.GetAddressOf());
        ComPtr<ID3D11Texture2D> source;
        require(SUCCEEDED(resource.As(&source)),"foliage shadow target must be a texture");
        D3D11_TEXTURE2D_DESC desc{}; source->GetDesc(&desc);
        require(desc.Format==DXGI_FORMAT_R32_TYPELESS && x<desc.Width && y<desc.Height,
                "foliage shadow readback must expose bounded 32-bit depth");
        desc.Usage=D3D11_USAGE_STAGING; desc.BindFlags=0; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging;
        require(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,staging.GetAddressOf())),
                "foliage shadow readback must allocate");
        context->CopyResource(staging.Get(),source.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        require(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)),
                "foliage shadow pixels must map");
        const auto value=reinterpret_cast<const float *>(
            static_cast<const unsigned char *>(mapped.pData)+y*mapped.RowPitch)[x];
        context->Unmap(staging.Get(),0);
        return value;
    }

    inline wp_material_dx11 material( wp_vec4f colour )
    {
        wp_material_dx11 result{};
        result.base_color=colour; result.surface={0,0.7f,1,1}; result.controls={1,1,-1,0};
        return result; // Unlit locks comparison lighting; alpha path remains active.
    }

    inline void setup( wp_renderer_dx11 *native )
    {
        const auto unit=identity();
        wp_renderer_dx11_set_world_matrix(native,&unit);
        wp_renderer_dx11_set_view_matrix(native,&unit);
        wp_renderer_dx11_set_projection_matrix(native,&unit);
        wp_renderer_dx11_set_cull_mode(native,WORKPHONE_CULL_MODE_NONE);
        wp_renderer_dx11_set_depth_test_enabled(native,0);
        wp_renderer_dx11_set_depth_write_enabled(native,0);
        wp_renderer_dx11_set_blend_mode(native,WORKPHONE_BLEND_MODE_NONE);
        wp_renderer_dx11_set_texture_native(native,nullptr);
        wp_renderer_dx11_set_material_textures(native,nullptr);
        wp_renderer_dx11_enable_shadow_receiving(native,0);
    }

    inline void validateInputs()
    {
        require(wp_renderer_dx11_validate_instances_pntc(nullptr,0),"empty instance population must be valid");
        require(!wp_renderer_dx11_validate_instances_pntc(nullptr,1) &&
                !wp_renderer_dx11_validate_instances_pntc(nullptr,-1),"invalid populations must reject");
        auto valid=instance(0,0,1,{1,1,1,1});
        valid.transform.m[0][0]=2; valid.transform.m[1][1]=0.5f;
        require(wp_renderer_dx11_validate_instances_pntc(&valid,1),"positive nonuniform scale must validate");
        auto invalid=valid; invalid.transform.m[0][0]=0;
        require(!wp_renderer_dx11_validate_instances_pntc(&invalid,1),"singular transform must reject");
        invalid=valid; invalid.transform.m[0][0]=-2;
        require(!wp_renderer_dx11_validate_instances_pntc(&invalid,1),"reflected transform must reject");
        invalid=valid; invalid.transform.m[3][0]=0.1f;
        require(!wp_renderer_dx11_validate_instances_pntc(&invalid,1),"projective instance must reject");
        invalid=valid; invalid.tint.x=std::numeric_limits<float>::quiet_NaN();
        require(!wp_renderer_dx11_validate_instances_pntc(&invalid,1),"nonfinite tint must reject");
        invalid=valid; invalid.transform.m[2][3]=std::numeric_limits<float>::infinity();
        require(!wp_renderer_dx11_validate_instances_pntc(&invalid,1),"nonfinite position must reject");
    }

    inline void distinctInstances( wp_renderer_dx11 *native )
    {
        const std::vector<wp_vertex_pntc> vertices={
            {{-0.18f,-0.18f,0},{0.70710678f,0,0.70710678f},{0,0},0xffffffff},
            {{ 0.18f,-0.18f,0},{0.70710678f,0,0.70710678f},{1,0},0xffffffff},
            {{ 0, 0.18f,0},{0.70710678f,0,0.70710678f},{0.5f,1},0xffffffff}};
        auto shared=geometry(native,vertices,{0,1,2});
        auto plain=material({1,1,1,1}); wp_renderer_dx11_set_material(native,&plain);
        std::vector<wp_instance_pntc_dx11> instances={instance(-0.5f,0,1,{1,0,0,1}),
                                                  instance(0.5f,0,1,{0,1,0,1})};
        const auto before=statistics(native);
        require(wp_renderer_dx11_draw_geometry_pntc_instanced(native,shared.get(),0,3,0,nullptr,0)==0,
                "zero instances must issue no draw");
        instances[1].transform.m[3][3]=0;
        require(wp_renderer_dx11_draw_geometry_pntc_instanced(native,shared.get(),0,3,0,instances.data(),2)==0 &&
                statistics(native).draws==before.draws,"invalid trailing instance must reject before any draw");
        instances[1].transform.m[3][3]=1;
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        require(wp_renderer_dx11_draw_geometry_pntc_instanced(native,shared.get(),0,3,0,instances.data(),2)==2,
                "two independent instances must submit");
        auto blue=material({0,0,1,1}); wp_renderer_dx11_set_material(native,&blue);
        wp_renderer_dx11_draw_geometry_pntc(native,shared.get(),0,3,0);
        const auto frame=pixels(native);
        require(frame[(64*128+32)*4]>200 && frame[(64*128+32)*4+1]<10 &&
                frame[(64*128+96)*4+1]>200 && frame[(64*128+96)*4]<10,
                "instances must retain distinct transforms and tints on the GPU");
        require(frame[(64*128+64)*4+2]>200,"ordinary draw after instancing must restore its shader/layout");
        const auto after=statistics(native);
        require(after.draws-before.draws==2 && after.instanced_draws-before.instanced_draws==1 &&
                after.instances-before.instances==2 && after.triangles-before.triangles==3,
                "native statistics must count actual instances and multiplied triangles");

        plain.controls.z=0.5f; wp_renderer_dx11_set_material(native,&plain);
        instances[0].tint.w=0;
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        require(wp_renderer_dx11_draw_geometry_pntc_instanced(native,shared.get(),0,3,0,instances.data(),2)==2,
                "cutout instance draw must submit");
        const auto cutout=pixels(native);
        require(cutout[(64*128+32)*4]==0 && cutout[(64*128+96)*4+1]>200,
                "instance alpha must participate in the real cutout material shader");

        const auto light=identity();
        require(wp_renderer_dx11_begin_shadow_map(native,&light,128),"foliage shadow pass must begin");
        struct ShadowScope { wp_renderer_dx11 *native; ~ShadowScope() {
            wp_renderer_dx11_end_shadow_map(native); } } shadowScope{native};
        wp_renderer_dx11_set_depth_test_enabled(native,1);
        wp_renderer_dx11_set_depth_write_enabled(native,1);
        wp_renderer_dx11_set_depth_func(native,WORKPHONE_DEPTH_FUNC_LESS);
        const auto shadowBefore=statistics(native);
        require(wp_renderer_dx11_draw_geometry_pntc_instanced(native,shared.get(),0,3,0,instances.data(),2)==2,
                "foliage shadow instances must submit through the shared cutout shader");
        require(shadowDepth(native,32,64)>0.99f && shadowDepth(native,96,64)<0.51f,
                "cutout instance colour and actual shadow-depth coverage must agree");
        require(statistics(native).draws-shadowBefore.draws==1 &&
                statistics(native).instances-shadowBefore.instances==2,
                "shadow draws and submitted instances must be counted separately from colour");
        wp_renderer_dx11_end_shadow_map(native);
        wp_renderer_dx11_set_depth_test_enabled(native,0);
        wp_renderer_dx11_set_depth_write_enabled(native,0);

        // Compare actual lit pixels against scalar draws under rotation and
        // nonuniform scale; an untransformed normal cannot satisfy this check.
        auto transformed=instance(0,0,1,{1,1,1,1});
        transformed.transform.m[0][0]=1.6f*0.8660254f;
        transformed.transform.m[0][2]=0.5f*0.4f;
        transformed.transform.m[2][0]=-0.5f*1.6f;
        transformed.transform.m[2][2]=0.8660254f*0.4f;
        transformed.transform.m[1][1]=0.8f;
        plain.controls.z=-1; plain.uv_transform.z=1; plain.light_color={1,1,1,3};
        plain.light_direction={-0.5f,0,-1,0}; plain.camera_position={0,0,3,0.05f};
        plain.specular_color={0.04f,0.04f,0.04f,0}; wp_renderer_dx11_set_material(native,&plain);
        wp_renderer_dx11_set_world_matrix(native,&transformed.transform);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        wp_renderer_dx11_draw_geometry_pntc(native,shared.get(),0,3,0);
        const auto scalar=pixels(native);
        const auto unit=identity(); wp_renderer_dx11_set_world_matrix(native,&unit);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        require(wp_renderer_dx11_draw_geometry_pntc_instanced(native,shared.get(),0,3,0,&transformed,1)==1,
                "one transformed lit instance must submit");
        equivalent(scalar,pixels(native),"instance inverse-transpose normals must match scalar lit output");
    }

    inline void drawBudget( wp_renderer_dx11 *native )
    {
        const std::vector<wp_vertex_pntc> vertices={
            {{-0.08f,-0.5f,0},{0,0,1},{0,0},0xffffffff},
            {{0.08f,-0.5f,0},{0,0,1},{1,0},0xffffffff},
            {{0.08f,0,0},{0,0,1},{1,1},0xffffffff},
            {{-0.08f,0,0},{0,0,1},{0,1},0xffffffff},
            {{-0.45f,-0.1f,0},{0,0,1},{0,0},0xffffffff},
            {{0.45f,-0.1f,0},{0,0,1},{1,0},0xffffffff},
            {{0,0.5f,0},{0,0,1},{0.5f,1},0xffffffff}};
        const std::vector<wp_u32> indices={0,1,2,0,2,3,4,5,6};
        auto shared=geometry(native,vertices,indices);
        std::vector<wp_instance_pntc_dx11> instances;
        for(int y=0;y<100;++y) for(int x=0;x<100;++x)
            instances.push_back(instance(-0.99f+x*0.02f,-0.99f+y*0.02f,0.017f,{1,1,1,1}));
        const wp_material_dx11 sections[]={material({0.55f,0.25f,0.08f,1}),material({0.15f,0.85f,0.2f,1})};
        const auto unit=identity();
        const auto start=std::chrono::steady_clock::now();
        const auto scalarBefore=statistics(native);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        for(int section=0;section<2;++section)
        {
            wp_renderer_dx11_set_material(native,&sections[section]);
            for(const auto &item:instances)
            {
                wp_renderer_dx11_set_world_matrix(native,&item.transform);
                wp_renderer_dx11_draw_geometry_pntc(native,shared.get(),section?6:0,section?3:6,0);
            }
        }
        const auto scalarEnd=std::chrono::steady_clock::now();
        const auto scalarAfter=statistics(native);
        const auto scalar=pixels(native);
        require(scalarAfter.draws-scalarBefore.draws==20000 &&
                scalarAfter.triangles-scalarBefore.triangles==30000,
                "scalar two-section population must actually submit twenty thousand draws");
        wp_renderer_dx11_set_world_matrix(native,&unit);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        const auto batchBefore=statistics(native);
        const auto batchStart=std::chrono::steady_clock::now();
        for(int section=0;section<2;++section)
        {
            wp_renderer_dx11_set_material(native,&sections[section]);
            require(wp_renderer_dx11_draw_geometry_pntc_instanced(native,shared.get(),section?6:0,section?3:6,0,
                    instances.data(),static_cast<wp_s32>(instances.size()))==10000,
                    "every capacity chunk must submit all requested instances");
        }
        const auto batchEnd=std::chrono::steady_clock::now();
        const auto batchAfter=statistics(native);
        equivalent(scalar,pixels(native),"instanced forest must preserve scalar coverage and colour");
        require(batchAfter.draws-batchBefore.draws==6 && batchAfter.instanced_draws-batchBefore.instanced_draws==6 &&
                batchAfter.instances-batchBefore.instances==20000 &&
                batchAfter.triangles-batchBefore.triangles==30000 &&
                batchAfter.instance_uploads-batchBefore.instance_uploads==6 &&
                batchAfter.geometry_creations==batchBefore.geometry_creations &&
                batchAfter.instance_buffer_creations==batchBefore.instance_buffer_creations,
                "ten thousand two-section instances must reuse resources and issue exactly six draws");

        std::vector<wp_vertex_pntc> mergedVertices; std::vector<wp_u32> mergedIndices;
        for(const auto &item:instances)
            for(auto vertex:vertices)
            {
                vertex.position.x=vertex.position.x*item.transform.m[0][0]+item.transform.m[0][3];
                vertex.position.y=vertex.position.y*item.transform.m[1][1]+item.transform.m[1][3];
                mergedVertices.push_back(vertex);
            }
        for(int section=0;section<2;++section) for(wp_u32 i=0;i<instances.size();++i)
            for(int index=section?6:0;index<(section?9:6);++index) mergedIndices.push_back(i*7+indices[index]);
        auto merged=geometry(native,mergedVertices,mergedIndices);
        wp_renderer_dx11_clear(native,WORKPHONE_CLEAR_FLAG_ALL);
        const auto mergedBefore=statistics(native);
        for(int section=0;section<2;++section)
        {
            wp_renderer_dx11_set_material(native,&sections[section]);
            wp_renderer_dx11_draw_geometry_pntc(native,merged.get(),section?60000:0,section?30000:60000,0);
        }
        equivalent(scalar,pixels(native),"static merged reference must preserve the same population coverage");
        require(statistics(native).draws-mergedBefore.draws==2,"ideal static merge must submit two material draws");
        std::printf("Foliage controlled colour view: 10000 trees, two sections; scalar=20000 draws, "
                    "instanced=6, ideal-static-merge=2, triangles=30000, capacity=4096; "
                    "scalar submission %.3f ms, instanced submission %.3f ms; "
                    "shared geometry %zu bytes, merged geometry %zu bytes, instance upload %llu bytes.\n",
            std::chrono::duration<double,std::milli>(scalarEnd-start).count(),
            std::chrono::duration<double,std::milli>(batchEnd-batchStart).count(),
            vertices.size()*sizeof(wp_vertex_pntc)+indices.size()*sizeof(wp_u32),
            mergedVertices.size()*sizeof(wp_vertex_pntc)+mergedIndices.size()*sizeof(wp_u32),
            static_cast<unsigned long long>(batchAfter.instance_upload_bytes-batchBefore.instance_upload_bytes));
    }

    inline bool run( ClawRendererDX11 &renderer )
    {
        struct Scope { ClawRendererDX11 &renderer; ~Scope() { renderer.disableShadows();
            renderer.setCamera(nullptr); renderer.setRenderTarget(nullptr); } } scope{renderer};
        try
        {
            validateInputs();
            auto target=make_ptr<ClawRenderTarget>(); target->setSize({128,128});
            renderer.setRenderTarget(target); renderer.setViewport(nullptr); renderer.setCamera(nullptr);
            renderer.disableShadows();
            auto *native=wp_renderer_get_dx11(renderer.getNativeRenderer());
            setup(native); distinctInstances(native);
            target=make_ptr<ClawRenderTarget>(); target->setSize({256,256});
            renderer.setRenderTarget(target); setup(native); drawBudget(native);
            std::puts("Foliage instance validation, pixels, material state, normals and native draw budgets passed.");
            return true;
        }
        catch(const std::exception &error)
        {
            std::fprintf(stderr,"Foliage instance contract failed: %s\n",error.what()); return false;
        }
    }
}
