#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/SkinnedMeshResource.hpp>
#include <WPGraphics/Resources/MeshResourceCompiler.hpp>
#include <Workphone/System/SkeletonResourceCompiler.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include "../ClawAssetBudget.hpp"
#include <workphone_graphics_mesh_reader.h>
#include <workphone_graphics_skinning.h>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <cstring>
#include <cmath>

namespace workphone::render
{
    namespace
    {
        constexpr size_t maximumVertices=65536,maximumSource=8u<<20;
        void require(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
        struct JsonBudget : rapidjson::BaseReaderHandler<rapidjson::UTF8<>,JsonBudget>
        {
            size_t count=0; unsigned depth=0;
            bool value(){return ++count<=1000000;}
            bool StartObject(){return value() && ++depth<=8;}
            bool StartArray(){return value() && ++depth<=8;}
            bool EndObject(rapidjson::SizeType){if(!depth)return false;--depth;return true;}
            bool EndArray(rapidjson::SizeType){if(!depth)return false;--depth;return true;}
            bool Key(const char*,rapidjson::SizeType length,bool){return length<=128 && value();}
            bool String(const char*,rapidjson::SizeType length,bool){return length<=1024 && value();}
            bool Null(){return value();} bool Bool(bool){return value();}
            bool Int(int){return value();} bool Uint(unsigned){return value();}
            bool Int64(int64_t){return value();} bool Uint64(uint64_t){return value();}
            bool Double(double){return value();}
        };
        void fields(const rapidjson::Value &object,const std::set<std::string> &expected)
        {
            require(object.IsObject() && object.MemberCount()==expected.size(),"Invalid skin descriptor fields");
            std::set<std::string> seen;
            for(auto i=object.MemberBegin();i!=object.MemberEnd();++i) {
                const std::string name(i->name.GetString(),i->name.GetStringLength());
                require(expected.count(name) && seen.insert(name).second,"Unknown or duplicate skin descriptor field");
            }
        }
        resource::ResourceID identity(const rapidjson::Value &value,const char *type)
        {
            require(value.IsString() && value.GetStringLength()>0 && value.GetStringLength()<=1024,"Invalid skin dependency ID");
            String text(value.GetString(),value.GetStringLength()); resource::ResourceID id(text);
            require(text.find('\0')==String::npos && id.isValid() && !id.isSubResource() && id.type()==resource::ResourceTypeID(type),"Skin dependency has an unsupported type/subasset");
            return id;
        }
        void weights(const CookedSkinInfluence &vertex)
        {
            double sum=0;
            for(size_t i=0;i<4;++i) {
                require(vertex.joints[i]<WP_SKIN_MAX_JOINTS,"Skin joint exceeds native joint limit");
                require(std::isfinite(vertex.weights[i]) && vertex.weights[i]>=0 && vertex.weights[i]<=1,"Invalid finite skin weight");
                sum+=vertex.weights[i];
            }
            require(std::abs(sum-1)<=0.00001,"Skin weights must sum to one");
        }
        CookedSkinnedMeshData source(const resource::CompileContext &context)
        {
            require(context.resourceId.type()==resource::ResourceTypeID("skinres") && !context.resourceId.isSubResource(),"Skin compiler requires a skinres descriptor");
            std::ifstream file(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);
            const auto length=file?file.tellg():std::streampos(-1);
            require(length>0 && length<=maximumSource,"Skin source exceeds eight MiB or is unavailable");
            auto budget=asset_budget::reserveCpuBytes(static_cast<u64>(length)*12+65536,true);
            require(bool(budget),"Skin parser working budget exhausted");
            String text(static_cast<size_t>(length),'\0'); file.seekg(0); file.read(&text[0],text.size());
            require(bool(file) && text.find('\0')==String::npos,"Invalid skin source bytes");
            rapidjson::StringStream stream(text.c_str()); rapidjson::Reader reader; JsonBudget limits;
            require(reader.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(stream,limits),"Skin JSON exceeds depth/token/string limits or is malformed");
            rapidjson::Document document; document.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(text.data(),text.size());
            require(!document.HasParseError(),"Malformed skin JSON"); fields(document,{"version","mesh","skeleton","vertices"});
            require(document["version"].IsUint() && document["version"].GetUint()==1,"Unsupported skin source version");
            CookedSkinnedMeshData result; result.mesh=identity(document["mesh"],"mesh"); result.skeleton=identity(document["skeleton"],"skelres");
            const auto &vertices=document["vertices"];
            require(vertices.IsArray() && !vertices.Empty() && vertices.Size()<=maximumVertices,"Skin requires 1 to 65536 vertex influences");
            result.vertices.reserve(vertices.Size());
            for(const auto &value:vertices.GetArray()) {
                fields(value,{"joints","weights"}); const auto &j=value["joints"],&w=value["weights"];
                require(j.IsArray() && w.IsArray() && j.Size()==4 && w.Size()==4,"Skin vertices require exactly four joints and weights");
                CookedSkinInfluence vertex;
                for(rapidjson::SizeType i=0;i<4;++i) {
                    require(j[i].IsUint() && j[i].GetUint()<WP_SKIN_MAX_JOINTS && w[i].IsNumber(),"Invalid skin influence scalar");
                    const auto weight=w[i].GetDouble(); require(std::isfinite(weight) && weight>=0 && weight<=1,"Invalid skin weight");
                    vertex.joints[i]=static_cast<u16>(j[i].GetUint()); vertex.weights[i]=static_cast<f32>(weight);
                }
                weights(vertex); float sum=0; for(const auto value:vertex.weights) sum+=value;
                for(auto &value:vertex.weights) value/=sum;
                result.vertices.push_back(vertex);
            }
            return result;
        }
        struct DependencyInput
        {
            // Payload/header storage must die before its working admission lease.
            StreamBudget budget;
            resource::RuntimeResource value;
        };
        DependencyInput dependency(const resource::CompileContext &context,const resource::ResourceID &id,u64 maximum)
        {
            const auto found=context.dependencyArtifacts.find(id.str()); require(found!=context.dependencyArtifacts.end(),"Missing pinned skin compile dependency");
            std::error_code failure;
            const auto bytes=std::filesystem::file_size(std::filesystem::u8path(found->second.c_str()),failure);
            require(!failure && bytes>0 && bytes<=maximum+(8ull<<20),"Skin dependency container exceeds payload/header bounds");
            DependencyInput input;
            // The reader's encoded header cap is 8 MiB. Reserve additional
            // conservative space for normalized strings, vectors and temporary
            // uniqueness checks before any default-Array payload allocation.
            input.budget=asset_budget::reserveCpuBytes(bytes+(32ull<<20),true);
            require(bool(input.budget),"Pinned skin dependency working budget exhausted");
            String error;
            const auto loaded=resource::CompiledResourceIO::read(found->second,input.value,error,maximum);
            require(loaded,error.c_str());
            require(input.value.header.resourceId==id,"Skin compile dependency identity mismatch"); return input;
        }
        void word(std::ostream &out,u32 value){for(unsigned i=0;i<4;++i)out.put(char(value>>(8*i)));}
        void text(std::ostream &out,const String &value){word(out,u32(value.size()));out.write(value.data(),value.size());}
        struct Reader
        {
            const Array<u8> &bytes; size_t offset=4;
            u32 word(){require(offset<=bytes.size() && bytes.size()-offset>=4,"Truncated skin payload");u32 value=0;for(unsigned i=0;i<4;++i)value|=u32(bytes[offset++])<<(8*i);return value;}
            u16 joint(){require(offset<=bytes.size() && bytes.size()-offset>=2,"Truncated skin joint");const auto value=u16(bytes[offset])|u16(bytes[offset+1])<<8;offset+=2;return value;}
            String text(size_t maximum){const auto length=word();require(length>0 && length<=maximum && length<=bytes.size()-offset,"Invalid skin payload string");String value(reinterpret_cast<const char*>(bytes.data()+offset),length);offset+=length;require(value.find('\0')==String::npos,"NUL in skin payload");return value;}
        };
    }
    String SkinnedMeshResourceCompiler::name() const { return "WorkphoneNativeSkinnedMesh"; }
    Array<resource::CompilerOutput> SkinnedMeshResourceCompiler::outputs() const { return {{resource::ResourceTypeID("skinres"),1}}; }
    bool SkinnedMeshResourceCompiler::getDependencies(const resource::CompileContext &context,resource::DependencySet &dependencies,String &error) const
    {
        try { const auto parsed=source(context);
            for(const auto &id:{parsed.mesh,parsed.skeleton}) { dependencies.compileDependencies.push_back(resource::CompileDependency::resource(id)); dependencies.installDependencies.push_back(id); }
            return true;
        } catch(const std::exception &failure){error=failure.what();return false;}
    }
    resource::CompilationStatus SkinnedMeshResourceCompiler::compile(const resource::CompileContext &context,std::ostream &output,Array<String> &messages) const
    {
        try {
            auto parsed=source(context); String error;
            const auto mesh=dependency(context,parsed.mesh,(64ull<<20)+(256ull<<10)); CookedMeshData geometry;
            const auto meshDecoded=MeshResourceCompiler::decode(mesh.value,geometry,error); require(meshDecoded,error.c_str());
            require(geometry.target==context.target && geometry.packagedBuild==context.packagedBuild,"Skin mesh dependency target/build mode mismatch");
            auto native=wp_graphics_mesh_read_from_buffer_budgeted(geometry.geometry.data(),static_cast<wp_u32>(geometry.geometry.size()),&asset_budget::meshMemoryCallbacks);
            require(native!=nullptr,"Cannot decode skin mesh geometry");
            struct Cleanup{wp_graphics_mesh *mesh;~Cleanup(){wp_graphics_mesh_destroy(mesh);}}cleanup{native};
            const auto format=wp_graphics_mesh_get_vertex_format(native);
            require((format==WORKPHONE_VERTEX_FORMAT_PNT || format==WORKPHONE_VERTEX_FORMAT_PNTC) && wp_graphics_mesh_get_vertex_count(native)==parsed.vertices.size(),"Skin influence count/vertex format differs from mesh geometry");
            const auto stride=wp_graphics_mesh_get_vertex_stride(native); const auto bytes=static_cast<const u8*>(wp_graphics_mesh_get_vertices(native));
            require(bytes && stride>=2*sizeof(wp_vec3f),"Missing skin bind geometry");
            for(size_t index=0;index<parsed.vertices.size();++index) {
                wp_vec3f position,normal; std::memcpy(&position,bytes+index*stride,sizeof(position)); std::memcpy(&normal,bytes+index*stride+sizeof(position),sizeof(normal));
                for(const auto value:{position.x,position.y,position.z,normal.x,normal.y,normal.z}) require(std::isfinite(value),"Non-finite skin bind attributes");
                for(const auto value:{position.x,position.y,position.z}) require(std::abs(value)<=1000000,"Skin bind positions exceed native deformation range");
            }
            const auto skeleton=dependency(context,parsed.skeleton,2ull<<20);
            auto rig=resource::SkeletonResourceCompiler::decode(skeleton.value,error); require(rig!=nullptr,error.c_str());
            bool present[WP_SKIN_MAX_JOINTS]{}; for(const auto &bone:rig->getBones()) if(bone) present[bone->getBoneHandle()]=true;
            for(const auto &vertex:parsed.vertices) for(const auto joint:vertex.joints) require(present[joint],"Skin joint is absent from its skeleton dependency");
            require(!context.target.empty() && context.target.size()<=128,"Invalid skin target");
            output.write("WPSK",4); word(output,1); word(output,context.packagedBuild?1:0); text(output,context.target);
            text(output,parsed.mesh.str()); text(output,parsed.skeleton.str()); word(output,u32(parsed.vertices.size()));
            for(const auto &vertex:parsed.vertices) {
                for(const auto joint:vertex.joints){output.put(char(joint));output.put(char(joint>>8));}
                for(const auto weight:vertex.weights){u32 bits=0;static_assert(sizeof(bits)==sizeof(weight));std::memcpy(&bits,&weight,sizeof(bits));word(output,bits);}
            }
            return output?resource::CompilationStatus::Success:resource::CompilationStatus::Failure;
        } catch(const std::exception &failure){messages.push_back(failure.what());return resource::CompilationStatus::Failure;}
    }
    bool SkinnedMeshResourceCompiler::decode(const resource::RuntimeResource &resource,CookedSkinnedMeshData &output,String &error)
    {
        error.clear();
        try {
            const auto type=workphone::resource::ResourceTypeID("skinres"); const SkinnedMeshResourceCompiler compiler;
            require(resource.header.isValid() && resource.header.resourceType==type && resource.header.resourceId.type()==type && !resource.header.resourceId.isSubResource() && resource.header.compilerVersion==compiler.versionFor(type),"Unsupported cooked skin header/identity/version");
            require(resource.payload.size()>=4 && resource.payload.size()<=(2u<<20) && !std::memcmp(resource.payload.data(),"WPSK",4),"Invalid cooked skin payload");
            require(resource.header.payloadSize==resource.payload.size() &&
                workphone::resource::hashBytes(resource.payload.data(),resource.payload.size())==resource.header.payloadHash,"Cooked skin payload size/hash mismatch");
            Reader reader{resource.payload}; require(reader.word()==1,"Unsupported skin envelope version");
            const auto flags=reader.word(); require(flags<=1,"Invalid skin envelope flags"); CookedSkinnedMeshData candidate; candidate.packagedBuild=flags!=0;
            candidate.target=reader.text(128); const auto mesh=reader.text(1024),skeleton=reader.text(1024);
            require(candidate.mesh.set(mesh) && candidate.mesh.type()==workphone::resource::ResourceTypeID("mesh") && !candidate.mesh.isSubResource() &&
                candidate.skeleton.set(skeleton) && candidate.skeleton.type()==workphone::resource::ResourceTypeID("skelres") && !candidate.skeleton.isSubResource(),"Invalid typed skin dependency IDs");
            require(candidate.mesh.str()==mesh && candidate.skeleton.str()==skeleton,"Skin dependency IDs are not canonical");
            std::set<String> expected{candidate.mesh.str(),candidate.skeleton.str()},actual;
            for(const auto &id:resource.header.installDependencies) require(actual.insert(id.str()).second,"Duplicate skin install dependency");
            require(expected==actual,"Skin install dependencies differ from its envelope");
            const auto count=reader.word(); require(count>0 && count<=maximumVertices && reader.offset<=resource.payload.size() && resource.payload.size()-reader.offset==u64(count)*24,"Invalid skin vertex payload span");
            candidate.vertices.resize(count);
            for(auto &vertex:candidate.vertices){for(auto &joint:vertex.joints)joint=reader.joint();for(auto &weight:vertex.weights){const auto bits=reader.word();std::memcpy(&weight,&bits,sizeof(weight));}weights(vertex);}
            output=std::move(candidate);return true;
        } catch(const std::exception &failure){error=failure.what();return false;}
    }
}
