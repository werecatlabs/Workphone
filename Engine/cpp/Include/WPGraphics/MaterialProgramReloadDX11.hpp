#ifndef WP_MATERIAL_PROGRAM_RELOAD_DX11_HPP
#define WP_MATERIAL_PROGRAM_RELOAD_DX11_HPP
#include <WPGraphics/MaterialProgramDX11.hpp>
namespace workphone::render
{
    /** Graphics-owner-thread reload coordinator for a fixed immutable CPU schema.
     * Watches C/ClawShader stage source/binary/define revisions automatically.
     * A detached native/map candidate and every subscribed material publication
     * must succeed before a nonallocating commit. Old draw records retain their
     * generation. Failed edits leave the complete last-good generation active.
     * Source edits, creation, subscription lifetime, Update and capture must be
     * serialized on the same owner thread. No compilation occurs in native Bind.
     * File/include watching, schema migration and device recreation are separate.
     */
    class WPGraphics_API MaterialProgramReloadDX11 : public std::enable_shared_from_this<MaterialProgramReloadDX11>
    {
    public:
        using Ptr = std::shared_ptr<MaterialProgramReloadDX11>;
        class WPGraphics_API Binding
        {
        public:
            ~Binding();
            MaterialProgramDX11::PublicationPtr GetPublication();
        private:
            friend class MaterialProgramReloadDX11;
            struct Data;
            std::unique_ptr<Data> m_data;
            Binding();
        };
        using BindingPtr = std::shared_ptr<Binding>;
        static Ptr Create(void *device, MaterialShaderParameterLayout::Ptr layout,
                          wp_shader_program *authoring, u64 deviceGeneration, String &error,
                          const MaterialProgramReflectionSelection *pixelSelection = nullptr,
                          const MaterialProgramReflectionSelection *vertexSelection = nullptr);
        static Ptr CreateFromFiles(void *device, MaterialShaderParameterLayout::Ptr layout,
                          const String &vertexFile, const String &pixelFile, u64 deviceGeneration, String &error);
        ~MaterialProgramReloadDX11();
        // The values object must outlive its binding. ClawMaterial owns this
        // relationship; a binding never retains the material or creates a cycle.
        BindingPtr Attach(MaterialShaderParametersInstance &values,
                          std::shared_ptr<MaterialProgramResourcesDX11> resources = {});
        // Called automatically before a subscribed material's publication capture.
        // Permanent failures are memoized by revision. Admission failures retry
        // when CPU/GPU headroom increases, or after explicit RequestRetry().
        bool Update();
        void RequestRetry();
        bool SetStageFiles(const String &vertexFile,const String &pixelFile,String &error);
        void RequestSchema(MaterialShaderParameterLayout::Ptr layout);
        static void UpdateAll();
        u64 GetId() const;
        static bool QueueReload(u64 id);
        static bool QueueWatching(u64 id,bool enabled);
        static SmartPtr<Properties> EditorStatus(u64 id);
        bool HasProgram() const;
        u64 GetGeneration() const;
        u64 GetAttemptCount() const;
        u64 GetCompileCount() const;
        const char *GetLastError() const;
    private:
        MaterialProgramReloadDX11();
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}
#endif
