#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/MorphResource.hpp>
#include <rapidjson/document.h>
#include <set>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <algorithm>

namespace workphone::resource
{
    namespace
    {
        void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
        void report(String &error,const char *message) noexcept
        {try{error=message;}catch(...){try{error="Morph resource failed";}catch(...){error.clear();}}}
        void name(const String &value,size_t limit=128)
        {require(!value.empty()&&value.size()<=limit,"Morph string exceeds bounds");for(unsigned char c:value)require(c>=32&&c<=126,"Morph strings require printable ASCII without NUL");}
        void fields(const rapidjson::Value &value,std::initializer_list<const char*> expected)
        {
            require(value.IsObject(),"Morph record must be an object");std::set<String> seen;
            for(auto i=value.MemberBegin();i!=value.MemberEnd();++i){require(!std::memchr(i->name.GetString(),0,i->name.GetStringLength()),"Morph field contains NUL");String key(i->name.GetString(),i->name.GetStringLength());bool known=false;for(auto label:expected)known|=key==label;require(known&&seen.insert(key).second,"Unknown or duplicate morph field");}
            for(auto key:expected)require(seen.count(key)!=0,"Missing morph field");
        }
        String text(const rapidjson::Value &value,size_t limit=128)
        {require(value.IsString()&&value.GetStringLength()<=limit,"Morph string has wrong type/length");String result(value.GetString(),value.GetStringLength());name(result,limit);return result;}
        f32 number(const rapidjson::Value &value,f32 limit=1000000)
        {require(value.IsNumber()&&std::isfinite(value.GetDouble())&&std::abs(value.GetDouble())<=limit,"Morph scalar must be finite and bounded");return f32(value.GetDouble());}
        Vector3F vector(const rapidjson::Value &value,f32 limit)
        {require(value.IsArray()&&value.Size()==3,"Morph delta requires three scalars");return {number(value[0],limit),number(value[1],limit),number(value[2],limit)};}
        void validate(const MorphClipData &value,bool compiled)
        {
            const auto &info=value.info;require(info.mesh.isValid()&&!info.mesh.isSubResource()&&info.mesh.type()==ResourceTypeID("mesh")&&info.mesh.str().size()<=1024,"Morph supports explicit static mesh ResourceIDs only; skinned bases are unsupported");name(info.clip);
            require(std::isfinite(info.duration)&&info.duration>0&&info.duration<=86400&&!value.targets.empty()&&value.targets.size()<=16&&value.keys.size()>=2&&value.keys.size()<=256,"Morph duration/target/key counts exceed bounds");
            if(compiled){name(info.target);require(info.vertices&&info.vertices<=1000000&&(info.format==3||info.format==4)&&info.topologyHash&&info.bindHash,"Morph native mesh signature is incomplete/unsupported");}
            std::set<String> ids;size_t total=0;
            for(const auto &target:value.targets){name(target.id);require(ids.insert(target.id).second&&!target.deltas.empty()&&target.deltas.size()<=65536-total,"Duplicate/empty morph target or excess sparse deltas");total+=target.deltas.size();u32 previous=0;bool first=true;
                for(const auto &delta:target.deltas){require((first||delta.vertex>previous)&&(!compiled||delta.vertex<info.vertices),"Morph vertices must be unique, ordered and within the pinned base mesh");first=false;previous=delta.vertex;
                    for(auto axis:{delta.position.X(),delta.position.Y(),delta.position.Z()})require(std::isfinite(axis)&&std::abs(axis)<=1000000,"Invalid morph position delta");for(auto axis:{delta.normal.X(),delta.normal.Y(),delta.normal.Z()})require(std::isfinite(axis)&&std::abs(axis)<=2,"Invalid morph normal delta");}}
            f32 previous=-1;
            for(const auto &key:value.keys){require(std::isfinite(key.time)&&key.time>=0&&key.time<=info.duration&&(previous<0||key.time-previous>std::numeric_limits<f32>::epsilon())&&key.weights.size()==value.targets.size(),"Morph keys require distinct ordered times and a complete weight vector");previous=key.time;
                for(auto weight:key.weights)require(std::isfinite(weight)&&weight>=0&&weight<=1,"Morph weights require [0,1]");}
            require(value.keys.front().time==0&&value.keys.back().time==info.duration,"Morph keys must cover exactly [0,duration]");
        }
        void word(std::ostream &out,u32 value){for(unsigned i=0;i<4;++i)out.put(char(value>>(i*8)));}
        void wide(std::ostream &out,u64 value){word(out,u32(value));word(out,u32(value>>32));}
        void scalar(std::ostream &out,f32 value){u32 bits;std::memcpy(&bits,&value,4);word(out,bits);}
        void string(std::ostream &out,const String &value){word(out,u32(value.size()));out.write(value.data(),value.size());}
        struct Reader
        {
            const Array<u8> &bytes;size_t at=0;
            u32 word(){require(at<=bytes.size()&&bytes.size()-at>=4,"Truncated morph payload");u32 value=0;for(unsigned i=0;i<4;++i)value|=u32(bytes[at++])<<(i*8);return value;}
            u64 wide(){const auto low=word();return u64(low)|(u64(word())<<32);}
            u32 count(u32 limit){const auto value=word();require(value&&value<=limit,"Morph count exceeds bound");return value;}
            f32 scalar(){const auto bits=word();f32 value;std::memcpy(&value,&bits,4);require(std::isfinite(value),"Nonfinite morph scalar");return value;}
            String string(size_t limit=128){const auto size=count(u32(limit));require(size<=bytes.size()-at,"Morph string exceeds its payload");String value(reinterpret_cast<const char*>(bytes.data()+at),size);at+=size;name(value,limit);return value;}
        };
        StreamBudget header(const RuntimeResource &value,String &error)
        {
            require(value.payload.size()>=8&&value.payload.size()<=MorphResourceCodec::maximumPayloadBytes&&value.header.resourceId.str().size()<=1024&&value.header.installDependencies.size()==1&&value.header.installArtifacts.size()<=1&&value.dependencies.size()==1,"Morph payload/dependency extent exceeds bounds");
            require(value.header.installDependencies[0].str().size()<=1024&&(value.header.installArtifacts.empty()||value.header.installArtifacts[0].path.size()<=4096),"Morph dependency identity exceeds bounds");
            auto work=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,4*value.payload.size()+65536,error);require(bool(work),"Morph decode working admission denied");
            require(value.header.isValid()&&value.header.resourceType==ResourceTypeID("amorph")&&value.header.resourceId.type()==ResourceTypeID("amorph")&&!value.header.resourceId.isSubResource()&&value.header.payloadSize==value.payload.size()&&hashBytes(value.payload.data(),value.payload.size())==value.header.payloadHash&&std::memcmp(value.payload.data(),"WPMO",4)==0,"Morph identity/payload integrity mismatch");return work;
        }
        MorphClipInfo info(Reader &in)
        {
            require(in.word()==1,"Unsupported morph encoding version");MorphClipInfo value;const auto mesh=in.string(1024);value.mesh=ResourceID(mesh);require(value.mesh.str()==mesh,"Morph mesh identity is not canonical");value.clip=in.string();value.target=in.string();const auto mode=in.word();require(mode<=1,"Invalid morph build mode");value.packagedBuild=mode!=0;value.duration=in.scalar();value.vertices=in.count(1000000);value.format=in.word();value.topologyHash=in.wide();value.bindHash=in.wide();
            require(value.mesh.type()==ResourceTypeID("mesh")&&!value.mesh.isSubResource()&&value.duration>0&&value.duration<=86400&&(value.format==3||value.format==4)&&value.topologyHash&&value.bindHash,"Invalid morph clip/native mesh metadata");return value;
        }
        void pins(const RuntimeResource &value,const MorphClipInfo &info)
        {
            const auto &child=value.dependencies[0];require(value.header.installDependencies[0]==info.mesh&&child&&child->header.resourceId==info.mesh&&child->header.resourceType==ResourceTypeID("mesh")&&child->header.isValid(),"Morph requires its exact pinned static mesh dependency");
            require(child->payload.empty()?bool(child->streamData):(child->payload.size()==child->header.payloadSize&&hashBytes(child->payload.data(),child->payload.size())==child->header.payloadHash),"Morph base payload is neither verified full data nor a captured streaming descriptor");
            if(value.header.formatVersion>=2){const auto &pin=value.header.installArtifacts[0];require(pin.sourceHash==child->header.sourceHash&&pin.payloadHash==child->header.payloadHash,"Morph base mesh generation differs from its installed pin");}
        }
    }
    bool MorphResourceCodec::parseSource(const String &source,MorphClipData &output,String &error)
    {
        error.clear();try{require(!source.empty()&&source.size()<=maximumSourceBytes&&source.find('\0')==String::npos,"Morph source extent/NUL invalid");auto work=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,16*source.size()+65536,error);require(bool(work),"Morph parser working admission denied");
            rapidjson::Document root;root.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseIterativeFlag>(source.data(),source.size());require(!root.HasParseError(),"Invalid morph JSON/UTF8");fields(root,{"version","mesh","clip","duration","targets","keys"});require(root["version"].IsUint()&&root["version"].GetUint()==1,"Unsupported morph source version");MorphClipData value;
            value.info.mesh=ResourceID(text(root["mesh"],1024));require(value.info.mesh.str()==text(root["mesh"],1024),"Morph mesh ResourceID must be canonical");value.info.clip=text(root["clip"]);value.info.duration=number(root["duration"],86400);
            require(root["targets"].IsArray()&&root["targets"].Size()<=16&&root["keys"].IsArray()&&root["keys"].Size()<=256,"Morph record count exceeds bounds");size_t total=0;
            for(const auto &entry:root["targets"].GetArray()){fields(entry,{"id","deltas"});MorphTarget target;target.id=text(entry["id"]);require(entry["deltas"].IsArray()&&entry["deltas"].Size()<=65536-total,"Morph sparse delta count exceeds bound");total+=entry["deltas"].Size();
                for(const auto &raw:entry["deltas"].GetArray()){fields(raw,{"vertex","position","normal"});require(raw["vertex"].IsUint(),"Morph vertex index requires unsigned integer");target.deltas.push_back({raw["vertex"].GetUint(),vector(raw["position"],1000000),vector(raw["normal"],2)});}value.targets.push_back(std::move(target));}
            for(const auto &entry:root["keys"].GetArray()){fields(entry,{"time","weights"});MorphWeightKey key;key.time=number(entry["time"],86400);require(entry["weights"].IsArray()&&entry["weights"].Size()<=16,"Morph weight vector exceeds bounds");for(const auto &weight:entry["weights"].GetArray())key.weights.push_back(number(weight,1));value.keys.push_back(std::move(key));}validate(value,false);output=std::move(value);return true;
        }catch(const std::exception &failure){report(error,failure.what());return false;}
    }
    bool MorphResourceCodec::write(const MorphClipData &value,std::ostream &out,String &error)
    {
        error.clear();try{validate(value,true);out.write("WPMO",4);word(out,1);const auto &info=value.info;string(out,info.mesh.str());string(out,info.clip);string(out,info.target);word(out,info.packagedBuild);scalar(out,info.duration);word(out,info.vertices);word(out,info.format);wide(out,info.topologyHash);wide(out,info.bindHash);
            word(out,u32(value.targets.size()));for(const auto &target:value.targets){string(out,target.id);word(out,u32(target.deltas.size()));for(const auto &delta:target.deltas){word(out,delta.vertex);for(auto axis:{delta.position.X(),delta.position.Y(),delta.position.Z(),delta.normal.X(),delta.normal.Y(),delta.normal.Z()})scalar(out,axis);}}
            word(out,u32(value.keys.size()));for(const auto &key:value.keys){scalar(out,key.time);word(out,u32(key.weights.size()));for(auto weight:key.weights)scalar(out,weight);}require(bool(out),"Cannot write morph payload");return true;
        }catch(const std::exception &failure){report(error,failure.what());return false;}
    }
    bool MorphResourceCodec::describe(const RuntimeResource &value,MorphClipInfo &output,String &error)
    {
        error.clear();try{auto work=header(value,error);Reader input{value.payload,4};auto candidate=info(input);pins(value,candidate);output=std::move(candidate);return true;}
        catch(const std::exception &failure){report(error,failure.what());return false;}
    }
    bool MorphResourceCodec::decode(const RuntimeResource &value,u64 version,MorphClipData &output,String &error)
    {
        error.clear();try{auto work=header(value,error);require(version&&value.header.compilerVersion==version,"Unsupported morph compiler ABI");Reader input{value.payload,4};MorphClipData candidate;candidate.admission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,2*value.payload.size()+65536,error);require(bool(candidate.admission),"Morph retained clip admission denied");candidate.info=info(input);pins(value,candidate.info);size_t total=0;auto size=input.count(16);
            for(u32 i=0;i<size;++i){MorphTarget target;target.id=input.string();const auto count=input.count(u32(65536-total));total+=count;for(u32 j=0;j<count;++j){MorphVertexDelta delta;delta.vertex=input.word();const auto x=input.scalar(),y=input.scalar(),z=input.scalar(),nx=input.scalar(),ny=input.scalar(),nz=input.scalar();delta.position={x,y,z};delta.normal={nx,ny,nz};target.deltas.push_back(delta);}candidate.targets.push_back(std::move(target));}
            size=input.count(256);for(u32 i=0;i<size;++i){MorphWeightKey key;key.time=input.scalar();const auto count=input.count(16);for(u32 j=0;j<count;++j)key.weights.push_back(input.scalar());candidate.keys.push_back(std::move(key));}require(input.at==value.payload.size(),"Trailing morph payload data");validate(candidate,true);output=std::move(candidate);return true;
        }catch(const std::exception &failure){report(error,failure.what());return false;}
    }
    bool MorphResourceCodec::weights(const MorphClipData &value,f32 time,Array<f32> &output,String &error)
    {
        error.clear();try{require(std::isfinite(time)&&std::isfinite(value.info.duration)&&value.info.duration>0&&value.keys.size()>=2&&value.keys.size()<=256&&!value.targets.empty()&&value.targets.size()<=16,"Invalid morph sample time/curve");time=std::clamp(time,0.0f,value.info.duration);size_t next=1;while(next+1<value.keys.size()&&value.keys[next].time<time)++next;const auto &a=value.keys[next-1],&b=value.keys[next];require(std::isfinite(a.time)&&std::isfinite(b.time)&&a.time<=time&&time<=b.time&&b.time-a.time>std::numeric_limits<f32>::epsilon()&&a.weights.size()==value.targets.size()&&b.weights.size()==value.targets.size(),"Invalid morph interpolation segment");const auto blend=(time-a.time)/(b.time-a.time);
            auto candidate=makeSourceBudgetSnapshot<f32>();candidate.reserve(value.targets.size());for(size_t i=0;i<value.targets.size();++i)candidate.push_back(a.weights[i]+(b.weights[i]-a.weights[i])*blend);output.swap(candidate);return true;
        }catch(const std::exception &failure){report(error,failure.what());return false;}
    }
}
