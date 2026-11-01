#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/ParticleResource.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include "../../Workphone/Graphics/FoliageJsonBudget.hpp"
#include <cJSON.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace workphone::render
{
    namespace
    {
        constexpr char magic[8]={'W','P','P','A','R','T','0','1'};
        constexpr u64 maximumPayload=8192,maximumSource=65536;
        void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
        void validate(const CookedParticleData &data)
        {
            const auto &s=data.settings;
            require(data.capacity&&data.capacity<=WP_PARTICLE_SIMULATION_MAX_CAPACITY,"Particle capacity is outside [1,1000000]");
            const auto scalar=[](f32 value,f32 low,f32 high){return std::isfinite(value)&&value>=low&&value<=high;};
            require(scalar(s.rate,0,1e6f)&&scalar(s.lifetime_min,.000001f,86400)&&scalar(s.lifetime_max,s.lifetime_min,86400)&&
                scalar(s.size_min,0,1e6f)&&scalar(s.size_max,s.size_min,1e6f)&&scalar(s.duration,0,86400)&&(s.looping==0||s.looping==1),"Invalid typed particle emission/lifetime/size settings");
            for(const auto value:{s.velocity.x,s.velocity.y,s.velocity.z,s.gravity.x,s.gravity.y,s.gravity.z})require(scalar(value,-1e6f,1e6f),"Invalid typed particle vector");
            for(unsigned i=0;i<4;++i)require(scalar(s.color_start[i],0,1)&&scalar(s.color_end[i],0,1),"Invalid typed particle colour");
            require(!data.material.isValid()||(data.material.type()==resource::ResourceTypeID("matres")&&!data.material.isSubResource()),"Particle material must be a matres file resource");
        }
        double number(const cJSON *node,double low,double high)
        {require(cJSON_IsNumber(node)&&std::isfinite(node->valuedouble)&&node->valuedouble>=low&&node->valuedouble<=high,"Invalid typed particle numeric field");return node->valuedouble;}
        void vector(const cJSON *object,const char *name,f32 *output,unsigned count,double low,double high)
        {
            auto values=cJSON_GetObjectItemCaseSensitive(object,name);require(cJSON_IsArray(values)&&cJSON_GetArraySize(values)==int(count),"Invalid typed particle vector field");
            for(unsigned i=0;i<count;++i)output[i]=static_cast<f32>(number(cJSON_GetArrayItem(values,i),low,high));
        }
        CookedParticleData recipe(const resource::CompileContext &context)
        {
            require(context.resourceId.type()==resource::ResourceTypeID("partres")&&!context.resourceId.isSubResource(),"Particle compiler requires a partres file resource");
            namespace fs=std::filesystem;std::error_code errorCode;const auto file=fs::u8path(context.sourcePath.c_str());const auto bytes=fs::file_size(file,errorCode);
            require(!errorCode&&bytes&&bytes<=maximumSource,"Particle descriptor is missing, empty or exceeds 64 KiB");
            String error;auto scratch=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,1ull<<20,error);if(!scratch)throw std::runtime_error(error.c_str());
            std::string text(static_cast<size_t>(bytes),'\0');std::ifstream input(file,std::ios::binary);input.read(text.data(),text.size());
            require(input.good()&&input.peek()==std::char_traits<char>::eof(),"Particle descriptor changed while reading");
            require(text.find("\\u0000")==std::string::npos,"Particle descriptor strings cannot contain escaped NUL");
            require(detail::checkFoliageJsonBudget(text)<=128,"Particle descriptor exceeds its bounded semantic node count");
            using Json=std::unique_ptr<cJSON,decltype(&cJSON_Delete)>;const char *end=nullptr;
            Json object(cJSON_ParseWithLengthOpts(text.c_str(),text.size()+1,&end,1),cJSON_Delete);require(object&&cJSON_IsObject(object.get())&&end==text.c_str()+text.size(),"Invalid particle JSON descriptor");
            const std::set<std::string> fields{"version","capacity","seed","rate","lifetime","size","velocity","gravity","colourStart","colourEnd","duration","looping"};
            std::set<std::string> present;for(auto field=object->child;field;field=field->next){require(field->string&&present.emplace(field->string).second,"Duplicate typed particle field");require(fields.count(field->string)||std::string(field->string)=="material","Unsupported particle descriptor field");}
            for(const auto &field:fields)require(present.count(field)!=0,"Missing particle descriptor field");
            require(number(cJSON_GetObjectItemCaseSensitive(object.get(),"version"),1,1)==1,"Unsupported particle descriptor version");
            CookedParticleData data;const auto integer=[&](const char *name,double low,double high){const auto result=number(cJSON_GetObjectItemCaseSensitive(object.get(),name),low,high);require(std::floor(result)==result,"Particle integer must be integral");return static_cast<u32>(result);};
            data.capacity=integer("capacity",1,WP_PARTICLE_SIMULATION_MAX_CAPACITY);data.seed=integer("seed",0,4294967295.0);
            data.settings.rate=static_cast<f32>(number(cJSON_GetObjectItemCaseSensitive(object.get(),"rate"),0,1e6));
            f32 range[2];vector(object.get(),"lifetime",range,2,.000001,86400);data.settings.lifetime_min=range[0];data.settings.lifetime_max=range[1];
            vector(object.get(),"size",range,2,0,1e6);data.settings.size_min=range[0];data.settings.size_max=range[1];
            f32 position[3];vector(object.get(),"velocity",position,3,-1e6,1e6);data.settings.velocity={position[0],position[1],position[2]};
            vector(object.get(),"gravity",position,3,-1e6,1e6);data.settings.gravity={position[0],position[1],position[2]};
            vector(object.get(),"colourStart",data.settings.color_start,4,0,1);vector(object.get(),"colourEnd",data.settings.color_end,4,0,1);
            data.settings.duration=static_cast<f32>(number(cJSON_GetObjectItemCaseSensitive(object.get(),"duration"),0,86400));
            const auto looping=cJSON_GetObjectItemCaseSensitive(object.get(),"looping");require(cJSON_IsBool(looping),"Particle looping must be a boolean");data.settings.looping=cJSON_IsTrue(looping)?1:0;
            if(const auto material=cJSON_GetObjectItemCaseSensitive(object.get(),"material")){
                require(cJSON_IsString(material)&&material->valuestring&&std::strlen(material->valuestring)<=4096,"Invalid particle material resource ID");
                const String name=material->valuestring;require(name.size()>7&&name.substr(0,7)=="data://","Particle material must use data://");
                AssetCatalogPath canonical;if(!canonicalAssetCatalogPath(context.sourceRoot,name.substr(7),canonical,error))throw std::runtime_error(error.c_str());
                require(data.material.set("data://"+canonical.path)&&data.material.type()==resource::ResourceTypeID("matres"),"Particle material must be a matres file resource");
            }
            validate(data);return data;
        }
        void writeNumber(std::ostream &out,u64 value,unsigned count){for(unsigned i=0;i<count;++i)out.put(char(value>>(8*i)));}
        void writeFloat(std::ostream &out,f32 value){u32 bits;std::memcpy(&bits,&value,4);writeNumber(out,bits,4);}
        void writeString(std::ostream &out,const String &text){writeNumber(out,text.size(),4);out.write(text.data(),text.size());}
        struct Reader {
            const Array<u8> &bytes;size_t at=0;
            u64 number(unsigned count){require(at<=bytes.size()&&count<=bytes.size()-at,"Truncated cooked particle payload");u64 value=0;for(unsigned i=0;i<count;++i)value|=u64(bytes[at++])<<(i*8);return value;}
            f32 floating(){const auto bits=static_cast<u32>(number(4));f32 value;std::memcpy(&value,&bits,4);return value;}
            String string(size_t limit){const auto size=number(4);require(size&&size<=limit&&size<=bytes.size()-at,"Invalid cooked particle string extent");String result(reinterpret_cast<const char *>(bytes.data()+at),static_cast<size_t>(size));at+=size;require(result.find('\0')==String::npos,"NUL in cooked particle string");return result;}
        };
    }
    String ParticleResourceCompiler::name()const{return "WPGraphics.ParticleResourceCompiler";}
    Array<resource::CompilerOutput> ParticleResourceCompiler::outputs()const{return {{resource::ResourceTypeID("partres"),1}};}
    bool ParticleResourceCompiler::getDependencies(const resource::CompileContext &context,resource::DependencySet &dependencies,String &error)const
    {
        error.clear();try {const auto data=recipe(context);resource::DependencySet result;if(data.material.isValid()){result.compileDependencies.push_back(resource::CompileDependency::resource(data.material));result.installDependencies.push_back(data.material);}dependencies=std::move(result);return true;}catch(const std::exception &failure){error=failure.what();return false;}
    }
    resource::CompilationStatus ParticleResourceCompiler::compile(const resource::CompileContext &context,std::ostream &out,Array<String> &messages)const
    {
        try {
            const auto data=recipe(context);require(!context.target.empty()&&context.target.size()<=128&&context.target.find('\0')==String::npos,"Invalid typed particle target");
            resource::CompiledResourceHeader material;
            if(data.material.isValid()){
                String admissionError;auto scratch=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16ull<<20,admissionError);if(!scratch)throw std::runtime_error(admissionError.c_str());
                auto artifact=context.dependencyArtifacts.find(data.material.str());require(artifact!=context.dependencyArtifacts.end(),"Particle compiler requires a pinned material artifact");
                AssetCatalogPath canonical;String error;if(!canonicalAssetCatalogPath(context.compiledRoot,artifact->second,canonical,error))throw std::runtime_error(error.c_str());
                const auto path=std::filesystem::u8path(context.compiledRoot.c_str())/std::filesystem::u8path(canonical.path.c_str());
                resource::RuntimeResource dependency;if(!resource::CompiledResourceIO::read(path.generic_u8string().c_str(),dependency,error,graphicsMaterialShaderByteLimit+65536,1ull<<20))throw std::runtime_error(error.c_str());
                require(dependency.header.resourceId==data.material&&dependency.header.resourceType==resource::ResourceTypeID("matres")&&dependency.header.compilerVersion==GraphicsResourceCompiler().versionFor(resource::ResourceTypeID("matres")),"Invalid particle material artifact identity/version");material=std::move(dependency.header);
            }
            out.write(magic,8);writeNumber(out,1,4);writeString(out,context.target);writeNumber(out,context.packagedBuild?1:0,4);writeNumber(out,data.capacity,4);writeNumber(out,data.seed,4);
            const auto &s=data.settings;for(const auto value:{s.rate,s.lifetime_min,s.lifetime_max,s.size_min,s.size_max,s.velocity.x,s.velocity.y,s.velocity.z,s.gravity.x,s.gravity.y,s.gravity.z})writeFloat(out,value);
            for(unsigned i=0;i<4;++i)writeFloat(out,s.color_start[i]);for(unsigned i=0;i<4;++i)writeFloat(out,s.color_end[i]);writeFloat(out,s.duration);writeNumber(out,s.looping,4);
            writeNumber(out,data.material.isValid()?1:0,4);if(data.material.isValid()){writeString(out,data.material.str());writeNumber(out,material.compilerVersion,8);writeNumber(out,material.sourceHash,8);writeNumber(out,material.payloadHash,8);}
            require(out.good(),"Cannot write typed particle payload");return resource::CompilationStatus::Success;
        }catch(const std::exception &failure){messages.push_back(failure.what());return resource::CompilationStatus::Failure;}
    }
    bool decodeCookedParticle(const resource::RuntimeResource &value,CookedParticleData &output,String &error)
    {
        error.clear();try {
            require(value.header.isValid()&&!value.header.resourceId.isSubResource()&&value.header.resourceId.type()==resource::ResourceTypeID("partres")&&value.header.resourceType==resource::ResourceTypeID("partres")&&value.header.compilerVersion==ParticleResourceCompiler().versionFor(resource::ResourceTypeID("partres")),"Unsupported cooked particle header/version");
            require(value.payload.size()>=32&&value.payload.size()<=maximumPayload&&value.header.payloadSize==value.payload.size()&&value.header.payloadHash==resource::hashBytes(value.payload.data(),value.payload.size()),"Cooked particle integrity/size mismatch");
            require(!std::memcmp(value.payload.data(),magic,8),"Invalid cooked particle magic");Reader read{value.payload,8};require(read.number(4)==1,"Unsupported cooked particle payload version");
            CookedParticleData data;data.target=read.string(128);const auto mode=read.number(4);require(mode<=1,"Invalid cooked particle build mode");data.packagedBuild=mode!=0;data.capacity=static_cast<u32>(read.number(4));data.seed=static_cast<u32>(read.number(4));
            auto &s=data.settings;s.rate=read.floating();s.lifetime_min=read.floating();s.lifetime_max=read.floating();s.size_min=read.floating();s.size_max=read.floating();
            s.velocity.x=read.floating();s.velocity.y=read.floating();s.velocity.z=read.floating();s.gravity.x=read.floating();s.gravity.y=read.floating();s.gravity.z=read.floating();
            for(unsigned i=0;i<4;++i)s.color_start[i]=read.floating();for(unsigned i=0;i<4;++i)s.color_end[i]=read.floating();s.duration=read.floating();s.looping=static_cast<s32>(read.number(4));
            const auto count=read.number(4);require(count<=1&&value.dependencies.size()==count&&value.header.installDependencies.size()==count,"Cooked particle material closure mismatch");
            if(count){require(data.material.set(read.string(4096)),"Invalid cooked particle material identity");const auto &dependency=value.dependencies[0];
                require(dependency&&dependency->header.resourceId==data.material&&value.header.installDependencies[0]==data.material&&dependency->header.resourceType==resource::ResourceTypeID("matres")&&read.number(8)==dependency->header.compilerVersion&&read.number(8)==dependency->header.sourceHash&&read.number(8)==dependency->header.payloadHash,"Cooked particle material generation mismatch");
                if(value.header.formatVersion>=2)require(value.header.installArtifacts[0].sourceHash==dependency->header.sourceHash&&value.header.installArtifacts[0].payloadHash==dependency->header.payloadHash,"Cooked particle material container pin mismatch");
            }
            require(read.at==value.payload.size(),"Trailing cooked particle payload bytes");validate(data);output=std::move(data);return true;
        }catch(const std::exception &failure){error=failure.what();return false;}
    }
}
