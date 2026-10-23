#ifndef WP_MATERIAL_PROGRAM_BINDING_HPP
#define WP_MATERIAL_PROGRAM_BINDING_HPP

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/MaterialShaderParameters.hpp>
#include <WorkphoneGraphics/workphone_graphics_shader.h>
#include <vector>

namespace workphone::render
{
    // Temporary reflection input. Names are resolved only when creating a map.
    struct MaterialProgramBuffer
    {
        u32 slot = 0;
        u32 size = 0;
        u32 packedOffset = 0; // Assigned by Create; input value is ignored.
    };
    struct MaterialProgramField
    {
        String name;
        MaterialShaderParameterType type = MaterialShaderParameterType::Scalar;
        u32 bufferIndex = 0;
        u32 offset = 0;
        u32 size = 0;
        bool columnMajor = false; // Only valid for a 4x4 float Matrix.
    };
    struct MaterialProgramResource
    {
        String name;
        MaterialShaderParameterType type = MaterialShaderParameterType::Texture;
        u32 slot = 0;
    };
    struct MaterialProgramResourceValue
    {
        MaterialShaderParameterType type = MaterialShaderParameterType::Texture;
        u32 slot = 0;
        MaterialResourceHandle handle = 0;
    };
    // Explicit ownership for programs that also contain view/object/lighting
    // bindings. When supplied, empty lists select none; every listed name must
    // exist. A null selection maps all reflected buffers and resources.
    struct MaterialProgramReflectionSelection
    {
        Array<String> constantBuffers;
        Array<String> resources;
    };

    /** Immutable CPU-schema to backend map. Does not modify the program's shared
     * parameter buffer. Retains the schema and program; callers own packed values
     * and resolve numeric resource handles through their backend resource owner.
     * Program mutation, map creation and packing require owning-thread use or
     * external synchronization, as does wp_shader_program itself.
     */
    class WPGraphics_API MaterialProgramBindingMap
    {
    public:
        using Ptr = std::shared_ptr<const MaterialProgramBindingMap>;
        static Ptr Create( MaterialShaderParameterLayout::Ptr layout, wp_shader_program *program,
                           u64 deviceGeneration, wp_shader_type stage,
                           const Array<MaterialProgramBuffer> &buffers,
                           const Array<MaterialProgramField> &fields,
                           const Array<MaterialProgramResource> &resources, String &error );
        ~MaterialProgramBindingMap();
        MaterialProgramBindingMap() = default;
        MaterialProgramBindingMap( const MaterialProgramBindingMap & ) = delete;
        MaterialProgramBindingMap &operator=( const MaterialProgramBindingMap & ) = delete;

        // Constant-time validity check for this map's fixed stage/selection.
        // Cache owners must also distinguish stage and reflection selection.
        // Call after wp_shader_program_update observes attached-stage edits.
        // Pending pipeline changes invalidate the map; publish a new map together
        // with the successfully linked native.
        bool IsCurrent( const MaterialShaderParameterLayout::Ptr &layout,
                        const wp_shader_program *program, u64 deviceGeneration ) const;
        // No allocation or name lookup. Failure preserves both output regions.
        // Outputs must not overlap each other or the instance's borrowed bytes.
        bool Pack( const MaterialShaderParametersInstance &instance, u64 deviceGeneration,
                   void *bytes, size_t byteCapacity, MaterialProgramResourceValue *resources,
                   size_t resourceCapacity ) const;
        u32 GetPackedSize() const { return m_packedSize; }
        size_t GetResourceCount() const { return m_resources.size(); }
        size_t GetBufferCount() const { return m_buffers.size(); }
        const MaterialProgramBuffer *GetBuffer( size_t index ) const;
        wp_shader_type GetStage() const { return m_stage; }
        // Owned payload/capacity, excluding shared schema/program and allocator overhead.
        size_t GetStorageBytes() const;
    private:
        struct Copy { u32 source, destination, size; bool transpose; };
        struct Resource { u32 source, slot; MaterialShaderParameterType type; };
        template <class T> using Storage = std::vector<T, SourceBudgetAllocator<T>>;
        MaterialShaderParameterLayout::Ptr m_layout;
        wp_shader_program *m_program = nullptr;
        u64 m_deviceGeneration = 0;
        u32 m_definitionRevision = 0, m_pipelineRevision = 0, m_linkedGeneration = 0;
        u32 m_packedSize = 0;
        wp_shader_type m_stage = WORKPHONE_SHADER_TYPE_VERTEX;
        Storage<MaterialProgramBuffer> m_buffers;
        Storage<Copy> m_copies;
        Storage<Resource> m_resources;
    };

    /** Reflect one compiled DXBC stage. Numeric arrays, structs, float3, non-4x4
     * matrices, UAVs and resource BindCount != 1 are rejected. Optimized resource
     * array[1] declarations can be indistinguishable from singleton bytecode.
     * The caller supplies the program definition corresponding to this bytecode.
     * This additive adapter does not install arbitrary shaders in built-in draws.
     */
    WPGraphics_API MaterialProgramBindingMap::Ptr CreateDX11MaterialProgramBindingMap(
        MaterialShaderParameterLayout::Ptr layout, wp_shader_program *program,
        u64 deviceGeneration, wp_shader_type stage, const void *bytecode,
        size_t bytecodeSize, String &error,
        const MaterialProgramReflectionSelection *selection = nullptr );
}
#endif
