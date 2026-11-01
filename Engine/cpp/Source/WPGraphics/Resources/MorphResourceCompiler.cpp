#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/MorphResourceCompiler.hpp>
#include <WPGraphics/Resources/MeshResourceCompiler.hpp>
#include "../ClawAssetBudget.hpp"
#include <workphone_graphics_mesh_reader.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <cmath>
#include <array>
#include <algorithm>

namespace workphone::render
{
    namespace
    {
        void require(bool test,const char *message) { if(!test) throw std::runtime_error(message); }
        void require(bool test,const String &message) { if(!test) throw std::runtime_error(message.c_str()); }
        resource::MorphClipData source(const resource::CompileContext &context)
        {
            require(context.resourceId.type()==resource::ResourceTypeID("amorph")&&!context.resourceId.isSubResource(),"Morph compiler requires an amorph root");
            std::ifstream file(std::filesystem::u8path(context.sourcePath.c_str()),std::ios::binary|std::ios::ate);
            const auto length=file?file.tellg():std::streampos(-1);
            require(length>0&&length<=resource::MorphResourceCodec::maximumSourceBytes,"Morph source is unavailable or exceeds four MiB");
            auto working=asset_budget::reserveCpuBytes(u64(length)+65536,true);
            require(bool(working),"Morph source read working budget exhausted");
            String text(static_cast<size_t>(length),'\0');file.seekg(0);file.read(&text[0],text.size());require(bool(file),"Incomplete morph source read");
            resource::MorphClipData result;String error;
            const auto parsed=resource::MorphResourceCodec::parseSource(text,result,error);require(parsed,error);
            return result;
        }
        void hash(u64 &value,const void *data,size_t count)
        { const auto bytes=static_cast<const u8*>(data);for(size_t i=0;i<count;++i){value^=bytes[i];value*=1099511628211ull;} }
        void word(u64 &value,u32 number)
        { for(unsigned i=0;i<4;++i){const u8 byte=u8(number>>(8*i));hash(value,&byte,1);} }
    }
    String MorphResourceCompiler::name() const {return "WorkphoneStaticMorph";}
    Array<resource::CompilerOutput> MorphResourceCompiler::outputs() const
    {return {{resource::ResourceTypeID("amorph"),1}};}
    bool MorphResourceCompiler::getDependencies(const resource::CompileContext &context,resource::DependencySet &dependencies,String &error) const
    {
        try {const auto parsed=source(context);dependencies.compileDependencies.push_back(resource::CompileDependency::resource(parsed.info.mesh));dependencies.installDependencies.push_back(parsed.info.mesh);return true;}
        catch(const std::exception &failure){error=failure.what();return false;}
    }
    bool MorphResourceCompiler::signature(wp_graphics_mesh *mesh,resource::MorphClipInfo &info,String &error)
    {
        error.clear();
        try {
            require(mesh!=nullptr,"Morph base has no native mesh");
            const auto format=wp_graphics_mesh_get_vertex_format(mesh);
            const auto count=wp_graphics_mesh_get_vertex_count(mesh),stride=wp_graphics_mesh_get_vertex_stride(mesh);
            require((format==WORKPHONE_VERTEX_FORMAT_PNT||format==WORKPHONE_VERTEX_FORMAT_PNTC)&&count>0&&count<=1000000,"Morph requires static PNT/PNTC geometry with at most one million vertices");
            require(stride==(format==WORKPHONE_VERTEX_FORMAT_PNT?sizeof(wp_graphics_mesh_vertex_pnt):sizeof(wp_graphics_mesh_vertex_pntc)),"Unexpected morph vertex stride");
            const auto vertices=static_cast<const u8*>(wp_graphics_mesh_get_vertices(mesh));require(vertices!=nullptr,"Morph base has no vertex data");
            for(u32 i=0;i<count;++i) {
                float values[6];std::memcpy(values,vertices+size_t(i)*stride,sizeof(values));double normal=0;
                for(size_t j=0;j<6;++j)require(std::isfinite(values[j])&&std::abs(values[j])<=1000000,"Morph base positions/normals must be finite and bounded");
                for(size_t j=3;j<6;++j)normal+=double(values[j])*values[j];
                require(normal>1e-12,"Morph base contains a degenerate normal");
            }
            u64 topology=14695981039346656037ull,bind=14695981039346656037ull;
            word(topology,format);word(topology,count);word(topology,stride);word(topology,wp_graphics_mesh_get_primitive_type(mesh));
            const auto indices=wp_graphics_mesh_get_index_count(mesh);
            const auto indexFormat=wp_graphics_mesh_get_index_format(mesh);
            word(topology,indices);word(topology,indexFormat);
            const auto indexData=wp_graphics_mesh_get_indices(mesh);
            require(!indices||indexData,"Morph base has missing indices");
            if(indices)hash(topology,indexData,size_t(indices)*(indexFormat==WORKPHONE_INDEX_FORMAT_UINT16?2:4));
            const auto sections=wp_graphics_mesh_get_submesh_count(mesh);require(sections>=0&&sections<=128,"Morph base exceeds 128 submeshes");word(topology,u32(sections));
            for(s32 i=0;i<sections;++i){const auto section=wp_graphics_mesh_get_submesh(mesh,i);require(section!=nullptr,"Missing morph submesh");word(topology,section->index_start);word(topology,section->index_count);word(topology,section->material_id);}
            hash(bind,vertices,size_t(count)*stride);info.vertices=count;info.format=format;info.topologyHash=topology;info.bindHash=bind;return true;
        }catch(const std::exception &failure){error=failure.what();return false;}
    }
    bool MorphResourceCompiler::validateSamples(wp_graphics_mesh *mesh,const resource::MorphClipData &clip,String &error)
    {
        error.clear();
        try {
            require(mesh&&clip.info.vertices&&clip.info.vertices<=1000000&&clip.info.vertices==wp_graphics_mesh_get_vertex_count(mesh)&&clip.keys.size()>=2&&clip.keys.size()<=256&&!clip.targets.empty()&&clip.targets.size()<=16,"Morph sample validation requires its exact bounded base");
            size_t deltas=0;for(const auto &target:clip.targets){require(target.deltas.size()<=65536-deltas,"Morph sample validation exceeds sparse limits");deltas+=target.deltas.size();}require(deltas>0,"Morph requires sparse deltas");
            auto working=asset_budget::reserveCpuBytes(u64(deltas)*(sizeof(u32)+3*sizeof(std::array<double,6>))+u64(clip.info.vertices)*sizeof(u32)+4096,true);
            require(bool(working),"Morph semantic sample working budget exhausted");
            Array<u32> touched;touched.reserve(deltas);for(const auto &target:clip.targets)for(const auto &delta:target.deltas){require(delta.vertex<clip.info.vertices,"Morph delta exceeds base vertex count");touched.push_back(delta.vertex);}
            std::sort(touched.begin(),touched.end());touched.erase(std::unique(touched.begin(),touched.end()),touched.end());
            Array<u32> lookup(clip.info.vertices);Array<std::array<double,6>> base(touched.size()),previous(touched.size()),current(touched.size());
            const auto bytes=static_cast<const u8*>(wp_graphics_mesh_get_vertices(mesh));const auto stride=wp_graphics_mesh_get_vertex_stride(mesh);require(bytes&&stride>=24,"Missing native morph bind vertices");
            for(size_t i=0;i<touched.size();++i){lookup[touched[i]]=u32(i);float value[6];std::memcpy(value,bytes+size_t(touched[i])*stride,sizeof(value));for(size_t j=0;j<6;++j)base[i][j]=value[j];}
            bool first=true;
            for(const auto &key:clip.keys){require(key.weights.size()==clip.targets.size(),"Morph key target count mismatch");std::copy(base.begin(),base.end(),current.begin());
                for(size_t i=0;i<clip.targets.size();++i)for(const auto &delta:clip.targets[i].deltas){auto &out=current[lookup[delta.vertex]];const auto amount=key.weights[i];out[0]+=amount*double(delta.position.X());out[1]+=amount*double(delta.position.Y());out[2]+=amount*double(delta.position.Z());out[3]+=amount*double(delta.normal.X());out[4]+=amount*double(delta.normal.Y());out[5]+=amount*double(delta.normal.Z());}
                for(size_t i=0;i<current.size();++i){const auto &out=current[i];for(const auto number:out)require(std::isfinite(number)&&std::abs(number)<=1000000,"Morph key produces an unbounded native vertex");
                    double length=0;for(size_t j=3;j<6;++j)length+=out[j]*out[j];require(length>1e-12,"Morph key produces a degenerate native normal");
                    if(!first){double denominator=0,numerator=0;for(size_t j=3;j<6;++j){const auto delta=out[j]-previous[i][j];denominator+=delta*delta;numerator+=previous[i][j]*delta;}
                        const auto at=denominator>0?std::clamp(-numerator/denominator,0.0,1.0):0.0;double minimum=0;for(size_t j=3;j<6;++j){const auto value=previous[i][j]+at*(out[j]-previous[i][j]);minimum+=value*value;}require(minimum>1e-12,"Morph interpolation crosses a degenerate native normal");}}
                previous.swap(current);first=false;
            }return true;
        }catch(const std::exception &failure){error=failure.what();return false;}
    }
    resource::CompilationStatus MorphResourceCompiler::compile(const resource::CompileContext &context,std::ostream &output,Array<String> &messages) const
    {
        try {
            auto parsed=source(context);String error;const auto found=context.dependencyArtifacts.find(parsed.info.mesh.str());
            require(found!=context.dependencyArtifacts.end(),"Missing pinned morph mesh compile dependency");
            std::error_code failure;const auto bytes=std::filesystem::file_size(std::filesystem::u8path(found->second.c_str()),failure);
            require(!failure&&bytes>0&&bytes<=((72ull<<20)+(256ull<<10)),"Morph mesh dependency exceeds container limits");
            auto working=asset_budget::reserveCpuBytes(bytes+(32ull<<20),true);require(bool(working),"Morph dependency working budget exhausted");
            resource::RuntimeResource dependency;const auto read=resource::CompiledResourceIO::read(found->second,dependency,error,(64ull<<20)+(256ull<<10));require(read,error);
            require(dependency.header.resourceId==parsed.info.mesh,"Pinned morph mesh identity mismatch");
            CookedMeshData decoded;const auto valid=MeshResourceCompiler::decode(dependency,decoded,error);require(valid,error);
            require(decoded.target==context.target&&decoded.packagedBuild==context.packagedBuild,"Morph base target/build mode mismatch");
            auto native=std::unique_ptr<wp_graphics_mesh,decltype(&wp_graphics_mesh_destroy)>(wp_graphics_mesh_read_from_buffer_budgeted(decoded.geometry.data(),u32(decoded.geometry.size()),&asset_budget::meshMemoryCallbacks),wp_graphics_mesh_destroy);
            const auto signedMesh=signature(native.get(),parsed.info,error);require(signedMesh,error);
            parsed.info.target=context.target;parsed.info.packagedBuild=context.packagedBuild;
            const auto samples=validateSamples(native.get(),parsed,error);require(samples,error);
            const auto written=resource::MorphResourceCodec::write(parsed,output,error);require(written,error);
            return resource::CompilationStatus::Success;
        }catch(const std::exception &failure){messages.push_back(failure.what());return resource::CompilationStatus::Failure;}
    }
}
