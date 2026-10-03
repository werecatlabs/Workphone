#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawRendererDX12.hpp>
#include <Workphone/Workphone.hpp>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawRendererDX12, IRenderer3 );

    // ── Embedded HLSL shaders ──────────────────────────────────────────────

    static constexpr char k_colourShaderSrc[] =
        "#pragma pack_matrix(row_major)\n"
        "cbuffer CBTransform : register(b0) { float4x4 g_wvp; };\n"
        "struct VSIn  { float3 pos : POSITION; float4 col : COLOR; };\n"
        "struct VSOut { float4 pos : SV_POSITION; float4 col : COLOR; };\n"
        "VSOut VSMain(VSIn v) {\n"
        "    VSOut o;\n"
        "    o.pos = mul(g_wvp, float4(v.pos, 1.0f));\n"
        "    o.col = v.col;\n"
        "    return o;\n"
        "}\n"
        "float4 PSMain(VSOut p) : SV_TARGET { return p.col; }\n";

    static constexpr char k_textureShaderSrc[] =
        "#pragma pack_matrix(row_major)\n"
        "cbuffer CBTransform : register(b0) { float4x4 g_wvp; };\n"
        "Texture2D    g_tex : register(t0);\n"
        "SamplerState g_smp : register(s0);\n"
        "struct VSIn  { float3 pos : POSITION; float2 uv : TEXCOORD0; float4 col : COLOR; };\n"
        "struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 col : COLOR; };\n"
        "VSOut VSMain(VSIn v) {\n"
        "    VSOut o;\n"
        "    o.pos = mul(g_wvp, float4(v.pos, 1.0f));\n"
        "    o.uv  = v.uv;\n"
        "    o.col = v.col;\n"
        "    return o;\n"
        "}\n"
        "float4 PSMain(VSOut p) : SV_TARGET {\n"
        "    return g_tex.Sample(g_smp, p.uv) * p.col;\n"
        "}\n";

    // ── Vertex layouts ─────────────────────────────────────────────────────

    struct VertexPC
    {
        float pos[3];
        float col[4];
    };

    struct VertexPUC
    {
        float pos[3];
        float uv[2];
        float col[4];
    };

    // ── Buffer sizes ───────────────────────────────────────────────────────

    static constexpr u32 k_lineVBSize = sizeof( VertexPC ) * 2;
    static constexpr u32 k_colourQuadVBSize = sizeof( VertexPC ) * 4;
    static constexpr u32 k_quadVBSize = sizeof( VertexPUC ) * 4;
    static constexpr u32 k_quadIBSize = sizeof( u16 ) * 6;
    static constexpr u32 k_cbSize = 256;  // CBV must be 256-byte aligned in DX12

    // ── Helpers ────────────────────────────────────────────────────────────

    template <typename T>
    void ClawRendererDX12::safeRelease( T *&ptr )
    {
        if( ptr )
        {
            ptr->Release();
            ptr = nullptr;
        }
    }

    // ── Construction / destruction ─────────────────────────────────────────

    ClawRendererDX12::ClawRendererDX12() = default;

    ClawRendererDX12::~ClawRendererDX12()
    {
        releaseResources();
    }

    // ── ISharedObject lifecycle ────────────────────────────────────────────

    void ClawRendererDX12::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        WP_LOG_INFO( "WPGraphics/DX12: initialization started." );
        const auto initializeStage = [this]( const char *stage, bool ( ClawRendererDX12::*create )() ) {
            WP_LOG_INFO( "WPGraphics/DX12: creating " + String( stage ) );
            if( !( this->*create )() )
            {
                WP_LOG_ERROR( "WPGraphics/DX12: initialization failed creating " + String( stage ) );
                return false;
            }
            return true;
        };
        const bool ok = initializeStage( "device", &ClawRendererDX12::createDevice ) &&
            initializeStage( "command objects", &ClawRendererDX12::createCommandObjects ) &&
            initializeStage( "descriptor heaps", &ClawRendererDX12::createDescriptorHeaps ) &&
            initializeStage( "render target", &ClawRendererDX12::createRenderTarget ) &&
            initializeStage( "depth stencil", &ClawRendererDX12::createDepthStencil ) &&
            initializeStage( "root signatures", &ClawRendererDX12::createRootSignatures ) &&
            initializeStage( "colour pipelines", &ClawRendererDX12::createColourPipelines ) &&
            initializeStage( "texture pipeline", &ClawRendererDX12::createTexturePipeline ) &&
            initializeStage( "vertex buffers", &ClawRendererDX12::createVertexBuffers ) &&
            initializeStage( "index buffer", &ClawRendererDX12::createIndexBuffer ) &&
            initializeStage( "constant buffer", &ClawRendererDX12::createConstantBuffer );

        if( !ok )
            releaseResources();

        setLoadingState( ok ? LoadingState::Loaded : LoadingState::Unloaded );
        if( ok )
            WP_LOG_INFO( "WPGraphics/DX12: renderer initialization completed." );
    }

    void ClawRendererDX12::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        releaseResources();

        m_camera = nullptr;
        m_renderTarget = nullptr;
        m_viewport = nullptr;

        setLoadingState( LoadingState::Unloaded );
    }

    void ClawRendererDX12::releaseResources()
    {
        waitForGpu();

        // Unmap all persistently-mapped upload resources before releasing them.
        if( m_cbMapped && m_cbUpload )
        {
            m_cbUpload->Unmap( 0, nullptr );
            m_cbMapped = nullptr;
        }
        if( m_lineVBMapped && m_lineVBUpload )
        {
            m_lineVBUpload->Unmap( 0, nullptr );
            m_lineVBMapped = nullptr;
        }
        if( m_colourQuadVBMapped && m_colourQuadVBUpload )
        {
            m_colourQuadVBUpload->Unmap( 0, nullptr );
            m_colourQuadVBMapped = nullptr;
        }
        if( m_quadVBMapped && m_quadVBUpload )
        {
            m_quadVBUpload->Unmap( 0, nullptr );
            m_quadVBMapped = nullptr;
        }

        safeRelease( m_cbUpload );
        safeRelease( m_lineVBUpload );
        safeRelease( m_colourQuadVBUpload );
        safeRelease( m_quadVBUpload );
        safeRelease( m_quadIBUpload );

        safeRelease( m_texturePSO );
        safeRelease( m_colourPSO );
        safeRelease( m_linePSO );
        safeRelease( m_textureRootSignature );
        safeRelease( m_colourRootSignature );

        safeRelease( m_depthStencilResource );
        safeRelease( m_renderTargetResource );

        safeRelease( m_cbvSrvHeap );
        safeRelease( m_dsvHeap );
        safeRelease( m_rtvHeap );

        if( m_fenceEvent )
        {
            CloseHandle( m_fenceEvent );
            m_fenceEvent = nullptr;
        }
        safeRelease( m_fence );

        safeRelease( m_commandList );
        safeRelease( m_commandAllocator );
        safeRelease( m_commandQueue );
        safeRelease( m_device );
    }

    // ── IRenderer ─────────────────────────────────────────────────────────

    void ClawRendererDX12::beginRender()
    {
        m_inFrame = true;
        m_primitiveCount = 0;

        if( !m_commandAllocator || !m_commandList )
            return;

        m_commandAllocator->Reset();
        m_commandList->Reset( m_commandAllocator, nullptr );

        // Apply a full-target viewport and scissor rect at the start of every frame.
        D3D12_VIEWPORT vp = {};
        vp.Width = static_cast<float>( m_rtWidth );
        vp.Height = static_cast<float>( m_rtHeight );
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        m_commandList->RSSetViewports( 1, &vp );

        D3D12_RECT scissor = { 0, 0, static_cast<LONG>( m_rtWidth ), static_cast<LONG>( m_rtHeight ) };
        m_commandList->RSSetScissorRects( 1, &scissor );

        // Transition the off-screen render target to RENDER_TARGET state.
        if( m_renderTargetResource )
            transitionResource( m_renderTargetResource, D3D12_RESOURCE_STATE_COMMON,
                                D3D12_RESOURCE_STATE_RENDER_TARGET );

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
        m_commandList->OMSetRenderTargets( 1, &rtvHandle, FALSE, &dsvHandle );
    }

    void ClawRendererDX12::endRender()
    {
        if( m_commandList )
        {
            if( m_renderTargetResource )
                transitionResource( m_renderTargetResource, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                    D3D12_RESOURCE_STATE_COMMON );

            m_commandList->Close();

            ID3D12CommandList *lists[] = { m_commandList };
            m_commandQueue->ExecuteCommandLists( 1, lists );
        }

        waitForGpu();

        m_inFrame = false;
        ++m_frameCount;

        constexpr f32 tickSeconds = 1.0f / 60.0f;
        m_frameTimer += tickSeconds;
        if( m_frameTimer >= 1.0f )
        {
            m_fps = static_cast<s32>( m_frameCount );
            m_frameCount = 0;
            m_frameTimer -= 1.0f;
        }
    }

    void ClawRendererDX12::flush()
    {
        waitForGpu();
    }

    void ClawRendererDX12::clear( const ColourF &colour )
    {
        if( !m_commandList )
            return;

        const float clearColour[4] = { colour.r, colour.g, colour.b, colour.a };

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        // Prefer an RTV extracted from the active render target over the default.
        if( m_renderTarget )
        {
            void *nativeRtv = nullptr;
            m_renderTarget->_getObject( &nativeRtv );
            if( nativeRtv )
                rtvHandle = *static_cast<D3D12_CPU_DESCRIPTOR_HANDLE *>( nativeRtv );
        }

        m_commandList->ClearRenderTargetView( rtvHandle, clearColour, 0, nullptr );

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
        m_commandList->ClearDepthStencilView(
            dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr );
    }

    void ClawRendererDX12::setRenderTarget( SmartPtr<IRenderTarget> renderTarget )
    {
        m_renderTarget = renderTarget;
    }

    SmartPtr<IRenderTarget> ClawRendererDX12::getRenderTarget() const
    {
        return m_renderTarget;
    }

    void ClawRendererDX12::setViewport( SmartPtr<IViewport> viewport )
    {
        m_viewport = viewport;

        // Viewport commands must be recorded into an open command list.
        if( !m_commandList || !m_viewport || !m_inFrame )
            return;

        D3D12_VIEWPORT vp = {};
        vp.Width = static_cast<float>( m_rtWidth );
        vp.Height = static_cast<float>( m_rtHeight );
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        m_commandList->RSSetViewports( 1, &vp );

        D3D12_RECT scissor = { 0, 0, static_cast<LONG>( m_rtWidth ), static_cast<LONG>( m_rtHeight ) };
        m_commandList->RSSetScissorRects( 1, &scissor );
    }

    SmartPtr<IViewport> ClawRendererDX12::getViewport() const
    {
        return m_viewport;
    }

    void ClawRendererDX12::render( const SmartPtr<ISharedObject> & /*renderData*/,
                                   const SmartPtr<ITexture> &texture, const Matrix4F &transform,
                                   const ColourF &colour )
    {
        if( !m_commandList || !m_quadVBMapped )
            return;

        Matrix4F view = m_camera ? Matrix4F( m_camera->getViewMatrix().ptr() ) : Matrix4F();
        Matrix4F proj = m_camera ? Matrix4F( m_camera->getProjectionMatrix().ptr() ) : Matrix4F();
        const Matrix4F wvp = proj * view * transform;

        updateConstantBuffer( wvp );

        const float r = colour.r, g = colour.g, b = colour.b, a = colour.a;
        const VertexPUC verts[4] = {
            { { -0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f }, { r, g, b, a } },
            { { 0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f }, { r, g, b, a } },
            { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f }, { r, g, b, a } },
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f }, { r, g, b, a } },
        };
        memcpy( m_quadVBMapped, verts, sizeof( verts ) );

        m_commandList->SetGraphicsRootSignature( m_textureRootSignature );
        m_commandList->SetPipelineState( m_texturePSO );

        ID3D12DescriptorHeap *heaps[] = { m_cbvSrvHeap };
        m_commandList->SetDescriptorHeaps( 1, heaps );

        m_commandList->SetGraphicsRootConstantBufferView( 0, m_cbUpload->GetGPUVirtualAddress() );

        // Copy the texture's CPU descriptor into heap slot 1; fall back to the null
        // SRV in slot 0 when no valid handle is available.
        D3D12_GPU_DESCRIPTOR_HANDLE gpuSrv = m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart();
        if( texture )
        {
            void *srvPtr = nullptr;
            texture->getTextureFinal( &srvPtr );
            if( srvPtr )
            {
                D3D12_CPU_DESCRIPTOR_HANDLE srcHandle;
                srcHandle.ptr = reinterpret_cast<SIZE_T>( srvPtr );

                D3D12_CPU_DESCRIPTOR_HANDLE dstHandle =
                    m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
                dstHandle.ptr += m_cbvSrvDescSize;  // slot 1

                m_device->CopyDescriptorsSimple( 1, dstHandle, srcHandle,
                                                 D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

                gpuSrv.ptr += m_cbvSrvDescSize;  // point to slot 1
            }
        }
        m_commandList->SetGraphicsRootDescriptorTable( 1, gpuSrv );

        D3D12_VERTEX_BUFFER_VIEW vbView = {};
        vbView.BufferLocation = m_quadVBUpload->GetGPUVirtualAddress();
        vbView.SizeInBytes = k_quadVBSize;
        vbView.StrideInBytes = sizeof( VertexPUC );
        m_commandList->IASetVertexBuffers( 0, 1, &vbView );

        D3D12_INDEX_BUFFER_VIEW ibView = {};
        ibView.BufferLocation = m_quadIBUpload->GetGPUVirtualAddress();
        ibView.SizeInBytes = k_quadIBSize;
        ibView.Format = DXGI_FORMAT_R16_UINT;
        m_commandList->IASetIndexBuffer( &ibView );

        m_commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        m_commandList->DrawIndexedInstanced( 6, 1, 0, 0, 0 );

        m_primitiveCount += 2;
    }

    void ClawRendererDX12::render( const SmartPtr<ISharedObject> & /*renderData*/,
                                   const SmartPtr<IMaterial> & /*material*/, const Matrix4F &transform,
                                   const ColourF &colour )
    {
        if( !m_commandList )
            return;

        Matrix4F view = m_camera ? Matrix4F( m_camera->getViewMatrix().ptr() ) : Matrix4F();
        Matrix4F proj = m_camera ? Matrix4F( m_camera->getProjectionMatrix().ptr() ) : Matrix4F();
        const Matrix4F wvp = proj * view * transform;

        renderQuad( wvp, colour );
    }

    void ClawRendererDX12::_getObject( void **ppObject )
    {
        if( ppObject )
            *ppObject = m_device;
    }

    // ── IRenderer3 ────────────────────────────────────────────────────────

    void ClawRendererDX12::setCamera( SmartPtr<IGraphicsCamera> camera )
    {
        m_camera = camera;
    }

    SmartPtr<IGraphicsCamera> ClawRendererDX12::getCamera() const
    {
        return m_camera;
    }

    void ClawRendererDX12::drawLine( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                     const ColourF &colour )
    {
        if( !m_commandList || !m_lineVBMapped )
            return;

        // Lines are in world space; apply view * projection only.
        Matrix4F view = m_camera ? Matrix4F( m_camera->getViewMatrix().ptr() ) : Matrix4F();
        Matrix4F proj = m_camera ? Matrix4F( m_camera->getProjectionMatrix().ptr() ) : Matrix4F();
        const Matrix4F vp = proj * view;

        updateConstantBuffer( vp );

        const float r = colour.r, g = colour.g, b = colour.b, a = colour.a;
        const VertexPC verts[2] = {
            { { ( start.X() ), ( start.Y() ), ( start.Z() ) }, { r, g, b, a } },
            { { ( end.X() ), ( end.Y() ), ( end.Z() ) }, { r, g, b, a } },
        };
        memcpy( m_lineVBMapped, verts, sizeof( verts ) );

        m_commandList->SetGraphicsRootSignature( m_colourRootSignature );
        m_commandList->SetPipelineState( m_linePSO );
        m_commandList->SetGraphicsRootConstantBufferView( 0, m_cbUpload->GetGPUVirtualAddress() );

        D3D12_VERTEX_BUFFER_VIEW vbView = {};
        vbView.BufferLocation = m_lineVBUpload->GetGPUVirtualAddress();
        vbView.SizeInBytes = k_lineVBSize;
        vbView.StrideInBytes = sizeof( VertexPC );
        m_commandList->IASetVertexBuffers( 0, 1, &vbView );

        m_commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_LINELIST );
        m_commandList->DrawInstanced( 2, 1, 0, 0 );

        ++m_primitiveCount;
    }

    // ── Internal helpers ───────────────────────────────────────────────────

    void ClawRendererDX12::waitForGpu()
    {
        if( !m_commandQueue || !m_fence || !m_fenceEvent )
            return;

        ++m_fenceValue;
        m_commandQueue->Signal( m_fence, m_fenceValue );

        if( m_fence->GetCompletedValue() < m_fenceValue )
        {
            m_fence->SetEventOnCompletion( m_fenceValue, m_fenceEvent );
            WaitForSingleObject( m_fenceEvent, INFINITE );
        }
    }

    void ClawRendererDX12::transitionResource( ID3D12Resource *resource, unsigned int stateBefore,
                                               unsigned int stateAfter )
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = static_cast<D3D12_RESOURCE_STATES>( stateBefore );
        barrier.Transition.StateAfter = static_cast<D3D12_RESOURCE_STATES>( stateAfter );
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier( 1, &barrier );
    }

    bool ClawRendererDX12::createDevice()
    {
#ifdef _DEBUG
        {
            ID3D12Debug *debugController = nullptr;
            if( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &debugController ) ) ) )
            {
                debugController->EnableDebugLayer();
                debugController->Release();
            }
        }
