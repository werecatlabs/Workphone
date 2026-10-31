#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/MeshResourceCompiler.hpp>
#include "../ClawAssetBudget.hpp"
#include <workphone_graphics_mesh_reader.h>
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <set>

namespace workphone::render
{
    namespace
    {
        constexpr u64 meshLimit=64ull<<20;
        void require(bool condition,const char *message) { if(!condition) throw std::runtime_error(message); }
        Array<u8> read(const std::filesystem::path &path,u64 limit)
        {
            std::ifstream input(path,std::ios::binary|std::ios::ate);
            const auto size=input?input.tellg():std::streampos(-1);
            require(size>0 && size<=limit,"Invalid mesh input size");
            Array<u8> result(static_cast<size_t>(size)); input.seekg(0);
            input.read(reinterpret_cast<char*>(result.data()),result.size()); require(bool(input),"Incomplete mesh input");
            return result;
        }
        Array<resource::ResourceID> bindings(const std::filesystem::path &path)
        {
            auto bytes=read(path,65536); boost::json::parse_options options; options.max_depth=8;
            const auto document=boost::json::parse(std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size()),{},options);
            require(document.is_array()&&!document.as_array().empty()&&document.as_array().size()<=128,
                "Mesh material bindings require a JSON array of 1 to 128 matres IDs");
            Array<resource::ResourceID> result;
            for(const auto &item:document.as_array()) {
                require(item.is_string()&&item.as_string().size()<=1024,"Invalid mesh material binding");
                String value(item.as_string().data(),item.as_string().size()); resource::ResourceID id(value);
                require(value.find('\0')==String::npos&&id.isValid()&&id.type()==resource::ResourceTypeID("matres"),
                    "Mesh material slots require matres ResourceIDs"); result.push_back(id);
            }
            return result;
        }
        bool check(const Array<u8> &bytes,const Array<resource::ResourceID> &materials,String &error)
        {
            if(bytes.size()<4||bytes.size()>meshLimit||std::memcmp(bytes.data(),"WPM1",4))
            { error="Static mesh requires WPM1 geometry of at most 64 MiB"; return false; }
            auto mesh=wp_graphics_mesh_read_from_buffer_budgeted(bytes.data(),static_cast<wp_u32>(bytes.size()),
                &asset_budget::meshMemoryCallbacks);
            if(!mesh) { error="Invalid mesh or exhausted mesh memory budget"; return false; }
            struct Cleanup { wp_graphics_mesh *mesh; ~Cleanup(){wp_graphics_mesh_destroy(mesh);} } cleanup{mesh};
            const auto count=wp_graphics_mesh_get_submesh_count(mesh);
            if(!wp_graphics_mesh_get_vertex_count(mesh)||!wp_graphics_mesh_get_index_count(mesh)||
                (!materials.empty()&&materials.size()!=static_cast<size_t>(count)))
            { error="Mesh geometry or material slot count is invalid"; return false; }
            for(wp_s32 i=0;i<count;++i)
                if(materials.empty()&&wp_graphics_mesh_get_submesh(mesh,i)->material_id)
                { error="Legacy mesh material IDs require explicit .mesh.materials bindings"; return false; }
            return true;
        }
        void word(std::ostream &out,u32 value) { for(int i=0;i<4;++i) out.put(char(value>>(i*8))); }
        void text(std::ostream &out,const String &value) { word(out,u32(value.size())); out.write(value.data(),value.size()); }
        struct Reader {
            const Array<u8> &bytes; size_t offset=4;
            u32 word() { require(offset<=bytes.size()&&bytes.size()-offset>=4,"Truncated cooked mesh");
                u32 result=0; for(int i=0;i<4;++i) result|=u32(bytes[offset++])<<(i*8); return result; }
            String text() { auto count=word(); require(count>0&&count<=1024&&count<=bytes.size()-offset,"Invalid cooked mesh string");
                String result(reinterpret_cast<const char*>(bytes.data()+offset),count); offset+=count;
                require(result.find('\0')==String::npos,"NUL in cooked mesh string"); return result; }
        };
    }
    String MeshResourceCompiler::name() const { return "WorkphoneStaticWpmMesh"; }
    Array<resource::CompilerOutput> MeshResourceCompiler::outputs() const
    { return {{resource::ResourceTypeID("mesh"),2}}; }
    bool MeshResourceCompiler::getDependencies(const resource::CompileContext &context,
        resource::DependencySet &dependencies,String &error) const
    {
        try {
            require(!context.resourceId.isSubResource(),"Static mesh subasset selection is not supported");
            const auto logical="data://"+context.resourceId.sourceRelativePath()+".materials";
            std::filesystem::path path=std::filesystem::u8path(context.sourcePath.c_str()); path+=".materials";
            if(context.snapshotInput) {
                String captured; if(!context.snapshotInput(logical,false,captured,error)) return false;
                if(captured.empty()) return true; path=std::filesystem::u8path(captured.c_str());
            } else if(!std::filesystem::exists(path)) return true;
            const auto ids=bindings(path); dependencies.compileDependencies.push_back(resource::CompileDependency::data(logical));
            std::set<String> seen;
            for(const auto &id:ids) if(seen.insert(id.str()).second) {
                dependencies.compileDependencies.push_back(resource::CompileDependency::resource(id));
                dependencies.installDependencies.push_back(id);
            }
            return true;
        } catch(const std::exception &e) { error=e.what(); return false; }
    }
    resource::CompilationStatus MeshResourceCompiler::compile(const resource::CompileContext &context,
        std::ostream &output,Array<String> &messages) const
    {
        try {
            require(!context.resourceId.isSubResource(),"Static mesh subasset selection is not supported");
            const auto inputPath=std::filesystem::u8path(context.sourcePath.c_str());
            const auto inputSize=std::filesystem::file_size(inputPath);
            require(inputSize>0&&inputSize<=meshLimit,"Invalid mesh input size");
            auto budget=asset_budget::reserveCpuBytes(inputSize+196608,true); require(bool(budget),"Mesh input budget exhausted");
            auto bytes=read(inputPath,meshLimit);
            auto sidecar=std::filesystem::u8path(context.sourceRoot.c_str())/
                std::filesystem::u8path((context.resourceId.sourceRelativePath()+".materials").c_str());
            Array<resource::ResourceID> materials; if(std::filesystem::exists(sidecar)) materials=bindings(sidecar);
            String error; require(check(bytes,materials,error),error.c_str());
            require(!context.target.empty()&&context.target.size()<=1024,"Invalid mesh target");
            output.write("WPMR",4); word(output,1); word(output,context.packagedBuild?1:0); text(output,context.target);
            word(output,u32(materials.size())); for(const auto &id:materials) text(output,id.str());
            word(output,u32(bytes.size())); output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
            return output?resource::CompilationStatus::Success:resource::CompilationStatus::Failure;
        } catch(const std::exception &e) { messages.push_back(e.what()); return resource::CompilationStatus::Failure; }
    }
    bool MeshResourceCompiler::decode(const resource::RuntimeResource &value,CookedMeshData &output,String &error)
    {
        error.clear();
        try {
            const auto type=resource::ResourceTypeID("mesh"); const MeshResourceCompiler compiler;
            require(value.header.isValid()&&value.header.resourceType==type&&value.header.resourceId.type()==type&&!value.header.resourceId.isSubResource(),
                "Unsupported cooked mesh identity or subasset selector");
            require(value.payload.size()<=meshLimit+(256ull<<10)&&value.header.payloadSize==value.payload.size()&&
                resource::hashBytes(value.payload.data(),value.payload.size())==value.header.payloadHash,
                "Cooked mesh payload size or integrity is invalid");
            CookedMeshData decoded; size_t offset=0,count=value.payload.size();
            const auto legacy=resource::combineHash(resource::combineHash(resource::combineHash(14695981039346656037ull,1),1),type.value());
            if(value.header.compilerVersion==legacy) {
                require(value.payload.size()>=4&&!std::memcmp(value.payload.data(),"WPM1",4),"Invalid legacy mesh");
                require(value.header.installDependencies.empty(),"Legacy mesh cannot bind install dependencies");
            } else {
                require(value.header.compilerVersion==compiler.versionFor(type)&&value.payload.size()>=4&&
                    !std::memcmp(value.payload.data(),"WPMR",4),"Unsupported cooked mesh version");
                Reader reader{value.payload}; require(reader.word()==1,"Unsupported mesh payload version");
                const auto flags=reader.word(); require(flags<=1,"Invalid mesh flags"); decoded.packagedBuild=flags!=0;
                decoded.target=reader.text(); const auto slots=reader.word(); require(slots<=128,"Too many mesh material slots");
                std::set<String> expected;
                for(u32 i=0;i<slots;++i) { resource::ResourceID id(reader.text());
                    require(id.isValid()&&id.type()==resource::ResourceTypeID("matres"),"Invalid cooked material slot");
                    decoded.materials.push_back(id); expected.insert(id.str()); }
                std::set<String> installed;
                for(const auto &id:value.header.installDependencies) installed.insert(id.str());
                require(expected==installed,"Mesh install dependencies differ from its material slots");
                count=reader.word(); offset=reader.offset;
                require(offset<=value.payload.size()&&count==value.payload.size()-offset,"Invalid mesh geometry span");
            }
            require(count>0&&count<=meshLimit,"Mesh geometry exceeds limit");
            decoded.budget=asset_budget::reserveCpuBytes(count,true); require(bool(decoded.budget),"Mesh decode budget exhausted");
            decoded.geometry.assign(value.payload.begin()+offset,value.payload.end());
            require(check(decoded.geometry,decoded.materials,error),error.c_str()); output=std::move(decoded); return true;
        } catch(const std::exception &e) { error=e.what(); return false; }
    }
    bool MeshResourceCompiler::validate(const resource::RuntimeResource &value,String &error)
    { CookedMeshData decoded; return decode(value,decoded,error); }
}
