#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/MeshResourceCompiler.hpp>
#include "../ClawAssetBudget.hpp"
#include <workphone_graphics_mesh_reader.h>
#include <filesystem>
#include <fstream>
#include <cstring>

namespace workphone::render
{
    namespace
    {
        bool check(const Array<u8> &bytes, String &error)
        {
            if(bytes.size()<4 || bytes.size()>64ull*1024*1024 || std::memcmp(bytes.data(),"WPM1",4))
            { error="Static mesh requires a WPM1 payload of at most 64 MiB"; return false; }
            auto mesh=wp_graphics_mesh_read_from_buffer_budgeted(bytes.data(),static_cast<wp_u32>(bytes.size()),
                &asset_budget::meshMemoryCallbacks);
            if(!mesh) { error="Invalid mesh or exhausted mesh memory budget"; return false; }
            struct Cleanup { wp_graphics_mesh *mesh; ~Cleanup(){wp_graphics_mesh_destroy(mesh);} } cleanup{mesh};
            if(!wp_graphics_mesh_get_vertex_count(mesh) || !wp_graphics_mesh_get_index_count(mesh))
            { error="Cooked static mesh requires nonempty geometry"; return false; }
            for(wp_s32 i=0;i<wp_graphics_mesh_get_submesh_count(mesh);++i)
                if(wp_graphics_mesh_get_submesh(mesh,i)->material_id)
                { error="Static mesh material IDs require explicit typed bindings; recook with unbound geometry"; return false; }
            return true;
        }
    }
    String MeshResourceCompiler::name() const { return "WorkphoneStaticWpmMesh"; }
    Array<resource::CompilerOutput> MeshResourceCompiler::outputs() const
    { return {{resource::ResourceTypeID("mesh"),1}}; }
    resource::CompilationStatus MeshResourceCompiler::compile(const resource::CompileContext &context,
        std::ostream &output,Array<String> &messages) const
    {
        std::ifstream input(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);
        const auto size=input?input.tellg():std::streampos(-1);
        if(size<4 || size>64ull*1024*1024)
        { messages.push_back("Invalid static mesh source size"); return resource::CompilationStatus::Failure; }
        auto budget=asset_budget::reserveCpuBytes(static_cast<u64>(size),true);
        if(!budget) { messages.push_back("Static mesh input budget exhausted"); return resource::CompilationStatus::Failure; }
        Array<u8> bytes(static_cast<size_t>(size)); input.seekg(0);
        input.read(reinterpret_cast<char*>(bytes.data()),bytes.size()); String error;
        if(!input || !check(bytes,error))
        { messages.push_back(error.empty()?"Incomplete static mesh read":error); return resource::CompilationStatus::Failure; }
        output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
        return output?resource::CompilationStatus::Success:resource::CompilationStatus::Failure;
    }
    bool MeshResourceCompiler::validate(const resource::RuntimeResource &value,String &error)
    {
        error.clear(); const MeshResourceCompiler compiler;
        if(value.header.resourceType!=resource::ResourceTypeID("mesh") ||
            value.header.resourceId.type()!=resource::ResourceTypeID("mesh") ||
            value.header.compilerVersion!=compiler.versionFor(resource::ResourceTypeID("mesh")))
        { error="Unsupported cooked mesh identity or version"; return false; }
        return check(value.payload,error);
    }
}