#endif

        IDXGIFactory4 *factory = nullptr;
        if( FAILED( CreateDXGIFactory1( IID_PPV_ARGS( &factory ) ) ) )
            return false;

        IDXGIAdapter1 *adapter = nullptr;
        for( UINT i = 0; factory->EnumAdapters1( i, &adapter ) != DXGI_ERROR_NOT_FOUND; ++i )
        {
            DXGI_ADAPTER_DESC1 desc = {};
            adapter->GetDesc1( &desc );
            if( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE )
            {
                adapter->Release();
                adapter = nullptr;
                continue;
            }

            if( SUCCEEDED(
                    D3D12CreateDevice( adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &m_device ) ) ) )
                break;

            adapter->Release();
            adapter = nullptr;
        }

        if( adapter )
            adapter->Release();

        factory->Release();

        return m_device != nullptr;
    }

    bool ClawRendererDX12::createCommandObjects()
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        if( FAILED( m_device->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &m_commandQueue ) ) ) )
            return false;

        if( FAILED( m_device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      IID_PPV_ARGS( &m_commandAllocator ) ) ) )
            return false;

        if( FAILED( m_device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator,
                                                 nullptr, IID_PPV_ARGS( &m_commandList ) ) ) )
            return false;

        // Close immediately so it can be Reset at the start of the first frame.
        m_commandList->Close();

        if( FAILED( m_device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_fence ) ) ) )
            return false;

        m_fenceValue = 0;
        m_fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
        return m_fenceEvent != nullptr;
    }

    bool ClawRendererDX12::createDescriptorHeaps()
    {
        // RTV heap – 1 descriptor for the off-screen render target.
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.NumDescriptors = 1;
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            if( FAILED( m_device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &m_rtvHeap ) ) ) )
                return false;
        }

        // DSV heap – 1 descriptor.
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.NumDescriptors = 1;
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            if( FAILED( m_device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &m_dsvHeap ) ) ) )
                return false;
        }

        // CBV/SRV/UAV heap – slot 0: null SRV (colour draws), slot 1: active texture SRV.
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.NumDescriptors = 2;
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            if( FAILED( m_device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &m_cbvSrvHeap ) ) ) )
                return false;
        }

        m_rtvDescSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
        m_cbvSrvDescSize =
            m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

        // Populate slot 0 with a null SRV used as a fallback for colour draws.
        D3D12_CPU_DESCRIPTOR_HANDLE nullHandle = m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc = {};
        nullDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        nullDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        nullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        nullDesc.Texture2D.MipLevels = 1;
        m_device->CreateShaderResourceView( nullptr, &nullDesc, nullHandle );

        return true;
    }

    bool ClawRendererDX12::createRenderTarget()
    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = m_rtWidth;
        desc.Height = m_rtHeight;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        clearValue.Color[3] = 1.0f;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        if( FAILED( m_device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                       D3D12_RESOURCE_STATE_COMMON, &clearValue,
                                                       IID_PPV_ARGS( &m_renderTargetResource ) ) ) )
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        m_device->CreateRenderTargetView( m_renderTargetResource, nullptr, rtvHandle );
        return true;
    }

    bool ClawRendererDX12::createDepthStencil()
    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = m_rtWidth;
        desc.Height = m_rtHeight;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_D32_FLOAT;
        desc.SampleDesc.Count = 1;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        if( FAILED( m_device->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                       D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
                                                       IID_PPV_ARGS( &m_depthStencilResource ) ) ) )
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
        m_device->CreateDepthStencilView( m_depthStencilResource, nullptr, dsvHandle );
        return true;
    }

    bool ClawRendererDX12::createRootSignatures()
    {
        // ── Colour root signature: single root CBV at b0 ────────────────────
        {
            D3D12_ROOT_PARAMETER params[1] = {};
            params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            params[0].Descriptor.ShaderRegister = 0;
            params[0].Descriptor.RegisterSpace = 0;
            params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

            D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
            rsDesc.NumParameters = 1;
            rsDesc.pParameters = params;
            rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ID3DBlob *blob = nullptr;
            ID3DBlob *error = nullptr;
            HRESULT hr =
                D3D12SerializeRootSignature( &rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error );
            if( error )
            {
                error->Release();
                error = nullptr;
            }
            if( FAILED( hr ) )
                return false;

            hr = m_device->CreateRootSignature( 0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                                IID_PPV_ARGS( &m_colourRootSignature ) );
            blob->Release();
            if( FAILED( hr ) )
                return false;
        }

        // ── Texture root signature: root CBV at slot 0, SRV table at slot 1 ──
        {
            D3D12_DESCRIPTOR_RANGE srvRange = {};
            srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            srvRange.NumDescriptors = 1;
            srvRange.BaseShaderRegister = 0;
            srvRange.RegisterSpace = 0;
            srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            D3D12_ROOT_PARAMETER params[2] = {};
            params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            params[0].Descriptor.ShaderRegister = 0;
            params[0].Descriptor.RegisterSpace = 0;
            params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

            params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            params[1].DescriptorTable.NumDescriptorRanges = 1;
            params[1].DescriptorTable.pDescriptorRanges = &srvRange;
            params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_STATIC_SAMPLER_DESC sampler = {};
            sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            sampler.MaxLOD = D3D12_FLOAT32_MAX;
            sampler.ShaderRegister = 0;
            sampler.RegisterSpace = 0;
            sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
            rsDesc.NumParameters = 2;
            rsDesc.pParameters = params;
            rsDesc.NumStaticSamplers = 1;
            rsDesc.pStaticSamplers = &sampler;
            rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ID3DBlob *blob = nullptr;
            ID3DBlob *error = nullptr;
            HRESULT hr =
                D3D12SerializeRootSignature( &rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error );
            if( error )
            {
                error->Release();
                error = nullptr;
            }
            if( FAILED( hr ) )
                return false;

            hr = m_device->CreateRootSignature( 0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                                IID_PPV_ARGS( &m_textureRootSignature ) );
            blob->Release();
            if( FAILED( hr ) )
                return false;
        }

        return true;
    }

    bool ClawRendererDX12::createColourPipelines()
    {
        ID3DBlob *vsBlob = nullptr;
        ID3DBlob *psBlob = nullptr;
        ID3DBlob *errBlob = nullptr;

        HRESULT hr = D3DCompile( k_colourShaderSrc, sizeof( k_colourShaderSrc ) - 1, "ColourVS", nullptr,
                                 nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errBlob );
        if( errBlob )
        {
            errBlob->Release();
            errBlob = nullptr;
        }
        if( FAILED( hr ) )
            return false;

        hr = D3DCompile( k_colourShaderSrc, sizeof( k_colourShaderSrc ) - 1, "ColourPS", nullptr,
                         nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errBlob );
        if( errBlob )
        {
            errBlob->Release();
            errBlob = nullptr;
        }
        if( FAILED( hr ) )
        {
            vsBlob->Release();
            return false;
        }

        const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof( float[3] ),
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // Base PSO descriptor shared by both colour sub-pipelines.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_colourRootSignature;
        psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
        psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
        psoDesc.InputLayout = { inputLayout, 2 };
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT_MAX;

        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;

        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        // Line PSO – topology type must be LINE to match LINELIST draws.
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        hr = m_device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_linePSO ) );
        if( FAILED( hr ) )
        {
            vsBlob->Release();
            psBlob->Release();
            return false;
        }

        // Colour quad PSO – TRIANGLE topology for DrawIndexedInstanced quads.
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        hr = m_device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_colourPSO ) );

        vsBlob->Release();
        psBlob->Release();
        return SUCCEEDED( hr );
    }

    bool ClawRendererDX12::createTexturePipeline()
    {
        ID3DBlob *vsBlob = nullptr;
        ID3DBlob *psBlob = nullptr;
        ID3DBlob *errBlob = nullptr;

        HRESULT hr = D3DCompile( k_textureShaderSrc, sizeof( k_textureShaderSrc ) - 1, "TextureVS",
                                 nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errBlob );
        if( errBlob )
        {
            errBlob->Release();
            errBlob = nullptr;
        }
        if( FAILED( hr ) )
            return false;

        hr = D3DCompile( k_textureShaderSrc, sizeof( k_textureShaderSrc ) - 1, "TexturePS", nullptr,
                         nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errBlob );
        if( errBlob )
        {
            errBlob->Release();
            errBlob = nullptr;
        }
        if( FAILED( hr ) )
        {
            vsBlob->Release();
            return false;
        }

        const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof( float[3] ),
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof( float[3] ) + sizeof( float[2] ),
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_textureRootSignature;
        psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
        psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
        psoDesc.InputLayout = { inputLayout, 3 };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT_MAX;

        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;

        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        hr = m_device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_texturePSO ) );
        vsBlob->Release();
        psBlob->Release();
        return SUCCEEDED( hr );
    }

    bool ClawRendererDX12::createVertexBuffers()
    {
        D3D12_HEAP_PROPERTIES uploadHeap = {};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

        // Helper: create an UPLOAD-heap buffer and return a persistent CPU mapping.
        auto createUploadBuffer = [&]( u32 size, ID3D12Resource **ppRes, void **ppMapped ) -> bool {
            D3D12_RESOURCE_DESC desc = {};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Width = size;
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            if( FAILED( m_device->CreateCommittedResource( &uploadHeap, D3D12_HEAP_FLAG_NONE, &desc,
                                                           D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                           IID_PPV_ARGS( ppRes ) ) ) )
                return false;

            D3D12_RANGE readRange = {};
            return SUCCEEDED( ( *ppRes )->Map( 0, &readRange, ppMapped ) );
        };

        if( !createUploadBuffer( k_lineVBSize, &m_lineVBUpload, &m_lineVBMapped ) )
            return false;
        if( !createUploadBuffer( k_colourQuadVBSize, &m_colourQuadVBUpload, &m_colourQuadVBMapped ) )
            return false;
        if( !createUploadBuffer( k_quadVBSize, &m_quadVBUpload, &m_quadVBMapped ) )
            return false;

        return true;
    }

    bool ClawRendererDX12::createIndexBuffer()
    {
        const u16 indices[6] = { 0, 1, 2, 0, 2, 3 };

        D3D12_HEAP_PROPERTIES uploadHeap = {};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = k_quadIBSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        if( FAILED( m_device->CreateCommittedResource( &uploadHeap, D3D12_HEAP_FLAG_NONE, &desc,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                       IID_PPV_ARGS( &m_quadIBUpload ) ) ) )
            return false;

        void *mapped = nullptr;
        D3D12_RANGE readRange = {};
        if( FAILED( m_quadIBUpload->Map( 0, &readRange, &mapped ) ) )
            return false;

        memcpy( mapped, indices, sizeof( indices ) );
        m_quadIBUpload->Unmap( 0, nullptr );
        return true;
    }

    bool ClawRendererDX12::createConstantBuffer()
    {
        D3D12_HEAP_PROPERTIES uploadHeap = {};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = k_cbSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        if( FAILED( m_device->CreateCommittedResource( &uploadHeap, D3D12_HEAP_FLAG_NONE, &desc,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                       IID_PPV_ARGS( &m_cbUpload ) ) ) )
            return false;

        D3D12_RANGE readRange = {};
        return SUCCEEDED( m_cbUpload->Map( 0, &readRange, &m_cbMapped ) );
    }

    void ClawRendererDX12::updateConstantBuffer( const Matrix4F &wvp )
    {
        if( m_cbMapped )
            memcpy( m_cbMapped, wvp.ptr(), 16 * sizeof( float ) );
    }

    void ClawRendererDX12::renderQuad( const Matrix4F &wvp, const ColourF &colour )
    {
        if( !m_commandList || !m_colourQuadVBMapped )
            return;

        updateConstantBuffer( wvp );

        const float r = colour.r, g = colour.g, b = colour.b, a = colour.a;
        const VertexPC verts[4] = {
            { { -0.5f, 0.5f, 0.0f }, { r, g, b, a } },
            { { 0.5f, 0.5f, 0.0f }, { r, g, b, a } },
            { { 0.5f, -0.5f, 0.0f }, { r, g, b, a } },
            { { -0.5f, -0.5f, 0.0f }, { r, g, b, a } },
        };
        memcpy( m_colourQuadVBMapped, verts, sizeof( verts ) );

        m_commandList->SetGraphicsRootSignature( m_colourRootSignature );
        m_commandList->SetPipelineState( m_colourPSO );
        m_commandList->SetGraphicsRootConstantBufferView( 0, m_cbUpload->GetGPUVirtualAddress() );

        D3D12_VERTEX_BUFFER_VIEW vbView = {};
        vbView.BufferLocation = m_colourQuadVBUpload->GetGPUVirtualAddress();
        vbView.SizeInBytes = k_colourQuadVBSize;
        vbView.StrideInBytes = sizeof( VertexPC );
        m_commandList->IASetVertexBuffers( 0, 1, &vbView );

        D3D12_INDEX_BUFFER_VIEW ibView = {};
        ibView.BufferLocation = m_quadIBUpload->GetGPUVirtualAddress();
        ibView.SizeInBytes = k_quadIBSize;
        ibView.Format = DXGI_FORMAT_R16_UINT;
        m_commandList->IASetIndexBuffer( &ibView );

        m_commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        m_commandList->DrawIndexedInstanced( 6, 1, 0, 0, 0 );

        m_primitiveCount += 2;
    }
}  // namespace workphone::render
