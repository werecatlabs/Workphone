#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/ClawShader.hpp"
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_shader.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#if defined(WP_PLATFORM_WIN32) && WP_BUILD_RENDERER_DX11
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <atomic>
#include <new>
#endif

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawShader, Shader );

        const String ClawShader::stageStr = "stage";
        const String ClawShader::languageStr = "language";
        const String ClawShader::sourceStr = "source";
        const String ClawShader::entryPointStr = "entryPoint";
        const String ClawShader::profileStr = "profile";
        const String ClawShader::statusStr = "status";
        const String ClawShader::definesStr = "defines";

        namespace
        {
            const wp_c8 *toCText( const char *text )
            {
                return static_cast<const wp_c8 *>( static_cast<const void *>( text ) );
            }

            const char *fromCText( const wp_c8 *text )
            {
                return static_cast<const char *>( static_cast<const void *>( text ) );
            }

            wp_shader_language toCLanguage( IShader::Language language )
            {
                switch( language )
                {
                case IShader::Language::GLSL:
                    return WORKPHONE_SHADER_LANGUAGE_GLSL;
                case IShader::Language::HLSL:
                    return WORKPHONE_SHADER_LANGUAGE_HLSL;
                case IShader::Language::Metal:
                    return WORKPHONE_SHADER_LANGUAGE_METAL;
                case IShader::Language::SPIRV:
                    return WORKPHONE_SHADER_LANGUAGE_SPIRV;
                case IShader::Language::DXIL:
                    return WORKPHONE_SHADER_LANGUAGE_DXIL;
                case IShader::Language::DXBC:
                    return WORKPHONE_SHADER_LANGUAGE_DXBC;
                case IShader::Language::WGSL:
                    return WORKPHONE_SHADER_LANGUAGE_WGSL;
                default:
                    return WORKPHONE_SHADER_LANGUAGE_UNKNOWN;
                }
            }

            IShader::Language fromCLanguage( wp_shader_language language )
            {
                switch( language )
                {
                case WORKPHONE_SHADER_LANGUAGE_GLSL:
                    return IShader::Language::GLSL;
                case WORKPHONE_SHADER_LANGUAGE_HLSL:
                    return IShader::Language::HLSL;
                case WORKPHONE_SHADER_LANGUAGE_METAL:
                    return IShader::Language::Metal;
                case WORKPHONE_SHADER_LANGUAGE_SPIRV:
                    return IShader::Language::SPIRV;
                case WORKPHONE_SHADER_LANGUAGE_DXIL:
                    return IShader::Language::DXIL;
                case WORKPHONE_SHADER_LANGUAGE_DXBC:
                    return IShader::Language::DXBC;
                case WORKPHONE_SHADER_LANGUAGE_WGSL:
                    return IShader::Language::WGSL;
                default:
                    return IShader::Language::Unknown;
                }
            }

            IShader::Status fromCStatus( wp_shader_status status )
            {
                return static_cast<IShader::Status>( static_cast<u8>( status ) );
            }

#if defined(WP_PLATFORM_WIN32) && WP_BUILD_RENDERER_DX11
            class ShaderBlob final : public ID3DBlob
            {
                std::atomic<ULONG> m_references{1};
                StreamBudget m_budget;
                Microsoft::WRL::ComPtr<ID3DBlob> m_bytes;
            public:
                ShaderBlob(Microsoft::WRL::ComPtr<ID3DBlob> bytes,StreamBudget budget) :
                    m_budget(std::move(budget)),m_bytes(std::move(bytes)) {}
                HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void **result) override
                {
                    if(!result) return E_POINTER;
                    *result=nullptr;
                    if(id!=__uuidof(IUnknown) && id!=__uuidof(ID3DBlob)) return E_NOINTERFACE;
                    *result=static_cast<ID3DBlob *>(this); AddRef(); return S_OK;
                }
                ULONG STDMETHODCALLTYPE AddRef() override { return ++m_references; }
                ULONG STDMETHODCALLTYPE Release() override
                {
                    const auto references=--m_references;
                    if(!references) delete this;
                    return references;
                }
                LPVOID STDMETHODCALLTYPE GetBufferPointer() override { return m_bytes->GetBufferPointer(); }
                SIZE_T STDMETHODCALLTYPE GetBufferSize() override { return m_bytes->GetBufferSize(); }
            };

            void releaseClawShader(void *native,void *)
            { if(native) static_cast<ID3DBlob *>(native)->Release(); }

            wp_s32 compileClawShader(wp_shader *shader,void **candidate,void *)
            {
                if(!shader || !candidate) return 0;
                *candidate=nullptr;
                try
                {
                    using Microsoft::WRL::ComPtr;
                    const auto stage=wp_shader_get_type(shader);
                    const char *profiles[]={"vs_5_0","ps_5_0","gs_5_0","hs_5_0","ds_5_0","cs_5_0"};
                    if(static_cast<unsigned>(stage)>=6)
                    { wp_shader_set_last_error(shader,toCText("This shader stage is unsupported by DirectX 11."));return 0; }
                    const auto language=wp_shader_get_language(shader);
                    if(language!=WORKPHONE_SHADER_LANGUAGE_HLSL && language!=WORKPHONE_SHADER_LANGUAGE_DXBC)
                    { wp_shader_set_last_error(shader,toCText("DirectX 11 shader compilation supports HLSL and DXBC only."));return 0; }
                    if(wp_shader_get_specialization_count(shader))
                    { wp_shader_set_last_error(shader,toCText("DirectX 11 specialization constants require an explicit compiler policy; use HLSL defines."));return 0; }
                    StreamBudget budget;
                    ComPtr<ID3DBlob> blob,diagnostic;
                    if(language==WORKPHONE_SHADER_LANGUAGE_DXBC)
                    {
                        wp_u32 size=0;
                        const auto bytes=wp_shader_get_binary(shader,&size);
                        if(!bytes || !size)
                        { wp_shader_set_last_error(shader,toCText("DXBC shader has no bytecode."));return 0; }
                        if(size>std::numeric_limits<size_t>::max()-sizeof(ShaderBlob)) throw std::bad_alloc();
                        budget=WorldStreamingScheduler::reserveSharedSource(sizeof(ShaderBlob)+size);
                        if(!budget) throw std::bad_alloc();
                        if(FAILED(D3DCreateBlob(size,&blob))) throw std::bad_alloc();
                        std::memcpy(blob->GetBufferPointer(),bytes,size);
                    }
                    else
                    {
                        const auto source=fromCText(wp_shader_get_source(shader));
                        if(!source || !*source)
                        { wp_shader_set_last_error(shader,toCText("HLSL source is empty. Precompiled shader bytes require the DXBC language."));return 0; }
                        std::array<D3D_SHADER_MACRO,WP_SHADER_MAX_DEFINES+1> macros{};
                        for(wp_s32 i=0;i<wp_shader_get_define_count(shader);++i)
                        {
                            const auto define=wp_shader_get_define_at(shader,i);
                            macros[i]={fromCText(define->name),fromCText(define->value)};
                        }
                        const auto entry=fromCText(wp_shader_get_entry_point(shader));
                        const auto authoredProfile=fromCText(wp_shader_get_profile(shader));
                        const auto profile=*authoredProfile?authoredProfile:profiles[static_cast<unsigned>(stage)];
                        const auto result=D3DCompile(source,std::strlen(source),"ClawShader",macros.data(),nullptr,
                            entry,profile,D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&blob,&diagnostic);
                        if(result==E_OUTOFMEMORY) throw std::bad_alloc();
                        if(FAILED(result))
                        {
                            wp_shader_set_last_error(shader,toCText(diagnostic ? static_cast<const char *>(diagnostic->GetBufferPointer()) : "HLSL compilation failed."));
                            return 0;
                        }
                    }
                    ComPtr<ID3D11ShaderReflection> reflection;
                    if(FAILED(D3DReflect(blob->GetBufferPointer(),blob->GetBufferSize(),__uuidof(ID3D11ShaderReflection),
                        reinterpret_cast<void **>(reflection.GetAddressOf()))))
                    { wp_shader_set_last_error(shader,toCText("Shader bytecode is not valid DXBC."));return 0; }
                    D3D11_SHADER_DESC description{};
                    if(FAILED(reflection->GetDesc(&description)))
                    { wp_shader_set_last_error(shader,toCText("DXBC shader reflection failed."));return 0; }
                    const unsigned kinds[]={D3D11_SHVER_VERTEX_SHADER,D3D11_SHVER_PIXEL_SHADER,D3D11_SHVER_GEOMETRY_SHADER,
                        D3D11_SHVER_HULL_SHADER,D3D11_SHVER_DOMAIN_SHADER,D3D11_SHVER_COMPUTE_SHADER};
                    if(D3D11_SHVER_GET_TYPE(description.Version)!=kinds[static_cast<unsigned>(stage)])
                    { wp_shader_set_last_error(shader,toCText("DXBC stage does not match the shader descriptor."));return 0; }
                    if(blob->GetBufferSize()>std::numeric_limits<size_t>::max()-sizeof(ShaderBlob)) throw std::bad_alloc();
                    if(!budget)
                    {
                        budget=WorldStreamingScheduler::reserveSharedSource(sizeof(ShaderBlob)+blob->GetBufferSize());
                        if(!budget) throw std::bad_alloc();
                    }
                    auto retained=new(std::nothrow) ShaderBlob(std::move(blob),std::move(budget));
                    if(!retained) throw std::bad_alloc();
                    *candidate=static_cast<ID3DBlob *>(retained); return 1;
                }
                catch(const std::bad_alloc &)
                { wp_shader_set_last_error(shader,toCText("Shader bytecode CPU admission denied; the previous compiled shader is retained.")); }
                catch(const std::exception &error) { wp_shader_set_last_error(shader,toCText(error.what())); }
                catch(...) { wp_shader_set_last_error(shader,toCText("DirectX 11 shader compiler failed.")); }
                return 0;
            }
