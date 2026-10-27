#ifndef WP_MATERIAL_PROGRAM_DX11_HPP
#define WP_MATERIAL_PROGRAM_DX11_HPP
#include <WPGraphics/MaterialProgramBinding.hpp>
#include <memory>
struct wp_renderer_dx11;
namespace workphone::render
{
    /** Owning-thread resource adapter. Return a borrowed SRV (Texture/Buffer) or
     * ID3D11SamplerState (Sampler). The adapter retains its resource owners;
     * Resolve is called for each submission, including previously absent views.
     * False defers the draw. InvalidMaterialResourceHandle explicitly unbinds without a call.
     */
    class WPGraphics_API MaterialProgramResourcesDX11
    {
    public:
        virtual ~MaterialProgramResourcesDX11() = default;
        virtual bool Resolve(MaterialShaderParameterType type, MaterialResourceHandle handle,
                             void *&native) = 0;
    };

    /** Numeric resource-owner table. Texture views are queried from the engine
     * texture adapter on every draw; native buffer SRVs and samplers are retained
     * with COM ownership. Mutations and submission use the same owning thread.
     * InvalidMaterialResourceHandle is reserved; zero is a valid identity.
     */
    class WPGraphics_API MaterialProgramResourceTableDX11 final : public MaterialProgramResourcesDX11
    {
    public:
        MaterialProgramResourceTableDX11();
        ~MaterialProgramResourceTableDX11() override;
        void SetTexture(MaterialResourceHandle, SmartPtr<ITexture>);
        void SetBuffer(MaterialResourceHandle, void *srv);
        void SetSampler(MaterialResourceHandle, void *sampler);
        bool Resolve(MaterialShaderParameterType, MaterialResourceHandle, void *&) override;
    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };

    /** Immutable shader/map identity with reusable device-owned upload buffers.
     * Create consumes compiled DXBC, never compiles on a draw. A pixel stage is
     * required; an optional vertex stage uses the engine PNTC vertex ABI. Without
     * one the built-in transformed vertex stage remains active (also instanced).
     * Selections identify material ownership; b0 remains transform-owned.
     * Custom vertex stages currently defer instanced submissions.
     */
    class WPGraphics_API MaterialProgramDX11 : public std::enable_shared_from_this<MaterialProgramDX11>
    {
    public:
        using Ptr = std::shared_ptr<MaterialProgramDX11>;
        struct Bytecode { const void *data = nullptr; size_t size = 0; };
        class WPGraphics_API Publication
        {
        public:
            ~Publication();
            Publication(const Publication &) = delete;
            Publication &operator=(const Publication &) = delete;
            bool Bind(wp_renderer_dx11 *, bool instanced) const;
            void Unbind(wp_renderer_dx11 *) const;
        private:
            friend class MaterialProgramDX11;
            struct Data;
            // Aliases the enclosing Data allocation; its shared control block
            // owns both this view and the immutable packed payload.
            Data *m_data;
            explicit Publication(Data *);
        };
        using PublicationPtr = std::shared_ptr<const Publication>;
        static Ptr Create(void *device, MaterialShaderParameterLayout::Ptr layout,
                          wp_shader_program *program, u64 deviceGeneration,
                          Bytecode pixel, Bytecode vertex, String &error,
                          const MaterialProgramReflectionSelection *pixelSelection = nullptr,
                          const MaterialProgramReflectionSelection *vertexSelection = nullptr);
        ~MaterialProgramDX11();
        PublicationPtr Capture(const MaterialShaderParametersInstance &,
                               std::shared_ptr<MaterialProgramResourcesDX11>) const;
        bool IsCurrent() const;
        bool IsCurrent(const MaterialShaderParameterLayout::Ptr &) const;
    private:
        friend class MaterialProgramReloadDX11;
        friend class ClawMaterialResource;
        void retainDefinitionOwner(std::shared_ptr<const void>);
        bool matchesDevice(void *) const;
        MaterialProgramDX11();
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}
#endif
