#ifndef __Graphics_IShader_h__
#define __Graphics_IShader_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * Backend-independent shader contract.
         *
         * IShader deliberately contains no API-specific types.  A renderer can
         * implement it using GLSL, HLSL, Metal, SPIR-V, or a vendor compiler while
         * tools and materials use the same source, build, diagnostics and reflection
         * interface.  Implementations must make failed compilation transactional:
         * the last successfully compiled binary remains usable until a new compile
         * succeeds.
         */
        class WPCore_API IShader : public ISharedObject
        {
        public:
            enum class Stage : u8
            {
                Vertex,
                Fragment,
                Geometry,
                TessellationControl,
                TessellationEvaluation,
                Compute,
                RayGeneration,
                AnyHit,
                ClosestHit,
                Miss,
                Callable,
                Intersection
            };

            enum class Language : u8
            {
                Unknown,
                GLSL,
                HLSL,
                Metal,
                SPIRV,
                DXIL,
                DXBC,
                WGSL
            };

            enum class Status : u8
            {
                Empty,
                SourceReady,
                Compiling,
                Compiled,
                Failed
            };

            enum class DiagnosticSeverity : u8
            {
                Info,
                Warning,
                Error
            };

            enum class ResourceKind : u8
            {
                Uniform,
                StorageBuffer,
                SampledTexture,
                StorageTexture,
                Sampler,
                Input,
                Output,
                PushConstant,
                Unknown
            };

            struct Define
            {
                String name;
                String value;
            };

            struct Diagnostic
            {
                DiagnosticSeverity severity = DiagnosticSeverity::Info;
                String message;
                String file;
                u32 line = 0;
                u32 column = 0;
            };

            /** A reflected shader resource or vertex/interface attribute. */
            struct ResourceBinding
            {
                String name;
                ResourceKind kind = ResourceKind::Unknown;
                u32 set = 0;
                u32 binding = 0;
                u32 location = 0;
                u32 arraySize = 1;
                u32 byteSize = 0;
                bool writable = false;
            };

            struct Reflection
            {
                Array<ResourceBinding> resources;
                Array<String> inputs;
                Array<String> outputs;
                u32 pushConstantSize = 0;
            };

            /** Virtual destructor. */
            ~IShader() override;

            virtual Stage getStage() const = 0;
            virtual void setStage( Stage stage ) = 0;

            virtual Language getLanguage() const = 0;
            virtual void setLanguage( Language language ) = 0;

            /** Source text, or an empty string when the shader is binary-only. */
            virtual String getSource() const = 0;
            virtual void setSource( const String &source ) = 0;

            virtual String getEntryPoint() const = 0;
            virtual void setEntryPoint( const String &entryPoint ) = 0;

            /** Target profile such as "450", "vs_6_6", or "macos-metal2.3". */
            virtual String getProfile() const = 0;
            virtual void setProfile( const String &profile ) = 0;

            virtual Array<Define> getDefines() const = 0;
            virtual void setDefines( const Array<Define> &defines ) = 0;
            virtual void setDefine( const String &name, const String &value = String() ) = 0;
            virtual void removeDefine( const String &name ) = 0;
            virtual void clearDefines() = 0;

            /** Compiles source and refreshes reflection on success. */
            virtual bool compile() = 0;
            /** Recompiles using the current configuration. */
            virtual bool reload() = 0;
            virtual void invalidate() = 0;
            virtual Status getStatus() const = 0;
            virtual bool isCompiled() const = 0;
            virtual Array<Diagnostic> getDiagnostics() const = 0;
            virtual void clearDiagnostics() = 0;

            virtual Reflection getReflection() const = 0;
            virtual bool hasResource( const String &name ) const = 0;

            /** Gets the compiled backend binary (SPIR-V, DXIL, etc.). */
            virtual Array<u8> getBinary() const = 0;
            /** Supplies a precompiled binary and discards source diagnostics. */
            virtual bool setBinary( const Array<u8> &binary, Language language ) = 0;

            /** Sets a compile-time specialization value by reflected name. */
            virtual bool setSpecializationConstant( const String &name, u32 value ) = 0;
            virtual bool getSpecializationConstant( const String &name, u32 &value ) const = 0;

            /** Returns an opaque backend handle; ownership remains with the shader. */
            virtual void getNativeHandle( void **handle ) const = 0;

            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

            virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // __Graphics_IShader_h__