#else
            wp_s32 compileClawShader(wp_shader *shader,void **candidate,void *)
            {
                if(candidate) *candidate=nullptr;
                wp_shader_set_last_error(shader,toCText("HLSL/DXBC shader compilation requires the Claw DirectX 11 build."));
                return 0;
            }
            void releaseClawShader(void *,void *) {}
#endif
        }  // namespace

        ClawShader::ClawShader() :
            m_shader( wp_shader_create( WORKPHONE_SHADER_TYPE_VERTEX, WORKPHONE_SHADER_LANGUAGE_UNKNOWN,
                                        nullptr, toCText( "main" ) ) )
        {
            if( !m_shader )
            {
                WP_LOG_ERROR( "ClawShader: failed to create native C shader." );
            }
            else
            {
                wp_shader_set_compile_func(m_shader,compileClawShader,nullptr);
                wp_shader_set_release_func(m_shader,releaseClawShader,nullptr);
            }
        }

        ClawShader::~ClawShader()
        {
#if defined(WP_PLATFORM_WIN32) && WP_BUILD_RENDERER_DX11
            if(m_compiledBlob) static_cast<ID3DBlob *>(m_compiledBlob)->Release();
#endif
            m_compiledBlob=nullptr;
            if( m_shader )
            {
                wp_shader_destroy( m_shader );
                m_shader = nullptr;
            }
        }

        IShader::Stage ClawShader::getStage() const
        {
            return m_shader ? static_cast<Stage>( wp_shader_get_type( m_shader ) ) : Shader::getStage();
        }

        void ClawShader::setStage( Stage stage )
        {
            if( m_shader )
            {
                wp_shader_set_type( m_shader, static_cast<wp_shader_type>( stage ) );
                Shader::setStage( static_cast<Stage>( wp_shader_get_type( m_shader ) ) );
            }
            else
            {
                Shader::setStage( stage );
            }
        }

        IShader::Language ClawShader::getLanguage() const
        {
            return m_shader ? fromCLanguage( wp_shader_get_language( m_shader ) )
                            : Shader::getLanguage();
        }

        void ClawShader::setLanguage( Language language )
        {
            if( m_shader )
            {
                wp_shader_set_language( m_shader, toCLanguage( language ) );
                Shader::setLanguage( fromCLanguage( wp_shader_get_language( m_shader ) ) );
            }
            else
            {
                Shader::setLanguage( language );
            }
        }

        String ClawShader::getSource() const
        {
            return m_shader ? String( fromCText( wp_shader_get_source( m_shader ) ) )
                            : Shader::getSource();
        }

        void ClawShader::setSource( const String &source )
        {
            if( m_shader )
            {
                if( !wp_shader_set_source( m_shader, toCText( source.c_str() ) ) )
                {
                    WP_LOG_ERROR( "ClawShader: failed to store shader source." );
                    return;
                }
                Shader::setSource( String( fromCText( wp_shader_get_source( m_shader ) ) ) );
                m_binary.clear();
            }
            else
            {
                Shader::setSource( source );
            }
        }

        String ClawShader::getEntryPoint() const
        {
            return m_shader ? String( fromCText( wp_shader_get_entry_point( m_shader ) ) )
                            : Shader::getEntryPoint();
        }

        void ClawShader::setEntryPoint( const String &entryPoint )
        {
            if( m_shader )
            {
                wp_shader_set_entry_point( m_shader, toCText( entryPoint.c_str() ) );
                Shader::setEntryPoint( String( fromCText( wp_shader_get_entry_point( m_shader ) ) ) );
            }
            else
            {
                Shader::setEntryPoint( entryPoint );
            }
        }

        String ClawShader::getProfile() const
        {
            return m_shader ? String( fromCText( wp_shader_get_profile( m_shader ) ) )
                            : Shader::getProfile();
        }

        void ClawShader::setProfile( const String &profile )
        {
            if( m_shader )
            {
                wp_shader_set_profile( m_shader, toCText( profile.c_str() ) );
                Shader::setProfile( String( fromCText( wp_shader_get_profile( m_shader ) ) ) );
            }
            else
            {
                Shader::setProfile( profile );
            }
        }

        Array<IShader::Define> ClawShader::getDefines() const
        {
            Array<Define> result;
            if( !m_shader )
            {
                return Shader::getDefines();
            }
            const auto count = wp_shader_get_define_count( m_shader );
            result.reserve( static_cast<size_t>( count ) );
            for( wp_s32 i = 0; i < count; ++i )
            {
                const auto *entry = wp_shader_get_define_at( m_shader, i );
                if( entry )
                {
                    result.push_back( Define{ String( fromCText( entry->name ) ),
                                              String( fromCText( entry->value ) ) } );
                }
            }
            return result;
        }

        void ClawShader::setDefines( const Array<Define> &defines )
        {
            if( !m_shader )
            {
                Shader::setDefines( defines );
                return;
            }
            wp_shader_clear_defines( m_shader );
            for( const auto &define : defines )
            {
                if( !wp_shader_set_define( m_shader, toCText( define.name.c_str() ),
                                           toCText( define.value.c_str() ) ) )
                {
                    WP_LOG_ERROR( "ClawShader: rejected invalid or excess shader define." );
                }
            }
            Shader::setDefines( getDefines() );
        }

        void ClawShader::setDefine( const String &name, const String &value )
        {
            if( m_shader )
            {
                if( wp_shader_set_define( m_shader, toCText( name.c_str() ), toCText( value.c_str() ) ) )
                {
                    Shader::setDefine( name, value );
                }
            }
            else
            {
                Shader::setDefine( name, value );
            }
        }

        void ClawShader::removeDefine( const String &name )
        {
            if( m_shader )
            {
                if( wp_shader_remove_define( m_shader, toCText( name.c_str() ) ) )
                {
                    Shader::removeDefine( name );
                }
            }
            else
            {
                Shader::removeDefine( name );
            }
        }

        void ClawShader::clearDefines()
        {
            if( m_shader )
            {
                wp_shader_clear_defines( m_shader );
            }
            Shader::clearDefines();
        }

        bool ClawShader::compile()
        {
            if(!m_shader)
            {
                WP_LOG_ERROR("ClawShader::compile: native shader is unavailable.");
                return false;
            }
            const bool defaultCompiler=wp_shader_get_compile_func(m_shader)==compileClawShader;
            const auto cacheNative=[&] {
#if defined(WP_PLATFORM_WIN32) && WP_BUILD_RENDERER_DX11
                if(defaultCompiler)
                {
                    auto blob=static_cast<ID3DBlob *>(wp_shader_get_native(m_shader));
                    if(blob!=m_compiledBlob)
                    {
                        if(blob) blob->AddRef();
                        if(m_compiledBlob) static_cast<ID3DBlob *>(m_compiledBlob)->Release();
                        m_compiledBlob=blob;
                    }
                    m_binary.clear();
                }
                else
                {
                    if(m_compiledBlob) static_cast<ID3DBlob *>(m_compiledBlob)->Release();
                    m_compiledBlob=nullptr;
                }
#endif
            };
            if(wp_shader_get_status(m_shader)==WORKPHONE_SHADER_STATUS_COMPILED &&
               !wp_shader_is_dirty(m_shader) && wp_shader_get_native(m_shader))
            {
                cacheNative(); m_status=Status::Compiled; return true;
            }
            m_stage=static_cast<Stage>(wp_shader_get_type(m_shader));
            m_language=fromCLanguage(wp_shader_get_language(m_shader));
            m_source=String(fromCText(wp_shader_get_source(m_shader)));
            m_entryPoint=String(fromCText(wp_shader_get_entry_point(m_shader)));
            m_profile=String(fromCText(wp_shader_get_profile(m_shader)));
            Shader::setDefines(getDefines());
            Shader::clearDiagnostics();
            m_specializationValues.clear();
            for(wp_s32 i=0;i<wp_shader_get_specialization_count(m_shader);++i)
            {
                const auto entry=wp_shader_get_specialization_at(m_shader,i);
                if(entry) m_specializationValues.push_back({String(fromCText(entry->name)),entry->value});
            }
            Array<u8> customBinary;
            if(!defaultCompiler)
            {
                wp_u32 size=0; const auto bytes=wp_shader_get_binary(m_shader,&size);
                if(bytes && size)
                {
                    customBinary.resize(size); std::memcpy(customBinary.data(),bytes,size);
                }
            }
            else wp_shader_set_release_func(m_shader,releaseClawShader,nullptr);
            // The C object validates authoring input and owns transactional native
            // replacement. A failed attempt never replaces this wrapper's last
            // successful output reference or exposes the new input as bytecode.
            if(!wp_shader_compile(m_shader))
            {
                m_status=Status::Failed;
                WP_LOG_ERROR("ClawShader::compile: "+String(fromCText(wp_shader_get_last_error(m_shader))));
                return false;
            }
            cacheNative();
            if(!defaultCompiler) m_binary=std::move(customBinary);
            m_status=Status::Compiled; return true;
        }

        bool ClawShader::reload()
        {
            if(m_shader) wp_shader_mark_dirty(m_shader);
            return compile();
        }

        void ClawShader::invalidate()
        {
            Shader::invalidate();
            if( m_shader )
            {
                wp_u32 binarySize = 0u;
                wp_shader_get_binary( m_shader, &binarySize );
                if( binarySize > 0u )
                {
                    wp_shader_set_binary( m_shader, nullptr, 0u );
                }
                else
                {
                    wp_shader_mark_dirty( m_shader );
                }
            }
        }

        IShader::Status ClawShader::getStatus() const
        {
            if(!m_shader) return Shader::getStatus();
            const auto status=wp_shader_get_status(m_shader);
            // set_binary is a raw C input operation and marks bytes compiled for
            // legacy callers. The default C++ backend must still validate them.
            if(status==WORKPHONE_SHADER_STATUS_COMPILED && wp_shader_is_dirty(m_shader) &&
               wp_shader_get_compile_func(m_shader)==compileClawShader) return Status::SourceReady;
            return fromCStatus(status);
        }

        bool ClawShader::isCompiled() const
        {
            return getStatus() == Status::Compiled;
        }

        Array<IShader::Diagnostic> ClawShader::getDiagnostics() const
        {
            auto diagnostics = Shader::getDiagnostics();
            if( m_shader )
            {
                const auto *rawError = wp_shader_get_last_error( m_shader );
                if( rawError && rawError[0] != 0 )
                {
                    const auto error = String( fromCText( rawError ) );
                    const auto found = std::find_if(
                        diagnostics.begin(), diagnostics.end(),
                        [&error]( const Diagnostic &entry ) { return entry.message == error; } );
                    if( found == diagnostics.end() )
                    {
                        Diagnostic diagnostic;
                        diagnostic.severity = DiagnosticSeverity::Error;
                        diagnostic.message = error;
                        diagnostics.push_back( diagnostic );
                    }
                }
            }
            return diagnostics;
        }

        void ClawShader::clearDiagnostics()
        {
            Shader::clearDiagnostics();
            if( m_shader )
            {
                wp_shader_set_last_error( m_shader, nullptr );
            }
        }

        Array<u8> ClawShader::getBinary() const
        {
#if defined(WP_PLATFORM_WIN32) && WP_BUILD_RENDERER_DX11
            auto compiled=m_compiledBlob;
            if(!compiled && m_shader && wp_shader_get_compile_func(m_shader)==compileClawShader)
                compiled=wp_shader_get_native(m_shader);
            if(compiled)
            {
                auto blob=static_cast<ID3DBlob *>(compiled);
                Array<u8> result(blob->GetBufferSize());
                if(!result.empty()) std::memcpy(result.data(),blob->GetBufferPointer(),result.size());
                return result;
            }
#endif
            return Shader::getBinary();
        }

        bool ClawShader::setBinary( const Array<u8> &binary, Language language )
        {
            const auto cLanguage = toCLanguage( language );
            if( !m_shader || binary.empty() || cLanguage == WORKPHONE_SHADER_LANGUAGE_UNKNOWN ||
                binary.size() > std::numeric_limits<wp_u32>::max() )
            {
                return false;
            }
            wp_shader_set_language( m_shader, cLanguage );
            if( !wp_shader_set_binary( m_shader, binary.data(), static_cast<wp_u32>( binary.size() ) ) )
            {
                return false;
            }
            // Submitted bytes are input until this backend has validated them.
            // Keep the previous output on malformed/unsupported replacement.
            return compile();
        }

        bool ClawShader::setSpecializationConstant( const String &name, u32 value )
        {
            if( m_shader && !wp_shader_set_specialization( m_shader, toCText( name.c_str() ), value ) )
            {
                return false;
            }
            return Shader::setSpecializationConstant( name, value );
        }

        bool ClawShader::getSpecializationConstant( const String &name, u32 &value ) const
        {
            if( !m_shader )
            {
                return Shader::getSpecializationConstant( name, value );
            }
            const auto count = wp_shader_get_specialization_count( m_shader );
            for( wp_s32 i = 0; i < count; ++i )
            {
                const auto *entry = wp_shader_get_specialization_at( m_shader, i );
                if( entry && String( fromCText( entry->name ) ) == name )
                {
                    value = entry->value;
                    return true;
                }
            }
            return false;
        }

        SmartPtr<Properties> ClawShader::getProperties() const
        {
            try
            {
                auto properties = Shader::getProperties();
                if( !properties )
                {
                    WP_LOG_WARNING( "ClawShader::getProperties: base class returned null properties." );
                    return {};
                }

                properties->setProperty( stageStr, static_cast<s32>( getStage() ) );
                properties->setProperty( languageStr, static_cast<s32>( getLanguage() ) );
                properties->setProperty( sourceStr, getSource() );
                properties->setProperty( entryPointStr, getEntryPoint() );
                properties->setProperty( profileStr, getProfile() );
                properties->setProperty( statusStr, static_cast<s32>( getStatus() ) );

                auto definesProperties = workphone::make_ptr<Properties>();
                definesProperties->setName( definesStr );
                properties->addChild( definesProperties );

                const auto defines = getDefines();
                for( size_t i = 0; i < defines.size(); ++i )
                {
                    auto key = "define" + String( std::to_string( i ) );
                    definesProperties->setProperty( key, defines[i].name + "=" + defines[i].value );
                }

                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return {};
        }

        void ClawShader::setProperties( SmartPtr<Properties> properties )
        {
            try
            {
                if( !properties )
                {
                    WP_LOG_WARNING( "ClawShader::setProperties: null properties supplied." );
                    return;
                }

                auto stage = static_cast<s32>( getStage() );
                if( properties->getPropertyValue( stageStr, stage ) )
                {
                    setStage( static_cast<Stage>( stage ) );
                }

                auto language = static_cast<s32>( getLanguage() );
                if( properties->getPropertyValue( languageStr, language ) )
                {
                    setLanguage( static_cast<Language>( language ) );
                }

                auto source = getSource();
                if( properties->getPropertyValue( sourceStr, source ) )
                {
                    setSource( source );
                }

                auto entryPoint = getEntryPoint();
                if( properties->getPropertyValue( entryPointStr, entryPoint ) )
                {
                    setEntryPoint( entryPoint );
                }

                auto profile = getProfile();
                if( properties->getPropertyValue( profileStr, profile ) )
                {
                    setProfile( profile );
                }

                if( auto definesProperties = properties->getChild( definesStr ) )
                {
                    clearDefines();

                    for( u32 i = 0; i < 256; ++i )
                    {
                        auto key = "define" + String( std::to_string( i ) );
                        auto defineStr = String();
                        if( !definesProperties->getPropertyValue( key, defineStr ) )
                        {
                            break;
                        }

                        if( defineStr.empty() )
                        {
                            continue;
                        }

                        auto eqPos = defineStr.find( '=' );
                        if( eqPos == String::npos )
                        {
                            setDefine( defineStr );
                        }
                        else
                        {
                            auto name = defineStr.substr( 0, eqPos );
                            auto value = defineStr.substr( eqPos + 1 );
                            setDefine( name, value );
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void ClawShader::getNativeHandle( void **handle ) const
        {
            if( handle )
            {
                *handle = m_shader ? wp_shader_get_native( m_shader ) : nullptr;
            }
        }

        wp_shader *ClawShader::getNativeShader() const
        {
            return m_shader;
        }

        bool ClawShader::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool ClawShader::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

    }  // namespace render
}  // namespace workphone
