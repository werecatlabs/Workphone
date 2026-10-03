#ifndef __WP_Shader_h__
#define __WP_Shader_h__

#include <Workphone/Interface/Graphics/IShader.hpp>

namespace workphone
{
    namespace render
    {

        // base class for render system specific implementations
        class WPCore_API Shader : public IShader
        {
        public:
            Shader();
            ~Shader() override;

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

            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override
            {
                return false;
            }

            bool handleStateChanged( SmartPtr<IState> &state ) override
            {
                return false;
            }

            WP_CLASS_REGISTER_DECL;

        protected:
            struct SpecializationValue
            {
                String name;
                u32 value = 0;
            };

            Stage m_stage = Stage::Vertex;
            Language m_language = Language::Unknown;
            String m_source;
            String m_entryPoint = "main";
            String m_profile;
            Array<Define> m_defines;
            Array<Diagnostic> m_diagnostics;
            Reflection m_reflection;
            Array<u8> m_binary;
            Array<SpecializationValue> m_specializationValues;
            Status m_status = Status::Empty;
        };

    }  // namespace render
}  // namespace workphone

#endif  // Shader_h__
