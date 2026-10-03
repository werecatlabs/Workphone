#ifndef ClawRendererDX12_h__
#define ClawRendererDX12_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IRenderer3.hpp>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Math/Matrix4.hpp>

// Forward-declare D3D12 COM interfaces to avoid including d3d12.h in this header.
struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12Fence;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12RootSignature;
struct ID3D12PipelineState;

namespace workphone
{
    namespace render
    {
        /**
         * DirectX 12 renderer implementing IRenderer3.
         *
         * Creates a D3D12 device and a direct command queue on load(), compiles two
         * built-in shader pipelines (colour-only and textured), and exposes frame
         * lifecycle, camera, render-target, viewport, sprite and 3D-line rendering
         * through the IRenderer3 contract.
         *
         * All dynamic vertex data and the per-draw constant buffer live in persistently
         * mapped UPLOAD-heap resources so CPU writes become immediately visible to the
         * GPU.  A fence is signalled at the end of every frame to guarantee the GPU has
         * finished before the allocator is reset at the start of the next frame.
         */
        class WPGraphics_API ClawRendererDX12 : public IRenderer3
        {
        public:
            ClawRendererDX12();
            ~ClawRendererDX12() override;

            // ── ISharedObject lifecycle ──────────────────────────────────────
            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            // ── IRenderer ───────────────────────────────────────────────────
            void beginRender() override;
            void endRender() override;
            void flush() override;
            void clear( const ColourF &colour ) override;
            void setRenderTarget( SmartPtr<IRenderTarget> renderTarget ) override;
            SmartPtr<IRenderTarget> getRenderTarget() const override;
            void setViewport( SmartPtr<IViewport> viewport ) override;
            SmartPtr<IViewport> getViewport() const override;
            void render( const SmartPtr<ISharedObject> &renderData, const SmartPtr<ITexture> &texture,
                         const Matrix4F &transform, const ColourF &colour ) override;
            void render( const SmartPtr<ISharedObject> &renderData, const SmartPtr<IMaterial> &material,
                         const Matrix4F &transform, const ColourF &colour ) override;
            void _getObject( void **ppObject ) override;

            // ClawRendererDX12 renders through its own D3D12 command queue and is not
            // bridgeable to the C89 `wp_renderer` API used by ClawImguiManager /
            // ClawUIWorkphoneRenderer.  Returning nullptr lets the call sites in
            // ClawHammerSystem::configure / switchRenderer keep their casts consistent;
            // the UI renderers bail early on a null renderer.  A future DX12 wp_renderer
            // bridge would replace this with a real pointer.
            wp_renderer *getNativeRenderer() const { return nullptr; }

            // ── IRenderer3 ──────────────────────────────────────────────────
            void setCamera( SmartPtr<IGraphicsCamera> camera ) override;
            SmartPtr<IGraphicsCamera> getCamera() const override;
            void drawLine( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                           const ColourF &colour ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            // ── Resource helpers ─────────────────────────────────────────────
            template <typename T>
            static void safeRelease( T *&ptr );

            void releaseResources();
            void waitForGpu();

            bool createDevice();
            bool createCommandObjects();
            bool createDescriptorHeaps();
            bool createRenderTarget();
            bool createDepthStencil();
            bool createRootSignatures();
            bool createColourPipelines();
            bool createTexturePipeline();
            bool createVertexBuffers();
            bool createIndexBuffer();
            bool createConstantBuffer();

            void updateConstantBuffer( const Matrix4F &wvp );
            void renderQuad( const Matrix4F &wvp, const ColourF &colour );
            void transitionResource( ID3D12Resource *resource, unsigned int stateBefore,
                                     unsigned int stateAfter );

            // ── D3D12 objects ────────────────────────────────────────────────
            ID3D12Device *m_device = nullptr;
            ID3D12CommandQueue *m_commandQueue = nullptr;
            ID3D12CommandAllocator *m_commandAllocator = nullptr;
            ID3D12GraphicsCommandList *m_commandList = nullptr;

            ID3D12Fence *m_fence = nullptr;
            u64 m_fenceValue = 0;
            void *m_fenceEvent = nullptr;  // stored as void* to avoid HANDLE in header

            ID3D12DescriptorHeap *m_rtvHeap = nullptr;
            ID3D12DescriptorHeap *m_dsvHeap = nullptr;
            ID3D12DescriptorHeap *m_cbvSrvHeap = nullptr;

            ID3D12Resource *m_renderTargetResource = nullptr;
            ID3D12Resource *m_depthStencilResource = nullptr;

            // Colour pipeline: two PSOs (line and triangle topology) with one root signature.
            ID3D12RootSignature *m_colourRootSignature = nullptr;
            ID3D12PipelineState *m_linePSO = nullptr;
            ID3D12PipelineState *m_colourPSO = nullptr;

            // Texture pipeline: one PSO with its own root signature (adds SRV table).
            ID3D12RootSignature *m_textureRootSignature = nullptr;
            ID3D12PipelineState *m_texturePSO = nullptr;

            // Upload-heap vertex / index / constant buffers (persistently mapped).
            ID3D12Resource *m_cbUpload = nullptr;
            ID3D12Resource *m_lineVBUpload = nullptr;
            ID3D12Resource *m_colourQuadVBUpload = nullptr;
            ID3D12Resource *m_quadVBUpload = nullptr;
            ID3D12Resource *m_quadIBUpload = nullptr;

            void *m_cbMapped = nullptr;
            void *m_lineVBMapped = nullptr;
            void *m_colourQuadVBMapped = nullptr;
            void *m_quadVBMapped = nullptr;

            u32 m_rtvDescSize = 0;
            u32 m_cbvSrvDescSize = 0;

            // ── Engine-level state ───────────────────────────────────────────
            SmartPtr<IGraphicsCamera> m_camera;
            SmartPtr<IRenderTarget> m_renderTarget;
            SmartPtr<IViewport> m_viewport;

            u32 m_rtWidth = 1280;
            u32 m_rtHeight = 720;

            s32 m_fps = 0;
            u32 m_primitiveCount = 0;
            u32 m_frameCount = 0;
            f32 m_frameTimer = 0.0f;
            bool m_inFrame = false;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawRendererDX12_h__
