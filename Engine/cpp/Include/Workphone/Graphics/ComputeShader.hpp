#ifndef ComputeShader_h__
#define ComputeShader_h__

#include <Workphone/Interface/Graphics/IComputeShader.hpp>

namespace workphone
{
    namespace render
    {

        // base class for render system specific implementations
        class WPCore_API ComputeShader : public IComputeShader
        {
        public:
            ComputeShader();
            ~ComputeShader() override;

            WP_CLASS_REGISTER_DECL;

            WorkgroupSize getWorkgroupSize() const override;

            void setWorkgroupSize( const WorkgroupSize &size ) override;

            DispatchSize getDispatchSize() const override;

            void setDispatchSize( const DispatchSize &size ) override;

            bool validateDispatch( const DispatchSize &size,
                                   String *errorMessage = nullptr ) const override;

            Array<ComputeBinding> getComputeBindings() const override;

            bool getComputeBinding( const String &name, ComputeBinding &binding ) const override;

            bool hasComputeBinding( const String &name ) const override;

            bool setBindingOverride( const String &name, u32 set, u32 binding ) override;

            void clearBindingOverrides() override;

            u32 getSharedMemorySize() const override;

            void setSharedMemorySize( u32 bytes ) override;

            bool validateBindings( Array<Diagnostic> *diagnostics = nullptr ) const override;

            bool createPipeline() override;

            void destroyPipeline() override;

            bool hasPipeline() const override;

            void getPipelineHandle( void **handle ) const override;

            bool setIndirectDispatchSource( void *buffer, u32 offset = 0 ) override;

            void clearIndirectDispatchSource() override;

            bool hasIndirectDispatchSource() const override;

            Stage getStage() const override;

            void setStage( Stage stage ) override;

            Language getLanguage() const override;

            void setLanguage( Language language ) override;

            String getSource() const override;

            void setSource( const String &source ) override;

            String getEntryPoint() const override;

            void setEntryPoint( const String &entryPoint ) override;

            String getProfile() const override;

            void setProfile( const String &profile ) override;

            Array<Define> getDefines() const override;

            void setDefines( const Array<Define> &defines ) override;

            void setDefine( const String &name, const String &value = String() ) override;

            void removeDefine( const String &name ) override;

            void clearDefines() override;

            bool compile() override;

            bool reload() override;

            void invalidate() override;

            Status getStatus() const override;

            bool isCompiled() const override;

            Array<Diagnostic> getDiagnostics() const override;

            void clearDiagnostics() override;

            Reflection getReflection() const override;

            bool hasResource( const String &name ) const override;

            Array<u8> getBinary() const override;

            bool setBinary( const Array<u8> &binary, Language language ) override;

            bool setSpecializationConstant( const String &name, u32 value ) override;

            bool getSpecializationConstant( const String &name, u32 &value ) const override;

            void getNativeHandle( void **handle ) const override;

        protected:
            struct BindingOverride
            {
                String name;
                u32 set = 0;
                u32 binding = 0;
            };

            struct SpecializationValue
            {
                String name;
                u32 value = 0;
            };

            WorkgroupSize m_workgroupSize;
            DispatchSize m_dispatchSize;
            Array<ComputeBinding> m_computeBindings;
            Array<BindingOverride> m_bindingOverrides;
            Array<SpecializationValue> m_specializationValues;
            u32 m_sharedMemorySize = 0;

            Stage m_stage = Stage::Compute;
            Language m_language = Language::Unknown;
            String m_source;
            String m_entryPoint = "main";
            String m_profile;
            Array<Define> m_defines;
            Array<Diagnostic> m_diagnostics;
            Reflection m_reflection;
            Array<u8> m_binary;
            Status m_status = Status::Empty;

            bool m_pipelineCreated = false;
            void *m_pipelineHandle = nullptr;
            void *m_indirectDispatchBuffer = nullptr;
            u32 m_indirectDispatchOffset = 0;
        };

    }  // namespace render
}  // namespace workphone

#endif  // ComputeShader_h__
