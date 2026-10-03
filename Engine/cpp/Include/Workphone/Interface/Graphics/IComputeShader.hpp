#ifndef __Graphics_IComputeShader_h__
#define __Graphics_IComputeShader_h__

#include <Workphone/Interface/Graphics/IShader.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * Compute-specific shader interface.
         *
         * IComputeShader extends IShader with the information required to build
         * and schedule compute pipelines.  It is intentionally independent of a
         * command buffer or graphics API; a renderer maps these descriptions to
         * Vulkan, DirectX, Metal, OpenGL, or another backend.
         */
        class WPCore_API IComputeShader : public IShader
        {
        public:
            enum class ResourceAccess : u8
            {
                ReadOnly,
                WriteOnly,
                ReadWrite
            };

            enum class ResourceType : u8
            {
                UniformBuffer,
                StorageBuffer,
                SampledTexture,
                StorageTexture,
                Sampler,
                AccelerationStructure,
                Unknown
            };

            /** Workgroup dimensions declared by the shader or selected by the tool. */
            struct WPCore_API WorkgroupSize
            {
                u32 x = 1;
                u32 y = 1;
                u32 z = 1;
            };

            /** Number of workgroups for a direct dispatch. */
            struct WPCore_API DispatchSize
            {
                u32 x = 1;
                u32 y = 1;
                u32 z = 1;
            };

            /** Compute resource metadata, including its API-neutral binding. */
            struct WPCore_API ComputeBinding
            {
                String name;
                ResourceType type = ResourceType::Unknown;
                ResourceAccess access = ResourceAccess::ReadOnly;
                u32 set = 0;
                u32 binding = 0;
                u32 arraySize = 1;
                u32 byteSize = 0;
                bool optional = false;
            };

            virtual ~IComputeShader();

            /**
             * Returns the required local workgroup size.  A zero component means
             * that the shader leaves that dimension specialization-defined.
             */
            virtual WorkgroupSize getWorkgroupSize() const = 0;
            virtual void setWorkgroupSize( const WorkgroupSize &size ) = 0;

            virtual DispatchSize getDispatchSize() const = 0;
            virtual void setDispatchSize( const DispatchSize &size ) = 0;

            /** Validates dimensions against the shader and backend limits. */
            virtual bool validateDispatch( const DispatchSize &size,
                                           String *errorMessage = nullptr ) const = 0;

            /** Returns reflected compute resources in stable declaration order. */
            virtual Array<ComputeBinding> getComputeBindings() const = 0;
            virtual bool getComputeBinding( const String &name, ComputeBinding &binding ) const = 0;
            virtual bool hasComputeBinding( const String &name ) const = 0;

            /** Sets or removes an explicit binding override before pipeline creation. */
            virtual bool setBindingOverride( const String &name, u32 set, u32 binding ) = 0;
            virtual void clearBindingOverrides() = 0;

            /** Shared workgroup memory requested by the kernel, in bytes. */
            virtual u32 getSharedMemorySize() const = 0;
            virtual void setSharedMemorySize( u32 bytes ) = 0;

            /**
             * Checks whether all mandatory resources have been assigned.  This is
             * useful for editor validation before a pipeline is created.
             */
            virtual bool validateBindings( Array<Diagnostic> *diagnostics = nullptr ) const = 0;

            /** Builds the backend compute pipeline without submitting work. */
            virtual bool createPipeline() = 0;
            virtual void destroyPipeline() = 0;
            virtual bool hasPipeline() const = 0;

            /** Returns an opaque backend compute-pipeline handle. */
            virtual void getPipelineHandle( void **handle ) const = 0;

            /**
             * Returns an opaque backend resource used for indirect dispatch.  The
             * returned pointer is non-owning; offset is measured in bytes.
             */
            virtual bool setIndirectDispatchSource( void *buffer, u32 offset = 0 ) = 0;
            virtual void clearIndirectDispatchSource() = 0;
            virtual bool hasIndirectDispatchSource() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // end namespace workphone

#endif  // __Graphics_IComputeShader_h__
